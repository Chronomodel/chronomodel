#ifndef PHASECONSTRAINT_H
#define PHASECONSTRAINT_H

#include "Constraint.h"
#include "MHVariable.h"
#include "StateKeys.h"

class Phase;


class PhaseConstraint: public Constraint
{
public:
    enum GammaType{
        eGammaUnknown = 0,
        eGammaFixed = 1,
        eGammaRange = 2
    };
    
    PhaseConstraint();
    PhaseConstraint(const PhaseConstraint& pc);
    PhaseConstraint& operator=(const PhaseConstraint& pc);
    void copyFrom(const PhaseConstraint& pc);
    virtual ~PhaseConstraint();
    
    static PhaseConstraint fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    void update();
    
public:
    float mGamma;
    
    Phase* mPhaseFrom;
    Phase* mPhaseTo;
    
    GammaType mGammaType;
    float mGammaFixed;
    float mGammaMin;
    float mGammaMax;
};

#endif
