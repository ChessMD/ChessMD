#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QFileInfo>

#include "databaseviewer.h"
#include "databaselibrary.h"
#include "ui_databaselibrary.h"
#include "chessgamefilesdata.h"
#include "mainwindow.h"
#include "chesstabhost.h"
#include "helpers.h"

// DatabaseLibrary Constructor
DatabaseLibrary::DatabaseLibrary(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DatabaseLibrary)
{

    //
    // setup ui
    //

    ui->setupUi(this);
    listView = new QListView(this);
    // configure the view for an icon mode grid
    listView->setViewMode(QListView::IconMode);
    listView->setIconSize(QSize(64, 64));
    listView->setGridSize(QSize(100, 100));
    listView->setSpacing(10);
    listView->setUniformItemSizes(true);
    listView->setResizeMode(QListView::Adjust);
    listView->setWrapping(true);
    listView->setWordWrap(true);
    listView->setContextMenuPolicy(Qt::CustomContextMenu);


    // setup model
    model = new QStandardItemModel(this);
    LoadGamesList();

    // setup listView
    listView->setModel(model);
    ui->MainLayout->addWidget(listView);


    //signals and slots
    connect(listView, &QListView::doubleClicked, this, &DatabaseLibrary::onDoubleClick);
    connect(listView, &QListView::clicked, this, &DatabaseLibrary::onClick);
    connect(listView, &QListView::customContextMenuRequested, this, &DatabaseLibrary::showContextMenu);



    host = new ChessTabHost;
}

DatabaseLibrary::~DatabaseLibrary()
{
    delete ui;
}

// Handle opening databases
void DatabaseLibrary::onDoubleClick(const QModelIndex &index)
{
    QString fileName = index.data(Qt::ToolTipRole).toString();


    if (!host->tabExists(fileName)) {
        // Open new window if not previously opened
        DatabaseViewer * databaseViewer = new DatabaseViewer(fileName);
        databaseViewer->setWindowTitle(fileName);

        auto *mainWindow = qobject_cast<MainWindow*>(this->window());
        mainWindow->setStatusBarText("Loading ...");
        QApplication::processEvents();

        databaseViewer->importPGN();
        host->addNewTab(databaseViewer, fileName);

        mainWindow->setStatusBarText("");
        QApplication::processEvents();

    } else {
        // Direct to previously opened window
        host->activateTabByLabel(fileName);
    }

    // set focus to new window
    // source: https://stackoverflow.com/questions/6087887/bring-window-to-front-raise-show-activatewindow-don-t-work
    host->setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive | Qt::WindowMaximized);
    host->raise();
    host->activateWindow(); // for Windows
    host->show();

}


// Handle adding new database
void DatabaseLibrary::onClick(const QModelIndex &index)
{
    QString fileName = index.data(Qt::ToolTipRole).toString();
    host->activateTabByLabel(fileName);
}

void DatabaseLibrary::importDatabase()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Select a chess PGN file", "", "PGN files (*.pgn)");
    int row = getFileNameRow(file_name);
    if (row > 0) { // already exist
        listView->setCurrentIndex(model->index(row, 0));
        return;
    }

    AddNewGame(file_name);
}

void DatabaseLibrary::newDatabase()
{
    QString savePath = QFileDialog::getSaveFileName(this, tr("Save PGN Database"), QString(), tr("PGN files (*.pgn)"));
    if (savePath.isEmpty()) {
        return;
    }

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save Error"), tr("Unable to open file for writing."));
        return;
    }

    AddNewGame(savePath);
}


void DatabaseLibrary::newChessboard()
{
    PGNGame emptyGame;
    ChessGameWindow *gameWin = new ChessGameWindow(nullptr, emptyGame);
    gameWin->mainSetup();
    gameWin->setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive | Qt::WindowMaximized);
    gameWin->show();

    connect(gameWin, &ChessGameWindow::PGNGameUpdated, this, [this, gameWin](PGNGame &game) {
        QString savePath = QFileDialog::getSaveFileName(this, tr("Save PGN Game"), QString(), tr("PGN files (*.pgn)"));
        if (savePath.isEmpty()) {
            return;
        }

        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("Save Error"), tr("Unable to open file for writing."));
            return;
        }

        QTextStream out(&file);
        out << game.serializePGN();
        file.close();
        AddNewGame(savePath);
        gameWin->close();

        if (!host->tabExists(savePath)) {
            DatabaseViewer *databaseViewer = new DatabaseViewer(savePath);
            databaseViewer->setWindowTitle(savePath);
            ((MainWindow *)parent())->setStatusBarText(tr("Loading ..."));
            QApplication::processEvents();
            databaseViewer->importPGN();
            host->addNewTab(databaseViewer, savePath);
            ((MainWindow *)parent())->setStatusBarText("");
            QApplication::processEvents();
        } else {
            host->activateTabByLabel(savePath);
        }

        host->setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive | Qt::WindowMaximized);
        host->raise();
        host->activateWindow(); // for Windows
        host->show();
    });
}

// Add game to library model
void DatabaseLibrary::AddNewGame(QString file_name)
{
    if (file_name.isEmpty())
        return;

    QString name = file_name.mid(file_name.lastIndexOf("/") + 1);

    // init item for listView
    QStandardItem *item = new QStandardItem;
    item->setIcon(QIcon(":/resource/img/fileicon.png"));
    item->setText(name);
    item->setToolTip(file_name);
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    model->appendRow(item);

    ChessGameFilesData gamesData;
    gamesData.addNewGame(file_name);
}

// Load games from database
void DatabaseLibrary::LoadGamesList()
{
    ChessGameFilesData gamesData;
    QList<QString> gameFilesList = gamesData.getGameFilesList();

    int size = gameFilesList.size();

    for (int i = 0; i < size; i++) {
        QString file_name = gameFilesList[i];

        QString name = file_name.mid(file_name.lastIndexOf("/") + 1);

        QStandardItem *item = new QStandardItem;
        item->setIcon(QIcon(":/resource/img/fileicon.png"));
        item->setText(name);
        item->setToolTip(file_name);
        item->setTextAlignment(Qt::AlignCenter);
        item->setEditable(false);
        model->appendRow(item);
    }
}

// Handle right-click menu
void DatabaseLibrary::showContextMenu(const QPoint& pos)
{
    QModelIndex index = listView->currentIndex();

    if (index.row() == 0 && index.column() == 0) // don't show context menu for first one (Add game)
        return;

    QPoint globalPos = listView->viewport()->mapToGlobal(pos);

    QMenu rightClickMenu(listView);

    QAction* deleteAction = rightClickMenu.addAction("Delete");

    QAction*  selectedMenuItem = rightClickMenu.exec(globalPos);

    if (!selectedMenuItem)
        return;

    if (selectedMenuItem == deleteAction)
    {
        QString fileName = index.data(Qt::ToolTipRole).toString();

        ChessGameFilesData gamesData;
        gamesData.removeGameFile(fileName);

        model->removeRows(index.row(), 1);
    }
}

// Get row by file name
int DatabaseLibrary::getFileNameRow(QString file_name)
{
    for (int i = 0; i < model->rowCount(); i++) {
        QStandardItem *	item = model->item(i);
        if (item->toolTip() == file_name)
            return i;
    }

    return -1;
}

