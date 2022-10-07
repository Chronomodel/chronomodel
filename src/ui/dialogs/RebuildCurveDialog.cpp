/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

Authors :
    Philippe LANOS
    Helori LANOS
    Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "RebuildCurveDialog.h"


#include <QHBoxLayout>
#include <QPushButton>

RebuildCurveDialog::RebuildCurveDialog(QStringList list, std::vector< std::pair<double, double>> *minMax, std::pair<unsigned, unsigned> mapSize, QWidget *parent): QDialog{parent},
    mCompoList(list),
    mYTabMinMax(*minMax)
{
    setWindowTitle(tr("export custom curve and map"));

    curveCB = new QCheckBox("curve") ;
    curveCB->setChecked(true);
    connect(curveCB, static_cast<void (QCheckBox::*)(bool)> (&QCheckBox::toggled), this, &RebuildCurveDialog:: updateOptions);

    mapCB  = new QCheckBox("map") ;
    connect(mapCB, static_cast<void (QCheckBox::*)(bool)> (&QCheckBox::toggled), this, &RebuildCurveDialog:: updateOptions);


    XspinBox = new QSpinBox();
    XspinBox->setRange(5, 10000);
    XspinBox->setSingleStep(1);
    XspinBox->setValue(mapSize.first);
    XspinBox->setToolTip(tr("Enter value of the grid on time axe"));

    YspinBox = new QSpinBox();
    YspinBox->setRange(5, 10000);
    YspinBox->setSingleStep(1);
    YspinBox->setValue(mapSize.second);
    YspinBox->setToolTip(tr("Enter value of the grid on G axe"));
    YspinBox->setEnabled(false);


    Y1minEdit = new QLineEdit(QLocale().toString(mYTabMinMax[0].first));
    connect(Y1minEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y1MinIsValid);
    Y1maxEdit = new QLineEdit(QLocale().toString(mYTabMinMax[0].second));
    connect(Y1maxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y1MaxIsValid);


    Y2minEdit = new QLineEdit(QLocale().toString(mYTabMinMax[1].first));
    connect(Y2minEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y2MinIsValid);
    Y2maxEdit = new QLineEdit(QLocale().toString(mYTabMinMax[2].second));
    connect(Y2maxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y2MaxIsValid);


    Y3minEdit = new QLineEdit(QLocale().toString(mYTabMinMax[2].first));
    connect(Y3minEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y3MinIsValid);
    Y3maxEdit = new QLineEdit(QLocale().toString(mYTabMinMax[2].second));
    connect(Y3maxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y3MaxIsValid);

    buttonBox = new QDialogButtonBox(Qt::Horizontal);
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &RebuildCurveDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &RebuildCurveDialog::reject);


    QGridLayout *midLayout = new QGridLayout;

    midLayout->addWidget(new QLabel(tr("Grid Length")), 0, 1);

    midLayout->addWidget(curveCB, 2, 0);
    midLayout->addWidget(new QLabel(tr("time")), 2, 1);
    midLayout->addWidget(XspinBox, 2, 2);

    midLayout->addWidget(mapCB, 3, 0);
    midLayout->addWidget(new QLabel(tr("value")), 3, 1);
    midLayout->addWidget(YspinBox, 3, 2);

    QString str = mCompoList[0] + " " + tr("min");
    midLayout->addWidget(new QLabel(str), 4, 0);
    str = mCompoList[0] + " " + tr("max");
    midLayout->addWidget(new QLabel(str), 4, 1);
    midLayout->addWidget(Y1minEdit, 5, 0);
    midLayout->addWidget(Y1maxEdit, 5, 1);

    if (mCompoList.size() > 1) {
        str = mCompoList[1] + " " + tr("min");
        midLayout->addWidget(new QLabel(str), 6, 0);

        str = mCompoList[1] + " " + tr("max");
        midLayout->addWidget(new QLabel(str), 6, 1);
        midLayout->addWidget(Y2minEdit, 7, 0);
        midLayout->addWidget(Y2maxEdit, 7, 1);
    }
    if (mCompoList.size() > 2) {
        str = mCompoList[2] + " " + tr("min");
        midLayout->addWidget(new QLabel(str), 8, 0);

        str = mCompoList[2] + " " + tr("max");
        midLayout->addWidget(new QLabel(str), 8, 1);
        midLayout->addWidget(Y3minEdit, 9, 0);
        midLayout->addWidget(Y3maxEdit, 9, 1);
    }
    QBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    mainLayout->addLayout(midLayout);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    updateOptions();

}

