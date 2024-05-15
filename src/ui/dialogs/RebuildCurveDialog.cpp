/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

RebuildCurveDialog::RebuildCurveDialog(QStringList list, std::vector< std::pair<double, double>> *minMax, std::vector< std::pair<double, double>> *minMaxP, std::pair<unsigned, unsigned> mapSize, QWidget *parent): QDialog{parent},
    mCompoList(list),
    mYTabMinMax(*minMax),
    mYpTabMinMax(*minMaxP)
{
    setWindowTitle(tr("Rescale Curve and its Density"));

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

    // Var. Rate

    Y1pminEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[0].first));
    connect(Y1pminEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y1pMinIsValid);
    Y1pmaxEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[0].second));
    connect(Y1pmaxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y1pMaxIsValid);


    Y2pminEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[1].first));
    connect(Y2pminEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y2pMinIsValid);
    Y2pmaxEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[2].second));
    connect(Y2pmaxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y2pMaxIsValid);


    Y3pminEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[2].first));
    connect(Y3pminEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y3pMinIsValid);
    Y3pmaxEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[2].second));
    connect(Y3pmaxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y3pMaxIsValid);

    buttonBox = new QDialogButtonBox(Qt::Horizontal);
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &RebuildCurveDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &RebuildCurveDialog::reject);

    int ligne = 0;
    QGridLayout *midLayout = new QGridLayout;

    midLayout->addWidget(new QLabel(tr("Grid Setting")), ligne, 0, 1, 2, Qt::AlignCenter);
    ligne++;
    midLayout->addWidget(curveCB, ligne, 0);
    ligne++;
    midLayout->addWidget(new QLabel(tr("Time Step")), ligne, 0);
    midLayout->addWidget(XspinBox, ligne, 1);
    ligne++;

    midLayout->addWidget(mapCB, ligne, 1);
    ligne++;

    midLayout->addWidget(new QLabel(tr("Value Step")), ligne, 0);
    midLayout->addWidget(YspinBox, ligne, 1);
    ligne++;

    QFrame* line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    midLayout->addWidget(line1, ligne, 0, 1, 2);
    ligne++;
    midLayout->addWidget(new QLabel(mCompoList[0] +tr(" Axis Setting")), ligne, 0, 1, 2, Qt::AlignCenter);

    ligne++;
    QString str = mCompoList[0] + " " + tr("min");
    midLayout->addWidget(new QLabel(str), ligne, 0);
    str = mCompoList[0] + " " + tr("max");
    midLayout->addWidget(new QLabel(str), ligne, 1);
    ligne++;
    midLayout->addWidget(Y1minEdit, ligne, 0);
    midLayout->addWidget(Y1maxEdit, ligne, 1);
    ligne++;
    str = mCompoList[0] + " Var. " + tr("min");
    midLayout->addWidget(new QLabel(str), ligne, 0);
    str = mCompoList[0] + " Var. " + tr("max");
    midLayout->addWidget(new QLabel(str), ligne, 1);
    ligne++;
    midLayout->addWidget(Y1pminEdit, ligne, 0);
    midLayout->addWidget(Y1pmaxEdit, ligne, 1);

    if (mCompoList.size() > 1) {
        ligne++;
        QFrame* line2 = new QFrame();
        line2->setFrameShape(QFrame::HLine);
        line2->setFrameShadow(QFrame::Sunken);
        midLayout->addWidget(line2);
        ligne++;
        midLayout->addWidget(new QLabel(mCompoList[1] +tr(" Axis Setting")), ligne, 0, 2, 1, Qt::AlignCenter);

        ligne++;
        str = mCompoList[1] + " " + tr("min");
        midLayout->addWidget(new QLabel(str), ligne, 0);
        str = mCompoList[1] + " " + tr("max");
        midLayout->addWidget(new QLabel(str), ligne, 1);
        ligne++;
        midLayout->addWidget(Y2minEdit, ligne, 0);
        midLayout->addWidget(Y2maxEdit, ligne, 1);
        ligne++;
        str = mCompoList[1] + " Var. " + tr("min");
        midLayout->addWidget(new QLabel(str), ligne, 0);
        str = mCompoList[1] + " Var. " + tr("max");
        midLayout->addWidget(new QLabel(str), ligne, 1);
        ligne++;
        midLayout->addWidget(Y2pminEdit, ligne, 0);
        midLayout->addWidget(Y2pmaxEdit, ligne, 1);
    }
    if (mCompoList.size() > 2) {
        ligne++;
        QFrame* line3 = new QFrame();
        line3->setFrameShape(QFrame::HLine);
        line3->setFrameShadow(QFrame::Sunken);
        midLayout->addWidget(line3);
        ligne++;
        ligne++;
        midLayout->addWidget(new QLabel(mCompoList[2] +tr(" Axis Setting")), ligne, 0, 2, 1, Qt::AlignCenter);

        str = mCompoList[2] + " " + tr("min");
        midLayout->addWidget(new QLabel(str), ligne, 0);
        str = mCompoList[2] + " " + tr("max");
        midLayout->addWidget(new QLabel(str), ligne, 1);
        ligne++;
        midLayout->addWidget(Y3minEdit, ligne, 0);
        midLayout->addWidget(Y3maxEdit, ligne, 1);
        ligne++;
        str = mCompoList[2] + " Var. " + tr("min");
        midLayout->addWidget(new QLabel(str), ligne, 0);
        str = mCompoList[2] + " Var. " + tr("max");
        midLayout->addWidget(new QLabel(str), ligne, 1);
        ligne++;
        midLayout->addWidget(Y3pminEdit, ligne, 0);
        midLayout->addWidget(Y3pmaxEdit, ligne, 1);
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
    double tmp = locale.toDouble(str, &ok);
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

// Y1p
void RebuildCurveDialog::Y1pMinIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double tmp =locale.toDouble(str, &ok);
    Y1pMinOK  = ok && tmp < mYpTabMinMax[0].second;
    if (Y1pMinOK)
        mYpTabMinMax[0].first = tmp;

    emit OkEnabled(Y1pMinOK);
}

