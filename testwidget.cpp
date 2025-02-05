#include "testwidget.h"

#include <QPushButton>
#include <QHBoxLayout>

testWidget::testWidget(QWidget *parent)
    : QWidget{parent}
{


    auto bbutton = new QPushButton("bbb");

    auto layout = new QHBoxLayout(this);
    layout->addWidget(bbutton);



}
