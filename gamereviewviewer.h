#ifndef GAMEREVIEWVIEWER_H
#define GAMEREVIEWVIEWER_H

#include "uciengine.h"
#include "notation.h"

#include <QLabel>
#include <QWidget>
#include <QTableWidget>

#include <QRegularExpression>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QGraphicsEllipseItem>
#include <QtCharts/QAreaSeries>

struct EvalPt { qreal x, y; };

class GameReviewViewer : public QWidget {
    Q_OBJECT

public:
    explicit GameReviewViewer(QWidget *parent = nullptr);
    void reviewGame(const QSharedPointer<NotationMove>& root);

signals:
    void moveSelected(QSharedPointer<NotationMove> &move);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    double evaluateFen(const QString &fen);

    UciEngine* m_engine;
    QTableWidget *m_table;
    QLabel *m_whiteLabel;
    QLabel *m_blackLabel;
    int m_movetimeMs = 50;

    QVector<QSharedPointer<NotationMove>> m_moves;
    QChartView* m_chartView;
    QChart* m_chart;
    QLineSeries* m_lineSeries;
    QLineSeries* m_zeroSeries;
    QScatterSeries* m_pointSeries;
    QGraphicsEllipseItem* m_hoverMarker;
    QScatterSeries* m_hoverPoint;
    QGraphicsLineItem* m_vLine;
    QVector<QAreaSeries*> m_areaSeries;
    QValueAxis* m_axisX;
    QValueAxis* m_axisY;
    std::vector<EvalPt> m_origPts;
    std::vector<EvalPt> m_areaPts;
};

#endif // GAMEREVIEWVIEWER_H
