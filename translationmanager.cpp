#include "translationmanager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSettings>

const QVector<Language> TranslationManager::m_supportedLanguages = {
    {"en", "English"},
    {"fr", "Français"},
    {"zh", "中文"}
};

TranslationManager::TranslationManager() = default;

bool TranslationManager::initializeLanguage()
{
    QSettings settings;

    int curLangIndex = settings.value("language").toInt();
    if (curLangIndex < 0 || curLangIndex >= m_supportedLanguages.size()) {
        m_curLangIndex = 0;
        return false;
    }
    setLanguage(curLangIndex);
    return true;
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

    if (m_curLangIndex == langIndex){
        return true;
    }

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



