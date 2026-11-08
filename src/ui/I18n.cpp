#include "I18n.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLocale>
#include <algorithm>

namespace {
constexpr const char *kBaseLang = "en";   // 英文基准：所有语言包的回退目标

struct Pack {
    QString id, name;
    QHash<QString, QString> dict;
};

// 解析一个语言包；格式不对时返回空 id
Pack readPack(QIODevice &dev, const QString &id) {
    const QJsonDocument doc = QJsonDocument::fromJson(dev.readAll());
    if (!doc.isObject()) return {};
    const QJsonObject obj = doc.object();
    Pack p;
    p.id = id;
    p.name = id;
    const QJsonObject meta = obj.value(QStringLiteral("_meta")).toObject();
    if (meta.value(QStringLiteral("name")).isString())
        p.name = meta.value(QStringLiteral("name")).toString();
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        if (it.key() == QLatin1String("_meta")) continue;
        if (it.value().isString()) p.dict.insert(it.key(), it.value().toString());
    }
    return p;
}
} // namespace

I18n::I18n() : m_langId(QLatin1String(kBaseLang)) {
    loadPacks();

    // 默认跟随系统：先精确匹配 locale 名（zh_CN），再退到主语言（en_US -> en）
    const QString sys = QLocale::system().name();
    if (m_packs.contains(sys)) { m_langId = sys; return; }
    const QString primary = sys.section(QLatin1Char('_'), 0, 0);
    if (m_packs.contains(primary)) { m_langId = primary; return; }
    for (const auto &l : m_languages) {
        if (l.id.startsWith(primary + QLatin1Char('_'))) { m_langId = l.id; break; }
    }
}

I18n &I18n::instance() {
    static I18n i;
    return i;
}

void I18n::loadFromDir(const QString &dirPath) {
    QDir dir(dirPath);
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    for (const QString &fn : files) {
        QFile f(dir.filePath(fn));
        if (!f.open(QIODevice::ReadOnly)) continue;
        const Pack p = readPack(f, QFileInfo(fn).completeBaseName());
        f.close();
        if (p.id.isEmpty()) continue;

        if (!m_packs.contains(p.id)) {
            m_languages.append({p.id, p.name});
        } else {
            for (auto &l : m_languages)
                if (l.id == p.id) { l.name = p.name; break; }
        }
        // 同名语言包按键合并：外部包可只提供部分词条，其余继续回退到内嵌版本
        QHash<QString, QString> &dst = m_packs[p.id];
        for (auto it = p.dict.constBegin(); it != p.dict.constEnd(); ++it)
            dst.insert(it.key(), it.value());
    }
}

void I18n::loadPacks() {
    loadFromDir(QStringLiteral(":/i18n"));   // 内嵌基线（en + zh_CN）
    const QString external = QCoreApplication::applicationDirPath() + QStringLiteral("/i18n");
    if (QDir(external).exists()) loadFromDir(external);   // 外部热插拔包

    // 英文基准排最前，其余按 id 稳定排序，保证子菜单顺序可预期
    std::sort(m_languages.begin(), m_languages.end(), [](const Language &a, const Language &b) {
        if (a.id == QLatin1String(kBaseLang)) return true;
        if (b.id == QLatin1String(kBaseLang)) return false;
        return a.id < b.id;
    });
}

QString I18n::languageName(const QString &id) const {
    for (const auto &l : m_languages)
        if (l.id == id) return l.name;
    return id;
}

QString I18n::t(const char *key) {
    I18n &i = instance();
    const QString k = QString::fromUtf8(key);
    auto lookup = [&](const QString &id) -> QString {
        auto pack = i.m_packs.constFind(id);
        if (pack == i.m_packs.constEnd()) return QString();
        auto it = pack->constFind(k);
        return it == pack->constEnd() ? QString() : *it;
    };
    QString v = lookup(i.m_langId);
    if (!v.isNull()) return v;                       // isNull 可区分「空译文」与「缺词条」
    if (i.m_langId != QLatin1String(kBaseLang)) {
        v = lookup(QLatin1String(kBaseLang));        // 未翻译词条回退英文
        if (!v.isNull()) return v;
    }
    return k;                                        // 最后兜底显示 key，便于发现漏译
}

bool I18n::zh() { return instance().m_langId == QLatin1String("zh_CN"); }

QString I18n::pick(const QString &zhText, const QString &enText) {
    return zh() ? zhText : enText;
}

void I18n::setLang(const QString &id) {
    if (id == m_langId || !m_packs.contains(id)) return;
    m_langId = id;
    emit languageChanged();
}
