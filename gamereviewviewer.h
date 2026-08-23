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
#include <QElapsedTimer>

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
    void autoStartReview();

signals:
    void moveSelected(QSharedPointer<NotationMove> &move);
    void reviewCompleted();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onInfoReceived(const QString& line);
    void onBestMove(const QString& bestMove);

private:
/*----------------------------/
\   game review procedure     \
/----------------------------*/
    void startNextEval();
    void finalizeReview();

/*----------------------------/
\   ui                        \
/----------------------------*/
    void retranslateUi();
    void createSummaryGrid();

    QPushButton *m_reviewBtn;
    QChartView* m_chartView;
    QChart* m_chart;
    QProgressBar *m_progressBar;
    QWidget* m_summaryWidget = nullptr;

    QLineSeries* m_lineSeries;
    QLineSeries* m_zeroSeries;
    QScatterSeries* m_pointSeries;
    QScatterSeries *m_inaccuracySeries;
    QScatterSeries *m_mistakeSeries;
    QScatterSeries *m_blunderSeries;
    QVector<QAreaSeries*> m_areaSeries;

    QGraphicsEllipseItem* m_hoverMarker;
    QScatterSeries* m_hoverPoint;
    QGraphicsLineItem* m_vLine;
    QValueAxis* m_axisX;
    QValueAxis* m_axisY;

    QLabel* m_engineLabel;
    QPushButton* m_selectEngineBtn;

    QLabel* m_whiteAccuracyLabel = nullptr;
    QLabel* m_blackAccuracyLabel = nullptr;
    QVector<QLabel*> m_whiteCategoryLabels;
    QVector<QLabel*> m_blackCategoryLabels;

/*----------------------------/
\   metadata                  \
/----------------------------*/
    enum class Category {
        Brilliant = 0,
        Great,
        Best,
        Inaccuracy,
        Mistake,
        Blunder
    };

    struct CategoryInfo
    {
        Category category;
        QString iconName;
        QString color;
    };

    static const QVector<CategoryInfo> s_categories;
    QString categoryDisplayName(Category category) const;

/*----------------------------/
\   engine                    \
/----------------------------*/
    ChessQSettings m_settings;
    UciEngine* m_engine;
    QMetaObject::Connection m_engineReadyConn;

/*----------------------------/
\   game review data          \
/----------------------------*/
    QVector<QSharedPointer<NotationMove>> m_moves;
    QSharedPointer<NotationMove> m_rootMove;

    QQueue<PendingEval> m_pending;
    int m_currentEvalIndex = -1;
    double m_lastCp = 0.0;
    bool m_isReviewing = false;
    int m_totalEvals = 0;
    int m_movetimeMs = 50;

    QVector<double> m_results;
    std::vector<EvalPt> m_origPts;
    std::vector<EvalPt> m_areaPts;
};

#endif // GAMEREVIEWVIEWER_H
