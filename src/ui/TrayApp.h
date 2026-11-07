#pragma once
#include <QObject>
#include "core/ProcessProbe.h"
#include "core/UpdateService.h"

class QMenu;
class QAction;
class QSystemTrayIcon;
class QNetworkAccessManager;
class QTimer;
class QLocalServer;
class QLocalSocket;
class MainWindow;

// 托盘 + 调度层：连接 MainWindow（UI）与 Config / Probe / UpdateService（Core）
class TrayApp : public QObject {
    Q_OBJECT
public:
    explicit TrayApp(QObject *parent = nullptr);
    ~TrayApp() override;

    void showWindow();

    // IPC 单实例：已有实例则通知其显示窗口并返回 true
    static bool notifyExistingInstance();
    static QString ipcName();

public slots:
    void retranslateUi();

private:
    void setupIpc();
    void refreshHealth();
    void updateUi();
    void doStart();
    void doStop();
    void doOpen();
    void doUpdate();
    void checkVersion();
    void applyVersion(const VersionInfo &v, bool notifyNew);
    void loadVersionCache();
    void notify(const QString &title, const QString &body);
    QString stateText(State st) const;

    MainWindow *m_win = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_actStatus = nullptr, *m_actShow = nullptr, *m_actStart = nullptr;
    QAction *m_actStop = nullptr, *m_actOpen = nullptr, *m_actVersion = nullptr;
    QAction *m_actCheck = nullptr, *m_actUpdate = nullptr, *m_actAutoRun = nullptr;
    QAction *m_actLanguage = nullptr, *m_actQuit = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_pollTimer = nullptr, *m_versionTimer = nullptr;
    QLocalServer *m_ipc = nullptr;
    UpdateService *m_updates = nullptr;

    Snapshot m_local, m_cur;
    VersionInfo m_lastVersion;
    QString m_lastRemoteTag;
    bool m_versionBusy = false, m_updateBusy = false;
    bool m_hasVersion = false;
};
