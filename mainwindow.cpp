#include <QFileDialog>
#include <QOperatingSystemVersion>

#include "databaselibrary.h"
#include "mainwindow.h"
#include "settingsdialog.h"
#include "helpers.h"

#include <QToolBar>
#include <QToolButton>
#include <QLabel>
#include <QAction>
#include <QHBoxLayout>
#include <QThread>
#include <QInputDialog>
#include <QCoreApplication>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>

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

    connect(this, &MainWindow::PGNReady, this, &MainWindow::onPGNReady);
    connect(this, &MainWindow::PGNFetchError, this, [this](const QString &msg) {
        QMessageBox::critical(this, tr("Download Error"), msg);
    });
}

void MainWindow::setupToolbar() {
    QToolBar* toolbar = new QToolBar(this);
    toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
    toolbar->setOrientation(Qt::Horizontal);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(32,32));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QAction* importAct = new QAction(QIcon(getIconPath("database-upload-icon.png")), tr("Import Database"), this);
    importAct->setToolTip(tr("Import Database"));
    connect(importAct, &QAction::triggered, m_dbLibrary, &DatabaseLibrary::importDatabase);
    toolbar->addAction(importAct);

    QAction* newDbAct = new QAction(QIcon(getIconPath("database-add-icon.png")), tr("New Database"), this);
    newDbAct->setToolTip(tr("New Database"));
    connect(newDbAct, &QAction::triggered, m_dbLibrary, &DatabaseLibrary::newDatabase);
    toolbar->addAction(newDbAct);

    QAction* newBoardAct = new QAction(QIcon(getIconPath("board-icon.png")), tr("New Board"), this);
    newBoardAct->setToolTip(tr("New Chessboard"));
    connect(newBoardAct, &QAction::triggered, m_dbLibrary, [this]{
        PGNGame emptyGame;
        m_dbLibrary->newChessboard(emptyGame);
    });
    toolbar->addAction(newBoardAct);

    QAction* playGameAct = new QAction(QIcon(getIconPath("robot-face.png")), tr("Play Bots"), this);
    playGameAct->setToolTip(tr("Play Game Against Engine"));
    connect(playGameAct, &QAction::triggered, m_dbLibrary, &DatabaseLibrary::newGameplayBoard);
    toolbar->addAction(playGameAct);

    QAction* chessComAct = new QAction(QIcon(getIconPath("cloud-file-download-icon.png")), tr("Import Online Database"), this);
    chessComAct->setToolTip(tr("Import games from Chess.com"));
    connect(chessComAct, &QAction::triggered, this, &MainWindow::onImportOnlineDatabase);
    toolbar->addAction(chessComAct);

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
    aboutButton->setIcon(QIcon(getIconPath("help-circle.png")));
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
                        ).arg(QCoreApplication::applicationVersion()));

        // find the internal QLabel and enable external link opening
        if (auto *label = msg.findChild<QLabel*>("qt_msgbox_label")) {
            label->setOpenExternalLinks(true);
        }
        qDebug() << "Highlight color:" << palette().color(QPalette::Highlight);

        msg.exec();
    });
    v->addWidget(aboutButton);


    // settings
    QToolButton* settingsButton = new QToolButton(this);
    settingsButton->setIcon(QIcon(getIconPath("settings.png")));
    settingsButton->setToolTip(tr("Settings"));
    settingsButton->setIconSize(QSize(32,32));
    settingsButton->setAutoRaise(true);
    connect(settingsButton, &QToolButton::clicked, this, &MainWindow::onSettings);
    v->addWidget(settingsButton);
    v->addSpacing(5);

    return sidebar;
}

