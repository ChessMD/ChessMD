#ifndef DATABASEVIEWERMODEL_H
#define DATABASEVIEWERMODEL_H

#include <QAbstractItemModel>

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


private:
    std::vector<std::vector<QString>> m_data;
    QString headers[10] = {"Number", "White", "Elo", "Black", "Elo", "Result", "Moves", "Event", "Date"};


};

#endif // DATABASEVIEWERMODEL_H
