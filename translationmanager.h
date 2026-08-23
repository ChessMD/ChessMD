#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QObject>
#include <QTranslator>
#include <QString>
#include <QVector>

#include <memory>

struct Language
{
    QString code;
    QString name;

    bool operator==(const Language& other) const
    {
        return code == other.code;
    }
};

class TranslationManager : public QObject
{
    Q_OBJECT

public:
    static TranslationManager& instance();

    const QVector<Language>& supportedLanguages() const;
    bool setLanguage(int langIndex);
    bool initializeLanguage();

private:
    explicit TranslationManager();

    static const QVector<Language> m_supportedLanguages;

    std::unique_ptr<QTranslator> m_translator;
    int m_curLangIndex;

signals:
    void languageChanged(int langIndex);
};

#endif // TRANSLATIONMANAGER_H
