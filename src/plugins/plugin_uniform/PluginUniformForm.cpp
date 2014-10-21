#include "PluginUniformForm.h"
#if USE_PLUGIN_UNIFORM

#include "PluginUniform.h"
#include "Label.h"
#include "LineEdit.h"
#include <QJsonObject>
#include <QtWidgets>


PluginUniformForm::PluginUniformForm(PluginUniform* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("Uniform Prior"), parent, flags)
{
    mMinLab = new Label(tr("Lower date") + " :", this);
    mMaxLab = new Label(tr("Upper date") + " :", this);
    
    mMinEdit = new LineEdit(this);
    mMinEdit->setText("0");
    
    mMaxEdit = new LineEdit(this);
    mMaxEdit->setText("100");
    
    setFixedHeight(sTitleHeight + 3*mMargin + 2*mLineH);
}

PluginUniformForm::~PluginUniformForm()
{

}

void PluginUniformForm::setData(const QJsonObject& data)
{
    double min = data.value(DATE_UNIFORM_MIN_STR).toDouble();
    double max = data.value(DATE_UNIFORM_MAX_STR).toDouble();
    
    mMinEdit->setText(QString::number(min));
    mMaxEdit->setText(QString::number(max));
}

QJsonObject PluginUniformForm::getData()
{
    QJsonObject data;
    
    double min = mMinEdit->text().toDouble();
    double max = mMaxEdit->text().toDouble();
    
    data.insert(DATE_UNIFORM_MIN_STR, min);
    data.insert(DATE_UNIFORM_MAX_STR, max);
    
    return data;
}

void PluginUniformForm::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = mMargin;
    int w = width();
    int w1 = 100;
    int w2 = w - 3*m - w1;
    
    mMinLab->setGeometry(m, sTitleHeight + m, w1, mLineH);
    mMaxLab->setGeometry(m, sTitleHeight + 2*m + mLineH, w1, mLineH);
    
    mMinEdit->setGeometry(2*m + w1, sTitleHeight + m, w2, mLineH);
    mMaxEdit->setGeometry(2*m + w1, sTitleHeight + 2*m + mLineH, w2, mLineH);
}

#endif

