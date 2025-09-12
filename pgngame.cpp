/*
March 5, 2025: File Creation
March 18, 2025: Completed PGN Parsing
April 20, 2025: Overhauled C++ headers with Qt framework
*/

#include <QDebug>
#include <QFile>

#include "pgngame.h"
#include "chessposition.h"

PGNGame::PGNGame()
{
    result = "*";
    bodyText = "";
    isParsed = false;
    ChessPosition startPos;
    rootMove = QSharedPointer<NotationMove>::create("", startPos);
    rootMove->m_zobristHash = rootMove->m_position->computeZobrist();
}

void PGNGame::copyFrom(PGNGame &other)
{
    headerInfo = other.headerInfo;
    result = other.result;
    bodyText = other.bodyText;
    dbIndex = other.dbIndex;
    isParsed = other.isParsed;
    rootMove = cloneNotationTree(other.rootMove);
}

QString PGNGame::serializePGN(){
    QString PGNtext;
    for (auto &kv: headerInfo){
        PGNtext += "[" + kv.first + " \"" + kv.second + "\"]\n";
    }
    PGNtext += "\n\n";
    PGNtext += bodyText;
    return PGNtext;
}

bool PGNGame::serializeHeaderData(const QString &path, const std::vector<PGNGame> &games)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    
    QDataStream out(&file);
    
    // game count
    out << quint32(games.size());
    
    // access by id
    quint64 indexTableSize = 4 + 8 * games.size(); 
    
    // reserve space for offsets
    for (size_t i = 0; i < games.size(); i++) {
        out << quint64(0); 
    }
    
    QVector<quint64> offsets;
    
    for (const PGNGame &game : games) {
        offsets.append(file.pos());
        
        QString white, whiteElo, black, blackElo, event, date, result;
        for (const auto &header : game.headerInfo) {
            if (header.first == "White") white = header.second;
            else if (header.first == "WhiteElo") whiteElo = header.second;
            else if (header.first == "Black") black = header.second;
            else if (header.first == "BlackElo") blackElo = header.second;
            else if (header.first == "Event") event = header.second;
            else if (header.first == "Date") date = header.second;
        }
        
        out << white << whiteElo << black << blackElo << event << date << game.result;
        out << game.bodyText;
    }
    
    // write offsets
    file.seek(4); 
    for (quint64 offset : offsets) {
        out << offset;
    }
    
    file.close();
    return true;
}
