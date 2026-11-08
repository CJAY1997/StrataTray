#include "MainWindow.h"
#include "I18n.h"
#include "LanguageMenu.h"
#include "ThemeMenu.h"
#include "ThemeManager.h"
#include "Theme.h"
#include "core/Config.h"

#include <QtCore/QTimer>
#include <QtCore/QPoint>
#include <QtCore/QSignalBlocker>
#include <QtCore/QUrl>
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QCloseEvent>
#include <QtGui/QCursor>
#include <QtGui/QDesktopServices>
#include <QtGui/QGuiApplication>
#include <QtGui/QKeySequence>
#include <QtGui/QShortcut>
#include <QtGui/QShowEvent>
#include <QtGui/QTextCharFormat>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dwmapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "dwmapi.lib")
#endif
#endif

#include <tuple>

static inline QString L(const char *key) { return I18n::t(key); }

// 调色板占位符渲染：@{token} -> 当前主题颜色（单次替换，不会链式覆盖）
static QString th(const QString &tpl) {
    const auto &p = ThemeManager::instance().palette();
    QString s = tpl;
    s.replace(QStringLiteral("@{surface}"), p.surface);
    s.replace(QStringLiteral("@{surface2}"), p.surface2);
    s.replace(QStringLiteral("@{border}"), p.border);
    s.replace(QStringLiteral("@{borderLight}"), p.borderLight);
    s.replace(QStringLiteral("@{text}"), p.text);
    s.replace(QStringLiteral("@{textStrong}"), p.textStrong);
    s.replace(QStringLiteral("@{muted}"), p.muted);
    s.replace(QStringLiteral("@{caption}"), p.caption);
    s.replace(QStringLiteral("@{codeBg}"), p.codeBg);
    s.replace(QStringLiteral("@{codeText}"), p.codeText);
    s.replace(QStringLiteral("@{okFg}"), p.okFg);
    s.replace(QStringLiteral("@{okBg}"), p.okBg);
    s.replace(QStringLiteral("@{okBorder}"), p.okBorder);
    s.replace(QStringLiteral("@{warnFg}"), p.warnFg);
    s.replace(QStringLiteral("@{warnBg}"), p.warnBg);
    s.replace(QStringLiteral("@{warnBorder}"), p.warnBorder);
    s.replace(QStringLiteral("@{errFg}"), p.errFg);
    s.replace(QStringLiteral("@{errBg}"), p.errBg);
    s.replace(QStringLiteral("@{errBorder}"), p.errBorder);
    s.replace(QStringLiteral("@{neutralFg}"), p.neutralFg);
    s.replace(QStringLiteral("@{neutralBg}"), p.neutralBg);
    s.replace(QStringLiteral("@{neutralBorder}"), p.neutralBorder);
    s.replace(QStringLiteral("@{dotOk}"), p.dotOk);
    s.replace(QStringLiteral("@{dotWarn}"), p.dotWarn);
    s.replace(QStringLiteral("@{dotErr}"), p.dotErr);
    s.replace(QStringLiteral("@{dotNeutral}"), p.dotNeutral);
    s.replace(QStringLiteral("@{hlBlue}"), p.hlBlue);
    s.replace(QStringLiteral("@{hlPurple}"), p.hlPurple);
    s.replace(QStringLiteral("@{dangerActiveBg}"), p.dangerActiveBg);
    s.replace(QStringLiteral("@{dangerActiveBorder}"), p.dangerActiveBorder);
    s.replace(QStringLiteral("@{onAccentText}"), p.onAccentText);
    return s;
}

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("rootWindow"));
    setWindowIcon(Theme::appIcon(State::Stopped));
    setMinimumSize(560, 440);
    resize(680, 540);
    buildUi();
    retranslateUi();
    refreshLog();
    applyTitleBarTheme();

    connect(&I18n::instance(), &I18n::languageChanged, this, &MainWindow::retranslateUi);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::refreshTheme);
    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::startRequested);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::stopRequested);
    connect(m_btnOpen, &QPushButton::clicked, this, &MainWindow::openRequested);
    connect(m_btnLog, &QPushButton::clicked, this, &MainWindow::refreshLog);
    connect(m_btnClearLog, &QPushButton::clicked, this, [this] { m_logView->clear(); });
    connect(m_btnOpenLog, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(Config::instance().logFile()));
    });
    connect(m_btnCopyBase, &QPushButton::clicked, this, [this] {
        copyUrl(Config::url(), m_btnCopyBase, L("copy_base"));
    });
    connect(m_btnCopyApi, &QPushButton::clicked, this, [this] {
        copyUrl(Config::url() + QStringLiteral("/v1"), m_btnCopyApi, L("copy_v1"));
    });
    connect(m_btnCheck, &QPushButton::clicked, this, &MainWindow::checkRequested);
    connect(m_btnRelease, &QPushButton::clicked, this, [this] {
        if (!m_releaseUrl.isEmpty()) QDesktopServices::openUrl(QUrl(m_releaseUrl));
    });
    connect(m_btnUpdate, &QPushButton::clicked, this, &MainWindow::updateRequested);

    auto *refreshShortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    refreshShortcut->setContext(Qt::WindowShortcut);
    connect(refreshShortcut, &QShortcut::activated, this, [this] {
        refreshLog();
        emit refreshRequested();
    });
    auto *hideShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    hideShortcut->setContext(Qt::WindowShortcut);
    connect(hideShortcut, &QShortcut::activated, this, [this] { hide(); });
}

