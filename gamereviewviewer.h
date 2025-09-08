#ifndef GAMEREVIEWVIEWER_H
#define GAMEREVIEWVIEWER_H

#include "uciengine.h"
#include "notation.h"
#include "chessqsettings.h"

#include <QLabel>
#include <QWidget>
#include <QTableWidget>
#include <QQueue>
#include <QProgressBar>
#include <QPushButton>
#include <QFileDialog>

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
    void createSummaryGrid();

    double evaluateFen(const QString &fen);
    void startNextEval();
    void finalizeReview();

    ChessQSettings m_settings;

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
    QLabel* m_engineLabel;
    QPushButton* m_selectEngineBtn;

    QWidget* m_summaryWidget = nullptr;

    QLabel* m_whiteAccuracyLabel = nullptr;
    QLabel* m_blackAccuracyLabel = nullptr;
    QLabel* m_whiteBrilliantLabel = nullptr;
    QLabel* m_blackBrilliantLabel = nullptr;
    QLabel* m_whiteGreatLabel = nullptr;
    QLabel* m_blackGreatLabel = nullptr;
    QLabel* m_whiteBestLabel = nullptr;
    QLabel* m_blackBestLabel = nullptr;
    QLabel* m_whiteInaccuracyLabel = nullptr;
    QLabel* m_blackInaccuracyLabel = nullptr;
    QLabel* m_whiteMistakeLabel = nullptr;
    QLabel* m_blackMistakeLabel = nullptr;
    QLabel* m_whiteBlunderLabel = nullptr;
    QLabel* m_blackBlunderLabel = nullptr;

    QLabel* m_whiteInaccuracyCount;
    QLabel* m_whiteMistakeCount;
    QLabel* m_whiteBlunderCount;
    QLabel* m_whiteGreatCount;
    QLabel* m_whiteBrilliantCount;
    QLabel* m_whiteBestCount;
    QLabel* m_whiteMovesCount;
    QLabel* m_blackInaccuracyCount;
    QLabel* m_blackMistakeCount;
    QLabel* m_blackBlunderCount;
    QLabel* m_blackGreatCount;
    QLabel* m_blackBrilliantCount;
    QLabel* m_blackBestCount;
    QLabel* m_blackMovesCount;

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
