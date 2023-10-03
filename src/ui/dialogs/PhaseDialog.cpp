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

#include "PhaseDialog.h"
#include "Phase.h"
#include "ColorPicker.h"
#include "LineEdit.h"

#include <QtWidgets>


PhaseDialog::PhaseDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags)
{
    setWindowTitle(tr("Create / Modify phase"));

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

    mTauFixedEdit = new LineEdit(this);

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
    
    Phase phase;
    setPhase(phase.toJson());
}

PhaseDialog::~PhaseDialog()
{
}

void PhaseDialog::showAppropriateTauOptions(int typeIndex)
{
    Q_UNUSED(typeIndex)
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

void PhaseDialog::setPhase(const QJsonObject& phase)
{
    mPhase = phase;

    mNameEdit->setText(mPhase.value(STATE_NAME).toString());
    mNameEdit->selectAll();
    mColorPicker->setColor(QColor(mPhase.value(STATE_COLOR_RED).toInt(),
                                  mPhase.value(STATE_COLOR_GREEN).toInt(),
                                  mPhase.value(STATE_COLOR_BLUE).toInt()));
    mTauTypeCombo->setCurrentIndex(mPhase.value(STATE_PHASE_TAU_TYPE).toInt());
    mTauFixedEdit->setText(QString::number(mPhase.value(STATE_PHASE_TAU_FIXED).toDouble()));

    showAppropriateTauOptions(mTauTypeCombo->currentIndex());
}

QJsonObject PhaseDialog::getPhase()
{
    mPhase[STATE_NAME] = mNameEdit->text();
    mPhase[STATE_COLOR_RED] = mColorPicker->getColor().red();
    mPhase[STATE_COLOR_GREEN] = mColorPicker->getColor().green();
    mPhase[STATE_COLOR_BLUE] = mColorPicker->getColor().blue();
    mPhase[STATE_PHASE_TAU_TYPE] = Phase::TauType (mTauTypeCombo->currentIndex());
    mPhase[STATE_PHASE_TAU_FIXED] = mTauFixedEdit->text().toDouble();
    return mPhase;
}

bool PhaseDialog::isValid()
{
    if (mTauTypeCombo->currentIndex() == 1) {
        const double tau = mTauFixedEdit->text().toDouble();
        if (tau < 1) {
            mError = tr("The fixed phase duration must be greater than 1!");
            return false;
        }
    }

    return true;
}

