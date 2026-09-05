#include "settingsdialog.h"
#include "streamparser.h"
#include "chessqsettings.h"
#include "openingviewer.h"
#include "translationmanager.h"
#include "helpers.h"


#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QProgressBar>
#include <QApplication>
#include <QOperatingSystemVersion>
#include <QSettings>
#include <QComboBox>
#include <QTemporaryFile>
#include <QMessageBox>
#include <QTimer>
#include <fstream>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent), mOpeningsPath("")
{
    setWindowTitle(tr("Settings"));
    resize(480, 260);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    mCategoryList = new QListWidget(this);
    mCategoryList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mCategoryList->addItem(new QListWidgetItem(QIcon(getIconPath("engine.png")), tr("Engine")));
    mCategoryList->addItem(new QListWidgetItem(QIcon(getIconPath("book.png")), tr("Opening")));
    mCategoryList->addItem(new QListWidgetItem(QIcon(getIconPath("edit.png")), tr("Theme")));
    mCategoryList->addItem(new QListWidgetItem(QIcon(getIconPath("translate.png")), tr("Language")));
    mCategoryList->setIconSize(QSize(20, 20));
    mCategoryList->setFixedWidth(120);
    mainLayout->addWidget(mCategoryList);

    mStackedWidget = new QStackedWidget(this);
    

    ChessQSettings s; s.loadSettings();

/*----------------------------/
\   engine page               \
/----------------------------*/
    QString engineSaved = s.getEngineFile();
    QWidget* enginePage = new QWidget(this);
    QVBoxLayout* engineLayout = new QVBoxLayout(enginePage);
    QString engineText = tr("Current engine: %1").arg((!engineSaved.isEmpty() && QFileInfo::exists(engineSaved)) ? engineSaved : tr("None"));
    mEnginePathLabel = new QLabel(engineText, enginePage);
    QPushButton* selectEngineBtn = new QPushButton(tr("Select Engine..."), enginePage);

    engineLayout->addWidget(mEnginePathLabel);
    engineLayout->addWidget(selectEngineBtn);
    engineLayout->addStretch();
    mStackedWidget->addWidget(enginePage);
    
/*----------------------------/
\   openings page             \
/----------------------------*/
    QWidget* openingsPage = new QWidget(this);
    QVBoxLayout* openingsLayout = new QVBoxLayout(openingsPage);
	
    ChessQSettings settings;
    const QString openingDirectory = settings.getOpeningDirectory();
    QDir openingDir(openingDirectory);
    bool openingFilesExist = openingDir.exists("openings.bin") && openingDir.exists("openings.headers");
    QString openingText = tr("Opening database: %1\nLocation: %2").arg(openingFilesExist ? tr("Exists!") : tr("Not found"), openingDirectory);

	mOpeningsPathLabel = new QLabel(openingText, openingsPage);
    QPushButton* loadPgnBtn = new QPushButton(tr("Create new opening database"), openingsPage);
    QPushButton *selectOpeningDirBtn = new QPushButton(tr("Switch opening database directory"), openingsPage);
    QLabel* info = new QLabel(tr("Convert a PGN to an opening database. RAM usage per PGN size in %1: 10 GB to 1 GB.").arg(QCoreApplication::applicationVersion()), openingsPage);
    QLabel* info2 = new QLabel(tr("The new directory should contain openings.bin and openings.header."), openingsPage);

    auto createHorizontalLine = []{
        QFrame *line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        return line;
    };

    openingsLayout->addWidget(mOpeningsPathLabel);
    openingsLayout->addWidget(loadPgnBtn);
    openingsLayout->addWidget(info);
    openingsLayout->addSpacing(8);
    openingsLayout->addWidget(createHorizontalLine());
    openingsLayout->addSpacing(8);
    openingsLayout->addWidget(selectOpeningDirBtn);
    openingsLayout->addWidget(info2);
    openingsLayout->addSpacing(8);
    openingsLayout->addWidget(createHorizontalLine());
    openingsLayout->addStretch();
    mStackedWidget->addWidget(openingsPage);

    mDownloadLinkLabel = new QLabel(openingsPage);
    mDownloadLinkLabel->setText(tr("Checking for a remote download link..."));
    mDownloadLinkLabel->setTextFormat(Qt::RichText);
    mDownloadLinkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mDownloadLinkLabel->setOpenExternalLinks(true); // clicking opens default browser
    QLabel *downloadInfoLabel = new QLabel(openingsPage);
    downloadInfoLabel->setText(tr("After downloading, extract the files and move them under the opening folder."));
    openingsLayout->addWidget(mDownloadLinkLabel);
    openingsLayout->addWidget(downloadInfoLabel);

    // start network fetch for the JSON
    QNetworkAccessManager *networkMgr = new QNetworkAccessManager(this);
    connect(networkMgr, &QNetworkAccessManager::finished, this, &SettingsDialog::onDownloadLinkReply);
    QUrl jsonUrl = QUrl::fromUserInput(QString::fromUtf8("https://chessmd.org/opening-download.json"));
    if (jsonUrl.isValid()) {
        QNetworkRequest req(jsonUrl);
        networkMgr->get(req);
    } else {
        mDownloadLinkLabel->setText(tr("No remote download JSON configured."));
    }
    
/*----------------------------/
\   theme page                \
/----------------------------*/
    QWidget* themePage = new QWidget(this);
    QVBoxLayout* themeLayout = new QVBoxLayout(themePage);

    QLabel* themeLabel = new QLabel(tr("Theme:"), themePage);
    mThemeComboBox = new QComboBox(themePage);
    mThemeComboBox->addItem(tr("Light"));
    mThemeComboBox->addItem(tr("Dark"));
    mThemeComboBox->addItem(tr("System"));

    QSettings tsettings;
    QString currentTheme = tsettings.value("theme").toString();
    if (currentTheme == "light") mThemeComboBox->setCurrentIndex(0);
    else if (currentTheme == "dark") mThemeComboBox->setCurrentIndex(1);
    else if (currentTheme == "system") mThemeComboBox->setCurrentIndex(2);
    else mThemeComboBox->setCurrentIndex(0);

    QLabel* themeInfo = new QLabel(tr("Theme changes will be applied when you restart the application."), themePage);
    themeInfo->setStyleSheet("color: palette(text); font-size: 11px;");

    themeLayout->addWidget(themeLabel);
    themeLayout->addWidget(mThemeComboBox);
    themeLayout->addWidget(themeInfo);
    themeLayout->addStretch();
    mStackedWidget->addWidget(themePage);

/*----------------------------/
\   language page             \
/----------------------------*/
    QWidget* languagePage = new QWidget(this);
    QVBoxLayout* languageLayout = new QVBoxLayout(languagePage);
    QLabel* languageLabel = new QLabel(tr("Language:"), languagePage);
    mLanguageComboBox = new QComboBox(languagePage);

    auto supportedLanguages = TranslationManager::instance().supportedLanguages();
    for (const auto& language: supportedLanguages){
        mLanguageComboBox->addItem(language.name);
    }

    int curLangIndex = tsettings.value("language", 0).toInt();
    if (curLangIndex < 0 || curLangIndex >= supportedLanguages.size()) {
        TranslationManager::instance().setLanguage(0);
        curLangIndex = 0; // english fallback
    }
    mLanguageComboBox->setCurrentIndex(curLangIndex);
    QTimer::singleShot(0, this, [](){ /* code here */ });

    mLanguageInfo = new QLabel(tr("Language changes will be applied when you restart the application."), languagePage);
    mLanguageInfo->setStyleSheet("color: palette(text); font-size: 11px;");

    updateLanguageInfo(curLangIndex);

    languageLayout->addWidget(languageLabel);
    languageLayout->addWidget(mLanguageComboBox);
    languageLayout->addWidget(mLanguageInfo);
    languageLayout->addStretch();
    mStackedWidget->addWidget(languagePage);

/*----------------------------/
\   final setup               \
/----------------------------*/
    mainLayout->addWidget(mStackedWidget);

    connect(mCategoryList, &QListWidget::currentRowChanged, mStackedWidget, &QStackedWidget::setCurrentIndex);
    mCategoryList->setCurrentRow(0);
    connect(loadPgnBtn, &QPushButton::clicked, this, &SettingsDialog::onLoadPgnClicked);
    connect(selectOpeningDirBtn, &QPushButton::clicked, this, &SettingsDialog::onSelectOpeningDirectoryClicked);
    connect(selectEngineBtn, &QPushButton::clicked, this, &SettingsDialog::onSelectEngineClicked);
    connect(mThemeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onThemeChanged);
    connect(mLanguageComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onLanguageChanged);

    QString enginePath = settings.getEngineFile();
    if (!enginePath.isEmpty()) {
        QFileInfo engineInfo(enginePath);
        mEnginePathLabel->setText(tr("Current engine: %1").arg(engineInfo.fileName()));
    }
}

