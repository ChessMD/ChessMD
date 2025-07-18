#ifndef FASTCHESSPOSITION_H
#define FASTCHESSPOSITION_H

#include <QString>
#include <QVector>

class FastChessPosition
{
public:
    FastChessPosition();
    
    // Fast algebraic to UCI conversion without full validation
    QString algebraicToUCI(const QString& algebraicMove);
    
    // Make move and update position (minimal validation)
    bool makeMove(const QString& uci);
    
    // Reset to starting position
    void reset();

private:
    // Simple board representation: piece type at each square
    // 0=empty, 1=P, 2=N, 3=B, 4=R, 5=Q, 6=K (lowercase for black)
    char board[64];
    bool whiteToMove;
    
    // Castling rights
    bool whiteKingside, whiteQueenside, blackKingside, blackQueenside;
    
    // En passant target square (-1 if none)
    int enPassantTarget;
    
    // Helper functions
    int squareToIndex(const QString& square);
    QString indexToSquare(int index);
    char pieceAt(int file, int rank);
    void setPiece(int file, int rank, char piece);
    bool isPathClear(int fromFile, int fromRank, int toFile, int toRank);
    
    // Fast move finding
    int findPieceForMove(char pieceType, int toFile, int toRank, int fromFile = -1, int fromRank = -1);
};

#endif // FASTCHESSPOSITION_H
