#include "fastchessposition.h"
#include <QDebug>

FastChessPosition::FastChessPosition()
{
    reset();
}

void FastChessPosition::reset()
{
    // Set up starting position - rank 1 should be at index 0-7, rank 8 at index 56-63
    const char startingPosition[64] = {
        'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R',  // rank 1 (indices 0-7)
        'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P',  // rank 2 (indices 8-15)
         0,   0,   0,   0,   0,   0,   0,   0,   // rank 3 (indices 16-23)
         0,   0,   0,   0,   0,   0,   0,   0,   // rank 4 (indices 24-31)
         0,   0,   0,   0,   0,   0,   0,   0,   // rank 5 (indices 32-39)
         0,   0,   0,   0,   0,   0,   0,   0,   // rank 6 (indices 40-47)
        'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p',  // rank 7 (indices 48-55)
        'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'   // rank 8 (indices 56-63)
    };
    
    for (int i = 0; i < 64; i++) {
        board[i] = startingPosition[i];
    }
    
    whiteToMove = true;
    whiteKingside = whiteQueenside = blackKingside = blackQueenside = true;
    enPassantTarget = -1;  // No en passant target initially
}

int FastChessPosition::squareToIndex(const QString& square)
{
    if (square.length() != 2) return -1;
    int file = square[0].toLatin1() - 'a';
    int rank = square[1].toLatin1() - '1';
    if (file < 0 || file > 7 || rank < 0 || rank > 7) return -1;
    return rank * 8 + file;
}

QString FastChessPosition::indexToSquare(int index)
{
    if (index < 0 || index > 63) return "";
    int file = index % 8;
    int rank = index / 8;
    return QString(QChar('a' + file)) + QString::number(rank + 1);
}

char FastChessPosition::pieceAt(int file, int rank)
{
    if (file < 0 || file > 7 || rank < 0 || rank > 7) return 0;
    return board[rank * 8 + file];
}

void FastChessPosition::setPiece(int file, int rank, char piece)
{
    if (file >= 0 && file <= 7 && rank >= 0 && rank <= 7) {
        board[rank * 8 + file] = piece;
    }
}

