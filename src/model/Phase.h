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
#define STATE_PHASE_EVENTS_IDS "events_ids"
#define STATE_PHASE_CONSTRAINTS_FWD_IDS "constraints_fwd_ids"
#define STATE_PHASE_CONSTRAINTS_BWD_IDS "constraints_bwd_ids"
#define STATE_PHASE_IS_SELECTED "is_selected"


class Phase
{
public:
    enum TauType{
        eTauUnknown = 0,
        eTauRange = 1
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
    
    QList<int> mEventsIds;
    QList<int> mConstraintsFwdIds;
    QList<int> mConstraintsBwdIds;
    
    MetropolisVariable mAlpha;
    MetropolisVariable mBeta;
    MetropolisVariable mThetaPredict;
    
    TauType mTauType;
    float mTauFixed;
    float mTauMin;
    float mTauMax;
    
    QColor mColor;
    
    float mItemX;
    float mItemY;
    
    bool mIsSelected;
};

#endif
