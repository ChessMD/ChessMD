/*
March 18, 2025: File Creation
*/

#ifndef CHESSGAMEWINDOW_H
#define CHESSGAMEWINDOW_H


#include "notationviewer.h"
#include "enginewidget.h"
#include "chessposition.h"
#include "pgngamedata.h"
#include "openingviewer.h"

#include <QQuickWidget>
#include <QMainWindow>
#include <QToolBar>
#include <QDockWidget>

// Main window that displays a chessboard, an engine, a notation viewer, and an opening viewer
class ChessGameWindow  : public QMainWindow
{
    Q_OBJECT
public:
    explicit ChessGameWindow (QWidget *parent, PGNGame game);

    void notationSetup();
    void engineSetup();
    void notationToolbarSetup();
    void toolbarSetup();
    void openingSetup();

    void mainSetup();
    void previewSetup();

protected:
    void showEvent(QShowEvent *ev) override;

public slots:
    void onMoveHovered(QSharedPointer<NotationMove> move);

private:
    NotationViewer* m_notationViewer;
    OpeningViewer* m_openingViewer;
    EngineWidget* m_engineViewer;
    ChessPosition* m_positionViewer;
    QToolBar* m_Toolbar;


    QDockWidget* m_notationDock;
    QDockWidget* m_engineDock;
    QDockWidget* m_openingDock;

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
    void onExportPgnClicked();

signals:
};

#endif // CHESSGAMEWINDOW_H
