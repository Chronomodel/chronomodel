#include "PluginGaussForm.h"
#if USE_PLUGIN_GAUSS

#include "PluginGauss.h"
#include "Label.h"
#include "LineEdit.h"
#include <QJsonObject>
#include <QtWidgets>


PluginGaussForm::PluginGaussForm(PluginGauss* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("Gauss Prior"), parent, flags)
{
    mAverageLab = new Label(tr("Measure") + " :", this);
    mErrorLab = new Label(tr("Error") + " :", this);
    
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
    
    setFixedHeight(sTitleHeight + 4*mMargin + 3*mLineH);
}

PluginGaussForm::~PluginGaussForm()
{

}

void PluginGaussForm::setData(const QJsonObject& data)
{
    float age = data.value(DATE_GAUSS_AGE_STR).toDouble();
    float error = data.value(DATE_GAUSS_ERROR_STR).toDouble();
    float a = data.value(DATE_GAUSS_A_STR).toDouble();
    float b = data.value(DATE_GAUSS_B_STR).toDouble();
    float c = data.value(DATE_GAUSS_C_STR).toDouble();
    
    mAverageEdit->setText(QString::number(age));
    mErrorEdit->setText(QString::number(error));
    mAEdit->setText(QString::number(a));
    mBEdit->setText(QString::number(b));
    mCEdit->setText(QString::number(c));
}

QJsonObject PluginGaussForm::getData()
{
    QJsonObject data;
    
    float age = mAverageEdit->text().toDouble();
    float error = mErrorEdit->text().toDouble();
    float a = mAEdit->text().toDouble();
    float b = mBEdit->text().toDouble();
    float c = mCEdit->text().toDouble();
    
    data.insert(DATE_GAUSS_AGE_STR, age);
    data.insert(DATE_GAUSS_ERROR_STR, error);
    data.insert(DATE_GAUSS_A_STR, a);
    data.insert(DATE_GAUSS_B_STR, b);
    data.insert(DATE_GAUSS_C_STR, c);
    
    return data;
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
    
    mEq1Lab->setGeometry(x, y, eltw, mLineH);
    mAEdit->setGeometry(x + eltw, y, eltw, mLineH);
    mEq2Lab->setGeometry(x + 2*eltw, y, eltw, mLineH);
    mBEdit->setGeometry(x + 3*eltw, y, eltw, mLineH);
    mEq3Lab->setGeometry(x + 4*eltw, y, eltw, mLineH);
    mCEdit->setGeometry(x + 5*eltw, y, eltw, mLineH);
}

#endif
