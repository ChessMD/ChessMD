/*
March 18, 2025: File Creation
*/


#include "notation.h"
#include "chessposition.h"

#include <QDebug>

NotationMove::NotationMove(const QString &text, ChessPosition &position)
{
    moveText = text;
    m_position = &position;
}

QSharedPointer<NotationMove> NotationMove::cloneNotationTree(QSharedPointer<NotationMove>& move)
{
    if (!move) return nullptr;

    ChessPosition* posCopy = new ChessPosition;
    posCopy->copyFrom(*move->m_position);
    auto copy = QSharedPointer<NotationMove>::create(move->moveText, *posCopy);
    copy->FEN           = move->FEN;
    copy->commentBefore = move->commentBefore;
    copy->annotation1   = move->annotation1;
    copy->annotation2   = move->annotation2;
    copy->commentAfter  = move->commentAfter;
    copy->isVarRoot     = move->isVarRoot;
    for (auto& childMove : move->m_nextMoves) {
        auto childCopy = cloneNotationTree(childMove);
        childCopy->m_previousMove = copy;
        copy->m_nextMoves.append(childCopy);
    }

    return copy;
}

// Links a new move to a previous move in the game tree
void linkMoves(const QSharedPointer<NotationMove>& parent, const QSharedPointer<NotationMove>& child)
{
    if (parent->m_nextMoves.size()){
        child->isVarRoot = true;
    }
    parent->m_nextMoves.append(child);
    child->m_previousMove = parent;
}

// Deletes all moves after the current selected move
void deleteMovesAfter(const QSharedPointer<NotationMove>& move)
{
    move->m_nextMoves.clear();
}

// Deletes the entire variation of the current selected move
QSharedPointer<NotationMove> deleteVariation(const QSharedPointer<NotationMove>& move)
{
    QSharedPointer<NotationMove> temp = move;
    while(temp->m_previousMove != nullptr && !temp->isVarRoot){
        temp = temp->m_previousMove;
    }
    if (temp->m_previousMove != nullptr){
        // Find the required variation and remove it
        for (int i = 0; i < temp->m_previousMove->m_nextMoves.size(); i++){
            if (temp->m_previousMove->m_nextMoves[i]->moveText == temp->moveText){
                temp->m_previousMove->m_nextMoves.erase(temp->m_previousMove->m_nextMoves.begin()+i);
                break;
            }
        }
        return temp->m_previousMove;
    } else {
        return move;
    }
}

void promoteVariation(const QSharedPointer<NotationMove>& move)
{
    QSharedPointer<NotationMove> temp = move;
    while(temp->m_previousMove != nullptr && !temp->isVarRoot){
        temp = temp->m_previousMove;
    }
    temp->isVarRoot = false;
    if (temp->m_previousMove != nullptr){
        // Find the required variation and remove it
        for (int i = 0; i < temp->m_previousMove->m_nextMoves.size(); i++){
            if (temp->m_previousMove->m_nextMoves[i]->moveText == temp->moveText){
                temp->m_previousMove->m_nextMoves[0]->isVarRoot = true;
                swap(temp->m_previousMove->m_nextMoves[0], temp->m_previousMove->m_nextMoves[i]);
                break;
            }
        }
    }
}
