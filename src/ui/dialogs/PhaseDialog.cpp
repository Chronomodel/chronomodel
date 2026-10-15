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

#include "PhaseDialog.h"
#include "Phase.h"
#include "ColorPicker.h"
#include "LineEdit.h"
#include "QtUtilities.h"
#include "StateKeys.h"

#include <QtWidgets>


PhaseDialog::PhaseDialog(QWidget* parent, Qt::WindowFlags flags):
    QDialog(parent, flags)
{
    setWindowTitle(tr("Create / Modify phase"));
    setMouseTracking(true);

    mNameLab = new QLabel(tr("Phase Name"), this);
    mNameLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    mColorLab = new QLabel(tr("Phase Color"), this);
    mColorLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    mTauTypeLab = new QLabel(tr("Max Duration"), this);
    mTauTypeLab->setAlignment(Qt::AlignVCenter |Qt::AlignRight);

    mTauFixedLab = new QLabel(tr("Max Duration Value") , this);
    mTauFixedLab->setAlignment(Qt::AlignVCenter |Qt::AlignRight);

    mNameEdit = new LineEdit(this);

    mColorPicker = new ColorPicker(QColor(), this);

    mTauTypeCombo = new QComboBox(this);
    mTauTypeCombo->addItem(tr("Unknown"));
    mTauTypeCombo->addItem(tr("Known"));
    mTauTypeCombo->addItem(tr("Uniform Span")); //z-only

    QDoubleValidator* RplusValidator = new QDoubleValidator();
    RplusValidator->setBottom(0.000001);

    mTauFixedEdit = new LineEdit(this);
    mTauFixedEdit->setValidator(RplusValidator);
    mTauFixedEdit->setToolTip(tr("The max phase duration must be greater than %1!").arg(QLocale().toString(RplusValidator->bottom())));

    QDialogButtonBox* buttonBox = new QDialogButtonBox();
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
    
    connect(mTauTypeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PhaseDialog::showAppropriateTauOptions);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PhaseDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PhaseDialog::reject);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->addWidget(mNameLab, 0, 0);
    gridLayout->addWidget(mNameEdit, 0, 1);
    gridLayout->addWidget(mColorLab, 1, 0);
    gridLayout->addWidget(mColorPicker, 1, 1);
    gridLayout->addWidget(mTauTypeLab, 2, 0);
    gridLayout->addWidget(mTauTypeCombo, 2, 1);
    gridLayout->addWidget(mTauFixedLab, 3, 0);
    gridLayout->addWidget(mTauFixedEdit, 3, 1);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(gridLayout);
    layout->addWidget(buttonBox);
    setLayout(layout);
    
    // Create empty phase

    _data[STATE_ID] = 0;
    _data[STATE_NAME] = tr("No Phase Name");

    const auto col = randomColor();
    _data[STATE_COLOR_RED] = col.red();
    _data[STATE_COLOR_GREEN] = col.green();
    _data[STATE_COLOR_BLUE] = col.blue();
    _data[STATE_ITEM_X] = 0;
    _data[STATE_ITEM_Y] = 0;
    _data[STATE_PHASE_TAU_TYPE] = 0;
    _data[STATE_PHASE_TAU_FIXED] = 0;
    _data[STATE_PHASE_TAU_MIN] = 0;
    _data[STATE_PHASE_TAU_MAX] = 0;
    _data[STATE_IS_SELECTED] = false;
    _data[STATE_IS_CURRENT] = false;

    mNameEdit->setText(_data.value(STATE_NAME).toString());
    mNameEdit->selectAll();
    mColorPicker->setColor(QColor(_data.value(STATE_COLOR_RED).toInt(),
                                  _data.value(STATE_COLOR_GREEN).toInt(),
                                  _data.value(STATE_COLOR_BLUE).toInt()));

    mTauTypeCombo->setCurrentIndex(_data.value(STATE_PHASE_TAU_TYPE).toInt());
    mTauFixedEdit->setText(QLocale().toString(_data.value(STATE_PHASE_TAU_FIXED).toDouble()));
    showAppropriateTauOptions();
}

PhaseDialog::~PhaseDialog()
{
}

void PhaseDialog::showAppropriateTauOptions()
{
    Phase::TauType type = Phase::TauType (mTauTypeCombo->currentIndex());
    switch (type) {
        case Phase::eZOnly:
        case Phase::eTauUnknown:
            mTauFixedLab->setVisible(false);
            mTauFixedEdit->setVisible(false);
            break;

        case Phase::eTauFixed:
            mTauFixedLab->setVisible(true);
            mTauFixedEdit->setVisible(true);
            break;

        default:
            break;
    }
}

void PhaseDialog::setPhase(QJsonObject phaseObj)
{
    _data = phaseObj;
    mNameEdit->setText(phaseObj.value(STATE_NAME).toString());
    mNameEdit->selectAll();
    mColorPicker->setColor(QColor(phaseObj.value(STATE_COLOR_RED).toInt(),
                                  phaseObj.value(STATE_COLOR_GREEN).toInt(),
                                  phaseObj.value(STATE_COLOR_BLUE).toInt()));
    mTauTypeCombo->setCurrentIndex(phaseObj.value(STATE_PHASE_TAU_TYPE).toInt());
    mTauFixedEdit->setText(QLocale().toString(phaseObj.value(STATE_PHASE_TAU_FIXED).toDouble()));

    showAppropriateTauOptions();
}

QJsonObject PhaseDialog::getPhase()
{
    QJsonObject phaseObj(_data);

    phaseObj[STATE_NAME] = mNameEdit->text();
    phaseObj[STATE_COLOR_RED] = mColorPicker->getColor().red();
    phaseObj[STATE_COLOR_GREEN] = mColorPicker->getColor().green();
    phaseObj[STATE_COLOR_BLUE] = mColorPicker->getColor().blue();
    phaseObj[STATE_PHASE_TAU_TYPE] = Phase::TauType (mTauTypeCombo->currentIndex());
    phaseObj[STATE_PHASE_TAU_FIXED] = QLocale().toDouble(mTauFixedEdit->text());

    return phaseObj;
}

bool PhaseDialog::isValid()
{
    if (mTauTypeCombo->currentIndex() == 1) {
        return mTauFixedEdit->hasAcceptableInput();
    }

    return true;
}

