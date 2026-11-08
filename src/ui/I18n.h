#pragma once
#include <QObject>
#include <QString>
#include <QLocale>
#include <vector>

// 轻量多语言框架：默认跟随系统 locale，支持通过菜单切换到任意已注册语种。
//
// 新增语种只需两步：
//   ① 在下面的 Lang 枚举里加一个值；
//   ② 在 I18n.cpp 的 kLanguages 注册表里加一行（母语名称 + QLocale 匹配值）。
// 所有菜单、勾选状态、热重载逻辑都由注册表驱动，无需改动 UI 代码。
//
// 界面文案目前由 s(zh, en) / L() 提供中英双串；尚未翻译的语种回退英文，
// 后续可在此扩展为按 Lang 查表，不影响调用方。
class I18n : public QObject {
    Q_OBJECT
public:
    enum class Lang { Chinese, English /*, Japanese, Korean, French ... */ };

    struct Language {
        Lang id;
        const char *nativeName;      // 母语自称，菜单里直接显示（不随界面语言变化）
        QLocale::Language locale;    // 用于与 QLocale::system() 自动匹配
    };

    static I18n &instance();

    // 通用查询接口：菜单构建、设置页、日志都从这里取，避免硬编码语言列表
    static const std::vector<Language> &supportedLanguages();
    static QString languageName(Lang l);
    QString currentLanguageName() const { return languageName(m_lang); }

    Lang lang() const { return m_lang; }
    void setLang(Lang l);            // 明确设置语言，替代旧的二元 toggle()

    // 现有文案辅助（中英双串；其它语种暂回退英文）
    static bool zh();
    static QString s(const char *zhText, const char *enText);
    static QString pick(const QString &zhText, const QString &enText);

signals:
    void languageChanged();

private:
    I18n();
    Lang m_lang;
};
