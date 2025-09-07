#ifndef DATABASEVIEWERMODEL_H
#define DATABASEVIEWERMODEL_H

#include "pgngame.h"

#include <QAbstractItemModel>
#include <QStringList>

// DatabaseViewerModel class is a model to store game headers
class DatabaseViewerModel : public QAbstractItemModel
{
public:
    explicit DatabaseViewerModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    int headerIndex(const QString& header);
    void addHeader(const QString& header);
    void removeHeader(int headerIndex);
    void addGame(const PGNGame& game);
    bool removeGame(const int row, const QModelIndex &parent);
    PGNGame& getGame(int row);

private:
    std::vector<std::vector<QString>> mData;
    QVector<PGNGame> mGameData;
    QStringList mHeaders;
};

#endif // DATABASEVIEWERMODEL_H
