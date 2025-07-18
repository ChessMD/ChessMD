/*
March 3, 2025: File Creation
April 20, 2025: Overhauled C++ headers with Qt framework
*/

#include <string>
#include <regex>
#include <QDebug>
#include <QRegularExpression>

#include "streamparser.h"
#include "pgngamedata.h"
#include "chessposition.h"

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

std::vector<PGNGame> StreamParser::parseDatabase(){
    std::vector<PGNGame> database;
    int gameNumber = 0;

    // Text files contain BOM indicators which should be skipped
    char c;
    while ((c = streamBuffer.peek()) != EOF && c != '[') {
        // qDebug() << c;
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

            if (tag == "Result"){
                game.result = value;
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

// Fast move extraction for simple PGN without variations
void StreamParser::parseSimplifiedBodyText(const QString& bodyText, QVector<QString>& moves) {
    moves.clear();
    
    // Simple regex to extract moves - matches algebraic notation
    QRegularExpression moveRegex(R"([NBRQK]?[a-h]?[1-8]?x?[a-h][1-8](?:=[NBRQK])?[+#]?|O-O(?:-O)?[+#]?)");
    
    QRegularExpressionMatchIterator it = moveRegex.globalMatch(bodyText);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString move = match.captured(0);
        
        // Skip move numbers and results
        if (!move.contains(QRegularExpression(R"(\d+\.|\*|1-0|0-1|1/2-1/2)"))) {
            moves.append(move);
        }
    }
}

// Parse a single game from the stream
bool StreamParser::parseNextGame(PGNGame& game) {
    game.headerInfo.clear();
    game.bodyText.clear();
    game.result.clear();
    
    std::string line;
    char c;
    
    // Skip to next game or EOF
    while ((c = streamBuffer.peek()) != EOF && c != '[') {
        streamBuffer.get();
    }
    
    if (streamBuffer.eof()) {
        return false;
    }
    
    // Parse headers
    while ((c = streamBuffer.peek()) != EOF && c == '[') {
        std::getline(streamBuffer, line);
        
        // Fast header parsing
        size_t spacePos = line.find(' ');
        size_t firstQuote = line.find('"', spacePos);
        size_t lastQuote = line.rfind('"');
        
        if (spacePos != std::string::npos && firstQuote != std::string::npos && lastQuote != std::string::npos) {
            QString tag = QString::fromStdString(line.substr(1, spacePos - 1));
            QString value = QString::fromStdString(line.substr(firstQuote + 1, lastQuote - firstQuote - 1));
            
            if (tag == "Result") {
                game.result = value;
            }
            
            game.headerInfo.push_back({tag, value});
        }
    }
    
    // Parse body text until next game or EOF
    std::string bodyText;
    while ((c = streamBuffer.peek()) != EOF && c != '[') {
        std::getline(streamBuffer, line);
        bodyText += line + " ";
    }
    
    game.bodyText = QString::fromStdString(bodyText);
    return true;
}

