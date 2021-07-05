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

#ifndef PHASE_H
#define PHASE_H

#include "StateKeys.h"
#include "PhaseConstraint.h"
#include "MetropolisVariable.h"

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QColor>

class Event;

class Phase
{
public:
    enum TauType{
        eTauUnknown = 0,
        eTauFixed = 1,
        eTauRange = 2
    };

    Phase();
    Phase(const Phase& phase);
    Phase& operator=(const Phase& phase);
    void copyFrom(const Phase& phase);
    virtual ~Phase();

    static Phase fromJson(const QJsonObject& json);
    QJsonObject toJson() const;

    double getMaxThetaEvents(double tmax);
    double getMinThetaEvents(double tmin);

    double getMinThetaNextPhases(const double tmax);
    double getMaxThetaPrevPhases(const double tmin);

    QPair<double,double> getFormatedTimeRange() const;

    void generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax);

    void updateAll(const double tmin, const double tmax);
    void memoAll();

    QString getTauTypeText() const;
    void initTau();
    void updateTau();

public:
    int mId;

    QString mName; //must be public, to be setting by dialogbox
    QColor mColor;

    QList<Event*> mEvents;
    QList<PhaseConstraint*> mConstraintsFwd;
    QList<PhaseConstraint*> mConstraintsBwd;

    MetropolisVariable mAlpha;
    MetropolisVariable mBeta;
    double mTau;
    QPair<double,double> mTimeRange;

    // Used to display correctly if alpha or beta is a fixed bound
   /* bool mIsAlphaFixed;
    bool mIsBetaFixed;
   */
    MetropolisVariable mDuration;
    QString mDurationCredibility;

    QMap<double, double> mTempo;
    QMap<double, double> mTempoInf;
    QMap<double, double> mTempoSup;

    QMap<double, double> mTempoCredibilityInf;
    QMap<double, double> mTempoCredibilitySup;

    QMap<double, double> mActivity;
    QMap<double, double> mActivityInf;
    QMap<double, double> mActivitySup;
    double mActivityMeanUnif;
    double mActivityStdUnif;

    // Raw curve without date format

    QMap<double, double> mRawTempo;
    QMap<double, double> mRawTempoInf;
    QMap<double, double> mRawTempoSup;

    QMap<double, double> mRawTempoCredibilityInf;
    QMap<double, double> mRawTempoCredibilitySup;

    QMap<double, double> mRawActivity;
    QMap<double, double> mRawActivityInf;
    QMap<double, double> mRawActivitySup;

    TauType mTauType;
    double mTauFixed;
    double mTauMin;
    double mTauMax;

    double mItemX;
    double mItemY;

    bool mIsSelected;
    bool mIsCurrent;

    int mLevel;

};

#endif
