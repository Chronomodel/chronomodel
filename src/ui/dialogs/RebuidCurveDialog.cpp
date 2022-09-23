#include "RebuidCurveDialog.h"
//#include "QtWidgets/qcombobox.h"

#include <QHBoxLayout>
#include <QPushButton>

RebuidCurveDialog::RebuidCurveDialog(QWidget *parent)
    : QDialog{parent}
{
    setWindowTitle(tr("export custom curve and map"));

    curveCB = new QCheckBox("curve") ;
    curveCB->setChecked(true);
    connect(curveCB, static_cast<void (QCheckBox::*)(bool)> (&QCheckBox::toggled), this, &RebuidCurveDialog:: updateOptions);

    mapCB  = new QCheckBox("map") ;
    connect(mapCB, static_cast<void (QCheckBox::*)(bool)> (&QCheckBox::toggled), this, &RebuidCurveDialog:: updateOptions);


    XspinBox = new QSpinBox();
    XspinBox->setRange(1, 10000);
    XspinBox->setSingleStep(1);
    XspinBox->setValue(300);
    XspinBox->setToolTip(tr("Enter value of the grid on time axe"));

    YspinBox = new QSpinBox();
    YspinBox->setRange(1, 10000);
    YspinBox->setSingleStep(1);
    YspinBox->setValue(300);
    YspinBox->setToolTip(tr("Enter value of the grid on G axe"));
    YspinBox->setEnabled(false);


    YminEdit = new QLineEdit("-100");
    connect(YminEdit, &QLineEdit::textChanged, this, &RebuidCurveDialog::YMinIsValid);

    YmaxEdit = new QLineEdit("100");
    connect(YmaxEdit, &QLineEdit::textChanged, this, &RebuidCurveDialog::YMaxIsValid);

    buttonBox = new QDialogButtonBox(Qt::Horizontal);
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &RebuidCurveDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &RebuidCurveDialog::reject);

    mCompo = new QComboBox(this);
   /* mCompo->addItem(tr("X"));
    mCompo->addItem(tr("Y"));
    mCompo->addItem(tr("Z"));
*/

    QGridLayout *midLayout = new QGridLayout;
    midLayout->addWidget(new QLabel(tr("Composante")), 0, 0);
    midLayout->addWidget(mCompo, 0, 1);

    midLayout->addWidget(new QLabel(tr("Grid Length")), 1, 1);
    midLayout->addWidget(curveCB, 2, 0);
    midLayout->addWidget(XspinBox, 2, 1);
    midLayout->addWidget(mapCB, 3, 0);
    midLayout->addWidget(YspinBox, 3, 1);
    midLayout->addWidget(new QLabel(tr("Y min")), 4, 0);
    midLayout->addWidget(new QLabel(tr("Y max")), 4, 1);
    midLayout->addWidget(YminEdit, 5, 0);
    midLayout->addWidget(YmaxEdit, 5, 1);

    QBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    mainLayout->addLayout(midLayout);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    updateOptions();

}

int RebuidCurveDialog::getResult() const
{
    if (curveCB->isChecked())
        return 0;
    else
        return 1;
}

int RebuidCurveDialog::getXSpinResult() const
{
    return XspinBox->value();
}

int RebuidCurveDialog::getYSpinResult() const
{
    return XspinBox->value();
}

void RebuidCurveDialog::YMinIsValid(QString str)
{
    bool ok;
    QLocale locale;
    locale.toDouble(str, &ok);
    YMinOK = ok;

    emit OkEnabled(ok);
}
double RebuidCurveDialog::getYMin() const
{
    bool ok;
    QLocale locale;
    return locale.toDouble(YminEdit->text(), &ok);

}

void RebuidCurveDialog::YMaxIsValid(QString str)
{
    bool ok;
    QLocale locale;
    locale.toDouble(str, &ok);
    YMaxOK = ok;
    emit OkEnabled(ok);
}
double RebuidCurveDialog::getYMax() const
{
    bool ok;
    QLocale locale;
    return locale.toDouble(YmaxEdit->text(), &ok);
}

void RebuidCurveDialog::setOkEnabled()
{
    const bool isValid = (mapCB->isChecked() && YMinOK && YMaxOK) || !mapCB->isChecked();
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
}

void RebuidCurveDialog::updateOptions()
{
    YminEdit->setEnabled(mapCB->isChecked());
    YmaxEdit->setEnabled(mapCB->isChecked());
    YspinBox->setEnabled(mapCB->isChecked());
}

QString RebuidCurveDialog::compo()
{
    return mCompo->currentText();
}

void RebuidCurveDialog::setCompo(QStringList &list)
{
    mCompo->clear();
    mCompo->addItems(list);
}
