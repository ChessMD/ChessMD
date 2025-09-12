#ifndef DATABASEFILTER_H
#define DATABASEFILTER_H

#include <QDialog>
#include <QDate>
#include <QQuickWidget>
#include "chessposition.h"


namespace Ui {
class DatabaseFilter;
}

// Widget for applying database filters
class DatabaseFilter : public QDialog
{
    Q_OBJECT

private:
    Ui::DatabaseFilter *ui;
    QQuickWidget *mChessboardWidget;
    ChessPosition *mChessPosition;  

    struct Filter {
        QString whiteFirst, whiteLast, blackFirst, blackLast, tournament, annotator, ecoMin, ecoMax;
        bool winsOnly, ignoreColours, dateCheck, ecoCheck, movesCheck;
        int eloMin, eloMax, movesMin, movesMax;
        QDate dateMin, dateMax;
        quint64 zobrist;
    } _filter;


public:
    explicit DatabaseFilter(QWidget *parent = nullptr);
    ~DatabaseFilter();
    Filter getNameFilters();

private slots:
    void onPositionChanged(const QString& fen, const QVariant& zobrist);

private:
    void setupPositionTab();
};

#endif // DATABASEFILTER_H
