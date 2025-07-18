/*
March 18, 2025: File Creation
*/

#ifndef CHESSGAMEWINDOW_H
#define CHESSGAMEWINDOW_H


#include "notationviewer.h"
#include "engineviewer.h"
#include "chessposition.h"
#include "pgngamedata.h"
#include "openingviewer.h"
#include "gamereviewviewer.h"

#include <QQuickWidget>
#include <QMainWindow>
#include <QToolBar>
#include <QDockWidget>
#include <QCloseEvent>
#include <QMessageBox>

// Main window that displays a chessboard, an engine, a notation viewer, and an opening viewer
class ChessGameWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ChessGameWindow (QWidget *parent, PGNGame game);

    void notationSetup();
    void notationToolbarSetup();
    void toolbarSetup();

    void mainSetup();
    void previewSetup();
    void saveGame();

    NotationViewer* getNotationViewer();

signals:
    void PGNGameUpdated(PGNGame &game);

protected:
    void showEvent(QShowEvent *ev) override;
    void closeEvent(QCloseEvent *event) override;

public slots:
    void engineSetup();
    void engineTeardown();
    void openingSetup();
    void openingTeardown();
    void gameReviewSetup();

    void onMoveHovered(QSharedPointer<NotationMove> move);

private:
    void updateEngineActions();
    void updateOpeningActions();

    NotationViewer* m_notationViewer;
    OpeningViewer* m_openingViewer;
    EngineWidget* m_engineViewer;
    ChessPosition* m_positionViewer;
    GameReviewViewer *m_gameReviewViewer;
    QToolBar* m_Toolbar;

    QDockWidget* m_notationDock;
    QDockWidget* m_engineDock;
    QDockWidget* m_openingDock;
    QDockWidget* m_gameReviewDock;

    QAction* m_startEngineAction;
    QAction* m_stopEngineAction;

    QAction* m_openOpeningExplorerAction;
    QAction* m_closeOpeningExplorerAction;

    bool m_isPreview;

private slots:
    void onMoveMade(QSharedPointer<NotationMove> move);
    void onMoveSelected(QSharedPointer<NotationMove> move);
    void onEvalScoreChanged(double evalScore);

    void onPrevMoveShortcut();
    void onNextMoveShortcut();
    void onDeleteAfterShortcut();
    void onDeleteVariationShortcut();
    void onPromoteVariationShortcut();

    void onPasteClicked();
    void onLoadPgnClicked();
    void onResetBoardClicked();
    void onSavePgnClicked();

signals:
};

#endif // CHESSGAMEWINDOW_H
