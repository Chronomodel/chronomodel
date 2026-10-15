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
#include "SilvermanDialog.h"

#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>


SilvermanDialog::SilvermanDialog(SilvermanParam *param, QWidget* parent, Qt::WindowFlags flags): QDialog(parent, flags),
    mParam(param)
{
    setWindowTitle(tr("Cubic Spline Regression"));
    setModal(true);
    mDescriptionLabel = new QLabel(tr("Non-parametric regression curve following the method described by Green & Silverman (1994), Time knots are fixed (no event model). Smoothing parameter, if not fixed, is estimated by cross-validation. Time constraints are not taken into account. Error envelop is estimated by unbiaised residual variance."), this);
    mDescriptionLabel->setAlignment(Qt::AlignCenter);
    mDescriptionLabel->setWordWrap(true);

    mUseErrMesureInput = new QCheckBox("Measurement Err. weighting", this);
    if (param->use_error_measure)
        mUseErrMesureInput->setCheckState(Qt::Checked);
    connect(mUseErrMesureInput, &QCheckBox::clicked, this, &SilvermanDialog::updateLayout);

    mLambdaTypeLabel = new QLabel(tr("Smoothing"), this);
    mLambdaTypeInput = new QComboBox(this);
    mLambdaTypeInput->addItem(tr("Cross-Validation"));
    mLambdaTypeInput->addItem(tr("Fixed"));

#ifdef DEBUG
    mLambdaTypeInput->addItem(tr("Kernel"));
    if (param->lambda_process == SilvermanParam::lambda_type::Kernel)
        mLambdaTypeInput->setCurrentIndex(1);
    else
#endif

    if (param->lambda_process == SilvermanParam::lambda_type::Fixed)
        mLambdaTypeInput->setCurrentIndex(1);
    else
        mLambdaTypeInput->setCurrentIndex(0);


    connect(mLambdaTypeInput, &QComboBox::currentIndexChanged, this, &SilvermanDialog::updateLayout);
    connect(mLambdaTypeInput, &QComboBox::currentIndexChanged,[this](int) { this->validateInputs(); });

    mLambdaLabel = new QLabel(tr("Smoothing Value 10E"), this);
    mLambdaInput = new QLineEdit(this);
    mLambdaInput->setText(QLocale().toString(param->log_lambda_value));

    connect(mLambdaInput, &QLineEdit::textChanged, [this](const QString&) { this->validateInputs(); });

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);


    connect(mButtonBox, &QDialogButtonBox::accepted, this, &SilvermanDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &SilvermanDialog::memo);

    connect(mButtonBox, &QDialogButtonBox::rejected, this, &SilvermanDialog::reject);

    /*connect(buttonBox, &QDialogButtonBox::accepted, this, &SilvermanDialog::memo);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SilvermanDialog::accept);

    connect(buttonBox, &QDialogButtonBox::rejected, this, &SilvermanDialog::reject);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SilvermanDialog::close);
*/

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(5);
    int row = -1;

    grid->addWidget(mLambdaTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLambdaTypeInput, row, 1);

    grid->addWidget(mLambdaLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLambdaInput, row, 1);

    grid->addWidget(mUseErrMesureInput, ++row, 1);

    updateLayout();

    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->addSpacing(20);
    vlayout->addWidget(mDescriptionLabel);
    vlayout->addSpacing(30);
    vlayout->addLayout(grid);
    vlayout->addWidget(mButtonBox);
    vlayout->addStretch();

    QWidget* vlayoutWidget = new QWidget();
    vlayoutWidget->setLayout(vlayout);

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    hlayout->addStretch();
    hlayout->addWidget(vlayoutWidget);

    hlayout->addStretch();

    setLayout(hlayout);
}

SilvermanDialog::~SilvermanDialog()
{
}

void SilvermanDialog::memo()
{
    switch (mLambdaTypeInput->currentIndex()) {
    case 2:
        mParam->lambda_process = SilvermanParam::lambda_type::Kernel;
        break;
    case 1:
        mParam->lambda_process = SilvermanParam::lambda_type::Fixed;
        break;
    case 0:
        mParam->lambda_process = SilvermanParam::lambda_type::Silverman;
        break;
    default:
        break;
    }


    mParam->log_lambda_value = QLocale().toDouble(mLambdaInput->text());
    mParam->use_error_measure = mUseErrMesureInput->isChecked();
    mParam->force_positive_curve = false;
    close();
}

void SilvermanDialog::updateLayout()
{
    mLambdaInput->setVisible(mLambdaTypeInput->currentIndex() == 1);
    mLambdaLabel->setVisible(mLambdaTypeInput->currentIndex() == 1);
}


void SilvermanDialog::validateInputs()
{
    bool enable = true;
    if (mLambdaTypeInput->currentIndex() == 1) {
        bool ok = false;
        QLocale().toDouble(mLambdaInput->text(), &ok);
        enable = ok;
    }
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}
