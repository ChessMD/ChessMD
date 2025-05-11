#ifndef DATABASEVIEWER_H
#define DATABASEVIEWER_H

#include "pgngamedata.h"
#include "streamparser.h"
#include "databaseviewermodel.h"
#include "databasefilterproxymodel.h"


#include <QTextEdit>
#include <QSortFilterProxyModel>

#include <QWidget>
#include <QTableWidget>
#include <QHeaderView>

class ChessTabHost;

namespace Ui {
class DatabaseViewer;
}

// DatabaseViewer class creates and handles a chess database table.
class DatabaseViewer : public QWidget
{
    Q_OBJECT

public:

    explicit DatabaseViewer(QWidget *parent = nullptr);
    ~DatabaseViewer();

    void addGame(QString file_name);
    void setWindowTitle(QString text);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onTableActivated(const QModelIndex &proxyIndex);
    void onTableSelected(const QModelIndex &current, const QModelIndex &previous);

private:
    void filter();
    void resizeTable();
    int DATA_ORDER[13] = {7, -1, 8, -1, 1, 3, 5, -1, 2, 4, 6, -1, -1};
    Ui::DatabaseViewer *ui;

    QTableView *dbView;
    DatabaseViewerModel *dbModel;
    DatabaseFilterProxyModel *proxyModel;

    ChessTabHost *host;

};

#endif // DATABASEVIEWER_H
