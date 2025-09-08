#include "gameplayviewer.h"
#include "chessqsettings.h"
#include "pgngame.h"
#include "chessgamewindow.h"

#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>
#include <QDebug>

GameplayViewer::GameplayViewer(ChessPosition *positionViewer, QWidget *parent)
    : QWidget(parent)
    , m_positionViewer(positionViewer)
    , m_engine(nullptr)
    , m_root(new QWidget(this))
    , m_controlsWidget(nullptr)
    , m_whiteMs(0)
    , m_blackMs(0)
    , m_incMs(0)
    , m_humanSide(0)
    , m_active(false)
{
    QVBoxLayout *rootLay = new QVBoxLayout(this);
    rootLay->setContentsMargins(6,6,6,6);
    rootLay->setSpacing(8);

    const int pad = 20;
    m_preGameWidget = new QWidget;
    QVBoxLayout *preLay = new QVBoxLayout(m_preGameWidget);
    preLay->setContentsMargins(pad, pad, pad, pad);
    preLay->setSpacing(6);

    QLabel *title = new QLabel(tr("Play vs Engine"));
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    preLay->addWidget(title);

    QFrame *titleSep = new QFrame;
    titleSep->setFrameShape(QFrame::HLine);
    titleSep->setFrameShadow(QFrame::Sunken);
    preLay->addSpacing(8);
    preLay->addWidget(titleSep);
    preLay->addSpacing(8);

    QHBoxLayout *timeRow = new QHBoxLayout;
    QLabel *minLbl = new QLabel(tr("Minutes:"));
    m_minutesSpin = new QSpinBox;
    m_minutesSpin->setRange(0, 180);
    m_minutesSpin->setValue(5);
    QLabel *secLbl = new QLabel(tr("Seconds:"));
    m_secondsSpin = new QSpinBox;
    m_secondsSpin->setRange(0, 59);
    m_secondsSpin->setValue(0);
    QLabel *incLbl = new QLabel(tr("Inc (s):"));
    m_incrementSpin = new QSpinBox;
    m_incrementSpin->setRange(0, 60);
    m_incrementSpin->setValue(0);
    timeRow->addWidget(minLbl);
    timeRow->addWidget(m_minutesSpin);
    timeRow->addWidget(secLbl);
    timeRow->addWidget(m_secondsSpin);
    timeRow->addWidget(incLbl);
    timeRow->addWidget(m_incrementSpin);
    timeRow->addStretch();
    preLay->addLayout(timeRow);

    QFrame *timeSep = new QFrame;
    timeSep->setFrameShape(QFrame::HLine);
    timeSep->setFrameShadow(QFrame::Sunken);
    preLay->addSpacing(8);
    preLay->addWidget(timeSep);
    preLay->addSpacing(8);

    QHBoxLayout *strRow = new QHBoxLayout;
    strRow->setSpacing(12);

    QLabel *eloLabel = new QLabel(tr("Engine Elo:"));
    m_eloSlider = new QSlider(Qt::Horizontal);
    m_eloSlider->setRange(1320, 3190);
    m_eloSlider->setPageStep(10);
    m_eloSlider->setSingleStep(10);
    m_eloSlider->setValue(1600);

    m_eloSpin = new QSpinBox;
    m_eloSpin->setRange(1320, 3190);
    m_eloSpin->setValue(m_eloSlider->value());
    connect(m_eloSlider, &QSlider::valueChanged, m_eloSpin, &QSpinBox::setValue);
    connect(m_eloSpin, qOverload<int>(&QSpinBox::valueChanged), m_eloSlider, &QSlider::setValue);

    strRow->addWidget(eloLabel);
    strRow->addWidget(m_eloSlider, 1);
    strRow->addWidget(m_eloSpin);
    preLay->addLayout(strRow);

    QFrame *eloSep = new QFrame;
    eloSep->setFrameShape(QFrame::HLine);
    eloSep->setFrameShadow(QFrame::Sunken);
    preLay->addSpacing(8);
    preLay->addWidget(eloSep);
    preLay->addSpacing(8);

    QLabel *sideLabel = new QLabel(tr("Play as:"));
    sideLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    sideLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    m_whiteBtn = new QPushButton(this);
    m_blackBtn = new QPushButton(this);
    m_randomBtn = new QPushButton(this);
    const int iconSize = 40, kingIconSize = 32;
    m_whiteBtn->setFixedSize(iconSize, iconSize);
    m_blackBtn->setFixedSize(iconSize, iconSize);
    m_randomBtn->setFixedSize(iconSize, iconSize);
    m_whiteBtn->setIconSize(QSize(kingIconSize, kingIconSize));
    m_blackBtn->setIconSize(QSize(kingIconSize, kingIconSize));
    m_whiteBtn->setCheckable(true);
    m_blackBtn->setCheckable(true);
    m_randomBtn->setCheckable(true);
    m_whiteBtn->setCursor(Qt::PointingHandCursor);
    m_blackBtn->setCursor(Qt::PointingHandCursor);
    m_randomBtn->setCursor(Qt::PointingHandCursor);

    const QString buttonStyle = QStringLiteral(R"(
        QPushButton {
            border: 2px solid #666;
            border-radius: 8px;
            background: palette(base);
            font-size: 24px;
            font-weight: bold;
            padding: 0px;
        }
       QPushButton:checked {
            border: 3px solid #4CAF50;
        }
    )");
    const QString whiteButtonStyle = buttonStyle + QStringLiteral(R"(QPushButton { background: white; })");
    const QString blackButtonStyle = buttonStyle + QStringLiteral(R"(QPushButton { background: black; })");
    const QString randomButtonStyle = buttonStyle + QStringLiteral(R"(QPushButton { background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 white, stop: 0.4999 white, stop: 0.5001 black, stop: 1 black); })");
    m_whiteBtn->setStyleSheet(whiteButtonStyle);
    m_blackBtn->setStyleSheet(blackButtonStyle);
    m_randomBtn->setStyleSheet(randomButtonStyle);

    QIcon whiteKingIcon = QIcon(":/resource/img/chess-king-white.png");
    QIcon blackKingIcon = QIcon(":/resource/img/chess-king-black.png");
    m_whiteBtn->setIcon(whiteKingIcon);
    m_blackBtn->setIcon(blackKingIcon);
    m_whiteBtn->setToolTip(tr("Play as White"));
    m_blackBtn->setToolTip(tr("Play as Black"));
    m_randomBtn->setToolTip(tr("Random"));
    m_sideButtonGroup = new QButtonGroup(this);
    m_sideButtonGroup->addButton(m_whiteBtn, 0);
    m_sideButtonGroup->addButton(m_randomBtn, 1);
    m_sideButtonGroup->addButton(m_blackBtn, 2);
    m_randomBtn->setChecked(true);

    QWidget *rightPlaceholder = new QWidget;
    rightPlaceholder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    rightPlaceholder->setFixedWidth(sideLabel->sizeHint().width());

    QHBoxLayout *sideRow = new QHBoxLayout;
    sideRow->setSpacing(12);
    sideRow->addWidget(sideLabel);
    sideRow->addStretch(1);
    sideRow->addWidget(m_whiteBtn);
    sideRow->addWidget(m_randomBtn);
    sideRow->addWidget(m_blackBtn);
    sideRow->addStretch(1);
    sideRow->addWidget(rightPlaceholder);
    preLay->addLayout(sideRow);

    QFrame *sideSep = new QFrame;
    sideSep->setFrameShape(QFrame::HLine);
    sideSep->setFrameShadow(QFrame::Sunken);
    preLay->addSpacing(8);
    preLay->addWidget(sideSep);
    preLay->addSpacing(8);

    m_playBtn = new QPushButton(tr("Start Game!"));
    m_playBtn->setObjectName("playButton");
    m_playBtn->setMinimumWidth(140);
    m_playBtn->setFixedHeight(44);
    m_playBtn->setCursor(Qt::PointingHandCursor);
    m_playBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_playBtn->setStyleSheet(R"(
        QPushButton#playButton {
            font-size: 16px;
            padding: 8px 16px;
            border-radius: 10px;
            border: 1px solid gray;
        }
    )");
    QHBoxLayout *playRow = new QHBoxLayout;
    playRow->setContentsMargins(0, 0, 0, 0);
    playRow->addStretch();
    playRow->addWidget(m_playBtn);
    playRow->addStretch();
    preLay->addLayout(playRow);

    m_inGameWidget = new QWidget;
    QVBoxLayout *inLay = new QVBoxLayout(m_inGameWidget);
    inLay->setContentsMargins(pad, pad, pad, pad);
    inLay->setSpacing(6);

    QLabel *inGameTitle = new QLabel(tr("Game Menu"));
    QFont inGameFont = inGameTitle->font();
    inGameFont.setPointSize(16);
    inGameFont.setBold(true);
    inGameTitle->setFont(inGameFont);
    inGameTitle->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    inGameTitle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    inLay->addWidget(inGameTitle);

    QFrame *gameTitleSep = new QFrame;
    gameTitleSep->setFrameShape(QFrame::HLine);
    gameTitleSep->setFrameShadow(QFrame::Sunken);
    inLay->addSpacing(8);
    inLay->addWidget(gameTitleSep);
    inLay->addSpacing(8);

    QHBoxLayout *topArea = new QHBoxLayout;
    m_blackPlayerLabel = new QLabel(tr("Stockfish"));
    m_blackPlayerLabel->setAlignment(Qt::AlignCenter);
    m_blackClock = new QLabel(tr("00:15"));
    m_blackClock->setAlignment(Qt::AlignCenter);
    QFont bigClockFont = m_blackClock->font();
    bigClockFont.setPointSize(22);
    bigClockFont.setBold(true);
    m_blackClock->setFont(bigClockFont);
    m_blackClock->setMinimumSize(160, 48);
    m_blackClock->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_blackClock->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  border: 1px solid grey;"
        "  border-radius: 10px;"
        "  padding: 6px;"
        "  background: palette(base);"
        "}"
    ));
    QWidget *blackTimerWidget = new QWidget;
    QVBoxLayout *blackTimerLay = new QVBoxLayout(blackTimerWidget);
    blackTimerLay->setContentsMargins(0, 0, 0, 0);
    blackTimerLay->addWidget(m_blackPlayerLabel, 0, Qt::AlignCenter);
    blackTimerLay->addWidget(m_blackClock, 0, Qt::AlignCenter);
    topArea->addStretch();
    topArea->addWidget(blackTimerWidget);
    topArea->addStretch();
    inLay->addLayout(topArea);

    // m_moveNotationWidget = new QWidget;
    // m_moveNotationWidget->setMinimumHeight(300);
    // m_moveNotationWidget->setMinimumWidth(500);
    // m_moveNotationWidget->setObjectName("notationPanel");
    // m_moveNotationWidget->setStyleSheet("QWidget#notationPanel { border: 1px solid grey; border-radius: 8px; }");
    // QVBoxLayout *notationLayout = new QVBoxLayout(m_moveNotationWidget);
    // QLabel *placeholderLabel = new QLabel(tr("Move notation will appear here"));
    // placeholderLabel->setAlignment(Qt::AlignCenter);
    // notationLayout->addWidget(placeholderLabel);
    // inLay->addWidget(m_moveNotationWidget);

    QHBoxLayout *bottomArea = new QHBoxLayout;
    m_whitePlayerLabel = new QLabel(tr("You"));
    m_whitePlayerLabel->setAlignment(Qt::AlignCenter);
    m_whiteClock = new QLabel(tr("00:15"));
    m_whiteClock->setAlignment(Qt::AlignCenter);
    m_whiteClock->setFont(bigClockFont);
    m_whiteClock->setMinimumSize(160, 48);
    m_whiteClock->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_whiteClock->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  border: 1px solid grey;"
        "  border-radius: 10px;"
        "  padding: 6px;"
        "  background: palette(base);"
        "}"
    ));
    QWidget *whiteTimerWidget = new QWidget;
    QVBoxLayout *whiteTimerLay = new QVBoxLayout(whiteTimerWidget);
    whiteTimerLay->setContentsMargins(0, 0, 0, 0);
    whiteTimerLay->addWidget(m_whiteClock, 0, Qt::AlignCenter);
    whiteTimerLay->addWidget(m_whitePlayerLabel, 0, Qt::AlignCenter);
    bottomArea->addStretch();
    bottomArea->addWidget(whiteTimerWidget);
    bottomArea->addStretch();
    inLay->addLayout(bottomArea);

    QFrame *bottomSep = new QFrame;
    bottomSep->setFrameShape(QFrame::HLine);
    bottomSep->setFrameShadow(QFrame::Sunken);
    inLay->addSpacing(8);
    inLay->addWidget(bottomSep);
    inLay->addSpacing(8);

    QHBoxLayout *actions = new QHBoxLayout;
    actions->setSpacing(8);
    m_resignBtn = new QPushButton(tr("GG"));
    m_playAgainBtn = new QPushButton(tr("↻"));
    m_openAnalysisBtn = new QPushButton(tr("⚙"));

    const QString actionButtonStyle = QStringLiteral(R"(
        QPushButton {
            min-width: 32px;
            max-width: 32px;
            min-height: 32px;
            max-height: 32px;
        }
    )");

    m_resignBtn->setStyleSheet(actionButtonStyle);
    m_playAgainBtn->setStyleSheet(actionButtonStyle);
    m_openAnalysisBtn->setStyleSheet(actionButtonStyle);

    m_resignBtn->setToolTip(tr("Resign"));
    m_playAgainBtn->setToolTip(tr("Play again"));
    m_openAnalysisBtn->setToolTip(tr("Open in analysis"));

    actions->addStretch();
    actions->addWidget(m_resignBtn);
    actions->addWidget(m_playAgainBtn);
    actions->addWidget(m_openAnalysisBtn);
    actions->addStretch();
    inLay->addLayout(actions);

    const int centerMaxWidth = 800;
    m_preGameWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_preGameWidget->setMaximumWidth(centerMaxWidth);
    m_inGameWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_inGameWidget->setMaximumWidth(centerMaxWidth);

    rootLay->addStretch();
    rootLay->addWidget(m_preGameWidget, 0, Qt::AlignHCenter);
    rootLay->addWidget(m_inGameWidget, 0, Qt::AlignHCenter);
    rootLay->addStretch();

    m_preGameWidget->setVisible(true);
    m_inGameWidget->setVisible(false);

    const QString panelStyle = QStringLiteral(R"(
        QWidget#panelCard {
            background: palette(base);
            border: 1px solid rgba(0,0,0,0.12);
            border-radius: 12px;
        }
    )");

    m_preGameWidget->setObjectName("panelCard");
    m_inGameWidget->setObjectName("panelCard");
    m_preGameWidget->setStyleSheet(panelStyle);
    m_inGameWidget->setStyleSheet(panelStyle);

    m_resignBtn->setEnabled(false);
    m_playAgainBtn->setEnabled(false);
    m_openAnalysisBtn->setEnabled(false);

    connect(m_playBtn, &QPushButton::clicked, this, &GameplayViewer::onPlayClicked);
    connect(m_resignBtn, &QPushButton::clicked, this, &GameplayViewer::onResignClicked);
    connect(m_playAgainBtn, &QPushButton::clicked, this, &GameplayViewer::onPlayAgainClicked);
    connect(m_openAnalysisBtn, &QPushButton::clicked, this, &GameplayViewer::onOpenInAnalysisClicked);

    connect(&m_updateTimer, &QTimer::timeout, this, &GameplayViewer::onClockTick);
    m_updateTimer.setSingleShot(true);
}

