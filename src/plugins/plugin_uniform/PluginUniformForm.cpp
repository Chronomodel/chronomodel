#include "PluginUniformForm.h"
#if USE_PLUGIN_UNIFORM

#include "PluginUniform.h"
#include <QJsonObject>
#include <QtWidgets>


PluginUniformForm::PluginUniformForm(PluginUniform* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("Uniform Prior"), parent, flags)
{
    //PluginUniform* pluginUnif = (PluginUniform*)mPlugin;
    
    mMinLab = new QLabel(tr("Lower date") + " :", this);
    mMaxLab = new QLabel(tr("Upper date") + " :", this);
    
    mMinEdit = new QLineEdit(this);
    mMinEdit->setText("0");
    
    mMaxEdit = new QLineEdit(this);
    mMaxEdit->setText("100");
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    
    grid->addWidget(mMinLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mMinEdit, 0, 1);
    
    grid->addWidget(mMaxLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mMaxEdit, 1, 1);
    
    setLayout(grid);
}

PluginUniformForm::~PluginUniformForm()
{

}

void PluginUniformForm::setData(const QJsonObject& data, bool isCombined)
{
    QLocale locale=QLocale();
    double min = data.value(DATE_UNIFORM_MIN_STR).toDouble();
    double max = data.value(DATE_UNIFORM_MAX_STR).toDouble();
    
    mMinEdit->setText(locale.toString(min));
    mMaxEdit->setText(locale.toString(max));
    connect(mMinEdit, &QLineEdit::textChanged, this, &PluginUniformForm::errorIsValid);
    connect(mMaxEdit, &QLineEdit::textChanged, this, &PluginUniformForm::errorIsValid);
}

QJsonObject PluginUniformForm::getData()
{
    QJsonObject data;
    QLocale locale=QLocale();
    
    double min = round(locale.toDouble(mMinEdit->text()));
    double max = round(locale.toDouble(mMaxEdit->text()));
    
    data.insert(DATE_UNIFORM_MIN_STR, min);
    data.insert(DATE_UNIFORM_MAX_STR, max);
    
    return data;
}

void PluginUniformForm::errorIsValid(QString str)
{
    bool oka,okb;
    QLocale locale;
    double a = locale.toDouble(mMinEdit->text(),&oka);
    double b = locale.toDouble(mMaxEdit->text(),&okb);

    emit PluginFormAbstract::OkEnabled(oka && okb && (a<b) );
}

bool PluginUniformForm::isValid()
{
    QLocale locale=QLocale();
    double min = round(locale.toDouble(mMinEdit->text()));
    double max = round(locale.toDouble(mMaxEdit->text()));
    if(min >= max)
        mError = tr("Forbidden : lower date must be > upper date");
    return min < max;
}

#endif

