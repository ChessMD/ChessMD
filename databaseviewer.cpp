#include "databaseviewer.h"
#include "ui_databaseviewer.h"

#include "pgnuploader.h"
#include "tabledelegate.h"
#include "pgngamedata.h"
#include "helpers.h"
#include "databasefilter.h"

#include <fstream>
#include <vector>
#include <QResizeEvent>


DatabaseViewer::DatabaseViewer(QWidget *parent)
    : QWidget(parent)
    , dbView(new QTableView(this))
    , ui(new Ui::DatabaseViewer)
{
    ui->setupUi(this);
    connect(ui->AddButton, &QPushButton::released, this, &DatabaseViewer::addEntry);
    connect(ui->FilterButton, &QPushButton::released, this, &DatabaseViewer::filter);
    connect(ui->ContentLayout, &QSplitter::splitterMoved, this, &DatabaseViewer::resizeTable);

    dbView->setSelectionBehavior(QAbstractItemView::SelectRows);
    dbView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Signal when a row is double clicked and open a board
    connect(dbView, &QAbstractItemView::doubleClicked, this, &DatabaseViewer::onTableActivated);

    dbModel = new DatabaseViewerModel(this);
    proxyModel = new DatabaseFilterProxyModel(parent);
    proxyModel->setSourceModel(dbModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);


    dbView->setModel(proxyModel);
    dbView->setSortingEnabled(true);



    ui->ContentLayout->insertWidget(0, dbView);

    dbView->setItemDelegate(new TableDelegate(this));
    dbView->setStyleSheet(getStyle("C:/Users/guana/Stuff/ChessMD/styles/tablestyle.qss"));
    dbView->verticalHeader()->setVisible(false);
    dbView->setShowGrid(false);


}

DatabaseViewer::~DatabaseViewer()
{
    delete ui;
}

void DatabaseViewer::resizeEvent(QResizeEvent *event){
    resizeTable();
}

void DatabaseViewer::showEvent(QShowEvent *event) {
    //force render on show
    QWidget::showEvent(event);
    resizeTable();
}

void DatabaseViewer::resizeTable(){
    const float widths[9] = {0.1, 0.15, 0.1, 0.15, 0.1, 0.1, 0.05, 0.15, 0.1};
    QList window_width = this->ui->ContentLayout->sizes();

    for(int i = 0; i < 9; i++){
        dbView->setColumnWidth(i, window_width.front()*widths[i]);
    }
}


void DatabaseViewer::addEntry(){
    PGNUploader upload(this);

    if(upload.exec() == QDialog::Accepted){
        QString file_name = upload.getFileName();

        std::ifstream file(file_name.toStdString());
        if(file.fail()) return;

        // Parse PGN and get headers
        StreamParser parser(file);
        std::vector<PGNGameData> database = parser.parseDatabase();

        for(auto &game: database){
            if(game.headerInfo.size() > 0){
                int row = dbModel->rowCount();

                dbModel->insertRow(row);
                // dbModel->addGame(game);
                for(int h = 0; h < game.headerInfo.size(); h++){
                    if(DATA_ORDER[h] > -1){
                        QModelIndex index = dbModel->index(row, DATA_ORDER[h]);
                        dbModel->setData(index, QString::fromStdString(game.headerInfo[h].second));
                    }
                }
            } else {
                qDebug() << "Error: no game found!";
            }
        }
    }
}

void DatabaseViewer::filter(){

    DatabaseFilter filterWindow(this);

    if(filterWindow.exec() == QDialog::Accepted){
        auto filters = filterWindow.getNameFilters();
        proxyModel->setTextFilter("Black", QString("^(?=.*%1)(?=.*%2).*").arg(filters.blackFirst, filters.blackLast));
        proxyModel->setTextFilter("White", QString("^(?=.*%1)(?=.*%2).*").arg(filters.whiteFirst, filters.whiteLast));
        proxyModel->setRangeFilter("Elo", filters.eloMin, filters.eloMax);
    }
}

void DatabaseViewer::onTableActivated(const QModelIndex &proxyIndex) {
    if (!proxyIndex.isValid())
        return;

    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    int row = sourceIndex.row();

    const PGNGameData& game = dbModel->getGame(row);
    qDebug() << game.headerInfo.front().first;
    emit gameActivated(game);
}


