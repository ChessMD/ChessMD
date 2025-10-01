#include "gameplayviewer.h"
#include "chessqsettings.h"
#include "helpers.h"

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
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QOperatingSystemVersion>
#include <QDateTime>


GameplayViewer::GameplayViewer(ChessPosition *positionViewer, QWidget *parent)
    : QWidget(parent)
    , m_positionViewer(positionViewer)
    , m_engine(nullptr)
    , m_startPosition(new ChessPosition)
    , m_lastPosition(new ChessPosition)
    , m_root(new QWidget(this))
    , m_controlsWidget(nullptr)
    , m_whiteMs(0)
    , m_blackMs(0)
    , m_incMs(0)
    , m_humanSide(0)
    , m_active(false)
    , m_engineIdle(true)
{
    m_startPosition->copyFrom(*m_positionViewer);

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

    QHBoxLayout *engineSelectLayout = new QHBoxLayout;
    m_engineLabel = new QLabel(tr("Engine: <none>"), this);
    m_selectEngineBtn  = new QPushButton(tr("Select Engine…"), this);
    engineSelectLayout->addWidget(m_engineLabel);
    engineSelectLayout->addWidget(m_selectEngineBtn);
    preLay->addLayout(engineSelectLayout);

    QFrame *engineSep = new QFrame;
    engineSep->setFrameShape(QFrame::HLine);
    engineSep->setFrameShadow(QFrame::Sunken);
    preLay->addSpacing(8);
    preLay->addWidget(engineSep);
    preLay->addSpacing(8);

    m_timeCheck = new QCheckBox;
    m_timeCheck->setChecked(true);
    QLabel *timeSideLabel = new QLabel(tr("Clock enabled:"));
    QHBoxLayout *timeCheckRow = new QHBoxLayout;
    timeCheckRow->setSpacing(12);
    timeCheckRow->addStretch();
    timeCheckRow->addWidget(timeSideLabel);
    timeCheckRow->addWidget(m_timeCheck);
    timeCheckRow->addStretch();
    preLay->addLayout(timeCheckRow);
    preLay->addSpacing(8);

    connect(m_timeCheck, &QCheckBox::clicked, this, [this]{
        if (m_timeWidget->isHidden()) m_timeWidget->show();
        else m_timeWidget->hide();
        m_minutesSpin->setEnabled(!m_minutesSpin->isEnabled());
        m_secondsSpin->setEnabled(!m_secondsSpin->isEnabled());
        m_incrementSpin->setEnabled(!m_incrementSpin->isEnabled());
    });

    m_timeWidget = new QWidget;
    QHBoxLayout *timeRow = new QHBoxLayout(m_timeWidget);
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
    preLay->addWidget(m_timeWidget);

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
    const int buttonSize = 40, iconSize = 32;
    m_whiteBtn->setFixedSize(buttonSize, buttonSize);
    m_blackBtn->setFixedSize(buttonSize, buttonSize);
    m_randomBtn->setFixedSize(buttonSize, buttonSize);
    m_whiteBtn->setIconSize(QSize(iconSize, iconSize));
    m_blackBtn->setIconSize(QSize(iconSize, iconSize));
    m_whiteBtn->setCheckable(true);
    m_blackBtn->setCheckable(true);
    m_randomBtn->setCheckable(true);
    m_whiteBtn->setCursor(Qt::PointingHandCursor);
    m_blackBtn->setCursor(Qt::PointingHandCursor);
    m_randomBtn->setCursor(Qt::PointingHandCursor);

    const QString buttonStyle = QStringLiteral(R"(
        QPushButton {
            border: 2px solid #666;
            border-radius: 10px;
            background: palette(base);
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
            font-weight: bold;
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

    ChessQSettings s;
    s.loadSettings();
    QString saved = s.getEngineFile();
    if (!saved.isEmpty() && QFileInfo::exists(saved)) {
        m_engineLabel->setText(tr("Engine: %1").arg(QFileInfo(saved).fileName()));
        m_playBtn->setEnabled(true);
    } else {
        m_engineLabel->setText(tr("Engine: <none>"));
        m_playBtn->setEnabled(false);
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

        if (osVersion.type() == QOperatingSystemVersion::Windows) {
            binary = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), "./engine", tr("(*.exe)"));
        } else {
			if (osVersion.type() == QOperatingSystemVersion::MacOS) {
				QDir dirBin(QApplication::applicationDirPath());
				dirBin.cdUp(), dirBin.cdUp(), dirBin.cdUp();
				binary = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), dirBin.filePath("./engine"), tr("(*)"));
			} else {
				binary = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), "./engine", tr("(*)"));
			}
		}

        if (!binary.isEmpty()){
            ChessQSettings s;
            s.loadSettings();
            s.setEngineFile(binary);
            s.saveSettings();
            m_engineLabel->setText(tr("Engine: %1").arg(QFileInfo(binary).fileName()));
            m_playBtn->setEnabled(true);
        }
    });

    m_inGameWidget = new QWidget;
    QVBoxLayout *inLay = new QVBoxLayout(m_inGameWidget);
    inLay->setContentsMargins(pad, pad, pad, pad);
    inLay->setSpacing(6);

    QLabel *inGameTitle = new QLabel(tr("Game Menu"));
    QFont inGameFont = inGameTitle->font();
    inGameFont.setPointSize(14);
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
    m_blackPlayerLabel = new QLabel(tr("Engine"));
    m_blackPlayerLabel->setAlignment(Qt::AlignCenter);
    QFont blackF = m_blackPlayerLabel->font();
    blackF.setPointSize(12);
    m_blackPlayerLabel->setFont(blackF);
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
    QFont whiteF = m_whitePlayerLabel->font();
    whiteF.setPointSize(12);
    m_whitePlayerLabel->setFont(whiteF);
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
    m_resignBtn = new QPushButton;
    m_openAnalysisBtn = new QPushButton;
    QIcon resignIcon = QIcon(getIconPath("flag.png"));
    m_resignBtn->setFixedSize(buttonSize, buttonSize);
    m_resignBtn->setIconSize(QSize(iconSize, iconSize));
    m_resignBtn->setIcon(resignIcon);
    m_resignBtn->setCursor(Qt::PointingHandCursor);
    m_resignBtn->setToolTip(tr("Resign"));
    QIcon analysisIcon = QIcon(getIconPath("sparkles.png"));
    m_openAnalysisBtn->setFixedSize(buttonSize, buttonSize);
    m_openAnalysisBtn->setIconSize(QSize(iconSize, iconSize));
    m_openAnalysisBtn->setIcon(analysisIcon);
    m_openAnalysisBtn->setCursor(Qt::PointingHandCursor);
    m_openAnalysisBtn->setToolTip(tr("Open in analysis"));
    m_returnBtn = new QPushButton(tr("New Game"));
    m_returnBtn->setFixedHeight(buttonSize);
    m_returnBtn->setCursor(Qt::PointingHandCursor);
    m_takebackBtn = new QPushButton(tr("Takeback"));
    m_takebackBtn->setFixedHeight(buttonSize);
    m_takebackBtn->setCursor(Qt::PointingHandCursor);

    const QString gameButtonStyle = QStringLiteral(R"(
        QPushButton {
            font-size: 14px;
            font-weight: bold;
            border-radius: 10px;
            border: 1px solid gray;
        }
    )");

    m_takebackBtn->setStyleSheet(gameButtonStyle);
    m_returnBtn->setStyleSheet(gameButtonStyle);
    m_resignBtn->setStyleSheet(gameButtonStyle);
    m_openAnalysisBtn->setStyleSheet(gameButtonStyle);

    actions->addStretch();
    actions->addWidget(m_resignBtn);
    actions->addWidget(m_takebackBtn);
    actions->addWidget(m_returnBtn);
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
    m_returnBtn->setEnabled(false);
    m_openAnalysisBtn->setEnabled(false);
    m_takebackBtn->setEnabled(false);

    connect(m_resignBtn, &QPushButton::clicked, this, &GameplayViewer::onResignClicked);
    connect(m_returnBtn, &QPushButton::clicked, this, &GameplayViewer::onReturnClicked);
    connect(m_openAnalysisBtn, &QPushButton::clicked, this, &GameplayViewer::onOpenInAnalysisClicked);
    connect(m_takebackBtn, &QPushButton::clicked, this, &GameplayViewer::onTakebackClicked);
    connect(m_playBtn, &QPushButton::clicked, this, [this]{
        ChessQSettings s;
        s.loadSettings();
        QString saved = s.getEngineFile();
        if (saved.isEmpty() || !QFileInfo::exists(saved)) {
            m_engineLabel->setText(tr("Engine: <none>"));
            m_playBtn->setEnabled(false);
            return;
        }
        int selectedSide = m_sideButtonGroup->checkedId();
        if (selectedSide == 1) selectedSide = QRandomGenerator::global()->bounded(2);
        if (selectedSide == 2) selectedSide = 1;
        onPlayClicked(selectedSide);
    });

    connect(&m_updateTimer, &QTimer::timeout, this, &GameplayViewer::onClockTick);
    m_updateTimer.setSingleShot(true);  
}