void MainWindow::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(12);

    // 弹出菜单（沿用全局深色 QMenu 样式）；按钮本体在右卡片标题栏中创建
    m_settingsMenu = new QMenu(this);
    // 「界面语言」与「外观主题」两个二级子菜单，均由各自的注册表驱动
    m_langMenu = LanguageMenu::create(m_settingsMenu, this);
    m_themeMenu = ThemeMenu::create(m_settingsMenu, this);

    m_settingsMenu->addSeparator();
    m_actAutoRun = m_settingsMenu->addAction(QString());
    m_actAutoRun->setCheckable(true);
    m_actAutoRun->setChecked(Config::autoRunEnabled());   // 先设状态再 connect，避免初始化误写注册表
    connect(m_actAutoRun, &QAction::toggled, this, [this](bool on) {
        Config::setAutoRun(on);
        emit autoRunChanged(on);                          // 同步托盘菜单的勾选状态
    });

    m_actOpenDir = m_settingsMenu->addAction(QString());
    connect(m_actOpenDir, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(Config::instance().root()));
    });

    auto *cards = new QHBoxLayout();
    cards->setSpacing(12);

    // 左卡片：服务状态
    auto *svc = new QWidget(this);
    svc->setObjectName(QStringLiteral("card"));
    auto *sv = new QVBoxLayout(svc);
    sv->setContentsMargins(16, 14, 16, 14);
    sv->setSpacing(9);

    auto *titleRow = new QHBoxLayout();
    m_svcTitle = new QLabel(svc);
    m_svcTitle->setObjectName(QStringLiteral("cardTitle"));
    titleRow->addWidget(m_svcTitle);
    titleRow->addStretch(1);
    m_badgePill = new QWidget(svc);
    m_badgePill->setObjectName(QStringLiteral("badgePill"));
    auto *bp = new QHBoxLayout(m_badgePill);
    bp->setContentsMargins(10, 4, 12, 4);
    bp->setSpacing(7);
    m_stateDot = new QLabel(m_badgePill);
    m_stateDot->setObjectName(QStringLiteral("stateDot"));
    m_stateDot->setFixedSize(8, 8);
    m_stateBadge = new QLabel(m_badgePill);
    m_stateBadge->setObjectName(QStringLiteral("badgeText"));
    bp->addWidget(m_stateDot);
    bp->addWidget(m_stateBadge);
    titleRow->addWidget(m_badgePill);
    sv->addLayout(titleRow);

    auto *urlRow = new QHBoxLayout();
    m_urlCap = new QLabel(svc);
    m_urlCap->setObjectName(QStringLiteral("caption"));
    m_urlCap->setFixedWidth(46);
    m_urlValue = new QLabel(Config::url(), svc);
    m_urlValue->setObjectName(QStringLiteral("mono"));
    m_urlValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_btnCopyBase = new QPushButton(svc);
    m_btnCopyApi = new QPushButton(svc);
    m_btnCopyBase->setObjectName(QStringLiteral("btnMini"));
    m_btnCopyApi->setObjectName(QStringLiteral("btnMini"));
    urlRow->addWidget(m_urlCap);
    urlRow->addWidget(m_urlValue, 1);
    urlRow->addWidget(m_btnCopyBase);
    urlRow->addWidget(m_btnCopyApi);
    sv->addLayout(urlRow);

    m_info = new QLabel(svc);
    m_info->setObjectName(QStringLiteral("muted"));
    m_info->setTextFormat(Qt::RichText);
    m_info->setWordWrap(true);
    m_info->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    sv->addWidget(m_info);

    auto *vramRow = new QHBoxLayout();
    m_vramCap = new QLabel(svc);
    m_vramCap->setObjectName(QStringLiteral("caption"));
    m_vramCap->setFixedWidth(46);
    m_vramLabel = new QLabel(svc);
    m_vramLabel->setObjectName(QStringLiteral("muted"));
    m_vramPct = new QLabel(svc);
    m_vramPct->setObjectName(QStringLiteral("caption"));
    vramRow->addWidget(m_vramCap);
    vramRow->addWidget(m_vramLabel);
    vramRow->addStretch(1);
    vramRow->addWidget(m_vramPct);
    sv->addLayout(vramRow);
    m_vramBar = new QProgressBar(svc);
    m_vramBar->setObjectName(QStringLiteral("vramBar"));
    m_vramBar->setRange(0, 100);
    m_vramBar->setTextVisible(false);
    m_vramBar->setFixedHeight(6);
    sv->addWidget(m_vramBar);
    sv->addStretch(1);

    // 右卡片：版本与更新
    auto *ver = new QWidget(this);
    ver->setObjectName(QStringLiteral("card"));
    auto *vv = new QVBoxLayout(ver);
    vv->setContentsMargins(16, 14, 16, 14);
    vv->setSpacing(9);
    auto *verTitleRow = new QHBoxLayout();
    verTitleRow->setSpacing(6);
    m_verTitle = new QLabel(ver);
    m_verTitle->setObjectName(QStringLiteral("cardTitle"));
    m_versionBadge = new QLabel(ver);
    m_versionBadge->setObjectName(QStringLiteral("verBadge"));

    // 设置按钮嵌在标题栏最右端：专属样式清零 padding，避免 ⚙ 被挤压裁切
    m_btnSettings = new QPushButton(QString::fromUtf8("⚙"), ver);
    m_btnSettings->setObjectName(QStringLiteral("btnSettings"));
    m_btnSettings->setFixedSize(24, 22);
    m_btnSettings->setCursor(QCursor(Qt::PointingHandCursor));
    applySettingsButtonStyle();   // 颜色随主题变化，切换时由 refreshTheme() 重新应用

    verTitleRow->addWidget(m_verTitle);
    verTitleRow->addStretch(1);
    verTitleRow->addWidget(m_versionBadge);
    verTitleRow->addWidget(m_btnSettings);
    vv->addLayout(verTitleRow);

    connect(m_btnSettings, &QPushButton::clicked, this, [this] {
        // 在按钮正下方弹出，避免遮住卡片内容
        m_settingsMenu->exec(m_btnSettings->mapToGlobal(
            QPoint(0, m_btnSettings->height() + 4)));
    });

    m_localInfo = new QLabel(ver);
    m_remoteInfo = new QLabel(ver);
    for (QLabel *l : {m_localInfo, m_remoteInfo}) {
        l->setObjectName(QStringLiteral("muted"));
        l->setTextFormat(Qt::RichText);
        l->setWordWrap(true);
        l->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    }
    vv->addWidget(m_localInfo);
    vv->addWidget(m_remoteInfo);

    auto *hr = new QFrame(ver);
    hr->setObjectName(QStringLiteral("hr"));
    hr->setFrameShape(QFrame::HLine);
    hr->setFixedHeight(1);
    vv->addWidget(hr);

    m_updateState = new QLabel(ver);
    m_updateState->setObjectName(QStringLiteral("caption"));
    m_updateState->setWordWrap(true);
    m_updateState->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_progressBar = new QProgressBar(ver);
    m_progressBar->setObjectName(QStringLiteral("updateBar"));
    m_progressBar->setRange(0, 5);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->hide();
    vv->addWidget(m_updateState);
    vv->addWidget(m_progressBar);
    vv->addStretch(1);

    cards->addWidget(svc, 3);
    cards->addWidget(ver, 2);
    root->addLayout(cards);

    // 操作栏
    auto *actions = new QHBoxLayout();
    actions->setSpacing(10);
    m_btnStart = new QPushButton(this);
    m_btnStop = new QPushButton(this);
    m_btnOpen = new QPushButton(this);
    m_btnStart->setObjectName(QStringLiteral("btnPrimary"));
    m_btnStop->setObjectName(QStringLiteral("btnDanger"));
    m_btnOpen->setObjectName(QStringLiteral("btnGhost"));
    m_btnStart->setMinimumHeight(36);
    m_btnStop->setMinimumHeight(36);
    m_btnOpen->setMinimumHeight(36);
    actions->addWidget(m_btnStart, 2);
    actions->addWidget(m_btnStop, 2);
    actions->addWidget(m_btnOpen, 1);
    root->addLayout(actions);

    // Tab
    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName(QStringLiteral("mainTabs"));
    m_tabs->setDocumentMode(true);

    m_logPage = new QWidget(m_tabs);
    auto *lg = new QVBoxLayout(m_logPage);
    lg->setContentsMargins(0, 8, 0, 0);
    lg->setSpacing(8);
    auto *logTools = new QHBoxLayout();
    m_btnLog = new QPushButton(m_logPage);
    m_btnOpenLog = new QPushButton(m_logPage);
    m_btnClearLog = new QPushButton(m_logPage);
    m_btnLog->setObjectName(QStringLiteral("btnMini"));
    m_btnOpenLog->setObjectName(QStringLiteral("btnMini"));
    m_btnClearLog->setObjectName(QStringLiteral("btnMini"));
    m_logAutoScroll = new QCheckBox(m_logPage);
    m_logAutoScroll->setChecked(true);
    logTools->addWidget(m_btnLog);
    logTools->addWidget(m_btnOpenLog);
    logTools->addWidget(m_btnClearLog);
    logTools->addStretch(1);
    logTools->addWidget(m_logAutoScroll);
    lg->addLayout(logTools);
    m_logView = new QTextEdit(m_logPage);
    m_logView->setObjectName(QStringLiteral("logView"));
    m_logView->setReadOnly(true);
    m_logView->setLineWrapMode(QTextEdit::WidgetWidth);
    m_logView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    lg->addWidget(m_logView, 1);
    m_tabs->addTab(m_logPage, QString());

    m_verPage = new QWidget(m_tabs);
    auto *vg = new QVBoxLayout(m_verPage);
    vg->setContentsMargins(0, 8, 0, 0);
    vg->setSpacing(8);
    m_versionView = new QTextEdit(m_verPage);
    m_versionView->setObjectName(QStringLiteral("versionView"));
    m_versionView->setReadOnly(true);
    m_versionView->setLineWrapMode(QTextEdit::WidgetWidth);
    m_versionView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    vg->addWidget(m_versionView, 1);
    auto *vbtn = new QHBoxLayout();
    m_btnCheck = new QPushButton(m_verPage);
    m_btnRelease = new QPushButton(m_verPage);
    m_btnUpdate = new QPushButton(m_verPage);
    m_btnCheck->setObjectName(QStringLiteral("btnGhost"));
    m_btnRelease->setObjectName(QStringLiteral("btnGhost"));
    m_btnUpdate->setObjectName(QStringLiteral("btnAccent"));
    vbtn->addWidget(m_btnCheck);
    vbtn->addWidget(m_btnRelease);
    vbtn->addStretch(1);
    vbtn->addWidget(m_btnUpdate);
    vg->addLayout(vbtn);
    m_tabs->addTab(m_verPage, QString());
    root->addWidget(m_tabs, 1);

    m_progressTimer = new QTimer(this);
    connect(m_progressTimer, &QTimer::timeout, this, &MainWindow::refreshProgressLabel);
}

