/*
March 5, 2025: File Creation
March 18, 2025: Completed PGN Parsing
April 20, 2025: Overhauled C++ headers with Qt framework
*/

#ifndef PGNGAMEDATA_H
#define PGNGAMEDATA_H

#include "notation.h"

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
    QString bodyText;
    QVector<QPair<QString,QString>> headerInfo;
private:
    QSharedPointer<VariationNode> rootVariation;
};

class PGNGame
{
public:
    PGNGame();
    void copyFrom(PGNGame &other);
    QString serializePGN();

    QSharedPointer<NotationMove> rootMove;
    QVector<QPair<QString,QString>> headerInfo;
    QString result;
    QString bodyText;
    int dbIndex;
    bool isParsed;
};

#endif // PGNGAMEDATA_H
