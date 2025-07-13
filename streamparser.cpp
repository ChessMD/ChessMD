/*
March 3, 2025: File Creation
April 20, 2025: Overhauled C++ headers with Qt framework
*/

#include <string>
#include <regex>

#include <qDebug>

#include "streamparser.h"
#include "pgngamedata.h"
#include "chessposition.h"

#include <QString>

// Ignore extra whitespace
void skipWhitespace(std::istream &streamBuffer){
    while (streamBuffer.peek() == '\n'){
        streamBuffer.ignore();
    }
}

void dfsParse(QString &bodyText, int &pos, QSharedPointer<VariationNode> &curVariation) {
    int plyCount = 0, n = bodyText.size();
    bool terminated = false;
    QString token;

    while (!terminated && pos < n) {
        QChar c = bodyText[pos++];

        // Create tokens separated by spaces and punctuation
        if (!c.isSpace() && c != ')' && c != '(') {
            token += c;
        }

        // Enter nested variation
        if (c == '(') {
            QSharedPointer<VariationNode> newVariation = QSharedPointer<VariationNode>::create();
            curVariation->variations.append(qMakePair(plyCount, newVariation));
            dfsParse(bodyText, pos, newVariation);
        }

        // Closing a variation
        if (c == ')') {
            terminated = true;
        }

        // Process token if delimiter or end-of-text
        if (!token.isEmpty() && (c.isSpace() || pos >= n || c == ')')) {
            // Check for game termination markers
            if (token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*") {
                terminated = true;
            } else {
                curVariation->moves.append(token);
                plyCount++;
            }
            token.clear();
        }
    }

    curVariation->plyCount = plyCount;
}

void parseBodyText(QString &bodyText, QSharedPointer<NotationMove> &rootMove){
    int pos = 0;
    QSharedPointer<VariationNode> rootVariation = QSharedPointer<VariationNode>::create();
    dfsParse(bodyText, pos, rootVariation);
    rootMove->m_position->setBoardData(convertFenToBoardData(rootMove->FEN));
    buildNotationTree(rootVariation, rootMove);
}

void dfsParseOld(std::istream &streamBuffer, const QSharedPointer<VariationNode> &curVariation){
    int plyCount = 0, terminated = 0;
    char c;
    QString token;

    while(!terminated && streamBuffer.get(c)){
        // Create tokens separated by spaces
        if (!isspace(c) && c != ')' && c != '('){
            token += c;
        }

        if (c == '('){
            QSharedPointer<VariationNode> newVariation = QSharedPointer<VariationNode>::create();
            curVariation->variations.append(qMakePair(plyCount, newVariation));
            dfsParseOld(streamBuffer, curVariation->variations.back().second);
        }

        if (c == ')'){
            terminated = 1;
        }

        // Process token if current character is whitespace or EOF
        if (!token.isEmpty() && (std::isspace(c) || streamBuffer.peek() == EOF || c == ')')){
            // Check for standard game termination
            if (token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*"){
                terminated = 1;
            } else {
                curVariation->moves.append(token); // Don't append result text
                plyCount++;
            }
            token.clear();
        }
    }

    curVariation->plyCount = plyCount;
    return;
}

std::vector<PGNGame> StreamParser::parseDatabase(){
    std::vector<PGNGame> database;
    int gameNumber = 0;

    // Text files contain BOM indicators which should be skipped
    char c;
    while ((c = streamBuffer.peek()) != EOF && c != '[') {
        qDebug() << c;
        streamBuffer.get();
    }

    // PGN files can have any number of games, continue parsing until end of file
    while(!streamBuffer.eof()){
        std::string line, bodyText;
        PGNGame game;

        // Get PGN header information which is formatted as [<string> "<string>"]
        while ((c = streamBuffer.peek()) != EOF && c == '['){
            QString tag, value;
            std::getline(streamBuffer, line);
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
            c++;
            while (c != line.end() && *c != '"'){
                value += *c;
                c++;
            }
            game.headerInfo.push_back({tag, value});
        }

        while ((c = streamBuffer.peek()) != EOF && c != '['){
            std::string line;
            std::getline(streamBuffer, line);
            bodyText += line + " ";
        }

        game.bodyText = QString::fromStdString(bodyText);
        gameNumber++;
        database.push_back(std::move(game));
    }

    return database;
}

std::vector<PGNGame> StreamParser::parseDatabaseOld(){
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
            dfsParseOld(streamBuffer, game.getRootVariation());
        } else {
            qDebug() << "Error: rootVariation is not initialized!\n";
        }

        skipWhitespace(streamBuffer);
        gameNumber++;
        database.push_back(std::move(game));
    }

    std::vector<PGNGame> PGNdatabase;
    for (int i = 0; i < database.size(); i++){
        QSharedPointer<NotationMove> rootMove(new NotationMove("", *new ChessPosition));
        rootMove->m_position->setBoardData(convertFenToBoardData(rootMove->FEN));
        buildNotationTree(database[i].getRootVariation(), rootMove);
        PGNGame game;
        game.headerInfo = database[i].headerInfo;
        game.bodyText = database[i].bodyText;
        game.rootMove = rootMove;
        game.dbIndex = i;
        for (auto &kv : database[i].headerInfo) {
            if (kv.first == "Result") {
                game.result = kv.second;
                break;
            }
        }
        PGNdatabase.push_back(game);
    }

    return PGNdatabase;
}
