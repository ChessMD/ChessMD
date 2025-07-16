/*
March 18, 2025: File Creation
*/

#include "chessgamewindow.h"
#include "notation.h"
#include "notationviewer.h"
#include "streamparser.h"
#include "chessposition.h"
#include "enginewidget.h"
#include "pgnsavedialog.h"

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

    // link to the rootmove containing the entire game tree
    m_notationViewer = new NotationViewer(game, this);
    m_notationViewer->setRootMove(m_notationViewer->getRootMove());

    // connect moveMade signal when user manually makes a move
    connect(m_positionViewer, &ChessPosition::moveMade, this, &ChessGameWindow::onMoveMade);

    // create and connect keyboard shortcuts
    QShortcut* saveGame = new QShortcut(QKeySequence("Ctrl+S"), this);
    QShortcut* prevMove = new QShortcut(QKeySequence(Qt::Key_Left), this);
    QShortcut* nextMove = new QShortcut(QKeySequence(Qt::Key_Right), this);
    QShortcut* delAfter = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    QShortcut* delVariation = new QShortcut(QKeySequence("Ctrl+D"), this);
    QShortcut* promoteVariation = new QShortcut(QKeySequence("Ctrl+Up"), this);

    connect(saveGame, &QShortcut::activated, this, &ChessGameWindow::onSavePgnClicked);
    connect(prevMove, &QShortcut::activated, this, &ChessGameWindow::onPrevMoveShortcut);
    connect(nextMove, &QShortcut::activated, this, &ChessGameWindow::onNextMoveShortcut);
    connect(delAfter, &QShortcut::activated, this, &ChessGameWindow::onDeleteAfterShortcut);
    connect(delVariation, &QShortcut::activated, this, &ChessGameWindow::onDeleteVariationShortcut);
    connect(promoteVariation, &QShortcut::activated, this, &ChessGameWindow::onPromoteVariationShortcut);
}

void ChessGameWindow::closeEvent(QCloseEvent *event)
{
    if (m_isPreview || !m_notationViewer->m_isEdited) {
        QMainWindow::closeEvent(event);
        return;
    }

    QMessageBox message = QMessageBox(this);
    message.setIcon(QMessageBox::Question);
    message.setWindowTitle(tr("Save changes?"));
    message.setText(tr("Do you want to save your changes to this game?"));
    auto saveBtn   = message.addButton(tr("Yes"), QMessageBox::AcceptRole);
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
        saveGame();
    }

    event->accept();
}

// Configures ChessGameWindow for complete analysis
void ChessGameWindow::mainSetup(){
    m_isPreview = false;
    notationSetup();
    // notationToolbarSetup();
    toolbarSetup();
    // engineSetup();
    updateEngineActions();
    resizeDocks({m_notationDock}, {int(width() )}, Qt::Horizontal);
}

// Configures ChessGameWindow for previewing
void ChessGameWindow::previewSetup()
{
    m_isPreview = true;
    notationSetup();
    addDockWidget(Qt::BottomDockWidgetArea, m_notationDock);
    m_notationDock->show();
}

void ChessGameWindow::saveGame(){
    m_notationViewer->m_isEdited = false;
    emit PGNGameUpdated(m_notationViewer->m_game);
}

// Builds a dockable notation panelw
void ChessGameWindow::notationSetup()
{
    m_notationDock = new QDockWidget(tr("Notation"), this);
    m_notationDock->setWidget(m_notationViewer);
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
    m_Toolbar = new QToolBar;
    m_Toolbar->setIconSize(QSize(32,32));
    m_Toolbar->setFloatable(false);
    m_Toolbar->setMovable(false);
    m_Toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QAction* back = m_Toolbar->addAction("Back (Left Arrow)");
    back->setIcon(QIcon(":/resource/img/arrow-left.png"));
    connect(back, &QAction::triggered, this, &ChessGameWindow::onPrevMoveShortcut);

    QAction* forward = m_Toolbar->addAction("Forward (Right Arrow)");
    forward->setIcon(QIcon(":/resource/img/arrow-right.png"));
    connect(forward, &QAction::triggered, this, &ChessGameWindow::onNextMoveShortcut);

    QAction* save = m_Toolbar->addAction("Save (Ctrl+S)");
    save->setIcon(QIcon(":/resource/img/savegame.png"));
    connect(save, &QAction::triggered, this, &ChessGameWindow::onSavePgnClicked);

    m_startEngineAction = m_Toolbar->addAction("Start Engine");
    m_startEngineAction->setIcon(QIcon(":/resource/img/engine-start.png"));
    connect(m_startEngineAction, &QAction::triggered, this, &ChessGameWindow::engineSetup);

    m_stopEngineAction = m_Toolbar->addAction("Stop engine");
    m_stopEngineAction->setIcon(QIcon(":/resource/img/engine-stop.png"));
    connect(m_stopEngineAction, &QAction::triggered, this, &ChessGameWindow::engineTeardown);

    addToolBar(m_Toolbar);
}

// Builds the engine dockable panel
void ChessGameWindow::engineSetup()
{
    if (m_engineDock){
        return;
    }

    // create the engine dockable panel
    m_engineViewer = new EngineWidget(this);
    m_engineDock = new QDockWidget(tr("Engine"), this);
    m_engineDock->setWidget(m_engineViewer);
    m_engineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
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
    emit m_notationViewer->moveSelected(m_notationViewer->m_selectedMove);

    connect(m_engineViewer, &EngineWidget::engineMoveClicked, m_notationViewer, &NotationViewer::onEngineMoveClicked);
    connect(m_engineViewer, &EngineWidget::moveHovered, this, &ChessGameWindow::onMoveHovered);
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
}

