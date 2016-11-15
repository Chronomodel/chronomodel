#include "StepDialog.h"

#include "Button.h"
#include "Label.h"
#include "CheckBox.h"
#include "Painting.h"
#include <QtWidgets>
#include <QDoubleSpinBox>

StepDialog::StepDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags)
{
    setWindowTitle(tr("Resolution of distribution of calibrated date"));
    
    mTitleLab = new Label(tr("Resolution of distribution of calibrated date"), this);
    mTitleLab -> setIsTitle(true);
    
    mForcedLab   = new Label(tr("Force resolution") + " : ", this);
    mForcedCheck = new CheckBox(this);
    mStepLab     = new Label(tr("Resolution in years") + " : ", this);
    
    //mStepSpin = new QSpinBox(this);//QDoubleSpinBox
    mStepSpin = new QDoubleSpinBox(this);//QDoubleSpinBox
    mStepSpin -> setRange(0.01, 10000);
    mStepSpin -> setSingleStep(0.01);
    mStepSpin -> setDecimals(2);
    
    mOkBut     = new Button(tr("Apply"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    mOkBut -> setAutoDefault(true);
    
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mForcedCheck, SIGNAL(toggled(bool)), mStepSpin, SLOT(setEnabled(bool)));
    
    setFixedSize(500, 110);
}

StepDialog::~StepDialog()
{

}

void StepDialog::setStep(double step, bool forced, double suggested)
{
    mForcedCheck -> setText("(" + tr("suggested/default value : ") + QString::number(suggested) + ")");
    mForcedCheck -> setChecked(forced);
    mStepSpin    -> setEnabled(forced);
    mStepSpin    -> setValue(step);
}

double StepDialog::step() const
{
    return mStepSpin -> value();
}

bool StepDialog::forced() const
{
    return mForcedCheck -> isChecked();
}

void StepDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = 5;
    int lineH = 20;
    int w1 = 200;
    int w2 = width() - 3*m - w1;
    int butW = 80;
    int butH = 25;
    
    int y = -lineH;
    
    mTitleLab    -> setGeometry(m, y += (lineH + m), width() - 2*m, lineH);
    
    mForcedLab   -> setGeometry(m, y += (lineH + m), w1, lineH);
    mForcedCheck -> setGeometry(2*m + w1, y, w2, lineH);
    
    mStepLab     -> setGeometry(m, y += (lineH + m), w1, lineH);
    mStepSpin    -> setGeometry(2*m + w1, y, 120, lineH);
    
    mOkBut       -> setGeometry(width() - 2*m - 2*butW, height() - m - butH, butW, butH);
    mCancelBut   -> setGeometry(width() - m - butW, height() - m - butH, butW, butH);
}

void StepDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(rect(), QColor(180, 180, 180));
}

