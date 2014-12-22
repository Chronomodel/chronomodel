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
    
    static Phase fromJson(const QJsonObject& json);
    QJsonObject toJson() const;

    void addEvent(Event* event);
    void removeEvent(Event* event);
    
    void addConstraintForward(PhaseConstraint* c);
    void addConstraintBackward(PhaseConstraint* c);
    void removeConstraintForward(PhaseConstraint* c);
    void removeConstraintBackward(PhaseConstraint* c);
    
    float getMaxThetaEvents();
    float getMinThetaEvents();
    float getMinAlphaNextPhases(float tmax);
    float getMaxBetaPrevPhases(float tmin);
    
    void updateAll();
    void memoAll();
    
    void initTau();
    void updateTau();
    
    void generateDurationCredibility();
    
protected:
    float updatePhaseBound(float a, float b, float old);

    //---------------------------------------------------
    
public:
    int mId;
    QString mName;
    
    QList<Event*> mEvents;
    QList<PhaseConstraint*> mConstraintsFwd;
    QList<PhaseConstraint*> mConstraintsBwd;
    
    MetropolisVariable mAlpha;
    MetropolisVariable mBeta;
    float mTau;
    
    QVector<float> mDurations;
    QString mDurationCredibility;
    
    TauType mTauType;
    float mTauFixed;
    float mTauMin;
    float mTauMax;
    
    QColor mColor;
    
    float mItemX;
    float mItemY;
    
    bool mIsSelected;
    bool mIsCurrent;
    
    int mLevel;
};

#endif
