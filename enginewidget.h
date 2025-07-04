/*
April 11, 2025: File Creation
*/

#ifndef ENGINEWIDGET_H
#define ENGINEWIDGET_H

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

class EngineWidget : public QWidget {
    Q_OBJECT
public:
    explicit EngineWidget(QWidget *parent = nullptr);
    ~EngineWidget();

signals:
    void engineMoveClicked(QSharedPointer<NotationMove>& move);
    void moveHovered(QSharedPointer<NotationMove> move);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void onMoveSelected(const QSharedPointer<NotationMove>& move);

private slots:
    void doPendingAnalysis();
    void onPvUpdate(const PvInfo &info);
    void onInfoLine(const QString &line);
    void onCmdSent(const QString &cmd);

private:
    void analysePosition();
    void flushBufferedInfo();

    bool m_isHovering;
    QMap<int, PvInfo> m_bufferedInfo;

    QTimer *m_debounceTimer;
    UciEngine *m_engine;
    QScrollArea *m_scroll;
    QWidget *m_container;
    QVBoxLayout *m_containerLay;
    QPushButton *m_buttonAnalyse;
    QPushButton *m_buttonStop;
    QPushButton *m_evalButton;
    QTextEdit *m_console;
    QString m_currentFen;
    QString m_sideToMove;
    QSharedPointer<NotationMove> m_currentMove;

    QMap<int, EngineLineWidget*> m_lineWidgets;

    int m_multiPv;
};

#endif // ENGINEWIDGET_H