void RebuildCurveDialog::Y1pMaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y1pMaxOK  = ok && tmp > mYpTabMinMax[0].first;
    if (Y1pMaxOK)
        mYpTabMinMax[0].second = tmp;

    emit OkEnabled(Y1pMaxOK);
}

// Y2p
void RebuildCurveDialog::Y2pMinIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y2pMinOK  = ok && tmp < mYpTabMinMax[1].second;
    if (Y2pMinOK)
        mYpTabMinMax[1].first = tmp;

    emit OkEnabled(Y2pMinOK);
}

void RebuildCurveDialog::Y2pMaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y2pMaxOK = ok && tmp > mYpTabMinMax[1].first;
    if (Y2pMaxOK)
        mYpTabMinMax[1].second = tmp;

    emit OkEnabled(Y2pMaxOK);
}


// Y3p
void RebuildCurveDialog::Y3pMinIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y3pMinOK  = ok && tmp < mYpTabMinMax[2].second;
    if (Y3pMinOK)
        mYpTabMinMax[2].first = tmp;

    emit OkEnabled(Y3pMinOK);
}

void RebuildCurveDialog::Y3pMaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y3pMaxOK = ok && tmp>mYpTabMinMax[2].first;
    if (Y3pMaxOK)
        mYpTabMinMax[2].second = tmp;

    emit OkEnabled(Y3pMaxOK);
}


void RebuildCurveDialog::setOkEnabled()
{
    const bool isValid = (mapCB->isChecked() && Y1MinOK && Y1MaxOK
                          && Y2MinOK && Y2MaxOK
                          && Y3MinOK && Y3MaxOK
                          && Y1pMinOK && Y1pMaxOK
                          && Y2pMinOK && Y2pMaxOK
                          && Y3pMinOK && Y3pMaxOK) || !mapCB->isChecked();
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
}

void RebuildCurveDialog::updateOptions()
{
    const bool show_map = mapCB->isChecked();

    Y1minEdit->setEnabled(show_map);
    Y1maxEdit->setEnabled(show_map);
    YspinBox->setEnabled(show_map);

    Y2minEdit->setVisible(mCompoList.size() > 1 && show_map);
    Y2maxEdit->setVisible(mCompoList.size() > 1 && show_map);
    if (mCompoList.size() > 1) {
        Y2minEdit->setEnabled(show_map);
        Y2maxEdit->setEnabled(show_map);
    }

    Y3minEdit->setVisible(mCompoList.size() > 2 && show_map);
    Y3maxEdit->setVisible(mCompoList.size() > 2 && show_map);
    if (mCompoList.size() > 2) {
        Y3minEdit->setEnabled(show_map);
        Y3maxEdit->setEnabled(show_map);
    }

    Y1pminEdit->setEnabled(show_map);
    Y1pmaxEdit->setEnabled(show_map);

    Y2pminEdit->setVisible(mCompoList.size() > 1 && show_map);
    Y2pmaxEdit->setVisible(mCompoList.size() > 1 && show_map);
    if (mCompoList.size() > 1) {
        Y2pminEdit->setEnabled(show_map);
        Y2pmaxEdit->setEnabled(show_map);
    }

    Y3pminEdit->setVisible(mCompoList.size() > 2 && show_map);
    Y3pmaxEdit->setVisible(mCompoList.size() > 2 && show_map);
    if (mCompoList.size() > 2) {
        Y3pminEdit->setEnabled(show_map);
        Y3pmaxEdit->setEnabled(show_map);
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