void GameplayViewer::resetPlay()
{
    emit resetBoard();

    m_whiteMs = m_blackMs = 0;
    m_incMs = 0;
    m_active = false;

    m_preGameWidget->setVisible(true);
    m_inGameWidget->setVisible(false);

    m_playBtn->setEnabled(true);
    m_resignBtn->setEnabled(false);
    m_playAgainBtn->setEnabled(false);
    m_openAnalysisBtn->setEnabled(false);

    if (m_updateTimer.isActive()) m_updateTimer.stop();
    m_clockTimer.invalidate();

    updateClockDisplays();
}

void GameplayViewer::onPlayClicked()
{
    // prepare clocks & engine
    int selectedSide = m_sideButtonGroup->checkedId();
    if (selectedSide == 1) selectedSide = QRandomGenerator::global()->bounded(2);
    if (selectedSide == 2) selectedSide = 1;
    m_humanSide = selectedSide;
    m_whiteMs = m_blackMs = (m_minutesSpin->value() * 60 + m_secondsSpin->value()) * 1000;
    m_incMs =  m_incrementSpin->value() * 1000;

    if (m_humanSide == 0) {
        m_whitePlayerLabel->setText(tr("You"));
        m_blackPlayerLabel->setText(tr("Stockfish"));
    } else {
        m_whitePlayerLabel->setText(tr("Stockfish"));
        m_blackPlayerLabel->setText(tr("You"));
    }

    startEngineProcess();
    if (!m_engine) {
        QMessageBox::warning(this, tr("Engine error"), tr("Could not start engine."));
        return;
    }

    // configure engine strength
    m_engine->setLimitStrength(true);
    m_engine->setOption("UCI_Elo", QString::number(m_eloSlider->value()));
    m_engine->setPosition("startpos");

    // UI state
    m_preGameWidget->setVisible(false);
    m_inGameWidget->setVisible(true);

    m_active = true;
    m_playBtn->setEnabled(false);
    m_resignBtn->setEnabled(true);
    m_playAgainBtn->setEnabled(false);
    m_openAnalysisBtn->setEnabled(false);

    m_clockTimer.restart();
    scheduleNextDisplayUpdate();

    bool enginePlaysWhite = (m_humanSide == 1);
    if (enginePlaysWhite) {
        m_engine->goWithClocks(m_whiteMs, m_blackMs, 0, 0);
    }
}

