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
    double mBandwidth;
    int mFFTLength;
    double mHActivity;
    // Stockage des courbes binomiales en fonction de n
    std::unordered_map<int, std::vector<double>> mBinomiale_Gx;

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
    QList<QStringList> getEventsTraces(const QLocale locale, const bool withDateFormat = false);

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

    t_reduceTime reduceTime(double t) const;
    std::vector<t_reduceTime> reduceTime(const std::vector<double> &vec_t) const;

    double yearTime(t_reduceTime reduceTime);

    virtual void saveToStream(QDataStream* out) const ;

    virtual bool loadFromStream(QDataStream* in) {return loadFromStream_v330(in);};

    bool loadFromStream_v323(QDataStream* in);
    bool loadFromStream_v324(QDataStream* in);
    bool loadFromStream_v328(QDataStream* in);
    bool loadFromStream_v330(QDataStream* in);

    // Only trace needed for this :
    virtual void generateCorrelations(const std::vector<ChainSpecs>& chains);

    double getThreshold() const;
    double getBandwidth() const;
    int getFFTLength() const;

    virtual void setThreshold(const double threshold);
    void setBandwidth(const double bandwidth);
    void setFFTLength(int FFTLength);
    void setHActivity(const double h, const double rangePercent);

    virtual void setThresholdToAllModel(const double threshold);
    void initDensities();
    void updateDensities(int fftLen, double bandwidth, double threshold);

    // Computed from trace using FFT :
    virtual void generatePosteriorDensities(const std::vector<ChainSpecs> &chains, int fftLen, double bandwidth);
    // Trace and Posterior density needed for this :

    virtual void generateCredibility(const double threshold);
    virtual void generateHPD(const double threshold);
    // Trace and Posterior density needed for this :
    virtual void generateNumericalResults(const std::vector<ChainSpecs>& chains);

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
    virtual void memo_accept(const unsigned i_chain);
    virtual void initVariablesForChain();




#pragma mark curve variables




};

#endif
