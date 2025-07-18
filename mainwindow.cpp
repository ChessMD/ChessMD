#include <QFileDialog>
#include <QOperatingSystemVersion>

#include "chesstabhost.h"
#include "databaselibrary.h"
#include "chessqsettings.h"

#include "mainwindow.h"
#include "settingsdialog.h"

#include <QToolBar>
#include <QAction>

MainWindow::MainWindow()
{

    m_dbLibrary = new DatabaseLibrary(this);
    setStatusBar(new QStatusBar);
    setCentralWidget(m_dbLibrary);
    setupSidebar();
    setMinimumSize(800,600);
}

void MainWindow::setStatusBarText(const QString &text)
{
    statusBar()->showMessage(text);
}

void MainWindow::setupSidebar() {
    QToolBar* sidebar = new QToolBar(this);
    sidebar->setOrientation(Qt::Vertical);
    sidebar->setMovable(false);
    sidebar->setFloatable(false);
    sidebar->setIconSize(QSize(32, 32));
    sidebar->setFixedWidth(48);
    
    sidebar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);


    //center icons
    sidebar->setStyleSheet(R"(
        QToolBar {
            padding: 0px;
            margin: 0px;
        }
        QToolButton {
            padding: 8px;
        }
    )");

    //spacer to put settings at bottom
    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    spacer->setMinimumHeight(10); 
    sidebar->addWidget(spacer);

    QToolButton* aboutButton = new QToolButton(this);
    aboutButton->setIcon(QIcon(":/resource/img/help-circle.png"));
    aboutButton->setToolTip(tr("About ChessMD"));
    aboutButton->setIconSize(QSize(32,32));
    connect(aboutButton, &QToolButton::clicked, this, [this](){
        QMessageBox::about(this, tr("About ChessMD"),
        tr("<h3>ChessMD</h3>"
          "<p>Version %1</p>"
          "<p>© 2025 ChessMD</p>"
          "<p>A lightweight PGN database viewer and analysis tool.</p>")
           .arg("v1.0-beta")
        );
    });
    sidebar->addWidget(aboutButton);

    // settings
    QToolButton* settingsButton = new QToolButton(this);
    settingsButton->setIcon(QIcon(":/resource/img/settings.png"));
    settingsButton->setToolTip(tr("Settings"));
    settingsButton->setIconSize(QSize(32,32));
    settingsButton->setAutoRaise(true);
    connect(settingsButton, &QToolButton::clicked, this, &MainWindow::onSettings);
    sidebar->addWidget(settingsButton);

    // bottom spacer
    QWidget* bottomSpacer = new QWidget(this);
    bottomSpacer->setFixedHeight(5);
    sidebar->addWidget(bottomSpacer);

    // add sidebar to the left
    addToolBar(Qt::LeftToolBarArea, sidebar);
}

void MainWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    setMinimumSize(0,0);
}

void MainWindow::onAddGame()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Select a chess PGN file", "", "PGN files (*.pgn)");

    m_dbLibrary->AddNewGame(file_name);
}

void MainWindow::onSettings(){
    SettingsDialog dlg(this);
    dlg.exec();
}
