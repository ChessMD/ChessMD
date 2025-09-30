/*
March 18, 2025: File Creation
*/

#include "chessgamewindow.h"
#include "notation.h"
#include "notationviewer.h"
#include "streamparser.h"
#include "chessposition.h"
#include "engineviewer.h"
#include "pgnsavedialog.h"
#include "helpers.h"

#include <QDebug>
#include <QLabel>
#include <QString>
#include <QTextStream>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QTextEdit>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QPushButton>
#include <QShortcut>
#include <QKeySequence>
#include <QQmlEngine>
#include <QDialogButtonBox>

// Constructs a ChessGameWindow inside a parent widget
ChessGameWindow::ChessGameWindow(QWidget *parent, PGNGame game)
    : QMainWindow{parent}
    , m_engineDock(nullptr)
    , m_openingDock(nullptr)
{
    setMinimumSize(1024,768);
    setGeometry(100, 100, 0, 0);
    setWindowFlags(Qt::Widget);
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AnimatedDocks);

    // set the chessboard as central widget of window, and link to QML
    QQuickWidget *boardView = new QQuickWidget;
    m_positionViewer = new ChessPosition(this);
    boardView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    boardView->rootContext()->setContextProperty("chessPosition", m_positionViewer);
    boardView->setSource(QUrl(QStringLiteral("qrc:/chessboard.qml")));
    boardView->setMinimumSize(200, 200);
    setCentralWidget(boardView);
    connect(m_positionViewer, &ChessPosition::moveMade, this, &ChessGameWindow::onMoveMade);
    connect(m_positionViewer, &ChessPosition::premoveMade, this, &ChessGameWindow::onPremoveMade);

    // link to the rootmove containing the entire game tree
    m_notationViewer = new NotationViewer(game, this);
    m_notationViewer->setRootMove(m_notationViewer->getRootMove());

    QShortcut* prevMove = new QShortcut(QKeySequence(Qt::Key_Left), this);
    QShortcut* nextMove = new QShortcut(QKeySequence(Qt::Key_Right), this);
    QShortcut* flipBoard = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(prevMove, &QShortcut::activated, this, &ChessGameWindow::onPrevMoveShortcut);
    connect(nextMove, &QShortcut::activated, this, &ChessGameWindow::onNextMoveShortcut);
    connect(flipBoard, &QShortcut::activated, this, &ChessGameWindow::onFlipBoardShortcut);
}

void ChessGameWindow::closeEvent(QCloseEvent *event)
{
    if (m_isPreview || m_isGameplay || !m_notationViewer->m_isEdited) {
        QMainWindow::closeEvent(event);
        return;
    }

    QMessageBox message = QMessageBox(this);
    message.setIcon(QMessageBox::Question);
    message.setWindowTitle(tr("Save changes?"));
    message.setText(tr("Do you want to save your changes to this game?"));
    auto saveBtn = message.addButton(tr("Yes"), QMessageBox::AcceptRole);
    auto noSaveBtn = message.addButton(tr("No"), QMessageBox::DestructiveRole);
    auto cancelBtn = message.addButton(tr("Cancel"), QMessageBox::RejectRole);
    message.setDefaultButton(saveBtn);
    message.exec();

    if (message.clickedButton() == cancelBtn) {
        event->ignore();
        return;
    }

    if (message.clickedButton() == saveBtn) {
        PGNSaveDialog dialog(this);
        dialog.setHeaders(m_notationViewer->m_game);
        if (dialog.exec() != QDialog::Accepted) {
            event->ignore();
            return;
        }
        dialog.applyTo(m_notationViewer->m_game);
        QString result; QTextStream out(&result); int plyCount = 0;
        writeMoves(m_notationViewer->getRootMove(), out, plyCount);
        m_notationViewer->m_game.bodyText = result.trimmed();
        saveGame();
    }

    event->accept();
}

// Configures ChessGameWindow for complete analysis
void ChessGameWindow::mainSetup(){
    m_isPreview = false;
    connectEditingShortcuts();
    notationSetup();
    // notationToolbarSetup();
    toolbarSetup();
    // engineSetup();
    updateEngineActions();
    gameReviewSetup();
    // openingSetup();
    updateOpeningActions();
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    resizeDocks({m_notationDock}, {int(width() )}, Qt::Horizontal);
    QShortcut* saveGame = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(saveGame, &QShortcut::activated, this, &ChessGameWindow::onSavePgnClicked);
}

