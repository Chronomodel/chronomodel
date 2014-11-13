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

float PluginUniform::getLikelyhood(const float& t, const QJsonObject& data)
{
    float min = data[DATE_UNIFORM_MIN_STR].toDouble();
    float max = data[DATE_UNIFORM_MAX_STR].toDouble();
    
    return (t >= min && t < max) ? 1.f / (max-min) : 0;
}

QString PluginUniform::getName() const
{
    return QString("Typo Ref.");
}
QIcon PluginUniform::getIcon() const
{
    return QIcon(":/uniform_w.png");
}
bool PluginUniform::doesCalibration() const
{
    return false;
}
Date::DataMethod PluginUniform::getDataMethod() const
{
    return Date::eMHIndependant;
}
QStringList PluginUniform::csvColumns() const
{
    QStringList cols;
    cols << "Name" << "Min" << "Max";
    return cols;
}


PluginFormAbstract* PluginUniform::getForm()
{
    PluginUniformForm* form = new PluginUniformForm(this);
    return form;
}

QJsonObject PluginUniform::dataFromList(const QStringList& list)
{
    QJsonObject json;
    if(list.size() >= csvMinColumns())
    {
        json.insert(DATE_UNIFORM_MIN_STR, list[1].toDouble());
        json.insert(DATE_UNIFORM_MAX_STR, list[2].toDouble());
    }
    return json;
}



// ------------------------------------------------------------------

GraphViewRefAbstract* PluginUniform::getGraphViewRef()
{
    return 0;
}

#endif
