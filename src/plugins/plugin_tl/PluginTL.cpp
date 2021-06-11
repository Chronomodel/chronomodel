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
    result = QPair<long double,long double>(1.l/(error*error), (-0.5l * powl((long double)(age - (ref_year - t)) / error, 2.l))) ;
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

MHVariable::SamplerProposal PluginTL::getDataMethod() const
{
    return MHVariable::eMHSymGaussAdapt;
}

QList<MHVariable::SamplerProposal> PluginTL::allowedDataMethods() const
{
    QList<MHVariable::SamplerProposal> methods;
    methods.append(MHVariable::eMHSymetric);
    methods.append(MHVariable::eInversion);
    methods.append(MHVariable::eMHSymGaussAdapt);
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

// Convert old project versions
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
    Q_ASSERT(date);
    QString result;
    
    const QLocale locale = QLocale();
    
    if (date->mOrigin == Date::eSingleDate) {
        QJsonObject data = date->mData;
        result += QObject::tr("Age : %1  ± %2").arg(locale.toString(data.value(DATE_TL_AGE_STR).toDouble()), locale.toString(data.value(DATE_TL_ERROR_STR).toDouble()));
        result += "; " + QObject::tr("Ref. year : %1").arg(locale.toString(data.value(DATE_TL_REF_YEAR_STR).toDouble()));

    } else {
         result = "Combine (";
        QStringList datesDesc;
        for (auto&& d: date->mSubDates) {
            Date subDate (d.toObject() );
            datesDesc.append(getDateDesc(&subDate));
        }
        result +=  datesDesc.join(" | ") + " )" ;
    }
    
    return result;
}

QPair<double,double> PluginTL::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    const double age = data.value(DATE_TL_AGE_STR).toDouble();
    const double error = data.value(DATE_TL_ERROR_STR).toDouble();
    const double ref_year = data.value(DATE_TL_REF_YEAR_STR).toDouble();

    const double k = 5.;

    double tmin = ref_year - age - k * error;
    double tmax = ref_year - age + k * error;

    return QPair<double,double>(tmin, tmax);
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


// Combine / Split
bool PluginTL::areDatesMergeable(const QJsonArray& )
{
    return true;
}
/**
 * @brief Combine several Gauss Age
 **/
QJsonObject PluginTL::mergeDates(const QJsonArray& dates)
{
    QJsonObject result;
    if (dates.size() > 1) {
       
        QJsonObject mergedData;

        mergedData[DATE_TL_AGE_STR] = 100;
        mergedData[DATE_TL_ERROR_STR] = 100;
        mergedData[DATE_TL_REF_YEAR_STR] = 0;
        
        
        QStringList names;

        for (auto && d: dates)
            names.append(d.toObject().value(STATE_NAME).toString());

        // inherits the first data propeties as plug-in and method...
        result = dates.at(0).toObject();
        result[STATE_NAME] =  names.join(" | ");
        result[STATE_DATE_DATA] = mergedData;
        result[STATE_DATE_ORIGIN] = Date::eCombination;
        result[STATE_DATE_SUB_DATES] = dates;
        result[STATE_DATE_VALID] = true;
        

    } else
        result["error"] = tr("Combine needs at least 2 data !");

    return result;

}
QPair<double,double> PluginTL::getTminTmaxRefsCurveCombine(const QJsonArray& subData)
{
    double tmin (INFINITY);
    double tmax (-INFINITY);
    QPair<double, double> tminTmax (tmin,tmax);

    for (auto&& d: subData)   {
        tminTmax = getTminTmaxRefsCurve( d.toObject().value(STATE_DATE_DATA).toObject() );
        tmin = std::min(tmin, tminTmax.first);
        tmax = std::max(tmax, tminTmax.second);
        
    }
    return qMakePair(tmin, tmax);
}


#endif