// Configures ChessGameWindow for previewing
void ChessGameWindow::previewSetup()
{
    m_isPreview = true;
    connectEditingShortcuts();
    notationSetup();
    addDockWidget(Qt::BottomDockWidgetArea, m_notationDock);
    m_notationDock->show();
}

void ChessGameWindow::gameplaySetup()
{
    m_isGameplay = true;
    notationSetup();
    toolbarSetup();
    m_saveGameAction->setEnabled(false);
    m_gameReviewAction->setEnabled(false);
    m_startEngineAction->setEnabled(false);
    m_stopEngineAction->setEnabled(false);
    m_openOpeningExplorerAction->setEnabled(false);
    m_closeOpeningExplorerAction->setEnabled(false);

    m_notationDock->hide();

    // create viewer with pointer to the same ChessPosition
    m_gameplayViewer = new GameplayViewer(m_positionViewer, this);
    m_gameplayDock = new QDockWidget(tr("Play vs Engine"), this);
    m_gameplayDock->setContextMenuPolicy(Qt::PreventContextMenu);
    m_gameplayDock->setWidget(m_gameplayViewer);
    m_gameplayDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_gameplayDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, m_gameplayDock);
    m_gameplayDock->show();

    connect(m_positionViewer, &ChessPosition::moveMade, m_gameplayViewer, &GameplayViewer::onBoardMoveMade);
    connect(m_positionViewer, &ChessPosition::isBoardFlippedChanged, m_gameplayViewer, &GameplayViewer::updateClockDisplays);
    connect(m_gameplayViewer, &GameplayViewer::selectLastMove, this, &ChessGameWindow::onSelectLastMove);
    connect(m_gameplayViewer, &GameplayViewer::resetBoard, this, &ChessGameWindow::onResetBoard);
    connect(m_gameplayViewer, &GameplayViewer::matchBoardFlip, this, &ChessGameWindow::onMatchBoardFlip);
    connect(m_gameplayViewer, &GameplayViewer::requestTakeback, this, &ChessGameWindow::onRequestTakeback);
    connect(m_gameplayViewer, &GameplayViewer::openAnalysisBoard, this, [this](QVector<QPair<QString,QString>> headerInfo){
        PGNGame game;
        game.copyFrom(m_notationViewer->m_game);
        game.headerInfo = headerInfo;
        emit openAnalysisBoard(game);
    });

    QTimer::singleShot(0, this, [this](){
        if (m_gameplayDock)
            resizeDocks({m_gameplayDock}, {int(width() * 0.4)}, Qt::Horizontal);
    });
}

void ChessGameWindow::startGameReview(){
    if (!m_gameReviewDock->isVisible()){
        m_gameReviewDock->setVisible(true);
        m_gameReviewViewer->autoStartReview();
    }
}

void ChessGameWindow::connectEditingShortcuts()
{
    QShortcut* deleteMove = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    QShortcut* deleteVariation = new QShortcut(QKeySequence("Ctrl+D"), this);
    QShortcut* promoteVariation = new QShortcut(QKeySequence("Ctrl+Up"), this);
    connect(deleteMove, &QShortcut::activated, this, &ChessGameWindow::onDeleteAfterShortcut);
    connect(deleteVariation, &QShortcut::activated, this, &ChessGameWindow::onDeleteVariationShortcut);
    connect(promoteVariation, &QShortcut::activated, this, &ChessGameWindow::onPromoteVariationShortcut);
}

void ChessGameWindow::saveGame(){
    m_notationViewer->m_isEdited = false;
    refreshHeader();
    emit PGNGameUpdated(m_notationViewer->m_game);
}