QString FastChessPosition::algebraicToUCI(const QString& algebraicMove)
{
    QString move = algebraicMove.trimmed();
    
    // Debug output for first few moves
    static int debugCount = 0;
    bool shouldDebug = debugCount < 50;  // Debug first 50 moves
    
    if (shouldDebug) {
        qDebug() << "Converting algebraic move:" << move;
        debugCount++;
    }
    
    // Remove check/checkmate indicators
    if (move.endsWith('+') || move.endsWith('#')) {
        move.chop(1);
        if (shouldDebug) qDebug() << "  After removing check/mate:" << move;
    }
    
    // Handle castling
    if (move == "O-O") {
        QString result = whiteToMove ? "e1g1" : "e8g8";
        if (shouldDebug) qDebug() << "  Castling kingside:" << result;
        return result;
    }
    if (move == "O-O-O") {
        QString result = whiteToMove ? "e1c1" : "e8c8";
        if (shouldDebug) qDebug() << "  Castling queenside:" << result;
        return result;
    }
    
    // Parse destination square (always last 2 characters)
    if (move.length() < 2) {
        if (shouldDebug) qDebug() << "  ERROR: Move too short";
        return "";
    }
    
    QString destSquare = move.right(2);
    if (destSquare[0] < 'a' || destSquare[0] > 'h' || 
        destSquare[1] < '1' || destSquare[1] > '8') {
        if (shouldDebug) qDebug() << "  ERROR: Invalid destination square:" << destSquare;
        return "";
    }
    
    if (shouldDebug) qDebug() << "  Destination square:" << destSquare;
    
    int toFile = destSquare[0].toLatin1() - 'a';
    int toRank = destSquare[1].toLatin1() - '1';
    
    // Remove capture indicator and destination square
    QString remaining = move;
    remaining.remove('x');
    remaining = remaining.left(remaining.length() - 2);
    
    if (shouldDebug) qDebug() << "  Remaining after parsing dest:" << remaining;
    
    char pieceType = 'P'; // Default to pawn
    int fromFile = -1, fromRank = -1;
    
    // Parse piece type and disambiguation
    if (!remaining.isEmpty()) {
        int pos = 0;
        
        // Check if first character is a piece type
        if (remaining[0].isUpper() && remaining[0] != 'P') {
            pieceType = remaining[0].toLatin1();
            pos = 1;
            if (shouldDebug) qDebug() << "  Piece type:" << pieceType;
        }
        
        // Parse disambiguation from remaining characters
        while (pos < remaining.length()) {
            QChar c = remaining[pos];
            if (c >= 'a' && c <= 'h') {
                fromFile = c.toLatin1() - 'a';
                if (shouldDebug) qDebug() << "  From file hint:" << fromFile;
            } else if (c >= '1' && c <= '8') {
                fromRank = c.toLatin1() - '1';
                if (shouldDebug) qDebug() << "  From rank hint:" << fromRank;
            }
            pos++;
        }
    }
    
    // Adjust piece type for current player
    if (!whiteToMove && pieceType != 'P') {
        pieceType = pieceType + 32; // Convert to lowercase
    } else if (!whiteToMove && pieceType == 'P') {
        pieceType = 'p';
    }
    
    if (shouldDebug) {
        qDebug() << "  Final piece type:" << pieceType << "White to move:" << whiteToMove;
        qDebug() << "  Disambiguation - fromFile:" << fromFile << "fromRank:" << fromRank;
    }
    
    // Find the piece that can make this move
    if (shouldDebug) {
        qDebug() << "  Searching for pieces of type:" << pieceType;
        qDebug() << "  Target square:" << QChar('a' + toFile) << (toRank + 1);
        qDebug() << "  Piece at target:" << (board[toRank * 8 + toFile] != 0 ? "occupied" : "empty");
        qDebug() << "  En passant target square:" << (enPassantTarget != -1 ? indexToSquare(enPassantTarget) : "none");
        
        if (board[toRank * 8 + toFile] != 0) {
            qDebug() << "  Target piece:" << board[toRank * 8 + toFile];
        }
        
        // Show all pieces of the type we're looking for
        for (int r = 0; r < 8; r++) {
            for (int f = 0; f < 8; f++) {
                if (board[r * 8 + f] == pieceType) {
                    qDebug() << "    Found" << pieceType << "at" << QChar('a' + f) << (r + 1);
                }
            }
        }
    }

    int fromIndex = findPieceForMove(pieceType, toFile, toRank, fromFile, fromRank);
    if (fromIndex == -1) {
        if (shouldDebug) {
            qDebug() << "  ERROR: Could not find piece for move";
            qDebug() << "  Board state (rank 8 to 1):";
            for (int r = 7; r >= 0; r--) {
                QString rankStr;
                for (int f = 0; f < 8; f++) {
                    char piece = board[r * 8 + f];
                    rankStr += (piece == 0) ? '.' : piece;
                }
                qDebug() << "    Rank" << (r+1) << ":" << rankStr;
            }
            qDebug() << "  Looking for piece type:" << pieceType << "to move to" << QChar('a' + toFile) << (toRank + 1);
            if (fromFile != -1) qDebug() << "  Must be on file:" << QChar('a' + fromFile);
            if (fromRank != -1) qDebug() << "  Must be on rank:" << (fromRank + 1);
        }
        return "";
    }
    
    QString fromSquare = indexToSquare(fromIndex);
    QString result = fromSquare + destSquare;
    
    if (shouldDebug) qDebug() << "  SUCCESS: Converted to UCI:" << result;
    
    return result;
}

