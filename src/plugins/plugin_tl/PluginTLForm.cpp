#include "PluginTLForm.h"
#if USE_PLUGIN_TL

#include "PluginTL.h"
#include "Label.h"
#include "LineEdit.h"
#include <QJsonObject>
#include <QtWidgets>


PluginTLForm::PluginTLForm(PluginTL* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("TL Measurements"), parent, flags)
{
    mAverageLab = new Label(tr("Age") + " :", this);
    mErrorLab = new Label(tr("Error") + " :", this);
    mYearLab = new Label(tr("Ref. year") + " :", this);
    
    mAverageEdit = new LineEdit(this);
    mAverageEdit->setText("0");
    
    mErrorEdit = new LineEdit(this);
    mErrorEdit->setText("30");
    
    mYearEdit = new LineEdit(this);
    mYearEdit->setText(QString::number(QDate::currentDate().year()));
    
    setFixedHeight(sTitleHeight + 4*mMargin + 3*mLineH);
}

PluginTLForm::~PluginTLForm()
{

}

void PluginTLForm::setData(const QJsonObject& data)
{
    double a = data.value(DATE_TL_AGE_STR).toDouble();
    double e = data.value(DATE_TL_ERROR_STR).toDouble();
    double y = data.value(DATE_TL_REF_YEAR_STR).toDouble();
    
    mAverageEdit->setText(QString::number(a));
    mErrorEdit->setText(QString::number(e));
    mYearEdit->setText(QString::number(y));
}

QJsonObject PluginTLForm::getData()
{
    QJsonObject data;
    
    double a = mAverageEdit->text().toDouble();
    double e = mErrorEdit->text().toDouble();
    double y = mYearEdit->text().toDouble();
    
    data.insert(DATE_TL_AGE_STR, a);
    data.insert(DATE_TL_ERROR_STR, e);
    data.insert(DATE_TL_REF_YEAR_STR, y);
    
    return data;
}

void PluginTLForm::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = mMargin;
    int w = width();
    int w1 = 100;
    int w2 = w - 3*m - w1;
    
    mAverageLab->setGeometry(m, sTitleHeight + m, w1, mLineH);
    mErrorLab->setGeometry(m, sTitleHeight + 2*m + mLineH, w1, mLineH);
    mYearLab->setGeometry(m, sTitleHeight + 3*m + 2*mLineH, w1, mLineH);
    
    mAverageEdit->setGeometry(2*m + w1, sTitleHeight + m, w2, mLineH);
    mErrorEdit->setGeometry(2*m + w1, sTitleHeight + 2*m + mLineH, w2, mLineH);
    mYearEdit->setGeometry(2*m + w1, sTitleHeight + 3*m + 2*mLineH, w2, mLineH);
}

#endif