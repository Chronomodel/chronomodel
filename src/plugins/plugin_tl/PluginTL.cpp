/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "PluginTL.h"
#if USE_PLUGIN_TL

#include "PluginTLForm.h"
#include "PluginTLRefView.h"
#include "Generator.h"

#include <QJsonObject>
#include <QtWidgets>

#include <cstdlib>

const unsigned k_sigma = 5;

PluginTL::PluginTL()
{
    mColor = QColor(216, 207, 52);
}

PluginTL::~PluginTL()
{
    if (mRefGraph)
        delete mRefGraph;
}

long double PluginTL::getLikelihood(const double t, const QJsonObject &data) const noexcept
{
    /* ------------------------------------------------------------------
     * 1️⃣  Extract the three numbers from the JSON object and promote them
     *     to long double once.  This avoids repeated casts later on.
     * ------------------------------------------------------------------ */
    const long double age      = static_cast<long double>(data.value(DATE_TL_AGE_STR).toDouble());
    const long double error    = static_cast<long double>(data.value(DATE_TL_ERROR_STR).toDouble());
    const long double refYear  = static_cast<long double>(data.value(DATE_TL_REF_YEAR_STR).toDouble());

    /* ------------------------------------------------------------------
     * 2️⃣  Compute the exponent:
     *     diff = age - (refYear - t)
     *     norm = diff / error
     *     exponent = -0.5 * norm²
     * ------------------------------------------------------------------ */
    const long double diff = age - (refYear - static_cast<long double>(t));
    const long double norm = diff / error;
    const long double exponent = -0.5L * norm * norm;   // same as powl(norm,2.0L) but faster

    /* ------------------------------------------------------------------
     * 3️⃣   Gaussian value (normalisation factor = 1 / error)
     * ------------------------------------------------------------------ */
    const long double likelihood = std::exp(exponent) / error;

    return likelihood;
}


std::pair<long double, long double> PluginTL::getLikelihoodArg(double t, const QJsonObject &data) const noexcept
{
    // ------------------------------------------------------------------
    // 1️⃣  Extract the three required numbers (still double from JSON)
    // ------------------------------------------------------------------
    const long double age      = static_cast<long double>(data.value(DATE_TL_AGE_STR).toDouble());
    const long double error    = static_cast<long double>(data.value(DATE_TL_ERROR_STR).toDouble());
    const long double refYear  = static_cast<long double>(data.value(DATE_TL_REF_YEAR_STR).toDouble());
    // ------------------------------------------------------------------
    // 2️⃣  Compute the variance = 1 / error²
    // ------------------------------------------------------------------
    const long double variance = 1.0L / (error * error);
    // ------------------------------------------------------------------
    // 3️⃣  Compute the exponent.
    //     (age - (refYear - t)) / error  →  then square it.
    // ------------------------------------------------------------------
    const long double diff = age - (refYear - t);
    const long double norm = diff / error;
    const long double exponent = -0.5L * norm * norm;   // same as powl(norm, 2.0L)
    // ------------------------------------------------------------------
    // 4️⃣  Return the pair.
    // ------------------------------------------------------------------
    return { variance, exponent };
}
/*
std::pair<long double, long double> PluginTL::getLikelihoodArg(const double t, const QJsonObject &data) const
{
    QPair<long double, long double> result;
    const double age = data.value(DATE_TL_AGE_STR).toDouble();
    const double error = data.value(DATE_TL_ERROR_STR).toDouble();
    const double ref_year = data.value(DATE_TL_REF_YEAR_STR).toDouble();

    // gaussian TL
    result = QPair<long double,long double>(1.l/(error*error), (-0.5l * powl((long double)(age - (ref_year - t)) / error, 2.l))) ;
    return result;
}*/

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
#ifndef FIXEDPRIOR
SamplerProposal PluginTL::getDataMethod() const
{
    return SamplerProposal::eDatePrior;
}

QList<SamplerProposal> PluginTL::allowedDataMethods() const
{
    QList<SamplerProposal> methods;
    methods.append(SamplerProposal::eDatePrior);
    methods.append(SamplerProposal::eLikelihood);
    methods.append(SamplerProposal::eRWAdaptGauss);
    return methods;
}
#endif
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
QJsonObject PluginTL::checkValuesCompatibility(const QJsonObject &values)
{
    QJsonObject result = values;

    //force type double
    result[DATE_TL_AGE_STR] = result.value(DATE_TL_AGE_STR).toDouble();
    result[DATE_TL_ERROR_STR] = result.value(DATE_TL_ERROR_STR).toDouble();
    result[DATE_TL_REF_YEAR_STR] = result.value(DATE_TL_REF_YEAR_STR).toDouble();

    return result;
}
QJsonObject PluginTL::fromCSV(const QStringList &list, const QLocale &csvLocale) const
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

QStringList PluginTL::toCSV(const QJsonObject &data, const QLocale &csvLocale) const
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
        result += QObject::tr("Age = %1  ± %2").arg(locale.toString(data.value(DATE_TL_AGE_STR).toDouble()), locale.toString(data.value(DATE_TL_ERROR_STR).toDouble()));
        result += ": " + QObject::tr("Ref. year = %1").arg(locale.toString(data.value(DATE_TL_REF_YEAR_STR).toDouble()));

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

QPair<double,double> PluginTL::getTminTmaxRefsCurve(const QJsonObject &data) const
{
    const double age = data.value(DATE_TL_AGE_STR).toDouble();
    const double error = data.value(DATE_TL_ERROR_STR).toDouble();
    const double ref_year = data.value(DATE_TL_REF_YEAR_STR).toDouble();

    double tmin = ref_year - age - k_sigma * error;
    double tmax = ref_year - age + k_sigma * error;

    return QPair<double, double>(tmin, tmax);
}

double PluginTL::getMinStepRefsCurve(const QJsonObject &data)
{
    constexpr double frac = 1.0 / 1001;
    const double age = data.value(DATE_TL_AGE_STR).toDouble();
    const double error = data.value(DATE_TL_ERROR_STR).toDouble();
    const double ref_year = data.value(DATE_TL_REF_YEAR_STR).toDouble();

    const double tmin = ref_year - age - k_sigma * error;
    const double tmax = ref_year - age + k_sigma * error;

    return std::abs(tmax-tmin) * frac;


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
 * @brief Combine several TL
 **/
QJsonObject PluginTL::mergeDates(const QJsonArray &dates)
{
    QJsonObject result;
    if (dates.size() > 1) {
       
        QJsonObject mergedData;

        mergedData[DATE_TL_AGE_STR] = 100;
        mergedData[DATE_TL_ERROR_STR] = 100;
        mergedData[DATE_TL_REF_YEAR_STR] = 0;
        
        
        QStringList names;

        for (auto&& d: dates)
            names.append(d.toObject().value(STATE_NAME).toString());

        // inherits the first data propeties as plug-in and method...
        result = dates.at(0).toObject();
        result[STATE_NAME] =  names.join(" | ");
        result[STATE_DATE_UUID] = QString::fromStdString( Generator::UUID());
        result[STATE_DATE_DATA] = mergedData;
        result[STATE_DATE_ORIGIN] = Date::eCombination;
        result[STATE_DATE_SUB_DATES] = dates;
        result[STATE_DATE_VALID] = true;

        result[STATE_DATE_DELTA_TYPE] = Date::eDeltaNone;
        

    } else
        result["error"] = tr("Combine needs at least 2 data !");

    return result;

}


#endif
