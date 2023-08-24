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

#ifndef PLUGINABSTRACT_H
#define PLUGINABSTRACT_H

#include "Date.h"
#include "StudyPeriodSettings.h"
#include "RefCurve.h"
#include "StateKeys.h"
#include "StdUtilities.h"

#include <QObject>
#include <QJsonObject>
#include <QDir>
#include <QString>
#include <QColor>
#include <QList>
#include <QTextStream>
#include <QPair>
#include <QLocale>
#include <QStandardPaths>

class ParamMCMC;
class GraphView;
class PluginFormAbstract;
class GraphViewRefAbstract;
class PluginSettingsViewAbstract;


struct GroupedAction{
    QString pluginId;
    QString title;
    QString label;

    QString inputType;

    // For combobox called with QInputDialog::getItem
    QStringList items;
    int current;
};

class PluginAbstract: public QObject
{
    Q_OBJECT
public:
    PluginAbstract():mRefGraph(nullptr){}
    virtual ~PluginAbstract(){}

    virtual bool withLikelihoodArg() {return false;}
    virtual long double getLikelihood(const double t, const QJsonObject &data) = 0;
    virtual QPair<long double, long double > getLikelihoodArg(const double t, const QJsonObject &data)
    {
        (void) t;
        (void) data;
        return QPair<long double, long double>();
    }
    
    long double getLikelihoodCombine  (const double t, const QJsonArray &subDateArray, const double step)
    {
        long double produit = 1.l;
        Date date;
        QJsonObject subDateJSon;
        QJsonObject data;
        for (auto&& sDA : subDateArray ) {
            subDateJSon = sDA.toObject();
            data = subDateJSon.value(STATE_DATE_DATA).toObject();

            int deltaType = subDateJSon.value(STATE_DATE_DELTA_TYPE).toInt();

            if ( wiggleAllowed() == true ) {
                 switch (deltaType) {
                    case Date::eDeltaNone:
                        produit *= getLikelihood(t, data );
                        break;
                    case Date::eDeltaFixed: {
                        double deltaFixed = subDateJSon.value(STATE_DATE_DELTA_FIXED).toDouble();
                        produit *= getLikelihood( t - deltaFixed, data );
                    }
                        break;
                    case Date::eDeltaGaussian: {
                         long double d = getLikelihood(t, data);

                         double deltaAverage = subDateJSon.value(STATE_DATE_DELTA_AVERAGE).toDouble();
                         double deltaError = subDateJSon.value(STATE_DATE_DELTA_ERROR).toDouble();
                         long double r (-5* deltaError);
                         while (r < (5* deltaError)) {
                              d += getLikelihood(t - deltaAverage + r, data) * expl((-0.5l) * powl(r, 2.l) / powl(deltaError, 2.l)) /sqrt(deltaError);
                              r += step;
                         }
                         produit *= d;
                    }
                     break;

                    case Date::eDeltaRange: {
                        //produit *= date.getLikelihoodFromWiggleCalib(t);

                        double deltaMin = subDateJSon.value(STATE_DATE_DELTA_MIN).toDouble();
                        double deltaMax = subDateJSon.value(STATE_DATE_DELTA_MAX).toDouble();
                        long double d = getLikelihood(t, data);
                        long double r (deltaMin);
                        while (r <= deltaMax) {
                            d += getLikelihood(t - r, data);
                            r += step;
                        }
                        produit *= d;
                    }
                        break;
                }
            } else {
                produit *= getLikelihood(t, data );
            }
        }
      
        return produit;
    }

