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

//using quint16 etc. so its ez to count space for mmap offset

struct Continuation {
    quint16 moveCode;
    quint32 count;
    float whitePct;
    float drawPct;
    float blackPct;
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
    
    void updatePosition(const QVector<QString>& uciMoves);

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
