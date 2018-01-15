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
    bool withLikelihoodArg() {return true; }
    QPair<long double, long double > getLikelihoodArg(const double& t, const QJsonObject& data);
    
    // virtual function
    QString getName() const;
    QIcon getIcon() const;
    bool doesCalibration() const;
    bool wiggleAllowed() const;
    Date::DataMethod getDataMethod() const;
    QList<Date::DataMethod> allowedDataMethods() const;
    QStringList csvColumns() const;
    int csvMinColumns() const;
    int csvOptionalColumns() const {return 2;} // Corresponding to  "ΔR" and "ΔR Error"
    QJsonObject fromCSV(const QStringList& list, const QLocale& csvLocale) ;
    QStringList toCSV(const QJsonObject& data, const QLocale& csvLocale) const;
    QString getDateDesc(const Date* date) const;
    QString getDateRefCurveName(const Date* date) ;
    
    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
    virtual void deleteGraphViewRef(GraphViewRefAbstract* graph);
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

