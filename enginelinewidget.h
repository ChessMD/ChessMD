#ifndef ENGINELINEWIDGET_H
#define ENGINELINEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QResizeEvent>

class EngineLineWidget : public QWidget {
    Q_OBJECT

public:
    explicit EngineLineWidget(const QString &eval, const QString &pv, QWidget *parent = nullptr);
    void updateEval(const QString &newEval);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void toggleExpanded();
    void doElide();

private:
    QString m_fullText;
    bool m_expanded = false;

    QPushButton *m_evalBtn;
    QLabel *m_truncLabel;
    QToolButton *m_arrow;

    void updateElided();
    void restyleEval(const QString &text);
};
#endif // ENGINELINEWIDGET_H

