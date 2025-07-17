#include "settingsdialog.h"
#include "streamparser.h"
#include "openingviewer.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QProgressBar>
#include <QApplication>
#include <fstream>


SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent), mOpeningsPath("")
{
    setWindowTitle(tr("Settings"));
    resize(480, 260);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    mCategoryList = new QListWidget(this);
    mCategoryList->addItem(tr("Openings"));
    mCategoryList->setFixedWidth(120);
    mainLayout->addWidget(mCategoryList);

    mStackedWidget = new QStackedWidget(this);
    QWidget* openingsPage = new QWidget(this);
    QVBoxLayout* openingsLayout = new QVBoxLayout(openingsPage);
    mOpeningsPathLabel = new QLabel(tr("Current opening database: None"), openingsPage);
    QPushButton* loadPgnBtn = new QPushButton(tr("Load PGN..."), openingsPage);
    openingsLayout->addWidget(mOpeningsPathLabel);
    openingsLayout->addWidget(loadPgnBtn);
    openingsLayout->addStretch();
    mStackedWidget->addWidget(openingsPage);
    mainLayout->addWidget(mStackedWidget);

    connect(mCategoryList, &QListWidget::currentRowChanged, mStackedWidget, &QStackedWidget::setCurrentIndex);
    mCategoryList->setCurrentRow(0);
    connect(loadPgnBtn, &QPushButton::clicked, this, &SettingsDialog::onLoadPgnClicked);
}

void SettingsDialog::onLoadPgnClicked() {
    QString file = QFileDialog::getOpenFileName(this, tr("Select a chess PGN file"), QString(), tr("PGN files (*.pgn)"));
    if (!file.isEmpty()) {
        mOpeningsPath = file;
        mOpeningsPathLabel->setText(tr("Processing PGN file..."));

        // progress bar
        QProgressBar* progressBar = new QProgressBar(this);
        QVBoxLayout* openingsLayout = qobject_cast<QVBoxLayout*>(mStackedWidget->currentWidget()->layout());
        openingsLayout->insertWidget(2, progressBar); // Insert before the stretch
        
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

        for (int i = 0; i < database.size(); ++i) {
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
            tree.insertGame(moveCodes);
        }
        
        progressBar->setValue(database.size());
        mOpeningsPathLabel->setText(tr("Serializing database..."));
        QApplication::processEvents();
        
        tree.serialize("./openings.bin");
        
        // Remove progress bar and update final status
        progressBar->deleteLater();
        mOpeningsPathLabel->setText(tr("Current opening database: %1").arg(file));
    }
}

QString SettingsDialog::getOpeningsPath() const {
    return mOpeningsPath;
}
