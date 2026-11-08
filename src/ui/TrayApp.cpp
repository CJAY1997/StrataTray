#include "TrayApp.h"
#include "I18n.h"
#include "LanguageMenu.h"
#include "ThemeMenu.h"
#include "ThemeManager.h"
#include "MainWindow.h"
#include "Theme.h"
#include "core/Config.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QSignalBlocker>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtGui/QAction>
#include <QtGui/QDesktopServices>
#include <QtGui/QIcon>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSystemTrayIcon>

static inline QString s(const char *key) { return I18n::t(key); }

QString TrayApp::ipcName() { return QStringLiteral("StrataTray_SingleInstance"); }

bool TrayApp::notifyExistingInstance() {
    QLocalSocket probe;
    probe.connectToServer(ipcName());
    if (!probe.waitForConnected(200)) return false;
    probe.write("show");
    probe.flush();
    probe.waitForBytesWritten(200);
    probe.disconnectFromServer();
    return true;
}

TrayApp::TrayApp(QObject *parent) : QObject(parent) {
    m_win = new MainWindow();
    m_tray = new QSystemTrayIcon(Theme::appIcon(State::Stopped), this);
    m_tray->setToolTip(s("app_title"));

    m_menu = new QMenu();
    m_actStatus = m_menu->addAction(QString());
    m_actStatus->setEnabled(false);
    m_menu->addSeparator();
    m_actShow = m_menu->addAction(QString());
    m_menu->addSeparator();
    m_actStart = m_menu->addAction(QString());
    m_actStop = m_menu->addAction(QString());
    m_actOpen = m_menu->addAction(QString());
    m_menu->addSeparator();
    m_actVersion = m_menu->addAction(QString());
    m_actVersion->setEnabled(false);
    m_actCheck = m_menu->addAction(QString());
    m_actUpdate = m_menu->addAction(QString());
    m_menu->addSeparator();
    m_actAutoRun = m_menu->addAction(QString());
    m_actAutoRun->setCheckable(true);
    m_actAutoRun->setChecked(Config::autoRunEnabled());
    // 「界面语言」与「外观主题」二级子菜单，与齿轮菜单共用构建逻辑
    m_langMenu = LanguageMenu::create(m_menu, this);
    m_themeMenu = ThemeMenu::create(m_menu, this);
    m_actQuit = m_menu->addAction(QString());

    connect(m_actShow, &QAction::triggered, this, [this] { showWindow(); });
    connect(m_actStart, &QAction::triggered, this, [this] { doStart(); });
    connect(m_actStop, &QAction::triggered, this, [this] { doStop(); });
    connect(m_actOpen, &QAction::triggered, this, [this] { doOpen(); });
    connect(m_actCheck, &QAction::triggered, this, [this] { checkVersion(); });
    connect(m_actUpdate, &QAction::triggered, this, [this] { doUpdate(); });
    connect(m_actAutoRun, &QAction::toggled, this, [this](bool on) {
        Config::setAutoRun(on);
        m_win->setAutoRunChecked(on);   // 同步主窗口设置菜单的勾选状态
    });
    connect(m_actQuit, &QAction::triggered, qApp, &QCoreApplication::quit);

    connect(m_win, &MainWindow::startRequested, this, [this] { doStart(); });
    connect(m_win, &MainWindow::stopRequested, this, [this] { doStop(); });
    connect(m_win, &MainWindow::openRequested, this, [this] { doOpen(); });
    connect(m_win, &MainWindow::checkRequested, this, [this] { checkVersion(); });
    connect(m_win, &MainWindow::updateRequested, this, [this] { doUpdate(); });
    connect(m_win, &MainWindow::refreshRequested, this, [this] {
        m_local = Probe::probeLocal();
        refreshHealth();
    });
    // 设置菜单里改动开机自启时，反向同步托盘菜单（阻塞信号避免循环写注册表）
    connect(m_win, &MainWindow::autoRunChanged, this, [this](bool on) {
        QSignalBlocker blocker(m_actAutoRun);
        m_actAutoRun->setChecked(on);
    });

    m_tray->setContextMenu(m_menu);
    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::DoubleClick || r == QSystemTrayIcon::Trigger) showWindow();
    });
    m_tray->show();

    m_nam = new QNetworkAccessManager(this);
    m_updates = new UpdateService(this);
    connect(m_updates, &UpdateService::versionReady, this,
            [this](const VersionInfo &v) { applyVersion(v, true); });
    connect(m_updates, &UpdateService::checkFailed, this, [this] {
        m_versionBusy = false;
        m_actCheck->setEnabled(true);
        m_win->setVersionError(s("version_check_failed_click_recheck_to_re"));
        loadVersionCache();
    });
    connect(m_updates, &UpdateService::versionReady, this,
            [this](const VersionInfo &) { m_versionBusy = false; m_actCheck->setEnabled(true); });
    connect(m_updates, &UpdateService::updateLog, this,
            [this](const QString &chunk, int step) { m_win->appendUpdateLog(chunk, step); });
    connect(m_updates, &UpdateService::updateFinished, this,
            [this](bool ok, int code) {
        m_updateBusy = false;
        m_win->finishUpdate(ok, code);
        m_actUpdate->setText(s("update_to_latest"));
        m_actUpdate->setEnabled(true);
        notify(s("strata_update"),
               ok ? s("update_complete_verifying_version")
                  : s("update_failed_exit_n_see_update_local_lo_2").arg(code));
        m_local = Probe::probeLocal();
        refreshHealth();
        checkVersion();
    });

    m_local = Probe::probeLocal();
    refreshHealth();
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, [this] {
        m_local = Probe::probeLocal();
        refreshHealth();
    });
    m_pollTimer->start(2000);

    loadVersionCache();
    m_versionTimer = new QTimer(this);
    connect(m_versionTimer, &QTimer::timeout, this, [this] { checkVersion(); });
    m_versionTimer->start(30 * 60 * 1000);
    QTimer::singleShot(1200, this, [this] { checkVersion(); });

    connect(&I18n::instance(), &I18n::languageChanged, this, &TrayApp::retranslateUi);
    setupIpc();
    retranslateUi();
}

