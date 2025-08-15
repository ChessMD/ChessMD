#include "gamereviewviewer.h"
#include "chessposition.h"
#include "chessqsettings.h"
#include "helpers.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QTimer>
#include <QHeaderView>
#include <QToolTip>
#include <QMouseEvent>
#include <QCursor>
#include <QOperatingSystemVersion>

GameReviewViewer::GameReviewViewer(QSharedPointer<NotationMove> rootMove, QWidget *parent)
    : QWidget(parent)
    , m_rootMove(rootMove)
{
    auto *lay = new QVBoxLayout(this);
    m_whiteAccuracyLabel = new QLabel(tr("White accuracy: –"), this);
    m_blackAccuracyLabel = new QLabel(tr("Black accuracy: –"), this);
    m_whiteAccuracyLabel->setFixedHeight(35);
    m_blackAccuracyLabel->setFixedHeight(35);
    m_whiteAccuracyLabel->setStyleSheet(R"(
        QLabel {
            background: white; //hcc
            color: black; //hcc
            border: 1px solid #888; /*hcc*/
            border-radius: 4px;
            padding: 4px 8px;
            font-weight: bold;
            font-size: 14px;
        }
    )");
    m_blackAccuracyLabel->setStyleSheet(R"(
        QLabel {
            background: #333; /*hcc*/
                    color: white; //hcc
            border: 1px solid #888; /*hcc*/
            border-radius: 4px;
            padding: 4px 8px;
            font-weight: bold;
            font-size: 14px;
        }
    )");
    auto *hLay = new QHBoxLayout;
    hLay->addWidget(m_whiteAccuracyLabel);
    hLay->addWidget(m_blackAccuracyLabel);
    lay->addLayout(hLay);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    lay->addWidget(m_progressBar);

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
    m_axisY->setRange(-6.30, 6.30);

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
    m_chart->setBackgroundBrush(QBrush(QColor(230, 230, 230))); //hcc

    m_inaccuracySeries = new QScatterSeries;
    m_mistakeSeries = new QScatterSeries;
    m_blunderSeries = new QScatterSeries;

    QColor outlineColor = m_lineSeries->pen().color();
    QPen outlinePen(outlineColor);
    outlinePen.setWidth(1);

    for (QScatterSeries* s : { m_inaccuracySeries, m_mistakeSeries, m_blunderSeries}){
        s->setMarkerSize(8);
        s->setMarkerShape(QScatterSeries::MarkerShapeCircle);
        s->setPen(outlinePen);
    }

    m_inaccuracySeries->setBrush(QBrush(QColor(247,198,49))); //hcc
    m_mistakeSeries->setBrush(QBrush(QColor(255,164,89))); //hcc
    m_blunderSeries->setBrush(QBrush(QColor(250, 65, 45))); //hcc

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setFixedHeight(250);
    m_chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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

    createSummaryGrid();
    lay->addWidget(m_summaryWidget);

    auto *engineSelectLayout = new QHBoxLayout;
    m_engineLabel      = new QLabel(tr("Engine: <none>"), this);
    m_selectEngineBtn  = new QPushButton(tr("Select Engine…"), this);
    engineSelectLayout->addWidget(m_engineLabel);
    engineSelectLayout->addWidget(m_selectEngineBtn);
    lay->addLayout(engineSelectLayout);

    m_reviewBtn = new QPushButton(tr("Start Game Review"), this);
    m_reviewBtn->setIcon(QIcon(getIconPath("sparkles.png")));
    m_reviewBtn->setIconSize(QSize(48,48));
    m_reviewBtn->setMinimumHeight(100);
    m_reviewBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_reviewBtn->setStyleSheet(
        "font-size: 28px; "
        "padding: 20px 40px;"
    );
    m_reviewBtn->setEnabled(false);
    lay->addWidget(m_reviewBtn);

    m_settings.loadSettings();
    QString saved = m_settings.getEngineFile();
    if (!saved.isEmpty() && QFileInfo(saved).exists()) {
        // auto-start
        m_engine = new UciEngine(this);
        m_engine->startEngine(saved);
        m_engineLabel->setText(tr("Engine: %1").arg(QFileInfo(saved).fileName()));
        m_reviewBtn->setEnabled(true);
    } else {
        // no engine yet
        m_engine = nullptr;
        m_engineLabel->setText(tr("Engine: <none>"));
        m_reviewBtn->setEnabled(false);
    }

    connect(m_selectEngineBtn, &QPushButton::clicked, this, [this]() {
        QOperatingSystemVersion osVersion = QOperatingSystemVersion::current();
        QString binary;
        QString exeDir = QCoreApplication::applicationDirPath();
        QDir dir(exeDir);
        if (dir.cd("engine")) {
            // path is "<parent_of_exe>/engine"
        } else {
            dir = QDir(exeDir);
        }

        if (osVersion.type() == QOperatingSystemVersion::Windows)
            binary = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), dir.absolutePath(), tr("(*.exe)"));
        else
            binary = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), dir.absolutePath(), tr("(*)"));

        m_settings.loadSettings();
        m_settings.setEngineFile(binary);
        m_settings.saveSettings();

        // start or restart the engine
        if (m_engine) {
            m_engine->quitEngine();
        }
        m_engine = new UciEngine(this);
        m_engine->startEngine(binary);

        m_engineLabel->setText(tr("Engine: %1").arg(QFileInfo(binary).fileName()));
        m_reviewBtn->setEnabled(true);
    });


    // hide UI until game review is clicked
    m_progressBar->setVisible(false);
    m_whiteAccuracyLabel->setVisible(false);
    m_blackAccuracyLabel->setVisible(false);
    m_chartView->setVisible(false);
    m_summaryWidget->setVisible(false);

    connect(m_reviewBtn, &QPushButton::clicked, this, [this]() {
        m_reviewBtn->hide();
        m_progressBar->setVisible(true);
        m_whiteAccuracyLabel->setVisible(true);
        m_blackAccuracyLabel->setVisible(true);
        m_chartView->setVisible(true);
        m_summaryWidget->setVisible(true);
        m_selectEngineBtn->setVisible(false);
        reviewGame(m_rootMove);
    });
}

