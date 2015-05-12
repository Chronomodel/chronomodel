#include "PluginUniform.h"
#if USE_PLUGIN_UNIFORM

#include "PluginUniformForm.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>


PluginUniform::PluginUniform()
{
    
}

double PluginUniform::getLikelyhood(const double& t, const QJsonObject& data)
{
    double min = data[DATE_UNIFORM_MIN_STR].toDouble();
    double max = data[DATE_UNIFORM_MAX_STR].toDouble();
    
    return (t >= min && t <= max) ? 1.f / (max-min) : 0;
}

QString PluginUniform::getName() const
{
    return QString("Typo Ref.");
}
QIcon PluginUniform::getIcon() const
{
    return QIcon(":/uniform_w.png");
}

QColor PluginUniform::getColor() const
{
    return QColor(220,204,173);//Qt::black;
}

bool PluginUniform::doesCalibration() const
{
    return false;
}
bool PluginUniform::wiggleAllowed() const
{
    return false;
}
Date::DataMethod PluginUniform::getDataMethod() const
{
    return Date::eMHIndependant;
}
QList<Date::DataMethod> PluginUniform::allowedDataMethods() const
{
    QList<Date::DataMethod> methods;
    methods.append(Date::eMHIndependant);
    methods.append(Date::eMHSymGaussAdapt);
    return methods;
}
QStringList PluginUniform::csvColumns() const
{
    QStringList cols;
    cols << "Name" << "Lower date" << "Upper date";
    return cols;
}


PluginFormAbstract* PluginUniform::getForm()
{
    PluginUniformForm* form = new PluginUniformForm(this);
    return form;
}

QJsonObject PluginUniform::fromCSV(const QStringList& list)
{
    QJsonObject json;
    if(list.size() >= csvMinColumns())
    {
        json.insert(DATE_UNIFORM_MIN_STR, list[1].toDouble());
        json.insert(DATE_UNIFORM_MAX_STR, list[2].toDouble());
    }
    return json;
}

QStringList PluginUniform::toCSV(const QJsonObject& data)
{
    QStringList list;
    list << QString::number(data[DATE_UNIFORM_MIN_STR].toDouble());
    list << QString::number(data[DATE_UNIFORM_MAX_STR].toDouble());
    return list;
}

QString PluginUniform::getDateDesc(const Date* date) const
{
    QString result;
    if(date)
    {
        QJsonObject data = date->mData;
        result += QObject::tr("Interval") + " : [" + QString::number(data[DATE_UNIFORM_MIN_STR].toDouble()) + "; " +
            QString::number(data[DATE_UNIFORM_MAX_STR].toDouble()) + "]";
    }
    return result;
}


// ------------------------------------------------------------------

GraphViewRefAbstract* PluginUniform::getGraphViewRef()
{
    return 0;
}

#endif