// 齿轮按钮的内联样式随主题刷新（透明底、悬停微亮，且不受全局 padding 挤压）
void MainWindow::applySettingsButtonStyle() {
    const auto &p = ThemeManager::instance().palette();
    m_btnSettings->setStyleSheet(QStringLiteral(
        "QPushButton#btnSettings{background:transparent;border:1px solid transparent;"
        "border-radius:4px;color:%1;font-size:11pt;padding:0px;}"
        "QPushButton#btnSettings:hover{background:%2;border-color:%3;color:%4;}")
        .arg(p.muted).arg(p.surface2).arg(p.border).arg(p.textStrong));
}

// 主题切换：DWM 标题栏、按钮内联样式、所有徽章 / 富文本 / 日志着色一起热重载
void MainWindow::refreshTheme() {
    applySettingsButtonStyle();
    applyTitleBarTheme();
    retranslateUi();
    refreshLog();
    update();
}

void MainWindow::applyTitleBarTheme() {
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL darkMode = ThemeManager::instance().isDark() ? TRUE : FALSE;
    // 19 = 早期 Win10 的 DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
    // 20 = Win10 20H1+ / Win11 的 DWMWA_USE_IMMERSIVE_DARK_MODE；两者都设以保证全版本兼容
    // 新版 Windows 会拒绝 19（E_INVALIDARG），忽略返回值即可，20 始终有效
    DwmSetWindowAttribute(hwnd, 19, &darkMode, sizeof(darkMode));
    DwmSetWindowAttribute(hwnd, 20, &darkMode, sizeof(darkMode));
    int corner = 2;   // DWMWCP_ROUND（Win11 原生圆角）
    DwmSetWindowAttribute(hwnd, 33, &corner, sizeof(corner));
    // 关键：通知 DWM 非客户区边框帧已变更，强制立即重算标题栏，消除延迟变色
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
#endif
}

