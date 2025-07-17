#include "gamereviewviewer.h"
#include "chessposition.h"
#include "chessqsettings.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include <QVBoxLayout>
#include <QLabel>
#include <QEventLoop>
#include <QTimer>
#include <QHeaderView>
#include <QToolTip>
#include <QMouseEvent>
#include <QCursor>

GameReviewViewer::GameReviewViewer(QWidget *parent)
    : QWidget(parent)
{
    ChessQSettings s; s.loadSettings();
    m_engine = new UciEngine(this);
    m_engine->startEngine(s.getEngineFile());

    auto *lay = new QVBoxLayout(this);
    m_whiteLabel = new QLabel(tr("White accuracy: –"), this);
    m_blackLabel = new QLabel(tr("Black accuracy: –"), this);
    lay->addWidget(m_whiteLabel);
    lay->addWidget(m_blackLabel);

    m_chart = new QChart;
    m_lineSeries = new QLineSeries;
    m_pointSeries = new QScatterSeries;
    m_pointSeries->setMarkerSize(6);
    m_pointSeries->setBrush(Qt::transparent);
    m_pointSeries->setPen(Qt::NoPen);

    m_axisX = new QValueAxis;
    m_axisY = new QValueAxis;
    m_axisX->setLabelFormat("%d");
    m_axisX->setLabelsVisible(false);
    m_axisX->setLineVisible(false);
    m_axisY->setLabelsVisible(false);
    m_axisY->setLineVisible(false);
    m_axisY->setRange(-6.10, 6.10);

    m_chart->addSeries(m_lineSeries);
    m_chart->addSeries(m_pointSeries);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_lineSeries->attachAxis(m_axisX);
    m_lineSeries->attachAxis(m_axisY);
    m_pointSeries->attachAxis(m_axisX);
    m_pointSeries->attachAxis(m_axisY);
    m_zeroSeries = new QLineSeries;
    m_zeroSeries->setPen(QPen(Qt::black, 1, Qt::SolidLine));
    m_chart->addSeries(m_zeroSeries);
    m_zeroSeries->attachAxis(m_axisX);
    m_zeroSeries->attachAxis(m_axisY);
    m_chart->legend()->hide();
    m_chart->setBackgroundRoundness(0);
    m_chart->setBackgroundBrush(QBrush(QColor(230, 230, 230)));

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMouseTracking(true);
    m_chartView->viewport()->setMouseTracking(true);
    m_chartView->viewport()->installEventFilter(this);
    lay->addWidget(m_chartView);

    const int R = 8;
    m_hoverMarker = new QGraphicsEllipseItem(0, 0, R, R);
    QColor lineColor = m_lineSeries->pen().color();
    m_hoverMarker->setBrush(QBrush(lineColor));
    m_hoverMarker->setPen(Qt::NoPen);
    m_hoverMarker->setVisible(false);
    m_hoverMarker->setAcceptHoverEvents(true);
    m_chart->scene()->addItem(m_hoverMarker);

    m_vLine = new QGraphicsLineItem;
    QPen linePen(Qt::DashLine);
    linePen.setWidth(1);
    m_vLine->setPen(linePen);
    m_vLine->setVisible(false);
    m_chart->scene()->addItem(m_vLine);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("Move #"), tr("SAN"), tr("Win% before"), tr("Win% after"), tr("Accuracy")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    lay->addWidget(m_table);

    connect(m_pointSeries, &QScatterSeries::hovered, this, [this](const QPointF &pt, bool entered){
        if (entered) {
            QPointF chartPt = m_chart->mapToPosition(pt, m_pointSeries);
            QPointF scenePt = m_chartView->mapToScene(chartPt.toPoint());
            QString text = QString("Eval: %1").arg(pt.y(), 0, 'f', 2);
            m_hoverMarker->setToolTip(text);
            m_hoverMarker->setPos(scenePt.x() - R/2, scenePt.y() - R/2);
            m_hoverMarker->setVisible(true);
        } else {
            m_hoverMarker->setVisible(false);
        }
    });

    connect(m_pointSeries, &QScatterSeries::clicked, this, [this](const QPointF &pt){
        // emit moveSelected(int(pt.x()));
    });

    // m_table->setVisible(false);
}

