/*
March 5, 2025: File Creation
Narch 18, 2025: Completed PGN Parsing V1
*/

#include <iostream>

#include <qDebug>

#include "pgngamedata.h"


void PGNGameData::addHeader(const std::string &tag, const std::string &value){
    headerInfo.push_back(std::make_pair(tag, value));
}

void PGNGameData::printHeader(){
    for (auto &tagPair: headerInfo){
        qDebug() << tagPair.first << " " << tagPair.second << "\n";
    }
}

void dfsPrint(const VariationNode &curVariation, int curPly){
    int variationIndex = 0;
    qDebug() << "\n";
    for (int i = 0; i < curVariation.plyCount; i++){
        qDebug() << curVariation.moves[i] << " ";
        while (variationIndex < curVariation.variations.size() && curVariation.variations[variationIndex].first == i){
            dfsPrint(*curVariation.variations[variationIndex].second, curPly);
            variationIndex++;
        }
    }
    qDebug() << "\n";
}

void PGNGameData::printGameTree(){
    if (PGNGameData::getRootVariation() != nullptr){
        dfsPrint(*PGNGameData::getRootVariation(), 0);
    } else {
        qDebug() << "Error: rootVariation is not initialized!\n";
    }
}
