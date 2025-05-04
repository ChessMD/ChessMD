#ifndef CHESSGAMETABDIALOG_H
#define CHESSGAMETABDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>

class ChessGameTabDialog : public QDialog
{
    Q_OBJECT

public:
    ChessGameTabDialog();
    void addTab(QWidget *page, const QString &label);
    bool TabExist(QString &label);
    void ActiveTabByLabel(QString &label);

private:

    QTabWidget *m_tabWidget;

protected:

    void showEvent(QShowEvent *ev) override;
    void closeEvent(QCloseEvent *event) override;
    QSize sizeHint() const override;

};

#endif // CHESSGAMETABDIALOG_H
