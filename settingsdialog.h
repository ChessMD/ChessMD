#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QString>

class QListWidget;
class QStackedWidget;
class QLabel;
class QPushButton;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    QString getOpeningsPath() const;

private slots:
    void onLoadPgnClicked();
    void onSelectEngineClicked();

private:
    QListWidget* mCategoryList;
    QStackedWidget* mStackedWidget;
    QLabel* mOpeningsPathLabel;
    QLabel* mEnginePathLabel;
    QString mOpeningsPath;
};

#endif // SETTINGSDIALOG_H
