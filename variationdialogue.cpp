#include "variationdialogue.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QPushButton>

VariationDialogue::VariationDialogue(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Select Variation");
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout* layout = new QVBoxLayout(this);
    listWidget = new QListWidget(this);
    layout->addWidget(listWidget);

    connect(listWidget, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem* item) {
        m_selectedIndex = listWidget->row(item);
        accept();
    });
}

void VariationDialogue::setVariations(const QSharedPointer<NotationMove>& currentMove) {
    listWidget->clear();

    for (const auto& move : currentMove->m_nextMoves) {
        listWidget->addItem(move->moveText);
        m_moves.append(move);
    }

    if (!m_moves.isEmpty()) {
        listWidget->setCurrentRow(0);
    }
}

QSharedPointer<NotationMove> VariationDialogue::selectedMove() const {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_moves.size()) {
        return m_moves[m_selectedIndex];
    }
    return nullptr;
}

void VariationDialogue::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Up:
        listWidget->setCurrentRow((listWidget->currentRow() - 1 + listWidget->count()) % listWidget->count());
        break;
    case Qt::Key_Down:
        listWidget->setCurrentRow((listWidget->currentRow() + 1) % listWidget->count());
        break;
    case Qt::Key_Right:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        m_selectedIndex = listWidget->currentRow();
        accept();
        break;
    default:
        QDialog::keyPressEvent(event);
    }
}
