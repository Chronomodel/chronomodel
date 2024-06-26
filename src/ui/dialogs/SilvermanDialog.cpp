/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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
#include "SilvermanDialog.h"

#include <QBoxLayout>
#include <QtWidgets>


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

    if (param->lambda_fixed)
        mLambdaTypeInput->setCurrentIndex(1);
    else
        mLambdaTypeInput->setCurrentIndex(0);
    connect(mLambdaTypeInput, &QComboBox::currentIndexChanged, this, &SilvermanDialog::updateLayout);

    mLambdaLabel = new QLabel(tr("Smoothing Value 10E"), this);
    mLambdaInput = new QLineEdit(this);
    mLambdaInput->setText(QString::number(param->log_lambda_value));

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SilvermanDialog::memo);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SilvermanDialog::accept);

    connect(buttonBox, &QDialogButtonBox::rejected, this, &SilvermanDialog::reject);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SilvermanDialog::close);

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
    vlayout->addWidget(buttonBox);
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
    mParam->lambda_fixed = mLambdaTypeInput->currentIndex() == 1;
    mParam->log_lambda_value = locale().toDouble(mLambdaInput->text());
    mParam->use_error_measure = mUseErrMesureInput->isChecked();
    mParam->force_positive_curve = false;
    close();
}

void SilvermanDialog::updateLayout()
{
    mLambdaInput->setVisible(mLambdaTypeInput->currentIndex() == 1);
    mLambdaLabel->setVisible(mLambdaTypeInput->currentIndex() == 1);
}
