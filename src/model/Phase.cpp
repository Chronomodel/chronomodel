#include "Phase.h"
#include "Event.h"
#include "PhaseConstraint.h"
#include "Generator.h"
#include "QtUtilities.h"
#include <QtWidgets>


Phase::Phase():
mId(0),
mName("no Phase Name"),
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
    mAlpha.mSupport = MetropolisVariable::eBounded;
    mAlpha.mFormat = DateUtils::eUnknown;

    mBeta.mSupport = MetropolisVariable::eBounded;
    mBeta.mFormat = DateUtils::eUnknown;

    mDuration.mSupport = MetropolisVariable::eRp;
    mDuration.mFormat = DateUtils::eUnknown;
    // Item initial position :
    mItemX = 0;
    mItemY = 0;
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
    mColor = phase.mColor;
    
    mAlpha = phase.mAlpha;
    mAlpha.mSupport = phase.mAlpha.mSupport;

    mBeta = phase.mBeta;
    mBeta.mSupport = phase.mBeta.mSupport;
    mDuration = phase.mDuration;
    mDuration.mSupport = phase.mDuration.mSupport;

    mTau = phase.mTau;
    
    mTauType = phase.mTauType;
    mTauFixed = phase.mTauFixed;
    mTauMin = phase.mTauMin;
    mTauMax = phase.mTauMax;
    
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
   foreach (Event* ev, mEvents) {
            ev = 0;
   }

   mEvents.clear();

    if(!mConstraintsFwd.isEmpty()) {
        foreach (PhaseConstraint* pc, mConstraintsFwd) {
            if(pc) delete pc;
            pc = 0;
        }
        mConstraintsFwd.clear();
    }
    if(!mConstraintsBwd.isEmpty()) {
        foreach (PhaseConstraint* pc, mConstraintsBwd) {
            if(pc) pc->deleteLater();
            pc = 0;
        }
        mConstraintsBwd.clear();
    }
}

#pragma mark Properties

/**
 * @todo Check the copy of the color if mJson is not set
 */
Phase Phase::fromJson(const QJsonObject& json)
{
    Phase p;
    p.mId = json.value(STATE_ID).toInt();
    p.mName = json.value(STATE_NAME).toString();
    p.mColor = QColor(json.value(STATE_COLOR_RED).toInt(), json.value(STATE_COLOR_GREEN).toInt(), json.value(STATE_COLOR_BLUE).toInt());
    
    p.mItemX = json.value(STATE_ITEM_X).toDouble();
    p.mItemY = json.value(STATE_ITEM_Y).toDouble();
    p.mTauType = (Phase::TauType)json.value(STATE_PHASE_TAU_TYPE).toInt();
    p.mTauFixed = json.value(STATE_PHASE_TAU_FIXED).toDouble();
    p.mTauMin = json.value(STATE_PHASE_TAU_MIN).toDouble();
    p.mTauMax = json.value(STATE_PHASE_TAU_MAX).toDouble();
    p.mIsSelected = json.value(STATE_IS_SELECTED).toBool();
    p.mIsCurrent = json.value(STATE_IS_CURRENT).toBool();
    
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
    double theta = double();
    bool found = false;
    QList<Event*>::const_iterator iterEvent = mEvents.constBegin();
    while(iterEvent != mEvents.constEnd()) {
        if((*iterEvent)->mInitialized)  {
            if(!found) {
                theta = (*iterEvent)->mTheta.mX;
                found = true;
            }
            else {
                theta = qMax(theta, (*iterEvent)->mTheta.mX);
            }
        }
        ++iterEvent;
    }

    return found ? theta : tmax;

/*    double theta = 0;
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
*/
}
/**
 * @brief Phase::getMinThetaEvents
 * @param tmin
 * @return
 * @todo could be faster with use of iterator
 */
double Phase::getMinThetaEvents(double tmin)
{
    double theta = double();
    bool found = false;
    QList<Event*>::const_iterator iterEvent = mEvents.constBegin();
    while(iterEvent != mEvents.constEnd()) {
        if((*iterEvent)->mInitialized)  {
            if(!found) {
                theta = (*iterEvent)->mTheta.mX;
                found = true;
            }
            else {
                theta = qMin(theta, (*iterEvent)->mTheta.mX);
            }
        }
        ++iterEvent;
    }
    return found ? theta : tmin;

}


double Phase::getMinThetaNextPhases(double tmax)
{
    
    double minTheta = tmax;
    for(int i=0; i<mConstraintsFwd.size(); ++i)
    {
        // we can juste look alpha and beta set in member mAlpha and mBeta
        //double theta= mConstraintsFwd[i]->mPhaseTo->getMinThetaEvents(tmax);
        double theta= mConstraintsFwd[i]->mPhaseTo->mAlpha.mX;
        
        if(mConstraintsFwd[i]->mGammaType != PhaseConstraint::eGammaUnknown)
            minTheta = qMin(minTheta, theta - mConstraintsFwd[i]->mGamma);
        else
            minTheta = qMin(minTheta, theta);
    }
    return minTheta;
}

double Phase::getMaxThetaPrevPhases(double tmin)
{
    double maxTheta = tmin;
    for(int i=0; i<mConstraintsBwd.size(); ++i)
    {
        //double theta= mConstraintsBwd[i]->mPhaseFrom->getMaxThetaEvents(tmin);
        double theta= mConstraintsBwd[i]->mPhaseFrom->mBeta.mX;
        
        if(mConstraintsBwd[i]->mGammaType != PhaseConstraint::eGammaUnknown)
            maxTheta = qMax(maxTheta, theta + mConstraintsBwd[i]->mGamma);
        else
            maxTheta = qMax(maxTheta, theta);
    }
    return maxTheta;
}

// --------------------------------------------------------------------------------

void Phase::updateAll(const double tmin, const double tmax)
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


