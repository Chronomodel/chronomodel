/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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
    MHVariable::SamplerProposal getDataMethod() const override;
    QList<MHVariable::SamplerProposal> allowedDataMethods() const override;
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


