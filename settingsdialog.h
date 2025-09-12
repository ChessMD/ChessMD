#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QString>
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <QProgressBar>

class QListWidget;
class QStackedWidget;
class QLabel;
class QPushButton;
class QComboBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    QString getOpeningsPath() const;

private slots:
    void onLoadPgnClicked();
    void onSelectEngineClicked();
    void onThemeChanged();
    void onDownloadLinkReply(QNetworkReply *reply);

private:
    void importPgnFileStreaming(const QString &file, QProgressBar *progressBar);
    void reportProgress(qint64 bytesRead, qint64 total, QProgressBar *progressBar);

    QListWidget* mCategoryList;
    QStackedWidget* mStackedWidget;
    QLabel* mOpeningsPathLabel;
    QLabel* mEnginePathLabel;
    QComboBox* mThemeComboBox;
    QString mOpeningsPath;

    QLabel *mDownloadLinkLabel;
};

#endif // SETTINGSDIALOG_H
