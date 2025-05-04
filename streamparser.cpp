/*
March 3, 2025: File Creation
April 20, 2025: Overhauled C++ headers with Qt framework
*/

#include <string>
#include <regex>

#include <qDebug>

#include "streamparser.h"
#include "pgngamedata.h"

void skipWhitespace(std::istream &streamBuffer){
    // Ignore extra whitespace
    while (streamBuffer.peek() == '\n'){
        streamBuffer.ignore();
    }
}

void dfsParse(std::istream &streamBuffer, const QSharedPointer<VariationNode> &curVariation){
    int plyCount = 0;
    char c;
    bool terminated = false;
    QString token;

    while(!terminated && streamBuffer.get(c)){
        // Create tokens separated by spaces
        if (!isspace(c) && c != ')' && c != '('){
            token += c;
        }

        if (c == '('){
            QSharedPointer<VariationNode> newVariation = QSharedPointer<VariationNode>::create();
            curVariation->variations.append(qMakePair(plyCount, newVariation));
            dfsParse(streamBuffer, curVariation->variations.back().second);
        }

        if (c == ')'){
            terminated = true;
        }

        // Process token if current character is whitespace or EOF
        if (!token.isEmpty() && (std::isspace(c) || streamBuffer.peek() == EOF || c == ')')){
            // Check for standard game termination
            if (token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*"){
                terminated = true;
            }

            plyCount++;
            curVariation->moves.append(token);
            token.clear();
        }
    }

    curVariation->plyCount = plyCount;
    return;
}

std::vector<PGNGameData> StreamParser::parseDatabase(){
    std::vector<PGNGameData> database;
    skipWhitespace(streamBuffer);
    int gameNumber = 1;

    // Text files contain BOM indicators which should be skipped
    char c;
    while ((c = streamBuffer.peek()) != EOF && c != '[') {
        streamBuffer.get();
    }

    // PGN files can have any number of games, continue parsing until end of file
    while(!streamBuffer.eof()){
        std::string line;
        PGNGameData game;

        // Get PGN header information which is formatted as [<string> "<string>"]
        while (std::getline(streamBuffer, line) && !line.empty()){
            line = std::regex_replace(line, std::regex("^ +| +$|( ) +"), "$1");
            if (line[0] != '[') break;

            std::string tag, value;
            auto c = line.begin();

            // Get header tag
            c++;
            while (c != line.end() && *c != '"'){
                tag += *c;
                c++;
            }

            // Remove extra whitespace in tag
            if (!tag.empty() && tag.back() == ' '){
                tag.pop_back();
            }

            // Get header value
            c++;
            while (c != line.end() && *c != '"'){
                value += *c;
                c++;
            }

            // Remove leading, trailing, and extra spaces from header information
            // https://stackoverflow.com/questions/1798112/removing-leading-and-trailing-spaces-from-a-string
            tag = std::regex_replace(tag, std::regex("^ +| +$|( ) +"), "$1");
            value = std::regex_replace(value, std::regex("^ +| +$|( ) +"), "$1");

            game.addHeader(QString::fromStdString(tag), QString::fromStdString(value));
        }

        skipWhitespace(streamBuffer);

        // Parse body of pgn
        if (game.getRootVariation() != nullptr) {
            dfsParse(streamBuffer, game.getRootVariation());
        } else {
            qDebug() << "Error: rootVariation is not initialized!\n";
        }

        skipWhitespace(streamBuffer);
        gameNumber++;
        database.push_back(std::move(game));
    }

    return database;
}