void SettingsDialog::onDownloadLinkReply(QNetworkReply *reply)
{
    if (!mDownloadLinkLabel) { if (reply) reply->deleteLater(); return; }

    if (!reply) {
        mDownloadLinkLabel->setText(tr("Failed to fetch remote link (no reply)."));
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        mDownloadLinkLabel->setText(tr("Failed to fetch remote link: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    const QByteArray body = reply->readAll();
    reply->deleteLater();

    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(body, &perr);
    if (perr.error != QJsonParseError::NoError) {
        mDownloadLinkLabel->setText(tr("Invalid JSON from server."));
        return;
    }

    QString foundUrl;
    auto pushIfString = [&](const QJsonValue &v){
        if (foundUrl.isEmpty() && v.isString()) {
            QString s = v.toString().trimmed();
            if (!s.isEmpty()) foundUrl = s;
        }
    };

    if (doc.isObject()) {
        QJsonObject root = doc.object();
        if (root.contains("url") && root.value("url").isString())
            foundUrl = root.value("url").toString().trimmed();

        if (foundUrl.isEmpty() && root.contains("links") && root.value("links").isArray()) {
            QJsonArray arr = root.value("links").toArray();
            for (const QJsonValue &v : arr) { pushIfString(v); if (!foundUrl.isEmpty()) break; }
        }

        // also try "items"
        if (foundUrl.isEmpty() && root.contains("items") && root.value("items").isArray()) {
            QJsonArray arr = root.value("items").toArray();
            for (const QJsonValue &v : arr) { pushIfString(v); if (!foundUrl.isEmpty()) break; }
        }
    } else if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const QJsonValue &v : arr) { pushIfString(v); if (!foundUrl.isEmpty()) break; }
    }

    if (foundUrl.isEmpty()) {
        mDownloadLinkLabel->setText(tr("No download link found in JSON."));
        return;
    }

    QUrl url(foundUrl);
    if (url.scheme().isEmpty()) url.setScheme("https");

    if (!url.isValid() || !(url.scheme().toLower() == "http" || url.scheme().toLower() == "https")) {
        mDownloadLinkLabel->setText(tr("Found link is invalid or unsupported."));
        return;
    }

    const QString labelText = QStringLiteral("<a href=\"%1\">%2</a>").arg(url.toString().toHtmlEscaped(), tr("Download processsed database with 1+ million games (open link in browser, requires 3 GB disk space)"));
    mDownloadLinkLabel->setText(labelText);
    mDownloadLinkLabel->setToolTip(url.toString());
}

void SettingsDialog::onSelectEngineClicked()
{
    QOperatingSystemVersion osVersion = QOperatingSystemVersion::current();
    
    QString file_name;
    
    if (osVersion.type() == QOperatingSystemVersion::Windows) {
        file_name = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), "./engine", tr("Executable files (*.exe)"));
    } else {
		if (osVersion.type() == QOperatingSystemVersion::MacOS) {
			QDir dirBin(QApplication::applicationDirPath());
			dirBin.cdUp(), dirBin.cdUp(), dirBin.cdUp();
            file_name = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), dirBin.filePath("./engine"), "(*)");
		} else {
            file_name = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), "./engine", "(*)");
		}
    }
    
    if (!file_name.isEmpty()) {
        ChessQSettings settings;
        settings.setEngineFile(file_name);
        settings.saveSettings();
        
        QFileInfo engineInfo(file_name);
        mEnginePathLabel->setText(tr("Current engine: %1").arg(engineInfo.fileName()));
    }
}

