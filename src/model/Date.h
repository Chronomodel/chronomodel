/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2026

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
    double mXi; //  test changement de variable pour le nouveau prior EDM2 sur sigmaTi

   // MHVariable mZi;// test reparametrisation, ,ne marche pas

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


    inline QString getQStringName() const {return QString::fromStdString(mName);}
    inline std::string name() const {return mName;}
    void setName(const std::string name) {mName = name;}
    void setName(const QString name) {mName = name.toStdString();}


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

    QPixmap generateCalibThumb(const StudyPeriodSettings &settings);
    QPixmap generateUnifThumb(const StudyPeriodSettings& settings);

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

#pragma mark Fonctions pour v3
    void updateDate_v3(const double theta, const double S02Theta);
    inline void updateTi_v3(const double theta)
    {
        (this->*updateti) (theta);
    }
    void applyTi_v3(const double theta);
    void updateDelta_v3(const double theta, const double = 0);
    inline void applyDelta_v3(const double theta, const double )
    {
        updateDelta_v3(theta);
    }

    void updateSigma_Variance(const double theta,
                              const double S02Theta);

    void updateSigma_Shrinkage(const double theta,
                          const double S02Theta);

    void updateSigma_Log10(const double theta, const double S02Theta);

#pragma mark Fonctions pour v4
    // fonction qui utilise mXi

    /*void updateDelta_v4(const double theta, const double S02Theta);
    void applyDelta_v4(const double theta, const double S02Theta);
    void updateTi_v4(const double theta, const double S02Theta);
    void applyTi_v4(const double theta, const double S02Theta);

    // obsolete
    inline void updateSigma_v4 (const double theta)
    { // mise à jour déterministe
       // const double t_delta_theta = abs((mTi.value() + mDelta) - theta);
       // mSigmaTi.setValue( t_delta_theta / sqrt(mXi) );
    }
*/
#pragma mark Fonction General

    inline void updateTi(const double theta)
    {
        updateTi_v3(theta);
    }

    inline void applyTi(const double theta)
    {
        applyTi_v3(theta);
    }

    inline void updateDelta(const double theta, const double S02Theta = 0)
    {
        updateDelta_v3(theta, S02Theta);
    }

    inline void applyDelta(const double theta, const double S02Theta)
    {
        applyDelta_v3(theta, S02Theta);
    }

    inline void updateSigma (const double theta, const double S02Theta)
    {
        updateSigma_Shrinkage(theta, S02Theta);
        //updateSigma_Variance(theta, S02Theta);
        //updateSigma_Log10(theta, S02Theta);
    }
    inline void applySigma (const double theta, const double S02Theta)
    {
        applySigmaShrinkage_K_tempering(theta, S02Theta);
    }

    inline void updateWiggle()
    {
        // déterministe
        mWiggle.setValue(mTi.value() + mDelta);
    }
    // Alias demandé – wrapper inline, zéro coût après optimisation
    inline void applyWiggle()
    {
        updateWiggle();   // le compilateur inline‑ra cet appel
    }

    inline void updateDate(const double theta, const double S02Theta)
    {
        updateTi(theta);

        updateDelta(theta, S02Theta);
        updateSigma(theta, S02Theta);

        updateWiggle();
    }

    inline void applyDate(const double theta, const double S02Theta)
    {
        applyTi(theta);

        applyDelta(theta, S02Theta);
        applySigma(theta, S02Theta);

        updateWiggle();
    }

    void applySigmaShrinkage_K_tempering(const double theta_mX,
                                         const double S02Theta_mX,
                                         const double T = 1.0);

    void updateSigmaJeffreys(const double theta_mX);


    void autoSetTiSampler(const bool bSet);

    void updateSigmaShrinkage0(const double theta_mX, const double S02Theta_mX);
    void updateSigmaShrinkage(const double theta_mX, const double S02Theta_mX);
    void updateSigmaShrinkage_K(const double theta_mX, const double S02Theta_mX);

    void updateSigmaWithX(const double theta, const double S02Theta);


    void setBandwidth(BandwidthType bwt, double bandwidth);
    void generateFormatedKDE(const std::vector<ChainSpecs> &chains, const int fftLen, const double tmin, const double tmax);

    double fProposalDensity(const double t, const double t0);

    // List of samplingFunction
    void Prior(const double theta_mX);
    void applyPrior(const double theta_mX);

    void Inversion(const double theta_mX);
    void applyInversion(const double theta_mX);

    void MHAdaptGauss(const double theta_mX);
    void applyMHAdaptGauss(const double theta_mX, const double T = 1);

    void PriorWithArg(const double theta_mX);//fMHSymetricWithArg(Event *event);
    void MHAdaptGaussWithArg(const double theta_mX);//void fMHSymGaussAdaptWithArg(Event* theta_mX);
    void InversionWithArg(const double theta_mX);

    typedef void (Date::*samplingFunction)(const double theta_mX);

    void applyDateProposal_v3(const double theta, const double S02Theta);
    void applyTi_MH_Tempering(const double theta_mX, const double T);


protected:
    double mTminRefCurve;
    double mTmaxRefCurve;

    samplingFunction updateti;

private:

    std::string mName;
    void moveFrom(Date&& other) noexcept;

};


CalibrationCurve generate_mixingCalibration(const std::vector<Date> &dates, const std::string description = "Mixing Calibrations");


#endif
