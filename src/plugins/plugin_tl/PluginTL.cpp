#include "PluginTL.h"
#if USE_PLUGIN_TL

#include "StdUtilities.h"
#include "PluginTLForm.h"
#include "PluginTLRefView.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>


PluginTL::PluginTL()
{
    
}

float PluginTL::getLikelyhood(const float& t, const QJsonObject& data)
{
    float age = data[DATE_TL_AGE_STR].toDouble();
    float error = data[DATE_TL_ERROR_STR].toDouble();
    float ref_year = data[DATE_TL_REF_YEAR_STR].toDouble();
    
    // gaussienne TL
    float v = exp(-0.5 * pow((age - (ref_year - t)) / error, 2)); // / (s * sqrt(2*M_PI))
    return v;
}

QString PluginTL::getName() const
{
    return QString("TL/OSL");
}
QIcon PluginTL::getIcon() const
{
    return QIcon(":/TL_w.png");
}
bool PluginTL::doesCalibration() const
{
    return true;
}
Date::DataMethod PluginTL::getDataMethod() const
{
    return Date::eMHIndependant;
}
QStringList PluginTL::csvColumns() const
{
    QStringList cols;
    cols << "Name" << "Age" << "Error" << "Reference year";
    return cols;
}


PluginFormAbstract* PluginTL::getForm()
{
    PluginTLForm* form = new PluginTLForm(this);
    return form;
}

QJsonObject PluginTL::dataFromList(const QStringList& list)
{
    QJsonObject json;
    if(list.size() >= csvMinColumns())
    {
        json.insert(DATE_TL_AGE_STR, list[1].toDouble());
        json.insert(DATE_TL_ERROR_STR, list[2].toDouble());
        json.insert(DATE_TL_REF_YEAR_STR, list[3].toDouble());
    }
    return json;
}


// ------------------------------------------------------------------

GraphViewRefAbstract* PluginTL::getGraphViewRef()
{
    if(!mRefGraph)
        mRefGraph = new PluginTLRefView();
    return mRefGraph;
}

#endif