void ChessGameWindow::refreshHeader() {
    QString white, whiteElo, black, blackElo, result, round, event, date;
    for (auto &hdr : m_notationViewer->m_game.headerInfo) {
        if (hdr.first == "White") white = hdr.second;
        else if (hdr.first == "WhiteElo") whiteElo = hdr.second;
        else if (hdr.first == "Black") black = hdr.second;
        else if (hdr.first == "BlackElo") blackElo = hdr.second;
        else if (hdr.first == "Result") result = hdr.second;
        else if (hdr.first == "Round") round = hdr.second;
        else if (hdr.first == "Event") event = hdr.second;
        else if (hdr.first == "Date") date = hdr.second;
    }
    m_whiteField->setText(white);
    m_whiteEloField->setText(whiteElo);
    m_blackField->setText(black);
    m_blackEloField->setText(blackElo);
    m_resultField->setText(result);
    m_eventField->setText(event);
    m_roundField->setText(round);
    m_dateField->setText(date);
    adjustFieldWidth(m_whiteField);
    adjustFieldWidth(m_whiteEloField);
    adjustFieldWidth(m_blackField);
    adjustFieldWidth(m_blackEloField);
    adjustFieldWidth(m_resultField);
    adjustFieldWidth(m_eventField);
    adjustFieldWidth(m_roundField);
    adjustFieldWidth(m_dateField);
}

void ChessGameWindow::toggleEditMode() {
    m_inEditMode = !m_inEditMode;
    // swap pencil/check icon
    m_toggleEditBtn->setIcon(QIcon(m_inEditMode ? getIconPath("check.png") : getIconPath("edit.png")));
    // toggle each field
    auto toggleField = [&](QLineEdit* e){
        e->setProperty("editable", m_inEditMode);
        e->setReadOnly(!m_inEditMode);
        e->style()->unpolish(e);
        e->style()->polish(e);
        e->setCursor(m_inEditMode ? Qt::IBeamCursor : Qt::ArrowCursor);
    };
    toggleField(m_whiteField);
    toggleField(m_whiteEloField);
    toggleField(m_blackField);
    toggleField(m_blackEloField);
    toggleField(m_resultField);
    toggleField(m_eventField);
    toggleField(m_roundField);
    toggleField(m_dateField);

    if (!m_inEditMode) {
        m_notationViewer->m_isEdited = true;
        auto &hdr = m_notationViewer->m_game.headerInfo;
        auto setOrAppend = [&](QString k, QString v){
            for (auto &p: hdr)
                if (p.first==k) { p.second=v; return; }
            hdr.append({k,v});
        };
        setOrAppend("White", m_whiteField->text());
        setOrAppend("WhiteElo", m_whiteEloField->text());
        setOrAppend("Black", m_blackField->text());
        setOrAppend("BlackElo", m_blackEloField->text());
        setOrAppend("Result", m_resultField->text());
        setOrAppend("Event", m_eventField->text());
        setOrAppend("Round", m_roundField->text());
        setOrAppend("Date", m_dateField->text());
        refreshHeader();
    }
}

void ChessGameWindow::adjustFieldWidth(QLineEdit* e, int buffer) {
    QString s = e->text().isEmpty() ? e->placeholderText() : e->text();
    int w = e->fontMetrics().horizontalAdvance(s);
    QMargins m = e->textMargins();
    QMargins contentsMargins = e->contentsMargins();
    int frame = e->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    int totalW = w + m.left() + m.right() + contentsMargins.left() + contentsMargins.right() + frame*2 + buffer;
    e->setFixedWidth(totalW);
}

