#ifndef PluginGauss_H
#define PluginGauss_H

#if USE_PLUGIN_GAUSS

#include "../PluginAbstract.h"

#define DATE_GAUSS_AGE_STR "age"
#define DATE_GAUSS_ERROR_STR "error"
#define DATE_GAUSS_A_STR "a"
#define DATE_GAUSS_B_STR "b"
#define DATE_GAUSS_C_STR "c"


class DATATION_SHARED_EXPORT PluginGauss : public PluginAbstract
{
    Q_OBJECT
    //Q_PLUGIN_METADATA(IID "chronomodel.PluginAbstract.PluginGauss")
    //Q_INTERFACES(PluginAbstract)
public:
    PluginGauss();
    
    float getLikelyhood(const float& t, const QJsonObject& data);
    
    QString getName() const;
    QIcon getIcon() const;
    bool doesCalibration() const;
    bool wiggleAllowed() const;
    Date::DataMethod getDataMethod() const;
    QList<Date::DataMethod> allowedDataMethods() const;
    QString csvHelp() const;
    QStringList csvColumns() const;
    QJsonObject dataFromList(const QStringList& list);
    
    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
};

#endif
#endif
