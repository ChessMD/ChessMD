#include "settingsdialog.h"
#include "streamparser.h"
#include "openingviewer.h"
#include "chessqsettings.h"

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
#include <fstream>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent), mOpeningsPath("")
{
    setWindowTitle(tr("Settings"));
    resize(480, 260);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    mCategoryList = new QListWidget(this);
    mCategoryList->addItem(tr("Engine"));
    mCategoryList->addItem(tr("Opening"));
    mCategoryList->addItem(tr("Theme"));
    mCategoryList->setFixedWidth(120);
    mainLayout->addWidget(mCategoryList);

    mStackedWidget = new QStackedWidget(this);
    

    ChessQSettings s; s.loadSettings();

    // engine page
    QString engineSaved = s.getEngineFile();
    QWidget* enginePage = new QWidget(this);
    QVBoxLayout* engineLayout = new QVBoxLayout(enginePage);
    QString engineText = "Current engine: " + ((!engineSaved.isEmpty() && QFileInfo::exists(engineSaved)) ? engineSaved : "None");
    mEnginePathLabel = new QLabel(engineText, enginePage);
    QPushButton* selectEngineBtn = new QPushButton(tr("Select Engine..."), enginePage);

    engineLayout->addWidget(mEnginePathLabel);
    engineLayout->addWidget(selectEngineBtn);
    engineLayout->addStretch();
    mStackedWidget->addWidget(enginePage);
    
    // openings page
    QWidget* openingsPage = new QWidget(this);
    QVBoxLayout* openingsLayout = new QVBoxLayout(openingsPage);
    QString openingText = QString("Current opening database: ") + ( (QFileInfo::exists("./opening/openings.bin") && QFileInfo::exists("./opening/openings.headers") ) ? "Exists! Uploading a new PGN will replace the existing database." : "Not found.");
    mOpeningsPathLabel = new QLabel(openingText, openingsPage);
    QPushButton* loadPgnBtn = new QPushButton(tr("Load PGN..."), openingsPage);
    QLabel* info = new QLabel(tr("Warning! In %1, 1 GB of memory (RAM) is required for every 3 MB of a PGN database.").arg(QCoreApplication::applicationVersion()), openingsPage);
    openingsLayout->addWidget(mOpeningsPathLabel);
    openingsLayout->addWidget(loadPgnBtn);
    openingsLayout->addWidget(info);
    openingsLayout->addStretch();
    mStackedWidget->addWidget(openingsPage);

    
    
    // theme page
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


    mainLayout->addWidget(mStackedWidget);

    connect(mCategoryList, &QListWidget::currentRowChanged, mStackedWidget, &QStackedWidget::setCurrentIndex);
    mCategoryList->setCurrentRow(0);
    connect(loadPgnBtn, &QPushButton::clicked, this, &SettingsDialog::onLoadPgnClicked);
    connect(selectEngineBtn, &QPushButton::clicked, this, &SettingsDialog::onSelectEngineClicked);
    connect(mThemeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onThemeChanged);
    
    ChessQSettings settings;
    QString enginePath = settings.getEngineFile();
    if (!enginePath.isEmpty()) {
        QFileInfo engineInfo(enginePath);
        mEnginePathLabel->setText(tr("Current engine: %1").arg(engineInfo.fileName()));
    }
}

void SettingsDialog::onSelectEngineClicked() {
    QOperatingSystemVersion osVersion = QOperatingSystemVersion::current();
    
    QString file_name;
    
    if (osVersion.type() == QOperatingSystemVersion::Windows) {
        file_name = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), "./engine", tr("Executable files (*.exe)"));
    } else {
        file_name = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), "./engine", tr("All files (*)"));
    }
    
    if (!file_name.isEmpty()) {
        ChessQSettings settings;
        settings.setEngineFile(file_name);
        settings.saveSettings();
        
        QFileInfo engineInfo(file_name);
        mEnginePathLabel->setText(tr("Current engine: %1").arg(engineInfo.fileName()));
    }
}

