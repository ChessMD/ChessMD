#ifndef OPENINGVIEWER_H
#define OPENINGVIEWER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QPushButton>
#include <QProgressBar>


class OpeningTree{
public:
    



};

class OpeningViewer : public QWidget
{
    Q_OBJECT
public:
    explicit OpeningViewer(QWidget *parent = nullptr);
    
    void updatePosition(const QString& fen);
    
signals:
    void moveClicked(const QString& move);
    
private slots:
    void onMoveSelected(QTreeWidgetItem* item, int column);
    
private:
    // ui
    void updateMovesList();
    void addMoveToList(const QString& move, int games, double winPercentage, const QString& score);
    
    QLabel* positionLabel;
    QLabel* statsLabel;
    QTreeWidget* movesList;  
    
    QString currentPosition;
    int totalGames = 0;
};

#endif // OPENINGVIEWER_H
