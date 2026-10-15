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

#ifndef MODEL_H
#define MODEL_H

#include "StudyPeriodSettings.h"
#include "MCMCSettings.h"
#include "Event.h"
#include "Phase.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"

#include <QJsonObject>


class Model
{

public:
    StudyPeriodSettings mSettings;

    MCMCSettings mMCMCSettings;

    std::vector<std::shared_ptr<Event>> mEvents;
    std::vector<std::shared_ptr<Phase>> mPhases;
    std::vector<std::shared_ptr<EventConstraint>> mEventConstraints;
    std::vector<std::shared_ptr<PhaseConstraint>> mPhaseConstraints;


    std::vector<ChainSpecs> mChains;

    QString mLogModel;
    QString mLogInit;
    QString mLogAdapt;
    QString mLogResults;

    int mNumberOfPhases;
    int mNumberOfEvents;
    int mNumberOfDates;

    double mThreshold; // used for TimeRange + Credibility + transition Range + GapRange

    BandwidthType mBandwidthType;
    double mBandwidth;


    int mFFTLength;
    double mHActivity;
    // Stockage des courbes binomiales en fonction de n
    std::unordered_map<int, std::vector<double>> mBinomiale_Gx;

    MCMCDiagnostic::ConvergenceSummary mConvergenceSummary;
protected:
    QStringList mCurveName;
    QStringList mCurveLongName;

public:
    Model();
    explicit Model(const QJsonObject& json);
    virtual ~Model();

    //void generateModelLog();
    QString getModelLog() const;

    virtual void generateResultsLog();
    QString getResultsLog() const;

    QString getInitLog() const;
    QString getAdaptLog() const;

    QList<QStringList> getStats(const QLocale locale, const int precision, const bool withDateFormat = false);
    QList<QStringList> getPhasesTraces(QLocale locale, const bool withDateFormat = false);
    QList<QStringList> getPhaseTrace(size_t phaseIdx, const QLocale locale, const bool withDateFormat = false);
    // liste des chaines des Events pendant la partie acquisition.
    QList<QStringList> getEventsTraces(const QLocale& locale, const bool withDateFormat = false) const noexcept;

    inline QStringList getCurvesName () const {return mCurveName;}
    inline QStringList getCurvesLongName () const {return mCurveLongName;}
    inline bool displayX () const {return !mCurveName.isEmpty();}
    inline bool displayY () const {return mCurveName.size()>1;}
    inline bool displayZ () const {return mCurveName.size()>2;}

    virtual void updateFormatSettings();

    virtual QJsonObject toJson() const;
    virtual void fromJson( const QJsonObject& json);

    bool isValid();
    void clear();
    void shrink_to_fit();
    void clear_and_shrink() noexcept;

    void initNodeEvents(); // use in MCMCLoopChrono::initialize()
    QString initializeTheta();

    virtual t_reduceTime reduceTime(const double t) const
    {
        const long double tmin = static_cast<long double>(mSettings.mTmin);
        const long double tmax = static_cast<long double>(mSettings.mTmax);
        const long double tL = static_cast<long double>(t);
        const long double denominator = tmax - tmin;

#ifdef DEBUG
        // Vérification de la stabilité numérique
        if (std::abs(denominator) < std::numeric_limits<long double>::epsilon()) {
            throw std::runtime_error(" [Model::reduceTime] Erreur : Division par une valeur trop petite !");
        }
#endif

        return static_cast<double>((tL - tmin) / denominator);
    }

    std::vector<t_reduceTime> reduceTime(const std::vector<double> &vec_t) const;

    inline double yearTime(const t_reduceTime reduceTime) const
    {
        const double tmin = mSettings.mTmin;
        const double tmax = mSettings.mTmax;
        return reduceTime * (tmax - tmin) + tmin ;
    }

    virtual void saveToStream(QDataStream* out) const ;

    virtual bool loadFromStream(QDataStream* in) {return loadFromStream_v338(in);};

    bool loadFromStream_v323(QDataStream* in);
    bool loadFromStream_v324(QDataStream* in);
    bool loadFromStream_v328(QDataStream* in);
    bool loadFromStream_v330(QDataStream* in);

    bool loadFromStream_v338(QDataStream* in);

    // Only trace needed for this :
    virtual void generateCorrelations(const std::vector<ChainSpecs>& chains);


    double getThreshold() const
    {
        return mThreshold;
    }

    double getBandwidth() const
    {
        return mBandwidth;
    }

    int getFFTLength() const
    {
        return mFFTLength;
    }
    virtual void setThreshold(const double threshold);
    void setBandwidth(BandwidthType bwt, const double bandwidth);
    void setFFTLength(int FFTLength);
    void setHActivity(const double h, const double rangePercent);

    virtual void setThresholdToAllModel(const double threshold);
    void initDensities();
    void updateDensities(int fftLen, BandwidthType bandwidthType, double bandwidth, double threshold);

    // Computed from trace using FFT :
    virtual void generatePosteriorDensities(const std::vector<ChainSpecs> &chains, int fftLen,  BandwidthType bwt, double bandwidth = 1.);
    // Trace and Posterior density needed for this :

    virtual void generateCredibility(const double threshold);
    virtual void generateHPD(const double threshold);


    // Trace and Posterior density needed for this :
    //virtual void generateNumericalResults(const std::vector<ChainSpecs>& chains); // obsolete
    virtual void generateDensityNumericalResults();
    virtual void generateTraceNumericalResults(const std::vector<ChainSpecs>& chains);

    void generateTempo(const size_t gridLength);

    void generateActivity(const size_t gridLenth, const double h, const double threshold, const double rangePercent = 95.);
    void generateActivityBinomialeCurve(const int n, std::vector<double>& C1x, std::vector<double>& C2x, const double alpha = .05);

    virtual void clearTraces();
    virtual void remove_smoothed_densities();//clearPosteriorDensities();
    virtual void clearCredibilityAndHPD();
    virtual void clearThreshold();
    
    bool hasSelectedEvents();
    bool hasSelectedPhases();

#pragma mark Loop
   // virtual void memo_accepted_state(const unsigned i_chain);
    virtual void setParametersForChain();



};

#endif