void MainWindow::showEvent(QShowEvent *e) {
    QWidget::showEvent(e);
    applyTitleBarTheme();
}

void MainWindow::closeEvent(QCloseEvent *e) {
    e->ignore();
    hide();
}

void MainWindow::setAutoRunChecked(bool on) {
    if (!m_actAutoRun) return;
    QSignalBlocker blocker(m_actAutoRun);
    m_actAutoRun->setChecked(on);
}

void MainWindow::showWindow() {
    show();
    raise();
    activateWindow();
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
#endif
}

void MainWindow::copyUrl(const QString &text, QPushButton *btn, const QString &label) {
    QGuiApplication::clipboard()->setText(text);
    btn->setText(L("copied"));
    btn->setStyleSheet(th(QStringLiteral("QPushButton#btnMini{background:@{okBg};color:@{okFg};border-color:@{okBorder};}")));
    QTimer::singleShot(1500, this, [this, btn, label] {
        btn->setText(label);
        btn->setStyleSheet(QString());
    });
}

void MainWindow::refreshLog() {
    const QString text = Probe::tailLog(200);
    m_logView->setUpdatesEnabled(false);
    m_logView->clear();
    QTextCursor cur = m_logView->textCursor();
    for (const QString &line : text.split(QStringLiteral("\n"))) {
        QTextCharFormat fmt;
        if (line.contains(QStringLiteral("[ERROR]"), Qt::CaseInsensitive)
            || line.contains(QStringLiteral("error"), Qt::CaseInsensitive)
            || line.contains(QStringLiteral("失败"))) {
            fmt.setForeground(QColor(th(QStringLiteral("@{errFg}"))));
        } else if (line.contains(QStringLiteral("warn"), Qt::CaseInsensitive)) {
            fmt.setForeground(QColor(th(QStringLiteral("@{warnFg}"))));
        } else {
            fmt.setForeground(QColor(th(QStringLiteral("@{codeText}"))));
        }
        cur.setCharFormat(fmt);
        cur.insertText(line + QStringLiteral("\n"));
    }
    m_logView->setUpdatesEnabled(true);
    if (m_logAutoScroll->isChecked()) m_logView->moveCursor(QTextCursor::End);
}

