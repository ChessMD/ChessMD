#ifndef DATABASEFILTER_H
#define DATABASEFILTER_H

#include <QDialog>
#include <QDate>

namespace Ui {
class DatabaseFilter;
}

// Widget for applying database filters
class DatabaseFilter : public QDialog
{
    Q_OBJECT

private:
    Ui::DatabaseFilter *ui;

    struct Filter {
        QString whiteFirst, whiteLast, blackFirst, blackLast, tournament, annotator, ecoMin, ecoMax;
        bool winsOnly, ignoreColours, dateCheck, ecoCheck, movesCheck;
        int eloMin, eloMax, movesMin, movesMax;
        QDate dateMin, dateMax;
    } _filter;


public:
    explicit DatabaseFilter(QWidget *parent = nullptr);
    ~DatabaseFilter();
    Filter getNameFilters();


};

#endif // DATABASEFILTER_H
