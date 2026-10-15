/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "PluginGaussForm.h"

#if USE_PLUGIN_GAUSS
#include "PluginGauss.h"

#include <QJsonObject>
#include <QtWidgets>


PluginGaussForm::PluginGaussForm(PluginGauss* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("Gaussian measurement"), parent, flags)
{
    mAverageLab = new QLabel(tr("Mean"), this);
    mErrorLab = new QLabel(tr("Error (sd)"), this);
    mCalibLab = new QLabel(tr("Calibration"), this);

    QDoubleValidator* validator_R = new QDoubleValidator(this);
    validator_R->setLocale(QLocale());

    QDoubleValidator* validator_Rplus = new QDoubleValidator(this);
    validator_Rplus->setBottom(0.0000001);
    validator_Rplus->setLocale(QLocale());

    mAverageEdit = new QLineEdit(this);
    mAverageEdit->setAlignment(Qt::AlignHCenter);  
    mAverageEdit->setValidator(validator_R);
    mAverageEdit->QWidget::setStyleSheet("QLineEdit { border: 0px; }");

    mErrorEdit = new QLineEdit(this);
    mErrorEdit->setAlignment(Qt::AlignHCenter);
    mErrorEdit->setValidator(validator_Rplus);
    mErrorEdit->QWidget::setStyleSheet("QLineEdit { border: 0px; }");

    connect(mErrorEdit, &QLineEdit::textChanged, this, &PluginGaussForm::errorIsValid);
    connect(mAverageEdit, &QLineEdit::textChanged, this, &PluginGaussForm::isValid);
    setTabOrder(mAverageEdit, mErrorEdit);

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
    mAEdit->setStyleSheet("QLineEdit { border: 0px; }");

    mBEdit = new QLineEdit(this);
    mBEdit->setAlignment(Qt::AlignHCenter);
    mBEdit->setStyleSheet("QLineEdit { border: 0px; }");

    mCEdit = new QLineEdit(this);
    mCEdit->setAlignment(Qt::AlignHCenter);
    mCEdit->setStyleSheet("QLineEdit { border: 0px; }");
    setTabOrder(mAEdit, mBEdit);
    setTabOrder(mBEdit, mCEdit);

    mAEdit->setText("0");
    mBEdit->setText("1");
    mCEdit->setText("0");

    connect(mAEdit, &QLineEdit::textChanged, this, &PluginGaussForm::equationIsValid);
    connect(mBEdit, &QLineEdit::textChanged, this, &PluginGaussForm::equationIsValid);

    connect(mAEdit, &QLineEdit::editingFinished, this, &PluginGaussForm::validOK);
    connect(mBEdit, &QLineEdit::editingFinished, this, &PluginGaussForm::validOK);

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
    const QStringList &refCurves = plugin->getRefsNames();
    for (int i = 0; i<refCurves.size(); ++i)
         mCurveCombo->addItem(refCurves[i]);


    connect(mEquationRadio, &QRadioButton::toggled, this, &PluginGaussForm::updateVisibleElements);
    connect(mNoneRadio, &QRadioButton::toggled, this, &PluginGaussForm::updateVisibleElements);
    connect(mCurveRadio, &QRadioButton::toggled, this, &PluginGaussForm::updateVisibleElements);

    connect(mEquationRadio, &QRadioButton::toggled, this, &PluginGaussForm::validOK);
    connect(mNoneRadio, &QRadioButton::toggled, this, &PluginGaussForm::validOK);
    connect(mCurveRadio, &QRadioButton::toggled, this, &PluginGaussForm::validOK);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 3, 0, 0);
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
        emit PluginFormAbstract::OkEnabled(true );
        
    } else {
        const QLocale locale = QLocale();
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
    const QLocale locale = QLocale();

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
    (void) str;
    emit PluginFormAbstract::OkEnabled(  mErrorEdit->hasAcceptableInput());
}

void PluginGaussForm::equationIsValid()
{
    if (mEquationRadio->isChecked()) {
        bool oka,okb;
        const QLocale locale;
        const double a = locale.toDouble(mAEdit->text(), &oka);
        if(a == 0)
            oka = false;

        const double b = locale.toDouble(mBEdit->text(), &okb);
        if (b == 0)
            okb = false;

        emit PluginFormAbstract::OkEnabled(oka || okb);
    }
    else emit PluginFormAbstract::OkEnabled(true);
}

void PluginGaussForm::validOK()
{
    bool ok = mErrorEdit->hasAcceptableInput() &&  mAverageEdit->hasAcceptableInput();
    if (ok && mEquationRadio->isChecked()) {
        bool oka, okb;
        const QLocale locale;
        const double a = locale.toDouble(mAEdit->text(),&oka);
        if(a == 0)
            oka = false;

        const double b = locale.toDouble(mBEdit->text(),&okb);
        if (b == 0)
            okb = false;

        ok = ok && (oka || okb);
    }

    emit PluginFormAbstract::OkEnabled(ok);

}

bool PluginGaussForm::isValid()
{
    return true;
}

#endif
