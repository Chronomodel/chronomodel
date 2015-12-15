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
    mColor = QColor(216,207,52);
}

double PluginTL::getLikelyhood(const double& t, const QJsonObject& data)
{
    double age = data[DATE_TL_AGE_STR].toDouble();
    double error = data[DATE_TL_ERROR_STR].toDouble();
    double ref_year = data[DATE_TL_REF_YEAR_STR].toDouble();
    
    // gaussienne TL
    double v = exp(-0.5f * pow((age - (ref_year - t)) / error, 2.f)) / error;
    return v;
}

QPair<double, double > PluginTL::getLikelyhoodArg(const double& t, const QJsonObject& data)
{
    QPair<double, double> result;
    double age = data[DATE_TL_AGE_STR].toDouble();
    double error = data[DATE_TL_ERROR_STR].toDouble();
    double ref_year = data[DATE_TL_REF_YEAR_STR].toDouble();
    
    // gaussienne TL
    result= QPair<double,double>(1/(error*error), (-0.5f * pow((age - (ref_year - t)) / error, 2.f))) ;
    
    return result;
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
    methods.append(Date::eMHSymetric);
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

QStringList PluginTL::toCSV(const QJsonObject& data, const QLocale& csvLocale)
{
    QStringList list;
    list << csvLocale.toString(data[DATE_TL_AGE_STR].toDouble());
    list << csvLocale.toString(data[DATE_TL_ERROR_STR].toDouble());
    list << csvLocale.toString(data[DATE_TL_REF_YEAR_STR].toDouble());
    return list;
}

QString PluginTL::getDateDesc(const Date* date) const
{
    QLocale locale=QLocale();
    QString result;
    if(date)
    {
        QJsonObject data = date->mData;
        result += QObject::tr("Age") + " : " + locale.toString(data[DATE_TL_AGE_STR].toDouble());
        result += " ± " + locale.toString(data[DATE_TL_ERROR_STR].toDouble());
        result += ", " + QObject::tr("Ref. year") + " : " + locale.toString(data[DATE_TL_REF_YEAR_STR].toDouble());
    }
    return result;
}

QPair<double,double> PluginTL::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    double age = data[DATE_TL_AGE_STR].toDouble();
    double error = data[DATE_TL_ERROR_STR].toDouble();
    double ref_year = data[DATE_TL_REF_YEAR_STR].toDouble();
    
    double k = 5;
    
    double tmin = ref_year - age - k * error;
    double tmax = ref_year - age + k * error;
    
    return qMakePair<double,double>(tmin, tmax);
}




// ------------------------------------------------------------------

GraphViewRefAbstract* PluginTL::getGraphViewRef()
{
    //if(!mRefGraph) mRefGraph = new PluginTLRefView();
    
    if(mRefGraph) delete mRefGraph;
    mRefGraph = new PluginTLRefView();
    return mRefGraph;
}

PluginSettingsViewAbstract* PluginTL::getSettingsView()
{
    return 0;
}

#endif