void MainWindow::setVersion(const VersionInfo &v) {
    m_lastVersion = v;
    m_hasVersion = true;
    m_hasUpdate = v.hasUpdate;
    m_releaseUrl = v.url;

    setVersionBadge(v.hasUpdate ? L("update_available")
                                : L("up_to_date"),
                    v.hasUpdate ? th(QStringLiteral("@{warnFg}")) : th(QStringLiteral("@{okFg}")),
                    v.hasUpdate ? th(QStringLiteral("@{warnBg}")) : th(QStringLiteral("@{okBg}")),
                    v.hasUpdate ? th(QStringLiteral("@{warnBorder}")) : th(QStringLiteral("@{okBorder}")));
    m_versionBadge->setToolTip(v.remoteName);

    const QString gap = v.behind > 0
        ? L("n_commits_behind").arg(v.behind)
        : L("none");
    m_localInfo->setText(th(QStringLiteral("<div style='line-height:1.6'><span style='color:@{caption}'>%1</span>&nbsp;&nbsp;<b style='color:@{text}'>%2</b><br><span style='color:@{caption}'>%3</span>&nbsp;&nbsp;%4 · %5 %6</div>"))
        .arg(L("local"), v.localDesc.toHtmlEscaped(), L("engine"),
             v.engine, L("commit"), v.localTag.toHtmlEscaped()));
    m_remoteInfo->setText(th(QStringLiteral("<div style='line-height:1.6'><span style='color:@{caption}'>%1</span>&nbsp;&nbsp;<b style='color:@{text}'>%2</b><br><span style='color:@{caption}'>%3</span>&nbsp;&nbsp;%4 · %5 %6<br><span style='color:@{caption}'>%7</span>&nbsp;&nbsp;%8</div>"))
        .arg(L("upstream"), v.remoteTag.toHtmlEscaped(), L("released"),
             v.published.toHtmlEscaped(), L("engine"), v.remoteEngine,
             L("gap"), gap));

    if (!m_updateMode) {
        if (v.hasUpdate) {
            m_updateState->setText(L("new_release_available_click_update_now_b"));
            m_updateState->setStyleSheet(th(QStringLiteral("QLabel{color:@{warnFg};font-size:9pt;}")));
        } else {
            const QString hhmm = v.checkedAt.length() >= 16
                ? v.checkedAt.mid(11, 5) : QStringLiteral("--:--");
            m_updateState->setText(L("up_to_date_checked_at_n").arg(hhmm));
            m_updateState->setStyleSheet(th(QStringLiteral("QLabel{color:@{caption};font-size:9pt;}")));
        }
        m_progressBar->hide();
        m_hasResult = false;
    }
    renderVersionText(v);
    m_btnRelease->setEnabled(!m_releaseUrl.isEmpty());
    m_btnCheck->setEnabled(!m_updateMode);
    refreshActionButtons();
}

