/*
March 3, 2025: File Creation
April 20, 2025: Overhauled C++ headers with Qt framework
*/

#include <string>
#include <QDebug>

#include "streamparser.h"
#include "pgngame.h"
#include "chessposition.h"

bool isHeaderLine(const std::string &line) {
    size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) ++i;
    if (i >= line.size()) return false;
    if (line[i] != '[') return false;
    ++i;
    size_t tagStart = i;
    while (i < line.size() && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_')) ++i;
    if (i == tagStart) return false; // no tag characters
    if (i >= line.size() || !(line[i] == ' ' || line[i] == '\t')) return false;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    if (i >= line.size()) return false;
    if (line[i] != '"') return false;
    ++i;
    while (i < line.size() && line[i] != '"') ++i;
    if (i >= line.size()) return false; // no closing quote
    ++i;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) ++i;
    if (i >= line.size()) return false;
    if (line[i] != ']') return false;
    ++i;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) ++i;
    return i == line.size();
}


void parseBodyText(QString &bodyText, QSharedPointer<NotationMove> &rootMove, bool openingCutoff){
    rootMove->m_position->setBoardData(convertFenToBoardData(rootMove->FEN));
    parseBodyAndBuild(bodyText, rootMove, openingCutoff);
}

std::vector<PGNGame> StreamParser::parseDatabase(){
    std::vector<PGNGame> database;
    std::string bufferedLine;
    bool hasBufferedLine = false;

    // Text files contain BOM indicators which should be skipped
    char c;
    while ((c = streamBuffer.peek()) != EOF && c != '[') {
        streamBuffer.get();
    }

    // PGN files can have any number of games, continue parsing until end of file
    while(!streamBuffer.eof()){
        std::string bodyText;
        PGNGame game;

        // Get PGN header information which is formatted as [<string> "<string>"]
        while (streamBuffer.peek() != EOF){
            QString tag, value;
            std::string line;
            if (hasBufferedLine){
                line = bufferedLine;
                hasBufferedLine = false;
            } else {
                std::getline(streamBuffer, line);
            }
            if(!isHeaderLine(line)){
                break;
            }

            auto c = line.begin();
            // Get header tag
            c++;
            while (c != line.end() && *c != '"'){
                tag += *c;
                c++;
            }

            // Remove extra whitespace in tag
            if (!tag.isEmpty() && tag.back() == ' '){
                tag.chop(1);
            }

            // Get header value
            if (c != line.end()) c++;
            while (c != line.end() && *c != '"'){
                value += *c;
                c++;
            }

            if (tag == "Result"){
                game.result = value;
            }

            game.headerInfo.push_back({tag, value});
        }

        // read body text until next header or EOF
        while (streamBuffer.peek() != EOF){
            std::string line;
            std::getline(streamBuffer, line);
            if (isHeaderLine(line)) {
                bufferedLine = line;
                hasBufferedLine = true;
                break;
            }
            bodyText += line + " ";
        }

        game.bodyText = QString::fromStdString(bodyText);
        database.push_back(std::move(game));
    }

    return database;
}