void GameplayViewer::startEngineProcess()
{
    if (m_engine) return;
    m_engine = new UciEngine(this);
    connect(m_engine, &UciEngine::bestMove, this, &GameplayViewer::onEngineBestMove);
    connect(m_engine, &UciEngine::infoReceived, this, &GameplayViewer::onEngineInfo);

    ChessQSettings s; s.loadSettings();
    QString engineSaved = s.getEngineFile();
    m_engine->startEngine(engineSaved);
    m_engine->sendRawCommand("ucinewgame");
}

void GameplayViewer::stopEngineProcess()
{
    if (!m_engine) return;
    m_engine->quitEngine();
    m_engine->deleteLater();
    m_engine = nullptr;
}

void GameplayViewer::onEngineBestMove(const QString &uci)
{
    if (!m_active || !applyUciMove(uci)) return;
    turnFinished();
}

bool GameplayViewer::applyUciMove(const QString &uci)
{
    if (uci.size() < 4) return false;
    int sc = uci[0].toLatin1() - 'a', sr = '8' - uci[1].toLatin1(), dc = uci[2].toLatin1() - 'a', dr = '8' - uci[3].toLatin1();
    QChar promo = (uci.size() >= 5 ? uci[4].toUpper() : '\0');
    if (!m_positionViewer->validateMove(sr, sc, dr, dc)) {
        qDebug() << "Engine played illegal move!" << sr << sc << dr << dc;
        return false;
    }
    m_positionViewer->buildUserMove(sr, sc, dr, dc, promo);
    emit m_positionViewer->boardDataChanged();
    emit m_positionViewer->lastMoveChanged();
    return true;
}