void MainWindow::renderVersionText(const VersionInfo &v) {
    if (m_updateMode) return;
    QString html = th(QStringLiteral("<div style='color:@{text}'>"));
    html += th(QStringLiteral("<h3 style='color:@{hlBlue};font-size:11pt;margin:0 0 8px 0;'>%1</h3>"))
            .arg(L("release_highlights"));
    if (v.highlights.isEmpty()) {
        html += th(QStringLiteral("<p style='color:@{caption};margin:0;'>%1</p>"))
                .arg(L("no_highlights_provided"));
    } else {
        html += th(QStringLiteral("<ul style='margin:0 0 12px 18px;line-height:1.65;color:@{codeText};'>"));
        for (const QString &h : v.highlights)
            html += QStringLiteral("<li>%1</li>").arg(h.toHtmlEscaped());
        html += QStringLiteral("</ul>");
    }
    html += th(QStringLiteral("<p style='color:@{caption};font-size:9pt;margin:0 0 12px 0;'>%1 %2"))
            .arg(L("checked_at"), v.checkedAt.toHtmlEscaped());
    if (v.behind >= 0)
        html += QStringLiteral(" · %1 %2").arg(L("commits_behind")).arg(v.behind);
    if (!v.error.isEmpty())
        html += th(QStringLiteral("<br><span style='color:@{warnFg}'>%1</span>")).arg(v.error.toHtmlEscaped());
    html += QStringLiteral("</p>");

    const QStringList &list = !v.commits.isEmpty() ? v.commits : v.recent;
    if (!list.isEmpty()) {
        html += th(QStringLiteral("<h3 style='color:@{hlPurple};font-size:11pt;margin:4px 0 8px 0;'>%1</h3>"))
                .arg(!v.commits.isEmpty()
                     ? L("pending_commits_latest_10")
                     : L("recent_commits"));
        html += th(QStringLiteral("<div style='font-family:Consolas,monospace;font-size:9pt;color:@{muted};line-height:1.6;'>"));
        for (const QString &c : list) html += QStringLiteral("%1<br>").arg(c.toHtmlEscaped());
        html += QStringLiteral("</div>");
    }
    html += QStringLiteral("</div>");
    m_versionView->setHtml(html);
    m_versionView->moveCursor(QTextCursor::Start);
}

void MainWindow::beginUpdate() {
    m_updateMode = true;
    m_hasResult = false;
    m_stageNum = 0;
    m_stageName = L("starting");
    m_btnCheck->setEnabled(false);
    refreshActionButtons();
    m_updateState->setStyleSheet(th(QStringLiteral("QLabel{color:@{warnFg};font-size:9pt;font-weight:600;}")));
    m_progressBar->setStyleSheet(QString());
    m_progressBar->setValue(0);
    m_progressBar->show();
    m_versionView->setPlainText(L("update_log_live"));
    m_elapsed.start();
    m_progressTimer->start(1000);
    refreshProgressLabel();
}

void MainWindow::appendUpdateLog(const QString &chunk, int step) {
    if (chunk.isEmpty()) return;
    if (step > 0) {
        static const char *stageKeys[] = {"", "stage_stop", "stage_pull",
                                         "stage_engine", "stage_reapply", "stage_done"};
        m_stageNum = step;
        m_stageName = L(stageKeys[step]);
        m_progressBar->setValue(step);
    }
    const QString all = m_versionView->toPlainText() + chunk;
    const QStringList ls = all.split(QStringLiteral("\n"));
    m_versionView->setPlainText(ls.mid(qMax(0, ls.size() - 300)).join(QStringLiteral("\n")));
    m_versionView->moveCursor(QTextCursor::End);
    refreshProgressLabel();
}

void MainWindow::finishUpdate(bool ok, int code) {
    m_progressTimer->stop();
    m_progressBar->hide();
    m_btnCheck->setEnabled(true);
    m_hasResult = true;
    m_resultOk = ok;
    m_resultCode = code;
    m_updateMode = false;
    m_serviceStopped = true;
    m_serviceReady = false;
    m_serviceOrphan = false;
    m_serviceStarting = false;
    renderUpdateResult();
    m_versionView->append(ok
        ? L("update_complete_exit_0")
        : L("update_failed_exit_n").arg(code));
    m_versionView->moveCursor(QTextCursor::End);
    refreshActionButtons();
}

void MainWindow::renderUpdateResult() {
    if (!m_hasResult) return;
    if (m_resultOk) {
        m_progressBar->setValue(5);
        m_progressBar->setStyleSheet(th(QStringLiteral("QProgressBar#updateBar::chunk{background:@{dotOk};}")));
        m_updateState->setText(L("update_complete_in_n_verifying").arg(elapsedText()));
        m_updateState->setStyleSheet(th(QStringLiteral("QLabel{color:@{okFg};font-size:9pt;font-weight:600;}")));
    } else {
        m_updateState->setText(L("update_failed_exit_n_see_update_local_lo").arg(m_resultCode));
        m_updateState->setStyleSheet(th(QStringLiteral("QLabel{color:@{errFg};font-size:9pt;font-weight:600;}")));
    }
}

void MainWindow::refreshProgressLabel() {
    if (!m_updateMode || !m_elapsed.isValid()) return;
    m_updateState->setText(L("updating_step_n_5_n_elapsed_n")
                           .arg(m_stageNum).arg(m_stageName).arg(elapsedText()));
}

