/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2020

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

#include "PluginGaussForm.h"
#if USE_PLUGIN_GAUSS

#include "PluginGauss.h"
#include "Label.h"
#include "LineEdit.h"
#include "RadioButton.h"
#include "AppSettings.h"

#include <QJsonObject>
#include <QtWidgets>


PluginGaussForm::PluginGaussForm(PluginGauss* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("Gaussian measurement"), parent, flags)
{
   // setFont(AppSettings::font());

    mAverageLab = new QLabel(tr("Mean"), this);
    mErrorLab = new QLabel(tr("Error (sd)"), this);
    mCalibLab = new QLabel(tr("Calibration"), this);

    mAverageEdit = new QLineEdit(this);
    mAverageEdit->setAlignment(Qt::AlignHCenter);

    mErrorEdit = new QLineEdit(this);
    mErrorEdit->setAlignment(Qt::AlignHCenter);

    connect(mErrorEdit, &QLineEdit::textChanged, this, &PluginGaussForm::errorIsValid);

    mAverageEdit->setText("0");
    mErrorEdit->setText("50");

    mEqWidget = new QWidget(this);

    mEq1Lab = new QLabel("g(t) =", this);
    mEq2Lab = new QLabel("t^2 +", this);
    mEq3Lab = new QLabel("t +", this);
    mEq1Lab->setAlignment(Qt::AlignCenter);
    mEq2Lab->setAlignment(Qt::AlignCenter);
    mEq3Lab->setAlignment(Qt::AlignCenter);

    mAEdit = new QLineEdit(this);
    mAEdit->setAlignment(Qt::AlignHCenter);
    mBEdit = new QLineEdit(this);
    mBEdit->setAlignment(Qt::AlignHCenter);
    mCEdit = new QLineEdit(this);
    mCEdit->setAlignment(Qt::AlignHCenter);

    mAEdit->setText("0");
    mBEdit->setText("1");
    mCEdit->setText("0");

    connect(mAEdit, &QLineEdit::textChanged, this, &PluginGaussForm::equationIsValid);
    connect(mBEdit, &QLineEdit::textChanged, this, &PluginGaussForm::equationIsValid);

    QHBoxLayout* eqLayout = new QHBoxLayout(this);
    eqLayout->setContentsMargins(0, 0, 0, 0);
    eqLayout->QLayout::addWidget(mEq1Lab);
    eqLayout->QLayout::addWidget(mAEdit);
    eqLayout->QLayout::addWidget(mEq2Lab);
    eqLayout->QLayout::addWidget(mBEdit);
    eqLayout->QLayout::addWidget(mEq3Lab);
    eqLayout->QLayout::addWidget(mCEdit);

    mEqWidget->setLayout(eqLayout);

    mNoneRadio = new QRadioButton(tr("None"), this);
    mEquationRadio = new QRadioButton(tr("Build your equation"), this);
    mCurveRadio = new QRadioButton(tr("Use custom curve file"), this);

    mNoneRadio->setChecked(true);

    mCurveCombo = new QComboBox(this);
    QStringList refCurves = plugin->getRefsNames();
    for (int i = 0; i<refCurves.size(); ++i)
         mCurveCombo->addItem(refCurves[i]);


    connect(mEquationRadio, &QRadioButton::toggled, this, &PluginGaussForm::updateVisibleElements);
    connect(mNoneRadio, &QRadioButton::toggled, this, &PluginGaussForm::updateVisibleElements);
    connect(mEquationRadio, &QRadioButton::toggled, this, &PluginGaussForm::updateVisibleElements);

    connect(mEquationRadio, &QRadioButton::toggled, this, &PluginGaussForm::equationIsValid);
    connect(mNoneRadio, &QRadioButton::toggled, this, &PluginGaussForm::equationIsValid);
    connect(mEquationRadio, &QRadioButton::toggled, this, &PluginGaussForm::equationIsValid);



    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 5, 0, 0);
    grid->addWidget(mAverageLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAverageEdit, 0, 1);

    grid->addWidget(mErrorLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mErrorEdit, 1, 1);

    grid->addWidget(mCalibLab, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mNoneRadio, 2, 1);
    grid->addWidget(mEquationRadio, 3, 1);
    grid->addWidget(mCurveRadio, 4, 1);

    grid->addWidget(mEqWidget, 5, 1);
    grid->addWidget(mCurveCombo, 5, 1);

    setLayout(grid);

    updateVisibleElements();
}

