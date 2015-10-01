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
#include <QColor>
#include <QList>
#include <QTextStream>

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
    
    virtual double getLikelyhood(const double& t, const QJsonObject& data) = 0;

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
    virtual QStringList toCSV(const QJsonObject& data) = 0;
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

