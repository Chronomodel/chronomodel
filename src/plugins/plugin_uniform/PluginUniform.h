#ifndef PluginUniform_H
#define PluginUniform_H

#if USE_PLUGIN_UNIFORM

#include "../PluginAbstract.h"

#define DATE_UNIFORM_MIN_STR "min"
#define DATE_UNIFORM_MAX_STR "max"


class DATATION_SHARED_EXPORT PluginUniform : public PluginAbstract
{
    Q_OBJECT
    //Q_PLUGIN_METADATA(IID "chronomodel.PluginAbstract.PluginUniform")
    //Q_INTERFACES(PluginAbstract)
public:
    PluginUniform();
    
    double getLikelyhood(const double& t, const QJsonObject& data);
    
    QString getName() const;
    QIcon getIcon() const;
    QColor getColor() const;
    bool doesCalibration() const;
    bool wiggleAllowed() const;
    Date::DataMethod getDataMethod() const;
    QList<Date::DataMethod> allowedDataMethods() const;
    QStringList csvColumns() const;
    QJsonObject fromCSV(const QStringList& list);
    QStringList toCSV(const QJsonObject& data);
    QString getDateDesc(const Date* date) const;
    
    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
};

#endif
#endif