PluginGaussForm::~PluginGaussForm()
{

}

void PluginGaussForm::setData(const QJsonObject& data, bool isCombined)
{
    mAverageEdit->setEnabled(!isCombined);
    mErrorEdit->setEnabled(!isCombined);
    mAEdit->setEnabled(!isCombined);
    mBEdit->setEnabled(!isCombined);
    mCEdit->setEnabled(!isCombined);
    
    mCurveRadio->setEnabled(!isCombined);
    mEquationRadio->setEnabled(!isCombined);
    mNoneRadio->setEnabled(!isCombined);
   
    
    if ( isCombined) {
        mAverageEdit->setText("Combined data");
        mErrorEdit->setText("Combined data");
        
    } else {
        const QLocale locale=QLocale();
        const double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
        const double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();
        const double a = data.value(DATE_GAUSS_A_STR).toDouble();
        const double b = data.value(DATE_GAUSS_B_STR).toDouble();
        const double c = data.value(DATE_GAUSS_C_STR).toDouble();
        const QString mode = data.value(DATE_GAUSS_MODE_STR).toString();
        const QString curve = data.value(DATE_GAUSS_CURVE_STR).toString();

        mAverageEdit->setText(locale.toString(age));
        mErrorEdit->setText(locale.toString(error));
        mAEdit->setText(locale.toString(a));
        mBEdit->setText(locale.toString(b));
        mCEdit->setText(locale.toString(c));

        mCurveRadio->setChecked(mode == DATE_GAUSS_MODE_CURVE);
        mEquationRadio->setChecked(mode == DATE_GAUSS_MODE_EQ);
        mNoneRadio->setChecked(mode == DATE_GAUSS_MODE_NONE);

        mCurveCombo->setCurrentText(curve);
        updateVisibleElements();
    }
}

void PluginGaussForm::updateVisibleElements()
{
    mCurveCombo->setVisible(mCurveRadio->isChecked());
    mEqWidget->setVisible(mEquationRadio->isChecked());
    mEqWidget->adjustSize();
    adjustSize();
    emit sizeChanged();
}

QJsonObject PluginGaussForm::getData()
{
    QJsonObject data;
    const QLocale locale=QLocale();

    const double age = locale.toDouble(mAverageEdit->text());
    const double error = locale.toDouble(mErrorEdit->text());
    const double a = locale.toDouble(mAEdit->text());
    const double b = locale.toDouble(mBEdit->text());
    const double c = locale.toDouble(mCEdit->text());

    QString mode = "";
    if(mCurveRadio->isChecked()) mode = DATE_GAUSS_MODE_CURVE;
    else if(mEquationRadio->isChecked()) mode = DATE_GAUSS_MODE_EQ;
    else if(mNoneRadio->isChecked()) mode = DATE_GAUSS_MODE_NONE;

    const QString curve = mCurveCombo->currentText();

    data.insert(DATE_GAUSS_AGE_STR, age);
    data.insert(DATE_GAUSS_ERROR_STR, error);
    data.insert(DATE_GAUSS_A_STR, a);
    data.insert(DATE_GAUSS_B_STR, b);
    data.insert(DATE_GAUSS_C_STR, c);
    data.insert(DATE_GAUSS_MODE_STR, mode);
    data.insert(DATE_GAUSS_CURVE_STR, curve);

    return data;
}

void PluginGaussForm::errorIsValid(QString str)
{
    bool ok;
    const QLocale locale;
    const double value = locale.toDouble(str,&ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );
}

void PluginGaussForm::equationIsValid()
{
    if (mEquationRadio->isChecked()) {
        bool oka,okb;
        const QLocale locale;
        const double a = locale.toDouble(mAEdit->text(),&oka);
        if(a == 0)
            oka = false;

        const double b = locale.toDouble(mBEdit->text(),&okb);
        if (b == 0)
            okb = false;

        emit PluginFormAbstract::OkEnabled(oka || okb);
    }
    else emit PluginFormAbstract::OkEnabled(true);
}

bool PluginGaussForm::isValid()
{
    return true;
}


#endif