bool finalizeHeaderFile(const QString &finalPath, const QString &tmpPath, const QVector<quint64> &relativeOffsets)
{
    QFile tmp(tmpPath);
    if (!tmp.open(QIODevice::ReadOnly)) return false;

    QDir dir(QFileInfo(finalPath).absolutePath());
    if (!dir.exists() && !dir.mkpath(".")) {
        tmp.close();
        return false;
    }

    QFile out(finalPath);
    if (!out.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open:" << finalPath;
        qDebug() << "Error:" << out.errorString();
        tmp.close();
        return false;
    }

    QDataStream outStream(&out);
    outStream.setVersion(QDataStream::Qt_6_5);

    quint32 gameCount = quint32(relativeOffsets.size());
    // header blob will start after: 4 bytes (count) + 8 * gameCount (offset table)
    quint64 base = 4 + quint64(8) * quint64(gameCount);

    // write count
    outStream << gameCount;

    // write offsets adjusted by base
    for (quint64 relOff : relativeOffsets) {
        outStream << (quint64)(base + relOff);
    }

    // copy tmp blob contents to final file
    const qint64 bufSize = 64 * 1024;
    QByteArray buf;
    buf.resize(bufSize);
    tmp.seek(0);
    while (!tmp.atEnd()) {
        qint64 n = tmp.read(buf.data(), bufSize);
        if (n <= 0) break;
        out.write(buf.constData(), n);
    }

    out.close();
    tmp.close();
    tmp.remove();
    return true;
}

