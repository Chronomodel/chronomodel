/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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
#include "StudyPeriodSettings.h"

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

class Date
{

public:
    enum OriginType {
        eSingleDate = 0,
        eCombination = 1
    };

    enum DeltaType {
        eDeltaNone = -1,
        eDeltaFixed = 0,
        eDeltaGaussian = 1,
        eDeltaRange = 2
    };

    MHVariable mTi;// t i de la date
    MHVariable mSigmaTi; // sigma i de la date (par rapport au fait)
    MHVariable mWiggle;
    double mDelta;

    int mId;
    std::string mUUID;

    QColor mColor;

    QJsonObject mData;
    OriginType mOrigin;
    PluginAbstract* mPlugin;
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
    StudyPeriodSettings mSettings;

    QJsonArray mSubDates;
    double mMixingLevel;

    constexpr static const double threshold_limit = 0.00001;

public:
    Date ();
    virtual ~Date();
    Date(const QJsonObject &json);
    Date(PluginAbstract* plugin);
    Date(const Date& date);
    Date(Date&& other) noexcept;

    Date& operator=(const Date& date);
    Date& operator=(Date&& other) noexcept;
    void copyFrom(const Date& date);

    void init();
    bool isNull() const;

    void fromJson(const QJsonObject& json);
    QJsonObject toJson() const;


    inline QString getQStringName() const {return QString::fromStdString(_name);}
    inline std::string name() const {return _name;}
    void setName(const std::string name) {_name = name;}
    void setName(const QString name) {_name = name.toStdString();}


    static Date fromCSV(const QStringList &dataStr, const QLocale& csvLocale, const StudyPeriodSettings settings);
    QStringList toCSV(const QLocale& csvLocale) const;

    long double getLikelihood(const double& t) const;
    QPair<long double, long double> getLikelihoodArg(const double t) const;
    QString getDesc() const;
    QString getWiggleDesc() const;
    static QString getWiggleDesc(const QJsonObject &json); // used in CalibrationView
    PluginAbstract* getPlugin() const {return mPlugin;}

    void clear();
    void shrink_to_fit() noexcept;
    void calibrate(const StudyPeriodSettings &priod_settings, std::shared_ptr<Project> project, bool truncate); // used for item
    inline void calibrate(std::shared_ptr<Project> project, bool truncate = true) {calibrate(mSettings, project, truncate);};

    void calibrateWiggle(const StudyPeriodSettings &settings, std::shared_ptr<Project> project);
    inline void calibrateWiggle(std::shared_ptr<Project> project) {calibrateWiggle(mSettings, project);};

    double getLikelihoodFromCalib(const double &t) const;
    double getLikelihoodFromWiggleCalib(const double &t) const;

    const std::map<double, double> getFormatedCalibMap() const;
    const std::map<double, double> getFormatedWiggleCalibMap() const;
    
    const std::map<double, double> getFormatedCalibToShow() const;
    const std::map<double, double> getFormatedWiggleCalibToShow() const;

    const std::map<double, double> &getRawCalibMap() const ;
    inline const std::map<double, double> &getRawWiggleCalibMap() const;

    std::vector<double> getFormatedRepartition() const;

    QPixmap generateCalibThumb(StudyPeriodSettings settings);
    QPixmap generateUnifThumb(StudyPeriodSettings settings);

    QColor getEventColor() const;

    inline double getTminRefCurve() const {return mTminRefCurve;}
    inline double getTmaxRefCurve() const {return mTmaxRefCurve;}
    inline void setTminRefCurve(const double tmin) { mTminRefCurve = tmin;}
    inline void setTmaxRefCurve(const double tmax) { mTmaxRefCurve = tmax;}

    double getFormatedTminRefCurve() const;
    double getFormatedTmaxRefCurve() const;

    double getFormatedTminCalib() const;
    double getFormatedTmaxCalib() const;

    void initDelta();

    void updateDate(const double theta_mX, const double S02Theta_mX, const double AShrinkage);

    void updateTi(const double theta_mX);
   // void updateTi_v4(Event* event);

    void autoSetTiSampler(const bool bSet);

    void updateDelta(const double theta_mX);
    void updateSigmaShrinkage(const double theta_mX, const double S02Theta_mX, const double AShrinkage);
   // void updateSigmaShrinkage_K(const Event *event);
   // void updateSigma_v4(Event* event);

    void updateSigmaJeffreys(const double theta_mX);
   // void updateSigmaReParam(const Event* event);
    inline void updateWiggle() { mWiggle.mX = mTi.mX + mDelta;};

    void generateHistos(const std::vector<ChainSpecs> &chains, const int fftLen, const double bandwidth, const double tmin, const double tmax);

    double fProposalDensity(const double t, const double t0);

    // List of samplingFunction
    void Prior(const double theta_mX);//fMHSymetric(Event* theta_mX);
    void Inversion(const double theta_mX);

    void MHAdaptGauss(const double theta_mX);//void fMHSymGaussAdapt(Event *event);

    void PriorWithArg(const double theta_mX);//fMHSymetricWithArg(Event *event);
    void MHAdaptGaussWithArg(const double theta_mX);//void fMHSymGaussAdaptWithArg(Event* theta_mX);
    void InversionWithArg(const double theta_mX);

    typedef void (Date::*samplingFunction)(const double theta_mX);

protected:
    double mTminRefCurve;
    double mTmaxRefCurve;

    samplingFunction updateti;

private:

    std::string _name;
    void moveFrom(Date&& other) noexcept;

};


CalibrationCurve generate_mixingCalibration(const std::vector<Date> &dates, const std::string description = "Mixing Calibrations");


#endif
