#include "openingviewer.h"

#include <QHeaderView>


OpeningViewer::OpeningViewer(QWidget *parent)
    : QWidget{parent}
{
    //
    //ui
    //

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(4);

    positionLabel = new QLabel("Starting Position");
    positionLabel->setStyleSheet("font-weight: bold; font-size: 13px;");

    statsLabel = new QLabel("No position data");
    statsLabel->setStyleSheet("font-size: 12px;"); 

    movesList = new QTreeWidget();
    movesList->setHeaderLabels(QStringList() << "Move" << "Games" << "Win %" << "Score");
    movesList->setRootIsDecorated(false);
    movesList->setAlternatingRowColors(true);
    movesList->setSortingEnabled(true);
    movesList->sortByColumn(1, Qt::DescendingOrder);
    movesList->setMinimumHeight(150);

    QHeaderView* header = movesList->header();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setStretchLastSection(true);
    
    //styles
    movesList->setStyleSheet(R"(
        QTreeWidget {
            border: 1px solid palette(mid);
            border-radius: 3px;
        }
        QTreeWidget::item {
            height: 22px; /* Consistent row height */
        }
        QTreeWidget::item:hover {
            background: palette(highlight);
            color: palette(highlighted-text);
            opacity: 0.5;
        }
        QTreeWidget::item:selected {
            background: palette(highlight);
            color: palette(highlighted-text);
        }
    )");
    
    mainLayout->addWidget(positionLabel);
    mainLayout->addWidget(statsLabel);
    mainLayout->addWidget(movesList);
    
    connect(movesList, &QTreeWidget::itemDoubleClicked, this, &OpeningViewer::onMoveSelected);
}

void OpeningViewer::updatePosition(const QString& fen)
{
    currentPosition = fen;
    
    // shorten fen
    QString shortFen = fen.split(" ").at(0);
    positionLabel->setText(shortFen);
    
    updateMovesList();
}

void OpeningViewer::updateMovesList()
{
    movesList->clear();
    
    //placeholder
    totalGames = 3450; 
    
    statsLabel->setText(QString("Games in database: %1").arg(totalGames));
    
    //placeholder
    addMoveToList("e4", 1720, 55.2, "0-0-0");
    addMoveToList("d4", 1240, 52.8, "0-0-0");
    addMoveToList("c4", 310, 49.1, "0-0-0");
    addMoveToList("Nf3", 180, 51.3, "0-0-0");
}

//helper
void OpeningViewer::addMoveToList(const QString& move, int games, double winPercentage, const QString& score)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(movesList);
    
    item->setText(0, move);
    item->setText(1, QString::number(games));
    item->setText(2, QString::number(winPercentage, 'f', 1) + "%");
    item->setText(3, score);
    
    // store move
    item->setData(0, Qt::UserRole, move);
    

}

void OpeningViewer::onMoveSelected(QTreeWidgetItem* item, int column)
{
    if (!item) return;
    
    QString move = item->data(0, Qt::UserRole).toString();
    emit moveClicked(move);
}