void SettingsDialog::reportProgress(qint64 bytesRead, qint64 total, QProgressBar *progressBar) {
    if (!progressBar) return;
    if (total > 0) {
        int scaled = int((double(bytesRead) / double(total)) * 1000.0);
        progressBar->setRange(0, 1000);
        progressBar->setValue(qBound(0, scaled, 1000));
    } else {
        // unknown total: pulse or increment small step
        progressBar->setRange(0, 0); // busy
    }
    QApplication::processEvents();
}

void SettingsDialog::importPgnFileStreaming(const QString &file, QProgressBar *progressBar) {
    if (file.isEmpty()) return;

    // open input file as binary
    std::ifstream ss(file.toStdString(), std::ios::binary);
    if (ss.fail()) {
        if (progressBar) progressBar->deleteLater();
        mOpeningsPathLabel->setText(tr("Failed to open file"));
        return;
    }

    // determine total bytes for progress reporting
    ss.clear();
    ss.seekg(0, std::ios::end);
    std::streamoff totalBytesStream = ss.tellg();
    ss.seekg(0, std::ios::beg);
    qint64 totalBytes = (totalBytesStream < 0 ? 0 : (qint64)totalBytesStream);

    // create temporary headers blob
    QTemporaryFile tmpHeader;
    // ensure it won't auto-remove until we finish:
    tmpHeader.setAutoRemove(false);
    tmpHeader.setFileTemplate(QDir::tempPath() + "/openings_headers_XXXXXX");
    if (!tmpHeader.open()) {
        if (progressBar) progressBar->deleteLater();
        mOpeningsPathLabel->setText(tr("Failed to create temporary headers file"));
        return;
    }
    QDataStream tmpOut(&tmpHeader);
    tmpOut.setVersion(QDataStream::Qt_6_5);
    QString tmpHeaderPath = tmpHeader.fileName();

    QVector<quint64> headerRelativeOffsets;
    QMap<quint64, QVector<quint32>> openingGameMap;
    QMap<quint64, PositionWinrate> openingWinrateMap;

    qint64 parseTime = 0;

    // skip leading BOM/garbage until '['
    const int EOF_MARK = std::char_traits<char>::eof();
    int ch;
    while ((ch = ss.peek()) != EOF_MARK && ch != '[') ss.get();

    quint32 gameIndex = 0;
    for (;;) {
        if (!ss.good()) break;

        std::string line;
        std::string bodyTextStd;
        QVector<QPair<QString, QString>> headersLocal;
        QString resultStr;

        // --- Read header lines
        ch = ss.peek();
        while (ch != EOF_MARK && ch == '[') {
            if (!std::getline(ss, line)) break;

            // parse tag and value robustly
            size_t firstQuote = line.find('"');
            QString tag, value;
            if (firstQuote != std::string::npos) {
                // tag between '[' and just before firstQuote (trim trailing whitespace)
                size_t tagStart = 1; // skip '['
                size_t tagEnd = firstQuote;
                while (tagEnd > tagStart && isspace((unsigned char)line[tagEnd - 1])) --tagEnd;
                if (tagEnd > tagStart) tag = QString::fromStdString(line.substr(tagStart, tagEnd - tagStart));

                // value between firstQuote and secondQuote
                size_t secondQuote = line.find('"', firstQuote + 1);
                if (secondQuote != std::string::npos && secondQuote > firstQuote + 1) {
                    value = QString::fromStdString(line.substr(firstQuote + 1, secondQuote - firstQuote - 1));
                }
            } else {
                // fallback: line like [Key Value] or malformed — try to trim brackets
                if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
                    std::string inner = line.substr(1, line.size() - 2);
                    // split by first space
                    size_t sp = inner.find(' ');
                    if (sp != std::string::npos) {
                        tag = QString::fromStdString(inner.substr(0, sp));
                        value = QString::fromStdString(inner.substr(sp + 1));
                    } else {
                        tag = QString::fromStdString(inner);
                    }
                }
            }

            if (!tag.isEmpty() && tag.endsWith(' ')) tag.chop(1);
            if (tag == "Result") resultStr = value;
            headersLocal.append(qMakePair(tag, value));
            ch = ss.peek();
        }

        // read body text until next header or EOF
        while (true) {
            std::streampos pos = ss.tellg();
            if (!std::getline(ss, line)) {
                break;
            }
            if (isHeaderLine(line)) {
                ss.clear();
                ss.seekg(pos);
                break;
            }
            bodyTextStd += line;
            bodyTextStd.push_back(' ');
        }

        // if no headers and no body, we are done
        if (headersLocal.isEmpty() && bodyTextStd.empty()) break;

        // Build PGNGame object for processing
        PGNGame game;
        game.headerInfo = headersLocal;
        game.result = resultStr;
        game.bodyText = QString::fromStdString(bodyTextStd);

        // record header offset and write header record to tmp blob
        quint64 relOff = quint64(tmpHeader.pos());
        headerRelativeOffsets.append(relOff);

        // extract a few canonical header fields for compact record
        QString white, whiteElo, black, blackElo, event, date;
        for (const auto &h : game.headerInfo) {
            if (h.first == "White") white = h.second;
            else if (h.first == "WhiteElo") whiteElo = h.second;
            else if (h.first == "Black") black = h.second;
            else if (h.first == "BlackElo") blackElo = h.second;
            else if (h.first == "Event") event = h.second;
            else if (h.first == "Date") date = h.second;
        }

        tmpOut << white << whiteElo << black << blackElo << event << date << game.result;
        tmpOut << game.bodyText;

        QElapsedTimer timer;
        timer.start();
        
        // Replacing old parseBodyText call
        parseGameFromPGN(game, true);
        
        parseTime += timer.elapsed();

        // collect zobrist hashes along first-variation/mainline
        QVector<quint64> zobristHashes;
        if (!game.rootMove.isNull()) {
            QSharedPointer<NotationMove> mv = game.rootMove;
            zobristHashes.push_back(mv->m_zobristHash);
            while (!mv->m_nextMoves.isEmpty()) {
                mv = mv->m_nextMoves.front();
                zobristHashes.push_back(mv->m_zobristHash);
            }
        }

        // interpret result
        enum GameResult { UNKNOWN, WHITE_WIN, BLACK_WIN, DRAW };
        GameResult gres = UNKNOWN;
        if (game.result == "1-0") gres = WHITE_WIN;
        else if (game.result == "0-1") gres = BLACK_WIN;
        else if (game.result == "1/2-1/2") gres = DRAW;

        // update maps (avoid counting same position twice per game)
        QSet<quint64> visited;
        for (int j = 0; j < qMin(MAX_OPENING_DEPTH, zobristHashes.size()); ++j) {
            quint64 z = zobristHashes[j];
            if (!visited.contains(z) && openingGameMap[z].size() < MAX_GAMES_TO_SHOW) {
                openingGameMap[z].push_back(gameIndex);
            }
            PositionWinrate &wr = openingWinrateMap[z];
            if (gres == WHITE_WIN) wr.whiteWin++;
            else if (gres == BLACK_WIN) wr.blackWin++;
            else if (gres == DRAW) wr.draw++;

            visited.insert(z);
        }

        // free heavy structures
        if (!game.rootMove.isNull()) game.rootMove.clear();
        game.bodyText.clear();
        game.headerInfo.clear();

        // UI progress update
        std::streamoff pos = ss.tellg();
        qint64 readPos = (pos < 0 ? 0 : (qint64)pos);
        reportProgress(readPos, totalBytes, progressBar);

        ++gameIndex;
        // loop continues to next game
        if (ss.eof()) break;
    } // end for-each-game

    // clean up tmp file (we need it closed before finalize)
    tmpHeader.close();
        
    ChessQSettings settings;
    QDir openingDir(settings.getOpeningDirectory());

    if (!openingDir.exists() && !openingDir.mkpath(".")) {
        mOpeningsPathLabel->setText(
            tr("Failed to create opening database directory")
        );
        if (progressBar) progressBar->deleteLater();
        return;
    }

    QString finalHeaderPath = openingDir.filePath("openings.headers");
    if (!finalizeHeaderFile(finalHeaderPath, tmpHeaderPath, headerRelativeOffsets)) {
        mOpeningsPathLabel->setText(tr("Failed to write opening headers file"));
        if (progressBar) progressBar->deleteLater();
        return;
    }

    // Build OpeningInfo from maps (same layout as earlier code)
    OpeningInfo openingInfo;
    openingInfo.startIndex.clear();
    openingInfo.startIndex.push_back(0);

    for (auto it = openingGameMap.begin(); it != openingGameMap.end(); ++it) {
        quint64 zobrist = it.key();
        const QVector<quint32> &games = it.value();
        openingInfo.zobristPositions.push_back(zobrist);
        for (quint32 gid : games) openingInfo.gameIDs.push_back(gid);
        openingInfo.insertedCount.push_back(games.size());
        openingInfo.startIndex.push_back(openingInfo.startIndex.back() + games.size());
    }

    for (auto winrates : std::as_const(openingWinrateMap)) {
        openingInfo.whiteWin.push_back(winrates.whiteWin);
        openingInfo.blackWin.push_back(winrates.blackWin);
        openingInfo.draw.push_back(winrates.draw);
    }

    QString finalBinPath = openingDir.filePath("openings.bin");
    if (!openingInfo.serialize(finalBinPath)) {
        mOpeningsPathLabel->setText(tr("Failed to write opening binary file"));
        if (progressBar) progressBar->deleteLater();
        return;
    }

    // finish UI
    if (progressBar) {
        progressBar->setValue(progressBar->maximum());
        progressBar->deleteLater();
    }
    mOpeningsPathLabel->setText(tr("Current opening database: %1").arg(file));


    QString human = QString("%1.%2 s").arg(parseTime / 1000).arg((parseTime % 1000), 3, 10, QChar('0'));
    qDebug() << human;
}

