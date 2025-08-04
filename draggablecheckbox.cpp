#include "draggablecheckbox.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPainter>
#include <QHBoxLayout>
#include <QLabel>

DraggableCheckBox::DraggableCheckBox(const QString& text, QWidget *parent)
    : QWidget{parent}, mDragEnabled(true)  
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(5);
    
    //6 dots
    mDragHandle = new QLabel(this);  
    mDragHandle->setPixmap(createDragIcon());
    mDragHandle->setFixedSize(12, 16);
    layout->addWidget(mDragHandle);
    
    //check
    mCheckBox = new QCheckBox(text, this);
    layout->addWidget(mCheckBox);

    //del button
    layout->addStretch();

    mDeleteButton = new QPushButton(this);
    mDeleteButton->setText("🗑");
    mDeleteButton->setFixedSize(20, 20);
    mDeleteButton->setCursor(Qt::PointingHandCursor);  
    mDeleteButton->setStyleSheet(
        "QPushButton { "
        "    background-color: #ff4444; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 3px; "
        "    font-size: 10px; "
        "} "
        "QPushButton:hover { "
        "    background-color: #ff6666; "
        "} "
        "QPushButton:pressed { "
        "    background-color: #cc3333; "
        "}"
    );
    mDeleteButton->setToolTip(tr("Delete header"));
    layout->addWidget(mDeleteButton);
    connect(mDeleteButton, &QPushButton::clicked, this, &DraggableCheckBox::deleteRequested);

    setLayout(layout);
    
    setCursor(Qt::OpenHandCursor);
}

void DraggableCheckBox::enterEvent(QEnterEvent* event) {
    if(mDragEnabled) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    QWidget::enterEvent(event);
}

void DraggableCheckBox::leaveEvent(QEvent* event) {
    setCursor(Qt::ArrowCursor);
    QWidget::leaveEvent(event);
}

QPixmap DraggableCheckBox::createDragIcon() {
    QPixmap pixmap(12, 16);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    //draw the dot pattern
    QPen pen(QColor(120, 120, 120));
    pen.setWidth(1);
    painter.setPen(pen);
    
    QBrush brush(QColor(120, 120, 120));
    painter.setBrush(brush);
    
    int dotSize = 2;
    int spacing = 4;
    
    for(int row = 0; row < 3; row++) {
        for(int col = 0; col < 2; col++) {
            int x = 2 + col * spacing;
            int y = 3 + row * spacing;
            painter.drawEllipse(x, y, dotSize, dotSize);
        }
    }
    
    return pixmap;
}

QPixmap DraggableCheckBox::createDisabledDragIcon() {
    QPixmap pixmap(12, 16);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // grayed out
    QPen pen(QColor(180, 180, 180));  
    pen.setWidth(1);
    painter.setPen(pen);
    
    QBrush brush(QColor(180, 180, 180));
    painter.setBrush(brush);
    
    int dotSize = 2;
    int spacing = 4;
    
    for(int row = 0; row < 3; row++) {
        for(int col = 0; col < 2; col++) {
            int x = 2 + col * spacing;
            int y = 3 + row * spacing;
            painter.drawEllipse(x, y, dotSize, dotSize);
        }
    }
    
    return pixmap;
}


void DraggableCheckBox::setDragEnabled(bool enabled){
    mDragEnabled = enabled;

    if(!enabled){
        mDragHandle->setPixmap(createDisabledDragIcon());
        mDragHandle->setToolTip(tr("Cannot reorder essential header"));
        setCursor(Qt::ArrowCursor);
            }
    else{
        mDragHandle->setPixmap(createDragIcon());
        mDragHandle->setToolTip(tr("Drag to reorder"));
        setCursor(Qt::OpenHandCursor);
    }


}


QString DraggableCheckBox::text() const {
    return mCheckBox->text();
}

void DraggableCheckBox::setText(const QString& text) {
    mCheckBox->setText(text);
}

bool DraggableCheckBox::isChecked() const {
    return mCheckBox->isChecked();
}

void DraggableCheckBox::setChecked(bool checked) {
    mCheckBox->setChecked(checked);
}

void DraggableCheckBox::setDeleteEnabled(bool enabled){
    mDeleteButton->setEnabled(enabled);
    if(!enabled){
        mDeleteButton->setCursor(Qt::ArrowCursor);  
        mDeleteButton->setStyleSheet(
            "QPushButton { "
            "    background-color: #cccccc; "
            "    color: #666666; "
            "    border: none; "
            "    border-radius: 3px; "
            "    font-size: 10px; "
            "}"
        );
        mDeleteButton->setToolTip(tr("Essential Header"));
    } else {
        mDeleteButton->setCursor(Qt::PointingHandCursor);
        
        mDeleteButton->setStyleSheet(
            "QPushButton { "
            "    background-color: #ff4444; "
            "    color: white; "
            "    border: none; "
            "    border-radius: 3px; "
            "    font-size: 10px; "
            "} "
            "QPushButton:hover { "
            "    background-color: #ff6666; "
            "} "
            "QPushButton:pressed { "
            "    background-color: #cc3333; "
            "}"
        );
        mDeleteButton->setToolTip(tr("Delete header"));
    }
}

void DraggableCheckBox::mousePressEvent(QMouseEvent* event){
    if(event->button() == Qt::LeftButton && mDragEnabled){
        mDragStart = event->pos();
        setCursor(Qt::ClosedHandCursor);  
    }
    QWidget::mousePressEvent(event);
}

void DraggableCheckBox::mouseMoveEvent(QMouseEvent* event){
    if(!mDragEnabled) return;  
    if(!(event->buttons() & Qt::LeftButton)) return;  
    if((event->position().toPoint() - mDragStart).manhattanLength() < QApplication::startDragDistance()) return; 

    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    mimeData->setText(text());
    drag->setMimeData(mimeData);

    // custom pixmap to copy dragged widget appearance
    QPixmap dragPixmap = grab();  
    dragPixmap = dragPixmap.scaled(dragPixmap.size() * 0.8, Qt::KeepAspectRatio, Qt::SmoothTransformation);
   
    QPixmap transparentPixmap(dragPixmap.size());
    transparentPixmap.fill(Qt::transparent);
    QPainter painter(&transparentPixmap);
    painter.setOpacity(0.7);
    painter.drawPixmap(0, 0, dragPixmap);
    painter.end();
    
    drag->setPixmap(transparentPixmap);
    drag->setHotSpot(QPoint(transparentPixmap.width() / 2, transparentPixmap.height() / 2));

    //prevent copy cursor
    Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
    
    if(mDragEnabled) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void DraggableCheckBox::mouseReleaseEvent(QMouseEvent* event) {
    if(mDragEnabled) {
        setCursor(Qt::OpenHandCursor);
    }
    QWidget::mouseReleaseEvent(event);
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

void DraggableCheckBoxContainer::removeCheckBox(DraggableCheckBox* checkBox) {
    int index = mBoxes.indexOf(checkBox);
    if(index >= 0){
        mBoxes.removeAt(index);
        mLayout->removeWidget(checkBox);
        checkBox->deleteLater();
    }
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
