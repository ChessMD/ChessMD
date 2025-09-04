#ifndef OPENINGVIEWER_H
#define OPENINGVIEWER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QHash>
#include <QFile>
#include <QVector>
#include <QByteArray>
#include <QTableWidget>

#include "chessposition.h"

// for sorting purposes
class MoveListItem : public QTreeWidgetItem {
public:
    MoveListItem(QTreeWidget* parent) : QTreeWidgetItem(parent) {}
    
    bool operator<(const QTreeWidgetItem &other) const override {
        int column = treeWidget()->sortColumn();
        
        if (column == 0) {
            return text(column) < other.text(column);
        } 
        else if (column == 1) {
            // games count
            return data(column, Qt::UserRole).toInt() < other.data(column, Qt::UserRole).toInt();
        }
        else if (column == 2) {
            // win % 
            return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
        }
        
        return QTreeWidgetItem::operator<(other);
    }
};

struct PositionWinrate {
    int whiteWin;
    int blackWin;
    int draw;
};

enum GameResult {UNKNOWN, WHITE_WIN, DRAW, BLACK_WIN};

struct OpeningInfo {
    // given N positions (quint64 zobrist keys), zobristPositions coordinate compresses them into indices from 0...N-1
    // where draw[i] + blackWin[i] + whiteWin[i] gives the number of games played at that position from its corresponding compressed zobrist key
    // during lookup, use binary search + prefix sum to find range of corresponding gameIDs in O(logN+K), where K is the number of games that reached the position
    QVector<quint32> gameIDs;
    QVector<quint64> zobristPositions;
    QVector<int> prefixSum;
    QVector<int> insertedCount;
    QVector<int> whiteWin;
    QVector<int> blackWin;
    QVector<int> draw;

    bool serialize(const QString& path) const;
    bool deserialize(const QString& path);
};

class OpeningViewer : public QWidget
{
    Q_OBJECT
public:
    explicit OpeningViewer(QWidget *parent = nullptr);
    
    void updatePosition(const quint64 zobrist, QSharedPointer<ChessPosition> position, const QString moveText);
    QPair<PositionWinrate, int> getWinrate(const quint64 zobrist);

public slots:
    void onMoveSelected(QSharedPointer<NotationMove>& move);

signals:
    void moveClicked(const QString& move);
    void gameSelected(int gameId); 

private slots:
    void onNextMoveSelected(QTreeWidgetItem* item, int column);
    void onGameSelected(int row, int column);  
    
private:
    bool mOpeningBookLoaded = false;

    QVector<PGNGame> loadGameHeadersBatch(const QString &path, const QVector<quint32> &ids);
    bool ensureHeaderOffsetsLoaded(const QString &path);

    void addMoveToList(const QString& move, int games, float whitePct, float drawPct, float blackPct);
    void updateGamesList(const int openingIndex);

    OpeningInfo mOpeningInfo;

    QVector<quint64> mHeaderOffsets;
    bool mHeaderOffsetsLoaded = false;

    QLabel* mPositionLabel;
    QLabel* mStatsLabel;
    QLabel* mGamesLabel;  
    QTreeWidget* mMovesList;
    QTableWidget* mGamesList;  
    
    QString mCurrentPosition;
    int mTotalGames = 0;
};

extern const int MAX_GAMES_TO_SHOW;
extern const int MAX_OPENING_DEPTH;

#endif // OPENINGVIEWER_H
