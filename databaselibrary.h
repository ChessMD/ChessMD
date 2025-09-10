#ifndef DATABASELIBRARY_H
#define DATABASELIBRARY_H

#include "pgngame.h"

#include <QWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QListView>
#include <QStandardItemModel>

class ChessTabHost;

namespace Ui {
class DatabaseLibrary;
}

// DatabaseLibrary features an interactable view of database files
class DatabaseLibrary : public QWidget
{
    Q_OBJECT

public:
    explicit DatabaseLibrary(QWidget *parent = nullptr);
    ~DatabaseLibrary();

    void LoadGamesList();

    void AddNewGame(QString file_name);

public slots:
    void importDatabase();
    void newDatabase();
    void newChessboard(PGNGame game, bool startGameReview = false);
    void newGameplayBoard();

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

    ChessTabHost *host;

    void addDatabase();
    int getFileNameRow(QString file_name);
};

#endif // DATABASELIBRARY_H
