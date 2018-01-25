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
    mColor = QColor(216, 207, 52);
}

PluginTL::~PluginTL()
{
    if (mRefGraph)
        delete mRefGraph;
}

long double PluginTL::getLikelihood(const double& t, const QJsonObject& data)
{
    const double age = data.value(DATE_TL_AGE_STR).toDouble();
    const long double error = (long double)data.value(DATE_TL_ERROR_STR).toDouble();
    const double ref_year = data.value(DATE_TL_REF_YEAR_STR).toDouble();
    
    // gaussian TL
    const long double v = expl(-0.5l * powl((long double)(age - (ref_year - t)) / error, 2.l)) / error;
    return v;
}

QPair<long double, long double> PluginTL::getLikelihoodArg(const double& t, const QJsonObject& data)
{
    QPair<long double, long double> result;
    const double age = data.value(DATE_TL_AGE_STR).toDouble();
    const double error = data.value(DATE_TL_ERROR_STR).toDouble();
    const double ref_year = data.value(DATE_TL_REF_YEAR_STR).toDouble();
    
    // gaussian TL
    result= QPair<long double,long double>(1/(error*error), (-0.5f * powl((long double)(age - (ref_year - t)) / error, 2.l))) ;
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
    cols << "Data Name" << "Age" << "Error (sd)" << "Reference year";
    return cols;
}

PluginFormAbstract* PluginTL::getForm()
{
    PluginTLForm* form = new PluginTLForm(this);
    return form;
}

//#pragma mark Convert old project versions
QJsonObject PluginTL::checkValuesCompatibility(const QJsonObject& values)
{
    QJsonObject result = values;

    //force type double
    result[DATE_TL_AGE_STR] = result.value(DATE_TL_AGE_STR).toDouble();
    result[DATE_TL_ERROR_STR] = result.value(DATE_TL_ERROR_STR).toDouble();
    result[DATE_TL_REF_YEAR_STR] = result.value(DATE_TL_REF_YEAR_STR).toDouble();

    return result;
}
QJsonObject PluginTL::fromCSV(const QStringList& list, const QLocale& csvLocale)
{
    QJsonObject json;
    if (list.size() >= csvMinColumns()) {
        json.insert(DATE_TL_AGE_STR, csvLocale.toDouble(list.at(1)));
        const double error = csvLocale.toDouble(list.at(2));
        if (error == 0)
            return QJsonObject();
        json.insert(DATE_TL_ERROR_STR, error);
        json.insert(DATE_TL_REF_YEAR_STR, csvLocale.toDouble(list.at(3)));
    }
    return json;
}

QStringList PluginTL::toCSV(const QJsonObject& data, const QLocale& csvLocale) const
{
    QStringList list;
    list << csvLocale.toString(data.value(DATE_TL_AGE_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_TL_ERROR_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_TL_REF_YEAR_STR).toDouble());
    return list;
}

QString PluginTL::getDateDesc(const Date* date) const
{
    const QLocale locale = QLocale();
    QString result;
    if (date) {
        QJsonObject data = date->mData;
        result += QObject::tr("Age : %1  ± %2").arg(locale.toString(data.value(DATE_TL_AGE_STR).toDouble()), locale.toString(data.value(DATE_TL_ERROR_STR).toDouble()));
        result += "; " + QObject::tr("Ref. year : %1").arg(locale.toString(data.value(DATE_TL_REF_YEAR_STR).toDouble()));
    }
    return result;
}

QPair<double,double> PluginTL::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    const double age = data.value(DATE_TL_AGE_STR).toDouble();
    const double error = data.value(DATE_TL_ERROR_STR).toDouble();
    const double ref_year = data.value(DATE_TL_REF_YEAR_STR).toDouble();
    
    const double k = 5.;
    
    const double tmin = ref_year - age - k * error;
    const double tmax = ref_year - age + k * error;
    
    return qMakePair<double,double>(tmin, tmax);
}




// ------------------------------------------------------------------

GraphViewRefAbstract* PluginTL::getGraphViewRef()
{
    mRefGraph = new PluginTLRefView();
    return mRefGraph;
}
void PluginTL::deleteGraphViewRef(GraphViewRefAbstract* graph)
{
    if (graph)
        delete static_cast<PluginTLRefView*>(graph);

    graph = nullptr;
    mRefGraph = nullptr;
}
PluginSettingsViewAbstract* PluginTL::getSettingsView()
{
    return nullptr;
}

#endif
