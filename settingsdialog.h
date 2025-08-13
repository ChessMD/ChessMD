#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QString>

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

private:
    QListWidget* mCategoryList;
    QStackedWidget* mStackedWidget;
    QLabel* mOpeningsPathLabel;
    QLabel* mEnginePathLabel;
    QComboBox* mThemeComboBox;
    QString mOpeningsPath;
};

#endif // SETTINGSDIALOG_H
