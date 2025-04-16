#ifndef CHESSPOSITION_H
#define CHESSPOSITION_H

#include <QString>
#include <QVector>
#include <QObject>

class ChessPosition: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVector<QVector<QString>> boardData READ boardData WRITE setBoardData NOTIFY boardDataChanged)

public:
    explicit ChessPosition(QObject *parent = nullptr);

    QVector<QVector<QString>> boardData() const;
    void setBoardData(const QVector<QVector<QString>> &data);

    Q_INVOKABLE void release(int oldRow, int oldCol, int newRow, int newCol);
    bool validateMove(int oldRow, int oldCol, int newRow, int newCol);

    int plyCount;

signals:
    void boardDataChanged();

private:
    QVector<QVector<QString>> m_boardData;
};

QVector<QVector<QString>> convertFenToBoardData(const QString &fen);

#endif // CHESSPOSITION_H
