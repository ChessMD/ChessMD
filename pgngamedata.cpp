/*
March 5, 2025: File Creation
March 18, 2025: Completed PGN Parsing
April 20, 2025: Overhauled C++ headers with Qt framework
*/

#include <QDebug>
#include <QFile>

#include "pgngamedata.h"
#include "chessposition.h"

PGNGame::PGNGame()
{
    result = "*";
    isParsed = false;
    ChessPosition startPos;
    rootMove = QSharedPointer<NotationMove>::create("", startPos);
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

bool PGNGame::serializeHeaderData(const QString &path, const std::vector<PGNGame> &games) {
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

// load game data
PGNGame PGNGame::loadGameHeader(const QString &path, int gameId) {
    PGNGame game;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return game;
    
    QDataStream in(&file);
    
    quint32 gameCount;
    in >> gameCount;
    
    if (gameId < 0 || static_cast<quint32>(gameId) >= gameCount) {
        qDebug() << "Game ID out of range:" << gameId;
        file.close();
        return game; 
    }
    
    // get requested game offset
    file.seek(4 + 8 * gameId);
    quint64 gameOffset;
    in >> gameOffset;
    
    file.seek(gameOffset);
    
    QString white, whiteElo, black, blackElo, event, date, result;
    in >> white >> whiteElo >> black >> blackElo >> event >> date >> result;
    
    if (!white.isEmpty()) game.headerInfo.push_back(qMakePair(QString("White"), white));
    if (!whiteElo.isEmpty()) game.headerInfo.push_back(qMakePair(QString("WhiteElo"), whiteElo));
    if (!black.isEmpty()) game.headerInfo.push_back(qMakePair(QString("Black"), black));
    if (!blackElo.isEmpty()) game.headerInfo.push_back(qMakePair(QString("BlackElo"), blackElo));
    if (!event.isEmpty()) game.headerInfo.push_back(qMakePair(QString("Event"), event));
    if (!date.isEmpty()) game.headerInfo.push_back(qMakePair(QString("Date"), date));
    if (!result.isEmpty()) {
        game.headerInfo.push_back(qMakePair(QString("Result"), result));
        game.result = result;
    }
    
    in >> game.bodyText;
    game.isParsed = false;
    
    file.close();
    return game;
}

PGNGameData::PGNGameData()
{
    rootVariation = QSharedPointer<VariationNode>::create();
}


void PGNGameData::addHeader(const QString &tag, const QString &value){
    headerInfo.push_back(qMakePair(tag, value));
}

void PGNGameData::printHeader(){
    for (auto &tagPair: headerInfo){
        qDebug() << tagPair.first << " " << tagPair.second << "\n";
    }
}

void dfsPrint(const QSharedPointer<VariationNode> &curVariation, int curPly){
    int variationIndex = 0;
    qDebug() << "\n";
    for (int i = 0; i < curVariation->plyCount; i++){
        qDebug() << curVariation->moves[i] << " ";
        while (variationIndex < curVariation->variations.size() && curVariation->variations[variationIndex].first == i){
            dfsPrint(curVariation->variations[variationIndex].second, curPly);
            variationIndex++;
        }
    }
    qDebug() << "\n";
}

void PGNGameData::printGameTree(){
    if (!rootVariation.isNull()){
        dfsPrint(rootVariation, 0);
    } else {
        qDebug() << "Error: rootVariation is not initialized!\n";
    }
}
