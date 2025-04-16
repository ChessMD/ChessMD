#ifndef CHESSGAMEWINDOW_H
#define CHESSGAMEWINDOW_H

#include <QMainWindow>
#include "NotationViewer.h"

class ChessGameWindow  : public QMainWindow
{
    Q_OBJECT
public:
    explicit ChessGameWindow (QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    NotationViewer* m_notationViewer;

private slots:
    void onPasteClicked();
    void onLoadPgnClicked();
    void onResetBoardClicked();
    void onExportPgnClicked();

signals:
};

#endif // CHESSGAMEWINDOW_H