void GameReviewViewer::createSummaryGrid()
{
    m_summaryWidget = new QWidget(this);
    m_summaryWidget->setObjectName("summaryPanel");
    auto *lay = new QVBoxLayout(m_summaryWidget);
    lay->setContentsMargins(6,6,6,6);
    lay->setSpacing(6);

    QWidget* gridW = new QWidget(m_summaryWidget);
    auto *g = new QGridLayout(gridW);
    g->setContentsMargins(0,0,0,0);
    g->setHorizontalSpacing(8);
    g->setVerticalSpacing(6);

    QStringList columns = { "Brilliant", "Great", "Best", "Inaccuracy", "Mistake", "Blunder" };
    QMap<QString, QString> colorMap;
    colorMap["Brilliant"] = "#26C2A3";
    colorMap["Great"] = "#749BBF";
    colorMap["Best"] = "#81B64C";
    colorMap["Inaccuracy"] = "#F7C631";
    colorMap["Mistake"] = "#FFA459";
    colorMap["Blunder"] = "#FA412D";

    // sizing for value cells
    const int valueCellWidth = 96;
    const int valueCellHeight = 36;
    const int leftLabelWidth = 72;

    g->setColumnMinimumWidth(0, leftLabelWidth);
    g->setColumnStretch(0, 0);

    for (int c = 1; c <= columns.size(); ++c) {
        g->setColumnMinimumWidth(c, valueCellWidth);
        g->setColumnStretch(c, 0);
    }

    gridW->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    const QString boldStyle = "font-weight:700;";

    // top-left blank
    QLabel* leftHeader = new QLabel(QString(), gridW);
    leftHeader->setObjectName("leftHeader");
    g->addWidget(leftHeader, 0, 0);

    for (int i = 0; i < columns.size(); i++) {
        QLabel* catLabel = new QLabel(QObject::tr(columns[i].toUtf8().constData()), gridW);
        catLabel->setAlignment(Qt::AlignCenter);
        catLabel->setMinimumSize(valueCellWidth, valueCellHeight);
        catLabel->setStyleSheet(QString("font-size:13px; %1 padding-left:6px;").arg(boldStyle));
        g->addWidget(catLabel, 0, i + 1, Qt::AlignCenter);
    }

    const int ICON_PX = 24;
    for (int i = 0; i < columns.size(); i++) {
        const QString cat = columns[i];
        QLabel* header = new QLabel(gridW);
        header->setAlignment(Qt::AlignCenter);
        header->setMinimumSize(valueCellWidth, valueCellHeight);

        QString iconName = QString(":/resource/img/%1-icon").arg(cat.toLower());
        QIcon icon(iconName);
        if (!icon.isNull()) {
            header->setPixmap(icon.pixmap(ICON_PX, ICON_PX));
        } else {
            header->setText(cat);
            header->setStyleSheet(QString("font-weight:700; font-size:12px; color:%1;").arg(colorMap.value(cat, "#444444")));
        }
        g->addWidget(header, 2, i + 1, Qt::AlignCenter);
    }

    QLabel* whiteLabel = new QLabel(QObject::tr("White:"), gridW);
    whiteLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    whiteLabel->setMinimumWidth(leftLabelWidth);
    whiteLabel->setStyleSheet(QString("font-size:13px; %1 padding-left:6px;").arg(boldStyle));
    g->addWidget(whiteLabel, 1, 0, Qt::AlignVCenter);

    QLabel* blackLabel = new QLabel(QObject::tr("Black:"), gridW);
    blackLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    blackLabel->setMinimumWidth(leftLabelWidth);
    blackLabel->setStyleSheet(QString("font-size:13px; %1 padding-left:6px;").arg(boldStyle));
    g->addWidget(blackLabel, 3, 0, Qt::AlignVCenter);

    auto makeValue = [&](QLabel*& out, const QString& cellName) -> QLabel* {
        out = new QLabel(QStringLiteral("–"), gridW);
        out->setObjectName(cellName); // "whiteCell" or "blackCell"
        out->setMinimumSize(valueCellWidth, valueCellHeight);
        out->setAlignment(Qt::AlignCenter);
        out->setStyleSheet(QString("font-weight:700;"));
        out->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        return out;
    };

    // add cells for each category
    for (int i = 0; i < columns.size(); ++i) {
        const int col = i + 1;
        const QString category = columns[i];
        if (category == "Brilliant") {
            makeValue(m_whiteBrilliantLabel, "whiteCell"); g->addWidget(m_whiteBrilliantLabel, 1, col, Qt::AlignCenter);
            makeValue(m_blackBrilliantLabel, "blackCell"); g->addWidget(m_blackBrilliantLabel, 3, col, Qt::AlignCenter);
        } else if (category == "Great") {
            makeValue(m_whiteGreatLabel, "whiteCell"); g->addWidget(m_whiteGreatLabel, 1, col, Qt::AlignCenter);
            makeValue(m_blackGreatLabel, "blackCell"); g->addWidget(m_blackGreatLabel, 3, col, Qt::AlignCenter);
        } else if (category == "Best") {
            makeValue(m_whiteBestLabel, "whiteCell"); g->addWidget(m_whiteBestLabel, 1, col, Qt::AlignCenter);
            makeValue(m_blackBestLabel, "blackCell"); g->addWidget(m_blackBestLabel, 3, col, Qt::AlignCenter);
        } else if (category == "Inaccuracy") {
            makeValue(m_whiteInaccuracyLabel, "whiteCell"); g->addWidget(m_whiteInaccuracyLabel, 1, col, Qt::AlignCenter);
            makeValue(m_blackInaccuracyLabel, "blackCell"); g->addWidget(m_blackInaccuracyLabel, 3, col, Qt::AlignCenter);
        } else if (category == "Mistake") {
            makeValue(m_whiteMistakeLabel, "whiteCell"); g->addWidget(m_whiteMistakeLabel, 1, col, Qt::AlignCenter);
            makeValue(m_blackMistakeLabel, "blackCell"); g->addWidget(m_blackMistakeLabel, 3, col, Qt::AlignCenter);
        } else if (category == "Blunder") {
            makeValue(m_whiteBlunderLabel, "whiteCell"); g->addWidget(m_whiteBlunderLabel, 1, col, Qt::AlignCenter);
            makeValue(m_blackBlunderLabel, "blackCell"); g->addWidget(m_blackBlunderLabel, 3, col, Qt::AlignCenter);
        }
    }

    lay->addWidget(gridW, 0, Qt::AlignHCenter);
    const QString qss = R"(
        QWidget#summaryPanel { background: transparent; }
        QLabel#whiteCell { background: #FFFFFF; color: #111111; border: 1px solid rgba(0,0,0,0.06); border-radius:6px; font-size:18px; padding:2px; }
        QLabel#blackCell { background: #0F0F0F; color: #FFFFFF; border: 1px solid rgba(255,255,255,0.06); border-radius:6px; font-size:18px; padding:2px; }
    )";
    gridW->setStyleSheet(qss);
}

