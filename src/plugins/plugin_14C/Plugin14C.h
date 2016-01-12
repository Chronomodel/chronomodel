#ifndef Plugin14C_H
#define Plugin14C_H

#if USE_PLUGIN_14C

#include "../PluginAbstract.h"

#define DATE_14C_AGE_STR "age"
#define DATE_14C_ERROR_STR "error"
#define DATE_14C_DELTA_R_STR "delta_r"
#define DATE_14C_DELTA_R_ERROR_STR "delta_r_error"
#define DATE_14C_REF_CURVE_STR "ref_curve"


class DATATION_SHARED_EXPORT Plugin14C : public PluginAbstract
{
    Q_OBJECT
    //Q_PLUGIN_METADATA(IID "chronomodel.PluginAbstract.Plugin14C")
    //Q_INTERFACES(PluginAbstract)
public:
    Plugin14C();
    virtual ~Plugin14C();

    long double getLikelihood(const double& t, const QJsonObject& data);
    bool withLikelihoodArg() {return true; };
    QPair<long double, long double > getLikelihoodArg(const double& t, const QJsonObject& data);
    
    QString getName() const;
    QIcon getIcon() const;
    bool doesCalibration() const;
    bool wiggleAllowed() const;
    Date::DataMethod getDataMethod() const;
    QList<Date::DataMethod> allowedDataMethods() const;
    QStringList csvColumns() const;
    int csvMinColumns() const;
    QJsonObject fromCSV(const QStringList& list);
    QStringList toCSV(const QJsonObject& data, const QLocale& csvLocale);
    QString getDateDesc(const Date* date) const;
    
    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
    PluginSettingsViewAbstract* getSettingsView();
    QList<QHash<QString, QVariant>> getGroupedActions();
    
    QJsonObject checkValuesCompatibility(const QJsonObject& values);
    bool isDateValid(const QJsonObject& data, const ProjectSettings& settings);
    
    bool areDatesMergeable(const QJsonArray& dates);
    QJsonObject mergeDates(const QJsonArray& dates);
    
    // ---------------------
    
    QString getRefExt() const;
    QString getRefsPath() const;
    RefCurve loadRefFile(QFileInfo refFile);
    
    double getRefValueAt(const QJsonObject& data, const double& t);
    double getRefErrorAt(const QJsonObject& data, const double& t);
    
    QPair<double,double> getTminTmaxRefsCurve(const QJsonObject& data) const;
};

#endif

#endif

