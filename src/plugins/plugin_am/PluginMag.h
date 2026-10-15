/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE
	Komlan NOUKPOAPE

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

#ifndef PLUGINMAG_H
#define PLUGINMAG_H

#if USE_PLUGIN_AM

#include "../PluginAbstract.h"


const long double rad = M_PIl/180.l;

enum ProcessTypeAM
{
    eCombine = -1,
    eNone = 0,
    eInc = 1,
    eDec = 2,
    eField = 3,
    eID = 4,
    eIF = 5,
    eIDF = 6
};

class PluginMagRefView;

#define DATE_AM_PROCESS_TYPE_STR "process_type"

#define DATE_AM_ALPHA95_STR "alpha95"
#define DATE_AM_ERROR_F_STR "error_f"

#define DATE_AM_INC_STR "inc"
#define DATE_AM_DEC_STR "dec"
#define DATE_AM_FIELD_STR "field"

#define DATE_AM_ITERATION_STR "integration_steps"

#define DATE_AM_REF_CURVEI_STR "refI_curve"
#define DATE_AM_REF_CURVED_STR "refD_curve"
#define DATE_AM_REF_CURVEF_STR "refF_curve"


class DATATION_SHARED_EXPORT PluginMag : public PluginAbstract
{
    Q_OBJECT

public:
    PluginMag();
    virtual ~PluginMag();

    long double Likelihood(const double t, const QJsonObject &data) const;

     // virtual function
    long double getLikelihood(const double t, const QJsonObject &data) const noexcept override;
    bool withLikelihoodArg() override {return false; }
    std::pair<long double, long double > getLikelihoodArg(const double t, const QJsonObject &data) const noexcept override;


    bool areDatesMergeable(const QJsonArray &dates) override;
    QJsonObject mergeDates(const QJsonArray &dates) override;

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
    QJsonObject checkValuesCompatibility(const QJsonObject &values) override;

    QString getDateDesc(const Date* date) const override;
    QString getDateRefCurveName(const Date* date) const override;

    PluginFormAbstract* getForm() override;
    GraphViewRefAbstract* getGraphViewRef() override;
    virtual void deleteGraphViewRef(GraphViewRefAbstract* graph) override;
    PluginSettingsViewAbstract* getSettingsView() override;

    bool isDateValid(const QJsonObject& data, const StudyPeriodSettings& settings) override;

    // ---------------------
    QString getRefExt() const override;
    QString getRefsPath() const override;
    RefCurve loadRefFile(QFileInfo refFile) override;

    double getRefValueAt(const QJsonObject& data, const double t);
    double getRefErrorAt(const QJsonObject& data, const double t);

    QPair<double,double> getTminTmaxRefsCurve(const QJsonObject& data) const override;
    double getMinStepRefsCurve(const QJsonObject &data) override;

private:
    bool measureIsValidForCurve(const double m, const QString &ref, const QJsonObject &data, const StudyPeriodSettings &settings);
};

#endif
#endif
