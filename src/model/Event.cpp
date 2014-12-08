#include "Event.h"
#include "Phase.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "EventKnown.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include <QString>
#include <QJsonArray>
#include <QObject>
#include <QDebug>


Event::Event():
mType(eDefault),
mId(0),
mMethod(Event::eDoubleExp),
mIsCurrent(false),
mIsSelected(false),
mInitialized(false)
{
    mColor = QColor(120 + rand() % 50,
                    120 + rand() % 50,
                    120 + rand() % 50);
    
    // Item initial position :
    int posDelta = 100;
    mItemX = rand() % posDelta - posDelta/2;
    mItemY = rand() % posDelta - posDelta/2;
}

Event::Event(const Event& event)
{
    copyFrom(event);
}

Event& Event::operator=(const Event& event)
{
    copyFrom(event);
    return *this;
}

void Event::copyFrom(const Event& event)
{
    mType = event.mType;
    mId = event.mId;
    mName = event.mName;
    mMethod = event.mMethod;
    mColor = event.mColor;
    
    mDates = event.mDates;
    mPhases = event.mPhases;
    mConstraintsFwd = event.mConstraintsFwd;
    mConstraintsBwd = event.mConstraintsBwd;
    
    mTheta = event.mTheta;
    mS02 = event.mS02;
    mAShrinkage = event.mAShrinkage;
    
    mItemX = event.mItemX;
    mItemY = event.mItemY;
    
    mIsCurrent = event.mIsCurrent;
    mIsSelected = event.mIsSelected;
    
    mDates = event.mDates;
    
    mPhasesIds = event.mPhasesIds;
    mConstraintsFwdIds = event.mConstraintsFwdIds;
    mConstraintsBwdIds = event.mConstraintsBwdIds;
    
    mPhases = event.mPhases;
    mConstraintsFwd = event.mConstraintsFwd;
    mConstraintsBwd = event.mConstraintsBwd;
}

Event::~Event()
{

}

#pragma mark JSON
Event Event::fromJson(const QJsonObject& json)
{
    Event event;
    
    event.mType = (Type)json[STATE_EVENT_TYPE].toInt();
    event.mId = json[STATE_ID].toInt();
    event.mName = json[STATE_NAME].toString();
    event.mColor = QColor(json[STATE_COLOR_RED].toInt(),
                          json[STATE_COLOR_GREEN].toInt(),
                          json[STATE_COLOR_BLUE].toInt());
    event.mMethod = (Method)json[STATE_EVENT_METHOD].toInt();
    event.mItemX = json[STATE_ITEM_X].toDouble();
    event.mItemY = json[STATE_ITEM_Y].toDouble();
    event.mIsSelected = json[STATE_IS_SELECTED].toBool();
    event.mIsCurrent = json[STATE_IS_CURRENT].toBool();
    
    event.mTheta.mProposal = ModelUtilities::getEventMethodText(event.mMethod);
    
    event.mPhasesIds = stringListToIntList(json[STATE_EVENT_PHASE_IDS].toString());
    
    QJsonArray dates = json[STATE_EVENT_DATES].toArray();
    for(int j=0; j<dates.size(); ++j)
    {
        QJsonObject date = dates[j].toObject();
        Date d = Date::fromJson(date);
        if(!d.isNull())
        {
            event.mDates.append(d);
        }
        else
        {
            qDebug() << "ERROR : date could not be created for plugin " << date[STATE_DATE_PLUGIN_ID].toString();
        }
    }
    return event;
}

QJsonObject Event::toJson() const
{
    QJsonObject event;
    
    event[STATE_EVENT_TYPE] = mType;
    event[STATE_ID] = mId;
    event[STATE_NAME] = mName;
    event[STATE_COLOR_RED] = mColor.red();
    event[STATE_COLOR_GREEN] = mColor.green();
    event[STATE_COLOR_BLUE] = mColor.blue();
    event[STATE_EVENT_METHOD] = mMethod;
    event[STATE_ITEM_X] = mItemX;
    event[STATE_ITEM_Y] = mItemY;
    event[STATE_IS_SELECTED] = mIsSelected;
    event[STATE_IS_CURRENT] = mIsCurrent;
    
    QString eventIdsStr;
    if(mPhasesIds.size() > 0)
    {
        QStringList eventIds;
        for(int i=0; i<mPhasesIds.size(); ++i)
            eventIds.append(QString::number(mPhasesIds[i]));
        eventIdsStr = eventIds.join(",");
    }
    event[STATE_EVENT_PHASE_IDS] = eventIdsStr;
    
    QJsonArray dates;
    for(int i=0; i<mDates.size(); ++i)
    {
        QJsonObject date = mDates[i].toJson();
        dates.append(date);
    }
    event[STATE_EVENT_DATES] = dates;
    
    return event;
}

#pragma mark Properties
Event::Type Event::type() const
{
    return mType;
}

#pragma mark MCMC
void Event::reset()
{
    mTheta.reset();
    mInitialized = false;
}

