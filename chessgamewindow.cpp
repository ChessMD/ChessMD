/*
March 18, 2025: File Creation
*/

#include "chessgamewindow.h"
#include "notation.h"
#include "notationviewer.h"
#include "streamparser.h"
#include "chessposition.h"
#include "enginewidget.h"

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

// Constructs a ChessGameWindow inside a parent widget
ChessGameWindow::ChessGameWindow(QWidget *parent, QSharedPointer<NotationMove> rootMove)
    : QMainWindow{parent}
{
    setMinimumSize(1024,768);
    setGeometry(100, 100, 0, 0);
    setWindowFlags(Qt::Widget);

    // set the chessboard as central widget of window, and link to QML
    QQuickWidget* boardView = new QQuickWidget;
    m_positionViewer = new ChessPosition(this);
    boardView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    boardView->rootContext()->setContextProperty("chessPosition", m_positionViewer);
    boardView->setSource(QUrl(QStringLiteral("qrc:/chessboard.qml")));
    boardView->setMinimumSize(200, 200);
    setCentralWidget(boardView);

    // link to the rootmove containing the entire game tree
    m_notationViewer = new NotationViewer(this);
    m_notationViewer->setRootMove(rootMove);

    // connect moveMade signal when user manually makes a move
    connect(m_positionViewer, &ChessPosition::moveMade, this, &ChessGameWindow::onMoveMade);

    // create and connect keyboard shortcuts
    QShortcut* prevMove = new QShortcut(QKeySequence(Qt::Key_Left), this);
    QShortcut* nextMove = new QShortcut(QKeySequence(Qt::Key_Right), this);
    QShortcut* delAfter = new QShortcut(QKeySequence(Qt::Key_BracketRight), this);
    QShortcut* delVariation = new QShortcut(QKeySequence("Ctrl+D"), this);
    QShortcut* promoteVariation = new QShortcut(QKeySequence(Qt::Key_W), this);

    connect(prevMove, &QShortcut::activated, this, &ChessGameWindow::onPrevMoveShortcut);
    connect(nextMove, &QShortcut::activated, this, &ChessGameWindow::onNextMoveShortcut);
    connect(delAfter, &QShortcut::activated, this, &ChessGameWindow::onDeleteAfterShortcut);
    connect(delVariation, &QShortcut::activated, this, &ChessGameWindow::onDeleteVariationShortcut);
    connect(promoteVariation, &QShortcut::activated, this, &ChessGameWindow::onPromoteVariationShortcut);
}

// Builds a dockable notation panel
void ChessGameWindow::notationSetup()
{
    m_notationDock = new QDockWidget(tr("Notation"), this);
    m_notationDock->setWidget(m_notationViewer);
    m_notationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    m_notationDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_notationDock->setMinimumSize(100, 0);

    // connect moveSelected signal when user clicks on a move in the notation
    connect(m_notationViewer, &NotationViewer::moveSelected, this, &ChessGameWindow::onMoveSelected);
    connect(m_notationViewer, &NotationViewer::moveHovered, this, &ChessGameWindow::onMoveHovered);

    addDockWidget(Qt::RightDockWidgetArea, m_notationDock);
}

// Builds the toolbar with additional game controls
void ChessGameWindow::toolbarSetup()
{
    m_Toolbar = new QToolBar;
    QAction* pAction;

    pAction = m_Toolbar->addAction("Back");
    pAction->setIcon(QIcon(":/resource/img/arrow-left.png"));
    connect(pAction, &QAction::triggered, this, &ChessGameWindow::onPrevMoveShortcut);

    pAction = m_Toolbar->addAction("Forward");
    pAction->setIcon(QIcon(":/resource/img/arrow-right.png"));
    connect(pAction, &QAction::triggered, this, &ChessGameWindow::onNextMoveShortcut);

    addToolBar(m_Toolbar);
}

// Builds the engine dockable panel
void ChessGameWindow::engineSetup()
{
    // create the engine dockable panel
    m_engineViewer = new EngineWidget(this);
    m_engineDock = new QDockWidget(tr("Engine"), this);
    m_engineDock->setWidget(m_engineViewer);
    addDockWidget(Qt::RightDockWidgetArea, m_engineDock);


    // update engine and board display when position changes
    connect(m_notationViewer, &NotationViewer::moveSelected, m_engineViewer, &EngineWidget::onMoveSelected);

    connect(m_engineViewer, &EngineWidget::engineMoveClicked, m_notationViewer, &NotationViewer::onEngineMoveClicked);
    connect(m_engineViewer, &EngineWidget::moveHovered, this, &ChessGameWindow::onMoveHovered);
}

