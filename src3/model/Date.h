/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#ifndef DATE_H
#define DATE_H

#include "MHVariable.h"
#include "StateKeys.h"
#include "../project/ProjectSettings.h"
//#include "PluginAbstract.h"
//#include "CalibrationCurve.h"

#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QPixmap>
#include <QObject>

class Event;
class PluginAbstract;

class CalibrationCurve;

class Project;

class Dato
{
public:
    Dato();

    void fromJson(const QJsonObject& json);
    virtual ~Dato();
    QString mName;
    MHVariable mTi; //mTheta; // theta i de la date
    MHVariable mSigma; // sigma i de la date (par rapport au fait)
    MHVariable mWiggle;
    double mDelta;

   // samplingFunctiono updateti;

    double mTminRefCurve;
    double mTmaxRefCurve;
};



class Date
{

public:
    enum OriginType{
        eSingleDate = 0,
        eCombination = 1
    };
/*    enum DataMethod{
        eMHSymetric = 0,
        eInversion = 1,
        eMHSymGaussAdapt = 2
    };
*/
    enum DeltaType{
        eDeltaNone = -1,
        eDeltaFixed = 0,
        eDeltaGaussian = 1,
        eDeltaRange = 2
    };

    MHVariable mTi;//mTheta; // theta i de la date
    MHVariable mSigmaTi; // sigma i de la date (par rapport au fait)
    MHVariable mWiggle;
    double mDelta;

    int mId;
    QString mUUID;

    QString mName; // must be public, to be setting by dialogbox
    QColor mColor;

    QJsonObject mData;
    OriginType mOrigin;
    PluginAbstract* mPlugin;
    //DataMethod mMethod;
    bool mIsValid;

    DeltaType mDeltaType;
    double mDeltaFixed;
    double mDeltaMin;
    double mDeltaMax;
    double mDeltaAverage;
    double mDeltaError;

    bool mIsCurrent;
    bool mIsSelected;

    CalibrationCurve* mCalibration;
    CalibrationCurve* mWiggleCalibration;

    QMap<double, double> mCalibHPD;
    ProjectSettings mSettings;

    //QList<TDate> mSubDates;
    QJsonArray mSubDates;
    double mMixingLevel;


public:

    Date();
    virtual ~Date();
    Date(const QJsonObject &json);
    Date(PluginAbstract* plugin);
    Date(const Date& date);
    Date& operator=(const Date& date);
    void copyFrom(const Date& date);

    void init();
    bool isNull() const;

    void fromJson(const QJsonObject& json);
    QJsonObject toJson() const;

    static Date fromCSV(const QStringList &dataStr, const QLocale& csvLocale);
    QStringList toCSV(const QLocale& csvLocale) const;

    long double getLikelihood(const double& t) const;
    QPair<long double, long double> getLikelihoodArg(const double& t) const;
    QString getDesc() const;
    QString getWiggleDesc() const;
    static QString getWiggleDesc(const QJsonObject& json); // used in CalibrationView
    PluginAbstract* getPlugin() const {return mPlugin;}

    void reset();
    void calibrate(const ProjectSettings & settings, Project *project, bool truncate=true);
    void calibrateWiggle(const ProjectSettings & settings, Project *project);

    double getLikelihoodFromCalib(const double &t) const;
    double getLikelihoodFromWiggleCalib(const double &t) const;

    const QMap<double, double> getFormatedCalibMap() const;
    const QMap<double, double> getFormatedWiggleCalibMap() const;
    
    const QMap<double, double> getFormatedCalibToShow() const;
    const QMap<double, double> getFormatedWiggleCalibToShow() const;

    const QMap<double, double> getRawCalibMap() const;
    const QMap<double, double> getRawWiggleCalibMap() const;

    QVector<double> getFormatedRepartition() const;

    QPixmap generateCalibThumb();
    QPixmap generateUnifThumb();

    QColor getEventColor() const;

    double getTminRefCurve() const {return mTminRefCurve;}
    double getTmaxRefCurve() const {return mTmaxRefCurve;}
    void setTminRefCurve(const double tmin) { mTminRefCurve = tmin;}
    void setTmaxRefCurve(const double tmax) { mTmaxRefCurve = tmax;}

    double getFormatedTminRefCurve() const;
    double getFormatedTmaxRefCurve() const;

    double getFormatedTminCalib() const;
    double getFormatedTmaxCalib() const;

    void initDelta(Event* event);

    void updateTheta(Event* event);
    void autoSetTiSampler(const bool bSet);

    void updateDelta(Event* event);
    void updateSigmaShrinkage(Event* event);
    void updateSigmaJeffreys(Event* event);
    void updateSigmaReParam(Event* event);
    void updateWiggle();

    void generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax);



    double fProposalDensity(const double t, const double t0);

    void fMHSymetric(Event* event);
    void fInversion(Event* event);
    void fMHSymGaussAdapt(Event* event);

    void fMHSymetricWithArg(Event* event);
    void fMHSymGaussAdaptWithArg(Event* event);
    void fInversionWithArg(Event* event);

    typedef void (Date::*samplingFunction)(Event* event);

protected:
    double mTminRefCurve;
    double mTmaxRefCurve;

    samplingFunction updateti;

};





#endif