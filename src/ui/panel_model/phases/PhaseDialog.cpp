#include "PhaseDialog.h"
#include "Phase.h"
#include "ColorPicker.h"
#include "LineEdit.h"
#include "Label.h"
#include "Button.h"
#include <QtWidgets>


PhaseDialog::PhaseDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mMargin(5),
mLineH(20),
mButH(25),
mButW(80)
{
    setWindowTitle(tr("Create / Modify phase"));
    
    mNameLab = new Label(tr("Phase name") + " :", this);
    mColorLab = new Label(tr("Phase color") + " :", this);
    mTauTypeLab = new Label(tr("Phase duration") + " :", this);
    mTauMinLab = new Label(tr("Value min") + " :", this);
    mTauMaxLab = new Label(tr("Value max") + " :", this);
    
    mNameEdit = new LineEdit(this);
    mColorPicker = new ColorPicker(QColor(), this);
    
    mTauTypeCombo = new QComboBox(this);
    mTauTypeCombo->addItem(tr("Unknown"));
    mTauTypeCombo->addItem(tr("Range (uniform)"));
    
    mTauMinEdit = new LineEdit(this);
    mTauMaxEdit = new LineEdit(this);
    
    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    connect(mTauTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showAppropriateTauOptions(int)));
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    
    mComboH = mTauTypeCombo->sizeHint().height();
    
    setFixedSize(400, mComboH + 2*mLineH + 5*mMargin + mButH);
    
    Phase phase;
    setPhase(phase.toJson());
}

PhaseDialog::~PhaseDialog()
{
    
}

void PhaseDialog::showAppropriateTauOptions(int typeIndex)
{
    Q_UNUSED(typeIndex)
    Phase::TauType type = (Phase::TauType) mTauTypeCombo->currentIndex();
    switch(type)
    {
        case Phase::eTauUnknown:
        {
            mTauMinLab->setVisible(false);
            mTauMaxLab->setVisible(false);
            mTauMinEdit->setVisible(false);
            mTauMaxEdit->setVisible(false);
            
            setFixedHeight(mComboH + 2*mLineH + 5*mMargin + mButH);
            
            break;
        }
        case Phase::eTauRange:
        {
            mTauMinLab->setVisible(true);
            mTauMaxLab->setVisible(true);
            mTauMinEdit->setVisible(true);
            mTauMaxEdit->setVisible(true);
            
            setFixedHeight(mComboH + 4*mLineH + 7*mMargin + mButH);
            
            break;
        }
        default:
            break;
    }
}

void PhaseDialog::setPhase(const QJsonObject& phase)
{
    mNameEdit->setText(phase[STATE_PHASE_NAME].toString());
    mColorPicker->setColor(QColor(phase[STATE_PHASE_RED].toInt(),
                                  phase[STATE_PHASE_GREEN].toInt(),
                                  phase[STATE_PHASE_BLUE].toInt()));
    mTauTypeCombo->setCurrentIndex(phase[STATE_PHASE_TAU_TYPE].toInt());
    mTauMinEdit->setText(phase[STATE_PHASE_TAU_MIN].toString());
    mTauMaxEdit->setText(phase[STATE_PHASE_TAU_MAX].toString());
    
    showAppropriateTauOptions(mTauTypeCombo->currentIndex());
}

QJsonObject PhaseDialog::getPhase() const
{
    Phase phase;
    phase.mName = mNameEdit->text();
    phase.mColor = mColorPicker->getColor();
    phase.mTauType = (Phase::TauType) mTauTypeCombo->currentIndex();
    phase.mTauMin = mTauMinEdit->text().toFloat();
    phase.mTauMax = mTauMaxEdit->text().toFloat();
    return phase.toJson();
}

void PhaseDialog::resizeEvent(QResizeEvent* event)
{
    int w1 = 100;
    int w2 = width() - w1 - 3*mMargin;
    
    mNameLab->setGeometry(mMargin, mMargin, w1, mLineH);
    mColorLab->setGeometry(mMargin, 2*mMargin + mLineH, w1, mLineH);
    mTauTypeLab->setGeometry(mMargin, 3*mMargin + 2*mLineH, w1, mComboH);
    mTauMinLab->setGeometry(mMargin, 4*mMargin + 2*mLineH + mComboH, w1, mLineH);
    mTauMaxLab->setGeometry(mMargin, 5*mMargin + 3*mLineH + mComboH, w1, mLineH);
    
    mNameEdit->setGeometry(2*mMargin + w1, mMargin, w2, mLineH);
    mColorPicker->setGeometry(2*mMargin + w1, 2*mMargin + mLineH, w2, mLineH);
    mTauTypeCombo->setGeometry(2*mMargin + w1, 3*mMargin + 2*mLineH, w2, mComboH);
    mTauMinEdit->setGeometry(2*mMargin + w1, 4*mMargin + 2*mLineH + mComboH, w2, mLineH);
    mTauMaxEdit->setGeometry(2*mMargin + w1, 5*mMargin + 3*mLineH + mComboH, w2, mLineH);
    
    mOkBut->setGeometry(width() - 2*mMargin - 2*mButW, height() - mMargin - mButH, mButW, mButH);
    mCancelBut->setGeometry(width() - mMargin - mButW, height() - mMargin - mButH, mButW, mButH);
}