bool GameReviewViewer::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_chartView->viewport() && event->type() == QEvent::MouseMove) {
        if (m_origPts.empty() || m_axisX->min() >= m_axisX->max() || m_axisY->min() >= m_axisY->max()) {
            return false;
        }
        QMouseEvent  *me = static_cast<QMouseEvent*>(event);
        QRectF plotArea = m_chart->plotArea();
        m_chartView->viewport()->setCursor(Qt::PointingHandCursor);
        if (!plotArea.contains(me->pos())) {
            m_hoverMarker->setVisible(false);
            m_vLine->setVisible(false);
            QToolTip::hideText();
            m_chartView->viewport()->unsetCursor();
            return false;
        }
        qreal relX = (me->pos().x() - plotArea.left()) / plotArea.width();
        int maxIdx = int(m_origPts.size()) - 1;
        int idx = std::clamp(qRound(relX * maxIdx), 0, maxIdx);
        EvalPt p = m_origPts[idx];

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

        QString prefix = QString::number((idx-1)/2+1) + QString(idx%2 ? "." : "...");
        QString tip = QString("%1%2\nEvaluation: %3").arg(idx ? prefix : "").arg((m_moves[idx]->moveText != "" ? m_moves[idx]->moveText : "Starting Position")).arg(p.y,0,'f',2);
        QToolTip::showText(glPt, tip, m_chartView);

        // vertical line
        qreal yTop = m_axisY->max(), yBot = m_axisY->min();
        QPointF topScene = m_chartView->mapToScene(m_chart->mapToPosition({ p.x, yTop }, m_lineSeries).toPoint());
        QPointF botScene = m_chartView->mapToScene(m_chart->mapToPosition({ p.x, yBot }, m_lineSeries).toPoint());
        m_vLine->setLine(topScene.x(), topScene.y(), botScene.x(), botScene.y());
        m_vLine->setVisible(true);

        return true;
    }
    if (watched == m_chartView->viewport() && event->type() == QEvent::MouseButtonPress){
        QMouseEvent  *me = static_cast<QMouseEvent*>(event);
        QRectF plotArea = m_chart->plotArea();
        m_chartView->viewport()->setCursor(Qt::PointingHandCursor);
        if (!plotArea.contains(me->pos())) {
            m_chartView->viewport()->unsetCursor();
            return false;
        }
        qreal relX = (me->pos().x() - plotArea.left()) / plotArea.width();
        int maxIdx = int(m_moves.size()) - 1;
        int idx = std::clamp(qRound(relX * maxIdx), 0, maxIdx);

        QSharedPointer<NotationMove> selectedMove = m_moves[idx];
        emit moveSelected(selectedMove);
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
    for (int i = 0; i < m; i++){
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

void GameReviewViewer::startNextEval()
{
    if (!m_isReviewing) return;

    if (m_pending.isEmpty()) {
        finalizeReview();
        return;
    }

    auto pe = m_pending.dequeue();
    m_currentEvalIndex = pe.index;
    m_lastCp = 0.0;

    m_progressBar->setValue(m_currentEvalIndex);

    // position + go
    m_engine->setPosition(pe.fen);
    m_engine->goMovetime(m_movetimeMs);
}

void GameReviewViewer::onInfoReceived(const QString& line)
{
    if (!m_isReviewing || !line.startsWith("info")) return;

    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    int scoreIndex = tokens.indexOf("score");
    if (scoreIndex < 0 || scoreIndex + 2 >= tokens.size()) return;

    QString type = tokens.value(scoreIndex+1);
    if (type == "cp"){
        m_lastCp = tokens[scoreIndex+2].toDouble();
    } else if (type == "mate"){
        const double MATE_CP_SENTINEL = 100000.0;
        m_lastCp = (tokens[scoreIndex+2].toInt() > 0) ? MATE_CP_SENTINEL - std::min(tokens[scoreIndex+2].toInt(), 900) : -MATE_CP_SENTINEL + std::min(tokens[scoreIndex+2].toInt(), 900);
    }
}

void GameReviewViewer::onBestMove(const QString& bestMove)
{
    if (!m_isReviewing) return;
    // store and continue
    m_results[m_currentEvalIndex] = m_lastCp;
    startNextEval();
}

void GameReviewViewer::reviewGame(const QSharedPointer<NotationMove>& root)
{
    QVector<QString> fens;
    m_moves.clear();
    if (!root || !root->m_position) return;
    fens.append(root->m_position->positionToFEN());
    m_moves.append(root);
    auto cur = root;
    while (cur && !cur->m_nextMoves.isEmpty()) {
        auto nxt = cur->m_nextMoves.front();
        if (!nxt->m_position) break;
        fens.append(nxt->m_position->positionToFEN());
        m_moves.append(nxt);
        cur = nxt;
    }

    m_totalEvals = fens.size();
    m_progressBar->setRange(0, m_totalEvals - 1);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);

    m_pending.clear();
    int N = fens.size();
    for (int i = 0; i < N; ++i)
        m_pending.enqueue({fens[i], i});
    m_results.assign(N, 0.0);
    m_isReviewing = true;

    connect(m_engine, &UciEngine::infoReceived, this, &GameReviewViewer::onInfoReceived);
    connect(m_engine, &UciEngine::bestMove, this, &GameReviewViewer::onBestMove);

    startNextEval();
}

void GameReviewViewer::finalizeReview()
{
    // clean up
    m_isReviewing = false;
    disconnect(m_engine, nullptr, this, nullptr);
    m_progressBar->setVisible(false);

    int whiteInacc = 0, whiteMist = 0, whiteBlund = 0, whiteBest = 0;
    int blackInacc = 0, blackMist = 0, blackBlund = 0, blackBest = 0;
    std::vector<double> winPercentages, evals;
    int moves = m_results.size() - 1;
    for (int i = 0; i < moves; i++) {
        double cpBefore = m_results[i];
        double cpAfter = -m_results[i+1];
        double wb = winProb(cpBefore);
        double wa = winProb(cpAfter);
        double acc = moveAccuracy(wb, wa);

        double drop = std::abs(wb - wa);
        QSharedPointer<NotationMove> move = m_moves[i+1];
        if (drop >= 0.18) move->annotation1 = "??";
        else if (drop >= 0.12) move->annotation1 = "?";
        else if (drop >= 0.06) move->annotation1 = "?!";
        else move->annotation1.clear();

        winPercentages.push_back(wb);
        evals.push_back(cpBefore/100.0);
        if (i + 1 == moves){
            winPercentages.push_back(1.0-wa);
            evals.push_back(-cpAfter/100.0);
        }

        if (i % 2 == 0) {
            if (move->annotation1 == "?!") whiteInacc++;
            else if (move->annotation1 == "?") whiteMist++;
            else if (move->annotation1 == "??") whiteBlund++;
            if (acc >= 95.0) whiteBest++;
        } else {
            if (move->annotation1 == "?!") blackInacc++;
            else if (move->annotation1 == "?") blackMist++;
            else if (move->annotation1 == "??") blackBlund++;
            if (acc >= 95.0) blackBest++;
        }
    }

    // adjust evaluation to be white's perspective
    for (int i = 0; i <= evals.size(); i++){
        if (i % 2 == 1){
            evals[i] = -evals[i];
        }
    }

    auto [avgW, avgB] = gameAccuracy(winPercentages, true);
    m_whiteAccuracyLabel->setText(tr("White accuracy: %1%").arg(QString::number(avgW, 'f', 1)));
    m_blackAccuracyLabel->setText(tr("Black accuracy: %1%").arg(QString::number(avgB, 'f', 1)));

    auto setCellText = [](QLabel* lbl, const QString &text){
        if (!lbl) return;
        lbl->setText(text);
    };

    setCellText(m_whiteBrilliantLabel, QString("?"));
    setCellText(m_blackBrilliantLabel, QString("?"));
    setCellText(m_whiteGreatLabel, QString("?"));
    setCellText(m_blackGreatLabel, QString("?"));
    setCellText(m_whiteBestLabel, QString::number(whiteBest));
    setCellText(m_blackBestLabel, QString::number(blackBest));
    setCellText(m_whiteInaccuracyLabel, QString::number(whiteInacc));
    setCellText(m_blackInaccuracyLabel, QString::number(blackInacc));
    setCellText(m_whiteMistakeLabel, QString::number(whiteMist));
    setCellText(m_blackMistakeLabel, QString::number(blackMist));
    setCellText(m_whiteBlunderLabel, QString::number(whiteBlund));
    setCellText(m_blackBlunderLabel, QString::number(blackBlund));

    m_origPts.clear();
    m_areaPts.clear();
    int N = int(evals.size());

    for (int i = 0; i < N; ++i) {
        qreal y0 = std::clamp<qreal>(evals[i], -6.00, 6.00);
        // original
        m_origPts.push_back({(qreal)i, y0});
        // area base
        m_areaPts.push_back({(qreal)i, y0});

        // insert zero‑crossing if sign changes next
        if (i+1 < N) {
            qreal y1 = std::clamp<qreal>(evals[i+1], -6.00, 6.00);
            if ((y0 > 0 && y1 < 0) || (y0 < 0 && y1 > 0)) {
                qreal t = y0 / (y0 - y1);
                m_areaPts.push_back({ i + t, 0.0 });
            }
        }
    }
    for (auto *series : std::as_const(m_areaSeries)) {
        m_chart->removeSeries(series);
        delete series;
    }
    m_areaSeries.clear();

    auto* posLine = new QLineSeries;
    auto* negLine = new QLineSeries;
    auto* zeroLine = new QLineSeries;
    for (auto &p : m_areaPts) {
        posLine->append(p.x, std::max<qreal>(p.y, 0.0));
        negLine->append(p.x, std::min<qreal>(p.y, 0.0));
        zeroLine->append(p.x, 0.0);
    }

    auto* whiteArea = new QAreaSeries(posLine, zeroLine);
    whiteArea->setPen(Qt::NoPen);
    whiteArea->setBrush(QColor(255,255,255,100)); //hcc

    auto* blackArea = new QAreaSeries(zeroLine, negLine);
    blackArea->setPen(Qt::NoPen);
    blackArea->setBrush(QColor(0,0,0,100)); //hcc

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

    m_inaccuracySeries->clear();
    m_mistakeSeries->clear();
    m_blunderSeries->clear();

    for (int i = 0; i < m_origPts.size(); ++i) {
        const EvalPt &pt = m_origPts[i];
        auto mv = m_moves[i];
        if (! mv) continue;  // skip index 0 "start"

        QString ann = mv->annotation1;
        if (ann == "?!") m_inaccuracySeries->append(pt.x, pt.y);
        else if (ann == "?")  m_mistakeSeries->append(pt.x, pt.y);
        else if (ann == "??") m_blunderSeries->append(pt.x, pt.y);
    }

    for (QScatterSeries* s : {m_inaccuracySeries, m_mistakeSeries, m_blunderSeries}){
        m_chart->removeSeries(s);
        m_chart->addSeries(s);
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisY);
    }

    m_axisX->setRange(0, m_areaPts.back().x);
    m_engine->quitEngine();
    emit reviewCompleted();
}
