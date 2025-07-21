#include "databasefilter.h"
#include "ui_databasefilter.h"

DatabaseFilter::DatabaseFilter(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DatabaseFilter)
{
    ui->setupUi(this);
    ui->WinsOnly->setEnabled(false);

}

DatabaseFilter::~DatabaseFilter()
{
    delete ui;
}


DatabaseFilter::Filter DatabaseFilter::getNameFilters(){
    // handle name filter values
    _filter.blackFirst = ui->BlackFirst->text();
    _filter.whiteFirst = ui->WhiteFirst->text();
    _filter.blackLast = ui->BlackLast->text();
    _filter.whiteLast = ui->WhiteLast->text();
    _filter.ignoreColours = ui->IgnoreColour->isChecked();
    _filter.winsOnly = ui->WinsOnly->isChecked();
    _filter.eloMin = ui->EloMin->value();
    _filter.eloMax = ui->EloMax->value();

    return _filter;

}
