#include "I18n.h"

namespace {
// 唯一的语言注册表：新增语种只改这里 + I18n.h 的枚举，UI 菜单自动扩展
const std::vector<I18n::Language> kLanguages = {
    { I18n::Lang::Chinese, "简体中文", QLocale::Chinese },
    { I18n::Lang::English, "English",  QLocale::English },
    // { I18n::Lang::Japanese, "日本語", QLocale::Japanese },
    // { I18n::Lang::Korean,   "한국어",  QLocale::Korean },
    // { I18n::Lang::French,   "Français", QLocale::French },
};
} // namespace

I18n::I18n() : m_lang(Lang::English) {
    const QLocale::Language sys = QLocale::system().language();
    for (const auto &def : kLanguages) {
        if (def.locale == sys) { m_lang = def.id; break; }
    }
}

I18n &I18n::instance() {
    static I18n i;
    return i;
}

const std::vector<I18n::Language> &I18n::supportedLanguages() {
    return kLanguages;
}

QString I18n::languageName(Lang l) {
    for (const auto &def : kLanguages)
        if (def.id == l) return QString::fromUtf8(def.nativeName);
    return QStringLiteral("Unknown");
}

bool I18n::zh() { return instance().m_lang == Lang::Chinese; }

QString I18n::s(const char *zhText, const char *enText) {
    return QString::fromUtf8(zh() ? zhText : enText);
}

QString I18n::pick(const QString &zhText, const QString &enText) {
    return zh() ? zhText : enText;
}

void I18n::setLang(Lang l) {
    if (l == m_lang) return;
    m_lang = l;
    emit languageChanged();
}
