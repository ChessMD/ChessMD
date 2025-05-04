/*
March 19, 2025: File Creation
*/

#ifndef VARIATIONDIALOGUE_H
#define VARIATIONDIALOGUE_H

#include "notation.h"

#include <QDialog>
#include <QListWidget>

class VariationDialogue : public QDialog
{
    Q_OBJECT
public:
    VariationDialogue(QWidget* parent = nullptr);

    void setVariations(const QSharedPointer<NotationMove>& currentMove);
    QSharedPointer<NotationMove> selectedMove() const;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    QListWidget* listWidget;
    QList<QSharedPointer<NotationMove>> m_moves;
    int m_selectedIndex = -1;
};

#endif // VARIATIONDIALOGUE_H