QString MainWindow::elapsedText() const {
    const qint64 sec = m_elapsed.isValid() ? m_elapsed.elapsed() / 1000 : 0;
    return QStringLiteral("%1:%2").arg(sec / 60, 2, 10, QChar('0'))
                                  .arg(sec % 60, 2, 10, QChar('0'));
}

void MainWindow::refreshActionButtons() {
    // 文案与「启用/样式」分离：无论状态怎么切，两个按钮的文字都不会消失
    m_btnStop->setText(L("stop_service_free_vram"));
    m_btnOpen->setText(L("open_webui"));

    if (m_updateMode) {
        m_btnStart->setText(L("start_service"));
        m_btnStart->setEnabled(false);
        m_btnStop->setEnabled(false);
        m_btnStop->setStyleSheet(QString());
        m_btnOpen->setEnabled(false);
        m_btnUpdate->setEnabled(false);
        m_btnUpdate->setText(L("updating"));
        m_btnUpdate->setToolTip(L("update_in_progress_service_controls_are"));
        return;
    }
    if (m_serviceStarting) {
        m_btnStart->setText(L("loading_model"));
        m_btnStart->setEnabled(false);
    } else if (m_serviceReady) {
        m_btnStart->setText(L("running"));
        m_btnStart->setEnabled(false);
    } else {
        m_btnStart->setText(L("start_service"));
        m_btnStart->setEnabled(m_serviceStopped || m_serviceOrphan);
    }
    m_btnStop->setEnabled(!m_serviceStopped);
    if (m_serviceReady) {
        m_btnStop->setStyleSheet(th(QStringLiteral("QPushButton#btnDanger{background:@{dangerActiveBg};border-color:@{dotErr};color:@{onAccentText};}")));
        m_btnStop->setToolTip(L("stop_the_service_and_free_vram"));
    } else {
        m_btnStop->setStyleSheet(QString());
        m_btnStop->setToolTip(m_serviceOrphan
            ? L("engine_running_but_port_not_responding_c")
            : L("stop_the_service_and_free_vram_2"));
    }
    m_btnOpen->setEnabled(m_serviceReady);
    m_btnUpdate->setEnabled(m_hasUpdate && m_serviceStopped);
    m_btnUpdate->setText(m_serviceStopped ? L("update_now")
                                          : L("stop_service_first"));
    m_btnUpdate->setToolTip(m_serviceStopped
        ? L("run_the_official_updater")
        : L("stop_the_service_before_updating"));
}

void MainWindow::setVersionBadge(const QString &text, const QString &fg,
                                 const QString &bg, const QString &border) {
    m_versionBadge->setText(text);
    m_versionBadge->setStyleSheet(QStringLiteral(
        "QLabel#verBadge{padding:2px 10px;border-radius:9px;font-size:9pt;font-weight:600;"
        "background:%1;color:%2;border:1px solid %3;}").arg(bg, fg, border));
}

void MainWindow::setVersionChecking() {
    setVersionBadge(L("checking"), th(QStringLiteral("@{muted}")),
                    th(QStringLiteral("@{neutralBg}")), th(QStringLiteral("@{border}")));
    m_btnCheck->setEnabled(false);
}

void MainWindow::setVersionError(const QString &message) {
    m_checkError = message;
    setVersionBadge(L("check_failed"), th(QStringLiteral("@{errFg}")),
                    th(QStringLiteral("@{errBg}")), th(QStringLiteral("@{errBorder}")));
    m_versionBadge->setToolTip(message);
    m_btnCheck->setEnabled(true);
}

