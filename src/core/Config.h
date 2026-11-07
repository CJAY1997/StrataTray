#pragma once
#include <QString>

// 单例配置：路径自动探测（std::filesystem）与开机自启读写
class Config {
public:
    static Config &instance();
    void initialize();                    // 必须在 QApplication 创建之后调用

    QString root() const;
    QString python() const;
    QString ctl() const;
    QString updater() const;
    QString logFile() const;
    QString cacheFile() const;
    QString pidFile() const;
    static QString url();
    static int port();

    static bool autoRunEnabled();
    static void setAutoRun(bool on);

private:
    Config() = default;
    QString detectRoot() const;
    QString m_root;
    bool m_ready = false;
};