void GameplayViewer::resetPlay()
{
    emit resetBoard();

    m_whiteMs = m_blackMs = 0;
    m_incMs = 0;
    m_active = false;
    m_engineIdle = true;
    m_positionStack.clear();
    m_positionHash.clear();

    m_preGameWidget->setVisible(true);
    m_inGameWidget->setVisible(false);

    m_playBtn->setEnabled(true);
    m_resignBtn->setEnabled(false);
    m_returnBtn->setEnabled(false);
    m_openAnalysisBtn->setEnabled(false);
    m_takebackBtn->setEnabled(false);

    if (m_updateTimer.isActive()) m_updateTimer.stop();
    m_clockTimer.invalidate();
}

void GameplayViewer::onPlayClicked(int selectedSide)
{
    emit resetBoard();

    m_active = true;
    m_engineIdle = true;
    m_humanSide = selectedSide;
    m_moveCount = 0;
    m_positionStack.clear();
    m_positionHash.clear();
    m_whiteMs = m_blackMs = (m_minutesSpin->value() * 60 + m_secondsSpin->value()) * 1000;
    m_incMs = m_incrementSpin->value() * 1000;
    m_engineDepth = 18 + int(std::round(double(m_eloSlider->value() - 1320) / double(3190 - 1320) * (25 - 18))); // linear depth function [20, 25]
    updateClockDisplays();
    m_lastPosition->copyFrom(*m_startPosition);
    m_positionViewer->m_premoveEnabled = selectedSide;
    m_engineElo = m_eloSlider->value();
    m_whitePlayerLabel->setText(tr("You"));
    m_blackPlayerLabel->setText(tr("Engine (%1)").arg(m_engineElo));
    m_preGameWidget->setVisible(false);
    m_inGameWidget->setVisible(true);
    m_playBtn->setEnabled(false);
    m_resignBtn->setEnabled(true);
    m_returnBtn->setEnabled(false);
    m_openAnalysisBtn->setEnabled(false);
    updateTakebackEnabled();
    emit matchBoardFlip(m_humanSide ? 'b' : 'w');

    startEngineProcess();
}

