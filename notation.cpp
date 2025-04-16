#include "notation.h"


#include <qDebug>

void linkMoves(const QSharedPointer<NotationMove>& parent, const QSharedPointer<NotationMove>& child)
{
    if (parent->m_nextMoves.size()){
        child->isVarRoot = true;
    }
    parent->m_nextMoves.append(child);
    child->m_previousMove = parent;
}

void deleteMovesAfter(const QSharedPointer<NotationMove>& move)
{
    move->m_nextMoves.clear();
}

QSharedPointer<NotationMove> deleteVariation(const QSharedPointer<NotationMove>& move)
{
    QSharedPointer<NotationMove> temp = move;
    while(temp->m_previousMove != nullptr && !temp->isVarRoot){
        temp = temp->m_previousMove;
    }
    qDebug() << temp->moveText;
    if (temp->m_previousMove != nullptr){
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
