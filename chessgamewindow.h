/*
March 18, 2025: File Creation
*/

#ifndef CHESSGAMEWINDOW_H
#define CHESSGAMEWINDOW_H


#include "notationviewer.h"
#include "engineviewer.h"
#include "chessposition.h"
#include "pgngame.h"
#include "openingviewer.h"
#include "gamereviewviewer.h"
#include "gameplayviewer.h"

#include <QQuickWidget>
#include <QMainWindow>
#include <QToolBar>
#include <QDockWidget>
#include <QCloseEvent>
#include <QMessageBox>
#include <QStackedWidget>
#include <QToolButton>
#include <QFormLayout>
#include <QGroupBox>

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
    void gameplaySetup();

    void connectEditingShortcuts();
    void saveGame();
    void startGameReview();

    NotationViewer* getNotationViewer();

signals:
    void PGNGameUpdated(PGNGame &game);
    void openAnalysisBoard(PGNGame &game);

protected:
    void showEvent(QShowEvent *ev) override;
    void closeEvent(QCloseEvent *event) override;

public slots:
    void engineSetup();
    void engineTeardown();
    void openingSetup();
    void openingTeardown();
    void gameReviewSetup();

    void onMoveHovered(QSharedPointer<NotationMove>& move);
    void onNoHover();

private slots:
    void onResetBoard();
    void onMatchBoardFlip(QChar side);
    void onSelectLastMove();
    void onRequestTakeback(QChar side);

    void onMoveMade(QSharedPointer<NotationMove>& move);
    void onMoveSelected(QSharedPointer<NotationMove>& move);
    void onEvalScoreChanged(double evalScore);

    void onPrevMoveShortcut();
    void onNextMoveShortcut();
    void onFlipBoardShortcut();
    void onDeleteAfterShortcut();
    void onDeleteVariationShortcut();
    void onPromoteVariationShortcut();

    void onPasteClicked();
    void onLoadPgnClicked();
    void onResetBoardClicked();
    void onSavePgnClicked();

private:
    void updateEngineActions();
    void updateOpeningActions();

    void toggleEditMode();
    void refreshHeader();
    void adjustFieldWidth(QLineEdit* e, int buffer = 5);

    NotationViewer* m_notationViewer;
    OpeningViewer* m_openingViewer;
    EngineWidget* m_engineViewer;
    ChessPosition* m_positionViewer;
    GameReviewViewer *m_gameReviewViewer;
    GameplayViewer *m_gameplayViewer;
    QToolBar* m_Toolbar;

    QDockWidget* m_notationDock;
    QDockWidget* m_engineDock;
    QDockWidget* m_openingDock;
    QDockWidget* m_gameReviewDock;
    QDockWidget *m_gameplayDock;

    QAction* m_gameReviewAction;
    QAction* m_saveGameAction;
    QAction* m_startEngineAction;
    QAction* m_stopEngineAction;
    QAction* m_openOpeningExplorerAction;
    QAction* m_closeOpeningExplorerAction;

    bool m_isPreview = false;
    bool m_isGameplay = false;

    QLineEdit* m_whiteField;
    QLineEdit* m_whiteEloField;
    QLineEdit* m_blackField;
    QLineEdit* m_blackEloField;
    QLineEdit* m_resultField;
    QLineEdit* m_eventField;
    QLineEdit* m_roundField;
    QLineEdit* m_dateField;
    QToolButton* m_toggleEditBtn;
    bool m_inEditMode = false;
};

#endif // CHESSGAMEWINDOW_H
