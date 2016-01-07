#ifndef PLUGINABSTRACT_H
#define PLUGINABSTRACT_H

#include "Date.h"
#include "GraphView.h"
#include "ProjectSettings.h"
#include "RefCurve.h"

//#include <QtPlugin>
#include <QObject>
#include <QJsonObject>
#include <QDir>
#include <QString>
#include <QColor>
#include <QList>
#include <QTextStream>
#include <QPair>

#include <QLocale>

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
    PluginAbstract():mRefGraph(0){}
    virtual ~PluginAbstract(){}
    
    virtual long double getLikelihood(const double& t, const QJsonObject& data) = 0;
    virtual QPair<long double, long double > getLikelihoodArg(const double& t, const QJsonObject& data){return QPair<long double, long double>();}
    virtual bool withLikelihoodArg() {return false;}

    virtual QString getName() const = 0;
    virtual QIcon getIcon() const = 0;
    virtual bool doesCalibration() const = 0;
    virtual bool wiggleAllowed() const {return true;}
    virtual Date::DataMethod getDataMethod() const = 0;
    virtual QList<Date::DataMethod> allowedDataMethods() const = 0;
    virtual QString csvHelp() const{return QString();}
    virtual QStringList csvColumns() const{return QStringList();}
    virtual int csvMinColumns() const {return csvColumns().size();}
    virtual QJsonObject fromCSV(const QStringList& list) = 0;
    virtual QStringList toCSV(const QJsonObject& data, const QLocale& csvLocale) = 0;
    
    /**
     * @brief getDateDesc is the description of the Data showing in the properties of Event, in the list of data
     */
    virtual QString getDateDesc(const Date* date) const = 0;
    virtual bool areDatesMergeable(const QJsonArray& dates) {return false;}
    virtual QJsonObject mergeDates(const QJsonArray& dates) {QJsonObject ret; ret["error"] = tr("Cannot combine dates of type ") + getName(); return ret;}
    
    QColor getColor() const{
        return mColor;
    }
    QString getId() const{
        QString name = getName().simplified().toLower();
        name = name.replace(" ", "_");
        return name;
    }
    
    // Function to check if data values are ok : depending on the application version, plugin data values may change.
    // eg. : a new parameter may be added to 14C plugin, ...
    virtual QJsonObject checkValuesCompatibility(const QJsonObject& values){return values;}
    virtual bool isDateValid(const QJsonObject& data, const ProjectSettings& settings){return true;}
    
    virtual PluginFormAbstract* getForm() = 0;
    virtual GraphViewRefAbstract* getGraphViewRef() = 0;
    virtual PluginSettingsViewAbstract* getSettingsView() = 0;
    virtual QList<QHash<QString, QVariant>> getGroupedActions() {return QList<QHash<QString, QVariant>>();}
    
    // -------------------------------
    // The following is for plugins using ref curves :
    // -------------------------------
    virtual QPair<double,double> getTminTmaxRefsCurve(const QJsonObject& data) const = 0;
    
    virtual QString getRefExt() const {return "";}
    virtual QString getRefsPath() const {return "";}
    virtual RefCurve loadRefFile(QFileInfo refFile)
    {
        RefCurve curve;
        return curve;
    }

    void loadRefDatas()
    {
        QString path = QDir::currentPath();
        QString calibPath = getRefsPath();
        mRefCurves.clear();
        QDir calibDir(calibPath);
        
        QFileInfoList files = calibDir.entryInfoList(QStringList(), QDir::Files);
        for(int i=0; i<files.size(); ++i)
        {
            if(files[i].suffix().toLower() == getRefExt())
            {
                RefCurve curve = loadRefFile(files[i]);
                mRefCurves.insert(files[i].fileName().toLower(), curve);
            }
        }
    }
    
    QStringList getRefsNames() const
    {
        QStringList refNames;
        QHash<QString, RefCurve>::const_iterator it = mRefCurves.constBegin();
        while(it != mRefCurves.constEnd())
        {
           const RefCurve& curve = mRefCurves[it.key()];
           // return only valid curve
           if(!curve.mDataMean.isEmpty())
               refNames.push_back(it.key());
            ++it;
        }
        return refNames;
    }
    
    // curveName must be in lower Case
   double getRefCurveValueAt(const QString& curveName, const double& t)
    {
        long double value = 0;
        if(mRefCurves.constFind(curveName) != mRefCurves.constEnd())
        {
            const RefCurve& curve = mRefCurves[curveName];
            
            if(curve.mDataMean.constFind(t) != curve.mDataMean.constEnd())
            {
                value = curve.mDataMean[t];
            }
            else if(t < curve.mTmin || t > curve.mTmax){
                value = interpolate(t, curve.mTmin, curve.mTmax, curve.mDataMean[curve.mTmin], curve.mDataMean[curve.mTmax]);
            }
            else
            {
                // This actually return the iterator with the nearest greater key !!!
                QMap<double, double>::const_iterator iter = curve.mDataMean.lowerBound(t);
                if(iter != curve.mDataMean.end())
                {
                    double t_upper = iter.key();
                    --iter;
                    double t_under = iter.key();
                    
                    double v_under = curve.mDataMean[t_under];
                    double v_upper = curve.mDataMean[t_upper];
                    
                    value = interpolate(t, t_under, t_upper, v_under, v_upper);
                }
            }
        }
        return value;
    }
    
    double getRefCurveErrorAt(const QString& curveName, const double& t)
    {
        double error = 0;
        if(mRefCurves.constFind(curveName) != mRefCurves.constEnd())
        {
            const RefCurve& curve = mRefCurves[curveName];
            
            if(curve.mDataError.constFind(t) != curve.mDataError.constEnd())
            {
                error = curve.mDataError[t];
            }
            else if(t < curve.mTmin || t > curve.mTmax){
                error = 1.0e+6 * (curve.mDataSupMax - curve.mDataInfMin);
            }
            else
            {
                // This actually return the iterator with the nearest greater key !!!
                QMap<double, double>::const_iterator iter = curve.mDataMean.lowerBound(t);
                if(iter != curve.mDataMean.end())
                {
                    double t_upper = iter.key();
                    --iter;
                    double t_under = iter.key();
                    
                    double v_under = curve.mDataError[t_under];
                    double v_upper = curve.mDataError[t_upper];
                    
                    error = interpolate(t, t_under, t_upper, v_under, v_upper);
                }
            }
        }
        return error;
    }
    
    const RefCurve& getRefCurve(const QString& name)
    {
        return mRefCurves[name.toLower()];
    }
    
    QHash<QString, RefCurve> mRefCurves;
    GraphViewRefAbstract* mRefGraph;
    QColor mColor;
};

//----------------------------------------------------
//  Pour les plugins
//----------------------------------------------------
/*#define PluginAbstract_iid "chronomodel.PluginAbstract"
Q_DECLARE_INTERFACE(PluginAbstract, PluginAbstract_iid)

#if defined(DATATION_LIBRARY)
#  define DATATION_SHARED_EXPORT Q_DECL_EXPORT
#else
#  define DATATION_SHARED_EXPORT Q_DECL_IMPORT
#endif*/

#define DATATION_SHARED_EXPORT


#endif

