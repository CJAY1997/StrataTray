#include "ProcessProbe.h"
#include "Config.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>
#include <algorithm>
#include <utility>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "iphlpapi.lib")
#endif
#endif

int GpuMem::percent() const {
    return (ok && totalGb > 0.0)
        ? std::min(100, int(usedGb / totalGb * 100.0 + 0.5)) : 0;
}

QString Probe::baseNameOf(const QString &path) { return QFileInfo(path).fileName(); }

bool Probe::procAlive(int pid) {
    if (pid <= 0) return false;
#ifdef Q_OS_WIN
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, DWORD(pid));
    if (!h) return false;
    DWORD code = 0;
    const BOOL ok = GetExitCodeProcess(h, &code);
    CloseHandle(h);
    return ok && code == STILL_ACTIVE;
#else
    Q_UNUSED(pid);
    return false;
#endif
}

int Probe::portOwner(int port) {
#ifdef Q_OS_WIN
    enum { kAfInet = 2 };   // AF_INET，避免 winsock2.h 与 Qt 头冲突
    DWORD size = 0;
    if (GetExtendedTcpTable(nullptr, &size, FALSE, kAfInet,
                            TCP_TABLE_OWNER_PID_LISTENER, 0) != ERROR_INSUFFICIENT_BUFFER)
        return 0;
    QByteArray buf(int(size), '\0');
    auto *table = reinterpret_cast<MIB_TCPTABLE_OWNER_PID *>(buf.data());
    if (GetExtendedTcpTable(table, &size, FALSE, kAfInet,
                            TCP_TABLE_OWNER_PID_LISTENER, 0) != NO_ERROR)
        return 0;
    for (DWORD i = 0; i < table->dwNumEntries; ++i) {
        const DWORD netPort = table->table[i].dwLocalPort;
        const int localPort = int(((netPort & 0xff) << 8) | ((netPort >> 8) & 0xff));
        if (localPort == port) return int(table->table[i].dwOwningPid);
    }
    return 0;
#else
    Q_UNUSED(port);
    return 0;
#endif
}

bool Probe::engineRunning() {
#ifdef Q_OS_WIN
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (QString::fromWCharArray(pe.szExeFile)
                    .compare(QStringLiteral("strata.exe"), Qt::CaseInsensitive) == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
#else
    return false;
#endif
}

int Probe::pidFromFile() {
    QFile f(Config::instance().pidFile());
    if (!f.open(QIODevice::ReadOnly)) return 0;
    return QJsonDocument::fromJson(f.readAll()).object().value(QStringLiteral("pid")).toInt();
}

QString Probe::procUptime(int pid) {
    if (pid <= 0) return QStringLiteral("—");
#ifdef Q_OS_WIN
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, DWORD(pid));
    if (!h) return QStringLiteral("—");
    FILETIME created, exited, kernel, user, now;
    const BOOL ok = GetProcessTimes(h, &created, &exited, &kernel, &user);
    CloseHandle(h);
    (void)exited; (void)kernel; (void)user;
    if (!ok) return QStringLiteral("—");
    GetSystemTimeAsFileTime(&now);
    ULARGE_INTEGER a, b;
    a.LowPart = created.dwLowDateTime; a.HighPart = created.dwHighDateTime;
    b.LowPart = now.dwLowDateTime;     b.HighPart = now.dwHighDateTime;
    if (b.QuadPart <= a.QuadPart) return QStringLiteral("0 秒");
    const quint64 secs = (b.QuadPart - a.QuadPart) / 10000000ULL;
    const auto [hh, rem] = std::pair<int, quint64>{int(secs / 3600), secs % 3600};
    const auto [mm, ss] = std::pair<int, quint64>{int(rem / 60), rem % 60};
    if (hh) return QStringLiteral("%1 小时 %2 分").arg(hh).arg(int(mm));
    if (mm) return QStringLiteral("%1 分 %2 秒").arg(int(mm)).arg(int(ss));
    return QStringLiteral("%1 秒").arg(int(ss));
#else
    return QStringLiteral("—");
#endif
}

std::optional<GpuMem> Probe::gpuMem() {
    QProcess p;
    p.start(QStringLiteral("nvidia-smi"),
            {QStringLiteral("--query-gpu=memory.used,memory.total"),
             QStringLiteral("--format=csv,noheader,nounits")});
    if (!p.waitForFinished(2000)) return std::nullopt;
    const QString line = QString::fromLocal8Bit(p.readAllStandardOutput()).split('\n').first();
    const QStringList f = line.simplified().split(',');
    if (f.size() < 2) return std::nullopt;
    bool ok1 = false, ok2 = false;
    const double used = f[0].trimmed().toDouble(&ok1), total = f[1].trimmed().toDouble(&ok2);
    if (!ok1 || !ok2 || total <= 0.0) return std::nullopt;
    GpuMem m;
    m.usedGb = used / 1024.0;
    m.totalGb = total / 1024.0;
    m.ok = true;
    return m;
}

Snapshot Probe::probeLocal() {
    Snapshot s;
    s.engine = engineRunning();
    const int owner = portOwner(Config::port());
    if (owner) s.pid = owner;
    else { const int g = pidFromFile(); if (procAlive(g)) s.pid = g; }
    return s;
}

QString Probe::tailLog(int lines) {
    QFile f(Config::instance().logFile());
    if (!f.open(QIODevice::ReadOnly)) return QStringLiteral("（无日志文件）");
    const qint64 size = f.size();
    constexpr qint64 limit = 64 * 1024;
    const bool tailOnly = size > limit;
    if (tailOnly) f.seek(size - limit);
    QString all = QString::fromUtf8(f.read(tailOnly ? limit : size));
    if (tailOnly) {
        const int nl = all.indexOf(QChar('\n'));
        if (nl < 0) return QString();
        all = all.mid(nl + 1);
    }
    const QStringList ls = all.split(QStringLiteral("\n"));
    QStringList keep;
    for (int i = ls.size() - 1; i >= 0 && keep.size() < lines; --i)
        if (!ls[i].trimmed().isEmpty()) keep.prepend(ls[i]);
    return keep.join(QStringLiteral("\n"));
}
