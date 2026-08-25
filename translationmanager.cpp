#include "translationmanager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSettings>
#include <QLocale>

/*------------------------------------------------------\
\   update the following to add new language:           /
/   1. m_supportedLanguages                             \
\   2. qt_add_translations() in CMakeLists.txt          /
/   3. TRANSLATIONS in ChessMD-master-static-build.pro  \
\------------------------------------------------------*/
const QVector<Language> TranslationManager::m_supportedLanguages = {
    {"en", "English"},
    {"fr", "Français"},
    {"zh", "中文"},
    {"es", "Español"},
    {"de", "Deutsch"},
    {"ru", "Русский"},
    {"pt", "Português"},
    {"hi", "हिन्दी"},
    {"tl", "Filipino"},
    {"id", "Bahasa"}
};

TranslationManager::TranslationManager() = default;

int TranslationManager::currentLanguage(){
    return m_curLangIndex;
}

bool TranslationManager::initializeLanguage()
{
    // attempt saved preference first
    QSettings settings;
    int curLangIndex = settings.value("language", -1).toInt();
    if (curLangIndex >= 0 && curLangIndex < m_supportedLanguages.size()){
        return setLanguage(curLangIndex);
    }

    // use system language for first-time launch or fallback
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &localeStr: uiLanguages) {
        QLocale sysLocale(localeStr);
        QString systemLangCode = QLocale::languageToCode(sysLocale.language());
        for (int i = 0; i < m_supportedLanguages.size(); i++) {
            if (systemLangCode == m_supportedLanguages[i].code) {
                return setLanguage(i);
            }
        }
    }

    return false;
}

TranslationManager& TranslationManager::instance()
{
    static TranslationManager translator;
    return translator;
}

const QVector<Language>& TranslationManager::supportedLanguages() const
{
    return m_supportedLanguages;
}

bool TranslationManager::setLanguage(int langIndex)
{
    if (langIndex < 0 || langIndex >= m_supportedLanguages.size()) {
        return false;
    }

    // if (m_curLangIndex == langIndex){
    //     return false;
    // }

    if (m_supportedLanguages[langIndex].code == "en"){
        if (m_translator) {
            QCoreApplication::removeTranslator(m_translator.get());
            m_translator.reset();
        }
        m_curLangIndex = langIndex;
        emit languageChanged(m_curLangIndex);
        return true;
    }

    auto tempTranslator = std::make_unique<QTranslator>();
    const QString resourcePath = QString(":/i18n/%1_%2.qm").arg(QCoreApplication::applicationName(), m_supportedLanguages[langIndex].code);

    if (!tempTranslator->load(resourcePath)){
        qWarning() << "Failed to load translation:" << m_supportedLanguages[langIndex].code << resourcePath;
        return false;
    }

    if (m_translator) {
        QCoreApplication::removeTranslator(m_translator.get());
    }

    m_translator = std::move(tempTranslator);
    QCoreApplication::installTranslator(m_translator.get());
    m_curLangIndex = langIndex;
    emit languageChanged(m_curLangIndex);

    return true;
}



