#ifndef PHASE_H
#define PHASE_H

#include "Event.h"
#include "PhaseConstraint.h"
#include "MetropolisVariable.h"

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QColor>

#define STATE_PHASE_ID "id"
#define STATE_PHASE_NAME "name"
#define STATE_PHASE_RED "color_red"
#define STATE_PHASE_GREEN "color_green"
#define STATE_PHASE_BLUE "color_blue"
#define STATE_PHASE_ITEM_X "item_y"
#define STATE_PHASE_ITEM_Y "item_x"
#define STATE_PHASE_TAU_TYPE "tau_type"
#define STATE_PHASE_TAU_FIXED "tau_fixed"
#define STATE_PHASE_TAU_MIN "tau_min"
#define STATE_PHASE_TAU_MAX "tau_max"
#define STATE_PHASE_IS_SELECTED "is_selected"
#define STATE_PHASE_IS_CURRENT "is_current"


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
    
    void update(float tmin, float tmax);
    void memoAll();
    
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
    MetropolisVariable mTau;
    
    TauType mTauType;
    float mTauFixed;
    float mTauMin;
    float mTauMax;
    
    QColor mColor;
    
    float mItemX;
    float mItemY;
    
    bool mIsSelected;
    bool mIsCurrent;
};

#endif