void MainWindow::applySnapshot(const Snapshot &s) {
    m_lastSnapshot = s;
    m_hasSnapshot = true;
    QString text, dot, fg, bg, border;
    switch (s.state) {
    case State::Ready:
        text = L("running_ready"); dot = th(QStringLiteral("@{dotOk}"));
        fg = th(QStringLiteral("@{okFg}")); bg = th(QStringLiteral("@{okBg}")); border = th(QStringLiteral("@{okBorder}")); break;
    case State::Starting:
        text = L("loading_model_2"); dot = th(QStringLiteral("@{dotWarn}"));
        fg = th(QStringLiteral("@{warnFg}")); bg = th(QStringLiteral("@{warnBg}")); border = th(QStringLiteral("@{warnBorder}")); break;
    case State::Orphan:
        text = L("error_no_listener"); dot = th(QStringLiteral("@{dotErr}"));
        fg = th(QStringLiteral("@{muted}")); bg = th(QStringLiteral("@{errBg}")); border = th(QStringLiteral("@{errBorder}")); break;
    case State::Stopped:
        text = L("stopped"); dot = th(QStringLiteral("@{caption}"));
        fg = th(QStringLiteral("@{muted}")); bg = th(QStringLiteral("@{neutralBg}")); border = th(QStringLiteral("@{border}")); break;
    }
    m_stateBadge->setText(text);
    m_badgePill->setToolTip(s.state == State::Orphan
        ? L("engine_running_but_port_not_responding_s")
        : QString());
    m_badgePill->setStyleSheet(QStringLiteral(
        "QWidget#badgePill{background:%1;border:1px solid %2;border-radius:11px;}"
        "QLabel#badgeText{color:%3;font-size:9pt;font-weight:600;background:transparent;}"
        "QLabel#stateDot{background:%4;border-radius:4px;}")
        .arg(bg, border, fg, dot));

    const QString pidText = s.pid
        ? L("pid_n_uptime_n").arg(s.pid).arg(s.uptime)
        : QStringLiteral("—");
    const QString modelText = s.hasHealth
        ? L("n_context_n_tokens").arg(s.model.toHtmlEscaped()).arg(s.maxContext)
        : L("no_listener");
    m_info->setText(th(QStringLiteral("<div style='line-height:1.65'><span style='color:@{caption}'>%1</span>&nbsp;&nbsp;%2<br><span style='color:@{caption}'>%3</span>&nbsp;&nbsp;%4</div>"))
        .arg(L("process"), pidText, L("model"), modelText));

    const auto gpu = Probe::gpuMem();
    if (gpu.has_value()) {
        m_vramLabel->setText(QStringLiteral("%1 / %2 GB")
                             .arg(gpu->usedGb, 0, 'f', 1).arg(gpu->totalGb, 0, 'f', 1));
        m_vramPct->setText(QStringLiteral("%1%").arg(gpu->percent()));
        m_vramBar->setValue(gpu->percent());
    } else {
        m_vramLabel->setText(L("unavailable"));
        m_vramPct->clear();
        m_vramBar->setValue(0);
    }

    m_serviceStopped = s.state == State::Stopped;
    m_serviceReady = s.state == State::Ready;
    m_serviceOrphan = s.state == State::Orphan;
    m_serviceStarting = s.state == State::Starting;
    refreshActionButtons();
}

void MainWindow::retranslateUi() {
    setWindowTitle(L("app_title"));
    m_svcTitle->setText(L("service_status"));
    m_verTitle->setText(L("version_update"));
    // 右上角设置按钮及其菜单：语言子菜单标题与勾选状态跟随当前语种
    m_btnSettings->setToolTip(L("settings_more"));
    LanguageMenu::retranslate(m_langMenu);
    ThemeMenu::retranslate(m_themeMenu);
    m_actAutoRun->setText(L("menu_run_at_startup"));
    {
        QSignalBlocker blocker(m_actAutoRun);   // 仅同步显示，不触发注册表写入
        m_actAutoRun->setChecked(Config::autoRunEnabled());
    }
    m_actOpenDir->setText(L("menu_open_strata_dir"));
    m_urlCap->setText(L("url"));
    m_vramCap->setText(L("vram"));
    // 三个主操作按钮的基准文案；状态相关的改写（加载中 / 运行中）由 refreshActionButtons 接管
    m_btnStart->setText(L("start_service"));
    m_btnStop->setText(L("stop_service_free_vram"));
    m_btnOpen->setText(L("open_webui"));
    m_btnCopyBase->setText(L("copy_base"));
    m_btnCopyApi->setText(L("copy_v1"));
    m_btnCopyApi->setToolTip(L("copy_the_openai_compatible_endpoint_n_v1").arg(Config::url()));
    m_btnLog->setText(L("refresh"));
    m_btnOpenLog->setText(L("open_log_file"));
    m_btnClearLog->setText(L("clear"));
    m_btnClearLog->setToolTip(L("clear_the_view_only_not_the_log_file"));
    m_logAutoScroll->setText(L("auto_scroll"));
    m_btnCheck->setText(L("recheck"));
    m_btnRelease->setText(L("full_release_notes"));
    m_tabs->setTabText(0, L("recent_logs"));
    m_tabs->setTabText(1, L("release_notes"));
    if (!m_hasSnapshot) {
        m_stateBadge->setText(L("detecting"));
        m_info->setText(QString());
        m_vramLabel->setText(QStringLiteral("—"));
    }
    if (!m_hasVersion) {
        setVersionBadge(L("checking"), th(QStringLiteral("@{muted}")),
                        th(QStringLiteral("@{neutralBg}")), th(QStringLiteral("@{border}")));
        m_localInfo->setText(L("local_loading"));
        m_remoteInfo->setText(L("upstream_loading"));
        m_updateState->setText(L("no_update_running"));
        m_versionView->setHtml(th(QStringLiteral("<p style='color:@{caption}'>%1</p>"))
                               .arg(L("checking_upstream_version")));
    }
    if (m_hasSnapshot) applySnapshot(m_lastSnapshot);
    if (m_hasVersion) {
        setVersion(m_lastVersion);
        if (m_updateMode) {
            refreshProgressLabel();
        } else if (m_hasResult) {
            renderVersionText(m_lastVersion);
            renderUpdateResult();
        }
    }
}
