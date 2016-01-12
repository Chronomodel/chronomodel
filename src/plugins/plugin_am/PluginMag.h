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
    virtual ~PluginMag();
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
    QJsonObject fromCSV(const QStringList& list);
    QStringList toCSV(const QJsonObject& data, const QLocale &csvLocale);
    QString getDateDesc(const Date* date) const;
    
    PluginFormAbstract* getForm();
    GraphViewRefAbstract* getGraphViewRef();
    PluginSettingsViewAbstract* getSettingsView();
    
    bool isDateValid(const QJsonObject& data, const ProjectSettings& settings);
    
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
