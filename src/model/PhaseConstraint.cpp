#include "PhaseConstraint.h"
#include "Phase.h"


PhaseConstraint::PhaseConstraint():
mId(-1),
mGammaType(PhaseConstraint::eGammaUnknown),
mGammaFixed(0),
mGammaMin(0),
mGammaMax(0),
mPhaseFrom(0),
mPhaseTo(0)
{
    
}

PhaseConstraint::PhaseConstraint(const PhaseConstraint& pc)
{
    copyFrom(pc);
}
                
PhaseConstraint& PhaseConstraint::operator=(const PhaseConstraint& pc)
{
    copyFrom(pc);
    return *this;
}

void PhaseConstraint::copyFrom(const PhaseConstraint& pc)
{
    mId = pc.mId;
    
    mPhaseFromId = pc.mPhaseFromId;
    mPhaseToId = pc.mPhaseToId;
    
    mGammaType = pc.mGammaType;
    mGammaFixed = pc.mGammaFixed;
    mGammaMin = pc.mGammaMin;
    mGammaMax = pc.mGammaMax;
    
    mPhaseFrom = pc.mPhaseFrom;
    mPhaseTo = pc.mPhaseTo;
}

PhaseConstraint::~PhaseConstraint()
{
    
}

PhaseConstraint PhaseConstraint::fromJson(const QJsonObject& json)
{
    PhaseConstraint c;
    c.mId = json[STATE_PHASE_CONSTRAINT_ID].toInt();
    c.mPhaseFromId = json[STATE_PHASE_CONSTRAINT_BWD_ID].toInt();
    c.mPhaseToId = json[STATE_PHASE_CONSTRAINT_FWD_ID].toInt();
    c.mGammaType = (PhaseConstraint::GammaType)json[STATE_PHASE_CONSTRAINT_GAMMA_TYPE].toInt();
    c.mGammaFixed = json[STATE_PHASE_CONSTRAINT_GAMMA_FIXED].toDouble();
    c.mGammaMin = json[STATE_PHASE_CONSTRAINT_GAMMA_MIN].toDouble();
    c.mGammaMax = json[STATE_PHASE_CONSTRAINT_GAMMA_MAX].toDouble();
    return c;
}

QJsonObject PhaseConstraint::toJson() const
{
    QJsonObject json;
    json[STATE_PHASE_CONSTRAINT_ID] = mId;
    json[STATE_PHASE_CONSTRAINT_BWD_ID] = mPhaseFromId;
    json[STATE_PHASE_CONSTRAINT_FWD_ID] = mPhaseToId;
    json[STATE_PHASE_CONSTRAINT_GAMMA_TYPE] = mGammaType;
    json[STATE_PHASE_CONSTRAINT_GAMMA_FIXED] = mGammaFixed;
    json[STATE_PHASE_CONSTRAINT_GAMMA_MIN] = mGammaMin;
    json[STATE_PHASE_CONSTRAINT_GAMMA_MAX] = mGammaMax;
    return json;
}

/*void PhaseConstraint::getAllPhasesFrom(Phase* phase, QList<Phase*>& results)
{
    QList<Phase*> phasesFrom = getPhasesFrom(phase);
    for(int i=0; i<(int)phasesFrom.size(); ++i)
    {
        results.push_back(phasesFrom[i]);
        getAllPhasesFrom(phasesFrom[i], results);
    }
}

QList<Phase*> PhaseConstraint::getPhasesFrom(Phase* phase)
{
    QList<PhaseConstraint*>& cb = phase->mConstraintsBwd;
    QList<Phase*> phasesFrom;
    for(int i=0; i<(int)cb.size(); ++i)
    {
        Phase* from = cb[i]->mPhaseFrom;
        if(from)
        {
            phasesFrom.push_back(from);
        }
    }
    return phasesFrom;
}

*/