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

#ifndef PLUGINF14C_H
#define PLUGINF14C_H

#if USE_PLUGIN_F14C

#include "../PluginAbstract.h"

#define DATE_F14C_FRACTION_STR "fraction"
#define DATE_F14C_ERROR_STR "error"
#define DATE_F14C_DELTA_R_STR "delta_r"
#define DATE_F14C_DELTA_R_ERROR_STR "delta_r_error"
#define DATE_F14C_REF_CURVE_STR "ref_curve"


class DATATION_SHARED_EXPORT PluginF14C : public PluginAbstract
{
    Q_OBJECT
    //Q_PLUGIN_METADATA(IID "chronomodel.PluginAbstract.PluginF14C")
    //Q_INTERFACES(PluginAbstract)
public:
    PluginF14C();
    virtual ~PluginF14C();

    long double getLikelihood(const double& t, const QJsonObject& data);
    bool withLikelihoodArg() {return true; }
    QPair<long double, long double > getLikelihoodArg(const double& t, const QJsonObject& data);
//    long double getLikelihoodCombine(const double& t, const QJsonArray& data);
//       QPair<long double, long double > getLikelihoodArgCombine(const double& t, const QJsonArray& data);
//       

    
    // virtual function
    QString getName() const;
    QIcon getIcon() const;
    bool doesCalibration() const;
    bool wiggleAllowed() const;
    Date::DataMethod getDataMethod() const;
    QList<Date::DataMethod> allowedDataMethods() const;
    QStringList csvColumns() const;
    int csvMinColumns() const;
    int csvOptionalColumns() const {return 2;} // Corresponding to  "ΔR" and "ΔR Error"
    QJsonObject fromCSV(const QStringList& list, const QLocale& csvLocale) ;
    QStringList toCSV(const QJsonObject& data, const QLocale& csvLocale) const;
    QString getDateDesc(const Date* date) const;
    QString getDateRefCurveName(const Date* date) ;

    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
    virtual void deleteGraphViewRef(GraphViewRefAbstract* graph);
    PluginSettingsViewAbstract* getSettingsView();
    QList<QHash<QString, QVariant>> getGroupedActions();

    QJsonObject checkValuesCompatibility(const QJsonObject& values);
    bool isDateValid(const QJsonObject& data, const ProjectSettings& settings);

    bool areDatesMergeable(const QJsonArray& dates);
    QJsonObject mergeDates(const QJsonArray& dates);

    // ---------------------

    QString getRefExt() const;
    QString getRefsPath() const;
    RefCurve loadRefFile(QFileInfo refFile);

    double getRefValueAt(const QJsonObject& data, const double& t);
    double getRefErrorAt(const QJsonObject& data, const double& t);

    QPair<double,double> getTminTmaxRefsCurve(const QJsonObject& data) const;
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


