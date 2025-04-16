#ifndef CHESSTABHOST_H
#define CHESSTABHOST_H

#include <QWidget>
#include <QTabBar>
#include <QToolButton>
#include <QStackedWidget>
#include <QVBoxLayout>

class ChessTabHost : public QWidget
{
    Q_OBJECT
public:
    explicit ChessTabHost(QWidget *parent = nullptr);
    void addNewTab(QWidget* embed, QString title);

private slots:
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onAddTabClicked();
    void onTabMoved(int from, int to);
    void onTabReplaced(const QString &fileIdentifier);

    void onGameActivated();

private:
    QTabBar* tabBar;
    QToolButton* addTabButton;
    QStackedWidget* stack;

signals:
};

#endif // CHESSTABHOST_H
