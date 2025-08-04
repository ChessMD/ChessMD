#include "draggablecheckbox.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

DraggableCheckBox::DraggableCheckBox(const QString& text, QWidget *parent)
    : QCheckBox{text, parent}
{}

void DraggableCheckBox::mousePressEvent(QMouseEvent* event){
    if(event->button() == Qt::LeftButton){
        mDragStart = event->pos();
    }
    QCheckBox::mousePressEvent(event);
}

void DraggableCheckBox::mouseMoveEvent(QMouseEvent* event){
    if(!(event->buttons() & Qt::LeftButton)) return;  
    if((event->position().toPoint() - mDragStart).manhattanLength() < QApplication::startDragDistance()) return; 

    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    mimeData->setText(text());
    drag->setMimeData(mimeData);

    Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

}

DraggableCheckBoxContainer::DraggableCheckBoxContainer(QWidget* parent) : QWidget(parent) {
    setAcceptDrops(true);
    mLayout = new QVBoxLayout(this);
    mLayout->setContentsMargins(0, 0, 0, 0);
}

void DraggableCheckBoxContainer::addCheckBox(DraggableCheckBox* checkBox) {
    mLayout->addWidget(checkBox);
    mBoxes.append(checkBox);
}

QVector<DraggableCheckBox*> DraggableCheckBoxContainer::getCheckBoxes() const {
    return mBoxes;
}

void DraggableCheckBoxContainer::clear() {
    while (QLayoutItem* item = mLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    mBoxes.clear();
}

void DraggableCheckBoxContainer::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void DraggableCheckBoxContainer::dropEvent(QDropEvent* event) {
    QString droppedText = event->mimeData()->text();
    QPoint dropPos = event->position().toPoint();  
    
    DraggableCheckBox* sourceBox = nullptr;
    for (auto* box : mBoxes) {
        if (box->text() == droppedText) {
            sourceBox = box;
            break;
        }
    }
    if (!sourceBox) return;

    int dropIndex = -1;
    for (int i = 0; i < mBoxes.size(); ++i) {
        QWidget* widget = mBoxes[i];
        QRect rect = widget->geometry();
        if (dropPos.y() < rect.center().y()) {
            dropIndex = i;
            break;
        }
    }
    if (dropIndex == -1) dropIndex = mBoxes.size();

    int oldIndex = mBoxes.indexOf(sourceBox);

    //alreadythere
    if (oldIndex == dropIndex || oldIndex == dropIndex - 1) return; 

    mBoxes.removeAt(oldIndex);
    mLayout->removeWidget(sourceBox);

    if (oldIndex < dropIndex) dropIndex--;

    mBoxes.insert(dropIndex, sourceBox);
    mLayout->insertWidget(dropIndex, sourceBox);

    event->acceptProposedAction();
}
