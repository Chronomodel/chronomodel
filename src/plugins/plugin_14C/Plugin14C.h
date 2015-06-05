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
    
    // ---------------------
    
    QString getRefsPath() const;
    void loadRefDatas();//const ProjectSettings& settings);
    QStringList getRefsNames() const;
    const QMap<QString, QMap<double, double> >& getRefData(const QString& name);
    
    QMap< QString, QMap<QString, QMap<double, double> > > mRefDatas;
};

#endif

#endif

