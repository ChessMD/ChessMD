#include "chessgamewindow.h"
#include "notation.h"
#include "NotationViewer.h"
#include "streamparser.h"
#include "chessposition.h"
#include "enginewidget.h"

#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QTextEdit>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QPushButton>

ChessGameWindow::ChessGameWindow (QWidget *parent, QSharedPointer<NotationMove> rootMove)
    : QMainWindow{parent}
{
    QQuickWidget* boardView = new QQuickWidget;
    ChessPosition* chessPosition = new ChessPosition(this);

    boardView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    boardView->rootContext()->setContextProperty("chessPosition", chessPosition);
    boardView->setSource(QUrl(QStringLiteral("qrc:/chessboard.qml")));
    boardView->setMinimumSize(200, 200);
    setCentralWidget(boardView);

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

    EngineWidget *engW = new EngineWidget(this);
    QDockWidget  *engDock = new QDockWidget(tr("Engine"), this);
    engDock->setWidget(engW);
    addDockWidget(Qt::RightDockWidgetArea, engDock);
    engW->setPosition(m_notationViewer->m_selectedMove->m_position->positionToFEN());

    connect(m_notationViewer, &NotationViewer::moveSelected, engW, &EngineWidget::onMoveSelected);

    QObject::connect(m_notationViewer, &NotationViewer::moveSelected, [=](const QSharedPointer<NotationMove>& move) {
        if (!move.isNull() && move->m_position) {
            chessPosition->copyFrom(*move->m_position);
            // QVector<QVector<QString>> boardData = convertFenToBoardData(move->FEN);
        }
    });
}

void ChessGameWindow::onPasteClicked() {
    qDebug() << "Paste clicked!";
}

void ChessGameWindow::onLoadPgnClicked() {
    qDebug() << "Load PGN clicked!";

}

void ChessGameWindow::onResetBoardClicked() {

}

void ChessGameWindow::onExportPgnClicked() {
    qDebug() << "Export clicked!";

    qDebug() << m_notationViewer->m_selectedMove->m_position->positionToFEN();
}

bool ChessGameWindow::eventFilter(QObject* obj, QEvent* event)
{
    // Intercept key press events globally.
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        // For left/right arrow keys, forward to the notation viewer.
        if (keyEvent->key() == Qt::Key_Left) {
            m_notationViewer->selectPreviousMove();
            // Do not block the event; let it propagate.
            return false;
        } else if (keyEvent->key() == Qt::Key_Right) {
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
    linkMoves(m_notationViewer->m_selectedMove, move);
    m_notationViewer->m_selectedMove = move;
    emit m_notationViewer->moveSelected(m_notationViewer->m_selectedMove);
    m_notationViewer->refresh();
}