// Builds a dockable notation panel
void ChessGameWindow::notationSetup()
{
    QWidget *container = new QWidget(this);
    QVBoxLayout *vlay = new QVBoxLayout(container);
    vlay->setContentsMargins(0,0,0,0);
    vlay->setSpacing(0);

    QWidget* headerWidget = new QWidget(container);
    headerWidget->setObjectName("gameInfoHeader");
    headerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    headerWidget->setStyleSheet(R"(
        QWidget#gameInfoHeader {
            background: palette(alternate-base); 
            font-weight: bold;
            font-size: 12pt;
        }
    )");
    QVBoxLayout* headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0,0,0,0);
    headerLayout->setSpacing(0);

    QWidget* topRow = new QWidget(headerWidget);
    QHBoxLayout* topHBox = new QHBoxLayout(topRow);
    topHBox->setContentsMargins(0,0,0,0);
    topHBox->setSpacing(0);

    auto makeSlot = [&](QLineEdit*& edit, const QString& placeholder){
        edit = new QLineEdit(topRow);
        edit->setReadOnly(true);
        edit->setFrame(false);
        edit->setCursor(Qt::ArrowCursor);
        edit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        edit->setPlaceholderText(placeholder);
        edit->setTextMargins(0, 0, 0, 0);
        edit->setStyleSheet(R"(
            QLineEdit {
                border: none;
                font-size: 12pt;
                font-weight: bold;
                background: palette(alternate-base); 
                padding: 0;
            }
            QLineEdit[editable="true"] {
                border: 1px solid palette(text); 
                font-size: 12pt;
                font-weight: bold;
                background: palette(alternate-base); 
            }
        )");
        edit->style()->unpolish(edit);
        edit->style()->polish(edit);
        return edit;
    };

    makeSlot(m_whiteField, tr("White"));
    makeSlot(m_whiteEloField, tr("WhiteElo"));
    makeSlot(m_blackField, tr("Black"));
    makeSlot(m_blackEloField, tr("BlackElo"));
    makeSlot(m_resultField, tr("Result"));

    topHBox->addStretch();
    topHBox->addWidget(m_whiteField, 2);
    topHBox->addWidget(m_whiteEloField, 1);
    QLabel* vsLabel = new QLabel(tr("vs"), topRow);
    vsLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    vsLabel->setContentsMargins(0, 0, 2, 0);
    vsLabel->setStyleSheet(R"(
        QLabel {
            border: none;
            font-size: 12pt;
            font-weight: bold;
        }
    )");
    topHBox->addWidget(vsLabel, 0);
    topHBox->addWidget(m_blackField, 2);
    topHBox->addWidget(m_blackEloField, 1);
    topHBox->addWidget(m_resultField, 1);
    topHBox->addStretch();

    m_toggleEditBtn = new QToolButton(topRow);
    m_toggleEditBtn->setIcon(QIcon(getIconPath("edit.png")));
    m_toggleEditBtn->setIconSize(QSize(20,20));
    m_toggleEditBtn->setAutoRaise(true);
    m_toggleEditBtn->setToolTip(tr("Edit game info"));
    topHBox->addWidget(m_toggleEditBtn, 0, Qt::AlignRight);

    headerLayout->addWidget(topRow);

    QWidget* bottomRow = new QWidget(headerWidget);
    QHBoxLayout* botHBox = new QHBoxLayout(bottomRow);
    botHBox->setContentsMargins(0,0,0,0);
    botHBox->setSpacing(0);

    makeSlot(m_eventField, tr("Event"));
    makeSlot(m_roundField, tr("Round"));
    makeSlot(m_dateField, tr("Date"));

    botHBox->addStretch();
    botHBox->addWidget(m_eventField, 3);
    botHBox->addWidget(m_roundField, 1);
    botHBox->addWidget(m_dateField, 1);
    botHBox->addStretch();

    headerLayout->addWidget(bottomRow);

    connect(m_toggleEditBtn, &QToolButton::clicked, this, &ChessGameWindow::toggleEditMode);
    connect(m_whiteField, &QLineEdit::textChanged, this, [this](const QString&){ adjustFieldWidth(m_whiteField); });
    connect(m_whiteEloField, &QLineEdit::textChanged, this, [this](const QString&){ adjustFieldWidth(m_whiteEloField); });
    connect(m_blackField, &QLineEdit::textChanged, this, [this](const QString&){ adjustFieldWidth(m_blackField); });
    connect(m_blackEloField, &QLineEdit::textChanged, this, [this](const QString&){ adjustFieldWidth(m_blackEloField); });
    connect(m_resultField, &QLineEdit::textChanged, this, [this](const QString&){ adjustFieldWidth(m_resultField); });
    connect(m_eventField, &QLineEdit::textChanged, this, [this](const QString&){ adjustFieldWidth(m_eventField); });
    connect(m_roundField, &QLineEdit::textChanged, this, [this](const QString&){ adjustFieldWidth(m_roundField); });
    connect(m_dateField, &QLineEdit::textChanged, this, [this](const QString&){ adjustFieldWidth(m_dateField); });
    refreshHeader();

    // Add headerWidget to main layout
    vlay->addWidget(headerWidget, 0);
    vlay->addWidget(m_notationViewer, 1);

    m_notationDock = new QDockWidget(tr("Notation"), this);
    m_notationDock->setContextMenuPolicy(Qt::PreventContextMenu);
    m_notationDock->setWidget(container);
    m_notationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    m_notationDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_notationDock->setMinimumSize(100, 0);

    // connect moveSelected signal when user clicks on a move in the notation
    connect(m_notationViewer, &NotationViewer::moveSelected, this, &ChessGameWindow::onMoveSelected);

    addDockWidget(Qt::RightDockWidgetArea, m_notationDock);
}

