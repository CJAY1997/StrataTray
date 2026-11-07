#include "UpdateService.h"
#include "Config.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonValue>
#include <QtCore/QProcess>

static QString str(const QJsonObject &o, const char *key, const QString &fallback = QString()) {
    return o.value(QLatin1String(key)).toString(fallback);
}

static QStringList strList(const QJsonObject &o, const char *key) {
    QStringList out;
    for (const QJsonValue &v : o.value(QLatin1String(key)).toArray()) out << v.toString();
    return out;
}

VersionInfo VersionInfo::fromJson(const QJsonObject &o) {
    VersionInfo v;
    v.raw         = o;
    v.ok          = o.value(QLatin1String("ok")).toBool();
    v.localDesc   = str(o, "local_desc", QStringLiteral("?"));
    v.localTag    = str(o, "local_tag");
    v.engine      = str(o, "engine", QStringLiteral("?"));
    v.remoteTag   = str(o, "remote_tag", QStringLiteral("?"));
    v.remoteName  = str(o, "remote_name");
    v.published   = str(o, "published");
    v.remoteEngine= str(o, "remote_engine", QStringLiteral("—"));
    v.url         = str(o, "url");
    v.checkedAt   = str(o, "checked_at");
    v.error       = str(o, "error");
    v.behind      = o.value(QLatin1String("behind")).toInt(-1);
    v.hasUpdate   = o.value(QLatin1String("has_update")).toBool();
    v.highlights  = strList(o, "highlights");
    v.commits     = strList(o, "commits");
    v.recent      = strList(o, "recent");
    return v;
}

std::optional<VersionInfo> UpdateService::loadCache() {
    QFile f(Config::instance().cacheFile());
    if (!f.open(QIODevice::ReadOnly)) return std::nullopt;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return std::nullopt;
    return VersionInfo::fromJson(doc.object());
}

UpdateService::UpdateService(QObject *parent) : QObject(parent) {}

bool UpdateService::busy() const { return m_check != nullptr || m_update != nullptr; }

void UpdateService::check() {
    if (m_check) return;
    auto *proc = new QProcess(this);
    m_check = proc;
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError) {
        if (m_check != proc) return;
        m_check = nullptr;
        proc->deleteLater();
        emit checkFailed(QStringLiteral("failed to start the version checker"));
    });
    connect(proc, &QProcess::finished, this, [this, proc](int, QProcess::ExitStatus) {
        if (m_check != proc) return;
        const QByteArray raw = proc->readAllStandardOutput();
        proc->deleteLater();
        m_check = nullptr;
        const QJsonDocument doc = QJsonDocument::fromJson(raw);
        if (doc.isObject()) emit versionReady(VersionInfo::fromJson(doc.object()));
        else emit checkFailed(QStringLiteral("version check returned no JSON"));
    });
    proc->start(Config::instance().python(),
                {Config::instance().updater(), QStringLiteral("--check-json")});
}

void UpdateService::startUpdate() {
    if (m_update) return;
    auto *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    m_update = proc;
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc] {
        const QString chunk = QString::fromLocal8Bit(proc->readAllStandardOutput());
        int step = 0;
        for (int i = 1; i <= 5; ++i) {
            if (chunk.contains(QStringLiteral("[%1]").arg(i))) { step = i; break; }
        }
        emit updateLog(chunk, step);
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError) {
        if (m_update != proc) return;
        m_update = nullptr;
        proc->deleteLater();
        emit updateFinished(false, -1);
    });
    connect(proc, &QProcess::finished, this, [this, proc](int code, QProcess::ExitStatus) {
        if (m_update != proc) return;
        const QString tail = QString::fromLocal8Bit(proc->readAllStandardOutput());
        if (!tail.isEmpty()) emit updateLog(tail, 0);
        proc->deleteLater();
        m_update = nullptr;
        emit updateFinished(code == 0, code);
    });
    proc->start(Config::instance().python(), {Config::instance().updater()});
}