// Builds the notation toolbar with notation controls
void ChessGameWindow::notationToolbarSetup()
{
    m_Toolbar = new QToolBar;

    QPushButton* pasteGame = new QPushButton("Paste");
    QPushButton* loadPgn = new QPushButton("Load PGN");
    QPushButton* resetBoard = new QPushButton("Reset");
    QPushButton* exportPgn = new QPushButton("Export");

    // horizontal layout for toolbar buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(pasteGame);
    buttonLayout->addWidget(loadPgn);
    buttonLayout->addWidget(resetBoard);
    buttonLayout->addWidget(exportPgn);

    // connect buttons to signals and slots
    connect(pasteGame, &QPushButton::clicked, this, &ChessGameWindow::onPasteClicked);
    connect(loadPgn, &QPushButton::clicked, this, &ChessGameWindow::onLoadPgnClicked);
    connect(resetBoard, &QPushButton::clicked, this, &ChessGameWindow::onResetBoardClicked);
    connect(exportPgn, &QPushButton::clicked, this, &ChessGameWindow::onExportPgnClicked);

    // attach button toolbar to NotationViewer
    QWidget* dockContent = new QWidget;
    QVBoxLayout* dockLayout = new QVBoxLayout(dockContent);
    dockLayout->addWidget(m_notationViewer);
    dockLayout->addLayout(buttonLayout);
    dockLayout->setContentsMargins(0, 0, 0, 0);

    m_notationDock->setWidget(dockContent);
}

void ChessGameWindow::onMoveHovered(QSharedPointer<NotationMove> move)
{
    if (!move.isNull() && move->m_position) {
        // preview that position
        m_positionViewer->copyFrom(*move->m_position);
        emit m_positionViewer->boardDataChanged();
    } else {
        // no hover → revert to the currently selected move’s position
        if (!m_notationViewer->getSelectedMove().isNull()) {
            m_positionViewer->copyFrom(*m_notationViewer->getSelectedMove()->m_position);
            emit m_positionViewer->boardDataChanged();
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

// Configures ChessGameWindow for complete analysis
void ChessGameWindow::mainSetup(){
    notationSetup();
    notationToolbarSetup();
    toolbarSetup();
    engineSetup();
}

// Configures ChessGameWindow for previewing
void ChessGameWindow::previewSetup()
{
    notationSetup();
    toolbarSetup();
    addDockWidget(Qt::BottomDockWidgetArea, m_notationDock);
    m_notationDock->show();
}

// Slot for when a new move is made on the board
void ChessGameWindow::onMoveMade(QSharedPointer<NotationMove> move)
{
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
        emit m_positionViewer->boardDataChanged();
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
    // m_notationViewer->m_selectedMove = promoteVariation(m_notationViewer->m_selectedMove);
    // emit m_notationViewer->moveSelected(m_notationViewer->m_selectedMove);
    // m_notationViewer->refresh();
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

void writeMovesRecursive(const QSharedPointer<NotationMove>& move, QTextStream& out, int plyCount)
{
    auto children = move->m_nextMoves;
    if (children.isEmpty()) return;

    auto principal = children.first();
    out << (plyCount % 2 == 0 ? QString::number(plyCount/2 + 1) + ". " : "") << buildMoveText(principal);

    for (int i = 1; i < children.size(); ++i) {
        out << "(";
        out << plyCount/2 + 1 << (plyCount % 2 == 0 ? ". " : "... ") << buildMoveText(children[i]);
        writeMovesRecursive(children[i], out, plyCount+1);
        out << ") ";
    }

    ++plyCount;
    writeMovesRecursive(principal, out, plyCount);
}

void ChessGameWindow::onExportPgnClicked()
{
    QString result;
    QTextStream out(&result);
    int plyCount = 0;

    // write moves recursively
    writeMovesRecursive(m_notationViewer->getRootMove(), out, plyCount);
    qDebug() << result.trimmed();
}

void ChessGameWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    setMinimumSize(0,0);
}
