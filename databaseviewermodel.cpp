#include "databaseviewermodel.h"
#include "streamparser.h"


#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <sstream>

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

void DatabaseViewerModel::setConnectionName(const QString &connectionName){
    m_connectionName = connectionName;
}


void DatabaseViewerModel::addGame(const PGNGame& game) {
    m_gameData.append(game);
    m_cachedGames.append(PGNGame());
}

const PGNGame& DatabaseViewerModel::getGame(int row) {
    if (row >= 0 && row < m_gameData.size()){
        return m_gameData[row];
    }
    throw std::out_of_range("Invalid row index in getGame()");
}

PGNGame DatabaseViewerModel::getGameFromSQL(int row){
    if (row >= 0 && row < m_cachedGames.size() && !m_cachedGames[row].rootMove.isNull()){
        return m_cachedGames[row];
    }

    if (m_connectionName.isEmpty()) {
        qWarning() << "No database connection set";
        return getGame(row); 
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare("SELECT PGNBody FROM games WHERE rowid = ?");
    query.addBindValue(row + 1); 


    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to load PGN from SQL:" << query.lastError().text();
        return getGame(row); 
    }

    QString pgnBody = query.value(0).toString();

    // fill in headers
    QString fullPGN;
    if (row < m_gameData.size()) {
        const PGNGame& headerGame = m_gameData[row];
        for (const auto& header : headerGame.headerInfo) {
            fullPGN += QString("[%1 \"%2\"]\n").arg(header.first, header.second);
        }
    }
    fullPGN += "\n" + pgnBody + "\n";

    // Parse the PGN
    std::istringstream pgnStream(fullPGN.toStdString());
    StreamParser parser(pgnStream);
    std::vector<PGNGame> games = parser.parseDatabase();

    if (games.empty()) {
        qWarning() << "Failed to parse PGN from database";
        return getGame(row); 
    }

    PGNGame loadedGame = games[0];
    loadedGame.dbIndex = row;

    if (row < m_cachedGames.size()) {
        m_cachedGames[row] = loadedGame;
    }

    return loadedGame;

}

void DatabaseViewerModel::loadExistingDatabase(){
    if (m_connectionName.isEmpty()){
        qWarning() << "No database connection set";
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT rowid, Event, Site, Date, Round, White, Black, Result, WhiteElo, BlackElo, ECO FROM games ORDER BY rowid")) {
        qWarning() << "Failed to load existing database:" << query.lastError().text();
        return;
    }

    beginResetModel();
    m_data.clear();
    m_gameData.clear();
    m_cachedGames.clear();

    int row = 0;
    while (query.next()) {
        // headers
        PGNGame game;
        game.dbIndex = row;

        QStringList headerNames = {"Event", "Site", "Date", "Round", "White", "Black", "Result", "WhiteElo", "BlackElo", "ECO"};
        for (int i = 0; i < headerNames.size(); i++) {
            QString value = query.value(i + 1).toString(); 
            if (!value.isEmpty()) {
                game.headerInfo.append(qMakePair(headerNames[i], value));
            }
        }

        // Set result
        for (const auto& header : game.headerInfo) {
            if (header.first == "Result") {
                game.result = header.second;
                break;
            }
        }

        m_gameData.append(game);
        m_cachedGames.append(PGNGame()); //cache

        // Build display data
        std::vector<QString> rowData(10);
        rowData[0] = QString::number(row + 1); // id

        for (const auto& header : game.headerInfo) {
            if (header.first == "White") rowData[1] = header.second;
            else if (header.first == "WhiteElo") rowData[2] = header.second;
            else if (header.first == "Black") rowData[3] = header.second;
            else if (header.first == "BlackElo") rowData[4] = header.second;
            else if (header.first == "Result") rowData[5] = header.second;
            else if (header.first == "Event") rowData[7] = header.second;
            else if (header.first == "Date") rowData[8] = header.second;
        }

        rowData[6] = "?"; //temporary todo
        m_data.push_back(rowData);
        row++;
    }

    endResetModel();
    qDebug() << "Loaded" << row << "games from existing database";


}
