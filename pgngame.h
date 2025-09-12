/*
March 5, 2025: File Creation
March 18, 2025: Completed PGN Parsing
April 20, 2025: Overhauled C++ headers with Qt framework
*/

#ifndef PGNGAME_H
#define PGNGAME_H

#include "notation.h"

class PGNGame
{
public:
    PGNGame();
    void copyFrom(PGNGame &other);
    QString serializePGN();
    static bool serializeHeaderData(const QString &path, const std::vector<PGNGame> &games);

    QSharedPointer<NotationMove> rootMove;
    QVector<QPair<QString,QString>> headerInfo;
    QString result;
    QString bodyText;
    int dbIndex;
    bool isParsed;
};

#endif // PGNGAME_H
