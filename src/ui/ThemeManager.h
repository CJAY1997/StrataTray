#pragma once
#include <QObject>
#include <QString>
#include <QVector>

// 语义调色板：QSS 管控件，Palette 管代码里动态生成的徽章 / 富文本 / 日志着色。
// 切换主题时 MainWindow 会用新的 palette 重新渲染这些元素，避免亮色主题下文字看不清。
struct ThemePalette {
    QString surface, surface2, border, borderLight;
    QString text, textStrong, muted, caption;
    QString codeBg, codeText;
    QString okFg, okBg, okBorder;
    QString warnFg, warnBg, warnBorder;
    QString errFg, errBg, errBorder;
    QString neutralFg, neutralBg, neutralBorder;
    QString dotOk, dotWarn, dotErr, dotNeutral;
    QString hlBlue, hlPurple;
    QString dangerActiveBg, dangerActiveBorder, onAccentText;
};

// 主题管理：注册表 + 外部扩展 + 持久化 + 热切换信号
//
// 内置 dark / light 两套；社区贡献者只需在 release/themes/ 或 resources/themes/
// 放一个 .qss 文件，重启后该主题会自动出现在「外观主题」子菜单中，无需改 C++。
// .qss 顶部可用以下注释指令声明元数据（全部可选）：
//   /* @theme-name Dracula */
//   /* @theme-dark true */
//   /* @theme-name-key theme_dracula */
class ThemeManager : public QObject {
    Q_OBJECT
public:
    struct ThemeInfo {
        QString id;             // "dark" / "light" / 自定义 id
        QString nameKey;        // 多语言字典 key，如 "theme_dark"
        bool isDark = true;     // 决定 Windows 原生标题栏明暗
        QString qss;            // 完整 QSS
        QString fallbackName;   // 字典缺 nameKey 时的兜底显示名（外部主题用）
    };

    static ThemeManager &instance();

    const QVector<ThemeInfo> &themes() const { return m_themes; }
    QString currentId() const { return m_currentId; }
    bool isDark() const;
    const ThemePalette &palette() const;
    QString displayName(const ThemeInfo &t) const;

    void apply(const QString &id);   // 切换 + 持久化 + 全局换肤 + 广播
    void applySaved();               // 启动时读取 QSettings 并应用（默认 dark）

signals:
    void themeChanged();

private:
    ThemeManager();
    void loadAll();
    void loadExternalDir(const QString &dirPath);
    int indexOf(const QString &id) const;

    QVector<ThemeInfo> m_themes;
    QString m_currentId = QStringLiteral("dark");
};
