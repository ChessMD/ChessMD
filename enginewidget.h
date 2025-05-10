/*
April 11, 2025: File Creation
*/

#ifndef ENGINEWIDGET_H
#define ENGINEWIDGET_H

#include "uciengine.h"
#include "notation.h"

#include <QWidget>
#include <QTreeWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>

class EngineWidget : public QWidget {
    Q_OBJECT
public:
    explicit EngineWidget(QWidget *parent = nullptr);
    void setPosition(const QString &fen);

public slots:
    void onMoveSelected(const QSharedPointer<NotationMove>& move);

private slots:
    void onAnalyseClicked();
    void onStopClicked();
    void onPvUpdate(const PvInfo &info);
    void onBestMove(const QString &move);
    void onInfoLine(const QString &line);
    void onCmdSent(const QString &cmd);

private:
    UciEngine       *m_engine;
    QTreeWidget     *m_tree;
    QSpinBox        *m_multiPv;
    QPushButton     *m_buttonAnalyse;
    QPushButton     *m_buttonStop;
    QTextEdit       *m_console;
    QString          m_currentFen;
    QMap<int, QTreeWidgetItem*> m_lineItems;
};

#endif // ENGINEWIDGET_H