void GameplayViewer::updateTakebackEnabled()
{
    if (m_moveCount < 2) m_takebackBtn->setEnabled(false);
    else m_takebackBtn->setEnabled(m_moveCount % 2 == m_humanSide);
}

void GameplayViewer::startEngineProcess()
{
    if (m_engine) return;

    ChessQSettings s; s.loadSettings();
    QString engineSaved = s.getEngineFile();

    m_engine = new UciEngine(this);
    connect(m_engine, &UciEngine::bestMove, this, &GameplayViewer::onEngineBestMove);
    connect(m_engine, &UciEngine::infoReceived, this, &GameplayViewer::onEngineInfo);
    connect(m_engine, &UciEngine::nameReceived, this, &GameplayViewer::onNameReceived);

    m_engineReadyConn = connect(m_engine, &UciEngine::engineReady, this, [this]{
        disconnect(m_engineReadyConn); // one-time connection
        m_engine->setLimitStrength(true);
        m_engine->setOption("UCI_Elo", QString::number(m_engineElo));
        m_engine->setPosition("startpos");
        scheduleNextDisplayUpdate();
        if (m_humanSide == 1) {
            m_engineIdle = false;
            if (m_timeCheck->isChecked()) m_engine->goDepthWithClocks(m_engineDepth, m_whiteMs, m_blackMs, m_incMs, m_incMs);
            else m_engine->goDepth(m_engineDepth);
        }
    });

    m_engine->startEngine(engineSaved);
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
    m_engineIdle = true;
    if (!m_active) return;
    applyUciMove(uci);
}

