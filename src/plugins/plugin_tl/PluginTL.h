#ifndef PLUGINTL_H
#define PLUGINTL_H

#if USE_PLUGIN_TL

#include "../PluginAbstract.h"

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
    
    float getLikelyhood(const float& t, const QJsonObject& data);
    
    QString getName() const;
    QIcon getIcon() const;
    bool doesCalibration() const;
    bool wiggleAllowed() const;
    Date::DataMethod getDataMethod() const;
    QList<Date::DataMethod> allowedDataMethods() const;
    QStringList csvColumns() const;
    QJsonObject dataFromList(const QStringList& list);
    QString getDateDesc(const Date* date) const;
    
    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
};

#endif
#endif
