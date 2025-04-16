#include "tabledelegate.h"

#include <QStyledItemDelegate>
#include <QPainter>
#include <QRect>

TableDelegate::TableDelegate(QObject *parent): QStyledItemDelegate(parent) {}

void TableDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const {
    QStyledItemDelegate::paint( painter, option, index );

    if(true){
        painter->setPen(QPen(QColor(5142583), 3));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight() );
    }
}