bool GameReviewViewer::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_chartView->viewport() && event->type() == QEvent::MouseMove) {
        if (m_origPts.empty() || m_axisX->min() >= m_axisX->max() || m_axisY->min() >= m_axisY->max()) {
            return false;
        }
        auto *me = static_cast<QMouseEvent*>(event);
        QPointF dataPt = m_chart->mapToValue(me->pos(), m_lineSeries);
        if (!std::isfinite(dataPt.x())) {
            return false;
        }
        int idx = int(std::round(dataPt.x()));
        idx = std::clamp(idx, 0, int(m_origPts.size()) - 1);
        auto p = m_origPts[idx];

        // compute scene‐pos of the data‐point
        QPointF chartPt = m_chart->mapToPosition({p.x,p.y}, m_pointSeries);
        QPointF scenePt = m_chartView->mapToScene(chartPt.toPoint());

        // move hover‐marker
        int R = int(m_hoverMarker->rect().width());
        m_hoverMarker->setPos(scenePt.x() - R/2, scenePt.y() - R/2);
        m_hoverMarker->setVisible(true);

        // tooltip at point
        QPoint vpPt = m_chartView->mapFromScene(scenePt);
        QPoint glPt = m_chartView->viewport()->mapToGlobal(vpPt);
        QString tip = QString("Move %1\nEval %2").arg(idx).arg(p.y,0,'f',2);
        QToolTip::showText(glPt, tip, m_chartView);

        // vertical line
        qreal yTop = m_axisY->max(), yBot = m_axisY->min();
        QPointF topScene = m_chartView->mapToScene(m_chart->mapToPosition({ p.x, yTop }, m_lineSeries).toPoint());
        QPointF botScene = m_chartView->mapToScene(m_chart->mapToPosition({ p.x, yBot }, m_lineSeries).toPoint());
        m_vLine->setLine(topScene.x(), topScene.y(), botScene.x(), botScene.y());
        m_vLine->setVisible(true);

        return true;
    }
    // hide on leave
    if (watched == m_chartView->viewport() && event->type() == QEvent::Leave) {
        m_hoverMarker->setVisible(false);
        m_vLine->setVisible(false);
        QToolTip::hideText();
        return false;
    }
    return QWidget::eventFilter(watched, event);
}

static double moveAccuracy(double wb, double wa)
{
    double diff = (wb - wa) * 100.0;
    double acc = 103.1668 * std::exp(-0.04354 * diff) - 3.1669;
    return std::clamp(acc, 0.0, 100.0);
}

static double winProb(double cp) {
    double k = 0.004;
    double e = std::exp(-k * cp);
    return 1.0 / (1.0 + e);
}

double stddev(const std::vector<double>& v)
{
    if (v.size() < 2) return 0.0;
    double mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    double sumsq = 0;
    for (double x : v) sumsq += (x - mean) * (x - mean);
    return std::sqrt(sumsq / v.size());
}

