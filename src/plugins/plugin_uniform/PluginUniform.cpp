/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2020

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "PluginUniform.h"
#if USE_PLUGIN_UNIFORM

#include "PluginUniformForm.h"
#include "PluginUniformRefView.h"

#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>

PluginUniform::PluginUniform()
{
    mColor = QColor(100, 50, 140);
}

PluginUniform::~PluginUniform()
{

}

long double PluginUniform::getLikelihood(const double& t, const QJsonObject& data)
{
    const double min = data.value(DATE_UNIFORM_MIN_STR).toDouble();
    const double max = data.value(DATE_UNIFORM_MAX_STR).toDouble();

    return (t >= min && t <= max) ? static_cast<long double>(1. / (max-min)) : 0.l;
}

QString PluginUniform::getName() const
{
    return QString("Unif");
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
        const double tmin = csvLocale.toDouble(list.at(1));
        const double tmax = csvLocale.toDouble(list.at(2));
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

    return true;
 }
// ------------------------------------------------------------------


GraphViewRefAbstract* PluginUniform::getGraphViewRef()
{
    mRefGraph = new PluginUniformRefView();
    return mRefGraph;
}
void PluginUniform::deleteGraphViewRef(GraphViewRefAbstract* graph)
{
    if (graph)
        delete static_cast<PluginUniformRefView*>(graph);

    graph = nullptr;
    mRefGraph = nullptr;
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

long double PluginUniform::getLikelihoodCombine(const double& t, const QJsonArray& subData)
{
    double min (-INFINITY);
    double max (INFINITY);
           //QJsonArray subDates = data.value(STATE_DATE_SUB_DATES).toArray();
      
   for (int i(0); i<subData.size(); ++i) {
       const QJsonObject subDate = subData.at(i).toObject();
       const QJsonObject data = subDate.value(STATE_DATE_DATA).toObject();

       min = std::max(min, data.value(DATE_UNIFORM_MIN_STR).toDouble() );
       max = std::min(max, data.value(DATE_UNIFORM_MAX_STR).toDouble() );
       
       
       
   }

    return (t >= min && t <= max) ? static_cast<long double>(1. / (max-min)) : 0.l;
    
    
}

bool PluginUniform::areDatesMergeable(const QJsonArray& dates)
{
    int pluginID = dates.at(0).toObject().value(STATE_DATE_DATA).toObject().value(STATE_DATE_PLUGIN_ID).toInt();
    for (int i=1; i<dates.size(); ++i) {
        QJsonObject date = dates.at(i).toObject();
        QJsonObject data = date.value(STATE_DATE_DATA).toObject();

        if (pluginID != data.value(STATE_DATE_PLUGIN_ID).toInt())
            return false;
    }
    return true;
}

/**
 * @brief Combine several Uniform Interval
 **/
QJsonObject PluginUniform::mergeDates(const QJsonArray& dates)
{
    QJsonObject result;
    if (dates.size() > 1) {
       
        
        QStringList names;
        
        double min (-INFINITY);
        double max (INFINITY);
        for (int i=0; i<dates.size(); ++i) {
            const QJsonObject date = dates.at(i).toObject();
          
            names.append(date.value(STATE_NAME).toString());
            
            min = std::max(min, date.value(STATE_DATE_DATA).toObject().value(DATE_UNIFORM_MIN_STR).toDouble() );
            max = std::min(max, date.value(STATE_DATE_DATA).toObject().value(DATE_UNIFORM_MAX_STR).toDouble() );
        
        }
             
        
        QJsonObject mergedData;
        
        mergedData[DATE_UNIFORM_MIN_STR] = min;
        mergedData[DATE_UNIFORM_MAX_STR] = max;
        
        // inherits the first data propeties as plug-in and method...
        result = dates.at(0).toObject();
        result[STATE_NAME] = "Combined ( " + names.join(" | ") + " )";
        
        result[STATE_DATE_DATA] = mergedData;
        result[STATE_DATE_ORIGIN] = Date::eSingleDate;
        result[STATE_DATE_SUB_DATES] = dates;
        result[STATE_DATE_VALID] = (max>min ? true : false);

    } else
        result["error"] = tr("Combine needs at least 2 data !");

    return result;

}
#endif