TrayApp::~TrayApp() {
    if (m_ipc) m_ipc->close();
    delete m_win;
}

void TrayApp::setupIpc() {
    QLocalServer::removeServer(ipcName());
    m_ipc = new QLocalServer(this);
    if (!m_ipc->listen(ipcName())) return;
    connect(m_ipc, &QLocalServer::newConnection, this, [this] {
        while (m_ipc->hasPendingConnections()) {
            QLocalSocket *c = m_ipc->nextPendingConnection();
            connect(c, &QLocalSocket::readyRead, c, [this, c] {
                c->readAll();
                showWindow();
            });
            connect(c, &QLocalSocket::disconnected, c, &QObject::deleteLater);
        }
    });
}

void TrayApp::showWindow() { m_win->showWindow(); }

void TrayApp::notify(const QString &title, const QString &body) {
    m_tray->showMessage(title, body, QSystemTrayIcon::Information, 3000);
}

void TrayApp::refreshHealth() {
    QNetworkRequest req(QUrl(Config::url() + QStringLiteral("/health")));
    req.setTransferTimeout(1500);
    QNetworkReply *rep = m_nam->get(req);
    connect(rep, &QNetworkReply::finished, this, [this, rep] {
        rep->deleteLater();
        Snapshot snap = m_local;
        snap.hasHealth = false;
        if (rep->error() == QNetworkReply::NoError) {
            const QJsonObject o = QJsonDocument::fromJson(rep->readAll()).object();
            snap.hasHealth = true;
            snap.model = o.value(QStringLiteral("model")).toString();
            snap.maxContext = o.value(QStringLiteral("max_context")).toInt();
            snap.loaded = o.value(QStringLiteral("loaded")).toBool();
        }
        if (snap.hasHealth && snap.loaded) snap.state = State::Ready;
        else if (snap.pid) snap.state = State::Starting;
        else if (snap.engine) snap.state = State::Orphan;
        else snap.state = State::Stopped;
        if (snap.pid) snap.uptime = Probe::procUptime(snap.pid);
        m_cur = snap;
        updateUi();
    });
}

QString TrayApp::stateText(State st) const {
    switch (st) {
    case State::Ready:    return s("running_ready_2");
    case State::Starting: return s("loading_model_3");
    case State::Orphan:   return s("error_no_listener_2");
    case State::Stopped:  return s("stopped_2");
    }
    return QString();
}

