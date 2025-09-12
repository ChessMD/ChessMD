#ifndef PGNSAVEDIALOG_H
#define PGNSAVEDIALOG_H

#include "pgngame.h"

#include <QDialog>
#include <QObject>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDateEdit>

class PGNSaveDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PGNSaveDialog(QWidget* parent = nullptr);

    void setHeaders(const PGNGame &game);
    void applyTo(PGNGame &game);

private:
    QLineEdit *m_whiteEdit;
    QLineEdit *m_blackEdit;
    QLineEdit *m_eventEdit;
    QCheckBox *m_ecoCheck;
    QLineEdit *m_ecoEdit;
    QCheckBox *m_eloWhiteCheck;
    QSpinBox *m_eloWhiteSpin;
    QCheckBox *m_eloBlackCheck;
    QSpinBox *m_eloBlackSpin;
    QCheckBox *m_roundCheck;
    QSpinBox *m_roundSpin;
    QCheckBox *m_dateCheck;
    QDateEdit *m_dateEdit;
    QButtonGroup*m_resultGroup;
};

#endif // PGNSAVEDIALOG_H
