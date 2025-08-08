#ifndef CHESSTABHOST_H
#define CHESSTABHOST_H

#include <QWidget>
#include <QTabBar>
#include <QToolButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>


class CustomTabBar : public QTabBar {
    Q_OBJECT

public:
    explicit CustomTabBar(int defaultWidth, QWidget* parent = nullptr);


protected:
    QSize tabSizeHint(int index) const override;
    #ifndef Q_OS_WIN
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    #endif
    void tabInserted(int index) override;
    void tabRemoved(int index) override;



private:
    const int defaultWidth;
    QPoint dragStartPos = QPoint(-1, -1);
    bool isDraggingWindow = false;


private slots:
    void addCloseButton(int index);
    void removeCloseButton(int index);
};

class CustomTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit CustomTitleBar(QWidget *parent = nullptr);
    ~CustomTitleBar();

    void startDrag(QPoint localPos);

    QTabBar* tabBar;
    QToolButton* addTabButton;

public slots:
    void MinimizeWindow();
    void MaximizeWindow();
    void CloseWindow();

protected:
    #ifndef Q_OS_WIN
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    #endif

    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    QPushButton* minimizeButton;
    QPushButton* maximizeButton;
    QPushButton* closeButton;
    
    QPoint clickPos = QPoint(-1, -1);
    bool isMoving = false;

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

#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

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
