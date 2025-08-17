#include "theme.h"
#include <QSettings>
#include <QPalette>
#include <QStyleFactory>

void Theme::applyTheme(QApplication& app) {
    QSettings settings;
    QString theme = settings.value("theme", "light").toString();
    
    if (theme == "dark") {
        applyDarkTheme(app);
    } else if (theme == "light") {
        applyLightTheme(app);
    }
}

void Theme::applyDarkTheme(QApplication& app) {
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(25, 25, 25)); //hcc
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(53, 53, 53)); //hcc
    darkPalette.setColor(QPalette::AlternateBase, QColor(25, 25, 25)); //hcc
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::black);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(25, 25, 25)); //hcc
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218)); //hcc
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218)); //hcc
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    darkPalette.setColor(QPalette::Light, QColor(80, 80, 80)); //hcc
    darkPalette.setColor(QPalette::Midlight, QColor(65, 65, 65)); //hcc
    darkPalette.setColor(QPalette::Dark, QColor(15, 15, 15)); //hcc
    darkPalette.setColor(QPalette::Mid, QColor(40, 40, 40)); //hcc
    darkPalette.setColor(QPalette::Shadow, QColor(0, 0, 0)); //hcc
    
    app.setPalette(darkPalette);
}

void Theme::applyLightTheme(QApplication& app) {
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240)); //hcc
    lightPalette.setColor(QPalette::WindowText, Qt::black);
    lightPalette.setColor(QPalette::Base, Qt::white);
    lightPalette.setColor(QPalette::AlternateBase, QColor(222, 222, 222)); //hcc
    lightPalette.setColor(QPalette::ToolTipBase, Qt::white);
    lightPalette.setColor(QPalette::ToolTipText, Qt::black);
    lightPalette.setColor(QPalette::Text, Qt::black);
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240)); //hcc
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    lightPalette.setColor(QPalette::BrightText, Qt::red);
    lightPalette.setColor(QPalette::Link, QColor(0, 0, 255)); //hcc
    lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215)); //hcc
    lightPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    lightPalette.setColor(QPalette::Light, QColor(255, 255, 255)); //hcc
    lightPalette.setColor(QPalette::Midlight, QColor(245, 245, 245)); //hcc
    lightPalette.setColor(QPalette::Dark, QColor(160, 160, 160)); //hcc
    lightPalette.setColor(QPalette::Mid, QColor(200, 200, 200)); //hcc
    lightPalette.setColor(QPalette::Shadow, QColor(120, 120, 120)); //hcc
    
    app.setPalette(lightPalette);
}
