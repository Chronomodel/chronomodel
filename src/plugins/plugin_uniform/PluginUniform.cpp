#include "PluginUniform.h"
#if USE_PLUGIN_UNIFORM

#include "PluginUniformForm.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>


PluginUniform::PluginUniform()
{
    mColor = QColor(220, 204, 173);
}

PluginUniform::~PluginUniform()
{

}

long double PluginUniform::getLikelihood(const double& t, const QJsonObject& data)
{
    double min = data.value(DATE_UNIFORM_MIN_STR).toDouble();
    double max = data.value(DATE_UNIFORM_MAX_STR).toDouble();
    
    return (t >= min && t <= max) ? (long double)(1. / (max-min)) : 0.;
}

QString PluginUniform::getName() const
{
    return QString("Typo");
}

QIcon PluginUniform::getIcon() const
{
    return QIcon(":/uniform_w.png");
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
    return Date::eMHSymetric;
}

QList<Date::DataMethod> PluginUniform::allowedDataMethods() const
{
    QList<Date::DataMethod> methods;
    methods.append(Date::eMHSymetric);
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
// Convert old project versions
QJsonObject PluginUniform::checkValuesCompatibility(const QJsonObject& values)
{
    QJsonObject result = values;

    //force type double
    result[DATE_UNIFORM_MIN_STR] = result.value(DATE_UNIFORM_MIN_STR).toDouble();
    result[DATE_UNIFORM_MAX_STR] = result.value(DATE_UNIFORM_MAX_STR).toDouble();

    return result;
}

QJsonObject PluginUniform::fromCSV(const QStringList& list, const QLocale& csvLocale)
{
    QJsonObject json;
    if (list.size() >= csvMinColumns()) {
        double tmin = csvLocale.toDouble(list.at(1));
        double tmax = csvLocale.toDouble(list.at(2));
        if (tmin >= tmax)
            return QJsonObject();

        json.insert(DATE_UNIFORM_MIN_STR, tmin);
        json.insert(DATE_UNIFORM_MAX_STR, tmax);
    }
    return json;
}

QStringList PluginUniform::toCSV(const QJsonObject& data, const QLocale& csvLocale) const
{
    QStringList list;
    list << csvLocale.toString(data.value(DATE_UNIFORM_MIN_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_UNIFORM_MAX_STR).toDouble());
    return list;
}

QString PluginUniform::getDateDesc(const Date* date) const
{
    Q_ASSERT(date);
    QLocale locale = QLocale();
    QString result;

    const QJsonObject &data = date->mData;
    result += QObject::tr("Interval") + QString(" : [ %1 ; %2 ] BC/AD").arg(locale.toString(data.value(DATE_UNIFORM_MIN_STR).toDouble()),
                                                                            locale.toString(data.value(DATE_UNIFORM_MAX_STR).toDouble()));

    return result;
}

bool PluginUniform::isDateValid(const QJsonObject& data, const ProjectSettings& settings)
{
    (void) data;
    (void) settings;
    /*double bmin = data[DATE_UNIFORM_MIN_STR].toDouble();
    double bmax = data[DATE_UNIFORM_MAX_STR].toDouble();
    return (bmax > settings.mTmin && bmin < settings.mTmax) ? true : false;*/
    
    return true;
 }
// ------------------------------------------------------------------

GraphViewRefAbstract* PluginUniform::getGraphViewRef()
{
    return nullptr;
}

PluginSettingsViewAbstract* PluginUniform::getSettingsView()
{
    return nullptr;
}


QPair<double,double> PluginUniform::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    const double min = data.value(DATE_UNIFORM_MIN_STR).toDouble();
    const double max = data.value(DATE_UNIFORM_MAX_STR).toDouble();
    return QPair<double, double>(min, max);
}

#endif
