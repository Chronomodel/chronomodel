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

#ifndef PLUGINF14C_H
#define PLUGINF14C_H

#if USE_PLUGIN_F14C

#include "PluginAbstract.h"

#define DATE_F14C_FRACTION_STR "fraction"
#define DATE_F14C_ERROR_STR "error"
#define DATE_F14C_DELTA_R_STR "delta_r"
#define DATE_F14C_DELTA_R_ERROR_STR "delta_r_error"
#define DATE_F14C_REF_CURVE_STR "ref_curve"


class DATATION_SHARED_EXPORT PluginF14C : public PluginAbstract
{
    Q_OBJECT

public:
    PluginF14C();
    virtual ~PluginF14C();

    long double getLikelihood(const double t, const QJsonObject &data) const noexcept override;
    bool withLikelihoodArg() override {return true; }
    std::pair<long double, long double > getLikelihoodArg(const double t, const QJsonObject &data) const noexcept override;

    // virtual function
    QString getName() const override;
    QIcon getIcon() const override;
    bool doesCalibration() const override;
    bool wiggleAllowed() const override;
#ifndef FIXEDPRIOR
    SamplerProposal getDataMethod() const override;
    QList<SamplerProposal> allowedDataMethods() const override;
#endif

    QStringList csvColumns() const override;
    qsizetype csvMinColumns() const override;
    qsizetype csvOptionalColumns() const override {return 2;} // Corresponding to  "ΔR" and "ΔR Error"
    QJsonObject fromCSV(const QStringList &list, const QLocale &csvLocale) const override;
    QStringList toCSV(const QJsonObject &data, const QLocale &csvLocale) const override;
    QString getDateDesc(const Date* date) const override;
    QString getDateRefCurveName(const Date* date) const override;

    PluginFormAbstract* getForm() override;
    GraphViewRefAbstract* getGraphViewRef() override;
    virtual void deleteGraphViewRef(GraphViewRefAbstract* graph) override;
    PluginSettingsViewAbstract* getSettingsView() override;
    QList<QHash<QString, QVariant>> getGroupedActions() override;

    QJsonObject checkValuesCompatibility(const QJsonObject &values) override;
    bool isDateValid(const QJsonObject &data, const StudyPeriodSettings &settings) override;

    bool areDatesMergeable(const QJsonArray &dates) override;
    QJsonObject mergeDates(const QJsonArray &dates) override;

    // ---------------------

    QString getRefExt() const override;
    QString getRefsPath() const override;
    RefCurve loadRefFile(QFileInfo refFile) override;

    double getRefValueAt(const QJsonObject &data, const double &t) const;
    double getRefErrorAt(const QJsonObject &data, const double &t) const;

    QPair<double, double> getTminTmaxRefsCurve(const QJsonObject &data) const override;
    double getMinStepRefsCurve(const QJsonObject &data) override;
};

// define in Plugin14C.h
//template <typename T>
//T F14CtoCRA ( const T F14C) { return( -8033*log(F14C) );}

//template <typename T>
//T errF14CtoErrCRA (const T errF14C, const T F14C) {return( -8033*log(F14C-errF14C) + 8033*log(F14C)); }
//

template <typename T>
T CRAtoF14C (const T CRA) { return(exp(-CRA/8033)); }
template <typename T>
T errCRAtoErrF14C (const T errCRA, const T CRA) { return( exp(-CRA/8033) -exp(-(CRA+ errCRA)/8033)); }

#endif

#endif