    virtual QString getName() const = 0;
    virtual QIcon getIcon() const = 0;
    virtual bool doesCalibration() const = 0;
    virtual bool wiggleAllowed() const = 0;
    virtual MHVariable::SamplerProposal getDataMethod() const = 0;
    virtual QList<MHVariable::SamplerProposal> allowedDataMethods() const = 0;
    virtual QString csvHelp() const{return QString();}
    virtual QStringList csvColumns() const{return QStringList();}
    virtual qsizetype csvMinColumns() const {return csvColumns().size();}
    virtual qsizetype csvOptionalColumns() const {return 0;}
    virtual QJsonObject fromCSV(const QStringList &list,const QLocale &csvLocale) = 0;
    virtual QStringList toCSV(const QJsonObject &data,const QLocale &csvLocale) const = 0;

    /**
     * @brief getDateDesc is the description of the Data showing in the properties of Event, in the list of data
     */
    virtual QString getDateDesc(const Date* date) const = 0;
    virtual QString getDateRefCurveName(const Date* ) {return QString();}
    virtual bool areDatesMergeable(const QJsonArray &dates) { (void) dates; return false;}
    virtual QJsonObject mergeDates(const QJsonArray &dates)
    {
        (void) dates;
        QJsonObject ret;
        ret["error"] = tr("Cannot combine dates of type %1").arg(getName());
        return ret;
    }

    QColor getColor() const {return mColor;}

    QString getId() const{
        QString name = getName().simplified().toLower();
        name = name.replace(" ", "_");
        return name;
    }

    // Function to check if data values are ok : depending on the application version, plugin data values may change.
    // eg. : a new parameter may be added to 14C plugin, ...
    virtual QJsonObject checkValuesCompatibility(const QJsonObject& values){return values;}
    virtual bool isDateValid(const QJsonObject& data, const StudyPeriodSettings& settings)
    {
        (void)data;
        (void) settings;
        return true;
    }
    virtual bool isCombineValid(const QJsonObject& data, const StudyPeriodSettings& settings)
   {
       //Q_ASSERT(&data);
       //Q_ASSERT(&settings);
       
       QJsonArray subData = data.value(STATE_DATE_SUB_DATES).toArray();
      
       bool valid (true);
       
       for (int i (0); i<subData.size(); ++i) {
           const bool isVal = isDateValid(subData.at(i).toObject().value(STATE_DATE_DATA).toObject(), settings);
           valid = valid & isVal;
       }
       return valid;
   }
    
    virtual PluginFormAbstract* getForm() = 0;
    virtual GraphViewRefAbstract* getGraphViewRef() = 0;
    virtual void deleteGraphViewRef(GraphViewRefAbstract* graph ) {(void) graph ;}
   
      
    virtual PluginSettingsViewAbstract* getSettingsView() = 0;
    virtual QList<QHash<QString, QVariant>> getGroupedActions() {return QList<QHash<QString, QVariant>>();}

    /* -------------------------------
     * The following is for plugins using ref curves :
     * ------------------------------- */
    virtual QPair<double, double> getTminTmaxRefsCurve(const QJsonObject &data) const = 0;

    virtual double getMinStepRefsCurve(const QJsonObject &data) const {(void) data; return INFINITY;};
    /*
     * For the majority of the plugins, i.e. having a calibration curve,
     * the wiggles densities are on the same support as the calibrated densities,
     * which is the range of the calibration curve.
     * Except for the Uniform plugin
     */
 /*   virtual QPair<double,double> getTminTmaxRefsCurveWiggle(const QJsonObject& data)
    {
        return getTminTmaxRefsCurve(data);
    }
*/
    /* Obsolete */
    virtual QPair<double,double> getTminTmaxRefsCurveCombine(const QJsonArray& subDateArray)
    {
        double tmin (INFINITY);
        double tmax (-INFINITY);
        for ( auto&& sD : subDateArray ) {
                QPair<double, double> tminTmax = getTminTmaxRefsCurve( sD.toObject());

                tmin = std::min(tmin, tminTmax.first);
                tmax = std::max(tmax, tminTmax.second);
        }
        return qMakePair(tmin, tmax);
    }

    virtual QString getRefExt() const {return "";}
    virtual QString getRefsPath() const {return "";}
    virtual RefCurve loadRefFile(QFileInfo refFile)
    {
        (void) refFile;
        RefCurve curve;
        return curve;
    }

