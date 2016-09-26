#include "PluginTLForm.h"
#if USE_PLUGIN_TL

#include "PluginTL.h"
#include <QJsonObject>
#include <QtWidgets>


PluginTLForm::PluginTLForm(PluginTL* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("TL Measurements"), parent, flags)
{
   // PluginTL* pluginTL = (PluginTL*)mPlugin;
    
    mAverageLab = new QLabel(tr("Age") + " :", this);
    mErrorLab = new QLabel(tr("Error (sd)") + " :", this);
    mYearLab = new QLabel(tr("Ref. year") + " :", this);
    
    mAverageEdit = new QLineEdit(this);
    mAverageEdit->setText("0");
    
    mErrorEdit = new QLineEdit(this);
    mErrorEdit->setText("30");
    connect(mErrorEdit, &QLineEdit::textChanged, this, &PluginTLForm::errorIsValid);
    
    mYearEdit = new QLineEdit(this);
    mYearEdit->setText(QString::number(QDate::currentDate().year()));
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    
    grid->addWidget(mAverageLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAverageEdit, 0, 1);
    
    grid->addWidget(mErrorLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mErrorEdit, 1, 1);
    
    grid->addWidget(mYearLab, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mYearEdit, 2, 1);
    
    setLayout(grid);
}

PluginTLForm::~PluginTLForm()
{

}

void PluginTLForm::setData(const QJsonObject& data, bool isCombined)
{
    QLocale locale=QLocale();
    double a = data.value(DATE_TL_AGE_STR).toDouble();
    double e = data.value(DATE_TL_ERROR_STR).toDouble();
    double y = data.value(DATE_TL_REF_YEAR_STR).toDouble();
    
    mAverageEdit->setText(locale.toString(a));
    mErrorEdit->setText(locale.toString(e));
    mYearEdit->setText(locale.toString(y));
}

QJsonObject PluginTLForm::getData()
{
    QJsonObject data;
    QLocale locale=QLocale();
    
    double a = locale.toDouble(mAverageEdit->text());
    double e = locale.toDouble(mErrorEdit->text());
    double y = locale.toDouble(mYearEdit->text());
    
    data.insert(DATE_TL_AGE_STR, a);
    data.insert(DATE_TL_ERROR_STR, e);
    data.insert(DATE_TL_REF_YEAR_STR, y);
    
    return data;
}

void PluginTLForm::errorIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str,&ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );

}

bool PluginTLForm::isValid()
{
    return true;
}
#endif
