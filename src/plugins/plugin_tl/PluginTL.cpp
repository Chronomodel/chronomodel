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

double PluginTL::getLikelyhood(const double& t, const QJsonObject& data)
{
    double age = data[DATE_TL_AGE_STR].toDouble();
    double error = data[DATE_TL_ERROR_STR].toDouble();
    double ref_year = data[DATE_TL_REF_YEAR_STR].toDouble();
    
    // gaussienne TL
    double v = exp(-0.5f * pow((age - (ref_year - t)) / error, 2.f)) / error; //  * sqrt(2.f * M_PI)
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
QColor PluginTL::getColor() const
{
    return Qt::red;
}
bool PluginTL::doesCalibration() const
{
    return true;
}
bool PluginTL::wiggleAllowed() const
{
    return false;
}
Date::DataMethod PluginTL::getDataMethod() const
{
    return Date::eMHSymGaussAdapt;
}
QList<Date::DataMethod> PluginTL::allowedDataMethods() const
{
    QList<Date::DataMethod> methods;
    methods.append(Date::eMHIndependant);
    methods.append(Date::eInversion);
    methods.append(Date::eMHSymGaussAdapt);
    return methods;
}
QStringList PluginTL::csvColumns() const
{
    QStringList cols;
    cols << "Name" << "Age" << "Error (sd)" << "Reference year";
    return cols;
}


PluginFormAbstract* PluginTL::getForm()
{
    PluginTLForm* form = new PluginTLForm(this);
    return form;
}

QJsonObject PluginTL::fromCSV(const QStringList& list)
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

QStringList PluginTL::toCSV(const QJsonObject& data)
{
    QStringList list;
    list << QString::number(data[DATE_TL_AGE_STR].toDouble());
    list << QString::number(data[DATE_TL_ERROR_STR].toDouble());
    list << QString::number(data[DATE_TL_REF_YEAR_STR].toDouble());
    return list;
}

QString PluginTL::getDateDesc(const Date* date) const
{
    QString result;
    if(date)
    {
        QJsonObject data = date->mData;
        result += QObject::tr("Age") + " : " + QString::number(data[DATE_TL_AGE_STR].toDouble());
        result += " ± " + QString::number(data[DATE_TL_ERROR_STR].toDouble());
        result += ", " + QObject::tr("Ref. year") + " : " + QString::number(data[DATE_TL_REF_YEAR_STR].toDouble());
    }
    return result;
}


// ------------------------------------------------------------------

GraphViewRefAbstract* PluginTL::getGraphViewRef()
{
    if(!mRefGraph)
        mRefGraph = new PluginTLRefView();
    return mRefGraph;
}

#endif