bool GameplayViewer::applyUciMove(const QString &uci)
{
    if (uci.size() < 4) return false;
    emit selectLastMove();
    m_positionViewer->copyFrom(*m_lastPosition);
    int sc = uci[0].toLatin1() - 'a', sr = '8' - uci[1].toLatin1(), dc = uci[2].toLatin1() - 'a', dr = '8' - uci[3].toLatin1();
    QChar promo = (uci.size() >= 5 ? uci[4].toUpper() : '\0');
    if (!m_positionViewer->validateMove(sr, sc, dr, dc)) {
        qDebug() << "Engine played illegal move!" << sr << sc << dr << dc;
        return false;
    }
    m_positionViewer->buildUserMove(sr, sc, dr, dc, promo);
    return true;
}

void GameplayViewer::onBoardMoveMade(QSharedPointer<NotationMove>& move)
{
    if (!m_active || !m_engineIdle) return;
    if (isPlayersTurn()) { // engine move
        turnFinished();
        return;
    }
    m_engineIdle = false;
    turnFinished();
    if (!m_engine) return;
    m_engine->setPosition(move->m_position->positionToFEN());
    if (m_timeCheck->isChecked()) m_engine->goDepthWithClocks(m_engineDepth, m_whiteMs, m_blackMs, m_incMs, m_incMs);
    else m_engine->goDepth(m_engineDepth);
}

void GameplayViewer::turnFinished(){
    if (!m_active) return;

    m_lastPosition->copyFrom(*m_positionViewer);
    m_positionViewer->updatePremoves(m_premoves);
    if (m_lastPosition->m_sideToMove != (m_humanSide?'b':'w')){
        m_positionViewer->m_premoveEnabled = true;
    }

    if (m_moveCount >= 2){
        int elapsedMs = m_clockTimer.elapsed();
        int& timeMs = (m_lastPosition->m_sideToMove == 'w' ? m_blackMs : m_whiteMs); // sideToMove == 'w' -> black finished turn
        timeMs -= (elapsedMs - m_incMs);
    }
    updateClockDisplays();

    m_moveCount++;
    updateTakebackEnabled();
    if (!m_lastPosition->generateLegalMoves().size()){
        if (m_lastPosition->inCheck(m_lastPosition->m_sideToMove)){
            finishGame(m_lastPosition->m_sideToMove == 'w' ? "0-1" : "1-0", tr("By checkmate"));
        } else {
            finishGame("1/2-1/2", tr("By stalement"));
        }
    }
    if (m_lastPosition->isFiftyMove()){
        finishGame("1/2-1/2", tr("By 50-move rule"));
    }
    QString fen = m_lastPosition->positionToFEN(/*forHash=*/true);
    m_positionStack.push(fen);
    m_positionHash[fen]++;
    if (m_positionHash[fen] >= 3){
        finishGame("1/2-1/2", tr("By repetition"));
    }

    scheduleNextDisplayUpdate();

    // apply premoves from queue
    if (m_lastPosition->m_sideToMove == (m_humanSide?'b':'w') && m_premoves.size()){
        auto move = m_premoves.takeFirst();
        auto [sr, sc, dr, dc, promo] = move;
        m_positionViewer->copyFrom(*m_lastPosition);
        if (!m_lastPosition->validateMove(sr, sc, dr, dc)) { // illegal premove
            m_premoves.clear();
            m_positionViewer->updatePremoves(m_premoves);
        } else {
            m_positionViewer->buildUserMove(sr, sc, dr, dc, promo);
        }
    }
}

void GameplayViewer::scheduleNextDisplayUpdate()
{
    if (m_updateTimer.isActive()) m_updateTimer.stop();
    int& timeMs = (m_positionViewer->m_sideToMove == 'w' ? m_whiteMs : m_blackMs);
    int tenths = (timeMs+99)/100, delay = qMax(1, timeMs-(tenths-1)*100);
    m_updateTimer.start(static_cast<int>(delay));
    m_clockTimer.restart();
}

