#ifndef PluginMag_H
#define PluginMag_H

#if USE_PLUGIN_AM

#include "../PluginAbstract.h"

class PluginMagRefView;

#define DATE_AM_IS_INC_STR "is_inc"
#define DATE_AM_IS_DEC_STR "is_dec"
#define DATE_AM_IS_INT_STR "is_int"
#define DATE_AM_ERROR_STR "error"
#define DATE_AM_INC_STR "inc"
#define DATE_AM_DEC_STR "dec"
#define DATE_AM_INTENSITY_STR "intensity"
#define DATE_AM_REF_CURVE_STR "ref_curve"


class DATATION_SHARED_EXPORT PluginMag : public PluginAbstract
{
    Q_OBJECT
    //Q_PLUGIN_METADATA(IID "chronomodel.PluginAbstract.PluginMag")
    //Q_INTERFACES(PluginAbstract)
public:
    PluginMag();
    
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
    
    // ---------------------
    
    QString getRefsPath() const;
    void loadRefDatas();
    QStringList getRefsNames() const;
    const QMap<QString, QMap<float, float>>& getRefData(const QString& name);
    
    QMap< QString, QMap<QString, QMap<float, float> > > mRefDatas;
};

#endif
#endif
