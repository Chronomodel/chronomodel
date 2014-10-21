#ifndef Plugin14C_H
#define Plugin14C_H

#if USE_PLUGIN_14C

#include "../PluginAbstract.h"

#define DATE_14C_AGE_STR "age"
#define DATE_14C_ERROR_STR "error"
#define DATE_14C_REF_CURVE_STR "ref_curve"


class DATATION_SHARED_EXPORT Plugin14C : public PluginAbstract
{
    Q_OBJECT
    //Q_PLUGIN_METADATA(IID "chronomodel.PluginAbstract.Plugin14C")
    //Q_INTERFACES(PluginAbstract)
public:
    Plugin14C();
    
    float getLikelyhood(const float& t, const QJsonObject& data);
    
    QString getName() const;
    QIcon getIcon() const;
    bool doesCalibration() const;
    Date::DataMethod getDataMethod() const;
    QStringList csvColumns() const;
    QJsonObject dataFromList(const QStringList& list);
    
    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
    
    // ---------------------
    
    QString getRefsPath() const;
    void loadRefDatas();
    QStringList getRefsNames() const;
    const QMap<QString, QMap<float, float>>& getRefData(const QString& name);
    
    QMap< QString, QMap<QString, QMap<float, float> > > mRefDatas;
};

#endif

#endif

