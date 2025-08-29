#include "databasefilterproxymodel.h"

//static
static QDate parseDate(const QString &s){
    QString t = s.trimmed();
    if (t.isEmpty()) return QDate();
    QDate d = QDate::fromString(t, Qt::ISODate);
    if (d.isValid()) return d;

    return QDate();
}

DatabaseFilterProxyModel::DatabaseFilterProxyModel(QObject *parent) : QSortFilterProxyModel{parent}{}

// Filter by text
void DatabaseFilterProxyModel::setTextFilter(QString header, const QString &pattern){
    if (pattern.isEmpty()){
        textFilters.remove(header);
    }
    else{
        textFilters[header] = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
    }

    invalidateFilter();
}

// Filter by range
void DatabaseFilterProxyModel::setRangeFilter(QString header, int lower, int higher){

    rangeFilters[header] = {lower, higher};

    invalidateFilter();

}

void DatabaseFilterProxyModel::setPlayerFilter(const QString& whiteFirst, const QString& whiteLast, const QString& blackFirst, const QString& blackLast, bool ignoreColor) {
    mWhiteFirst = whiteFirst.trimmed();
    mWhiteLast = whiteLast.trimmed();
    mBlackFirst = blackFirst.trimmed();
    mBlackLast = blackLast.trimmed();
    mIgnoreColour = ignoreColor;
    
    mHasPlayerFilter = !mWhiteFirst.isEmpty() || !mWhiteLast.isEmpty() || !mBlackFirst.isEmpty() || !mBlackLast.isEmpty();
    
    if (mIgnoreColour) {
        textFilters.remove("White");
        textFilters.remove("Black");
    }
    else{
        setTextFilter("Black", QString("^(?=.*%1)(?=.*%2).*").arg(mBlackFirst, mBlackLast));
        setTextFilter("White", QString("^(?=.*%1)(?=.*%2).*").arg(mWhiteFirst, mWhiteLast));
    }
    
    
    invalidateFilter();
}

void DatabaseFilterProxyModel::setDateFilter(const QDate &minDate, const QDate &maxDate){
    mHasDateFilter = minDate.isValid() && maxDate.isValid();
    //maybe later display if not valid
    
    mDateMin = minDate;
    mDateMax = maxDate;
    invalidateFilter();
}

// Translate filters to display
bool DatabaseFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const{

    //player filter
    if(mHasPlayerFilter){
        //get indices
        int whiteCol = -1, blackCol = -1;
        for(int i = 0; i < sourceModel()->columnCount(); i++) {
            QVariant headerData = sourceModel()->headerData(i, Qt::Horizontal);
            QString headerName = headerData.toString();
            if(headerName == "White") whiteCol = i;
            else if(headerName == "Black") blackCol = i;
        }

        if (whiteCol >= 0 && blackCol >= 0) {
            QModelIndex whiteIndex = sourceModel()->index(sourceRow, whiteCol, sourceParent);
            QModelIndex blackIndex = sourceModel()->index(sourceRow, blackCol, sourceParent);
            QString whitePlayer = sourceModel()->data(whiteIndex).toString();
            QString blackPlayer = sourceModel()->data(blackIndex).toString();

            if(mIgnoreColour){
                QString player1Pattern, player2Pattern;
                
                if (!mWhiteFirst.isEmpty() || !mWhiteLast.isEmpty()) {
                    QStringList parts;
                    if (!mWhiteFirst.isEmpty()) parts << mWhiteFirst;
                    if (!mWhiteLast.isEmpty()) parts << mWhiteLast;
                    player1Pattern = QString("^(?=.*%1).*").arg(parts.join(")(?=.*"));
                }
                
                if (!mBlackFirst.isEmpty() || !mBlackLast.isEmpty()) {
                    QStringList parts;
                    if (!mBlackFirst.isEmpty()) parts << mBlackFirst;
                    if (!mBlackLast.isEmpty()) parts << mBlackLast;
                    player2Pattern = QString("^(?=.*%1).*").arg(parts.join(")(?=.*"));
                }

                bool hasPlayer1 = false, hasPlayer2 = false;
                
                if (!player1Pattern.isEmpty()) {
                    QRegularExpression regex1(player1Pattern, QRegularExpression::CaseInsensitiveOption);
                    hasPlayer1 = regex1.match(whitePlayer).hasMatch() || regex1.match(blackPlayer).hasMatch();
                }
                
                if (!player2Pattern.isEmpty()) {
                    QRegularExpression regex2(player2Pattern, QRegularExpression::CaseInsensitiveOption);
                    hasPlayer2 = regex2.match(whitePlayer).hasMatch() || regex2.match(blackPlayer).hasMatch();
                }
                
                //if both check both else check each
                if (!player1Pattern.isEmpty() && !player2Pattern.isEmpty()) {
                    if (!hasPlayer1 || !hasPlayer2) return false;
                } else {
                    if (!player1Pattern.isEmpty() && !hasPlayer1) return false;
                    if (!player2Pattern.isEmpty() && !hasPlayer2) return false;
                }

            }
            
        }

    }

    //date filter
    if (mHasDateFilter){
        int col = -1;
        for (int i = 0; i < sourceModel()->columnCount(); ++i) {
            QVariant headerData = sourceModel()->headerData(i, Qt::Horizontal);
            if (headerData.toString().compare("Date", Qt::CaseInsensitive) == 0) {
                col = i;
                break;
            }
        }
        if (col < 0) {
            return false;
        }
        QModelIndex idx = sourceModel()->index(sourceRow, col, sourceParent);
        QString dateStr = sourceModel()->data(idx).toString();
        QDate rowDate = parseDate(dateStr);
        if (!rowDate.isValid()) {
            return false;
        }
        if (rowDate < mDateMin || rowDate > mDateMax) return false;
    }


    for(auto [key, value]: textFilters.asKeyValueRange()){
        // iterate through rows and apply text filter
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
        // iterate through rows and apply range filter
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

// Custom comparator for integer sorting
bool DatabaseFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const{
    QString headerName = sourceModel()->headerData(left.column(), Qt::Horizontal).toString();
    QVector<QString> numericHeaders {"Number", "#", "Elo", "Move", "Moves"};

    if(std::find(numericHeaders.begin(), numericHeaders.end(), headerName) != numericHeaders.end()){
        return sourceModel()->data(left).toString().toInt() < sourceModel()->data(right).toString().toInt();
    }

    return QSortFilterProxyModel::lessThan(left, right);
}



