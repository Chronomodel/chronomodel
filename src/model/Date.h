#ifndef DATE_H
#define DATE_H

#include "MHVariable.h"
#include "StateKeys.h"
#include "ProjectSettings.h"

#include <QMap>
#include <QJsonObject>
#include <QString>
#include <QPixmap>
#include <QObject>

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
        eDeltaNone = -1,
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
    QJsonObject toJson() const;
    
    static Date fromCSV(const QStringList &dataStr, const QLocale& csvLocale);
    QStringList toCSV(const QLocale& csvLocale) const;
    
    double getLikelihood(const double& t) const;
    QPair<long double, long double> getLikelihoodArg(const double& t) const;
    QString getDesc() const;
    
    void reset();
    void calibrate(const ProjectSettings& settings);
    double getLikelihoodFromCalib(const double t);

    const QMap<double, double> getFormatedCalibMap() const;
    const QMap<double, double> getRawCalibMap() const;

    QVector<double> getFormatedRepartition() const;

    QPixmap generateCalibThumb();
    QPixmap generateTypoThumb();
    
    void initDelta(Event* event);
    
    void updateTheta(Event* event);
    void autoSetTiSampler(const bool bSet);
    
    void updateDelta(Event* event);
    void updateSigma(Event* event);
    void updateWiggle();
    
    QColor getEventColor() const;

    double getTminRefCurve() const {return mTminRefCurve;}
    double getTmaxRefCurve() const {return mTmaxRefCurve;}
    void setTminRefCurve(const double tmin) { mTminRefCurve = tmin;}
    void setTmaxRefCurve(const double tmax) { mTmaxRefCurve = tmax;}

    double getTminCalib() const {return mTminCalib;}
    double getTmaxCalib() const {return mTmaxCalib;}
    void setTminCalib(const double tmin) { mTminCalib = tmin;}
    void setTmaxCalib(const double tmax) { mTmaxCalib = tmax;}

    double getFormatedTminRefCurve() const;
    double getFormatedTmaxRefCurve() const;

    double getFormatedTminCalib() const;
    double getFormatedTmaxCalib() const;

    void generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax);
    
public:
    MHVariable mTheta; // theta i de la date
    MHVariable mSigma; // sigma i de la date (par rapport au fait)
    MHVariable mWiggle;
    double mDelta;
    
    int mId;
    
    QString mName; // must be public, to be setting by dialogbox
    QColor mColor;

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
    
    //const QJsonObject * mJsonEvent;
    double mMixingLevel;
    
protected:
    samplingFunction updateti;
    
    double mTminRefCurve;
    double mTmaxRefCurve;

    double mTminCalib;
    double mTmaxCalib;
};

#endif
