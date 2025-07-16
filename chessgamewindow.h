/*
March 18, 2025: File Creation
*/

#ifndef CHESSGAMEWINDOW_H
#define CHESSGAMEWINDOW_H


#include "notationviewer.h"
#include "enginewidget.h"
#include "chessposition.h"
#include "pgngamedata.h"

#include <QQuickWidget>
#include <QMainWindow>
#include <QToolBar>
#include <QDockWidget>
#include <QCloseEvent>
#include <QMessageBox>

class HeaderEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit HeaderEditDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(tr("Edit PGN Headers"));
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(tr("Here you will edit your PGN headers…"), this));

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal, this
            );
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
};

// Main window that displays a chessboard, an engine, a notation viewer, and an opening viewer
class ChessGameWindow  : public QMainWindow
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

    void onMoveHovered(QSharedPointer<NotationMove> move);

private:
    void updateEngineActions();

    NotationViewer* m_notationViewer;
    EngineWidget* m_engineViewer;
    ChessPosition* m_positionViewer;
    QToolBar* m_Toolbar;

    QDockWidget* m_notationDock;
    QDockWidget* m_engineDock;

    QAction* m_startEngineAction;
    QAction* m_stopEngineAction;

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