std::pair<double,double> gameAccuracy(std::vector<double> winPcts, bool whiteStarts)
{
    int m = int(winPcts.size());
    int total = m - 1;
    if (total <= 0) return {100.0, 100.0};

    // alternate win percentages with white and black perspectives
    for (int i = 0; i <= m; i++){
        if (i % 2 == 1){
            winPcts[i] = 1.0 - winPcts[i];
        }
    }

    // window size = clamp(total/10, 2, 8)
    int w = std::clamp(total / 10, 2, 8);
    w = std::min(w, m);

    // build windows: (w-2) copies of the first, then sliding windows
    int headFill = std::max(w - 2, 0);
    std::vector<std::vector<double>> windows;
    windows.reserve(headFill + (m - w + 1));
    std::vector<double> first(winPcts.begin(), winPcts.begin() + w);
    for (int i = 0; i < headFill; ++i) windows.push_back(first);
    for (int i = 0; i + w <= m; ++i) windows.emplace_back(winPcts.begin() + i, winPcts.begin() + i + w);

    // per‑move volatilities (clamped [0.5,12])
    std::vector<double> vol(total);
    for (int i = 0; i < total; ++i)
        vol[i] = std::clamp(stddev(windows[i]), 0.5, 12.0);

    // compute accuracies & bucket them by color
    std::vector<double> accW, accB, wW, wB;
    accW.reserve((total + 1) / 2);
    accB.reserve(total / 2);

    for (int i = 0; i < total; ++i) {
        bool isWhiteMove = ((i % 2) == 0) == whiteStarts;

        // fold to mover’s perspective by swapping for Black
        double before = isWhiteMove ? winPcts[i]     : winPcts[i + 1];
        double after  = isWhiteMove ? winPcts[i + 1] : winPcts[i];

        double a = moveAccuracy(before, after);
        if (isWhiteMove) {
            accW.push_back(a);
            wW.push_back(vol[i]);
        } else {
            accB.push_back(a);
            wB.push_back(vol[i]);
        }
    }

    // combine = (vol‑weighted mean + harmonic mean) / 2
    auto combine = [&](const std::vector<double>& A, const std::vector<double>& W) {
        int n = int(A.size());
        if (n == 0) return 100.0;
        double num = 0.0, den = 0.0;
        for (int i = 0; i < n; ++i) {
            num += A[i] * W[i];
            den += W[i];
        }
        double wm = den > 0 ? num / den : 100.0;
        double invSum = 0.0;
        for (double a : A) invSum += 1.0 / std::max(a, 1e-6);
        double hm = n > 0 ? n / invSum : 100.0;
        return 0.5 * (wm + hm);
    };

    return { combine(accW, wW), combine(accB, wB) };
}