void GameplayViewer::onBoardMoveMade(QSharedPointer<NotationMove>& move)
{
    if (!m_active || isPlayersTurn()) return;
    turnFinished();
    m_engine->setPosition(move->m_position->positionToFEN());
    m_engine->goWithClocks(m_whiteMs, m_blackMs, m_incMs, m_incMs);
}

void GameplayViewer::turnFinished(){
    int elapsedMs = m_clockTimer.elapsed();
    int& timeMs = (m_positionViewer->m_sideToMove == 'b' ? m_whiteMs : m_blackMs); // sideToMove == 'w' -> black finished turn
    timeMs -= (elapsedMs - m_incMs);
    updateClockDisplays();
    scheduleNextDisplayUpdate();
    m_clockTimer.restart();
}

void GameplayViewer::scheduleNextDisplayUpdate()
{
    if (m_updateTimer.isActive()) m_updateTimer.stop();
    int& timeMs = (m_positionViewer->m_sideToMove == 'w' ? m_whiteMs : m_blackMs);
    int tenths = (timeMs+99)/100, delay = qMax(1, timeMs-(tenths-1)*100);
    m_updateTimer.start(static_cast<int>(delay));
    m_clockTimer.restart();
    if (m_positionViewer->m_sideToMove == 'w'){
        m_whiteClock->setStyleSheet("border: 1px solid green; border-radius: 10px; padding: 6px; background: palette(base);");
        m_blackClock->setStyleSheet("border: 1px solid grey; border-radius: 10px; padding: 6px; background: palette(base);");
    } else {
        m_whiteClock->setStyleSheet("border: 1px solid grey; border-radius: 10px; padding: 6px; background: palette(base);");
        m_blackClock->setStyleSheet("border: 1px solid green; border-radius: 10px; padding: 6px; background: palette(base);");
    }
}