void SettingsDialog::onLoadPgnClicked() {
    QString file = QFileDialog::getOpenFileName(this, tr("Select a chess PGN file"), QString(), tr("PGN files (*.pgn)"));
    if (file.isEmpty()) {
        return;
    }

    ChessQSettings settings;
    const QString currentDirectory = settings.getOpeningDirectory();
    const QString selectedDirectory = QFileDialog::getExistingDirectory(this, tr("Select opening database directory"), currentDirectory, QFileDialog::ShowDirsOnly);
    if (selectedDirectory.isEmpty()) {
        return;
    }

    QDir openingDir(selectedDirectory);
    if (openingDir.exists("openings.bin") || openingDir.exists("openings.headers")){
        QMessageBox box(this);
        box.setWindowTitle(tr("Opening Database Already Exists"));
        box.setText(tr("The selected directory already contains opening database files. Creating a new opening database will replace the existing files. Do you want to continue?"));
        QPushButton *yesButton = box.addButton(tr("Yes"), QMessageBox::YesRole);
        QPushButton *noButton = box.addButton(tr("No"), QMessageBox::NoRole);
        box.setDefaultButton(noButton);
        box.exec();
        if (box.clickedButton() != yesButton)
            return;
    }

    settings.setOpeningDirectory(selectedDirectory);

    mOpeningsPath = file;
    mOpeningsPathLabel->setText(tr("Processing PGN file..."));

    // progress bar
    QProgressBar* progressBar = new QProgressBar(this);
    QVBoxLayout* openingsLayout = qobject_cast<QVBoxLayout*>(mStackedWidget->currentWidget()->layout());
    openingsLayout->insertWidget(2, progressBar);
    QApplication::processEvents();

    QElapsedTimer timer;
    timer.start();
    importPgnFileStreaming(file, progressBar);
    qint64 elapsedMs = timer.elapsed();
    QString human = QString("%1.%2 s").arg(elapsedMs / 1000).arg((elapsedMs % 1000), 3, 10, QChar('0'));
    qDebug() << "total time: " << human;

    if (progressBar && !progressBar->parent()) {
        progressBar->deleteLater();
    }
}

