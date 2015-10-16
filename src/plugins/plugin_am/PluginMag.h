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
    
    double getLikelyhood(const double& t, const QJsonObject& data);
    bool withLikelyhoodArg() {return true; };
    QPair<double, double > getLikelyhoodArg(const double& t, const QJsonObject& data);
    
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
    
    QString getRefsPath() const;
    void loadRefDatas();
    QStringList getRefsNames() const;
    const QMap<QString, QMap<double, double> >& getRefData(const QString& name);
    
    QMap< QString, QMap<QString, QMap<double, double> > > mRefDatas;
    // Used to store ref curves min and max values on a given study period.
    // This is only used in isDateValid() and prevents going through all ref curves points each time we check a date validity!!
    QMap<QString, QPair< QPair<double, double>, QPair<double, double> > > mLastRefsMinMax;
};

#endif
#endif
