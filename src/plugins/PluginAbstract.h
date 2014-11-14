#ifndef PLUGINABSTRACT_H
#define PLUGINABSTRACT_H

#include "Date.h"
#include "GraphView.h"
#include "ProjectSettings.h"

//#include <QtPlugin>
#include <QObject>
#include <QJsonObject>
#include <QDir>
#include <QString>
#include <QTextStream>

class ParamMCMC;
class GraphView;
class PluginFormAbstract;
class GraphViewRefAbstract;


class PluginAbstract: public QObject
{
    Q_OBJECT
public:
    PluginAbstract():mRefGraph(0){}
    virtual ~PluginAbstract(){}
    
    virtual float getLikelyhood(const float& t, const QJsonObject& data) = 0;

    virtual QString getName() const = 0;
    virtual QIcon getIcon() const = 0;
    virtual bool doesCalibration() const = 0;
    virtual bool wiggleAllowed() const {return true;}
    virtual Date::DataMethod getDataMethod() const = 0;
    virtual QList<Date::DataMethod> allowedDataMethods() const = 0;
    virtual QString csvHelp() const{return QString();}
    virtual QStringList csvColumns() const{return QStringList();}
    int csvMinColumns() const {return csvColumns().size();}
    virtual QJsonObject dataFromList(const QStringList& list) = 0;
    
    QString getId() const{
        QString name = getName().simplified().toLower();
        name = name.replace(" ", "_");
        return name;
    }
    
    virtual PluginFormAbstract* getForm() = 0;
    virtual GraphViewRefAbstract* getGraphViewRef() = 0;
    
    GraphViewRefAbstract* mRefGraph;
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

