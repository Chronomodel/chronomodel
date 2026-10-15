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

    double getMinThetaNextPhases(const double tmax) const;
    double getMaxThetaPrevPhases(const double tmin) const;

    double init_max_theta(const double max_default) const;
    double init_min_theta(const double min_default) const;

    std::pair<double, double> getFormatedTimeRange() const;

    void setBandwidth(BandwidthType bwt, double bandwidth);
    void generateFormatedKDE(const std::vector<ChainSpecs>& chains, const int fftLen, const double tmin, const double tmax);
    void generateActivity(size_t gridLength, double h, const double threshold, const double timeRangeLevel = 95.);

    void update_AlphaBeta(const double tminPeriod, const double tmaxPeriod);
    void update_All(const double tminPeriod, const double tmaxPeriod);
    //void memoAll(); obsolete

    inline void acquire()
    {
        mAlpha.acquire();
        mBeta.acquire();
        mDuration.acquire();
        // if (mTauType == eZOnly)
        //   mTau.memo();


#ifdef DEBUG
        if (mBeta.value() - mAlpha.value() < 0.)
            qDebug()<<"[Phase::acquire] : "<<getQStringName()<<" Warning mBeta.mX - mAlpha.mX<0";
#endif
    }

    inline void recordBurnAdapt()
    {
        mAlpha.recordBurnAdapt();
        mBeta.recordBurnAdapt();
        mDuration.recordBurnAdapt();
        // if (mTauType == eZOnly)
        //   mTau.memo();
    }
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
