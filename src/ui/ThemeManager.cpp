#include "ThemeManager.h"
#include "I18n.h"
#include "Theme.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtWidgets/QApplication>

namespace {

const ThemePalette &darkPalette() {
    static const ThemePalette p = [] {
        ThemePalette x;
        x.surface = "#202023"; x.surface2 = "#2b2b31";
        x.border = "#3a3a42"; x.borderLight = "#2f2f35";
        x.text = "#e4e4e7"; x.textStrong = "#fafafa";
        x.muted = "#a1a1aa"; x.caption = "#71717a";
        x.codeBg = "#141417"; x.codeText = "#d4d4d8";
        x.okFg = "#4ade80"; x.okBg = "#143024"; x.okBorder = "#1d5b3b";
        x.warnFg = "#fbbf24"; x.warnBg = "#3a2a12"; x.warnBorder = "#78500f";
        x.errFg = "#f87171"; x.errBg = "#3a1a1a"; x.errBorder = "#7f2d2d";
        x.neutralFg = "#a1a1aa"; x.neutralBg = "#252529"; x.neutralBorder = "#3a3a42";
        x.dotOk = "#22c55e"; x.dotWarn = "#f59e0b";
        x.dotErr = "#ef4444"; x.dotNeutral = "#71717a";
        x.hlBlue = "#7dd3fc"; x.hlPurple = "#a78bfa";
        x.dangerActiveBg = "#7f1d1d"; x.dangerActiveBorder = "#ef4444";
        x.onAccentText = "#ffffff";
        return x;
    }();
    return p;
}

const ThemePalette &lightPalette() {
    static const ThemePalette p = [] {
        ThemePalette x;
        x.surface = "#ffffff"; x.surface2 = "#f4f4f5";
        x.border = "#d4d4d8"; x.borderLight = "#e4e4e7";
        x.text = "#3f3f46"; x.textStrong = "#18181b";
        x.muted = "#52525b"; x.caption = "#71717a";
        x.codeBg = "#f8fafc"; x.codeText = "#3f3f46";
        x.okFg = "#15803d"; x.okBg = "#dcfce7"; x.okBorder = "#86efac";
        x.warnFg = "#b45309"; x.warnBg = "#fef3c7"; x.warnBorder = "#fcd34d";
        x.errFg = "#dc2626"; x.errBg = "#fee2e2"; x.errBorder = "#fca5a5";
        x.neutralFg = "#52525b"; x.neutralBg = "#f4f4f5"; x.neutralBorder = "#e4e4e7";
        x.dotOk = "#16a34a"; x.dotWarn = "#d97706";
        x.dotErr = "#dc2626"; x.dotNeutral = "#a1a1aa";
        x.hlBlue = "#0369a1"; x.hlPurple = "#7c3aed";
        x.dangerActiveBg = "#b91c1c"; x.dangerActiveBorder = "#ef4444";
        x.onAccentText = "#ffffff";
        return x;
    }();
    return p;
}

// 读取 .qss 顶部的 @theme-xxx 元数据指令
QString metaValue(const QString &qss, const QString &key) {
    QRegularExpression re(QStringLiteral("@theme-%1\\s+([^\\r\\n*]+)").arg(key));
    const QRegularExpressionMatch m = re.match(qss);
    return m.hasMatch() ? m.captured(1).trimmed() : QString();
}

const QString &settingsOrg() {
    static const QString v = QStringLiteral("StrataTray");
    return v;
}

} // namespace

ThemeManager::ThemeManager() { loadAll(); }

ThemeManager &ThemeManager::instance() {
    static ThemeManager m;
    return m;
}

int ThemeManager::indexOf(const QString &id) const {
    for (int i = 0; i < m_themes.size(); ++i)
        if (m_themes.at(i).id == id) return i;
    return -1;
}

void ThemeManager::loadAll() {
    // 内置两套：暗色（默认）与亮色
    m_themes.append({QStringLiteral("dark"), QStringLiteral("theme_dark"),
                     true, QString::fromUtf8(Theme::darkQss()), QString()});
    m_themes.append({QStringLiteral("light"), QStringLiteral("theme_light"),
                     false, QString::fromUtf8(Theme::lightQss()), QString()});

    // 外部扩展：内嵌资源优先，exe 旁 themes/ 目录可热插拔（同名 id 不覆盖内置）
    loadExternalDir(QStringLiteral(":/themes"));
    const QString ext = QCoreApplication::applicationDirPath() + QStringLiteral("/themes");
    if (QDir(ext).exists()) loadExternalDir(ext);
}

void ThemeManager::loadExternalDir(const QString &dirPath) {
    QDir dir(dirPath);
    const QStringList files = dir.entryList({QStringLiteral("*.qss")}, QDir::Files, QDir::Name);
    for (const QString &fn : files) {
        QFile f(dir.filePath(fn));
        if (!f.open(QIODevice::ReadOnly)) continue;
        const QString qss = QString::fromUtf8(f.readAll());
        f.close();
        const QString id = QFileInfo(fn).completeBaseName();
        if (id.isEmpty() || indexOf(id) >= 0) continue;

        ThemeInfo t;
        t.id = id;
        t.qss = qss;
        t.nameKey = metaValue(qss, QStringLiteral("name-key"));
        if (t.nameKey.isEmpty()) t.nameKey = QStringLiteral("theme_") + id;
        t.fallbackName = metaValue(qss, QStringLiteral("name"));
        t.isDark = metaValue(qss, QStringLiteral("dark")).compare(
                       QStringLiteral("false"), Qt::CaseInsensitive) != 0;
        m_themes.append(t);
    }
}

const ThemePalette &ThemeManager::palette() const {
    return isDark() ? darkPalette() : lightPalette();
}

bool ThemeManager::isDark() const {
    const int i = indexOf(m_currentId);
    return i < 0 ? true : m_themes.at(i).isDark;
}

QString ThemeManager::displayName(const ThemeInfo &t) const {
    if (!t.nameKey.isEmpty()) {
        const QString translated = I18n::t(t.nameKey.toUtf8().constData());
        if (translated != t.nameKey) return translated;   // 字典里存在该 key
    }
    return t.fallbackName.isEmpty() ? t.id : t.fallbackName;
}

void ThemeManager::apply(const QString &id) {
    const int i = indexOf(id);
    if (i < 0) return;
    m_currentId = id;

    QSettings s(settingsOrg(), settingsOrg());
    s.setValue(QStringLiteral("ui/theme"), id);
    s.sync();

    if (qApp) qApp->setStyleSheet(m_themes.at(i).qss);
    emit themeChanged();
}

void ThemeManager::applySaved() {
    const QSettings s(settingsOrg(), settingsOrg());
    const QString saved = s.value(QStringLiteral("ui/theme"),
                                  QStringLiteral("dark")).toString();
    apply(indexOf(saved) >= 0 ? saved : QStringLiteral("dark"));
}
