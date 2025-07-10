#ifndef ENGINELINEWIDGET_H
#define ENGINELINEWIDGET_H

#include "notation.h"
#include "notationviewer.h"
#include "uciengine.h"

#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>

class EngineLineWidget : public QWidget {
    Q_OBJECT

public:
    explicit EngineLineWidget(const QString &eval, const QString &pv, const QSharedPointer<NotationMove>& rootMove, QWidget *parent = nullptr);
    void updateEval(const QString &newEval);

signals:
    void moveClicked(QSharedPointer<NotationMove> &move);
    void moveHovered(QSharedPointer<NotationMove> move);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void toggleExpanded();

private:
    QSharedPointer<NotationMove> m_rootMove;
    QList<MoveSegment> m_moveSegments;
    MoveSegment* m_hoveredSegment = nullptr;
    PvInfo m_info;

    QString m_fullText;
    QString m_evalTxt;
    bool m_expanded = false;



    QPushButton *m_evalBtn;
    QLabel *m_truncLabel;
    QToolButton *m_arrow;

    void restyleEval(const QString &text);
};
#endif // ENGINELINEWIDGET_H
