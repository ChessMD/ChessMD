#include "tabledelegate.h"

#include <QStyledItemDelegate>
#include <QPainter>
#include <QRect>

TableDelegate::TableDelegate(QObject *parent): QStyledItemDelegate(parent) {}

// Adds custom decoration while rendering a table
void TableDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const {
    QStyledItemDelegate::paint( painter, option, index );

    // creates a line under a table row
    if(true){
        painter->setPen(QPen(QColor(5142583), 3)); //hcc
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight() );
    }
}