void TrayApp::updateUi() {
    const QString st = stateText(m_cur.state);
    m_actStatus->setText(st);
    m_tray->setToolTip(st);
    m_tray->setIcon(Theme::appIcon(m_cur.state));

    const bool busy = m_updateBusy;
    m_actStart->setEnabled(!busy && (m_cur.state == State::Stopped || m_cur.state == State::Orphan));
    m_actStop->setEnabled(!busy && m_cur.state != State::Stopped);
    m_actOpen->setEnabled(!busy && m_cur.state == State::Ready);
    m_actCheck->setEnabled(!busy && !m_versionBusy);
    m_actUpdate->setEnabled(!busy && m_cur.state == State::Stopped);
    m_actUpdate->setToolTip(m_cur.state == State::Stopped
        ? s("run_the_official_updater")
        : s("stop_the_service_before_updating"));
    m_win->applySnapshot(m_cur);
}

void TrayApp::doStart() {
    auto *p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p](int code, QProcess::ExitStatus) {
        p->deleteLater();
        if (code == 0) notify(QStringLiteral("Strata"),
                              s("starting_loading_model"));
        m_local = Probe::probeLocal();
        refreshHealth();
    });
    p->start(Config::instance().python(),
             {Config::instance().ctl(), QStringLiteral("start"), QStringLiteral("--no-open")});
}

void TrayApp::doStop() {
    auto *p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p](int, QProcess::ExitStatus) {
        p->deleteLater();
        notify(QStringLiteral("Strata"), s("stopped_vram_released"));
        m_local = Probe::probeLocal();
        refreshHealth();
    });
    p->start(Config::instance().python(), {Config::instance().ctl(), QStringLiteral("stop")});
}

void TrayApp::doOpen() { QDesktopServices::openUrl(QUrl(Config::url())); }

void TrayApp::checkVersion() {
    if (m_versionBusy || m_updates->busy()) return;
    m_versionBusy = true;
    m_win->setVersionChecking();
    m_actCheck->setEnabled(false);
    m_updates->check();
}

void TrayApp::applyVersion(const VersionInfo &v, bool notifyNew) {
    m_lastVersion = v;
    m_hasVersion = true;
    m_actVersion->setText(s("version_n_nn")
        .arg(v.localDesc, v.remoteTag,
             v.hasUpdate ? s("update_available_2")
                         : s("up_to_date_2")));
    m_win->setVersion(v);
    if (notifyNew && v.hasUpdate && !v.remoteTag.isEmpty()
        && v.remoteTag != m_lastRemoteTag) {
        notify(s("new_strata_release"),
               s("upstream_n_local_n_update_from_the_tray")
               .arg(v.remoteTag, v.localDesc));
    }
    if (!v.remoteTag.isEmpty()) m_lastRemoteTag = v.remoteTag;
}

void TrayApp::loadVersionCache() {
    const auto cached = UpdateService::loadCache();
    if (cached.has_value()) applyVersion(*cached, false);
}

void TrayApp::doUpdate() {
    if (m_updateBusy) return;
    if (m_cur.state != State::Stopped) {
        notify(s("strata_update"),
               s("stop_the_service_before_updating_2"));
        return;
    }
    m_updateBusy = true;
    m_actUpdate->setEnabled(false);
    m_actUpdate->setText(s("updating"));
    m_win->beginUpdate();
    m_updates->startUpdate();
}

void TrayApp::retranslateUi() {
    m_actStatus->setText(stateText(m_cur.state));
    m_actShow->setText(s("show_window"));
    m_actStart->setText(s("start_service"));
    m_actStop->setText(s("stop_service_free_vram"));
    m_actOpen->setText(s("open_webui"));
    m_actCheck->setText(s("check_for_updates"));
    m_actUpdate->setText(m_updateBusy ? s("updating")
                                      : s("update_to_latest"));
    m_actAutoRun->setText(s("run_at_startup"));
    LanguageMenu::retranslate(m_langMenu);
    ThemeMenu::retranslate(m_themeMenu);
    m_actQuit->setText(s("quit"));
    m_tray->setToolTip(stateText(m_cur.state));
    if (m_hasVersion) {
        m_actVersion->setText(s("version_n_nn")
            .arg(m_lastVersion.localDesc, m_lastVersion.remoteTag,
                 m_lastVersion.hasUpdate ? s("update_available_2")
                                        : s("up_to_date_2")));
    } else {
        m_actVersion->setText(s("version_checking"));
    }
    updateUi();
}
