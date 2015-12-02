#ifndef PLUGINGAUSS_H
#define PLUGINGAUSS_H

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
    int csvMinColumns() const;
    QJsonObject fromCSV(const QStringList& list);
    QStringList toCSV(const QJsonObject& data, const QLocale &csvLocale);
    QString getDateDesc(const Date* date) const;
    
    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
    PluginSettingsViewAbstract* getSettingsView();
    
    QJsonObject checkValuesCompatibility(const QJsonObject& values);
    bool isDateValid(const QJsonObject& data, const ProjectSettings& settings);
    
    // ---------------------
    
    QString getRefExt() const;
    QString getRefsPath() const;
    QMap<QString, QMap<double, double> > loadRefFile(QFileInfo refFile);
    QPair<double,double> getTminTmaxRefsCurve(const QJsonObject& data) const;

    double getRefValueAt(const QJsonObject& data, const double& t);
    double getRefErrorAt(const QJsonObject& data, const double& t);
    
    // Used to store ref curves min and max values on a given study period.
    // This is only used in isDateValid() and prevents going through all ref curves points each time we check a date validity!!
    QMap<QString, QPair< QPair<double, double>, QPair<double, double> > > mLastRefsMinMax;
};

#endif
#endif
