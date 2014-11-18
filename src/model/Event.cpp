#include "Event.h"
#include "Phase.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "EventKnown.h"
#include <QString>
#include <QJsonArray>
#include <QObject>
#include <QDebug>


Event::Event():
mType(eDefault),
mId(0),
mMethod(Event::eDoubleExp),
mIsCurrent(false),
mIsSelected(false)
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
    event.mId = json[STATE_EVENT_ID].toInt();
    event.mName = json[STATE_EVENT_NAME].toString();
    event.mColor = QColor(json[STATE_EVENT_RED].toInt(),
                          json[STATE_EVENT_GREEN].toInt(),
                          json[STATE_EVENT_BLUE].toInt());
    event.mMethod = (Method)json[STATE_EVENT_METHOD].toInt();
    event.mItemX = json[STATE_EVENT_ITEM_X].toDouble();
    event.mItemY = json[STATE_EVENT_ITEM_Y].toDouble();
    event.mIsSelected = json[STATE_EVENT_IS_SELECTED].toBool();
    event.mIsCurrent = json[STATE_EVENT_IS_CURRENT].toBool();
    
    QString eventIdsStr = json[STATE_EVENT_PHASE_IDS].toString();
    if(!eventIdsStr.isEmpty())
    {
        QStringList eventIds = eventIdsStr.split(",");
        for(int i=0; i<eventIds.size(); ++i)
            event.mPhasesIds.append(eventIds[i].toInt());
    }
    
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
    event[STATE_EVENT_ID] = mId;
    event[STATE_EVENT_NAME] = mName;
    event[STATE_EVENT_RED] = mColor.red();
    event[STATE_EVENT_GREEN] = mColor.green();
    event[STATE_EVENT_BLUE] = mColor.blue();
    event[STATE_EVENT_METHOD] = mMethod;
    event[STATE_EVENT_ITEM_X] = mItemX;
    event[STATE_EVENT_ITEM_Y] = mItemY;
    event[STATE_EVENT_IS_SELECTED] = mIsSelected;
    event[STATE_EVENT_IS_CURRENT] = mIsCurrent;
    
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
        float thetaf = mConstraintsBwd[i]->mEventFrom->mTheta.mX;
        min2 = qMax(min2, thetaf);
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
            float thetaMax = 0;
            for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
            {
                Event* event = mPhases[i]->mEvents[j];
                if(event != this)
                {
                    thetaMax = qMax(event->mTheta.mX, thetaMax);
                }
            }
            min3 = qMax(min3, thetaMax - mPhases[i]->mTau.mX);
        }
    }
    
    // Contraintes des phases précédentes
    float min4 = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        float thetaMax = 0;
        for(int j=0; j<mPhases[i]->mConstraintsBwd.size(); ++j)
        {
            PhaseConstraint* constraint = mPhases[i]->mConstraintsBwd[j];
            Phase* phaseFrom = constraint->mPhaseFrom;
            
            if(constraint->mGammaType != PhaseConstraint::eGammaUnknown)
                thetaMax = qMax(phaseFrom->mBeta.mX, thetaMax + constraint->mGamma.mX);
            else
                thetaMax = qMax(phaseFrom->mBeta.mX, thetaMax);
        }
        min4 = qMax(min4, thetaMax);
    }
    
    float min_tmp1 = qMax(min1, min2);
    float min_tmp2 = qMax(min3, min3);
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
        float thetaf = mConstraintsFwd[i]->mEventTo->mTheta.mX;
        max2 = qMin(max2, thetaf);
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
            float thetaMin = 0;
            for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
            {
                Event* event = mPhases[i]->mEvents[j];
                if(event != this)
                {
                    thetaMin = qMin(event->mTheta.mX, thetaMin);
                }
            }
            max3 = qMin(max3, thetaMin + mPhases[i]->mTau.mX);
        }
    }
    
    // Contraintes des phases suivantes
    float max4 = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        float thetaMin = 0;
        for(int j=0; j<mPhases[i]->mConstraintsFwd.size(); ++j)
        {
            PhaseConstraint* constraint = mPhases[i]->mConstraintsFwd[j];
            Phase* phaseTo = constraint->mPhaseTo;
            
            if(constraint->mGammaType != PhaseConstraint::eGammaUnknown)
                thetaMin = qMax(phaseTo->mAlpha.mX, thetaMin - constraint->mGamma.mX);
            else
                thetaMin = qMax(phaseTo->mAlpha.mX, thetaMin);
        }
        max4 = qMin(max4, thetaMin);
    }
    
    float max_tmp1 = qMax(max1, max2);
    float max_tmp2 = qMax(max3, max3);
    float max = qMax(max_tmp1, max_tmp2);
    
    return max;
}

void Event::updateTheta(float tmin, float tmax)
{
    float min = getThetaMin(tmin);
    float max = getThetaMax(tmax);
    
    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------
    
    float sum_p = 0;
    float sum_t = 0;
    for(int i=0; i<mDates.size(); ++i)
    {
        sum_t += (mDates[i].mTheta.mX + mDates[i].mDelta) / pow(mDates[i].mSigma.mX, 2);
        sum_p += 1 / pow(mDates[i].mSigma.mX, 2);
    }
    float theta_avg = sum_t / sum_p;
    float sigma = 1/sqrt(sum_p);
    
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
            do{
                theta = Generator::gaussByBoxMuller(theta_avg, sigma);
            }while(theta < min || theta > max);
            
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
                rapport = exp((-0.5/(sigma*sigma)) * (pow(theta - theta_avg, 2) - pow(mTheta.mX - theta_avg, 2)));
            }
            mTheta.tryUpdate(theta, rapport);
            break;
        }
        default:
            break;
    }
}


