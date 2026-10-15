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

#ifndef PLUGINGAUSS_H
#define PLUGINGAUSS_H

#if USE_PLUGIN_GAUSS

#include "PluginAbstract.h"

#define DATE_GAUSS_AGE_STR "age"
#define DATE_GAUSS_ERROR_STR "error"
#define DATE_GAUSS_A_STR "a"
#define DATE_GAUSS_B_STR "b"
#define DATE_GAUSS_C_STR "c"
#define DATE_GAUSS_MODE_STR "mode"
#define DATE_GAUSS_CURVE_STR "curve"

#define DATE_GAUSS_MODE_CURVE "curve"
#define DATE_GAUSS_MODE_EQ "eq"
#define DATE_GAUSS_MODE_NONE "none"


class DATATION_SHARED_EXPORT PluginGauss : public PluginAbstract
{
    Q_OBJECT
public:
    PluginGauss();
    virtual ~PluginGauss();

    //virtual function
    long double getLikelihood(const double t, const QJsonObject& data) const noexcept override;
    bool withLikelihoodArg() override {return true; }
    std::pair<long double, long double > getLikelihoodArg(const double t, const QJsonObject &data) const noexcept override;
    
    QPair<double,double> getTminTmaxRefsCurve(const QJsonObject &data) const override;
    double getMinStepRefsCurve(const QJsonObject &data) override;

    QString getName() const override;
    QIcon getIcon() const override;
    bool doesCalibration() const override;
    bool wiggleAllowed() const override;
#ifndef FIXEDPRIOR
    SamplerProposal getDataMethod() const override;
    QList<SamplerProposal> allowedDataMethods() const override;
#endif
    QString csvHelp() const override;
    QStringList csvColumns() const override;
    qsizetype csvMinColumns() const override;
    QJsonObject fromCSV(const QStringList &list, const QLocale &csvLocale) const override;
    QStringList toCSV(const QJsonObject &data, const QLocale &csvLocale) const override;
    QString getDateDesc(const Date* date) const override;
    QString getDateRefCurveName(const Date* date) const override ;

    PluginFormAbstract* getForm() override;
    GraphViewRefAbstract* getGraphViewRef() override;
    virtual void deleteGraphViewRef(GraphViewRefAbstract* graph) override;
    PluginSettingsViewAbstract* getSettingsView() override;

    QJsonObject checkValuesCompatibility(const QJsonObject &values) override;
    bool isDateValid(const QJsonObject &data, const StudyPeriodSettings &settings) override;

    bool areDatesMergeable(const QJsonArray &dates) override;
    QJsonObject mergeDates(const QJsonArray &dates) override;
    // ---------------------

    QString getRefExt() const override;
    QString getRefsPath() const override;
    RefCurve loadRefFile(QFileInfo refFile) override;

    long double getRefValueAt(const QJsonObject &data, const double t) const;
    double getRefErrorAt(const QJsonObject &data, const double t, const QString mode) const;


};

#endif
#endif