void GameplayViewer::onClockTick()
{
    if (!m_active) return;
    int& timeMs = (m_positionViewer->m_sideToMove == 'w' ? m_whiteMs : m_blackMs);
    timeMs -= m_updateTimer.interval();
    updateClockDisplays();
    scheduleNextDisplayUpdate();
    if (timeMs <= 0) {
        finishGame(tr("Time out"));
    }
}

void GameplayViewer::updateClockDisplays()
{
    auto msToString = [](int ms)->QString {
        int tenths = (ms+99)/100, sec = tenths/10;
        if (sec < 50) return QString("%1:%2.%3").arg(sec/60).arg(sec%60, 2, 10, QLatin1Char('0')).arg(tenths%10);
        else return QString("%1:%2").arg(sec/60).arg(sec%60, 2, 10, QLatin1Char('0'));
    };
    m_whiteClock->setText(msToString(m_whiteMs));
    m_blackClock->setText(msToString(m_blackMs));
}

void GameplayViewer::finishGame(const QString &result)
{
    m_active = false;
    m_updateTimer.stop();
    stopEngineProcess();

    QMessageBox dlg(this);
    dlg.setWindowTitle(tr("Game Over"));
    dlg.setText(result);
    dlg.setInformativeText(tr("Open in analysis or play again?"));
    QPushButton *openBtn = dlg.addButton(tr("Open in analysis"), QMessageBox::AcceptRole);
    QPushButton *againBtn = dlg.addButton(tr("Play again"), QMessageBox::ActionRole);
    dlg.addButton(tr("Close"), QMessageBox::RejectRole);
    dlg.exec();

    if (dlg.clickedButton() == openBtn) {
        onOpenInAnalysisClicked();
    } else if (dlg.clickedButton() == againBtn) {
        onPlayAgainClicked();
    }

    m_resignBtn->setEnabled(false);
    m_playAgainBtn->setEnabled(true);
    m_openAnalysisBtn->setEnabled(true);
}

bool GameplayViewer::isPlayersTurn() const
{
    qDebug() << m_humanSide <<  m_positionViewer->m_sideToMove ;
    bool whiteToMove = m_positionViewer->m_sideToMove == 'w';
    if (whiteToMove) return (m_humanSide == 0);
    return (m_humanSide == 1);
}

void GameplayViewer::onResignClicked()
{
    finishGame(tr("Resigned - human resigned"));
}

void GameplayViewer::onPlayAgainClicked()
{
    stopEngineProcess();
    resetPlay();
}

void GameplayViewer::onOpenInAnalysisClicked()
{
    emit openAnalysisBoard();
}

void GameplayViewer::onEngineInfo(const QString &line)
{

}
