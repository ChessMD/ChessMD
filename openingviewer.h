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
#include <QStyledItemDelegate>
#include <QPainter>

#include "chessposition.h"

class ResultBarDelegate : public QStyledItemDelegate
{
public:
    ResultBarDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    void paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

enum GameResult {UNKNOWN, WHITE_WIN, DRAW, BLACK_WIN};

struct PositionWinrate {
    int whiteWin;
    int blackWin;
    int draw;
};

class OpeningInfo
{
public:
    // given N positions (quint64 zobrist keys), zobristPositions coordinate compresses them into indices from 0...N-1
    // where draw[i] + blackWin[i] + whiteWin[i] gives the number of games played at that position from its corresponding compressed zobrist key
    // during lookup, use binary search + prefix sum to find range of corresponding gameIDs in O(logN+K), where K is the number of games that reached the position
    QVector<quint32> gameIDs;
    QVector<quint64> zobristPositions;
    QVector<int> startIndex;
    QVector<int> insertedCount;
    QVector<int> whiteWin;
    QVector<int> blackWin;
    QVector<int> draw;

    struct PositionInfo {
        quint32 insertedCount;
        quint32 whiteWin;
        quint32 blackWin;
        quint32 draw;
        quint32 startIndex;
    };

    bool serialize(const QString& path) const;
    bool deserialize(const QString& path);

    bool mapDataFile();
    void unmapDataFile();

    QPair<PositionWinrate, int> getWinrate(const quint64 zobrist);
    QVector<quint32> readGameIDs(int openingIndex);

private:
    QString m_dataFilePath;
    quint64 m_gameIdsDataStart = 0;
    quint64 m_positionInfoStart = 0;

    QFile m_mappedFile;
    const uchar *m_mappedBase = nullptr;
    qint64 m_mappedSize = 0;
    const quint64* m_zobristBase = nullptr;
    int m_nPositions = 0;
};

class OpeningViewer : public QWidget
{
    Q_OBJECT
public:
    explicit OpeningViewer(QWidget *parent = nullptr);
    
    void updatePosition(const quint64 zobrist, QSharedPointer<ChessPosition> position, const QString moveText);

public slots:
    void onMoveSelected(QSharedPointer<NotationMove>& move);

signals:
    void moveClicked(const SimpleMove& moveData);
    void gameSelected(int gameId); 

private slots:
    void onNextMoveSelected(QTableWidgetItem* item);
    void onGameSelected(QTableWidgetItem* item);
    
private:
    bool mOpeningBookLoaded = false;

    QVector<PGNGame> loadGameHeadersBatch(const QString &path, const QVector<quint32> &ids);
    bool ensureHeaderOffsetsLoaded(const QString &path);

    void addMoveToList(const QString& move, int games, float whitePct, float drawPct, float blackPct, SimpleMove moveData);
    void updateGamesList(const int openingIndex, const PositionWinrate winrate);

    OpeningInfo mOpeningInfo;

    QVector<quint64> mHeaderOffsets;
    bool mHeaderOffsetsLoaded = false;

    QLabel* mPositionLabel;
    QLabel* mStatsLabel;
    QLabel* mGamesLabel;  
    QTableWidget* mMovesList;
    QTableWidget* mGamesList;  
    
    QString mCurrentPosition;
    int mTotalGames = 0;
};

extern const int MAX_GAMES_TO_SHOW;
extern const int MAX_OPENING_DEPTH;

#endif // OPENINGVIEWER_H
