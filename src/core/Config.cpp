#include "Config.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <filesystem>

namespace fs = std::filesystem;

static std::wstring toWide(const QString &s) { return s.toStdWString(); }
static QString fromWide(const std::wstring &s) { return QString::fromStdWString(s); }

static bool looksLikeRoot(const fs::path &p) {
    std::error_code ec;
    return fs::is_regular_file(p / "stratactl.py", ec);
}

Config &Config::instance() {
    static Config c;
    return c;
}

QString Config::detectRoot() const {
    // 1) 环境变量优先
    for (const char *key : {"STRATA_PATH", "STRATA_DIR"}) {
        const QByteArray raw = qgetenv(key);
        if (raw.isEmpty()) continue;
        const fs::path p = fs::absolute(toWide(QString::fromLocal8Bit(raw).trimmed()));
        if (looksLikeRoot(p)) return fromWide(p.wstring());
    }
    // 2) 从 exe 所在目录向上递归：trayapp/release -> trayapp -> Strata
    fs::path dir = fs::absolute(toWide(QCoreApplication::applicationDirPath()));
    for (int i = 0; i < 8 && dir != dir.root_path(); ++i) {
        if (looksLikeRoot(dir)) return fromWide(dir.wstring());
        dir = dir.parent_path();
    }
    // 3) 当前工作目录 + 各磁盘根下的 Strata 文件夹
    const fs::path cwd = fs::current_path();
    if (looksLikeRoot(cwd)) return fromWide(cwd.wstring());
    for (const QFileInfo &drive : QDir::drives()) {
        const fs::path c = fs::path(toWide(drive.absoluteFilePath())) / "Strata";
        if (looksLikeRoot(c)) return fromWide(c.wstring());
    }
    return fromWide(cwd.wstring());   // 找不到也正常启动，按钮会给出错误提示
}

void Config::initialize() {
    m_root = detectRoot();
    m_ready = true;
}

QString Config::root() const { return m_ready ? m_root : QDir::currentPath(); }
QString Config::python() const { return root() + QStringLiteral("/.venv/Scripts/python.exe"); }
QString Config::ctl() const { return root() + QStringLiteral("/stratactl.py"); }
QString Config::updater() const { return root() + QStringLiteral("/update-local.py"); }
QString Config::logFile() const { return root() + QStringLiteral("/strata-server-bg.log"); }
QString Config::cacheFile() const { return root() + QStringLiteral("/.version-check.json"); }
QString Config::pidFile() const { return root() + QStringLiteral("/.strata-bg.pid"); }
QString Config::url() { return QStringLiteral("http://127.0.0.1:8080"); }
int Config::port() { return 8080; }

bool Config::autoRunEnabled() {
#ifdef Q_OS_WIN
    QSettings s(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                QSettings::NativeFormat);
    return s.contains(QStringLiteral("StrataTray"));
#else
    return false;   // 其它平台暂不提供开机自启，保留扩展点
#endif
}

void Config::setAutoRun(bool on) {
#ifdef Q_OS_WIN
    QSettings s(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                QSettings::NativeFormat);
    if (on) {
        const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        s.setValue(QStringLiteral("StrataTray"), QStringLiteral("\"%1\"").arg(exe));
    } else {
        s.remove(QStringLiteral("StrataTray"));
    }
    s.sync();
#else
    Q_UNUSED(on);
#endif
}