void GameReviewViewer::reviewGame(const QSharedPointer<NotationMove>& root)
{

    // build the main‐line FEN list
    QVector<QString> fens;
    QVector<QString> sans;

    if (!root || !root->m_position) return;
    fens.append(root->m_position->positionToFEN());
    sans.append(tr("start"));

    auto cur = root;
    while (cur && !cur->m_nextMoves.isEmpty()) {
        auto nxt = cur->m_nextMoves.front();
        if (!nxt->m_position) break;
        fens.append(nxt->m_position->positionToFEN());
        sans.append(nxt->moveText);
        cur = nxt;
    }

    int moves = fens.size() - 1;
    m_table->setRowCount(moves);

    double sumWhite = 0.0, sumBlack = 0.0;
    int cntWhite = 0, cntBlack = 0;

    std::vector<double> winPercentages, evals;

    for (int i = 0; i < moves; i++) {
        double cpBefore = evaluateFen(fens[i]);
        double cpAfter = -evaluateFen(fens[i+1]);
        double wb = winProb(cpBefore);
        double wa = winProb(cpAfter);
        double acc = moveAccuracy(wb, wa);

        if (i % 2 == 0) {
            sumWhite += acc;
            ++cntWhite;
        } else {
            sumBlack += acc;
            ++cntBlack;
        }

        winPercentages.push_back(wb);
        evals.push_back(cpBefore/100.0);
        if (i + 1 == moves){
            winPercentages.push_back(1.0-wa);
            evals.push_back(-cpAfter/100.0);
        }

        int row = i;
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(i+1)));
        m_table->setItem(row, 1, new QTableWidgetItem(sans[i+1]));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::number(wb, 'f', 3)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(wa, 'f', 3)));
        m_table->setItem(row, 4, new QTableWidgetItem(QString::number(acc, 'f', 1)));
    }

    // adjust evaluation to be white's perspective
    for (int i = 0; i <= moves; i++){
        if (i % 2 == 1){
            evals[i] = -evals[i];
        }
    }

    auto [avgW, avgB] = gameAccuracy(winPercentages, true);
    m_whiteLabel->setText(tr("White accuracy: %1 %").arg(QString::number(avgW, 'f', 1)));
    m_blackLabel->setText(tr("Black accuracy: %1 %").arg(QString::number(avgB, 'f', 1)));

    m_origPts.clear();
    m_areaPts.clear();
    int N = int(evals.size());

    for (int i = 0; i < N; ++i) {
        qreal y0 = std::clamp<qreal>(evals[i], -6.00, 6.00);
        // original
        m_origPts.push_back({ (qreal)i, y0 });
        // area base
        m_areaPts.push_back({ (qreal)i, y0 });

        // insert zero‑crossing if sign changes next
        if (i+1 < N) {
            qreal y1 = std::clamp<qreal>(evals[i+1], -6.00, 6.00);
            if ((y0 > 0 && y1 < 0) || (y0 < 0 && y1 > 0)) {
                qreal t = y0 / (y0 - y1);
                m_areaPts.push_back({ i + t, 0.0 });
            }
        }
    }

    for (auto *ser : m_areaSeries) {
        m_chart->removeSeries(ser);
        delete ser;
    }
    m_areaSeries.clear();

    auto* posLine  = new QLineSeries;
    auto* negLine  = new QLineSeries;
    auto* zeroLine = new QLineSeries;
    for (auto &p : m_areaPts) {
        posLine ->append(p.x, std::max<qreal>(p.y, 0.0));
        negLine ->append(p.x, std::min<qreal>(p.y, 0.0));
        zeroLine->append(p.x, 0.0);
    }

    auto* whiteArea = new QAreaSeries(posLine, zeroLine);
    whiteArea->setPen(Qt::NoPen);
    whiteArea->setBrush(QColor(255,255,255,100));

    auto* blackArea = new QAreaSeries(zeroLine, negLine);
    blackArea->setPen(Qt::NoPen);
    blackArea->setBrush(QColor(0,0,0,100));

    m_chart->addSeries(whiteArea);
    m_chart->addSeries(blackArea);
    for (auto* s : { whiteArea, blackArea }) {
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisY);
    }

    m_lineSeries->clear();
    m_pointSeries->clear();
    m_zeroSeries->clear();

    for (auto &p : m_areaPts) {
        m_lineSeries ->append(p.x, p.y);
        m_pointSeries->append(p.x, p.y);
    }
    m_zeroSeries->append(0, 0);
    m_zeroSeries->append(m_areaPts.back().x, 0);

    // attach axes & set ranges
    QAbstractSeries* drawList[] = {
        static_cast<QAbstractSeries*>(m_lineSeries),
        static_cast<QAbstractSeries*>(m_pointSeries),
        static_cast<QAbstractSeries*>(m_zeroSeries)
    };
    for (auto* s : drawList) {
        m_chart->addSeries(s);
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisY);
    }

    m_axisX->setRange(0, m_areaPts.back().x);
    m_engine->quitEngine();
    delete m_engine;
}

// blocking evaluation
double GameReviewViewer::evaluateFen(const QString& fen)
{
    double lastCp = 0.0;
    QEventLoop loop;

    // catch info cp
    QMetaObject::Connection infoConn = connect(
        m_engine, &UciEngine::infoReceived,
        this, [&](const QString &line) {
            if (line.startsWith("info") && line.contains(" score cp ")) {
                auto toks = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                int idx = toks.indexOf("cp");
                if (idx >= 0 && idx+1 < toks.size())
                    lastCp = toks[idx+1].toDouble();
            }
        }
    );

    // exit on bestmove
    QMetaObject::Connection bestConn = connect(
        m_engine, &UciEngine::bestMove,
        &loop,  &QEventLoop::quit
    );

    // fail‐safe after movetime+20ms
    QTimer::singleShot(m_movetimeMs + 20, &loop, &QEventLoop::quit);

    m_engine->setPosition(fen);
    m_engine->goMovetime(m_movetimeMs);

    loop.exec();

    disconnect(infoConn);
    disconnect(bestConn);
    return lastCp;
}
