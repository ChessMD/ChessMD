#ifndef DATABASELIBRARY_H
#define DATABASELIBRARY_H

#include <QWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QListView>
#include <QStandardItemModel>

class ChessTabHost;

namespace Ui {
class DatabaseLibrary;
}

class DatabaseLibrary : public QWidget
{
    Q_OBJECT

public:
    explicit DatabaseLibrary(QWidget *parent = nullptr);
    ~DatabaseLibrary();

    void LoadGamesList();

    void AddNewGame(QString file_name);

signals:
    void fileDoubleClicked(const QString &fileIdentifier);

private slots:
    void onDoubleClick(const QModelIndex &index);
    void onClick(const QModelIndex &index);
    void showContextMenu(const QPoint& pos);

private:
    Ui::DatabaseLibrary *ui;


    QListView *listView;
    QStandardItemModel *model;

    QWidget* m_parent;

    ChessTabHost *host;

    void addDatabase();
    int getFileNameRow(QString file_name);
};

#endif // DATABASELIBRARY_H