void GameplayViewer::onClockTick()
{
    if (!m_active || !m_timeCheck->isChecked() || m_moveCount < 2) return;
    int& timeMs = (m_positionViewer->m_sideToMove == 'w' ? m_whiteMs : m_blackMs);
    timeMs -= m_updateTimer.interval();
    updateClockDisplays();
    scheduleNextDisplayUpdate();
    if (timeMs <= 0) {
        finishGame(m_positionViewer->m_sideToMove == 'w' ? "0-1" : "1-0", tr("By timeout"));
    }
}

void GameplayViewer::updateClockDisplays()
{
    auto msToString = [this](int ms)->QString {
        if (!m_timeCheck->isChecked()) return "--:--";
        int tenths = (ms+99)/100, sec = tenths/10;
        if (sec < 30) return QString("%1:%2.%3").arg(sec/60).arg(sec%60, 2, 10, QLatin1Char('0')).arg(tenths%10);
        else return QString("%1:%2").arg(sec/60).arg(sec%60, 2, 10, QLatin1Char('0'));
    };
    bool isFlipped = m_positionViewer->isBoardFlipped();
    m_whiteClock->setText(msToString(isFlipped ? m_blackMs : m_whiteMs));
    m_blackClock->setText(msToString(isFlipped ? m_whiteMs : m_blackMs));
    m_whitePlayerLabel->setText((isFlipped && !m_humanSide) || (!isFlipped && m_humanSide) ? tr("%1 (%2)").arg(m_engineName).arg(m_engineElo) : tr("You"));
    m_blackPlayerLabel->setText((isFlipped && !m_humanSide) || (!isFlipped && m_humanSide) ? tr("You") : tr("%1 (%2)").arg(m_engineName).arg(m_engineElo));
    if ((m_positionViewer->m_sideToMove == 'w' && !isFlipped) || (m_positionViewer->m_sideToMove == 'b' && isFlipped)){
        m_whiteClock->setStyleSheet("border: 2px solid green; border-radius: 10px; padding: 6px; background: palette(base);");
        m_blackClock->setStyleSheet("border: 1px solid grey; border-radius: 10px; padding: 6px; background: palette(base);");
    } else {
        m_whiteClock->setStyleSheet("border: 1px solid grey; border-radius: 10px; padding: 6px; background: palette(base);");
        m_blackClock->setStyleSheet("border: 2px solid green; border-radius: 10px; padding: 6px; background: palette(base);");
    }
}

