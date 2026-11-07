#include "MainWindow.h"
#include "I18n.h"
#include "Theme.h"
#include "core/Config.h"

#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtGui/QClipboard>
#include <QtGui/QCloseEvent>
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

static inline QString L(const char *zhText, const char *enText) {
    return I18n::s(zhText, enText);
}

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("rootWindow"));
    setWindowIcon(Theme::appIcon(State::Stopped));
    setMinimumSize(560, 440);
    resize(680, 540);
    buildUi();
    retranslateUi();
    refreshLog();
    applyDarkTitleBar();

    connect(&I18n::instance(), &I18n::languageChanged, this, &MainWindow::retranslateUi);
    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::startRequested);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::stopRequested);
    connect(m_btnOpen, &QPushButton::clicked, this, &MainWindow::openRequested);
    connect(m_btnLog, &QPushButton::clicked, this, &MainWindow::refreshLog);
    connect(m_btnClearLog, &QPushButton::clicked, this, [this] { m_logView->clear(); });
    connect(m_btnOpenLog, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(Config::instance().logFile()));
    });
    connect(m_btnCopyBase, &QPushButton::clicked, this, [this] {
        copyUrl(Config::url(), m_btnCopyBase, L("复制 Base", "Copy Base"));
    });
    connect(m_btnCopyApi, &QPushButton::clicked, this, [this] {
        copyUrl(Config::url() + QStringLiteral("/v1"), m_btnCopyApi, L("复制 /v1", "Copy /v1"));
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
    m_verTitle = new QLabel(ver);
    m_verTitle->setObjectName(QStringLiteral("cardTitle"));
    m_versionBadge = new QLabel(ver);
    m_versionBadge->setObjectName(QStringLiteral("verBadge"));
    verTitleRow->addWidget(m_verTitle);
    verTitleRow->addStretch(1);
    verTitleRow->addWidget(m_versionBadge);
    vv->addLayout(verTitleRow);

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

void MainWindow::applyDarkTitleBar() {
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &darkMode, sizeof(darkMode));
    int corner = 2;
    DwmSetWindowAttribute(hwnd, 33, &corner, sizeof(corner));
#endif
}

void MainWindow::showEvent(QShowEvent *e) {
    QWidget::showEvent(e);
    applyDarkTitleBar();
}

void MainWindow::closeEvent(QCloseEvent *e) {
    e->ignore();
    hide();
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
    btn->setText(L("已复制 ✓", "Copied ✓"));
    btn->setStyleSheet(QStringLiteral(
        "QPushButton#btnMini{background:#143024;color:#4ade80;border-color:#1d5b3b;}"));
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
            fmt.setForeground(QColor(QStringLiteral("#f87171")));
        } else if (line.contains(QStringLiteral("warn"), Qt::CaseInsensitive)) {
            fmt.setForeground(QColor(QStringLiteral("#fbbf24")));
        } else {
            fmt.setForeground(QColor(QStringLiteral("#d4d4d8")));
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

    setVersionBadge(v.hasUpdate ? L("有新版可更新", "Update Available")
                                : L("已是最新", "Up to date"),
                    v.hasUpdate ? QStringLiteral("#fbbf24") : QStringLiteral("#4ade80"),
                    v.hasUpdate ? QStringLiteral("#3a2a12") : QStringLiteral("#143024"),
                    v.hasUpdate ? QStringLiteral("#78500f") : QStringLiteral("#1d5b3b"));
    m_versionBadge->setToolTip(v.remoteName);

    const QString gap = v.behind > 0
        ? L("落后 %1 个提交", "%1 commits behind").arg(v.behind)
        : L("无", "none");
    m_localInfo->setText(QStringLiteral(
        "<div style='line-height:1.6'><span style='color:#71717a'>%1</span>&nbsp;&nbsp;"
        "<b style='color:#e4e4e7'>%2</b><br><span style='color:#71717a'>%3</span>&nbsp;&nbsp;%4 · %5 %6</div>")
        .arg(L("本地", "Local"), v.localDesc.toHtmlEscaped(), L("引擎", "Engine"),
             v.engine, L("提交", "commit"), v.localTag.toHtmlEscaped()));
    m_remoteInfo->setText(QStringLiteral(
        "<div style='line-height:1.6'><span style='color:#71717a'>%1</span>&nbsp;&nbsp;"
        "<b style='color:#e4e4e7'>%2</b><br><span style='color:#71717a'>%3</span>&nbsp;&nbsp;%4 · %5 %6<br>"
        "<span style='color:#71717a'>%7</span>&nbsp;&nbsp;%8</div>")
        .arg(L("线上", "Upstream"), v.remoteTag.toHtmlEscaped(), L("发布", "Released"),
             v.published.toHtmlEscaped(), L("引擎", "Engine"), v.remoteEngine,
             L("差距", "Gap"), gap));

    if (!m_updateMode) {
        if (v.hasUpdate) {
            m_updateState->setText(L("发现官方新版，可点击下方立即更新",
                                    "New release available. Click Update Now below."));
            m_updateState->setStyleSheet(QStringLiteral("QLabel{color:#fbbf24;font-size:9pt;}"));
        } else {
            const QString hhmm = v.checkedAt.length() >= 16
                ? v.checkedAt.mid(11, 5) : QStringLiteral("--:--");
            m_updateState->setText(L("已是最新版本 · 检查于 %1",
                                    "Up to date · checked at %1").arg(hhmm));
            m_updateState->setStyleSheet(QStringLiteral("QLabel{color:#71717a;font-size:9pt;}"));
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
    QString html = QStringLiteral("<div style='color:#e4e4e7'>");
    html += QStringLiteral("<h3 style='color:#7dd3fc;font-size:11pt;margin:0 0 8px 0;'>%1</h3>")
            .arg(L("本次更新要点", "Release Highlights"));
    if (v.highlights.isEmpty()) {
        html += QStringLiteral("<p style='color:#71717a;margin:0;'>%1</p>")
                .arg(L("官方未提供要点", "No highlights provided"));
    } else {
        html += QStringLiteral("<ul style='margin:0 0 12px 18px;line-height:1.65;color:#d4d4d8;'>");
        for (const QString &h : v.highlights)
            html += QStringLiteral("<li>%1</li>").arg(h.toHtmlEscaped());
        html += QStringLiteral("</ul>");
    }
    html += QStringLiteral("<p style='color:#71717a;font-size:9pt;margin:0 0 12px 0;'>%1 %2")
            .arg(L("检查时间", "Checked at"), v.checkedAt.toHtmlEscaped());
    if (v.behind >= 0)
        html += QStringLiteral(" · %1 %2").arg(L("待更新提交", "commits behind")).arg(v.behind);
    if (!v.error.isEmpty())
        html += QStringLiteral("<br><span style='color:#fbbf24'>%1</span>").arg(v.error.toHtmlEscaped());
    html += QStringLiteral("</p>");

    const QStringList &list = !v.commits.isEmpty() ? v.commits : v.recent;
    if (!list.isEmpty()) {
        html += QStringLiteral("<h3 style='color:#a78bfa;font-size:11pt;margin:4px 0 8px 0;'>%1</h3>")
                .arg(!v.commits.isEmpty()
                     ? L("待更新的提交（最近 10 条）", "Pending commits (latest 10)")
                     : L("当前版本最近提交", "Recent commits"));
        html += QStringLiteral("<div style='font-family:Consolas,monospace;font-size:9pt;color:#a1a1aa;line-height:1.6;'>");
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
    m_stageName = L("启动", "Starting");
    m_btnCheck->setEnabled(false);
    refreshActionButtons();
    m_updateState->setStyleSheet(QStringLiteral("QLabel{color:#fbbf24;font-size:9pt;font-weight:600;}"));
    m_progressBar->setStyleSheet(QString());
    m_progressBar->setValue(0);
    m_progressBar->show();
    m_versionView->setPlainText(L("=== 更新日志（实时）===\n", "=== Update log (live) ===\n"));
    m_elapsed.start();
    m_progressTimer->start(1000);
    refreshProgressLabel();
}

void MainWindow::appendUpdateLog(const QString &chunk, int step) {
    if (chunk.isEmpty()) return;
    if (step > 0) {
        static const char *zh[] = {"", "停止服务", "拉取官方新版", "更新引擎与依赖", "重新接入定制层", "完成"};
        static const char *en[] = {"", "Stop service", "Pull upstream", "Update engine", "Reapply local layer", "Done"};
        m_stageNum = step;
        m_stageName = I18n::zh() ? QString::fromUtf8(zh[step]) : QString::fromUtf8(en[step]);
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
        ? L("\n=== 更新完成（退出码 0）===", "\n=== Update complete (exit 0) ===")
        : L("\n=== 更新中断（退出码 %1）===", "\n=== Update failed (exit %1) ===").arg(code));
    m_versionView->moveCursor(QTextCursor::End);
    refreshActionButtons();
}

void MainWindow::renderUpdateResult() {
    if (!m_hasResult) return;
    if (m_resultOk) {
        m_progressBar->setValue(5);
        m_progressBar->setStyleSheet(QStringLiteral("QProgressBar#updateBar::chunk{background:#22c55e;}"));
        m_updateState->setText(L("更新完成 · 耗时 %1 · 正在核对版本",
                                 "Update complete in %1 · verifying").arg(elapsedText()));
        m_updateState->setStyleSheet(QStringLiteral("QLabel{color:#4ade80;font-size:9pt;font-weight:600;}"));
    } else {
        m_updateState->setText(L("更新未完成 · 退出码 %1 · 详见 update-local.log",
                                 "Update failed (exit %1) · see update-local.log").arg(m_resultCode));
        m_updateState->setStyleSheet(QStringLiteral("QLabel{color:#f87171;font-size:9pt;font-weight:600;}"));
    }
}

void MainWindow::refreshProgressLabel() {
    if (!m_updateMode || !m_elapsed.isValid()) return;
    m_updateState->setText(L("更新中 · 第 %1/5 步：%2　已用时 %3",
                             "Updating · step %1/5: %2 · elapsed %3")
                           .arg(m_stageNum).arg(m_stageName).arg(elapsedText()));
}

QString MainWindow::elapsedText() const {
    const qint64 sec = m_elapsed.isValid() ? m_elapsed.elapsed() / 1000 : 0;
    return QStringLiteral("%1:%2").arg(sec / 60, 2, 10, QChar('0'))
                                  .arg(sec % 60, 2, 10, QChar('0'));
}

void MainWindow::refreshActionButtons() {
    if (m_updateMode) {
        m_btnStart->setText(L("启动服务", "Start Service"));
        m_btnStart->setEnabled(false);
        m_btnStop->setEnabled(false);
        m_btnStop->setStyleSheet(QString());
        m_btnOpen->setEnabled(false);
        m_btnUpdate->setEnabled(false);
        m_btnUpdate->setText(L("更新中…", "Updating…"));
        m_btnUpdate->setToolTip(L("升级进行中，启动 / 停止 / 打开网页已暂停",
                                  "Update in progress; service controls are paused"));
        return;
    }
    if (m_serviceStarting) {
        m_btnStart->setText(L("加载模型中…", "Loading model…"));
        m_btnStart->setEnabled(false);
    } else if (m_serviceReady) {
        m_btnStart->setText(L("运行中", "Running"));
        m_btnStart->setEnabled(false);
    } else {
        m_btnStart->setText(L("启动服务", "Start Service"));
        m_btnStart->setEnabled(m_serviceStopped || m_serviceOrphan);
    }
    m_btnStop->setEnabled(!m_serviceStopped);
    if (m_serviceReady) {
        m_btnStop->setStyleSheet(QStringLiteral(
            "QPushButton#btnDanger{background:#7f1d1d;border-color:#ef4444;color:#ffffff;}"));
        m_btnStop->setToolTip(L("点击停止服务并释放显存", "Stop the service and free VRAM"));
    } else {
        m_btnStop->setStyleSheet(QString());
        m_btnStop->setToolTip(m_serviceOrphan
            ? L("引擎运行但端口未响应，点击停止服务进行清理",
                "Engine running but port not responding; click to clean up")
            : L("停止服务并释放显存", "Stop the service and free VRAM"));
    }
    m_btnOpen->setEnabled(m_serviceReady);
    m_btnUpdate->setEnabled(m_hasUpdate && m_serviceStopped);
    m_btnUpdate->setText(m_serviceStopped ? L("立即更新", "Update Now")
                                          : L("先停止服务", "Stop Service First"));
    m_btnUpdate->setToolTip(m_serviceStopped
        ? L("按官方流程升级（会先停止服务）", "Run the official updater")
        : L("运行中的模型不能升级，请先点「停止服务」", "Stop the service before updating"));
}

void MainWindow::setVersionBadge(const QString &text, const QString &fg,
                                 const QString &bg, const QString &border) {
    m_versionBadge->setText(text);
    m_versionBadge->setStyleSheet(QStringLiteral(
        "QLabel#verBadge{padding:2px 10px;border-radius:9px;font-size:9pt;font-weight:600;"
        "background:%1;color:%2;border:1px solid %3;}").arg(bg, fg, border));
}

void MainWindow::setVersionChecking() {
    setVersionBadge(L("检查中…", "Checking…"), QStringLiteral("#a1a1aa"),
                    QStringLiteral("#252529"), QStringLiteral("#3a3a42"));
    m_btnCheck->setEnabled(false);
}

void MainWindow::setVersionError(const QString &message) {
    m_checkError = message;
    setVersionBadge(L("检查失败", "Check failed"), QStringLiteral("#f87171"),
                    QStringLiteral("#3a1a1a"), QStringLiteral("#7f2d2d"));
    m_versionBadge->setToolTip(message);
    m_btnCheck->setEnabled(true);
}

void MainWindow::applySnapshot(const Snapshot &s) {
    m_lastSnapshot = s;
    m_hasSnapshot = true;
    QString text, dot, fg, bg, border;
    switch (s.state) {
    case State::Ready:
        text = L("运行中（就绪）", "Running (Ready)"); dot = QStringLiteral("#22c55e");
        fg = QStringLiteral("#4ade80"); bg = QStringLiteral("#143024"); border = QStringLiteral("#1d5b3b"); break;
    case State::Starting:
        text = L("加载中（模型加载）", "Loading model"); dot = QStringLiteral("#f59e0b");
        fg = QStringLiteral("#fbbf24"); bg = QStringLiteral("#3a2a12"); border = QStringLiteral("#78500f"); break;
    case State::Orphan:
        text = L("异常（服务未监听）", "Error (no listener)"); dot = QStringLiteral("#ef4444");
        fg = QStringLiteral("#fca5a5"); bg = QStringLiteral("#3a1a1a"); border = QStringLiteral("#7f2d2d"); break;
    case State::Stopped:
        text = L("已停止", "Stopped"); dot = QStringLiteral("#71717a");
        fg = QStringLiteral("#a1a1aa"); bg = QStringLiteral("#252529"); border = QStringLiteral("#3a3a42"); break;
    }
    m_stateBadge->setText(text);
    m_badgePill->setToolTip(s.state == State::Orphan
        ? L("引擎运行但端口未响应，可点「停止服务」进行清理",
            "Engine running but port not responding; stop the service to clean up")
        : QString());
    m_badgePill->setStyleSheet(QStringLiteral(
        "QWidget#badgePill{background:%1;border:1px solid %2;border-radius:11px;}"
        "QLabel#badgeText{color:%3;font-size:9pt;font-weight:600;background:transparent;}"
        "QLabel#stateDot{background:%4;border-radius:4px;}")
        .arg(bg, border, fg, dot));

    const QString pidText = s.pid
        ? L("PID %1 · 已运行 %2", "PID %1 · uptime %2").arg(s.pid).arg(s.uptime)
        : QStringLiteral("—");
    const QString modelText = s.hasHealth
        ? L("%1 · 上下文 %2 tokens", "%1 · context %2 tokens").arg(s.model.toHtmlEscaped()).arg(s.maxContext)
        : L("—（服务没在监听）", "— (no listener)");
    m_info->setText(QStringLiteral(
        "<div style='line-height:1.65'><span style='color:#71717a'>%1</span>&nbsp;&nbsp;%2<br>"
        "<span style='color:#71717a'>%3</span>&nbsp;&nbsp;%4</div>")
        .arg(L("进程", "Process"), pidText, L("模型", "Model"), modelText));

    const auto gpu = Probe::gpuMem();
    if (gpu.has_value()) {
        m_vramLabel->setText(QStringLiteral("%1 / %2 GB")
                             .arg(gpu->usedGb, 0, 'f', 1).arg(gpu->totalGb, 0, 'f', 1));
        m_vramPct->setText(QStringLiteral("%1%").arg(gpu->percent()));
        m_vramBar->setValue(gpu->percent());
    } else {
        m_vramLabel->setText(L("不可用", "unavailable"));
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
    setWindowTitle(L("Strata 本地大模型", "Strata Local LLM"));
    m_svcTitle->setText(L("服务状态", "Service Status"));
    m_verTitle->setText(L("版本与更新", "Version & Update"));
    m_urlCap->setText(L("地址", "URL"));
    m_vramCap->setText(L("显存", "VRAM"));
    m_btnCopyBase->setText(L("复制 Base", "Copy Base"));
    m_btnCopyApi->setText(L("复制 /v1", "Copy /v1"));
    m_btnCopyApi->setToolTip(L("复制 OpenAI 兼容接口 %1/v1",
                               "Copy the OpenAI-compatible endpoint %1/v1").arg(Config::url()));
    m_btnLog->setText(L("刷新", "Refresh"));
    m_btnOpenLog->setText(L("打开日志文件", "Open Log File"));
    m_btnClearLog->setText(L("清空", "Clear"));
    m_btnClearLog->setToolTip(L("只清空当前视图，不删除日志文件",
                                "Clear the view only, not the log file"));
    m_logAutoScroll->setText(L("自动滚动到末尾", "Auto Scroll to End"));
    m_btnCheck->setText(L("重新检查", "Recheck"));
    m_btnRelease->setText(L("完整更新说明", "Full Release Notes"));
    m_tabs->setTabText(0, L("最近日志", "Recent Logs"));
    m_tabs->setTabText(1, L("更新内容", "Release Notes"));
    if (!m_hasSnapshot) {
        m_stateBadge->setText(L("检测中…", "Detecting…"));
        m_info->setText(QString());
        m_vramLabel->setText(QStringLiteral("—"));
    }
    if (!m_hasVersion) {
        setVersionBadge(L("检查中…", "Checking…"), QStringLiteral("#a1a1aa"),
                        QStringLiteral("#252529"), QStringLiteral("#3a3a42"));
        m_localInfo->setText(L("本地：读取中…", "Local: loading…"));
        m_remoteInfo->setText(L("线上：读取中…", "Upstream: loading…"));
        m_updateState->setText(L("尚未开始更新", "No update running"));
        m_versionView->setHtml(QStringLiteral("<p style='color:#71717a'>%1</p>")
                               .arg(L("正在检查线上版本…", "Checking upstream version…")));
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
