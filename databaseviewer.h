#ifndef DATABASEVIEWER_H
#define DATABASEVIEWER_H

#include "chessgamewindow.h"
#include "pgngamedata.h"
#include "streamparser.h"
#include "databaseviewermodel.h"
#include "databasefilterproxymodel.h"
#include "pgngamedata.h"

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

    explicit DatabaseViewer(QString filePath, QWidget *parent = nullptr);
    ~DatabaseViewer();

    void importPGN();
    void exportPGN();
    void setWindowTitle(QString text);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void filter();
    void addGame();
    void onPGNGameUpdated(PGNGame &game);
    void onDoubleSelected(const QModelIndex &proxyIndex);
    void onSingleSelected(const QModelIndex &current, const QModelIndex &previous);
    void onContextMenu(const QPoint &pos);

private:
    void resizeTable();

    int DATA_ORDER[13] = {7, -1, 8, -1, 1, 3, 5, -1, 2, 4, 6, -1, -1};
    Ui::DatabaseViewer *ui;

    ChessGameWindow *m_embed;
    QTableView *dbView;
    DatabaseViewerModel *dbModel;
    DatabaseFilterProxyModel *proxyModel;

    ChessTabHost *host;

    QString m_filePath;
};

#endif // DATABASEVIEWER_H
