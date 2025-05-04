#ifndef DATABASEFILTER_H
#define DATABASEFILTER_H

#include <QDialog>

namespace Ui {
class DatabaseFilter;
}

class DatabaseFilter : public QDialog
{
    Q_OBJECT

private:
    Ui::DatabaseFilter *ui;

    struct Filter {
        QString whiteFirst, whiteLast, blackFirst, blackLast;
        bool winsOnly, ignoreColours;
        int eloMin, eloMax;
    } _filter;


public:
    explicit DatabaseFilter(QWidget *parent = nullptr);
    ~DatabaseFilter();

    Filter getNameFilters();


};

#endif // DATABASEFILTER_H
