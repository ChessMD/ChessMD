#ifndef ENGINEWIDGET_H
#define ENGINEWIDGET_H

#include "uciengine.h"
#include "notation.h"

#include <QWidget>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>

class EngineWidget : public QWidget {
    Q_OBJECT
public:
    explicit EngineWidget(QWidget *parent = nullptr);

    void setPosition(const QString &fen);

public slots:
    void onMoveSelected(const QSharedPointer<NotationMove>& move);

private slots:
    void onAnalyseClicked();
    void onEngineInfo(const QString &info);
    void onBestMove(const QString &move);

private:
    UciEngine   *m_engine;
    QTextEdit   *m_output;
    QSpinBox    *m_depth;
    QPushButton *m_button;
    QString      m_currentFen;
};
#endif // ENGINEWIDGET_H
