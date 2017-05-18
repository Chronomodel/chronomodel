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
    mBeta = phase.mBeta;
    mDuration = phase.mDuration;

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
   for (Event* ev: mEvents)
            ev = nullptr;

   mEvents.clear();

    if (!mConstraintsFwd.isEmpty()) {
        for (PhaseConstraint* pc : mConstraintsFwd)
            pc = nullptr;

        mConstraintsFwd.clear();
    }
    if (!mConstraintsBwd.isEmpty()) {
        for (PhaseConstraint* pc : mConstraintsBwd)
            pc = nullptr;

        mConstraintsBwd.clear();
    }
}

//#pragma mark Properties

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

    p.mAlpha.setName("Begin of Phase : "+p.mName);
    p.mBeta.setName("End of Phase : "+p.mName);
    p.mDuration.setName("Duration of Phase : "+p.mName);
    
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

QPair<double,double> Phase::getFormatedTimeRange() const
{
    const double t1 = DateUtils::convertToAppSettingsFormat(mTimeRange.first);
    const double t2 = DateUtils::convertToAppSettingsFormat(mTimeRange.second);

    if (t1<t2)
        return QPair<double,double>(t1,t2);

    else
        return QPair<double,double>(t2,t1);

}

double Phase::getMaxThetaEvents(double tmax)
{
    Q_ASSERT_X(!mEvents.isEmpty(), "Phase::getMaxThetaEvents", QString("No Event in Phase :" + this->mName).toStdString().c_str());
    (void) tmax;
    double theta (mEvents[0]->mTheta.mX);

    // All Event must be Initialized
    std::for_each(mEvents.begin(), mEvents.end(), [&theta] (Event* ev) {theta= std::max(ev->mTheta.mX, theta);});
    return theta;

    // if we need to use this function with event not initalized we have to use the next code
 /*

    double theta;
    bool found = false;
    QList<Event*>::const_iterator iterEvent = mEvents.constBegin();
    while (iterEvent != mEvents.constEnd()) {
        if ((*iterEvent)->mInitialized)  {
            if (!found) {
                theta = (*iterEvent)->mTheta.mX;
                found = true;
            }
            else
                theta = qMax(theta, (*iterEvent)->mTheta.mX);

        }
        ++iterEvent;
    }

    return found ? theta : tmax;
*/
}
/**
 * @brief Phase::getMinThetaEvents
 * @param tmin
 * @return the min of Event inside the phase, used in Phase::updateAll() to set alpha and beta
 */
double Phase::getMinThetaEvents(double tmin)
{
    Q_ASSERT_X(!mEvents.isEmpty(), "Phase::getMinThetaEvents", QString("No Event in Phase :" + this->mName).toStdString().c_str());
    (void) tmin;
    double theta (mEvents[0]->mTheta.mX);

    // All Event must be Initialized
    std::for_each(mEvents.begin(), mEvents.end(), [&theta] (Event* ev){theta= std::min(ev->mTheta.mX, theta);});
    return theta;

    // if we need to use this function with event not initalized we have to use the next code
 /*
    bool found = false;
    QList<Event*>::const_iterator iterEvent = mEvents.constBegin();
    while(iterEvent != mEvents.constEnd()) {
        if ((*iterEvent)->mInitialized)  {
            if (!found) {
                theta = (*iterEvent)->mTheta.mX;
                found = true;
            } else
                theta = qMin(theta, (*iterEvent)->mTheta.mX);

        }
        ++iterEvent;
    }
    return found ? theta : tmin;
*/
}


double Phase::getMinThetaNextPhases(const double tmax)
{
    double minTheta = tmax;
    for (auto &&constFwd : mConstraintsFwd) {
        // we can juste look alpha and beta set in member mAlpha and mBeta
        //double theta= mConstraintsFwd[i]->mPhaseTo->getMinThetaEvents(tmax);
        double theta (constFwd->mPhaseTo->mAlpha.mX);
        
        if (constFwd->mGammaType != PhaseConstraint::eGammaUnknown)
            minTheta = std::min(minTheta, theta - constFwd->mGamma);
        else
            minTheta = std::min(minTheta, theta);
    }
    return minTheta;
}

double Phase::getMaxThetaPrevPhases(const double tmin)
{
    double maxTheta (tmin);

    for (auto &&constBwd : mConstraintsBwd) {
        const double theta (constBwd->mPhaseFrom->mBeta.mX);
        
        if (constBwd->mGammaType != PhaseConstraint::eGammaUnknown)
            maxTheta = std::max(maxTheta, theta + constBwd->mGamma);
        else
            maxTheta = std::max(maxTheta, theta);
    }
    return maxTheta;
}

// --------------------------------------------------------------------------------

void Phase::updateAll(const double tmin, const double tmax)
{
    static bool initalized = false; // What is it??
    
    mAlpha.mX = getMinThetaEvents(tmin);
    mBeta.mX = getMaxThetaEvents(tmax);
    mDuration.mX = mBeta.mX - mAlpha.mX;
    
   /*   if (initalized) {
            double oldAlpha = mAlpha.mX;
            double oldBeta = mBeta.mX;
            if (mAlpha.mX != oldAlpha)
                mIsAlphaFixed = false;

            if (mBeta.mX != oldBeta)
                mIsAlphaFixed = false;
        }
    */
    
    updateTau();
    
    initalized = true;
}

QString Phase::getTauTypeText() const
{
    switch (mTauType) {
        case eTauFixed:
                return QObject::tr("Max duration ≤ ") + QString::number(mTauFixed);
            break;
        case eTauUnknown:
                return QObject::tr("Max duration unknown");
            break;

        case eTauRange: // no longer used
            return QObject::tr("Tau Range")+ " [ " + QString::number(mTauMin) + " ; " + QString::number(mTauMax) + " ]";
        break;
        default:
                return QObject::tr("Tau Undefined->Error");
            break;
    }

}

void Phase::initTau()
{
    if (mTauType == eTauFixed && mTauFixed != 0.)
        mTau = mTauFixed;

    else if (mTauType == eTauRange && mTauMax > mTauMin) // no longer used
        mTau = mTauMax;

    else if (mTauType == eTauUnknown) {
        // nothing to do
    }
}

void Phase::updateTau()
{
    if (mTauType == eTauFixed && mTauFixed != 0.)
        mTau = mTauFixed;

    else if (mTauType == eTauRange && mTauMax > mTauMin)
        mTau = Generator::randomUniform(qMax(mTauMin, mBeta.mX - mAlpha.mX), mTauMax);

    else if (mTauType == eTauUnknown) {
        // Nothing to do!
    }
}

void Phase::memoAll()
{
    mAlpha.memo();
    mBeta.memo();
    mDuration.memo();
#ifdef DEBUG
    if (mBeta.mX - mAlpha.mX < 0.)
        qDebug()<<"in Phase::memoAll : "<<mName<<" Warning mBeta.mX - mAlpha.mX<0";
#endif
}

void Phase::generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    mAlpha.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
    mBeta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
    mDuration.generateHistos(chains, fftLen, bandwidth);
}