void GameplayViewer::finishGame(const QString &result, const QString &description)
{
    m_result = result;
    m_premoves.clear();
    m_positionViewer->updatePremoves(m_premoves);
    m_active = false;
    m_updateTimer.stop();
    stopEngineProcess();
    m_resignBtn->setEnabled(false);
    m_takebackBtn->setEnabled(false);
    m_returnBtn->setEnabled(true);
    m_openAnalysisBtn->setEnabled(true);

    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Game Complete"));
    dlg->setSizeGripEnabled(false);
    dlg->setMinimumWidth(220);
    dlg->setAttribute(Qt::WA_DeleteOnClose); // auto-delete when closed
    dlg->setModal(false);

    QVBoxLayout *dlgLay = new QVBoxLayout(dlg);
    dlgLay->setContentsMargins(16,16,16,16);
    dlgLay->setSpacing(12);

    QString resultText;
    if (result == "1-0") resultText = (m_humanSide == 0 ? tr("You Won!") : tr("%1 Won").arg(m_engineName));
    else if (result == "0-1") resultText = (m_humanSide == 0 ? tr("%1 Won").arg(m_engineName) : tr("You Won!"));
    else if (result == "1/2-1/2") resultText = tr("Draw");
    QLabel *title = new QLabel(resultText);
    QFont titleF = title->font();
    titleF.setPointSize(16);
    titleF.setBold(true);
    title->setFont(titleF);
    title->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    dlgLay->addWidget(title);
    dlgLay->addSpacing(0);

    QLabel *msg = new QLabel(description);
    QFont msgF = msg->font();
    msgF.setPointSize(12);
    msg->setFont(msgF);
    msg->setAlignment(Qt::AlignHCenter);
    msg->setWordWrap(true);
    dlgLay->addWidget(msg);

    QFrame *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setFixedHeight(2);
    dlgLay->addWidget(sep);

    QPushButton *openBtn = new QPushButton(tr("Game Review"));
    openBtn->setObjectName("endButton");
    openBtn->setFixedWidth(220);
    openBtn->setFixedHeight(44);
    openBtn->setCursor(Qt::PointingHandCursor);
    openBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    openBtn->setFocusPolicy(Qt::NoFocus);
    dlgLay->addWidget(openBtn);

    QPushButton *rematchBtn = new QPushButton(tr("Rematch"));
    rematchBtn->setObjectName("endButton");
    rematchBtn->setFixedHeight(44);
    rematchBtn->setCursor(Qt::PointingHandCursor);
    rematchBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rematchBtn->setFocusPolicy(Qt::NoFocus);

    QPushButton *returnBtn = new QPushButton(tr("New Game"));
    returnBtn->setObjectName("endButton");
    returnBtn->setFixedHeight(44);
    returnBtn->setCursor(Qt::PointingHandCursor);
    returnBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    returnBtn->setFocusPolicy(Qt::NoFocus);

    QWidget *btnContainer = new QWidget;
    btnContainer->setFixedWidth(220);
    QHBoxLayout *btnContainerLay = new QHBoxLayout(btnContainer);
    btnContainerLay->setContentsMargins(0, 0, 0, 0);
    btnContainerLay->setSpacing(12);
    btnContainerLay->addWidget(rematchBtn);
    btnContainerLay->addWidget(returnBtn);
    QHBoxLayout *centerRow = new QHBoxLayout;
    centerRow->addStretch();
    centerRow->addWidget(btnContainer);
    centerRow->addStretch();
    dlgLay->addLayout(centerRow);

    const QString endButtonStyle = QStringLiteral(R"(
        QPushButton#endButton {
            font-size: 14px;
            font-weight: bold;
            border-radius: 10px;
            border: 1px solid gray;
        }
    )");

    openBtn->setStyleSheet(endButtonStyle);
    rematchBtn->setStyleSheet(endButtonStyle);
    returnBtn->setStyleSheet(endButtonStyle);

    connect(rematchBtn, &QPushButton::clicked, dlg, [dlg]() { dlg->done(1); });
    connect(openBtn, &QPushButton::clicked, dlg, [dlg]() { dlg->done(2); });
    connect(returnBtn, &QPushButton::clicked, dlg, [dlg]() { dlg->done(3); });

    int res = dlg->exec();
    if (res == 1) {
        onRematchClicked();
    } else if (res == 2){
        onOpenInAnalysisClicked();
    } else if (res == 3){
        onReturnClicked();
    }
}

bool GameplayViewer::isPlayersTurn() const
{
    bool whiteToMove = m_positionViewer->m_sideToMove == 'w';
    if (whiteToMove) return (m_humanSide == 0);
    return (m_humanSide == 1);
}

void GameplayViewer::onResignClicked()
{
    finishGame(m_humanSide == 0 ? "0-1" : "1-0", tr("By Resignation"));
}

void GameplayViewer::onReturnClicked()
{
    stopEngineProcess();
    resetPlay();
}

void GameplayViewer::onRematchClicked()
{
    stopEngineProcess();
    resetPlay();
    onPlayClicked(!m_humanSide);
}

void GameplayViewer::onOpenInAnalysisClicked()
{
    QVector<QPair<QString,QString>> headerInfo;
    QDate date = QDateTime::currentDateTime().date();
    headerInfo.push_back({"Event", "ChessMD"});
    headerInfo.push_back({"White", m_humanSide == 0 ? "You" : m_engineName});
    headerInfo.push_back({"Black", m_humanSide == 0 ? m_engineName : "You"});
    headerInfo.push_back({QString("%1Elo").arg(m_humanSide == 0 ? "Black" : "White"), QString::number(m_engineElo)});
    headerInfo.push_back({"Result", m_result});
    headerInfo.push_back({"Date", QString("%1.%2.%3").arg(date.month()).arg(date.day()).arg(date.year())});
    emit openAnalysisBoard(headerInfo);
}

void GameplayViewer::onTakebackClicked()
{
    emit requestTakeback(m_humanSide == 0 ? 'w' : 'b');
    m_moveCount -= 2;
    updateTakebackEnabled();
    for (int i = 0; i < 2; i++){
        m_positionHash[m_positionStack.top()]--;
        m_positionStack.pop();
    }
}

void GameplayViewer::onNameReceived(const QString &name)
{
    m_engineName = name;
}

void GameplayViewer::onEngineInfo(const QString &line)
{

}
