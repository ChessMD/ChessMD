 #include "databasefilter.h"
#include "ui_databasefilter.h"

#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QQmlContext>

DatabaseFilter::DatabaseFilter(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DatabaseFilter)
{
    ui->setupUi(this);
    ui->WinsOnly->setEnabled(false);

    mChessPosition = new ChessPosition(this);
    
    // starting pos
    QVector<QVector<QString>> startingBoard = {
        {"bR", "bN", "bB", "bQ", "bK", "bB", "bN", "bR"},
        {"bP", "bP", "bP", "bP", "bP", "bP", "bP", "bP"},
        {"", "", "", "", "", "", "", ""},
        {"", "", "", "", "", "", "", ""},
        {"", "", "", "", "", "", "", ""},
        {"", "", "", "", "", "", "", ""},
        {"wP", "wP", "wP", "wP", "wP", "wP", "wP", "wP"},
        {"wR", "wN", "wB", "wQ", "wK", "wB", "wN", "wR"}
    };
    mChessPosition->setBoardData(startingBoard);
    
    setupPositionTab();

    // remove tabs for release since we have unfinished implementations
    delete ui->MaterialTab;
}

void DatabaseFilter::setupPositionTab()
{
    QVBoxLayout* layout = new QVBoxLayout(ui->PositionTab);

    mChessboardWidget = new QQuickWidget(ui->PositionTab);
    mChessboardWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    //qml
    QQmlContext* context = mChessboardWidget->rootContext();
    context->setContextProperty("chessPosition", mChessPosition);
    
    mChessboardWidget->setSource(QUrl("qrc:/chessboardsetup.qml"));
    mChessboardWidget->setMinimumSize(400, 400);
    
    layout->addWidget(mChessboardWidget);

    //connect qml
    connect(mChessPosition, &ChessPosition::boardDataChanged, this, [this]() {
        if (mChessPosition) {
            const QString fen = mChessPosition->positionToFEN();
            const quint64 zobrist = mChessPosition->computeZobrist();
            onPositionChanged(fen, QVariant::fromValue(zobrist));
        }
    });
}

DatabaseFilter::~DatabaseFilter()
{
    delete ui;
}


DatabaseFilter::Filter DatabaseFilter::getNameFilters(){
    // handle name filter values
    _filter.blackFirst = ui->BlackFirst->text();
    _filter.whiteFirst = ui->WhiteFirst->text();
    _filter.blackLast = ui->BlackLast->text();
    _filter.whiteLast = ui->WhiteLast->text();
    _filter.tournament = ui->TournamentEdit->text();
    _filter.annotator = ui->AnnotatorEdit->text();
    _filter.eloMin = ui->EloMin->value();
    _filter.eloMax = ui->EloMax->value();
    _filter.dateMin = ui->DateMin->date();
    _filter.dateMax = ui->DateMax->date();
    _filter.ecoMin = ui->EcoMin->text();
    _filter.ecoMax = ui->EcoMax->text();

    _filter.ignoreColours = ui->IgnoreColour->isChecked();
    _filter.winsOnly = ui->WinsOnly->isChecked();
    _filter.dateCheck = ui->DateCheck->isChecked();
    _filter.ecoCheck = ui->EcoCheck->isChecked();
    _filter.movesCheck = ui->MovesCheck->isChecked();


    

    return _filter;

}

void DatabaseFilter::onPositionChanged(const QString& fen, const QVariant& zobrist)
{
    _filter.zobrist = zobrist.toULongLong();
}
