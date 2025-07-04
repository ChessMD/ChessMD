/*
March 18, 2025: File Creation
*/

#ifndef CHESSGAMEWINDOW_H
#define CHESSGAMEWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <qdockwidget.h>
#include "notationviewer.h"
#include "enginewidget.h"
#include "chessposition.h"

// Main window that displays a chessboard, an engine, a notation viewer, and an opening viewer
class ChessGameWindow  : public QMainWindow
{
    Q_OBJECT
public:
    explicit ChessGameWindow (QWidget *parent, QSharedPointer<NotationMove> rootMove);
    void notationSetup();
    void engineSetup();
    void notationToolbarSetup();
    void toolbarSetup();

    void mainSetup();
    void previewSetup();

protected:
    void showEvent(QShowEvent *ev) override;

public slots:
    void onMoveHovered(QSharedPointer<NotationMove> move);

private:
    NotationViewer* m_notationViewer;
    EngineWidget* m_engineViewer;
    ChessPosition* m_positionViewer;
    QToolBar* m_Toolbar;

    QDockWidget* m_notationDock;
    QDockWidget* m_engineDock;

private slots:
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
