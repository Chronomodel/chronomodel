#include "StudyPeriodDialog.h"

#include "Button.h"
#include "Label.h"
#include "LineEdit.h"
#include "CheckBox.h"
#include "Painting.h"
#include "ProjectSettings.h"
#include <QtWidgets>
#include <QDoubleSpinBox>

StudyPeriodDialog::StudyPeriodDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags)
{
    setWindowTitle(tr("Study Period Settings"));

    mStudyLab = new Label(tr("STUDY PERIOD (BC/AD)"), this);
    mStudyLab-> setIsTitle(true);

    mMinLab = new Label(tr("Start"), this);
    mMinEdit = new LineEdit(this);

    mMaxLab = new Label(tr("End"), this);
    mMaxEdit = new LineEdit(this);

    //----
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
    
    connect(mOkBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &StudyPeriodDialog::accept);
    connect(mCancelBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &StudyPeriodDialog::reject);
    connect(mForcedCheck, &CheckBox::toggled, mStepSpin, &QDoubleSpinBox::setEnabled);
    
    setFixedSize(500, 300);//(500, 110);
}

StudyPeriodDialog::~StudyPeriodDialog()
{

}

void StudyPeriodDialog::setSettings(const ProjectSettings& s)
{
    mMinEdit->setText(locale().toString(s.mTmin));
    mMaxEdit->setText(locale().toString(s.mTmax));
    double suggested = s.getStep(s.mTmin, s.mTmax);
    mForcedCheck -> setText("(" + tr("suggested/default value : ") + QString::number(suggested) + ")");
    mForcedCheck -> setChecked(s.mStepForced);
    mStepSpin    -> setEnabled(s.mStepForced);
    mStepSpin    -> setValue(s.mStep);
}

void StudyPeriodDialog::setStep(double step, bool forced, double suggested)
{
    mForcedCheck -> setText("(" + tr("suggested/default value : ") + QString::number(suggested) + ")");
    mForcedCheck -> setChecked(forced);
    mStepSpin    -> setEnabled(forced);
    mStepSpin    -> setValue(step);
}

ProjectSettings StudyPeriodDialog::getSettings() const
{
    ProjectSettings s = ProjectSettings();
    s.mTmin = locale().toDouble(mMinEdit->text());
    s.mTmax = locale().toDouble(mMaxEdit->text());
    s.mStep = mStepSpin->value();
    s.mStepForced = mForcedCheck -> isChecked();
    return s;
}

double StudyPeriodDialog::step() const
{
    return mStepSpin -> value();
}

bool StudyPeriodDialog::forced() const
{
    return mForcedCheck -> isChecked();
}

void StudyPeriodDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    QFontMetrics fm(font());
    int m (15);
    int lineH (20);
    int w1 (200);
    int w2 (width() - 5*m - w1);
    int middleWidth (width()/2);
    int butW (fm.width("_______")+ 5);//(80);
    int butH (fm.height()+ 5);//25);
    int verticalSpacer (10);

    int y (verticalSpacer);
    
    mStudyLab->setGeometry(m, y, width() - 2*m , butH);

    y += (mStudyLab->height() + verticalSpacer);
    mMinLab->setGeometry(middleWidth - m - fm.width(mMinLab->text()), y, fm.width(mMinLab->text()), butH);
    mMinEdit->setGeometry(middleWidth + m, y , butW, butH);

    y += (mMinLab->height() + verticalSpacer);
    mMaxLab->setGeometry(middleWidth - m - fm.width(mMaxLab->text()), y, fm.width(mMaxLab->text()), butH);
    mMaxEdit->setGeometry(middleWidth + m, y , butW, butH);

    y += (mMinLab->height() + verticalSpacer);

    //---
    mTitleLab    -> setGeometry(m, y += (lineH + m), width() - 2*m, lineH);
    
    mForcedLab   -> setGeometry(m, y += (lineH + m), w1, lineH);
    mForcedCheck -> setGeometry(2*m + w1, y, w2, lineH);
    
    mStepLab     -> setGeometry(m, y += (lineH + m), w1, lineH);
    mStepSpin    -> setGeometry(2*m + w1, y, 120, lineH);
    
    mOkBut       -> setGeometry(width() - 2*m - 2*butW, height() - m - butH, butW, butH);
    mCancelBut   -> setGeometry(width() - m - butW, height() - m - butH, butW, butH);
}

void StudyPeriodDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(rect(), QColor(180, 180, 180));
}

