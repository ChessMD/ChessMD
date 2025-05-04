#include "chessgametabdialog.h"

ChessGameTabDialog::ChessGameTabDialog()
{
    m_tabWidget = new QTabWidget;

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_tabWidget);

    setLayout(mainLayout);

    setWindowTitle(tr("Chess Games"));

    setMinimumSize(1024,768);
}

void ChessGameTabDialog::addTab(QWidget *page, const QString &label)
{
    int  index = m_tabWidget->addTab(page, label);

    m_tabWidget->setCurrentIndex(index);
}

bool ChessGameTabDialog::isTabExist(QString &label)
{
    for (int i = 0; i < m_tabWidget->count(); i++) {
        if (m_tabWidget->tabText(i) == label)
            return true;
    }

    return false;
}

void ChessGameTabDialog::ActiveTabByLabel(QString &label)
{
    for (int i = 0; i < m_tabWidget->count(); i++) {
        if (m_tabWidget->tabText(i) == label) {
            m_tabWidget->setCurrentIndex(i);
            break;
        }
    }
}

void ChessGameTabDialog::closeEvent(QCloseEvent *event)
{
    m_tabWidget->clear();
}

void ChessGameTabDialog::showEvent(QShowEvent *ev)
{
    QDialog::showEvent(ev);
    setMinimumSize(0,0);
}

QSize ChessGameTabDialog::sizeHint() const
{
    return m_tabWidget->size();
}
