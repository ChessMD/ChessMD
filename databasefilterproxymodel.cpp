#include "databasefilterproxymodel.h"

DatabaseFilterProxyModel::DatabaseFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{




}

void DatabaseFilterProxyModel::setTextFilter(QString header, const QString &pattern){
    if (pattern.isEmpty()){
        textFilters.remove(header);
    }
    else{
        textFilters[header] = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
    }

    invalidateFilter();
}

void DatabaseFilterProxyModel::setRangeFilter(QString header, int lower, int higher){

    rangeFilters[header] = {lower, higher};

    invalidateFilter();

}

bool DatabaseFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const{

    for(auto [key, value]: textFilters.asKeyValueRange()){

        int col = -1;
        for(int i = 0; i < sourceModel()->columnCount(); i++){
            QVariant headerData = sourceModel()->headerData(i, Qt::Horizontal);
            if(headerData.toString() == key){
                col = i; break;
            }
        }

        if(col >= 0){
            QModelIndex index = sourceModel()->index(sourceRow, col, sourceParent);
            QString data = sourceModel()->data(index).toString();
            if(!value.match(data).hasMatch()) return false;
        }
    }

    for(auto [key, value]: rangeFilters.asKeyValueRange()){
        int col = -1;
        for(int i = 0; i < sourceModel()->columnCount(); i++){
            QVariant headerData = sourceModel()->headerData(i, Qt::Horizontal);
            if(headerData.toString() == key){
                col = i; break;
            }
        }

        if(col >= 0){
            QModelIndex index = sourceModel()->index(sourceRow, col, sourceParent);
            int data = sourceModel()->data(index).toString().toInt();
            if(!(value.first <= data && data <= value.second)) return false;
        }


    }

    return true;
}

bool DatabaseFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const{
    QString headerName = sourceModel()->headerData(left.column(), Qt::Horizontal).toString();
    if(headerName == "Elo" || headerName == "Moves"){
        return sourceModel()->data(left).toString().toInt() < sourceModel()->data(right).toString().toInt();
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