    void loadRefDatas()
    {
        const QString calibPath = getRefsPath();
        mRefCurves.clear();
        QDir calibDir(calibPath);

        const QFileInfoList files = calibDir.entryInfoList(QStringList(), QDir::Files);
        for (int i=0; i<files.size(); ++i) {
            if (files.at(i).suffix().toLower() == getRefExt()) {
                RefCurve curve = loadRefFile(files.at(i));
                const QString fileInLower = files.at(i).fileName().toLower();
                mRefCurves.insert(fileInLower, curve);
            }
        }
    }

    QStringList getRefsNames() const
    {
        QStringList refNames;
        QHash<QString, RefCurve>::const_iterator it = mRefCurves.constBegin();
        while (it != mRefCurves.constEnd()) {
           const RefCurve& curve = mRefCurves.value(it.key());
           // return only valid curve
           if (!curve.mDataMean.isEmpty())
               refNames.push_back(it.key());
            ++it;
        }
        return refNames;
    }

    // curveName must be in lower Case
    double getRefCurveValueAt(const QString &curveName, const double t)
    {
        long double value = 0.;
        if (mRefCurves.constFind(curveName) != mRefCurves.constEnd()) {
            const RefCurve& curve = mRefCurves.value(curveName);

            if (t >= curve.mTmin && t <= curve.mTmax) {
                // This actually return the iterator with the nearest greater key !!!
                QMap<double, double>::const_iterator iter = curve.mDataMean.lowerBound(t);
                if (iter != curve.mDataMean.constBegin())  {
                        const double t_upper = iter.key();
                        const double v_upper = iter.value();

                        --iter;
                        const double t_under = iter.key();
                        const double v_under = iter.value();

                        value = interpolate(t, t_under, t_upper, v_under, v_upper);
                } else {
                        value = iter.value();
                }
            }
            else { // onExtension depreciated
                value = (curve.mDataSupMax + curve.mDataInfMin )/2.;
            }

        }
        return value;
    }

    double getRefCurveErrorAt(const QString &curveName, const double t)
    {
        double error = 0.;
        if (mRefCurves.constFind(curveName) != mRefCurves.constEnd()) {
            const RefCurve& curve = mRefCurves.value(curveName);

            if (t >= curve.mTmin && t <= curve.mTmax) {
               // This actually return the iterator with the nearest greater key !!!
                QMap<double, double>::const_iterator iter = curve.mDataError.lowerBound(t);
                // the higher value must be mTmax.
                if (iter != curve.mDataError.constBegin()) {

                    const double t_upper = iter.key();
                    const double v_upper = iter.value();//memo curve.mDataError[t_upper];
                    --iter;
                    const double t_under = iter.key();
                    const double v_under = iter.value();//memo curve.mDataError[t_under];


                    error = interpolate(t, t_under, t_upper, v_under, v_upper);
                } else {
                    error = iter.value();
                }
            }
            else { // onExtension
                    error = 1.0e+6 * (curve.mDataSupMax - curve.mDataInfMin);
            }
        }
        return error;
    }

    const RefCurve& getRefCurve(const QString& name)
    {
        return mRefCurves[name.toLower()];
    }

    PluginAbstract &operator=(const PluginAbstract& other)
    {
        mRefCurves = other.mRefCurves;
        mColor = other.mColor;
        mRefGraph = other.mRefGraph;

        return *this;
    }

    QHash<QString, RefCurve> mRefCurves;
    GraphViewRefAbstract* mRefGraph;
    QColor mColor;

    static QString AppPluginLibrary()
    {
#ifdef Q_OS_MAC
        return QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).at(0) + QString("/CNRS/ChronoModel");
#else
        return QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).at(0) + QString("/CNRS/ChronoModel");
#endif

    }


};


#define DATATION_SHARED_EXPORT


#endif
