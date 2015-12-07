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
class Date;

typedef void (*samplingFunction)(Date* date, Event* event);

void fMHSymetric(Date* date, Event* event);
void fInversion(Date* date, Event* event);
void fMHSymGaussAdapt(Date* date,Event* event);

void fMHSymetricWithArg(Date* date, Event* event);
void fMHSymGaussAdaptWithArg(Date* date, Event* event);
void fInversionWithArg(Date* date, Event* event);

double fProposalDensity(const double t, const double t0, Date* date);



class Date
{
public:
    enum DataMethod{
        eMHSymetric = 0,
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
    bool isNull() const;
    
    static Date fromJson(const QJsonObject& json);

    void setModelJson(const QJsonObject & json, const int eventIdx, const int dateIdx);
    void setIdxInEventArray(int j);
    QJsonObject toJson() const;
    
    static Date fromCSV(QStringList dataStr);
    QStringList toCSV(QLocale csvLocale) const;
    
    double getLikelyhood(const double& t);
    QPair<double, double > getLikelyhoodArg(const double& t);
    QString getDesc() const;
    
    void reset();
    void calibrate(const ProjectSettings& settings);
    double getLikelyhoodFromCalib(const double t);
    QMap<double, double> getCalibMap() const;
    QPixmap generateCalibThumb();
    QPixmap generateTypoThumb();
    
    void initDelta(Event* event);
    
    void updateTheta(Event* event);
    void autoSetTiSampler(const bool bSet);
    
    void updateDelta(Event* event);
    void updateSigma(Event* event);
    void updateWiggle();
    
    QColor getColor() const;
    QColor getEventColor() const;
    QString getName() const;

    double getTminRefCurve() const {return mTminRefCurve;}
    double getTmaxRefCurve() const {return mTmaxRefCurve;}
    
    double getTminCalib() const {return mTminCalib;}
    double getTmaxCalib() const {return mTmaxCalib;}
    
public:
    MHVariable mTheta; // theta i de la date
    MHVariable mSigma; // sigma i de la date (par rapport au fait)
    MHVariable mWiggle;
    double mDelta;
    
    int mId;
    
    QString mInitName; //must be public, to be setting by dialogbox
    QColor mInitColor;

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
    QVector<double> mRepartition;
    QMap<double, double> mCalibHPD;
    ProjectSettings mSettings;
    
    QList<Date> mSubDates;
    
    const QJsonObject * mJsonEvent;
    double mMixingLevel;
    
protected:
    samplingFunction updateti;
    
    const QJsonObject * mModelJsonDate;
    int mJsonEventIdx;
    int mIdxInEventArray;
    
    double mTminRefCurve;
    double mTmaxRefCurve;

    double mTminCalib;
    double mTmaxCalib;
};

#endif
