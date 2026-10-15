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

#ifndef PLUGINUNIFORM_H
#define PLUGINUNIFORM_H

#if USE_PLUGIN_UNIFORM

#include "PluginAbstract.h"

#define DATE_UNIFORM_MIN_STR "min"
#define DATE_UNIFORM_MAX_STR "max"


class DATATION_SHARED_EXPORT PluginUniform : public PluginAbstract
{
    Q_OBJECT
 public:
    PluginUniform();
    virtual ~PluginUniform();

    bool areDatesMergeable(const QJsonArray &dates) override;
    QJsonObject mergeDates(const QJsonArray &dates) override;

    long double getLikelihood(const double t, const QJsonObject &data) const  noexcept override;
    bool withLikelihoodArg() override {return false; }
    long double getLikelihoodCombine(const double t, const QJsonArray &data);

    QString getName() const override;
    QIcon getIcon() const override;
    bool doesCalibration() const override;
    bool wiggleAllowed() const override;
#ifndef FIXEDPRIOR
    SamplerProposal getDataMethod() const override;
    QList<SamplerProposal> allowedDataMethods() const override;
#endif
    QStringList csvColumns() const override;
    QJsonObject fromCSV(const QStringList& list, const QLocale &csvLocale) const override;
    QStringList toCSV(const QJsonObject& data, const QLocale &csvLocale) const override;
    QString getDateDesc(const Date* date) const override;
    QJsonObject checkValuesCompatibility(const QJsonObject& values) override;
    bool isDateValid(const QJsonObject& data, const StudyPeriodSettings& ) override;

    PluginFormAbstract* getForm() override;
    GraphViewRefAbstract* getGraphViewRef() override;

    PluginSettingsViewAbstract* getSettingsView() override;

    QPair<double,double> getTminTmaxRefsCurve(const QJsonObject& data) const override;
    double getMinStepRefsCurve(const QJsonObject &data) override;

    void deleteGraphViewRef(GraphViewRefAbstract* graph) override;

};

#endif
#endif
