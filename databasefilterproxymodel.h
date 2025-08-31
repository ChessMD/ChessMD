#ifndef DATABASEFILTERPROXYMODEL_H
#define DATABASEFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QDate>

// Model for efficient search and sort of a table
class DatabaseFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit DatabaseFilterProxyModel(QObject *parent = nullptr);

    void setTextFilter(QString header, const QString &pattern);
    void setRangeFilter(QString header, int lower, int higher);
    void setPlayerFilter(const QString& whiteFirst, const QString& whiteLast, const QString& blackFirst, const QString& blackLast, bool ignoreColor);
    void setDateFilter(const QDate &minDate, const QDate &maxDate);

    void resetFilters();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QMap<QString, QRegularExpression> textFilters;
    QMap<QString, QPair<int,int>> rangeFilters;

    QString mWhiteFirst, mWhiteLast;
    QString mBlackFirst, mBlackLast;
    QDate mDateMin, mDateMax;

    bool mIgnoreColour = false;
    bool mHasPlayerFilter = false;
    bool mHasDateFilter = false;
    
};

#endif // DATABASEFILTERPROXYMODEL_H
