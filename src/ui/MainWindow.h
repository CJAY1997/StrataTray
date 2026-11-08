#pragma once
#include <QElapsedTimer>
#include <QWidget>
#include "core/ProcessProbe.h"
#include "core/UpdateService.h"

class QLabel;
class QPushButton;
class QCheckBox;
class QProgressBar;
class QTabWidget;
class QTextEdit;
class QTimer;
class QShowEvent;
class QCloseEvent;

// 主窗口：双平级卡片 + 分段 Tab，只负责展示与发出意图信号
class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    void applySnapshot(const Snapshot &s);
    void setVersion(const VersionInfo &v);
    void beginUpdate();
    void appendUpdateLog(const QString &chunk, int step);
    void finishUpdate(bool ok, int code);
    void setVersionChecking();
    void setVersionError(const QString &message);
    void refreshLog();
    void showWindow();

public slots:
    void retranslateUi();

signals:
    void startRequested();
    void stopRequested();
    void openRequested();
    void checkRequested();
    void updateRequested();
    void refreshRequested();

protected:
    void showEvent(QShowEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private:
    void buildUi();
    void applyDarkTitleBar();
    void refreshActionButtons();
    void refreshProgressLabel();
    void setVersionBadge(const QString &text, const QString &fg,
                         const QString &bg, const QString &border);
    void renderVersionText(const VersionInfo &v);
    void renderUpdateResult();
    void copyUrl(const QString &text, QPushButton *btn, const QString &label);
    QString elapsedText() const;

    QWidget *m_badgePill = nullptr;
    QLabel *m_svcTitle = nullptr, *m_verTitle = nullptr;
    QLabel *m_stateDot = nullptr, *m_stateBadge = nullptr, *m_info = nullptr;
    QLabel *m_urlCap = nullptr, *m_urlValue = nullptr;
    QLabel *m_vramCap = nullptr, *m_vramLabel = nullptr, *m_vramPct = nullptr;
    QLabel *m_localInfo = nullptr, *m_remoteInfo = nullptr;
    QLabel *m_versionBadge = nullptr, *m_updateState = nullptr;
    QPushButton *m_btnStart = nullptr, *m_btnStop = nullptr, *m_btnOpen = nullptr;
    QPushButton *m_btnLog = nullptr, *m_btnOpenLog = nullptr, *m_btnClearLog = nullptr;
    QPushButton *m_btnCopyBase = nullptr, *m_btnCopyApi = nullptr;
    QPushButton *m_btnCheck = nullptr, *m_btnRelease = nullptr, *m_btnUpdate = nullptr;
    QPushButton *m_btnLang = nullptr;   // 右上角语言切换按钮
    QCheckBox *m_logAutoScroll = nullptr;
    QProgressBar *m_vramBar = nullptr, *m_progressBar = nullptr;
    QTabWidget *m_tabs = nullptr;
    QWidget *m_logPage = nullptr, *m_verPage = nullptr;
    QTextEdit *m_logView = nullptr, *m_versionView = nullptr;
    QTimer *m_progressTimer = nullptr;
    QElapsedTimer m_elapsed;

    QString m_releaseUrl, m_stageName, m_checkError;
    bool m_updateMode = false;
    bool m_serviceStopped = true, m_serviceReady = false;
    bool m_serviceOrphan = false, m_serviceStarting = false;
    bool m_hasUpdate = false;
    bool m_hasSnapshot = false, m_hasVersion = false;
    bool m_hasResult = false, m_resultOk = false;
    int m_resultCode = 0;
    int m_stageNum = 0;
    Snapshot m_lastSnapshot;
    VersionInfo m_lastVersion;
};
