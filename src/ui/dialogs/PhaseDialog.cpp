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
    QPalette Pal(palette());
    
    // set black background
    Pal.setColor(QPalette::Background, Qt::gray);
    this->setAutoFillBackground(true);
    this->setPalette(Pal);
    
    setWindowTitle(tr("Create / Modify phase"));
    
    mNameLab = new Label(tr("Phase name") + " :", this);
    mColorLab = new Label(tr("Phase color") + " :", this);
    mTauTypeLab = new Label(tr("Max duration") + " :", this);
    mTauFixedLab = new Label(tr("Max duration value") + " :", this);
    //mTauMinLab = new Label(tr("Lower date") + " :", this);
    //mTauMaxLab = new Label(tr("Upper date") + " :", this);
    
    mNameEdit = new LineEdit(this);
    mNameEdit->setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    mColorPicker = new ColorPicker(QColor(), this);
    
    mTauTypeCombo = new QComboBox(this);
    mTauTypeCombo->addItem(tr("Unknown"));
    mTauTypeCombo->addItem(tr("Known"));
    //mTauTypeCombo->addItem(tr("Range (uniform)"));
    
    mTauFixedEdit = new LineEdit(this);
    //mTauMinEdit = new LineEdit(this);
    //mTauMaxEdit = new LineEdit(this);
    
    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    mOkBut->setAutoDefault(true);
    
    connect(mTauTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showAppropriateTauOptions(int)));
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    
    mComboH = mTauTypeCombo->sizeHint().height();
    
    setFixedWidth(400);
    
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
            mTauFixedLab->setVisible(false);
            mTauFixedEdit->setVisible(false);
            
            /*mTauMinLab->setVisible(false);
            mTauMaxLab->setVisible(false);
            mTauMinEdit->setVisible(false);
            mTauMaxEdit->setVisible(false);
            */
            
            setFixedHeight(mComboH + 2*mLineH + 5*mMargin + mButH);
            
            break;
        }
        case Phase::eTauFixed:
        {
            mTauFixedLab->setVisible(true);
            mTauFixedEdit->setVisible(true);
            
            /*mTauMinLab->setVisible(false);
            mTauMaxLab->setVisible(false);
            mTauMinEdit->setVisible(false);
            mTauMaxEdit->setVisible(false);
            */
            setFixedHeight(mComboH + 3*mLineH + 6*mMargin + mButH);
            
            break;
        }
        /*case Phase::eTauRange:
        {
            mTauFixedLab->setVisible(false);
            mTauFixedEdit->setVisible(false);
            
            mTauMinLab->setVisible(true);
            mTauMaxLab->setVisible(true);
            mTauMinEdit->setVisible(true);
            mTauMaxEdit->setVisible(true);
            
            setFixedHeight(mComboH + 4*mLineH + 7*mMargin + mButH);
            
            break;
        }*/
        default:
            break;
    }
}

void PhaseDialog::setPhase(const QJsonObject& phase)
{
    mPhase = phase;
    
    mNameEdit->setText(mPhase[STATE_NAME].toString());
    mColorPicker->setColor(QColor(mPhase[STATE_COLOR_RED].toInt(),
                                  mPhase[STATE_COLOR_GREEN].toInt(),
                                  mPhase[STATE_COLOR_BLUE].toInt()));
    mTauTypeCombo->setCurrentIndex(mPhase[STATE_PHASE_TAU_TYPE].toInt());
    mTauFixedEdit->setText(QString::number(mPhase[STATE_PHASE_TAU_FIXED].toDouble()));
    //mTauMinEdit->setText(QString::number(mPhase[STATE_PHASE_TAU_MIN].toDouble()));
    //mTauMaxEdit->setText(QString::number(mPhase[STATE_PHASE_TAU_MAX].toDouble()));
    
    showAppropriateTauOptions(mTauTypeCombo->currentIndex());
}

QJsonObject PhaseDialog::getPhase()
{
    mPhase[STATE_NAME] = mNameEdit->text();
    mPhase[STATE_COLOR_RED] = mColorPicker->getColor().red();
    mPhase[STATE_COLOR_GREEN] = mColorPicker->getColor().green();
    mPhase[STATE_COLOR_BLUE] = mColorPicker->getColor().blue();
    mPhase[STATE_PHASE_TAU_TYPE] = (Phase::TauType) mTauTypeCombo->currentIndex();
    mPhase[STATE_PHASE_TAU_FIXED] = mTauFixedEdit->text().toDouble();
    //mPhase[STATE_PHASE_TAU_MIN] = mTauMinEdit->text().toDouble();
    //mPhase[STATE_PHASE_TAU_MAX] = mTauMaxEdit->text().toDouble();
    return mPhase;
}

bool PhaseDialog::isValid()
{
    if(mTauTypeCombo->currentIndex() == 1)
    {
        int tau = mTauFixedEdit->text().toInt();
        if(tau < 1)
        {
            mError = tr("Phase fixed duration must be more than 1 !");
            return false;
        }
    }
    /*else if(mTauTypeCombo->currentIndex() == 2)
    {
        int tauMin = mTauMinEdit->text().toInt();
        int tauMax = mTauMaxEdit->text().toInt();
        if(tauMin >= tauMax)
        {
            mError = tr("Duration min must be lower than duration max !");
            return false;
        }
    }*/
    return true;
}

void PhaseDialog::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    
    int w1 = 105;
    int w2 = width() - w1 - 3*mMargin;
    
    mNameLab->setGeometry(mMargin, mMargin, w1, mLineH);
    mColorLab->setGeometry(mMargin, 2*mMargin + mLineH, w1, mLineH);
    mTauTypeLab->setGeometry(mMargin, 3*mMargin + 2*mLineH, w1, mComboH);
    mTauFixedLab->setGeometry(mMargin, 4*mMargin + 2*mLineH + mComboH, w1, mLineH);
    //mTauMinLab->setGeometry(mMargin, 4*mMargin + 2*mLineH + mComboH, w1, mLineH);
    //mTauMaxLab->setGeometry(mMargin, 5*mMargin + 3*mLineH + mComboH, w1, mLineH);
    
    mNameEdit->setGeometry(2*mMargin + w1, mMargin, w2, mLineH);
    mColorPicker->setGeometry(2*mMargin + w1, 2*mMargin + mLineH, w2, mLineH);
    mTauTypeCombo->setGeometry(2*mMargin + w1, 3*mMargin + 2*mLineH, w2, mComboH);
    mTauFixedEdit->setGeometry(2*mMargin + w1, 4*mMargin + 2*mLineH + mComboH, w2, mLineH);
    //mTauMinEdit->setGeometry(2*mMargin + w1, 4*mMargin + 2*mLineH + mComboH, w2, mLineH);
    //mTauMaxEdit->setGeometry(2*mMargin + w1, 5*mMargin + 3*mLineH + mComboH, w2, mLineH);
    
    mOkBut->setGeometry(width() - 2*mMargin - 2*mButW, height() - mMargin - mButH, mButW, mButH);
    mCancelBut->setGeometry(width() - mMargin - mButW, height() - mMargin - mButH, mButW, mButH);
}
