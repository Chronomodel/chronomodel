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

#ifndef PLUGINTL_H
#define PLUGINTL_H

#if USE_PLUGIN_TL

#include <PluginAbstract.h>

#define DATE_TL_AGE_STR "age"
#define DATE_TL_ERROR_STR "error"
#define DATE_TL_REF_YEAR_STR "ref_year"


class DATATION_SHARED_EXPORT PluginTL : public PluginAbstract
{
    Q_OBJECT
    //Q_PLUGIN_METADATA(IID "chronomodel.PluginAbstract.PluginTL")
    //Q_INTERFACES(PluginAbstract)
public:
    PluginTL();
    virtual ~PluginTL();

    //virtual function
    
    bool withLikelihoodArg() {return true; }
    
    long double getLikelihood(const double t, const QJsonObject &data);
    QPair<long double, long double > getLikelihoodArg(const double t, const QJsonObject &data);
    QPair<double, double> getTminTmaxRefsCurve(const QJsonObject &data) const;
    double getMinStepRefsCurve(const QJsonObject &data) const;
    
    bool areDatesMergeable(const QJsonArray &dates);
    QJsonObject mergeDates(const QJsonArray &dates);

    QString getName() const;
    QIcon getIcon() const;
    bool doesCalibration() const;
    bool wiggleAllowed() const;
    MHVariable::SamplerProposal getDataMethod() const;
    QList<MHVariable::SamplerProposal> allowedDataMethods() const;
    QStringList csvColumns() const;
    QJsonObject fromCSV(const QStringList &list, const QLocale &csvLocale);
    QStringList toCSV(const QJsonObject &data, const QLocale &csvLocale) const;
    QString getDateDesc(const Date* date) const;
    QJsonObject checkValuesCompatibility(const QJsonObject &values);

    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
    void deleteGraphViewRef(GraphViewRefAbstract* graph);
    PluginSettingsViewAbstract* getSettingsView();
};

#endif
#endif
