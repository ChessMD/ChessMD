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
    QLabel* info = new QLabel(tr("Warning! In v1.0-beta, 1 GB of memory (RAM) is required for every 3 MB of a PGN database."), openingsPage);
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
    mThemeComboBox->setCurrentIndex(0);
    
    QLabel* themeInfo = new QLabel(tr("Theme changes will be applied when you restart the application."), themePage);
    themeInfo->setStyleSheet("color: #666; font-size: 11px;"); //hcc
    
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
        file_name = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), QString(), tr("Executable files (*.exe)"));
    } else {
        file_name = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), QString(), tr("All files (*)"));
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
    if (!file.isEmpty()) {
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
            
            // every 100 games update bar
            if (i % 100 == 0) {
                progressBar->setValue(i);
                QApplication::processEvents();
            }
            
            if(!game.isParsed){
                parseBodyText(game.bodyText, game.rootMove);
                game.isParsed = true;
            }
            QVector<quint16> moveCodes;

            QSharedPointer<NotationMove> move = game.rootMove;
            while(!move->m_nextMoves.isEmpty()){
                move = move->m_nextMoves.front();
                moveCodes.push_back(OpeningViewer::encodeMove(move->lanText));
            }

            GameResult result = UNKNOWN;
            if (game.result == "1-0") result = WHITE_WIN;
            else if (game.result == "0-1") result = BLACK_WIN;
            else if (game.result == "1/2-1/2") result = DRAW;
            tree.insertGame(moveCodes, i, result);
        }
        
        progressBar->setValue(database.size());
        mOpeningsPathLabel->setText(tr("Serializing database..."));
        QApplication::processEvents();
        
        tree.serialize("./opening/openings.bin");
        
        PGNGame::serializeHeaderData("./opening/openings.headers", database);
        
        progressBar->deleteLater();
        mOpeningsPathLabel->setText(tr("Current opening database: %1").arg(file));
    }
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
