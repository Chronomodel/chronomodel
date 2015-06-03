#include "Phase.h"
#include "Event.h"
#include "PhaseConstraint.h"
#include "Generator.h"
#include "QtUtilities.h"
#include <QtWidgets>


Phase::Phase():
mId(0),
mTau(0.),
mIsAlphaFixed(true),
mIsBetaFixed(true),
mTauType(Phase::eTauUnknown),
mTauFixed(0),
mTauMin(0),
mTauMax(0),
mIsSelected(false),
mIsCurrent(false),
mLevel(0)
{
    mColor = randomColor();
    mAlpha.mIsDate = true;
    mBeta.mIsDate = true;
    mDuration.mIsDate = false;
    // Item initial position :
    //int posDelta = 100;
    mItemX = 0;//rand() % posDelta - posDelta/2;
    mItemY = 0;//rand() % posDelta - posDelta/2;
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

// --------------------------------------------------------------------------------

double Phase::getMaxThetaEvents(double tmax)
{
    double theta = 0;
    bool found = false;
    for(int i=0; i<mEvents.size(); ++i)
    {
        if(mEvents[i]->mInitialized)
        {
            if(!found)
            {
                theta = mEvents[i]->mTheta.mX;
                found = true;
            }
            else
            {
                theta = std::max(theta, mEvents[i]->mTheta.mX);
            }
        }
    }
    return found ? theta : tmax;
}

double Phase::getMinThetaEvents(double tmin)
{
    double theta = 0;
    bool found = false;
    for(int i=0; i<mEvents.size(); ++i)
    {
        if(mEvents[i]->mInitialized)
        {
            if(!found)
            {
                theta = mEvents[i]->mTheta.mX;
                found = true;
            }
            else
            {
                theta = std::min(theta, mEvents[i]->mTheta.mX);
            }
        }
    }
    return found ? theta : tmin;
}

// On pourra regarder juste les alpha et beta qui sont déjà mémorisés
double Phase::getMinThetaNextPhases(double tmax)
{
    //qDebug() << "=> Phase étudiée : " << mName << " : getMinThetaNextPhases";
    
    double minTheta = tmax;
    for(int i=0; i<mConstraintsFwd.size(); ++i)
    {
        //Phase* phaseTo = mConstraintsFwd[i]->mPhaseTo;
        
        //qDebug() << "==> Phase après : " << phaseTo->mName << " (" << phaseTo->mEvents.size() << " events)";
        

        // PhD : On pourra regarder juste les alpha et beta qui sont déjà mémorisés 
        /* double theta = tmax;
        for(int j=0; j<phaseTo->mEvents.size(); ++j)
        {
            if(phaseTo->mEvents[j]->mInitialized)
            {
                theta = std::min(theta, phaseTo->mEvents[j]->mTheta.mX);
            }
        } */
        //qDebug() << "===> Min Theta : " << theta;
        
        double theta= mConstraintsFwd[i]->mPhaseTo->getMinThetaEvents(tmax);
        
        if(mConstraintsFwd[i]->mGammaType != PhaseConstraint::eGammaUnknown)
            minTheta = std::min(minTheta, theta - mConstraintsFwd[i]->mGamma);
        else
            minTheta = std::min(minTheta, theta);
    }
    return minTheta;
}

double Phase::getMaxThetaPrevPhases(double tmin)
{
    double maxTheta = tmin;
    for(int i=0; i<mConstraintsBwd.size(); ++i)
    {
        //Phase* phaseFrom = mConstraintsBwd[i]->mPhaseFrom;

        
        /* double theta = tmin;
        for(int j=0; j<phaseFrom->mEvents.size(); ++j)
        {
            if(phaseFrom->mEvents[j]->mInitialized)
            {
                theta = std::max(theta, phaseFrom->mEvents[j]->mTheta.mX);
            }
        } */
        double theta= mConstraintsBwd[i]->mPhaseFrom->getMaxThetaEvents(tmin);
        
        if(mConstraintsBwd[i]->mGammaType != PhaseConstraint::eGammaUnknown)
            maxTheta = std::max(maxTheta, theta + mConstraintsBwd[i]->mGamma);
        else
            maxTheta = std::max(maxTheta, theta);
    }
    return maxTheta;
}

// --------------------------------------------------------------------------------

void Phase::updateAll(double tmin, double tmax)
{
    static bool initalized = false;
    
    double oldAlpha = mAlpha.mX;
    double oldBeta = mBeta.mX;
    
    mAlpha.mX = getMinThetaEvents(tmin);
    mBeta.mX = getMaxThetaEvents(tmax);
    mDuration.mX = mBeta.mX - mAlpha.mX;
    
    if(initalized)
    {
        if(mAlpha.mX != oldAlpha)
            mIsAlphaFixed = false;
        if(mBeta.mX != oldBeta)
            mIsAlphaFixed = false;
    }
    
    updateTau();
    
    initalized = true;
}

void Phase::initTau()
{
    if(mTauType == eTauFixed && mTauFixed != 0)
        mTau = mTauFixed;
    else if(mTauType == eTauRange && mTauMax > mTauMin)
        mTau = mTauMax;
    else if(mTauType == eTauUnknown)
    {
        // nothing to do
    }
}

void Phase::updateTau()
{
    if(mTauType == eTauFixed && mTauFixed != 0)
        mTau = mTauFixed;
    else if(mTauType == eTauRange && mTauMax > mTauMin)
        mTau = Generator::randomUniform(qMax(mTauMin, mBeta.mX - mAlpha.mX), mTauMax);
    else if(mTauType == eTauUnknown)
    {
        // Nothing to do!
    }
}

void Phase::memoAll()
{
    mAlpha.memo();
    mBeta.memo();
    mDuration.memo();
}