QString SettingsDialog::getOpeningsPath() const {
    return mOpeningsPath;
}

void SettingsDialog::onSelectOpeningDirectoryClicked()
{
    ChessQSettings settings;
    const QString currentDirectory = settings.getOpeningDirectory();
    const QString selectedDirectory = QFileDialog::getExistingDirectory(this, tr("Select opening database directory"), currentDirectory, QFileDialog::ShowDirsOnly);

    if (selectedDirectory.isEmpty()) {
        return;
    }

    QDir openingDir(selectedDirectory);
    if (!openingDir.exists("openings.bin") || !openingDir.exists("openings.headers")){
        QMessageBox::warning(this, tr("Invalid Opening Database"), tr("The selected directory does not contain both openings.bin and openings.headers."));
        return;
    }

    settings.setOpeningDirectory(selectedDirectory);
    mOpeningsPathLabel->setText(tr("Opening database: %1\nLocation: %2").arg(tr("Exists!"), selectedDirectory));
}

void SettingsDialog::onThemeChanged() {
    QString theme;
    int index = mThemeComboBox->currentIndex();
    switch (index) {
        case 0:
            theme = "light";
            break;
        case 1:
            theme = "dark";
            break;
        case 2:
            theme = "system";
            break;
        default:
            theme = "light";
    }

    QSettings settings;
    settings.setValue("theme", theme);
}


void SettingsDialog::updateLanguageInfo(const int languageInd){
    auto supportedLanguages = TranslationManager::instance().supportedLanguages();
    QTranslator translator;

    if (languageInd == 0){
        mLanguageInfo->setText("Language changes will be applied when you restart the application.");
    } else if (translator.load(QString(":/i18n/ChessMD_%1.qm").arg(supportedLanguages[languageInd].code))) {
        mLanguageInfo->setText(translator.translate("SettingsDialog", "Language changes will be applied when you restart the application."));
    } else {
        QMessageBox::warning(this, tr("Translation Error"), tr("Unable to translate to language: %1.").arg(supportedLanguages[languageInd].name));
    }
}

void SettingsDialog::onLanguageChanged() {
    int index = mLanguageComboBox->currentIndex();
    auto supportedLanguages = TranslationManager::instance().supportedLanguages();

    if (index < 0 || index >= supportedLanguages.size()){
        index = 0; // english fallback
    }

    // TranslationManager::instance().setLanguage(index);
    // ^ uncomment when implementing instant language switching ^

    QSettings settings;
    settings.setValue("language", index);
    updateLanguageInfo(index);
}
