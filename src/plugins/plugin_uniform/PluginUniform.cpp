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

#include "PluginUniform.h"
#if USE_PLUGIN_UNIFORM

#include "PluginUniformForm.h"
#include "PluginUniformRefView.h"
#include "Generator.h"

#include <QJsonObject>
#include <QtWidgets>



PluginUniform::PluginUniform()
{
    mColor = QColor(100, 50, 140);
}

PluginUniform::~PluginUniform()
{

}

long double PluginUniform::getLikelihood(const double t, const QJsonObject &data) const noexcept
{
    const double min = data.value(DATE_UNIFORM_MIN_STR).toDouble();
    const double max = data.value(DATE_UNIFORM_MAX_STR).toDouble();

    return (t >= min && t <= max) ? static_cast<long double>(1.0 / (max-min)) : 0.0l;
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
    return true;
}
#ifndef FIXEDPRIOR
SamplerProposal PluginUniform::getDataMethod() const
{
    return SamplerProposal::eLikelihood;
}

QList<SamplerProposal> PluginUniform::allowedDataMethods() const
{
    QList<SamplerProposal> methods;
    methods.append(SamplerProposal::eDatePrior);
    methods.append(SamplerProposal::eLikelihood); // since version v3.2.4
    methods.append(SamplerProposal::eRWAdaptGauss);
    return methods;
}
#endif
QStringList PluginUniform::csvColumns() const
{
    QStringList cols;
    cols << "Name" << "Lower date" << "Upper date" <<"";
    return cols;
}

PluginFormAbstract* PluginUniform::getForm()
{
    PluginUniformForm* form = new PluginUniformForm(this);
    return form;
}
// Convert old project versions
QJsonObject PluginUniform::checkValuesCompatibility(const QJsonObject &values)
{
    QJsonObject result = values;

    //force type double
    result[DATE_UNIFORM_MIN_STR] = result.value(DATE_UNIFORM_MIN_STR).toDouble();
    result[DATE_UNIFORM_MAX_STR] = result.value(DATE_UNIFORM_MAX_STR).toDouble();

    return result;
}

QJsonObject PluginUniform::fromCSV(const QStringList &list, const QLocale &csvLocale) const
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

QStringList PluginUniform::toCSV(const QJsonObject &data, const QLocale &csvLocale) const
{
    QStringList list;
    list << csvLocale.toString(data.value(DATE_UNIFORM_MIN_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_UNIFORM_MAX_STR).toDouble());
    list << "";
    return list;
}

QString PluginUniform::getDateDesc(const Date* date) const
{
    Q_ASSERT(date);
    QLocale locale = QLocale();
    QString result;

    const QJsonObject &data = date->mData;
    result += QObject::tr("Interval") + QString(" = [ %1 : %2 ] BC/AD").arg(locale.toString(data.value(DATE_UNIFORM_MIN_STR).toDouble()),
                                                                            locale.toString(data.value(DATE_UNIFORM_MAX_STR).toDouble()));

    return result;
}

