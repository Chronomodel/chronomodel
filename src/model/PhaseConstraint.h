#ifndef PHASECONSTRAINT_H
#define PHASECONSTRAINT_H

#include "MHVariable.h"
#include <QJsonObject>

class Phase;

#define STATE_PHASE_CONSTRAINT_ID "id"
#define STATE_PHASE_CONSTRAINT_BWD_ID "bwd_id"
#define STATE_PHASE_CONSTRAINT_FWD_ID "fwd_id"
#define STATE_PHASE_CONSTRAINT_GAMMA_TYPE "gamma_type"
#define STATE_PHASE_CONSTRAINT_GAMMA_FIXED "gamma_fixed"
#define STATE_PHASE_CONSTRAINT_GAMMA_MIN "gamma_min"
#define STATE_PHASE_CONSTRAINT_GAMMA_MAX "gamma_max"


class PhaseConstraint
{
public:
    enum GammaType{
        eGammaUnknown = 0,
        eGammaRange = 1,
        eGammaFixed = 2
    };
    
    PhaseConstraint();
    PhaseConstraint(const PhaseConstraint& pc);
    PhaseConstraint& operator=(const PhaseConstraint& pc);
    void copyFrom(const PhaseConstraint& pc);
    virtual ~PhaseConstraint();
    
    static PhaseConstraint fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
public:
    int mId;
    int mPhaseFromId;
    int mPhaseToId;
    
    GammaType mGammaType;
    float mGammaFixed;
    float mGammaMin;
    float mGammaMax;
    
    Phase* mPhaseFrom;
    Phase* mPhaseTo;
    
    MHVariable mGamma;
};

#endif