void SettingsDialog::onLoadPgnClicked() {
    QString file = QFileDialog::getOpenFileName(this, tr("Select a chess PGN file"), QString(), tr("PGN files (*.pgn)"));
    if (file.isEmpty()) return;

    mOpeningsPath = file;
    mOpeningsPathLabel->setText(tr("Processing PGN file..."));

    // progress bar
    QProgressBar* progressBar = new QProgressBar(this);
    QVBoxLayout* openingsLayout = qobject_cast<QVBoxLayout*>(mStackedWidget->currentWidget()->layout());
    openingsLayout->insertWidget(2, progressBar);

    std::ifstream ss(file.toStdString());
    if(ss.fail()) {
        progressBar->deleteLater();
        mOpeningsPathLabel->setText(tr("Failed to open file"));
        return;
    }

    StreamParser parser(ss);
    std::vector<PGNGame> database = parser.parseDatabase();

    progressBar->setMaximum(database.size());
    progressBar->setValue(0);

    OpeningTree tree;

    for (int i = 0; i < database.size(); i++) {
        auto &game = database[i];

        // update progress bar every 100 games
        if (i % 100 == 0) {
            progressBar->setValue(i);
            QApplication::processEvents();
        }

        parseBodyText(game.bodyText, game.rootMove);
        QVector<quint16> moveCodes;
        QVector<quint64> zobristHashes;

        QSharedPointer<NotationMove> move = game.rootMove;
        zobristHashes.push_back(move->m_zobristHash);
        while(!move->m_nextMoves.isEmpty()){
            move = move->m_nextMoves.front();
            zobristHashes.push_back(move->m_zobristHash);
            moveCodes.push_back(OpeningViewer::encodeMove(move->lanText));
        }

        GameResult result = UNKNOWN;
        if (game.result == "1-0") result = WHITE_WIN;
        else if (game.result == "0-1") result = BLACK_WIN;
        else if (game.result == "1/2-1/2") result = DRAW;

        QHash<quint64, bool> visitedPositions;
        for (int j = 0; j < qMin(35, zobristHashes.size()); j++){
            if (!visitedPositions.count(zobristHashes[j]) && tree.openingGameMap[zobristHashes[j]].size() < 1000) {
                tree.openingGameMap[zobristHashes[j]].push_back(i);
            }
            tree.openingWinrateMap[zobristHashes[j]].whiteWin += (result == WHITE_WIN);
            tree.openingWinrateMap[zobristHashes[j]].blackWin += (result == BLACK_WIN);
            tree.openingWinrateMap[zobristHashes[j]].draw += (result == DRAW);
            visitedPositions[zobristHashes[j]] = 1;
        }

        // tree.insertGame(moveCodes, i, result);

        game.rootMove.clear();
        game.bodyText.clear();
    }

    progressBar->setValue(database.size());

    mOpeningsPathLabel->setText(tr("Serializing database..."));
    tree.mOpeningInfo.zobristPositions.reserve(tree.openingGameMap.size());
    tree.mOpeningInfo.whiteWin.reserve(tree.openingGameMap.size());
    tree.mOpeningInfo.blackWin.reserve(tree.openingGameMap.size());
    tree.mOpeningInfo.draw.reserve(tree.openingGameMap.size());
    tree.mOpeningInfo.gameIDs.reserve(tree.openingGameMap.size());
    for (auto it = tree.openingGameMap.begin(); it != tree.openingGameMap.end(); it++) {
        quint64 zobrist = it.key();
        QVector<quint32> games = it.value();
        tree.mOpeningInfo.zobristPositions.push_back(zobrist);
        for (quint32 gameID: games) tree.mOpeningInfo.gameIDs.push_back(gameID);
    }
    for (auto winrates: std::as_const(tree.openingWinrateMap)) {
        tree.mOpeningInfo.whiteWin.push_back(winrates.whiteWin);
        tree.mOpeningInfo.blackWin.push_back(winrates.blackWin);
        tree.mOpeningInfo.draw.push_back(winrates.draw);
    }

    QApplication::processEvents();
    // for (auto it: tree.mOpeningInfo.zobristPositions) qDebug() << it;


    tree.mOpeningInfo.serialize("./opening/serialtest.bin");
    // for (auto it: tree.mOpeningInfo.zobristPositions) qDebug() << it;



    // tree.serialize("./opening/openings.bin");

    // PGNGame::serializeHeaderData("./opening/openings.headers", database);

    progressBar->deleteLater();
    mOpeningsPathLabel->setText(tr("Current opening database: %1").arg(file));
}

QString SettingsDialog::getOpeningsPath() const {
    return mOpeningsPath;
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