// Builds the toolbar with additional game controls
void ChessGameWindow::toolbarSetup()
{
    m_Toolbar = new QToolBar(tr("Toolbar"));
    m_Toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_Toolbar->setIconSize(QSize(32,32));
    m_Toolbar->setFloatable(false);
    m_Toolbar->setMovable(false);
    m_Toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QAction* back = m_Toolbar->addAction("Back (Left Arrow)");
    back->setIcon(QIcon(getIconPath("arrow-left.png")));
    connect(back, &QAction::triggered, this, &ChessGameWindow::onPrevMoveShortcut);

    QAction* forward = m_Toolbar->addAction("Forward (Right Arrow)");
    forward->setIcon(QIcon(getIconPath("arrow-right.png")));
    connect(forward, &QAction::triggered, this, &ChessGameWindow::onNextMoveShortcut);

    QAction* flipBoard = m_Toolbar->addAction("Flip Board (Ctrl+F)");
    flipBoard->setIcon(QIcon(getIconPath("flip-board.png")));
    connect(flipBoard, &QAction::triggered, this, &ChessGameWindow::onFlipBoardShortcut);

    m_saveGameAction = m_Toolbar->addAction("Save (Ctrl+S)");
    m_saveGameAction->setIcon(QIcon(getIconPath("savegame.png")));
    connect(m_saveGameAction, &QAction::triggered, this, &ChessGameWindow::onSavePgnClicked);

    m_gameReviewAction = m_Toolbar->addAction("Game Review");
    m_gameReviewAction->setIcon(QIcon(getIconPath("sparkles.png")));
    connect(m_gameReviewAction, &QAction::triggered, this, [this]() {
        if (!m_gameReviewDock) return;
        bool visible = m_gameReviewDock->isVisible();
        m_gameReviewDock->setVisible(!visible);
    });

    m_startEngineAction = m_Toolbar->addAction("Start Engine");
    m_startEngineAction->setIcon(QIcon(getIconPath("engine-start.png")));
    connect(m_startEngineAction, &QAction::triggered, this, &ChessGameWindow::engineSetup);

    m_stopEngineAction = m_Toolbar->addAction("Stop engine");
    m_stopEngineAction->setIcon(QIcon(getIconPath("engine-stop.png")));
    connect(m_stopEngineAction, &QAction::triggered, this, &ChessGameWindow::engineTeardown);

    m_openOpeningExplorerAction = m_Toolbar->addAction("Open Opening Explorer");
    m_openOpeningExplorerAction->setIcon(QIcon(getIconPath("book.png")));
    connect(m_openOpeningExplorerAction, &QAction::triggered, this, &ChessGameWindow::openingSetup);

    m_closeOpeningExplorerAction = m_Toolbar->addAction("Close Opening Explorer");
    m_closeOpeningExplorerAction->setIcon(QIcon(getIconPath("book-off.png")));
    connect(m_closeOpeningExplorerAction, &QAction::triggered, this, &ChessGameWindow::openingTeardown);
    addToolBar(m_Toolbar);
}