bool PluginUniform::isDateValid(const QJsonObject &data, const StudyPeriodSettings &)
{
    const double min = data.value(DATE_UNIFORM_MIN_STR).toDouble();
    const double max = data.value(DATE_UNIFORM_MAX_STR).toDouble();
    return max > min;

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

QPair<double, double> PluginUniform::getTminTmaxRefsCurve(const QJsonObject &data) const
{
    const double min = data.value(DATE_UNIFORM_MIN_STR).toDouble();
    const double max = data.value(DATE_UNIFORM_MAX_STR).toDouble();
    return QPair<double, double>(min, max);
}

double PluginUniform::getMinStepRefsCurve(const QJsonObject &data)
{
    const double min = data.value(DATE_UNIFORM_MIN_STR).toDouble();
    const double max = data.value(DATE_UNIFORM_MAX_STR).toDouble();

    constexpr double frac = 1.0/ 51.0;
    return std::abs(max-min) * frac;
}

long double PluginUniform::getLikelihoodCombine(const double t, const QJsonArray &subData)
{
    double min (-INFINITY);
    double max (INFINITY);

   for (auto&& d : subData) {
       const QJsonObject &data = d.toObject().value(STATE_DATE_DATA).toObject();

       min = std::max(min, data.value(DATE_UNIFORM_MIN_STR).toDouble() );
       max = std::min(max, data.value(DATE_UNIFORM_MAX_STR).toDouble() );
   }

    return (t >= min && t <= max) ? static_cast<long double>(1.0L / (max-min)) : 0.0L;
    
    
}

bool PluginUniform::areDatesMergeable(const QJsonArray& dates)
{
    const int pluginID = dates.at(0).toObject().value(STATE_DATE_DATA).toObject().value(STATE_DATE_PLUGIN_ID).toInt();

    for (auto&& d:dates) {
        if (pluginID != d.toObject().value(STATE_DATE_DATA).toObject().value(STATE_DATE_PLUGIN_ID).toInt())
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

        bool withWiggle (false);

        QJsonObject mergedData;
        QStringList names;
        double min (-INFINITY);
        double max (+INFINITY);

        double Wmin (+INFINITY);
        double Wmax (-INFINITY);

        const auto & project = MainWindow::getInstance()->getProject();

        for (auto&& d : dates ) {
            names.append(d.toObject().value(STATE_NAME).toString());

            const bool hasWiggle (d.toObject().value(STATE_DATE_DELTA_TYPE).toInt() != Date::eDeltaNone);

            // wiggle existence test
            withWiggle = withWiggle || hasWiggle;

            if (hasWiggle) {
                const std::string toFind = "WID::" + d.toObject().value(STATE_DATE_UUID).toString().toStdString();
                auto it = project->mCalibCurves.find(toFind);

                if ( it!=project->mCalibCurves.end()) {
                    auto d_mCalibration (& it->second);
                    min = std::max(d_mCalibration->mTmin, min);
                    max = std::min(d_mCalibration->mTmax, max);

                } else {
                    min = +INFINITY;
                    max = -INFINITY;
                }


            } else {

                const QJsonObject &data = d.toObject().value(STATE_DATE_DATA).toObject();
                min = std::max(min, data.value(DATE_UNIFORM_MIN_STR).toDouble() );
                max = std::min(max, data.value(DATE_UNIFORM_MAX_STR).toDouble() );

                Wmin = std::min(Wmin, data.value(DATE_UNIFORM_MIN_STR).toDouble() );
                Wmax = std::max(Wmax, data.value(DATE_UNIFORM_MAX_STR).toDouble() );
            }


        }


        if (min >= max) {
            result["error"] = tr("Combine is not possible, not enough coincident densities");
            return result;
        }

        // inherits the first data propeties as plug-in and method...
        result = dates.at(0).toObject();
        result[STATE_NAME] = names.join(" | ");
        result[STATE_DATE_UUID] = QString::fromStdString( Generator::UUID());


        if (withWiggle) {
            result[STATE_DATE_ORIGIN] = Date::eCombination;

            result[STATE_DATE_VALID] = (max>min ? true : false);
            result[STATE_DATE_DELTA_TYPE] = Date::eDeltaNone;

            mergedData[DATE_UNIFORM_MIN_STR] = Wmin;
            mergedData[DATE_UNIFORM_MAX_STR] = Wmax;

        } else {
            result[STATE_DATE_ORIGIN] = Date::eSingleDate;
            result[STATE_DATE_VALID] = (max>min ? true : false);
            mergedData[DATE_UNIFORM_MIN_STR] = min;
            mergedData[DATE_UNIFORM_MAX_STR] = max;
        }
        result[STATE_DATE_DATA] = mergedData;
        result[STATE_DATE_SUB_DATES] = dates;

    } else
        result["error"] = tr("Combine needs at least 2 data !");

    return result;

}
#endif

