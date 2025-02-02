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

#ifndef PHASE_H
#define PHASE_H

#include "PhaseConstraint.h"
#include "MetropolisVariable.h"

#include <unordered_map>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QColor>

class Model;
class Event;

class Phase
{
public:
    enum TauType{
        eTauUnknown = 0,
        eTauFixed = 1,
     //   eTauRange = 2, // usefull to convert old file in Project::checkDatesCompatibility()
        eZOnly = 2
    };

    Phase ();
    Phase (const Phase& phase);
    explicit Phase (const QJsonObject& json);
    Phase& operator=(const Phase& phase);
    void copyFrom(const Phase& phase);
    virtual ~Phase();

    void clear();
    void shrink_to_fit();
    void clear_and_shrink() noexcept;

    static Phase fromJson(const QJsonObject& json);
    QJsonObject toJson() const;


    inline QString getQStringName() const {return QString::fromStdString(_name);}
    inline std::string name() const {return _name;}
    void setName(const std::string name) {_name = name;}
    void setName(const QString name) {_name = name.toStdString();}

    double sum_gamma_prev_phases();
    double sum_gamma_next_phases();
    void init_alpha_beta_phase(std::vector<std::shared_ptr<Phase> > &phases);
    void init_update_alpha_phase(double theta_max_phase_prev);
    void init_update_beta_phase(double beta_sup);

    double getMaxThetaEvents(double tmax);
    double getMinThetaEvents(double tmin);

    double getMinThetaNextPhases(const double tmax);
    double getMaxThetaPrevPhases(const double tmin);

    double init_max_theta(const double max_default);
    double init_min_theta(const double min_default);

    std::pair<double, double> getFormatedTimeRange() const;

    void generateHistos(const std::vector<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax);
    void generateActivity(size_t gridLength, double h, const double threshold, const double timeRangeLevel = 95.);

    void update_AlphaBeta(const double tminPeriod, const double tmaxPeriod);
    void update_All(const double tminPeriod, const double tmaxPeriod);
    void memoAll();

    QString getTauTypeText() const;
    void initTau(const double tminPeriod, const double tmaxPeriod);
    void update_Tau(const double tminPeriod, const double tmaxPeriod);

public:
    int mId;
    QColor mColor;

    std::vector<std::shared_ptr<Event>> mEvents;
    std::vector<std::shared_ptr<PhaseConstraint>> mConstraintsNextPhases;
    std::vector<std::shared_ptr<PhaseConstraint>> mConstraintsPrevPhases;

    MetropolisVariable mAlpha;
    MetropolisVariable mBeta;

    std::pair<double, double> mTimeRange;

    MetropolisVariable mDuration;
    QString mDurationCredibility;

    std::map<double, double> mTempo;
    std::map<double, double> mTempoInf;
    std::map<double, double> mTempoSup;

    std::map<double, double> mActivity;
    std::map<double, double> mActivityInf;
    std::map<double, double> mActivitySup;
    std::map<double, double> mActivityUnifTheo;


    // Raw curve without date format

    std::map<double, double> mRawTempo;
    std::map<double, double> mRawTempoInf;
    std::map<double, double> mRawTempoSup;

    std::map<double, double> mRawActivity;
    std::map<double, double> mRawActivityInf;
    std::map<double, double> mRawActivitySup;
    std::map<double, double> mRawActivityUnifTheo;

    std::unordered_map<std::string, double> mValueStack;

    MetropolisVariable mTau;
    TauType mTauType;
    double mTauFixed;
    double mTauMin;
    double mTauMax;

    double mItemX;
    double mItemY;

    bool mIsSelected;
    bool mIsCurrent;

    int mLevel; // ?? is it usefull ??

private:
    std::string _name;

};

#endif