void MainWindow::setStatusBarText(const QString &text)
{
    statusBar()->showMessage(text);
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

void MainWindow::fetchChesscomGamesAndSave(const QString &username, const int maxGames)
{
    QThread *worker = QThread::create([this, username, maxGames]() {
        QNetworkAccessManager networkManager;

        QUrl profileUrl(QString("https://api.chess.com/pub/player/%1").arg(username));
        QNetworkRequest profileRequest(profileUrl);
        QString agentInfo = QString("ChessMD/%2 (Contact: support@chessmd.org)").arg(QCoreApplication::applicationVersion());
        profileRequest.setRawHeader("User-Agent", agentInfo.toUtf8());
        QNetworkReply *profileReply = networkManager.get(profileRequest);
        if (!profileReply) {
            emit PGNFetchError(tr("Failed to start network request for '%1'.").arg(username));
            return;
        }

        QEventLoop profileLoop;
        connect(profileReply, &QNetworkReply::finished, &profileLoop, &QEventLoop::quit);
        if (!profileReply->isFinished()){
            profileLoop.exec();
        }

        QVariant profStatusVar = profileReply ? profileReply->attribute(QNetworkRequest::HttpStatusCodeAttribute) : QVariant();
        int profStatus = profStatusVar.isValid() ? profStatusVar.toInt() : 0;

        if (profStatus == 200) {
            // success retrieving profile
            profileReply->deleteLater();
        } else if (profStatus == 404) {
            profileReply->deleteLater();
            emit PGNFetchError(tr("Player not found: '%1'.").arg(username));
            return;
        } else if (profStatus == 410) {
            profileReply->deleteLater();
            emit PGNFetchError(tr("Player not found / account unavailable: '%1'. The account may have been removed or disabled (HTTP 410).").arg(username));
            return;
        } else if (profStatus == 429) {
            profileReply->deleteLater();
            emit PGNFetchError(tr("Rate limited by chess.com while checking player '%1' (HTTP 429). Try again later.").arg(username));
            return;
        } else if (profStatus > 0) {
            // other HTTP status
            profileReply->deleteLater();
            emit PGNFetchError(tr("HTTP %1 while checking player '%2'.").arg(QString::number(profStatus), username));
            return;
        } else {
            profileReply->deleteLater();
            emit PGNFetchError(tr("Network error while checking player '%1': Connection failed (no HTTP status returned").arg(username));
            return;
        }

        QNetworkRequest networkRequest(QUrl(QString("https://api.chess.com/pub/player/%1/games/archives").arg(username)));
        QNetworkReply *archivesReply = networkManager.get(networkRequest);
        if (!archivesReply){
            emit PGNFetchError(tr("Failed to start archives request for '%1'.").arg(username));
            return;
        }

        QEventLoop archiveLoop;
        connect(archivesReply, &QNetworkReply::finished, &archiveLoop, &QEventLoop::quit);
        if (!archivesReply->isFinished()){
            archiveLoop.exec();
        }

        QByteArray archivesBody;
        if (archivesReply) {
            archivesBody = archivesReply->readAll();
        }

        if (archivesReply->error() != QNetworkReply::NoError) {
            QString networkError = archivesReply ? archivesReply->errorString() : tr("Unknown network error");
            archivesReply->deleteLater();
            emit PGNFetchError(tr("Network error while fetching archives: %1").arg(networkError));
            return;
        }

        QVariant statusVar = archivesReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        int statusCode = statusVar.isValid() ? statusVar.toInt() : 0;

        if (statusCode == 200){
            // success retrieving archive
            archivesReply->deleteLater();
        } else if (statusCode == 404) {
            archivesReply->deleteLater();
            emit PGNFetchError(tr("Player not found: '%1'.").arg(username));
            return;
        } else if (statusCode == 429) {
            archivesReply->deleteLater();
            emit PGNFetchError(tr("Rate limited by chess.com (HTTP 429). Try again later."));
            return;
        } else {
            archivesReply->deleteLater();
            emit PGNFetchError(tr("HTTP %1 while fetching archives for '%2'.").arg(QString::number(statusCode), username));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(archivesBody);
        if (!doc.isObject()) {
            emit PGNFetchError(tr("Unexpected response when fetching archives for '%1'.").arg(username));
            return;
        }

        QJsonObject rootObj = doc.object();
        if (!rootObj.contains("archives") || !rootObj.value("archives").isArray()) {
            emit PGNFetchError(tr("Invalid response for '%1'.").arg(username));
            return;
        }

        QJsonArray archives = rootObj.value("archives").toArray();
        if (archives.isEmpty()) {
            emit PGNFetchError(tr("No games found for '%1'.").arg(username));
            return;
        }

        QString combinedPGN;
        int gamesCollected = 0;
        for (int i = archives.size()-1; i >= 0 && gamesCollected < maxGames; --i) {
            QUrl monthUrl = QUrl(archives[i].toString());
            QString PGNUrl = monthUrl.toString() + "/pgn"; // format: https://api.chess.com/pub/player/{username}/games/YYYY/MM
            QNetworkRequest request((QUrl(PGNUrl)));
            QNetworkReply *PGNReply = networkManager.get(request);
            if (!PGNReply){
                emit PGNFetchError(tr("Failed to start PGN request for '%1'.").arg(username));
                return;
            }

            QEventLoop PGNLoop;
            connect(PGNReply, &QNetworkReply::finished, &PGNLoop, &QEventLoop::quit);
            if (!PGNReply->isFinished()){
                PGNLoop.exec();
            }

            if (PGNReply->error() != QNetworkReply::NoError) {
                emit PGNFetchError(tr("Error fetching PGN for %1: %2").arg(PGNUrl, PGNReply->errorString()));
                PGNReply->deleteLater();
                return;
            }

            QByteArray monthlyPGN = PGNReply->readAll();
            QString monthly = QString::fromUtf8(monthlyPGN);

            QVector<int> eventPos;
            int idx = monthly.indexOf("[Event", 0);
            while (idx != -1) {
                eventPos.append(idx);
                idx = monthly.indexOf("[Event", idx + 1);
            }

            int monthCount = eventPos.size();
            if (!monthCount) continue;

            int remaining = maxGames - gamesCollected;
            if (monthCount <= remaining) {
                combinedPGN += monthly.trimmed() + "\n";
                gamesCollected += monthCount;
            } else {
                QString trimmed = monthly.left(eventPos[remaining]).trimmed();
                combinedPGN += trimmed + "\n";
                gamesCollected += remaining;
                break;
            }

            PGNReply->deleteLater();
            QThread::msleep(200); // small delay to be nice to the API
        }

        if (combinedPGN.isEmpty()) {
            emit PGNFetchError(tr("PGN string empty"));
            return;
        }

        QString filename = QString("Chesscom_%1_%2.pgn").arg(username, QDate::currentDate().toString("yyyyMMdd"));
        emit PGNReady(combinedPGN, filename);
    });

    connect(worker, &QThread::finished, this, [this, worker]() {
        worker->deleteLater();
        m_chesscomFetchRunning = false;
        QMetaObject::invokeMethod(this, "startNextChesscomFetch", Qt::QueuedConnection);
    });

    worker->start();
}

void MainWindow::enqueueChesscomFetch(const QString &username, int maxGames)
{
    m_chesscomFetchQueue.enqueue(qMakePair(username, maxGames));
    if (!m_chesscomFetchRunning) {
        startNextChesscomFetch();
    }
}

void MainWindow::startNextChesscomFetch()
{
    if (m_chesscomFetchQueue.isEmpty()) {
        m_chesscomFetchRunning = false;
        return;
    }

    auto pair = m_chesscomFetchQueue.dequeue();
    m_chesscomFetchRunning = true;
    fetchChesscomGamesAndSave(pair.first, pair.second);
}


void MainWindow::onPGNReady(const QString &combinedPGN, const QString &filename)
{
    QString savePath = QFileDialog::getSaveFileName(this, tr("Save combined PGN"), filename, tr("PGN files (*.pgn)"));
    if (savePath.isEmpty()) return;

    QFile filePath(savePath);
    if (!filePath.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save Error"), tr("Unable to open file for writing."));
        return;
    }

    QTextStream out(&filePath);
    out << combinedPGN;
    filePath.close();

    m_dbLibrary->AddNewGame(savePath);
}

void MainWindow::onImportOnlineDatabase()
{
    bool ok;
    QString username = QInputDialog::getText(this, tr("Chess.com Import"), tr("Chess.com username:"), QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || username.isEmpty()) return;
    int maxGames = QInputDialog::getInt(this, tr("Chess.com Import"), tr("Number of recent games to fetch:"), 20, 1, 1000, 1, &ok);
    if (!ok) return;

    enqueueChesscomFetch(username, maxGames);
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

void MainWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    setMinimumSize(0,0);
}