int FastChessPosition::findPieceForMove(char pieceType, int toFile, int toRank, int fromFile, int fromRank)
{
    static int debugCount = 0;
    bool shouldDebug = debugCount < 50;
    if (shouldDebug) debugCount++;
    
    // Simple piece finding - check all squares for the piece type
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            if (board[rank * 8 + file] == pieceType) {
                // Check disambiguation
                if (fromFile != -1 && file != fromFile) continue;
                if (fromRank != -1 && rank != fromRank) continue;
                
                if (shouldDebug) {
                    qDebug() << "    Checking piece at" << QChar('a' + file) << (rank + 1);
                }
                
                // Basic move validation for piece type
                bool canMove = false;
                int deltaFile = toFile - file;
                int deltaRank = toRank - rank;
                
                switch (pieceType) {
                case 'P': // White pawn
                    if (deltaFile == 0) { // Forward move
                        canMove = ((rank == 1 && (toRank == 2 || (toRank == 3 && board[16 + toFile] == 0))) ||
                                  (rank > 1 && rank < 7 && toRank == rank + 1)) &&
                                  board[toRank * 8 + toFile] == 0;
                    } else if (abs(deltaFile) == 1 && deltaRank == 1) { // Diagonal move (capture or en passant)
                        // Regular capture
                        if (board[toRank * 8 + toFile] != 0) {
                            canMove = true;
                        }
                        // En passant capture
                        else if (enPassantTarget == toRank * 8 + toFile) {
                            canMove = true;
                        }
                    }
                    if (shouldDebug && pieceType == 'P') {
                        qDebug() << "      White pawn: deltaFile=" << deltaFile << "deltaRank=" << deltaRank;
                        qDebug() << "      Target occupied:" << (board[toRank * 8 + toFile] != 0);
                        qDebug() << "      En passant target:" << enPassantTarget << "current target:" << (toRank * 8 + toFile);
                        qDebug() << "      Can move:" << canMove;
                    }
                    break;
                    
                case 'p': // Black pawn
                    if (deltaFile == 0) { // Forward move
                        canMove = ((rank == 6 && (toRank == 5 || (toRank == 4 && board[40 + toFile] == 0))) ||
                                  (rank < 6 && rank > 0 && toRank == rank - 1)) &&
                                  board[toRank * 8 + toFile] == 0;
                    } else if (abs(deltaFile) == 1 && deltaRank == -1) { // Diagonal move (capture or en passant)
                        // Regular capture
                        if (board[toRank * 8 + toFile] != 0) {
                            canMove = true;
                        }
                        // En passant capture
                        else if (enPassantTarget == toRank * 8 + toFile) {
                            canMove = true;
                        }
                    }
                    if (shouldDebug && pieceType == 'p') {
                        qDebug() << "      Black pawn: deltaFile=" << deltaFile << "deltaRank=" << deltaRank;
                        qDebug() << "      Target occupied:" << (board[toRank * 8 + toFile] != 0);
                        qDebug() << "      En passant target:" << enPassantTarget << "current target:" << (toRank * 8 + toFile);
                        qDebug() << "      Can move:" << canMove;
                    }
                    break;
                    
                case 'R': case 'r': // Rook
                    canMove = (file == toFile || rank == toRank) && isPathClear(file, rank, toFile, toRank);
                    break;
                    
                case 'N': case 'n': // Knight
                    canMove = (abs(deltaFile) == 2 && abs(deltaRank) == 1) ||
                              (abs(deltaFile) == 1 && abs(deltaRank) == 2);
                    break;
                    
                case 'B': case 'b': // Bishop
                    canMove = abs(deltaFile) == abs(deltaRank) && isPathClear(file, rank, toFile, toRank);
                    break;
                    
                case 'Q': case 'q': // Queen
                    canMove = ((file == toFile || rank == toRank || abs(deltaFile) == abs(deltaRank)) &&
                              isPathClear(file, rank, toFile, toRank));
                    break;
                    
                case 'K': case 'k': // King
                    canMove = abs(deltaFile) <= 1 && abs(deltaRank) <= 1;
                    break;
                }
                
                if (canMove) {
                    if (shouldDebug) {
                        qDebug() << "    Found valid piece at" << QChar('a' + file) << (rank + 1);
                    }
                    return rank * 8 + file;
                }
            }
        }
    }
    
    return -1; // Piece not found
}

