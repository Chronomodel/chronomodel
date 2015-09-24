#ifndef PHASE_H
#define PHASE_H

#include "StateKeys.h"
#include "Event.h"
#include "PhaseConstraint.h"
#include "MetropolisVariable.h"

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QColor>


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
    
    //Added by PhD
    void setJson(QJsonObject & iJson, const int idxPhase);
    QJsonObject & getJson();
    
    static Phase fromJson(const QJsonObject& json);
    
    QJsonObject toJson() const;
    
    double getMaxThetaEvents(double tmax);
    double getMinThetaEvents(double tmin);
    
    double getMinThetaNextPhases(double tmax);
    double getMaxThetaPrevPhases(double tmin);
    
    void updateAll(double tmin, double tmax);
    void memoAll();
    
    void initTau();
    void updateTau();
    
    QColor getColor() const;
    
public:
    int mId;
    QString mName;
    
    QList<Event*> mEvents;
    QList<PhaseConstraint*> mConstraintsFwd;
    QList<PhaseConstraint*> mConstraintsBwd;
    
    MetropolisVariable mAlpha;
    MetropolisVariable mBeta;
    double mTau;
    
    // Used to display correctly if alpha or beta is a fixed bound
    bool mIsAlphaFixed;
    bool mIsBetaFixed;
    
    MetropolisVariable mDuration;
    QString mDurationCredibility;
    
    TauType mTauType;
    double mTauFixed;
    double mTauMin;
    double mTauMax;
    
    QColor mColor;
    
    double mItemX;
    double mItemY;
    
    bool mIsSelected;
    bool mIsCurrent;
    
    int mLevel;
    
private:
    QJsonObject * mJson;
    int mJsonPhaseIdx;
};

#endif