// Builds the engine dockable panel
void ChessGameWindow::engineSetup()
{
    if (m_engineDock){
        return;
    }

    // create the engine dockable panel
    m_engineViewer = new EngineWidget(m_notationViewer->getSelectedMove(), this);
    m_engineDock = new QDockWidget(tr("Engine"), this);
    m_engineDock->setContextMenuPolicy(Qt::PreventContextMenu);
    m_engineDock->setWidget(m_engineViewer);
    m_engineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    m_engineDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, m_engineDock);

    if (m_notationDock){
        splitDockWidget(m_notationDock, m_engineDock, Qt::Vertical);
    }

    QTimer::singleShot(0, this, [this](){
        if (!m_notationDock || !m_engineDock) return;
        resizeDocks({m_notationDock, m_engineDock}, {2, 1}, Qt::Vertical);
    });

    // update engine and board display when position changes
    connect(m_notationViewer, &NotationViewer::moveSelected, m_engineViewer, &EngineWidget::onMoveSelected);
    connect(m_engineViewer, &EngineWidget::engineMoveClicked, m_notationViewer, &NotationViewer::onEngineMoveClicked);
    connect(m_engineViewer, &EngineWidget::moveHovered, this, &ChessGameWindow::onMoveHovered);
    connect(m_engineViewer, &EngineWidget::noHover, this, &ChessGameWindow::onNoHover);
    connect(m_engineViewer, &EngineWidget::engineEvalScoreChanged, this, &ChessGameWindow::onEvalScoreChanged);

    updateEngineActions();
}

// Destroys the engine dockable panel
void ChessGameWindow::engineTeardown()
{
    if (!m_engineDock){
        return;
    }

    removeDockWidget(m_engineDock);
    m_engineDock->setWidget(nullptr);
    m_engineViewer->deleteLater();
    m_engineDock->deleteLater();
    m_engineViewer = nullptr;
    m_engineDock = nullptr;

    updateEngineActions();
}

void ChessGameWindow::updateEngineActions()
{
    bool hasEngine = (m_engineDock != nullptr);
    m_startEngineAction->setEnabled(!hasEngine);
    m_stopEngineAction->setEnabled(hasEngine);
    m_positionViewer->setIsEvalActive(hasEngine);
}

void ChessGameWindow::gameReviewSetup()
{
    m_gameReviewViewer = new GameReviewViewer(m_notationViewer->getRootMove(), this);
    m_gameReviewDock = new QDockWidget(tr("Game Review"), this);
    m_gameReviewDock->setContextMenuPolicy(Qt::PreventContextMenu);
    m_gameReviewDock->setWidget(m_gameReviewViewer);
    m_gameReviewDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    m_gameReviewDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_gameReviewDock->setMinimumSize(300, 300);

    addDockWidget(Qt::RightDockWidgetArea, m_gameReviewDock);

    connect(m_gameReviewViewer, &GameReviewViewer::moveSelected, this, &ChessGameWindow::onMoveSelected);
    connect(m_gameReviewViewer, &GameReviewViewer::reviewCompleted, this, [this](){
        m_notationViewer->m_isEdited = true;
        m_notationViewer->refresh();
    });

    m_gameReviewDock->setVisible(false);
}

void ChessGameWindow::openingSetup()
{
    if (m_openingDock){
        return;
    }

    m_openingViewer = new OpeningViewer(this);
    auto currentMove = m_notationViewer->getSelectedMove();
    if (!currentMove->m_zobristHash) currentMove->m_zobristHash = currentMove->m_position->computeZobrist();
    m_openingViewer->updatePosition(currentMove->m_zobristHash, currentMove->m_position, currentMove->moveText);

    m_openingDock = new QDockWidget(tr("Opening Explorer"), this);
    m_openingDock->setContextMenuPolicy(Qt::PreventContextMenu);
    m_openingDock->setWidget(m_openingViewer);
    m_openingDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_openingDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::BottomDockWidgetArea, m_openingDock);
    
    // update openingViewer when notationViewer changes
    connect(m_notationViewer, &NotationViewer::moveSelected, m_openingViewer, &OpeningViewer::onMoveSelected);
    
    connect(m_openingViewer, &OpeningViewer::moveClicked, this, [this](const SimpleMove& moveData) {
        if (!m_notationViewer->getSelectedMove().isNull() && m_notationViewer->getSelectedMove()->m_position) {
            auto [sr, sc, dr, dc, promo] = moveData;
            m_positionViewer->buildUserMove(sr, sc, dr, dc, promo);
        }
    });

    updateOpeningActions();
}

void ChessGameWindow::openingTeardown()
{
    if (!m_openingDock){
        return;
    }

    // disconnect(m_openingViewer, &OpeningViewer::moveSelected, nullptr, nullptr);
    // disconnect(m_openingViewer, &OpeningViewer::moveClicked, nullptr, nullptr);

    removeDockWidget(m_openingDock);
    m_openingDock->setWidget(nullptr);
    m_openingViewer->deleteLater();
    m_openingDock->deleteLater();
    m_openingViewer = nullptr;
    m_openingDock = nullptr;

    updateOpeningActions();
}

