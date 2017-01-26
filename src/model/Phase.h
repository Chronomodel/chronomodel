#ifndef PHASE_H
#define PHASE_H

#include "StateKeys.h"
//#include "Event.h"
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