float Event::getThetaMin(float defaultValue)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne min courante pour le tirage de theta
    // ------------------------------------------------------------------
    
    float min1 = defaultValue;
    
    // Max des thetas des faits en contrainte directe antérieure
    float min2 = defaultValue;
    for(int i=0; i<mConstraintsBwd.size(); ++i)
    {
        if(mConstraintsBwd[i]->mEventFrom->mInitialized)
        {
            float thetaf = mConstraintsBwd[i]->mEventFrom->mTheta.mX;
            min2 = qMax(min2, thetaf);
        }
    }
    
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    float min3 = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases[i]->mTauType != Phase::eTauUnknown)
        {
            float thetaMax = defaultValue;
            for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
            {
                Event* event = mPhases[i]->mEvents[j];
                if(event != this && event->mInitialized)
                {
                    thetaMax = qMax(event->mTheta.mX, thetaMax);
                }
            }
            min3 = qMax(min3, thetaMax - mPhases[i]->mTau);
        }
    }
    
    // Contraintes des phases précédentes
    float min4 = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        float thetaMax = defaultValue;
        for(int j=0; j<mPhases[i]->mConstraintsBwd.size(); ++j)
        {
            PhaseConstraint* constraint = mPhases[i]->mConstraintsBwd[j];
            Phase* phaseFrom = constraint->mPhaseFrom;
            
            if(constraint->mGammaType != PhaseConstraint::eGammaUnknown)
                thetaMax = qMax(phaseFrom->mBeta.mX + constraint->mGamma, thetaMax);
            else
                thetaMax = qMax(phaseFrom->mBeta.mX, thetaMax);
        }
        min4 = qMax(min4, thetaMax);
    }
    
    float min_tmp1 = qMax(min1, min2);
    float min_tmp2 = qMax(min3, min4);
    float min = qMax(min_tmp1, min_tmp2);
    
    return min;
}

float Event::getThetaMax(float defaultValue)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne max
    // ------------------------------------------------------------------
    
    float max1 = defaultValue;
    
    // Min des thetas des faits en contrainte directe et qui nous suivent
    float max2 = defaultValue;
    for(int i=0; i<mConstraintsFwd.size(); ++i)
    {
        if(mConstraintsFwd[i]->mEventTo->mInitialized)
        {
            float thetaf = mConstraintsFwd[i]->mEventTo->mTheta.mX;
            max2 = qMin(max2, thetaf);
        }
    }
    
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    float max3 = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases[i]->mTauType != Phase::eTauUnknown)
        {
            float thetaMin = defaultValue;
            for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
            {
                Event* event = mPhases[i]->mEvents[j];
                if(event != this && event->mInitialized)
                {
                    thetaMin = qMin(event->mTheta.mX, thetaMin);
                }
            }
            max3 = qMin(max3, thetaMin + mPhases[i]->mTau);
        }
    }
    
    // Contraintes des phases suivantes
    float max4 = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        float thetaMin = defaultValue;
        for(int j=0; j<mPhases[i]->mConstraintsFwd.size(); ++j)
        {
            PhaseConstraint* constraint = mPhases[i]->mConstraintsFwd[j];
            Phase* phaseTo = constraint->mPhaseTo;
            
            if(constraint->mGammaType != PhaseConstraint::eGammaUnknown)
                thetaMin = qMin(phaseTo->mAlpha.mX - constraint->mGamma, thetaMin);
            else
                thetaMin = qMin(phaseTo->mAlpha.mX, thetaMin);
        }
        max4 = qMin(max4, thetaMin);
    }
    
    float max_tmp1 = qMin(max1, max2);
    float max_tmp2 = qMin(max3, max4);
    float max = qMin(max_tmp1, max_tmp2);
    
    return max;
}

void Event::updateTheta(float tmin, float tmax)
{
    float min = getThetaMin(tmin);
    float max = getThetaMax(tmax);
    
    //qDebug() << "[" << min << ", " << max << "]";
    
    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------
    
    float sum_p = 0.f;
    float sum_t = 0.f;
    for(int i=0; i<mDates.size(); ++i)
    {
        float variance = (mDates[i].mSigma.mX * mDates[i].mSigma.mX);
        sum_t += (mDates[i].mTheta.mX + mDates[i].mDelta) / variance;
        sum_p += 1.f / variance;
    }
    float theta_avg = sum_t / sum_p;
    float sigma = 1.f / sqrtf(sum_p);
    
    switch(mMethod)
    {
        case eDoubleExp:
        {
            float theta = Generator::gaussByDoubleExp(theta_avg, sigma, min, max);
            mTheta.tryUpdate(theta, 1);
            break;
        }
        case eBoxMuller:
        {
            float theta;
            int counter = 0;
            do{
                theta = Generator::gaussByBoxMuller(theta_avg, sigma);
                ++counter;
            }while(theta < min || theta > max);
            qDebug() << "Event update num trials : " << counter;
            mTheta.tryUpdate(theta, 1);
            break;
        }
        case eMHAdaptGauss:
        {
            // MH : Seul cas où le taux d'acceptation a du sens car on utilise sigma MH :
            float theta = Generator::gaussByBoxMuller(mTheta.mX, mTheta.mSigmaMH);
            
            float rapport = 0;
            if(theta >= min && theta <= max)
            {
                rapport = expf((-0.5/(sigma*sigma)) * (powf(theta - theta_avg, 2) - powf(mTheta.mX - theta_avg, 2)));
            }
            mTheta.tryUpdate(theta, rapport);
            break;
        }
        default:
            break;
    }
}


