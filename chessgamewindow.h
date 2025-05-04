/*
March 18, 2025: File Creation
*/

#ifndef CHESSGAMEWINDOW_H
#define CHESSGAMEWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include "notationviewer.h"
#include "enginewidget.h"
#include "chessposition.h"

// Main window that displays a chessboard, an engine, a notation viewer, and an opening viewer
class ChessGameWindow  : public QMainWindow
{
    Q_OBJECT
public:
    explicit ChessGameWindow (QWidget *parent, QSharedPointer<NotationMove> rootMove);
    void setRoot(QSharedPointer<NotationMove> rootMove);

protected:
    void showEvent(QShowEvent *ev) override;

private:
    NotationViewer* m_notationViewer; // Notation tree viewer
    EngineWidget* m_engineViewer; // UCI Engine viewer
    ChessPosition* m_positionViewer; // Chessboard viewer
    QToolBar* m_Toolbar;//Game window toolbar

private slots:
    // Appends a new user move to the notation tree
    void onMoveMade(QSharedPointer<NotationMove> move);
    void onMoveSelected(QSharedPointer<NotationMove> move);

    void onPrevMoveShortcut();
    void onNextMoveShortcut();
    void onDeleteAfterShortcut();
    void onDeleteVariationShortcut();
    void onPromoteVariationShortcut();

    void onPasteClicked();
    void onLoadPgnClicked();
    void onResetBoardClicked();
    void onExportPgnClicked();


signals:
};

#endif // CHESSGAMEWINDOW_H
