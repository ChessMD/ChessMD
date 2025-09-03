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

// using quint16 etc. so its ez to count space for mmap offset

struct Continuation {
    quint16 moveCode;
    quint32 count;
    float whitePct;
    float drawPct;
    float blackPct;
};

struct PositionWinrate {
    int whiteWin;
    int blackWin;
    int draw;
};

enum GameResult {UNKNOWN, WHITE_WIN, DRAW, BLACK_WIN};

class OpeningTree{
public:
    OpeningTree();
    ~OpeningTree();

    void insertGame(const QVector<quint16>& moves, int gameID, GameResult result);
    bool serialize(const QString& path);

    bool load(const QString& file);
    void reset();
    bool play(quint16 moveCode);

    quint32 gamesReached() const;
    QVector<Continuation> continuations() const;
    QVector<int> getIds() const;

    QMap<quint64, QVector<quint32>> openingGameMap;
    QMap<quint64, PositionWinrate> openingWinrateMap;

    // given N positions (quint64 zobrist keys), zobristPositions coordinate compresses them into indices from 0...N-1
    // where draw[i] + blackWin[i] + whiteWin[i] gives the number of games played at that position from its corresponding compressed zobrist key
    // during lookup, use binary search + prefix sum to find range of corresponding gameIDs in O(logN+K), where K is the number of games that reached the position
    struct OpeningInfo {
        QVector<quint32> gameIDs;
        QVector<quint64> zobristPositions;
        QVector<int> prefixSum;
        QVector<int> whiteWin;
        QVector<int> blackWin;
        QVector<int> draw;

        bool serialize(const QString& path) const {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) return false;
            QDataStream out(&file);
            out.setVersion(QDataStream::Qt_6_5); // set version for compatibility

            // write arrays
            out << gameIDs;
            out << zobristPositions;
            out << whiteWin;
            out << blackWin;
            out << draw;

            return out.status() == QDataStream::Ok;
        }

        bool deserialize(const QString& path) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) return false;
            QDataStream in(&file);
            in.setVersion(QDataStream::Qt_6_5);

            in >> gameIDs;
            in >> zobristPositions;
            in >> whiteWin;
            in >> blackWin;
            in >> draw;

            return in.status() == QDataStream::Ok;
        }
    };

    OpeningInfo mOpeningInfo;

private:
    struct BuildNode {
        quint32 gamesReached = 0;
        quint32 whiteWins = 0;
        quint32 draws = 0;
        QVector<QPair<quint16, BuildNode*>> children;
        QVector<int> gameIds;
    };

    //pack so it is strictly 14 bytes
    #pragma pack(push,1)
    struct ChildEntry {
        quint16 moveCode;  
        quint32 count;
        quint32 whiteWins;
        quint32 draws;    
        quint64 offset;   
    };
    #pragma pack(pop)
    struct NodeView{
        quint32 gamesReached;
        quint32 whiteWins;
        quint32 draws;
        quint8 childCount;
        const ChildEntry* children;
    };

    BuildNode* mRoot;
    QVector<BuildNode*> mBfsOrder;
    QHash<BuildNode*, quint64> mOffsets;

    QFile mFile;
    const char* mMappedBase;
    quint64 mMappedSize;
    quint64 mCurOffset;

    //helpers
    void deleteSubtree(BuildNode*);
    void assignOffsets();
    void collectGameIds(quint64 nodeOffset, QVector<int>& ids) const;
    NodeView readNode(quint64 offset) const;
};

class OpeningViewer : public QWidget
{
    Q_OBJECT
public:
    explicit OpeningViewer(QWidget *parent = nullptr);
    
    void updatePosition(const quint64 zobrist, QSharedPointer<ChessPosition> position);
    PositionWinrate getWinrate(const quint64 zobrist);

    // static helpers
    static quint16 encodeMove(const QString& uciCode);
    static QString decodeMove(quint16 code);

signals:
    void moveClicked(const QString& move);
    void gameSelected(int gameId); 

    
private slots:
    void onMoveSelected(QTreeWidgetItem* item, int column);
    void onGameSelected(int row, int column);  

    
private:
    bool mOpeningBookLoaded = false;
    // ui
    void addMoveToList(const QString& move, int games, float whitePct, float drawPct, float blackPct);
    void updateGamesList();

    OpeningTree mTree;

    QLabel* mPositionLabel;
    QLabel* mStatsLabel;
    QLabel* mGamesLabel;  
    QTreeWidget* mMovesList;
    QTableWidget* mGamesList;  
    
    QString mCurrentPosition;
    int mTotalGames = 0;
};

#endif // OPENINGVIEWER_H