void ChessGameWindow::updateOpeningActions()
{
    bool hasOpening = (m_openingDock != nullptr);
    m_openOpeningExplorerAction->setEnabled(!hasOpening);
    m_closeOpeningExplorerAction->setEnabled(hasOpening);
}

// Builds the notation toolbar with notation controls
void ChessGameWindow::notationToolbarSetup()
{
    m_Toolbar = new QToolBar;

    QPushButton* pasteGame = new QPushButton("Paste");
    QPushButton* loadPgn = new QPushButton("Load PGN");
    QPushButton* resetBoard = new QPushButton("Reset");
    QPushButton* savePgn = new QPushButton("Save");

    // horizontal layout for toolbar buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(pasteGame);
    buttonLayout->addWidget(loadPgn);
    buttonLayout->addWidget(resetBoard);
    buttonLayout->addWidget(savePgn);

    // connect buttons to signals and slots
    connect(pasteGame, &QPushButton::clicked, this, &ChessGameWindow::onPasteClicked);
    connect(loadPgn, &QPushButton::clicked, this, &ChessGameWindow::onLoadPgnClicked);
    connect(resetBoard, &QPushButton::clicked, this, &ChessGameWindow::onResetBoardClicked);
    connect(savePgn, &QPushButton::clicked, this, &ChessGameWindow::onSavePgnClicked);

    // attach button toolbar to NotationViewer
    QWidget* dockContent = new QWidget;
    QVBoxLayout* dockLayout = new QVBoxLayout(dockContent);
    dockLayout->addWidget(m_notationViewer);
    dockLayout->addLayout(buttonLayout);
    dockLayout->setContentsMargins(0, 0, 0, 0);

    m_notationDock->setWidget(dockContent);
}

NotationViewer* ChessGameWindow::getNotationViewer()
{
    return m_notationViewer;
}

void ChessGameWindow::onResetBoard()
{
    auto rootMove = m_notationViewer->getRootMove();
    m_notationViewer->m_selectedMove = rootMove;
    emit m_notationViewer->moveSelected(rootMove);
    emit m_positionViewer->boardDataChanged();
    emit m_positionViewer->lastMoveChanged();
    deleteSubtree(rootMove);
}

void ChessGameWindow::onMatchBoardFlip(QChar side)
{
    if ((side == 'w' && m_positionViewer->isBoardFlipped()) || (side == 'b' && !m_positionViewer->isBoardFlipped())){
        m_positionViewer->flipBoard();
    }
}

void ChessGameWindow::onSelectLastMove()
{
    auto curMove =  m_notationViewer->m_selectedMove;
    while (curMove->m_nextMoves.size()){
        curMove = curMove->m_nextMoves.front();
    }
    m_notationViewer->m_selectedMove = curMove;
    emit m_notationViewer->moveSelected(curMove);
    m_notationViewer->refresh();
}

void ChessGameWindow::onRequestTakeback(QChar side)
{
    auto curMove = m_notationViewer->m_selectedMove;
    if (!curMove->m_previousMove.toStrongRef()) return;
    while (curMove->m_nextMoves.size()){
        curMove = curMove->m_nextMoves.front();
    }
    curMove = curMove->m_previousMove.toStrongRef()->m_previousMove.toStrongRef();
    deleteSubtree(curMove);
    emit m_notationViewer->moveSelected(curMove);
    m_notationViewer->refresh();
}

void ChessGameWindow::onEvalScoreChanged(double evalScore)
{
    m_positionViewer->setEvalScore(evalScore);
}

void ChessGameWindow::onMoveHovered(QSharedPointer<NotationMove>& move)
{
    // preview position of move
    m_positionViewer->copyFrom(*move->m_position);
    emit m_positionViewer->boardDataChanged();
    emit m_positionViewer->lastMoveChanged();
    m_positionViewer->setIsPreview(true);
    m_positionViewer->copyFrom(*move->m_position);
    emit m_positionViewer->boardDataChanged();
}

