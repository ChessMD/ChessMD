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
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53)); //hcc
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25)); //hcc
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53)); //hcc
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53)); //hcc
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218)); //hcc
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218)); //hcc
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    app.setPalette(darkPalette);
}

void Theme::applyLightTheme(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240)); //hcc
    lightPalette.setColor(QPalette::WindowText, Qt::black);
    lightPalette.setColor(QPalette::Base, Qt::white);
    lightPalette.setColor(QPalette::AlternateBase, QColor(233, 233, 233)); //hcc
    lightPalette.setColor(QPalette::ToolTipBase, Qt::white);
    lightPalette.setColor(QPalette::ToolTipText, Qt::black);
    lightPalette.setColor(QPalette::Text, Qt::black);
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240)); //hcc
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    lightPalette.setColor(QPalette::BrightText, Qt::red);
    lightPalette.setColor(QPalette::Link, QColor(0, 0, 255)); //hcc
    lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215)); //hcc
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);
    
    app.setPalette(lightPalette);
}