/* 
March 3, 2025: File Creation
April 20, 2025: Overhauled C++ headers with Qt framework
*/

#include <vector>
#include <iostream>
#include <istream>

#include "pgngamedata.h"

class StreamParser
{

public:
    explicit StreamParser(std::istream &stream) : streamBuffer(stream) {}
    std::vector<PGNGame> parseDatabaseOld();
    std::vector<PGNGame> parseDatabase();
    
    bool parseNextGame(PGNGame& game);
    void parseSimplifiedBodyText(const QString& bodyText, QVector<QString>& moves);

private:
    std::istream &streamBuffer;
    // bool skipToNextGame();
};

void parseBodyText(QString &bodyText, QSharedPointer<NotationMove> &rootMove);
