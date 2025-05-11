#include "databaseviewermodel.h"

DatabaseViewerModel::DatabaseViewerModel(QObject *parent): QAbstractItemModel{parent} {

}

// Returns the number of rows
int DatabaseViewerModel::rowCount(const QModelIndex &parent) const {
    return static_cast<int>(m_data.size());
}

// Returns the number of columns
int DatabaseViewerModel::columnCount(const QModelIndex &parent) const {
    return m_data.empty() ? 10 : static_cast<int>(m_data[0].size());
}


// Returns the values in the model
QVariant DatabaseViewerModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        int row = index.row();
        int col = index.column();
        if (row >= 0 && row < rowCount() && col >= 0 && col < columnCount())
            return m_data[row][col];
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
    if (row < 0 || row > static_cast<int>(m_data.size()))
        return false;

    beginInsertRows(parent, row, row + count - 1);

    for (int i = 0; i < count; ++i) {
        std::vector<QString> newRow(columnCount());
        if (row == static_cast<int>(m_data.size())) {
            m_data.push_back(newRow);
        } else {
            m_data.insert(m_data.begin() + row, newRow);
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

    m_data[row][col] = value.toString();

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

    if (orientation == Qt::Horizontal) {
        // Column headers
        return headers[section];
    }

    return QVariant();
}


void DatabaseViewerModel::addGame(const PGNGameData& game) {
    m_gameData.append(game);
}

const PGNGameData& DatabaseViewerModel::getGame(int row) {
    if (row >= 0 && row < m_gameData.size()){
        return m_gameData[row];
    }
    throw std::out_of_range("Invalid row index in getGame()");
}

