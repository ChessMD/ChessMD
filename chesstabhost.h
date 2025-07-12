
#ifndef CHESSTABHOST_H
#define CHESSTABHOST_H

#include <QWidget>
#include <QTabBar>
#include <QToolButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QLabel>


class CustomTabBar : public QTabBar {
    Q_OBJECT

public:
    explicit CustomTabBar(int defaultWidth, QWidget* parent = nullptr);


protected:
    QSize tabSizeHint(int index) const override;

private:
    const int defaultWidth;
};

namespace Ui {
class CustomTitleBar;
}

class CustomTitleBar : public QWidget
{
    Q_OBJECT
public:
    Ui::CustomTitleBar *ui;
    explicit CustomTitleBar(QWidget *parent = nullptr);
    ~CustomTitleBar();
    QTabBar* tabBar;
    QToolButton* addTabButton;

public slots:
    void MinimizeWindow();
    void MaximizeWindow();
    void CloseWindow();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QPoint clickPos;
    bool isMoving;

};

class ChessTabHost : public QWidget
{
    Q_OBJECT
public:
    explicit ChessTabHost(QWidget *parent = nullptr);
    void addNewTab(QWidget* embed, QString title);
    int rowCount();
    bool tabExists(QString label);
    void activateTabByLabel(QString label);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onAddTabClicked();
    void onTabMoved(int from, int to);
    void onTabReplaced(const QString &fileIdentifier);

private:
    QTabBar* tabBar;
    QToolButton* addTabButton;
    QStackedWidget* stack;

signals:
};

#endif // CHESSTABHOST_H
