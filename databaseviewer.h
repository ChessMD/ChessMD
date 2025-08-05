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
#include <QStringList>
#include <QTimer>

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

signals:
    void saveRequested(const QString &filePath, const QVector<PGNGame> &database);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void filter();
    void saveColumnRatios();
    void addGame();
    void onPGNGameUpdated(PGNGame &game);
    void onDoubleSelected(const QModelIndex &proxyIndex);
    void onSingleSelected(const QModelIndex &current, const QModelIndex &previous);
    void onContextMenu(const QPoint &pos);
    void onHeaderContextMenu(const QPoint &pos);

private:
    void resizeTable();
    void resizeSplitter();

    Ui::DatabaseViewer *ui;

    ChessGameWindow *m_embed;
    QTableView *dbView;
    DatabaseViewerModel *dbModel;
    DatabaseFilterProxyModel *proxyModel;

    QStringList mShownHeaders;
    QTimer *mSaveTimer;

    QVector<float> mRatios;

    ChessTabHost *host;

    QString m_filePath;
};

#endif // DATABASEVIEWER_H
