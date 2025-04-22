#include "databaselibrary.h"
#include "ui_databaselibrary.h"

#include <QLabel>
#include <QLineEdit>

DatabaseLibrary::DatabaseLibrary(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DatabaseLibrary)
{
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

    model = new QStandardItemModel(this);

    //examples
    for (int i = 0; i < 20; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setIcon(QIcon(":/resource/img/fileicon.png"));
        item->setText(QString("Database %1").arg(i));
        item->setTextAlignment(Qt::AlignCenter);
        item->setEditable(false);
        model->appendRow(item);
    }

    listView->setModel(model);

    connect(listView, &QListView::doubleClicked, this, &DatabaseLibrary::onDoubleClick);
    connect(ui->AddButton, &QPushButton::clicked, this, &DatabaseLibrary::addDatabase);

    ui->MainLayout->addWidget(listView);
}

DatabaseLibrary::~DatabaseLibrary()
{
    delete ui;
}

void DatabaseLibrary::onDoubleClick(const QModelIndex &index)
{
    QString fileIdentifier = index.data(Qt::DisplayRole).toString();

    emit fileDoubleClicked(fileIdentifier);
}



void DatabaseLibrary::addDatabase(){

    QDialog dialog;
    dialog.setWindowTitle("Enter Name");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *label = new QLabel("Name:");
    QLineEdit *lineEdit = new QLineEdit();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(label);
    layout->addWidget(lineEdit);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    QString dbName = QString("Database %1").arg(model->rowCount());

    if (dialog.exec() == QDialog::Accepted) {
        dbName = lineEdit->text();
        qDebug() << "Entered name:" << dbName;
    }



    QStandardItem *item = new QStandardItem;
    item->setIcon(QIcon(":/resources/img/fileicon.png"));
    item->setText(dbName);
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    model->appendRow(item);

}