// Add this helper method for path checking
bool FastChessPosition::isPathClear(int fromFile, int fromRank, int toFile, int toRank)
{
    int deltaFile = (toFile > fromFile) ? 1 : (toFile < fromFile) ? -1 : 0;
    int deltaRank = (toRank > fromRank) ? 1 : (toRank < fromRank) ? -1 : 0;
    
    int currentFile = fromFile + deltaFile;
    int currentRank = fromRank + deltaRank;
    
    while (currentFile != toFile || currentRank != toRank) {
        if (board[currentRank * 8 + currentFile] != 0) {
            return false; // Path is blocked
        }
        currentFile += deltaFile;
        currentRank += deltaRank;
    }
    
    return true;
}

bool FastChessPosition::makeMove(const QString& uci)
{
    if (uci.length() != 4) return false;
    
    int fromFile = uci[0].toLatin1() - 'a';
    int fromRank = uci[1].toLatin1() - '1';
    int toFile = uci[2].toLatin1() - 'a';
    int toRank = uci[3].toLatin1() - '1';
    
    if (fromFile < 0 || fromFile > 7 || fromRank < 0 || fromRank > 7 ||
        toFile < 0 || toFile > 7 || toRank < 0 || toRank > 7) {
        return false;
    }
    
    char piece = pieceAt(fromFile, fromRank);
    if (piece == 0) return false;
    
    // Clear previous en passant target
    int oldEnPassantTarget = enPassantTarget;
    enPassantTarget = -1;
    
    // Check for en passant capture
    bool isEnPassant = false;
    if ((piece == 'P' || piece == 'p') && 
        abs(toFile - fromFile) == 1 && 
        board[toRank * 8 + toFile] == 0 &&
        oldEnPassantTarget == toRank * 8 + toFile) {
        isEnPassant = true;
    }
    
    // Make the move
    setPiece(fromFile, fromRank, 0);
    setPiece(toFile, toRank, piece);
    
    // Handle en passant capture - remove the captured pawn
    if (isEnPassant) {
        if (piece == 'P') {
            // White captures black pawn
            setPiece(toFile, toRank - 1, 0);  // Remove black pawn on rank behind
        } else {
            // Black captures white pawn  
            setPiece(toFile, toRank + 1, 0);  // Remove white pawn on rank behind
        }
    }
    
    // Set new en passant target for two-square pawn moves
    if (piece == 'P' && fromRank == 1 && toRank == 3) {
        // White pawn moved two squares
        enPassantTarget = 2 * 8 + toFile;  // Target square is on rank 3
    } else if (piece == 'p' && fromRank == 6 && toRank == 4) {
        // Black pawn moved two squares
        enPassantTarget = 5 * 8 + toFile;  // Target square is on rank 6
    }
    
    // Handle castling
    if ((piece == 'K' || piece == 'k') && abs(toFile - fromFile) == 2) {
        // Move the rook too
        if (toFile == 6) { // Kingside
            char rook = pieceAt(7, fromRank);
            setPiece(7, fromRank, 0);
            setPiece(5, fromRank, rook);
        } else if (toFile == 2) { // Queenside
            char rook = pieceAt(0, fromRank);
            setPiece(0, fromRank, 0);
            setPiece(3, fromRank, rook);
        }
    }
    
    // Update castling rights (simplified)
    if (piece == 'K') whiteKingside = whiteQueenside = false;
    if (piece == 'k') blackKingside = blackQueenside = false;
    if (piece == 'R' && fromFile == 0 && fromRank == 0) whiteQueenside = false;
    if (piece == 'R' && fromFile == 7 && fromRank == 0) whiteKingside = false;
    if (piece == 'r' && fromFile == 0 && fromRank == 7) blackQueenside = false;
    if (piece == 'r' && fromFile == 7 && fromRank == 7) blackKingside = false;
    
    whiteToMove = !whiteToMove;
    return true;
}
