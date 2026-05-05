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

#ifndef PLUGINDENSITY_H
#define PLUGINDENSITY_H

#if USE_PLUGIN_DENSITY

#include "PluginAbstract.h"

#define DATE_DENSITY_CURVE_STR "curve"


class DATATION_SHARED_EXPORT PluginDensity : public PluginAbstract
{
    Q_OBJECT
public:
    PluginDensity();
    virtual ~PluginDensity();

    //virtual function
    long double getLikelihood(const double t, const QJsonObject &data) const noexcept override;
    bool withLikelihoodArg() override {return true; }
    std::pair<long double, long double > getLikelihoodArg(const double t, const QJsonObject &data) const noexcept override;
    
    QPair<double,double> getTminTmaxRefsCurve(const QJsonObject &data) const override;
    double getMinStepRefsCurve(const QJsonObject &data) override;

    QString getName() const override;
    QIcon getIcon() const override;
    bool doesCalibration() const override;
    bool wiggleAllowed() const override;

    MHVariable::SamplerProposal getDataMethod() const override;
    QList<MHVariable::SamplerProposal> allowedDataMethods() const override;
    QString csvHelp() const override;
    QStringList csvColumns() const override;
    qsizetype csvMinColumns() const override;
    QJsonObject fromCSV(const QStringList& list, const QLocale &csvLocale) const override;
    QStringList toCSV(const QJsonObject& data, const QLocale &csvLocale) const override;
    QString getDateDesc(const Date* date) const override;
    QString getDateRefCurveName(const Date* date) const override ;

    PluginFormAbstract* getForm() override;
    GraphViewRefAbstract* getGraphViewRef() override;
    virtual void deleteGraphViewRef(GraphViewRefAbstract* graph) override;
    PluginSettingsViewAbstract* getSettingsView() override;

    QJsonObject checkValuesCompatibility(const QJsonObject& values) override;
    bool isDateValid(const QJsonObject&, const StudyPeriodSettings&) override;

    bool areDatesMergeable(const QJsonArray& dates) override;
    QJsonObject mergeDates(const QJsonArray& dates) override;
    // ---------------------

    QString getRefExt() const override;
    QString getRefsPath() const override;
    RefCurve loadRefFile(QFileInfo refFile) override;

    double getRefValueAt(const QJsonObject& data, const double& t);
    double getRefErrorAt(const QJsonObject& data, const double& t, const QString mode);


};

#endif
#endif
