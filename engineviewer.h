/*
April 11, 2025: File Creation
*/

#ifndef ENGINEVIEWER_H
#define ENGINEVIEWER_H

#include "uciengine.h"
#include "notation.h"
#include "enginelinewidget.h"

#include <QWidget>
#include <QTreeWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>

class EngineWidget : public QWidget {
    Q_OBJECT
public:
    explicit EngineWidget(QWidget *parent = nullptr);

signals:
    void engineMoveClicked(QSharedPointer<NotationMove>& move);
    void moveHovered(QSharedPointer<NotationMove> move);
    void engineEvalScoreChanged(double evalScore);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void onMoveSelected(const QSharedPointer<NotationMove>& move);

private slots:
    void onEngineMoveClicked(QSharedPointer<NotationMove>& move);
    void onConfigEngineClicked();
    void doPendingAnalysis();
    void onPvUpdate(PvInfo &info);
    void onInfoLine(const QString &line);
    void onCmdSent(const QString &cmd);

private:
    void analysePosition();
    void flushBufferedInfo();

    bool m_isHovering;
    bool m_ignoreHover;
    QMap<int, PvInfo> m_bufferedInfo;

    QTimer *m_debounceTimer;
    UciEngine *m_engine;
    QScrollArea *m_scroll;
    QWidget *m_container;
    QVBoxLayout *m_containerLay;
    QPushButton *m_buttonAnalyse;
    QPushButton *m_buttonStop;
    QPushButton *m_evalButton;
    QLabel      *m_engineLabel;
    QTextEdit *m_console;
    QString m_currentFen;
    QString m_sideToMove;
    QSharedPointer<NotationMove> m_currentMove;

    QMap<int, EngineLineWidget*> m_lineWidgets;

    int m_multiPv;
};

#endif // ENGINEVIEWER_H
