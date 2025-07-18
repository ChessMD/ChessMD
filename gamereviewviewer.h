#ifndef GAMEREVIEWVIEWER_H
#define GAMEREVIEWVIEWER_H

#include "uciengine.h"
#include "notation.h"

#include <QLabel>
#include <QWidget>
#include <QTableWidget>
#include <QQueue>
#include <QProgressBar>
#include <QPushButton>

#include <QRegularExpression>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QGraphicsEllipseItem>
#include <QtCharts/QAreaSeries>

struct EvalPt { qreal x, y; };
struct PendingEval { QString fen; int index; };

class GameReviewViewer : public QWidget {
    Q_OBJECT

public:
    explicit GameReviewViewer(QSharedPointer<NotationMove> rootMove, QWidget *parent = nullptr);

    void reviewGame(const QSharedPointer<NotationMove>& root);

signals:
    void moveSelected(QSharedPointer<NotationMove> &move);
    void reviewCompleted();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onInfoReceived(const QString& line);
    void onBestMove(const QString& bestMove);

private:
    double evaluateFen(const QString &fen);
    void startNextEval();
    void finalizeReview();

    QLabel *m_whiteLabel;
    QLabel *m_blackLabel;
    QTableWidget *m_table;
    QPushButton *m_reviewBtn;
    QChartView* m_chartView;
    QChart* m_chart;
    QLineSeries* m_lineSeries;
    QLineSeries* m_zeroSeries;
    QScatterSeries* m_pointSeries;
    QScatterSeries *m_inaccuracySeries;
    QScatterSeries *m_mistakeSeries;
    QScatterSeries *m_blunderSeries;
    QGraphicsEllipseItem* m_hoverMarker;
    QScatterSeries* m_hoverPoint;
    QGraphicsLineItem* m_vLine;
    QVector<QAreaSeries*> m_areaSeries;
    QValueAxis* m_axisX;
    QValueAxis* m_axisY;
    QProgressBar *m_progressBar;

    UciEngine* m_engine;
    QQueue<PendingEval> m_pending;
    QVector<double> m_results;
    int m_currentEvalIndex = -1;
    double m_lastCp = 0.0;
    bool m_isReviewing = false;
    int m_totalEvals = 0;

    QVector<QSharedPointer<NotationMove>> m_moves;
    QSharedPointer<NotationMove> m_rootMove;
    std::vector<EvalPt> m_origPts;
    std::vector<EvalPt> m_areaPts;
    int m_movetimeMs = 50;

};

#endif // GAMEREVIEWVIEWER_H
