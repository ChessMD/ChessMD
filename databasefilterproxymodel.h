#ifndef DATABASEFILTERPROXYMODEL_H
#define DATABASEFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

// Model for efficient search and sort of a table
class DatabaseFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit DatabaseFilterProxyModel(QObject *parent = nullptr);


    void setTextFilter(QString header, const QString &pattern);
    void setRangeFilter(QString header, int lower, int higher);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QMap<QString, QRegularExpression> textFilters;
    QMap<QString, QPair<int,int>> rangeFilters;

    
};

#endif // DATABASEFILTERPROXYMODEL_H
