#include "chessgamewindow.h"
#include "notation.h"
#include "NotationViewer.h"
#include "streamparser.h"
#include "chessposition.h"

#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QTextEdit>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QPushButton>

void setup(const QSharedPointer<NotationMove>& root){
    ChessPosition *pos = new ChessPosition;
    QSharedPointer<NotationMove> move1(new NotationMove("e4", *pos));
    move1->FEN = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";
    QSharedPointer<NotationMove> move2(new NotationMove("e5", *pos));
    move2->FEN = "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2";
    QSharedPointer<NotationMove> move3(new NotationMove("e6", *pos));
    move3->FEN = "rnbqkbnr/pppp1ppp/4p3/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2";
    QSharedPointer<NotationMove> move4(new NotationMove("c5", *pos));
    move4->FEN = "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2";
    QSharedPointer<NotationMove> move5(new NotationMove("Nf3", *pos));
    move5->FEN = "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 0 2";
    QSharedPointer<NotationMove> move6(new NotationMove("Nf3", *pos));
    move6->FEN = "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 0 2";

    linkMoves(root, move1);
    linkMoves(move1, move2);
    linkMoves(move1, move3);
    linkMoves(move1, move4);
    linkMoves(move2, move5);
    linkMoves(move3, move6);
}

ChessGameWindow::ChessGameWindow (QWidget *parent)
    : QMainWindow{parent}
{
    QQuickWidget* boardView = new QQuickWidget;
    ChessPosition* chessPosition = new ChessPosition(this);

    boardView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    boardView->rootContext()->setContextProperty("chessPosition", chessPosition);
    boardView->setSource(QUrl(QStringLiteral("qrc:/chessboard.qml")));
    boardView->setMinimumSize(200, 200);
    setCentralWidget(boardView);

    QSharedPointer<NotationMove> rootMove(new NotationMove("", *new ChessPosition));
    if (rootMove->m_position != nullptr)
        rootMove->m_position->setBoardData( convertFenToBoardData(rootMove->FEN));
    // setup(rootMove);

    // Create our custom NotationViewer widget and set the notation data.
    m_notationViewer = new NotationViewer(parent);
    m_notationViewer->setRootMove(rootMove);

    // Create buttons for the toolbar
    QPushButton* pasteGame = new QPushButton("Paste");
    QPushButton* loadPgn = new QPushButton("Load PGN");
    QPushButton* resetBoard = new QPushButton("Reset");
    QPushButton* exportPgn = new QPushButton("Export");

    // Horizontal layout for toolbar buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(pasteGame);
    buttonLayout->addWidget(loadPgn);
    buttonLayout->addWidget(resetBoard);
    buttonLayout->addWidget(exportPgn);

    connect(chessPosition, &ChessPosition::moveMade, this, &ChessGameWindow::onMoveMade);
    connect(pasteGame, &QPushButton::clicked, this, &ChessGameWindow::onPasteClicked);
    connect(loadPgn, &QPushButton::clicked, this, &ChessGameWindow::onLoadPgnClicked);
    connect(resetBoard, &QPushButton::clicked, this, &ChessGameWindow::onResetBoardClicked);
    connect(exportPgn, &QPushButton::clicked, this, &ChessGameWindow::onExportPgnClicked);

    // Vertical layout: NotationViewer + button toolbar
    QWidget* dockContent = new QWidget;
    QVBoxLayout* dockLayout = new QVBoxLayout(dockContent);
    dockLayout->addWidget(m_notationViewer);
    dockLayout->addLayout(buttonLayout);
    dockLayout->setContentsMargins(0, 0, 0, 0);

    // Create a dockable notation panel.
    QDockWidget* notationDock = new QDockWidget(tr("Notation"), this);
    notationDock->setWidget(dockContent);
    notationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    notationDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    notationDock->setMinimumSize(100, 0);

    // Add the dock widget to the right side.
    addDockWidget(Qt::RightDockWidgetArea, notationDock);

    // Install an event filter on the main window so that key presses are intercepted globally.
    installEventFilter(this);

    QObject::connect(m_notationViewer, &NotationViewer::moveSelected, [=](const QSharedPointer<NotationMove>& move) {
        if (!move.isNull() && move->m_position) {
            qDebug() << move->m_position->plyCount;
            chessPosition->setBoardData(move->m_position->boardData());
            chessPosition->plyCount = move->m_position->plyCount;
            // QVector<QVector<QString>> boardData = convertFenToBoardData(move->FEN);
        }
    });
}

void ChessGameWindow::onPasteClicked() {
    qDebug() << "Paste clicked!";
    // Example: maybe show a QInputDialog to paste PGN
}

void ChessGameWindow::onLoadPgnClicked() {
    qDebug() << "Load PGN clicked!";

}

void ChessGameWindow::onResetBoardClicked() {

}

void ChessGameWindow::onExportPgnClicked() {
    qDebug() << "Export clicked!";
    // You could traverse notation tree and export here
}

bool ChessGameWindow::eventFilter(QObject* obj, QEvent* event)
{
    // Intercept key press events globally.
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        // For left/right arrow keys, forward to the notation viewer.
        if (keyEvent->key() == Qt::Key_Left) {
            // qDebug() << "Left arrow key pressed!";
            m_notationViewer->selectPreviousMove();
            // Do not block the event; let it propagate.
            return false;
        } else if (keyEvent->key() == Qt::Key_Right) {
            // qDebug() << "Right arrow key pressed!";
            m_notationViewer->selectNextMove();
            return false;
        } else if (keyEvent->key() == Qt::Key_BracketRight){ // Key: ']'
            deleteMovesAfter(m_notationViewer->m_selectedMove);
            m_notationViewer->refresh();
        } else if (keyEvent->key() == Qt::Key_BracketLeft){ // Key: '['
            m_notationViewer->m_selectedMove = deleteVariation(m_notationViewer->m_selectedMove);
            emit m_notationViewer->moveSelected(m_notationViewer->m_selectedMove);
            m_notationViewer->refresh();
        } else if (keyEvent->key() == Qt::Key_W){ // Key: '['
            // m_notationViewer->m_selectedMove = promoteVariation(m_notationViewer->m_selectedMove);
            // emit m_notationViewer->moveSelected(m_notationViewer->m_selectedMove);
            // m_notationViewer->refresh();
        }
    }
    // For all other events, call the base class event filter.
    return QMainWindow::eventFilter(obj, event);
}

void ChessGameWindow::onMoveMade(QSharedPointer<NotationMove> move) {
    // e.g. push it into your notation pane
    qDebug() << "Received";
    linkMoves(m_notationViewer->m_selectedMove, move);
    m_notationViewer->m_selectedMove = move;
    emit m_notationViewer->moveSelected(m_notationViewer->m_selectedMove);
    m_notationViewer->refresh();
}
