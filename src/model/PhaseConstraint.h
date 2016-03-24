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
    void copyFrom(const Constraint& pc);
    virtual ~PhaseConstraint();
    
    static PhaseConstraint fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    void initGamma();
    void updateGamma();

    QPair<double,double> getFormatedGapRange() const;

    
public:
    double mGamma;

    Phase* mPhaseFrom;
    Phase* mPhaseTo;
    
    GammaType mGammaType;
    double mGammaFixed;
    double mGammaMin;
    double mGammaMax;

    QPair<double,double> mGapRange;

};

#endif
