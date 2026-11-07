#include "I18n.h"

I18n::I18n()
    : m_lang(QLocale::system().language() == QLocale::Chinese ? Lang::Chinese : Lang::English) {}

I18n &I18n::instance() {
    static I18n i;
    return i;
}

bool I18n::zh() { return instance().m_lang == Lang::Chinese; }

QString I18n::s(const char *zhText, const char *enText) {
    return QString::fromUtf8(zh() ? zhText : enText);
}

QString I18n::pick(const QString &zhText, const QString &enText) {
    return zh() ? zhText : enText;
}

void I18n::set(Lang l) {
    if (l == m_lang) return;
    m_lang = l;
    emit languageChanged();
}

void I18n::toggle() {
    set(m_lang == Lang::Chinese ? Lang::English : Lang::Chinese);
}
