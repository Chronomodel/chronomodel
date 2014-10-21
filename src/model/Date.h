#ifndef DATE_H
#define DATE_H

#include "MHVariable.h"
#include <QMap>
#include <QJsonObject>
#include <QString>
#include <QPixmap>

#define STATE_DATE_ID "id"
#define STATE_DATE_NAME "name"
#define STATE_DATE_DATA "data"
#define STATE_DATE_PLUGIN_ID "plugin_id"
#define STATE_DATE_METHOD "method"
#define STATE_DATE_DELTA_TYPE "delta_type"
#define STATE_DATE_DELTA_FIXED "delta_fixed"
#define STATE_DATE_DELTA_MIN "delta_min"
#define STATE_DATE_DELTA_MAX "delta_max"
#define STATE_DATE_DELTA_AVERAGE "delta_average"
#define STATE_DATE_DELTA_ERROR "delta_error"

class Event;
class PluginAbstract;


class Date
{
public:
    enum DataMethod{
        eMHIndependant = 0,
        eInversion = 1,
        eMHSymGaussAdapt = 2
    };
    
    enum DeltaType{
        eDeltaFixed = 0,
        eDeltaGaussian = 1,
        eDeltaRange = 2
    };
    
    Date();
    Date(PluginAbstract* plugin);
    Date(const Date& date);
    Date& operator=(const Date& date);
    void copyFrom(const Date& date);
    virtual ~Date();
    
    void init();
    bool isNull();
    
    static Date fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    float getLikelyhood(const float& t);
    QString getDesc() const;
    
    void reset();
    void calibrate(const float& tmin, const float& tmax, const float& step);
    float getLikelyhoodFromCalib(const float t);
    
    void updateTheta(const float& tmin, const float& tmax);
    void updateDelta();
    void updateSigma();
    
public:
    MHVariable mTheta; // theta i de la date
    MHVariable mSigma; // sigma i de la date (par rapport au fait)
    MHVariable mDelta;
    
    int mId;
    QString mName;
    QJsonObject mData;
    PluginAbstract* mPlugin;
    DataMethod mMethod;

    DeltaType mDeltaType;
    float mDeltaFixed;
    float mDeltaMin;
    float mDeltaMax;
    float mDeltaAverage;
    float mDeltaError;
    
    Event* mEvent;
    
    bool mIsCurrent;
    bool mIsSelected;
    
    QMap<float, float> mCalibration;
    QMap<float, float> mRepartition;
    QMap<float, float> mCalibHPD;
    
    QPixmap mCalibThumb;
    QList<Date*> mSubDates;
};

#endif
