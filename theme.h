#ifndef THEME_H
#define THEME_H

#include <QApplication>

class Theme {
public:
    static void applyTheme(QApplication& app);
    
private:
    static void applyDarkTheme(QApplication& app);
    static void applyLightTheme(QApplication& app);
};

#endif // THEME_H