#ifndef PluginAM_H
#define PluginAM_H

#if USE_PLUGIN_AM

#include "../PluginAbstract.h"

class PluginAMRefView;

#define DATE_AM_MODE "mode"
#define DATE_AM_I "inclination"
#define DATE_AM_D "declination"
#define DATE_AM_F "intensity"
#define DATE_AM_ALPHA_95 "alpha95"
#define DATE_AM_SIGMA_F "sigmaF"
#define DATE_AM_CURVE_I "curveI"
#define DATE_AM_CURVE_D "curveD"
#define DATE_AM_CURVE_F "curveF"

#define DATE_AM_MODE_ID "ID"
#define DATE_AM_MODE_IF "IF"
#define DATE_AM_MODE_IDF "IDF"

class DATATION_SHARED_EXPORT PluginAM : public PluginAbstract
{
    Q_OBJECT
    //Q_PLUGIN_METADATA(IID "chronomodel.PluginAbstract.PluginAM")
    //Q_INTERFACES(PluginAbstract)
public:
    PluginAM();
    virtual ~PluginAM();

     // virtual function
    long double getLikelihood(const double& t, const QJsonObject& data);
    bool withLikelihoodArg() {return true; }
    QPair<long double, long double > getLikelihoodArg(const double& t, const QJsonObject& data);
    
    QString getName() const;
    QIcon getIcon() const;
    bool doesCalibration() const;
    bool wiggleAllowed() const;
    Date::DataMethod getDataMethod() const;
    QList<Date::DataMethod> allowedDataMethods() const;
    QStringList csvColumns() const;
    QJsonObject fromCSV(const QStringList& list, const QLocale &csvLocale) ;
    QStringList toCSV(const QJsonObject& data, const QLocale &csvLocale) const;
    QJsonObject checkValuesCompatibility(const QJsonObject& values);
    
    Date::CalibrationType getDateCalibrationType(const QJsonObject& data);
    QString getDateDesc(const Date* date) const;
    
    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
    PluginSettingsViewAbstract* getSettingsView();
    
    bool isDateValid(const QJsonObject& data, const ProjectSettings& settings);
    bool isCurveValid(const QJsonObject& data, const QString& curveName, const double& mesure, const double& step);
    
    // ---------------------
    QString getRefExt() const;
    QString getRefsPath() const;
    RefCurve loadRefFile(QFileInfo refFile);
    
    QPair<double,double> getTminTmaxRefsCurve(const QJsonObject& data) const;
    QPair<double,double> getTminTmaxRefCurve(const QString& curveName) const;
};

#endif
#endif