void ChessGameWindow::openingSetup()
{
    m_openingViewer = new OpeningViewer(this);
    m_openingViewer->updatePosition(QVector<QString>());

    m_openingDock = new QDockWidget(tr("Opening Explorer"), this);
    m_openingDock->setWidget(m_openingViewer);
    m_openingDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_openingDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    
    addDockWidget(Qt::BottomDockWidgetArea, m_openingDock);
    
    connect(m_notationViewer, &NotationViewer::moveSelected, 
        [this](QSharedPointer<NotationMove> move) {
            //update openingviewer when notation viewer changed
            QVector<QString> moveSequence;
            QSharedPointer<NotationMove> currentMove = move;
            int moveCount = 0;
            while (currentMove && !currentMove->lanText.isEmpty()) {
                moveSequence.prepend(currentMove->lanText);
                currentMove = currentMove->m_previousMove;
                moveCount++;
            }
            m_openingViewer->updatePosition(moveSequence);
        });
    
    connect(m_openingViewer, &OpeningViewer::moveClicked,
        [this](const QString& move) {
            if (!m_notationViewer->getSelectedMove().isNull() && m_notationViewer->getSelectedMove()->m_position) {
                
                //todo, make it play the move and create a variation maybe idk
            }
        });


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

void ChessGameWindow::onEvalScoreChanged(double evalScore){
    m_positionViewer->setEvalScore(evalScore);
}

void ChessGameWindow::onMoveHovered(QSharedPointer<NotationMove> move)
{
    if (!move.isNull() && move->m_position) {
        // preview that position
        m_positionViewer->copyFrom(*move->m_position);
        emit m_positionViewer->boardDataChanged();
        emit m_positionViewer->lastMoveChanged();
    } else {
        // no hover, revert to the currently selected move’s position
        if (!m_notationViewer->getSelectedMove().isNull()) {
            m_positionViewer->copyFrom(*m_notationViewer->getSelectedMove()->m_position);
            emit m_positionViewer->boardDataChanged();
            emit m_positionViewer->lastMoveChanged();
        }
    }

    bool preview = !move.isNull();
    m_positionViewer->setIsPreview(preview);

    if (preview) {
        m_positionViewer->copyFrom(*move->m_position);
    } else {
        auto sel = m_notationViewer->getSelectedMove();
        if (!sel.isNull())
            m_positionViewer->copyFrom(*sel->m_position);
    }
    emit m_positionViewer->boardDataChanged();
}

QString buildMoveText(const QSharedPointer<NotationMove>& move)
{
    QString fullMoveText;
    if (!move->commentBefore.isEmpty()) {
        fullMoveText += QString("{%1} ").arg(move->commentBefore.trimmed());
    }

    fullMoveText += move->moveText;
    fullMoveText += move->annotation1;
    fullMoveText += move->annotation2;
    if (!move->commentAfter.isEmpty()) {
        fullMoveText += QString(" {%1}").arg(move->commentAfter.trimmed());
    }
    fullMoveText += " ";
    return fullMoveText;
}

void writeMoves(const QSharedPointer<NotationMove>& move, QTextStream& out, int plyCount)
{
    auto children = move->m_nextMoves;
    if (children.isEmpty()) return;

    auto principal = children.first();
    out << (plyCount % 2 == 0 ? QString::number(plyCount/2 + 1) + ". " : "") << buildMoveText(principal);

    for (int i = 1; i < children.size(); ++i) {
        out << "(";
        out << plyCount/2 + 1 << (plyCount % 2 == 0 ? ". " : "... ") << buildMoveText(children[i]);
        writeMoves(children[i], out, plyCount+1);
        out << ") ";
    }

    ++plyCount;
    writeMoves(principal, out, plyCount);
}

// Slot for when a new move is made on the board
void ChessGameWindow::onMoveMade(QSharedPointer<NotationMove> move)
{
    m_notationViewer->m_isEdited = true;
    linkMoves(m_notationViewer->m_selectedMove, move);
    m_notationViewer->m_selectedMove = move;
    emit m_notationViewer->moveSelected(m_notationViewer->m_selectedMove);
    m_notationViewer->refresh();
}

// Slot for when a move is selected
void ChessGameWindow::onMoveSelected(QSharedPointer<NotationMove> move)
{
    if (!move.isNull() && move->m_position) {

        m_positionViewer->copyFrom(*move->m_position);
        m_positionViewer->setIsPreview(false);
        emit m_positionViewer->boardDataChanged();
        emit m_positionViewer->lastMoveChanged();

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

void ChessGameWindow::onDeleteAfterShortcut()
{
    deleteMovesAfter(m_notationViewer->m_selectedMove);
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
    PGNSaveDialog dialog(this);
    dialog.setHeaders(m_notationViewer->m_game);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    dialog.applyTo(m_notationViewer->m_game);
    QString result; QTextStream out(&result); int plyCount = 0;
    writeMoves(m_notationViewer->getRootMove(), out, plyCount);
    m_notationViewer->m_game.bodyText = result.trimmed();
    saveGame();
}

void ChessGameWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    setMinimumSize(0,0);
}
