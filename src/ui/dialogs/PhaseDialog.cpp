#include "PhaseDialog.h"
#include "Phase.h"
#include "ColorPicker.h"
#include "LineEdit.h"
#include "Label.h"
#include "Button.h"
#include <QtWidgets>


PhaseDialog::PhaseDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mMargin(15),
mLineH(20),
mButH(25),
mButW(80)
{
    QPalette Pal(palette());
    
    // set black background
    Pal.setColor(QPalette::Background, Qt::gray);
    this->setAutoFillBackground(true);
    this->setPalette(Pal);
    
    setWindowTitle(tr("Create / Modify phase"));
    
    mNameLab = new Label(tr("Phase name"), this);
    mNameLab->setAlignment(Qt::AlignRight);
    mColorLab = new Label(tr("Phase colour"), this);
    mColorLab->setAlignment(Qt::AlignRight);
    mTauTypeLab = new Label(tr("Max duration"), this);
    mTauTypeLab->setAlignment(Qt::AlignRight);
    mTauFixedLab = new Label(tr("Max duration value") , this);
    mTauFixedLab->setAlignment(Qt::AlignRight);

    mNameEdit = new LineEdit(this);
    
    mColorPicker = new ColorPicker(QColor(), this);
    
    mTauTypeCombo = new QComboBox(this);
    mTauTypeCombo->addItem(tr("Unknown"));
    mTauTypeCombo->addItem(tr("Known"));

    mTauFixedEdit = new LineEdit(this);

    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    mOkBut->setAutoDefault(true);
    
    connect(mTauTypeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PhaseDialog::showAppropriateTauOptions);
    connect(mOkBut, &Button::clicked, this, &PhaseDialog::accept);
    connect(mCancelBut, &Button::clicked, this, &PhaseDialog::reject);
    
    mComboH = mTauTypeCombo->height();
    
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
        case Phase::eTauUnknown:
        {
            mTauFixedLab->setVisible(false);
            mTauFixedEdit->setVisible(false);
            
            setFixedHeight(mComboH + 2*mLineH + 5*mMargin + mButH);
            
            break;
        }
        case Phase::eTauFixed:
        {
            mTauFixedLab->setVisible(true);
            mTauFixedEdit->setVisible(true);
            
            setFixedHeight(mComboH + 3*mLineH + 6*mMargin + mButH);
            
            break;
        }

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
        const int tau = mTauFixedEdit->text().toInt();
        if (tau < 1) {
            mError = tr("Phase fixed duration must be more than 1 !");
            return false;
        }
    }

    return true;
}

void PhaseDialog::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    QFontMetrics fm (font());
    int w1 (0);
    Phase::TauType type = Phase::TauType (mTauTypeCombo->currentIndex());
    switch (type) {
        case Phase::eTauUnknown:
        {
            w1 = qMax(fm.width(mTauTypeLab->text()),qMax(fm.width(mNameLab->text()), fm.width(mColorLab->text())));
            break;
        }
        case Phase::eTauFixed:
        {
            w1 =  fm.width(mTauFixedLab->text());
            break;
        }

        default:
            break;
    }

    int w2 = qMax( 200, fm.width(mNameEdit->text())) + 2 * mMargin;

    setFixedWidth( w1 + w2 + 3*mMargin);

    mNameLab->setGeometry(mMargin, mMargin, w1, mLineH);
    mColorLab->setGeometry(mMargin, 2*mMargin + mLineH, w1, mLineH);
    mTauTypeLab->setGeometry(mMargin, 3*mMargin + 2*mLineH, w1, mLineH);
    mTauFixedLab->setGeometry(mMargin, 4*mMargin + 2*mLineH + mLineH, w1, mLineH);
    
    mNameEdit->setGeometry(2*mMargin + w1, mMargin, w2, mLineH);
    mColorPicker->setGeometry(2*mMargin + w1, 2*mMargin + mLineH, w2, mLineH +3);
    //int dy = (mComboH - mTauTypeLab->height())/2;
    mTauTypeCombo->setGeometry(2*mMargin + w1, 3*mMargin + 2*mLineH  - 5, w2, mComboH);
    mTauFixedEdit->setGeometry(2*mMargin + w1, 4*mMargin + 2*mLineH + mLineH, w2, mLineH);

    mOkBut->setGeometry(width() - 2*mMargin - 2*mButW, height() - mMargin - mButH, mButW, mButH);
    mCancelBut->setGeometry(width() - mMargin - mButW, height() - mMargin - mButH, mButW, mButH);

}
