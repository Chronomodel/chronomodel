#include "Phase.h"
#include "Event.h"
#include "PhaseConstraint.h"
#include "Generator.h"
#include <QtWidgets>


Phase::Phase():
mId(0),
mTauType(Phase::eTauUnknown),
mTauFixed(0),
mTauMin(0),
mTauMax(0),
mIsSelected(false),
mIsCurrent(false)
{
    mColor = QColor(120 + rand() % 50,
                    120 + rand() % 50,
                    120 + rand() % 50);
    
    // Item initial position :
    int posDelta = 100;
    mItemX = rand() % posDelta - posDelta/2;
    mItemY = rand() % posDelta - posDelta/2;
}

Phase::Phase(const Phase& phase)
{
    copyFrom(phase);
}

Phase& Phase::operator=(const Phase& phase)
{
    copyFrom(phase);
    return *this;
}

void Phase::copyFrom(const Phase& phase)
{
    mId = phase.mId;
    mName = phase.mName;
    
    mAlpha = phase.mAlpha;
    mBeta = phase.mBeta;
    mTau = phase.mTau;
    
    mTauType = phase.mTauType;
    mTauFixed = phase.mTauFixed;
    mTauMin = phase.mTauMin;
    mTauMax = phase.mTauMax;
    
    mColor = phase.mColor;
    
    mItemX = phase.mItemX;
    mItemY = phase.mItemY;
    
    mIsSelected = phase.mIsSelected;
    mIsCurrent = phase.mIsCurrent;
    
    mEvents = phase.mEvents;
    mConstraintsFwd = phase.mConstraintsFwd;
    mConstraintsBwd = phase.mConstraintsBwd;
}

Phase::~Phase()
{
    
}


Phase Phase::fromJson(const QJsonObject& json)
{
    Phase p;
    p.mId = json[STATE_ID].toInt();
    p.mName = json[STATE_NAME].toString();
    p.mColor = QColor(json[STATE_COLOR_RED].toInt(), json[STATE_COLOR_GREEN].toInt(), json[STATE_COLOR_BLUE].toInt());
    p.mItemX = json[STATE_ITEM_X].toDouble();
    p.mItemY = json[STATE_ITEM_Y].toDouble();
    p.mTauType = (Phase::TauType)json[STATE_PHASE_TAU_TYPE].toInt();
    p.mTauFixed = json[STATE_PHASE_TAU_FIXED].toDouble();
    p.mTauMin = json[STATE_PHASE_TAU_MIN].toDouble();
    p.mTauMax = json[STATE_PHASE_TAU_MAX].toDouble();
    p.mIsSelected = json[STATE_IS_SELECTED].toBool();
    p.mIsCurrent = json[STATE_IS_CURRENT].toBool();
    
    return p;
}

QJsonObject Phase::toJson() const
{
    QJsonObject phase;
    
    phase[STATE_ID] = mId;
    phase[STATE_NAME] = mName;
    phase[STATE_COLOR_RED] = mColor.red();
    phase[STATE_COLOR_GREEN] = mColor.green();
    phase[STATE_COLOR_BLUE] = mColor.blue();
    phase[STATE_ITEM_X] = mItemX;
    phase[STATE_ITEM_Y] = mItemY;
    phase[STATE_PHASE_TAU_TYPE] = mTauType;
    phase[STATE_PHASE_TAU_FIXED] = mTauFixed;
    phase[STATE_PHASE_TAU_MIN] = mTauMin;
    phase[STATE_PHASE_TAU_MAX] = mTauMax;
    phase[STATE_IS_SELECTED] = mIsSelected;
    phase[STATE_IS_CURRENT] = mIsCurrent;
    
    return phase;
}

void Phase::addEvent(Event* event)
{
    if(event)
    {
        for(int i=0; i<mEvents.size(); ++i)
            if(mEvents[i] == event)
                return;
        
        mEvents.push_back(event);
    }
}

void Phase::removeEvent(Event* event)
{
    for(int i=0; i<mEvents.size(); ++i)
    {
        if(mEvents[i] == event)
        {
            mEvents.erase(mEvents.begin() + i);
            break;
        }
    }
}


void Phase::addConstraintForward(PhaseConstraint* c)
{
    if(c)
    {
        mConstraintsFwd.push_back(c);
    }
    else
        throw "Cannot add null phase constraint to phase.";
}
void Phase::addConstraintBackward(PhaseConstraint* c)
{
    if(c)
    {
        mConstraintsBwd.push_back(c);
    }
    else
        throw "Cannot add null phase constraint to phase.";
}

void Phase::removeConstraintForward(PhaseConstraint* c)
{
    for(int i=0; i<(int)mConstraintsFwd.size(); ++i)
    {
        if(mConstraintsFwd[i] == c)
        {
            mConstraintsFwd.erase(mConstraintsFwd.begin() + i);
        }
    }
}

