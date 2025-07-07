/*
March 5, 2025: File Creation
March 18, 2025: Completed PGN Parsing
April 20, 2025: Overhauled C++ headers with Qt framework
*/

#include <qDebug>

#include "pgngamedata.h"
#include "chessposition.h"

PGNGame::PGNGame()
{
    result = "*";
    rootMove = QSharedPointer<NotationMove>::create("", *new ChessPosition);
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
