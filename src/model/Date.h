#ifndef DATE_H
#define DATE_H

#include "MHVariable.h"
#include "StateKeys.h"
#include "ProjectSettings.h"

#include <QMap>
#include <QJsonObject>
#include <QString>
#include <QPixmap>

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
    //void setJson(const QJsonObject& jsonDate);
    void setJson(const QJsonObject & json, const int eventIdx, const int dateIdx);
    void setEventJson(const QJsonObject& jsonEvent);
    void setIdxInEventArray(int j);
    QJsonObject toJson() const;
    
    static Date fromCSV(QStringList dataStr);
    QStringList toCSV() const;
    
    double getLikelyhood(const double& t);
    QString getDesc() const;
    
    void reset();
    void calibrate(const ProjectSettings& settings);
    double getLikelyhoodFromCalib(const double t);
    QMap<double, double> getCalibMap() const;
    QPixmap generateCalibThumb();
    
    void initDelta(Event* event);
    
    void updateTheta(Event* event);
    void updateDelta(Event* event);
    void updateSigma(Event* event);
    void updateWiggle();
    
    QColor getColor() const;
    QString getName() const;
    
public:
    MHVariable mTheta; // theta i de la date
    MHVariable mSigma; // sigma i de la date (par rapport au fait)
    MHVariable mWiggle;
    double mDelta;
    
    int mId;
    QString mInitName;
    QJsonObject mData;
    PluginAbstract* mPlugin;
    DataMethod mMethod;
    bool mIsValid;

    DeltaType mDeltaType;
    double mDeltaFixed;
    double mDeltaMin;
    double mDeltaMax;
    double mDeltaAverage;
    double mDeltaError;
    
    bool mIsCurrent;
    bool mIsSelected;
    
    QVector<double> mCalibration;
    double mCalibSum;
    QVector<double> mRepartition;
    QMap<double, double> mCalibHPD;
    ProjectSettings mSettings;
    
    QList<Date> mSubDates;
    
    const QJsonObject * mJsonEvent;

private:
    const QJsonObject * mJsonDate;
    int mEventIdx;
    int mIdxInEventArray;
    QColor mColor;
};

#endif
