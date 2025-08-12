#include "pgnsavedialog.h"
#include "pgngamedata.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>

PGNSaveDialog::PGNSaveDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Save Game Headers"));
    setModal(true);

    auto *form = new QFormLayout;

    m_whiteEdit = new QLineEdit;
    form->addRow(tr("White:"), m_whiteEdit);
    m_blackEdit = new QLineEdit;
    form->addRow(tr("Black:"), m_blackEdit);
    m_eventEdit = new QLineEdit;
    form->addRow(tr("Event:"), m_eventEdit);

    m_ecoCheck = new QCheckBox(tr("ECO code:"));
    m_ecoEdit = new QLineEdit; m_ecoEdit->setMaximumWidth(60);
    {
        auto *h = new QHBoxLayout;
        h->addWidget(m_ecoCheck);
        h->addWidget(m_ecoEdit);
        h->addStretch();
        form->addRow(h);
    }

    m_eloWhiteCheck = new QCheckBox(tr("Elo White:"));
    m_eloWhiteSpin = new QSpinBox; m_eloWhiteSpin->setRange(100, 3000);
    form->addRow(m_eloWhiteCheck, m_eloWhiteSpin);

    m_eloBlackCheck = new QCheckBox(tr("Elo Black:"));
    m_eloBlackSpin = new QSpinBox; m_eloBlackSpin->setRange(100, 3000);
    form->addRow(m_eloBlackCheck, m_eloBlackSpin);

    m_roundSpin = new QSpinBox;
    m_roundSpin->setRange(1, 100);
    m_roundCheck = new QCheckBox(tr("Round:"));

    {
        auto *h = new QHBoxLayout;
        h->addWidget(m_roundCheck);
        h->addWidget(m_roundSpin);
        h->addStretch();
        form->addRow(h);
    }

    m_dateEdit  = new QDateEdit;
    m_dateEdit->setCalendarPopup(true);
    m_dateCheck = new QCheckBox(tr("Date:"));

    {
        auto *h = new QHBoxLayout;
        h->addWidget(m_dateCheck);
        h->addWidget(m_dateEdit);
        h->addStretch();
        form->addRow(h);
    }
    {
        auto *h = new QHBoxLayout;
        m_resultGroup = new QButtonGroup(this);
        auto *r1 = new QRadioButton("1-0");
        auto *r2 = new QRadioButton("½-½");
        auto *r3 = new QRadioButton("0-1");
        auto *r4 = new QRadioButton("*");
        m_resultGroup->addButton(r1, 0);
        m_resultGroup->addButton(r2, 1);
        m_resultGroup->addButton(r3, 2);
        m_resultGroup->addButton(r4, 3);
        r4->setChecked(true);
        h->addWidget(r1);
        h->addWidget(r2);
        h->addWidget(r3);
        h->addWidget(r4);
        h->addStretch();
        form->addRow(tr("Result:"), h);
    }

    connect(m_ecoEdit, &QLineEdit::textChanged, this, [this](const QString&){ m_ecoCheck->setChecked(true); });
    connect(m_eloWhiteSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int){ m_eloWhiteCheck->setChecked(true); });
    connect(m_eloBlackSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int){ m_eloBlackCheck->setChecked(true); });
    connect(m_roundSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int){ m_roundCheck->setChecked(true); });
    connect(m_dateEdit, &QDateEdit::dateChanged, this, [this](const QDate&){ m_dateCheck->setChecked(true); });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *lay = new QVBoxLayout(this);
    lay->addLayout(form);
    lay->addWidget(buttons);
    setLayout(lay);
}

