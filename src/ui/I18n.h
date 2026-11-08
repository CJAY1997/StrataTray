#pragma once
#include <QObject>
#include <QString>
#include <QVector>
#include <QHash>

// 基于 JSON 键值字典的开放式多语言框架。
//
// 语言包来源（按加载顺序，后者同名时覆盖/补全前者）：
//   ① 内嵌资源 :/i18n/*.json        —— en.json 为英文基准，保证单 exe 也能用
//   ② exe 旁 release/i18n/*.json    —— 外部可热插拔，放入新语言包重启即生效
//
// 语言包格式：扁平 JSON
//   { "_meta": {"name": "简体中文", "locale": "zh_CN"}, "key": "文案", ... }
// 查词顺序：当前语言包 → en.json → key 本身（便于发现漏译）。
//
// 新增语言：只需在 resources/i18n/ 放一个 JSON 文件，
// 二级子菜单会自动挂载该语言，无需改动任何 C++ 代码。
class I18n : public QObject {
    Q_OBJECT
public:
    struct Language {
        QString id;      // 文件名（不含 .json），如 en / zh_CN / ja_JP
        QString name;    // 语言包 _meta.name，母语自称
    };

    static I18n &instance();

    const QVector<Language> &languages() const { return m_languages; }
    QString languageName(const QString &id) const;
    QString currentLanguageName() const { return languageName(m_langId); }
    QString langId() const { return m_langId; }

    void setLang(const QString &id);           // 按语言包 id 切换
    static QString t(const char *key);         // 字典查词，缺词条自动回退英文

    static bool zh();                          // 兼容辅助：当前是否为中文语言包
    static QString pick(const QString &zhText, const QString &enText);

signals:
    void languageChanged();

private:
    I18n();
    void loadPacks();
    void loadFromDir(const QString &dirPath);

    QVector<Language> m_languages;
    QHash<QString, QHash<QString, QString>> m_packs;   // id -> (key -> text)
    QString m_langId;
};