void Phase::removeConstraintBackward(PhaseConstraint* c)
{
    for(int i=0; i<(int)mConstraintsBwd.size(); ++i)
    {
        if(mConstraintsBwd[i] == c)
        {
            mConstraintsBwd.erase(mConstraintsBwd.begin() + i);
        }
    }
}

// --------------------------------------------------------------------------------

float Phase::getMaxThetaEvents()
{
    if(mEvents.size() > 0)
    {
        float maxTheta = (*(mEvents.begin()))->mTheta.mX;;
        // ------------------------------------------------------------------------------------
        //  Itération sur les faits de la phase
        // ------------------------------------------------------------------------------------
        for(QList<Event*>::iterator it = mEvents.begin() ; it != mEvents.end(); ++it)
        {
            float theta = (*it)->mTheta.mX;
            maxTheta = std::max(maxTheta, theta);
        }
        return maxTheta;
    }
    return 0;
}

float Phase::getMinThetaEvents()
{
    if(mEvents.size() > 0)
    {
        float minTheta = (*(mEvents.begin()))->mTheta.mX;
        // ------------------------------------------------------------------------------------
        //  Itération sur les faits de la phase
        // ------------------------------------------------------------------------------------
        for(QList<Event*>::iterator it = mEvents.begin() ; it != mEvents.end(); ++it)
        {
            float theta = (*it)->mTheta.mX;
            minTheta = std::min(minTheta, theta);
        }
        return minTheta;
    }
    return 0;
}

float Phase::getMinAlphaNextPhases(float tmax)
{
    float minAlpha = tmax;
    // ------------------------------------------------------------------------------------
    //  Itération sur les phases en contrainte après pour trouver leur alpha min
    // ------------------------------------------------------------------------------------
    for(QList<PhaseConstraint*>::iterator it = mConstraintsFwd.begin() ; it != mConstraintsFwd.end(); ++it)
    {
        float alpha = (*it)->mPhaseTo->mAlpha.mX;
        minAlpha = std::min(minAlpha, alpha);
    }
    return minAlpha;
}

float Phase::getMaxBetaPrevPhases(float tmin)
{
    float maxBeta = tmin;
    // ------------------------------------------------------------------------------------
    //  Itération sur les phases en contrainte avant pour trouver leur beta max
    // ------------------------------------------------------------------------------------
    for(QList<PhaseConstraint*>::iterator it = mConstraintsBwd.begin() ; it != mConstraintsBwd.end(); ++it)
    {
        float beta = (*it)->mPhaseFrom->mBeta.mX;
        maxBeta = std::min(maxBeta, beta);
    }
    return maxBeta;
}

// --------------------------------------------------------------------------------

void Phase::update(float tmin, float tmax)
{
    mAlpha.mX = getMinThetaEvents();
    mBeta.mX = getMaxThetaEvents();
    
    if(mTauType == eTauUnknown)
    {
        // Nothing to do!
    }
    else if(mTauType == eTauFixed && mTauFixed != 0)
        mTau = mTauFixed;
    else if(mTauType == eTauRange && mTauMax > mTauMin)
        mTau = Generator::randomUniform(qMax(mTauMin, mBeta.mX - mAlpha.mX), mTauMax);
    
    
    // ----------------------------------------
    // Buck :
    
    /*float a = getMaxBetaPrevPhases(tmin);
    float b = getMinThetaEvents();
    mAlpha.mX = updatePhaseBound(a, b, mBeta.mX);
    
    a = getMaxThetaEvents();
    b = getMinAlphaNextPhases(tmax);
    mBeta.mX = updatePhaseBound(a, b, mAlpha.mX);
    
    mThetaPredict.mX = Generator::randomUniform(mAlpha.mX, mBeta.mX);*/
}

void Phase::memoAll()
{
    mAlpha.memo();
    mBeta.memo();
    //mTau.memo();
}



// ------------------------------------------------------------------------------------
//  Formule d'inversion avec alpha et beta
// ------------------------------------------------------------------------------------
// TODO : formule spéciale si alpha = beta
float Phase::updatePhaseBound(float a, float b, float bound)
{
    float newBound = bound;
    if(bound != a && bound != b)
    {
        float u = Generator::randomUniform();
        float m = (float) mEvents.size();
        
        if(m >= 2)
        {
            newBound = bound - (bound - a) / powf(u * powf((bound - b) / (bound - a), 1-m) - u + 1, 1/(m-1));
        }
        else if(m == 1)
        {
            // Problem : nan au bout d'un moment ??? Formule symétrique pour alpha & beta ???
            newBound = bound - (bound - a) * expf(-u * logf((bound - a) / (bound - b)));
            //qDebug() << "Bound : " << bound << ", a : " << a << ", b : " << b;
            //qDebug() << newBound;
        }
    }
    return newBound;
}

