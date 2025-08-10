#include "databaseviewermodel.h"
#include "streamparser.h"

DatabaseViewerModel::DatabaseViewerModel(QObject *parent): QAbstractItemModel{parent} {
    mHeaders << "#" << "White" << "wElo" << "Black" << "bElo" << "Result" << "Moves" << "Event" << "Date";
}

// Returns the number of rows
int DatabaseViewerModel::rowCount(const QModelIndex &parent) const {
    return static_cast<int>(mData.size());
}

// Returns the number of columns
int DatabaseViewerModel::columnCount(const QModelIndex &parent) const {
    return mHeaders.size();
}


// Returns the values in the model
QVariant DatabaseViewerModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        int row = index.row();
        int col = index.column();
        if (row >= 0 && row < rowCount() && col >= 0 && col < columnCount())
            return mData[row][col];
    }

    else if(role == Qt::TextAlignmentRole){
        return Qt::AlignCenter;
    }

    return QVariant();
}

// Returns the index given a row and column
QModelIndex DatabaseViewerModel::index(int row, int column, const QModelIndex &parent) const {

    if (row < 0 || row >= rowCount() || column < 0 || column >= columnCount())
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex DatabaseViewerModel::parent(const QModelIndex &child) const{
    Q_UNUSED(child);
    return QModelIndex();
}

// Inserts rows given an index and a row count
bool DatabaseViewerModel::insertRows(int row, int count, const QModelIndex &parent) {
    if (row < 0 || row > static_cast<int>(mData.size()))
        return false;

    beginInsertRows(parent, row, row + count - 1);

    for (int i = 0; i < count; ++i) {
        std::vector<QString> newRow(columnCount());
        if (row == static_cast<int>(mData.size())) {
            mData.push_back(newRow);
        } else {
            mData.insert(mData.begin() + row, newRow);
        }
    }

    endInsertRows();
    return true;
}

// Changes the model data
bool DatabaseViewerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    int row = index.row();
    int col = index.column();

    if (row < 0 || row >= rowCount() || col < 0 || col >= columnCount())
        return false;

    mData[row][col] = value.toString();

    emit dataChanged(index, index);

    return true;
}

// Returns the header data
QVariant DatabaseViewerModel::headerData(int section, Qt::Orientation orientation, int role) const{

    if (role == Qt::TextAlignmentRole && orientation == Qt::Horizontal) {
        return Qt::AlignCenter;
    }
    else if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal && section >= 0 && section <= mHeaders.size()) {
        // Column headers
        return mHeaders[section];
    }

    return QVariant();
}

int DatabaseViewerModel::headerIndex(const QString& header){
    return mHeaders.indexOf(header);
}

void DatabaseViewerModel::addHeader(const QString& header){
    if(!mHeaders.contains(header)){
        int newCol = mHeaders.size();
        beginInsertColumns(QModelIndex(), newCol, newCol);
        mHeaders << header;
        for(auto &row: mData){
            row.push_back("");
        }
        endInsertColumns();
        emit headerDataChanged(Qt::Horizontal, mHeaders.size()-1, mHeaders.size()-1);
    }
}

void DatabaseViewerModel::removeHeader(int headerIndex) {
    if(headerIndex < 0 || headerIndex >= mHeaders.size()) return;
    
    beginRemoveColumns(QModelIndex(), headerIndex, headerIndex);
    
    mHeaders.removeAt(headerIndex);
    
    for(auto& row : mData) {
        if(headerIndex < static_cast<int>(row.size())) {
            row.erase(row.begin() + headerIndex);
        }
    }
    
    endRemoveColumns();
    
    emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
}



void DatabaseViewerModel::addGame(const PGNGame& game)
{
    mGameData.append(game);
}

bool DatabaseViewerModel::removeGame(const int row, const QModelIndex &parent)
{
    if (row < 0 || row >= mGameData.size()){
        return false;
    }

    beginRemoveRows(parent, row, row);
    mGameData.remove(row);
    mData.erase(mData.begin() + row);
    endRemoveRows();
    return true;
}

PGNGame& DatabaseViewerModel::getGame(int row) {
    if (row >= 0 && row < mGameData.size()){
        return mGameData[row];
    }
    throw std::out_of_range("Invalid row index in getGame()");
}


