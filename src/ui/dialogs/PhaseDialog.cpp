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
    mTauFixedEdit->setText(locale().toString(_data.value(STATE_PHASE_TAU_FIXED).toDouble()));

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
    mTauFixedEdit->setText(locale().toString(phaseObj.value(STATE_PHASE_TAU_FIXED).toDouble()));

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
    phaseObj[STATE_PHASE_TAU_FIXED] = locale().toDouble(mTauFixedEdit->text());

    return phaseObj;
}

bool PhaseDialog::isValid()
{
    if (mTauTypeCombo->currentIndex() == 1) {
        return mTauFixedEdit->hasAcceptableInput();
    }

    return true;
}

