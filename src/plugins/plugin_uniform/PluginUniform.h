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
    bool withLikelyhoodArg() {return false; };
    
    QString getName() const;
    QIcon getIcon() const;
    bool doesCalibration() const;
    bool wiggleAllowed() const;
    Date::DataMethod getDataMethod() const;
    QList<Date::DataMethod> allowedDataMethods() const;
    QStringList csvColumns() const;
    QJsonObject fromCSV(const QStringList& list);
    QStringList toCSV(const QJsonObject& data, const QLocale &csvLocale);
    QString getDateDesc(const Date* date) const;
    
    bool isDateValid(const QJsonObject& data, const ProjectSettings& settings);

    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
    PluginSettingsViewAbstract* getSettingsView();
    
    QPair<double,double> getTminTmaxRefsCurve(const QJsonObject& data) const;
};

#endif
#endif
