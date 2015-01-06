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

double PluginGauss::getLikelyhood(const double& t, const QJsonObject& data)
{
    double age = data[DATE_GAUSS_AGE_STR].toDouble();
    double error = data[DATE_GAUSS_ERROR_STR].toDouble();
    double a = data[DATE_GAUSS_A_STR].toDouble();
    double b = data[DATE_GAUSS_B_STR].toDouble();
    double c = data[DATE_GAUSS_C_STR].toDouble();
    
    double v = exp(-0.5f * pow((age - (a * t * t + b * t + c)) / error, 2.f)) / error; //  * sqrt(2.f * M_PI)
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
bool PluginGauss::wiggleAllowed() const
{
    return false;
}
Date::DataMethod PluginGauss::getDataMethod() const
{
    return Date::eMHSymGaussAdapt;
}
QList<Date::DataMethod> PluginGauss::allowedDataMethods() const
{
    QList<Date::DataMethod> methods;
    methods.append(Date::eMHIndependant);
    methods.append(Date::eInversion);
    methods.append(Date::eMHSymGaussAdapt);
    return methods;
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

QJsonObject PluginGauss::fromCSV(const QStringList& list)
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

QStringList PluginGauss::toCSV(const QJsonObject& data)
{
    QStringList list;
    list << QString::number(data[DATE_GAUSS_AGE_STR].toDouble());
    list << QString::number(data[DATE_GAUSS_ERROR_STR].toDouble());
    list << QString::number(data[DATE_GAUSS_A_STR].toDouble());
    list << QString::number(data[DATE_GAUSS_B_STR].toDouble());
    list << QString::number(data[DATE_GAUSS_C_STR].toDouble());
    return list;
}

QString PluginGauss::getDateDesc(const Date* date) const
{
    QString result;
    if(date)
    {
        QJsonObject data = date->mData;
        
        double a = data[DATE_GAUSS_A_STR].toDouble();
        double b = data[DATE_GAUSS_B_STR].toDouble();
        double c = data[DATE_GAUSS_C_STR].toDouble();
        
        QString aStr;
        if(a != 0.f)
        {
            if(a == -1.f) aStr += "-";
            else if(a != 1.f) aStr += QString::number(a);
            aStr += "t²";
        }
        QString bStr;
        if(b != 0.f)
        {
            if(b == -1.f) bStr += "-";
            else if(b != 1.f) bStr += QString::number(b);
            bStr += "t";
        }
        QString cStr;
        if(c != 0.f)
        {
            cStr += QString::number(c);
        }
        QString eq = aStr;
        if(!eq.isEmpty() && !bStr.isEmpty())
           eq += " + ";
        eq += bStr;
        if(!eq.isEmpty() && !cStr.isEmpty())
            eq += " + ";
        eq += cStr;
        
        result += QObject::tr("Age") + " : " + QString::number(data[DATE_GAUSS_AGE_STR].toDouble());
        result += " ± " + QString::number(data[DATE_GAUSS_ERROR_STR].toDouble());
        result += ", " + QObject::tr("Ref. curve") + " : g(t) = " + eq;
    }
    return result;
}

// ------------------------------------------------------------------

GraphViewRefAbstract* PluginGauss::getGraphViewRef()
{
    if(!mRefGraph)
        mRefGraph = new PluginGaussRefView();
    return mRefGraph;
}

#endif
