#include <vector>
#include <iostream>
#include <istream>

#include "pgngame.h"

class StreamParser
{

public:
    explicit StreamParser(std::istream &stream) : streamBuffer(stream) {}
    std::vector<PGNGame> parseDatabase();
    
private:
    std::istream &streamBuffer;
};

void parseBodyText(QString &bodyText, QSharedPointer<NotationMove> &rootMove, bool openingCutoff = false, const QString &fenHeader = "");
void parseGameFromPGN(PGNGame &game, bool openingCutoff = false);
bool isHeaderLine(const std::string &line);
