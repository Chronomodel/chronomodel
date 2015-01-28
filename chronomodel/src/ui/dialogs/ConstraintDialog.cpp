#include "ConstraintDialog.h"
#include "Label.h"
#include "LineEdit.h"
#include "Button.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"
#include "Button.h"
#include <QtWidgets>


ConstraintDialog::ConstraintDialog(QWidget* parent, ConstraintDialog::Type type, Qt::WindowFlags flags):QDialog(parent, flags),
mType(type),
mDeleteRequested(false)
{
    setWindowTitle(tr("Constraint"));
    
    // -----------
    
    mTypeLab = new Label(tr("Hiatus min") + " :", this);
    mFixedLab = new Label(tr("Hiatus min fixed") + " :", this);
    mMinLab = new Label(tr("Lower hiatus min") + " :", this);
    mMaxLab = new Label(tr("Upper hiatus min") + " :", this);
    
    mTypeCombo = new QComboBox(this);
    mTypeCombo->addItem(tr("Unknown"));
    mTypeCombo->addItem(tr("Fixed"));
    mTypeCombo->addItem(tr("Range"));
    
    mFixedEdit = new LineEdit(this);
    mMinEdit = new LineEdit(this);
    mMaxEdit = new LineEdit(this);
    
    QIntValidator* positiveValidator = new QIntValidator();
    //positiveValidator->setBottom(0);
    mMinEdit->setValidator(positiveValidator);
    positiveValidator->setBottom(1);
    mFixedEdit->setValidator(positiveValidator);
    mMaxEdit->setValidator(positiveValidator);
    
    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    mDeleteBut = new Button(tr("Delete constraint"), this);
    
    mOkBut->setAutoDefault(true);
    
    mComboH = mTypeCombo->sizeHint().height();
    
    // ----------
    
    connect(mTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showAppropriateOptions()));
    
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mDeleteBut, SIGNAL(clicked()), this, SLOT(deleteConstraint()));
    
    setFixedWidth(400);
    
    showAppropriateOptions();
}

ConstraintDialog::~ConstraintDialog()
{
    
}

void ConstraintDialog::setConstraint(const QJsonObject& constraint)
{
    mConstraint = constraint;
    
    if(mType == eEvent)
    {
        /*mTypeCombo->setCurrentIndex(mConstraint[STATE_EVENT_CONSTRAINT_PHI_TYPE].toInt());
        mMinEdit->setText(QString::number(mConstraint[STATE_EVENT_CONSTRAINT_PHI_MIN].toDouble()));
        mMaxEdit->setText(QString::number(mConstraint[STATE_EVENT_CONSTRAINT_PHI_MAX].toDouble()));*/
    }
    else if(mType == ePhase)
    {
        mTypeCombo->setCurrentIndex(mConstraint[STATE_CONSTRAINT_GAMMA_TYPE].toInt());
        mFixedEdit->setText(QString::number(mConstraint[STATE_CONSTRAINT_GAMMA_FIXED].toDouble()));
        mMinEdit->setText(QString::number(mConstraint[STATE_CONSTRAINT_GAMMA_MIN].toDouble()));
        mMaxEdit->setText(QString::number(mConstraint[STATE_CONSTRAINT_GAMMA_MAX].toDouble()));
    }
    showAppropriateOptions();
}

QJsonObject ConstraintDialog::constraint() const
{
    QJsonObject c = mConstraint;
    if(mType == eEvent)
    {
        /*mConstraint[STATE_EVENT_CONSTRAINT_PHI_TYPE] = mTypeCombo->currentIndex();
        mConstraint[STATE_EVENT_CONSTRAINT_PHI_MIN] = mMinEdit->text().toDouble();
        mConstraint[STATE_EVENT_CONSTRAINT_PHI_MAX] = mMaxEdit->text().toDouble();*/
    }
    else if(mType == ePhase)
    {
        c[STATE_CONSTRAINT_GAMMA_TYPE] = mTypeCombo->currentIndex();
        c[STATE_CONSTRAINT_GAMMA_FIXED] = mFixedEdit->text().toDouble();
        c[STATE_CONSTRAINT_GAMMA_MIN] = mMinEdit->text().toDouble();
        c[STATE_CONSTRAINT_GAMMA_MAX] = mMaxEdit->text().toDouble();
    }
    return c;
}

bool ConstraintDialog::deleteRequested() const
{
    return mDeleteRequested;
}

void ConstraintDialog::deleteConstraint()
{
    mDeleteRequested = true;
    accept();
}

void ConstraintDialog::showAppropriateOptions()
{
    int m = 5;
    int lineH = 20;
    int butH = 25;
    
    if(mTypeCombo->currentIndex() == 0)
    {
        mFixedLab->setVisible(false);
        mFixedEdit->setVisible(false);
        
        mMinLab->setVisible(false);
        mMaxLab->setVisible(false);
        
        mMinEdit->setVisible(false);
        mMaxEdit->setVisible(false);
        
        setFixedHeight(mComboH + 3*m + butH);
    }
    else if(mTypeCombo->currentIndex() == 1)
    {
        mFixedLab->setVisible(true);
        mFixedEdit->setVisible(true);
        
        mMinLab->setVisible(false);
        mMaxLab->setVisible(false);
        
        mMinEdit->setVisible(false);
        mMaxEdit->setVisible(false);
        
        mFixedEdit->selectAll();
        mFixedEdit->setFocus();
        
        setFixedHeight(mComboH + 4*m + butH + lineH);
    }
    else if(mTypeCombo->currentIndex() == 2)
    {
        mFixedLab->setVisible(false);
        mFixedEdit->setVisible(false);
        
        mMinLab->setVisible(true);
        mMaxLab->setVisible(true);
        
        mMinEdit->setVisible(true);
        mMaxEdit->setVisible(true);
        
        mMinEdit->selectAll();
        mMinEdit->setFocus();
        
        setFixedHeight(mComboH + 5*m + butH + 2*lineH);
    }
}

void ConstraintDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = 5;
    int w1 = 100;
    int w2 = width() - w1 - 3*m;
    int lineH = 20;
    int butW = 80;
    int butH = 25;
    int deleteButW = 130;
    
    mTypeLab->setGeometry(m, m, w1, mComboH);
    mFixedLab->setGeometry(m, 2*m + mComboH, w1, lineH);
    mMinLab->setGeometry(m, 2*m + mComboH, w1, lineH);
    mMaxLab->setGeometry(m, 2*m + lineH + mComboH, w1, lineH);
    
    mTypeCombo->setGeometry(2*m + w1, m, w2, mComboH);
    mFixedEdit->setGeometry(2*m + w1, 2*m + mComboH, w2, lineH);
    mMinEdit->setGeometry(2*m + w1, 2*m + mComboH, w2, lineH);
    mMaxEdit->setGeometry(2*m + w1, 3*m + mComboH + lineH, w2, lineH);
    
    mDeleteBut->setGeometry(width() -3*m - 2*butW - deleteButW, height() - m - butH, deleteButW, butH);
    mOkBut->setGeometry(width() -2*m - 2*butW, height() - m - butH, butW, butH);
    mCancelBut->setGeometry(width() -1*m - 1*butW, height() - m - butH, butW, butH);
}
