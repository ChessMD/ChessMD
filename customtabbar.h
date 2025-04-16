#ifndef CUSTOMTABBAR_H
#define CUSTOMTABBAR_H

#include <QTabBar>

class CustomTabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit CustomTabBar(QWidget* parent = nullptr)
        : QTabBar(parent)
    {}

    QSize tabSizeHint(int index) const override {
        QSize size = QTabBar::tabSizeHint(index);
        int maxWidth = 600; // maximum width for each tab
        if (size.width() > maxWidth)
            size.setWidth(maxWidth);
        return size;
    }
};

#endif // CUSTOMTABBAR_H