int RebuildCurveDialog::getResult() const
{
    if (curveCB->isChecked())
        return 0;
    else
        return 1;
}

int RebuildCurveDialog::getXSpinResult() const
{
    return XspinBox->value();
}

int RebuildCurveDialog::getYSpinResult() const
{
    return YspinBox->value();
}


// Y1
void RebuildCurveDialog::Y1MinIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double tmp =locale.toDouble(str, &ok);
    Y1MinOK  = ok && tmp < mYTabMinMax[0].second;
    if (Y1MinOK)
        mYTabMinMax[0].first = tmp;

    emit OkEnabled(Y1MinOK);
}

void RebuildCurveDialog::Y1MaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y1MaxOK  = ok && tmp > mYTabMinMax[0].first;
    if (Y1MaxOK)
        mYTabMinMax[0].second = tmp;

    emit OkEnabled(Y1MaxOK);
}


// Y2
void RebuildCurveDialog::Y2MinIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y2MinOK  = ok && tmp < mYTabMinMax[1].second;
    if (Y2MinOK)
        mYTabMinMax[1].first = tmp;

    emit OkEnabled(Y2MinOK);
}

void RebuildCurveDialog::Y2MaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y2MaxOK = ok && tmp > mYTabMinMax[1].first;
    if (Y2MaxOK)
        mYTabMinMax[1].second = tmp;

    emit OkEnabled(Y2MaxOK);
}


// Y3
void RebuildCurveDialog::Y3MinIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y3MinOK  = ok && tmp < mYTabMinMax[2].second;
    if (Y3MinOK)
        mYTabMinMax[2].first = tmp;

    emit OkEnabled(Y3MinOK);
}

void RebuildCurveDialog::Y3MaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y3MaxOK = ok && tmp>mYTabMinMax[2].first;
    if (Y3MaxOK)
        mYTabMinMax[2].second = tmp;

    emit OkEnabled(Y3MaxOK);
}


void RebuildCurveDialog::setOkEnabled()
{
    const bool isValid = (mapCB->isChecked() && Y1MinOK && Y1MaxOK
                          && Y2MinOK && Y2MaxOK
                          && Y3MinOK && Y3MaxOK) || !mapCB->isChecked();
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
}

void RebuildCurveDialog::updateOptions()
{
    Y1minEdit->setEnabled(mapCB->isChecked());
    Y1maxEdit->setEnabled(mapCB->isChecked());
    YspinBox->setEnabled(mapCB->isChecked());

    Y2minEdit->setVisible(mCompoList.size() > 1);
    Y2maxEdit->setVisible(mCompoList.size() > 1);
    if (mCompoList.size() > 1) {
        Y2minEdit->setEnabled(mapCB->isChecked());
        Y2maxEdit->setEnabled(mapCB->isChecked());
    }

    Y3minEdit->setVisible(mCompoList.size() > 2);
    Y3maxEdit->setVisible(mCompoList.size() > 2);
    if (mCompoList.size() > 2) {
        Y3minEdit->setEnabled(mapCB->isChecked());
        Y3maxEdit->setEnabled(mapCB->isChecked());
    }

}

void RebuildCurveDialog::setCompoList(QStringList &list)
{
    mCompoList = list;
}

std::pair<unsigned, unsigned> RebuildCurveDialog::getMapSize() const
{
    return std::pair<unsigned, unsigned>{XspinBox->value(), YspinBox->value()};
}
