#include <QFileDialog>
#include <QOperatingSystemVersion>

#include "databaselibrary.h"
#include "mainwindow.h"
#include "settingsdialog.h"

#include <QToolBar>
#include <QToolButton>
#include <QLabel>
#include <QAction>
#include <QHBoxLayout>
#include <QThread>

MainWindow::MainWindow()
{

    m_dbLibrary = new DatabaseLibrary(this);
    setStatusBar(new QStatusBar);
    setupToolbar();
    QWidget* container = new QWidget(this);
    QHBoxLayout* h = new QHBoxLayout(container);
    h->setContentsMargins(0,0,0,0);
    h->setSpacing(0);
    QWidget* sidebarWidget = setupSidebar();
    h->addWidget(sidebarWidget, 0);
    h->addWidget(m_dbLibrary, 1);
    setCentralWidget(container);
    setMinimumSize(800,600);
}

void MainWindow::setStatusBarText(const QString &text)
{
    statusBar()->showMessage(text);
}

void MainWindow::setupToolbar() {
    QToolBar* toolbar = new QToolBar(this);
    toolbar->setOrientation(Qt::Horizontal);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(32,32));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QAction* importAct = new QAction(QIcon(":/resource/img/database-upload-icon.png"), tr("Import Database"), this);
    importAct->setToolTip(tr("Import Database"));
    connect(importAct, &QAction::triggered, m_dbLibrary, &DatabaseLibrary::importDatabase);
    toolbar->addAction(importAct);

    QAction* newDbAct = new QAction(QIcon(":/resource/img/database-add-icon.png"), tr("New Database"), this);
    newDbAct->setToolTip(tr("New Database"));
    connect(newDbAct, &QAction::triggered, m_dbLibrary, &DatabaseLibrary::newDatabase);
    toolbar->addAction(newDbAct);

    QAction* newBoardAct = new QAction(QIcon(":/resource/img/board-icon.png"), tr("New Board"), this);
    newBoardAct->setToolTip(tr("New Chessboard"));
    connect(newBoardAct, &QAction::triggered, m_dbLibrary, &DatabaseLibrary::newChessboard);
    toolbar->addAction(newBoardAct);

    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    addToolBar(Qt::TopToolBarArea, toolbar);
}


QWidget* MainWindow::setupSidebar() {
    QWidget* sidebar = new QWidget(this);
    sidebar->setFixedWidth(48);
    sidebar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    QVBoxLayout* v = new QVBoxLayout(sidebar);
    v->setContentsMargins(0,0,0,0);
    v->setSpacing(0);
    v->addStretch();

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
    v->addWidget(spacer);

    QToolButton* aboutButton = new QToolButton(this);
    aboutButton->setIcon(QIcon(":/resource/img/help-circle.png"));
    aboutButton->setToolTip(tr("About ChessMD"));
    aboutButton->setIconSize(QSize(32,32));
    aboutButton->setAutoRaise(true);
    connect(aboutButton, &QToolButton::clicked, this, [this](){
        QMessageBox msg(this);
        msg.setWindowTitle(tr("About ChessMD"));
        msg.setTextFormat(Qt::RichText);
        // allow the user to click links
        msg.setTextInteractionFlags(Qt::TextBrowserInteraction);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setText(tr(
                        "<h3>ChessMD</h3>"
                        "<p>Version %1</p>"
                        "<p>© 2025 ChessMD</p>"
                        "<p>A lightweight PGN database viewer and analysis tool.</p>"
                        "<p>Visit our <a href=\"https://chessmd.org/\">website</a> for more info.</p>"
                        ).arg("v1.0-beta.2"));

        // find the internal QLabel and enable external link opening
        if (auto *label = msg.findChild<QLabel*>("qt_msgbox_label")) {
            label->setOpenExternalLinks(true);
        }

        msg.exec();
    });
    v->addWidget(aboutButton);


    // settings
    QToolButton* settingsButton = new QToolButton(this);
    settingsButton->setIcon(QIcon(":/resource/img/settings.png"));
    settingsButton->setToolTip(tr("Settings"));
    settingsButton->setIconSize(QSize(32,32));
    settingsButton->setAutoRaise(true);
    connect(settingsButton, &QToolButton::clicked, this, &MainWindow::onSettings);
    v->addWidget(settingsButton);
    v->addSpacing(5);

    return sidebar;
}

void MainWindow::onSaveRequested(const QString &path, const QVector<PGNGame> &database) {
    if (auto oldThread = m_saveThreads.value(path, nullptr)) {
        if (oldThread->isRunning()) {
            oldThread->requestInterruption();
            oldThread->wait();
        }
        oldThread->deleteLater();
        m_saveThreads.remove(path);
    }

    QThread *worker = QThread::create([path, database]() {
        QString tempPath = path + ".tmp";
        QFile tempFile(tempPath);
        if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            return;
        QTextStream out(&tempFile);

        for (PGNGame game: database) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                tempFile.close();
                return;
            }
            out << game.serializePGN() << "\n\n";
        }

        tempFile.close();
        QFile::remove(path);
        tempFile.rename(path);
    });

    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &QThread::finished, this, [this, path]() { m_saveThreads.remove(path); }, Qt::QueuedConnection);

    m_saveThreads.insert(path, worker);
    worker->start();
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
