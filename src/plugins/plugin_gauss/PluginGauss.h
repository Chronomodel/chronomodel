#ifndef PluginGauss_H
#define PluginGauss_H

#if USE_PLUGIN_GAUSS

#include "../PluginAbstract.h"

#define DATE_GAUSS_AGE_STR "age"
#define DATE_GAUSS_ERROR_STR "error"
#define DATE_GAUSS_A_STR "a"
#define DATE_GAUSS_B_STR "b"
#define DATE_GAUSS_C_STR "c"
#define DATE_GAUSS_MODE_STR "mode"
#define DATE_GAUSS_CURVE_STR "curve"

#define DATE_GAUSS_MODE_CURVE "curve"
#define DATE_GAUSS_MODE_EQ "eq"
#define DATE_GAUSS_MODE_NONE "none"


class DATATION_SHARED_EXPORT PluginGauss : public PluginAbstract
{
    Q_OBJECT
    //Q_PLUGIN_METADATA(IID "chronomodel.PluginAbstract.PluginGauss")
    //Q_INTERFACES(PluginAbstract)
public:
    PluginGauss();
    
    double getLikelyhood(const double& t, const QJsonObject& data);
    bool withLikelyhoodArg() {return true; };
    QPair<double, double > getLikelyhoodArg(const double& t, const QJsonObject& data);
    
    QString getName() const;
    QIcon getIcon() const;
    bool doesCalibration() const;
    bool wiggleAllowed() const;
    Date::DataMethod getDataMethod() const;
    QList<Date::DataMethod> allowedDataMethods() const;
    QString csvHelp() const;
    QStringList csvColumns() const;
    QJsonObject fromCSV(const QStringList& list);
    QStringList toCSV(const QJsonObject& data);
    QString getDateDesc(const Date* date) const;
    
    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
    PluginSettingsViewAbstract* getSettingsView();
    
    QJsonObject checkValuesCompatibility(const QJsonObject& values);
    bool isDateValid(const QJsonObject& data, const ProjectSettings& settings);
    
    // ---------------------
    
    QString getRefsPath() const;
    void loadRefDatas();//const ProjectSettings& settings);
    QStringList getRefsNames() const;
    const QMap<QString, QMap<double, double> >& getRefData(const QString& name);
    
    QMap< QString, QMap<QString, QMap<double, double> > > mRefDatas;
};

#endif
#endif
