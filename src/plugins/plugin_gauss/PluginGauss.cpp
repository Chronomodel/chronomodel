#include "PluginGauss.h"
#if USE_PLUGIN_GAUSS

#include "StdUtilities.h"
#include "PluginGaussForm.h"
#include "PluginGaussRefView.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>


PluginGauss::PluginGauss()
{
    
}

float PluginGauss::getLikelyhood(const float& t, const QJsonObject& data)
{
    float age = data[DATE_GAUSS_AGE_STR].toDouble();
    float error = data[DATE_GAUSS_ERROR_STR].toDouble();
    float a = data[DATE_GAUSS_A_STR].toDouble();
    float b = data[DATE_GAUSS_B_STR].toDouble();
    float c = data[DATE_GAUSS_C_STR].toDouble();
    
    float v = exp(-0.5 * powf((age - (a * t * t + b * t + c)) / error, 2)); // / (s * sqrt(2*M_PI))
    return v;
}

QString PluginGauss::getName() const
{
    return QString("Gauss");
}
QIcon PluginGauss::getIcon() const
{
    return QIcon(":/gauss_w.png");
}
bool PluginGauss::doesCalibration() const
{
    return true;
}
Date::DataMethod PluginGauss::getDataMethod() const
{
    return Date::eMHIndependant;
}
QString PluginGauss::csvHelp() const
{
    return "Calibration : g(t) = at^2 + bt + c\n";
}
QStringList PluginGauss::csvColumns() const
{
    QStringList cols;
    cols << "Name" << "Age" << "Error" << "a" << "b" << "c";
    return cols;
}


PluginFormAbstract* PluginGauss::getForm()
{
    PluginGaussForm* form = new PluginGaussForm(this);
    return form;
}

QJsonObject PluginGauss::dataFromList(const QStringList& list)
{
    QJsonObject json;
    if(list.size() >= csvMinColumns())
    {
        json.insert(DATE_GAUSS_AGE_STR, list[1].toDouble());
        json.insert(DATE_GAUSS_ERROR_STR, list[2].toDouble());
        json.insert(DATE_GAUSS_A_STR, list[3].toDouble());
        json.insert(DATE_GAUSS_B_STR, list[4].toDouble());
        json.insert(DATE_GAUSS_C_STR, list[5].toDouble());
    }
    return json;
}


// ------------------------------------------------------------------

GraphViewRefAbstract* PluginGauss::getGraphViewRef()
{
    if(!mRefGraph)
        mRefGraph = new PluginGaussRefView();
    return mRefGraph;
}

#endif
