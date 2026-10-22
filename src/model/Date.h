/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
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

#include <array>
#include <limits>
#include <vector>

class Event;
class PluginAbstract;
class CalibrationCurve;
class Project;
class RefCurve;

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

    // ------------------------------------------------------------------
    // Log-vraisemblance rapide pour les samplers MCMC (tempering_339_...)
    // Meme fonction que log(getLikelihood(t)), sans approximation :
    //  - noyau precompile : parametres de mesure figes + courbe de reference a plat
    //    (aucun acces QJsonObject / QString / QMap par evaluation) ;
    //    plugin non pris en charge -> repli sur log(getLikelihood(t)) ;
    //  - cache a 2 entrees indexe par la valeur exacte de t (L(t_courant) n'est
    //    jamais recalcule).
    // Etat ecrit uniquement par le thread MCMC ; remis a zero par copie, deplacement
    // et fromJson (reconstruit a la premiere demande).
    // ------------------------------------------------------------------
    // Composante c (c < nComp) : mesure y_c, variance de mesure s2_c, courbe m_c(t), e_c(t)
    //   v_c(t) = s2_c + e_c(t)^2 ,  q_c(t) = -1/2 (y_c - m_c(t))^2
    // eGauss   (nComp = 1) : 14C, F14C, Gauss (mode courbe), AM I, D ou F seul
    //   log L = q_0 / v_0 - 1/2 log v_0
    // eMixture (nComp = 2 ou 3) : AM ID, IF, IDF (somme sur integration_steps de PluginMag)
    //   L = sum_i exp( sum_c q_c / (v_c + add_c[i]) ) / sqrt( prod_c (v_c + add_c[i]) )
    // ePoly    (nComp = 1) : Gauss modes « none » et « eq » (PluginGauss::getRefValueAt), sans courbe
    //   g(t) = a t^2 + b t + c  (none : a = 0, b = 1, c = 0) ,  log L = -1/2 (y_0 - g(t))^2 / s2_0 - 1/2 log s2_0
    struct LogLikeKernel {
        enum Kind { eNotBuilt, eGeneric, eGauss, eMixture, ePoly };
        Kind                               kind  = eNotBuilt;
        int                                nComp = 0;
        std::array<double, 3>              y     {};
        std::array<double, 3>              s2    {};
        std::array<const RefCurve*, 3>     curve {};
        std::array<std::vector<double>, 3> add;     // eMixture : terme ajoute a v_c, par pas i
        std::array<double, 3>              poly  {};  // ePoly : coefficients a, b, c de g(t)
        double                             halfLogS2 = 0.0; // ePoly : 1/2 log s2_0 (constant)
    };

    struct LogLikeMemo {
        std::array<double, 2> t { std::numeric_limits<double>::quiet_NaN(),
                                  std::numeric_limits<double>::quiet_NaN() };
        std::array<double, 2> v { -std::numeric_limits<double>::infinity(),
                                  -std::numeric_limits<double>::infinity() };
        int slot = 0;
    };

    void   buildLogLikelihoodKernel();
    double getLogLikelihoodKernel(const double t) const;   // sans cache ; noyau deja construit
    double getLogLikelihoodMemo(const double t);           // avec cache ; construit le noyau si besoin
    void   resetLogLikelihoodCache() noexcept { mLogLikeKernel = LogLikeKernel(); mLogLikeMemo = LogLikeMemo(); }

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

    // vraisemblance rapide (voir getLogLikelihoodMemo) : jamais copie
    LogLikeKernel mLogLikeKernel;
    LogLikeMemo   mLogLikeMemo;

};


CalibrationCurve generate_mixingCalibration(const std::vector<Date> &dates, const std::string description = "Mixing Calibrations");


#endif
