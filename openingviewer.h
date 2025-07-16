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

//using quint16 etc. so its ez to count space for mmap offset

struct Continuation {
    //optimal space
    quint16 moveCode;
    quint32 count;
};



class OpeningTree{
public:
    OpeningTree();
    ~OpeningTree();

    void insertGame(const QVector<quint16>& moves);

    bool serialize(const QString& path);

    bool load(const QString& file);
    void reset();
    bool play(quint16 moveCode);
    quint32 gamesReached() const;
    QVector<Continuation> continuations() const;

private:
    struct BuildNode {
        quint32 gamesReached = 0;
        QVector<QPair<quint16, BuildNode*>> children;
    };

    //pack so it is strictly 14 bytes
    #pragma pack(push,1)
    struct ChildEntry {
        quint16 moveCode;  
        quint32 count;    
        quint64 offset;   
    };
    #pragma pack(pop)
    struct NodeView{
        quint32 gamesReached;
        quint32 childCount;
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
    NodeView readNode(quint64 offset) const;
};

class OpeningViewer : public QWidget
{
    Q_OBJECT
public:
    explicit OpeningViewer(QWidget *parent = nullptr);
    
    void updatePosition(const QVector<QString>& uciMoves);
    
signals:
    void moveClicked(const QString& move);
    
private slots:
    void onMoveSelected(QTreeWidgetItem* item, int column);
    
private:
    bool mOpeningBookLoaded = false;
    // ui
    void addMoveToList(const QString& move, int games, double winPercentage, const QString& score);

    // static helpers
    static quint16 encodeMove(const QString& uciCode);
    static QString decodeMove(quint16 code);
    
    OpeningTree mTree;

    QLabel* mPositionLabel;
    QLabel* mStatsLabel;
    QTreeWidget* mMovesList;  
    
    QString mCurrentPosition;
    int mTotalGames = 0;
};

#endif // OPENINGVIEWER_H
