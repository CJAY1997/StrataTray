#pragma once
#include <QObject>
#include <QString>
#include <QLocale>

// 轻量多语言：默认跟随系统 locale，托盘菜单可一键切换
class I18n : public QObject {
    Q_OBJECT
public:
    enum class Lang { Chinese, English };

    static I18n &instance();
    static bool zh();
    static QString s(const char *zhText, const char *enText);
    static QString pick(const QString &zhText, const QString &enText);

    Lang lang() const { return m_lang; }
    void set(Lang l);
    void toggle();

signals:
    void languageChanged();

private:
    I18n();
    Lang m_lang;
};
