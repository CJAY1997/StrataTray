#pragma once
#include <QJsonObject>
#include <QMetaType>
#include <QObject>
#include <QStringList>
#include <optional>

class QProcess;

// 版本信息与更新内容（由 update-local.py --check-json 提供）
struct VersionInfo {
    bool ok = false;
    QString localDesc, localTag, engine, remoteTag, remoteName;
    QString published, remoteEngine, url, checkedAt, error;
    int behind = -1;
    bool hasUpdate = false;
    QStringList highlights, commits, recent;
    QJsonObject raw;

    static VersionInfo fromJson(const QJsonObject &o);
};
Q_DECLARE_METATYPE(VersionInfo)

// 更新服务：只读版本检查 + 5 步更新流程 + 实时日志收集
class UpdateService : public QObject {
    Q_OBJECT
public:
    explicit UpdateService(QObject *parent = nullptr);

    void check();                         // --check-json
    void startUpdate();                   // 完整更新流程
    bool busy() const;

    static std::optional<VersionInfo> loadCache();

signals:
    void versionReady(const VersionInfo &info);
    void checkFailed(const QString &message);
    void updateLog(const QString &chunk, int step);
    void updateFinished(bool ok, int code);

private:
    QProcess *m_check = nullptr;
    QProcess *m_update = nullptr;
};
