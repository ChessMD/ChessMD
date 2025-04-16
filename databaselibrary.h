#ifndef DATABASELIBRARY_H
#define DATABASELIBRARY_H

#include <QWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QListView>
#include <QStandardItemModel>

namespace Ui {
class DatabaseLibrary;
}

class DatabaseLibrary : public QWidget
{
    Q_OBJECT

public:
    explicit DatabaseLibrary(QWidget *parent = nullptr);
    ~DatabaseLibrary();

signals:
    void fileDoubleClicked(const QString &fileIdentifier);

private slots:
    void onDoubleClick(const QModelIndex &index);

private:
    Ui::DatabaseLibrary *ui;


    QListView *listView;
    QStandardItemModel *model;

    void addDatabase();
};

#endif // DATABASELIBRARY_H