void ChessGameWindow::onNoHover(){
    // no hover, revert to the currently selected move’s position
    m_positionViewer->copyFrom(*m_notationViewer->getSelectedMove()->m_position);
    emit m_positionViewer->boardDataChanged();
    emit m_positionViewer->lastMoveChanged();
    m_positionViewer->setIsPreview(false);
    QSharedPointer<NotationMove> selectedMove = m_notationViewer->getSelectedMove();
    if (!selectedMove.isNull()) m_positionViewer->copyFrom(*selectedMove->m_position);
    emit m_positionViewer->boardDataChanged();
}

// Slot for when a new move is made on the board
void ChessGameWindow::onMoveMade(QSharedPointer<NotationMove>& move)
{
    if (m_isGameplay){
        bool isLastMove = !m_notationViewer->m_selectedMove->m_nextMoves.size();
        if (!isLastMove) {
            onSelectLastMove();
            return;
        } else if (!m_gameplayViewer->isEngineIdle()){
            return; // engine's turn, don't make move
        }
    }
    auto child = getUniqueNextMove(m_notationViewer->m_selectedMove, move);
    // link move if unique
    if (child == move){
        m_notationViewer->m_isEdited = true;
        linkMoves(m_notationViewer->m_selectedMove, move);
    }
    m_notationViewer->m_selectedMove = child;
    emit m_notationViewer->moveSelected(m_notationViewer->m_selectedMove);
    m_notationViewer->refresh();
}

void ChessGameWindow::onPremoveMade(SimpleMove move)
{
    m_gameplayViewer->m_premoves.append(move);
    m_positionViewer->insertPremove(move);
}

// Slot for when a move is selected
void ChessGameWindow::onMoveSelected(QSharedPointer<NotationMove>& move)
{
    if (!move.isNull() && move->m_position) {
        m_notationViewer->m_selectedMove = move;
        m_notationViewer->refresh();
        m_positionViewer->copyFrom(*move->m_position);
        m_positionViewer->setIsPreview(false);
        emit m_positionViewer->boardDataChanged();
        emit m_positionViewer->lastMoveChanged();
        if (m_gameplayViewer && m_gameplayViewer->m_premoves.size() && move->m_nextMoves.size()) {
            m_gameplayViewer->m_premoves.clear();
            m_positionViewer->updatePremoves(m_gameplayViewer->m_premoves);
        }
    }
}

void ChessGameWindow::onPrevMoveShortcut()
{
    m_notationViewer->selectPreviousMove();
}

void ChessGameWindow::onNextMoveShortcut()
{
    m_notationViewer->selectNextMove();
}

void ChessGameWindow::onFlipBoardShortcut()
{
    m_positionViewer->flipBoard();
}

void ChessGameWindow::onDeleteAfterShortcut()
{
    m_notationViewer->m_selectedMove = deleteMove(m_notationViewer->m_selectedMove);
    emit m_notationViewer->moveSelected(m_notationViewer->m_selectedMove);
    m_notationViewer->refresh();
}

void ChessGameWindow::onDeleteVariationShortcut()
{
    m_notationViewer->m_selectedMove = deleteVariation(m_notationViewer->m_selectedMove);
    emit m_notationViewer->moveSelected(m_notationViewer->m_selectedMove);
    m_notationViewer->refresh();
}

void ChessGameWindow::onPromoteVariationShortcut()
{
    promoteVariation(m_notationViewer->m_selectedMove);
    m_notationViewer->refresh();
}

void ChessGameWindow::onPasteClicked()
{
    qDebug() << "Paste clicked!";
}

void ChessGameWindow::onLoadPgnClicked()
{
    qDebug() << "Load PGN clicked!";

}

void ChessGameWindow::onResetBoardClicked()
{
}

void ChessGameWindow::onSavePgnClicked()
{
    PGNSaveDialog dialog(nullptr);
    dialog.setHeaders(m_notationViewer->m_game);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    dialog.applyTo(m_notationViewer->m_game);
    QString result;
    QTextStream out(&result);
    int plyCount = 0;
    writeMoves(m_notationViewer->getRootMove(), out, plyCount);
    m_notationViewer->m_game.bodyText = result.trimmed();
    QTimer::singleShot(200, this, [this]{ saveGame(); });
}

void ChessGameWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    setMinimumSize(0,0);
}
