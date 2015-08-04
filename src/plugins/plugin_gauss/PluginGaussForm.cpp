#include "PluginGaussForm.h"
#if USE_PLUGIN_GAUSS

#include "PluginGauss.h"
#include "Label.h"
#include "LineEdit.h"
#include "RadioButton.h"
#include <QJsonObject>
#include <QtWidgets>


PluginGaussForm::PluginGaussForm(PluginGauss* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("Gaussian measurement"), parent, flags)
{
    mAverageLab = new Label(tr("Measure") + " :", this);
    mErrorLab = new Label(tr("Error (sd)") + " :", this);
    
    mEq1Lab = new Label("g(t) = ", this);
    mEq2Lab = new Label(" t^2 + ", this);
    mEq3Lab = new Label(" t + ", this);
    
    mEq1Lab->setAlignment(Qt::AlignCenter);
    mEq2Lab->setAlignment(Qt::AlignCenter);
    mEq3Lab->setAlignment(Qt::AlignCenter);
    
    mAverageEdit = new LineEdit(this);
    mErrorEdit = new LineEdit(this);
    
    mAverageEdit->setText("0");
    mErrorEdit->setText("50");
    
    mAEdit = new LineEdit(this);
    mBEdit = new LineEdit(this);
    mCEdit = new LineEdit(this);
    
    mAEdit->setText("0");
    mBEdit->setText("1");
    mCEdit->setText("0");
    
    mCurveRadio = new RadioButton(tr("Use custom curve file"), this);
    mEquationRadio = new RadioButton(tr("Build your equation"), this);
    mCurveRadio->setChecked(true);
    
    mCurveCombo = new QComboBox(this);
    QStringList refCurves = plugin->getRefsNames();
    for(int i = 0; i<refCurves.size(); ++i){
        mCurveCombo->addItem(refCurves[i]);
    }
    mComboH = mCurveCombo->sizeHint().height();
    
    
    connect(mCurveRadio, SIGNAL(toggled(bool)), this, SLOT(updateVisibleElements()));
    connect(mEquationRadio, SIGNAL(toggled(bool)), this, SLOT(updateVisibleElements()));
    
    updateVisibleElements();
    
    setFixedHeight(sTitleHeight + 5*mMargin + 3*mLineH + mComboH);
}

PluginGaussForm::~PluginGaussForm()
{

}

void PluginGaussForm::setData(const QJsonObject& data, bool isCombined)
{
    double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
    double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();
    double a = data.value(DATE_GAUSS_A_STR).toDouble();
    double b = data.value(DATE_GAUSS_B_STR).toDouble();
    double c = data.value(DATE_GAUSS_C_STR).toDouble();
    QString mode = data.value(DATE_GAUSS_MODE_STR).toString();
    QString curve = data.value(DATE_GAUSS_CURVE_STR).toString();
    
    mAverageEdit->setText(QString::number(age));
    mErrorEdit->setText(QString::number(error));
    mAEdit->setText(QString::number(a));
    mBEdit->setText(QString::number(b));
    mCEdit->setText(QString::number(c));
    
    mCurveRadio->setChecked(mode == DATE_GAUSS_MODE_CURVE);
    mEquationRadio->setChecked(mode == DATE_GAUSS_MODE_EQ);
    
    mCurveCombo->setCurrentText(curve);
    updateVisibleElements();
}

void PluginGaussForm::updateVisibleElements()
{
    bool isCurve = mCurveRadio->isChecked();
    
    mCurveCombo->setVisible(isCurve);
    
    mAEdit->setVisible(!isCurve);
    mBEdit->setVisible(!isCurve);
    mCEdit->setVisible(!isCurve);
    mEq1Lab->setVisible(!isCurve);
    mEq2Lab->setVisible(!isCurve);
    mEq3Lab->setVisible(!isCurve);
}

QJsonObject PluginGaussForm::getData()
{
    QJsonObject data;
    
    double age = mAverageEdit->text().toDouble();
    double error = mErrorEdit->text().toDouble();
    double a = mAEdit->text().toDouble();
    double b = mBEdit->text().toDouble();
    double c = mCEdit->text().toDouble();
    QString mode = "";
    if(mCurveRadio->isChecked()) mode = DATE_GAUSS_MODE_CURVE;
    else if(mEquationRadio->isChecked()) mode = DATE_GAUSS_MODE_EQ;
    QString curve = mCurveCombo->currentText();
    
    data.insert(DATE_GAUSS_AGE_STR, age);
    data.insert(DATE_GAUSS_ERROR_STR, error);
    data.insert(DATE_GAUSS_A_STR, a);
    data.insert(DATE_GAUSS_B_STR, b);
    data.insert(DATE_GAUSS_C_STR, c);
    data.insert(DATE_GAUSS_MODE_STR, mode);
    data.insert(DATE_GAUSS_CURVE_STR, curve);
    
    return data;
}

bool PluginGaussForm::isValid()
{
    return true;
}

void PluginGaussForm::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = mMargin;
    int w = width();
    int w1 = 100;
    int w2 = w - 3*m - w1;
    
    mAverageLab->setGeometry(m, sTitleHeight + m, w1, mLineH);
    mErrorLab->setGeometry(m, sTitleHeight + 2*m + mLineH, w1, mLineH);
    
    mAverageEdit->setGeometry(2*m + w1, sTitleHeight + m, w2, mLineH);
    mErrorEdit->setGeometry(2*m + w1, sTitleHeight + 2*m + mLineH, w2, mLineH);
    
    int eltw = 40;
    int eqw = eltw * 6;
    int x = (w - eqw) / 2;
    int y = sTitleHeight + 3*m + 2*mLineH;
    
    mEquationRadio->setGeometry(2*m + w1, y, (w2 - m) / 2, mLineH);
    mCurveRadio->setGeometry(2*m + w1 + (w2 - m) / 2, y, (w2 - m) / 2, mLineH);
    
    y += m + mLineH;
    
    mCurveCombo->setGeometry(2*m + w1, y, w2, mComboH);
    
    mEq1Lab->setGeometry(x, y, eltw, mLineH);
    mAEdit->setGeometry(x + eltw, y, eltw, mLineH);
    mEq2Lab->setGeometry(x + 2*eltw, y, eltw, mLineH);
    mBEdit->setGeometry(x + 3*eltw, y, eltw, mLineH);
    mEq3Lab->setGeometry(x + 4*eltw, y, eltw, mLineH);
    mCEdit->setGeometry(x + 5*eltw, y, eltw, mLineH);
}

#endif
