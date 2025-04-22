#ifndef PGNGAMEDATA_H
#define PGNGAMEDATA_H

#include <QVector>
#include <QString>
#include <QSharedPointer>
#include <QPair>

class VariationNode
{
public:
    QVector<QString> moves;
    QVector<QPair<int, QSharedPointer<VariationNode>>> variations;
    int plyCount;
};


class PGNGameData
{
public:
    PGNGameData();
    void addHeader(const QString &tag, const QString &value);
    void printHeader();
    void printGameTree();
    QSharedPointer<VariationNode> getRootVariation() const { return rootVariation; }
    QVector<QPair<QString,QString>> headerInfo;
private:
    QSharedPointer<VariationNode> rootVariation;
};

#endif // PGNGAMEDATA_H