void PGNSaveDialog::setHeaders(const PGNGame &g)
{
    // clear defaults
    m_whiteEdit->clear();
    m_blackEdit->clear();
    m_eventEdit->clear();
    m_ecoCheck->setChecked(false); m_ecoEdit->clear();
    m_eloWhiteCheck->setChecked(false); m_eloWhiteSpin->setValue(100);
    m_eloBlackCheck->setChecked(false); m_eloBlackSpin->setValue(100);
    m_roundSpin->setValue(1);
    m_dateEdit->setDate(QDate::currentDate());
    // default result = "*"
    m_resultGroup->button(3)->setChecked(true);

    // fill from headerInfo
    for (auto &kv : g.headerInfo) {
        const QString &k = kv.first, &v = kv.second;
        if (k == "White") m_whiteEdit->setText(v);
        else if (k == "Black") m_blackEdit->setText(v);
        else if (k == "Event") m_eventEdit->setText(v);
        else if (k == "ECO") {
            m_ecoCheck->setChecked(true);
            m_ecoEdit->setText(v);
        }
        else if (k == "WhiteElo") {
            bool ok; int e = v.toInt(&ok);
            if (ok) { m_eloWhiteCheck->setChecked(true); m_eloWhiteSpin->setValue(e); }
        }
        else if (k == "BlackElo") {
            bool ok; int e = v.toInt(&ok);
            if (ok) { m_eloBlackCheck->setChecked(true); m_eloBlackSpin->setValue(e); }
        }
        else if (k == "Round") {
            bool ok; int r = v.toInt(&ok);
            if (ok) {
                m_roundCheck->setChecked(true);
                m_roundSpin->setValue(r);
            }
        }
        else if (k == "Date") {
            QDate d = QDate::fromString(v, Qt::ISODate);
            if (d.isValid()) {
                m_dateCheck->setChecked(true);
                m_dateEdit->setDate(d);
            }
        }
        else if (k == "Result") {
            if (v == "1-0") m_resultGroup->button(0)->setChecked(true);
            else if (v == "1/2-1/2") m_resultGroup->button(1)->setChecked(true);
            else if (v == "0-1") m_resultGroup->button(2)->setChecked(true);
            else m_resultGroup->button(3)->setChecked(true);
        }
    }
}

void PGNSaveDialog::applyTo(PGNGame &game)
{
    QMap<QString, QString> headerMap;
    for (auto &kv : game.headerInfo)
        headerMap[kv.first] = kv.second;

    auto setOrRemove = [&](const QString &key, const QString &val, bool use) {
        if (use && !val.isEmpty())
            headerMap[key] = val;
        else
            headerMap.remove(key);
    };

    setOrRemove("White", m_whiteEdit->text(), true);
    setOrRemove("Black", m_blackEdit->text(), true);
    setOrRemove("Event", m_eventEdit->text(), true);
    setOrRemove("ECO", m_ecoEdit->text(), m_ecoCheck->isChecked());
    setOrRemove("WhiteElo", QString::number(m_eloWhiteSpin->value()), m_eloWhiteCheck->isChecked());
    setOrRemove("BlackElo", QString::number(m_eloBlackSpin->value()), m_eloBlackCheck->isChecked());
    setOrRemove("Round", QString::number(m_roundSpin->value()), m_roundCheck->isChecked());
    setOrRemove("Date", m_dateEdit->date().toString(Qt::ISODate), m_dateCheck->isChecked());

    // Result
    static const QVector<QString> results = { "1-0", "1/2-1/2", "0-1", "*" };
    int rid = m_resultGroup->checkedId();
    if (rid >= 0 && rid < results.size())
        headerMap["Result"] = results[rid];
    else
        headerMap.remove("Result");

    QVector<QPair<QString,QString>> newInfo;
    QSet<QString> seen;
    for (auto &kv : game.headerInfo) {
        if (headerMap.contains(kv.first)) {
            newInfo.append({kv.first, headerMap[kv.first]});
            seen.insert(kv.first);
        }
    }

    auto appendIfNew = [&](const QString &key) {
        if (!seen.contains(key) && headerMap.contains(key)) {
            newInfo.append({key, headerMap[key]});
            seen.insert(key);
        }
    };

    for (auto &key : {"White","Black","Event","ECO","WhiteElo","BlackElo","Round","Date","Result"}) {
        appendIfNew(key);
    }

    game.headerInfo = newInfo;
}

