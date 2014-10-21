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
    
    QJsonArray dates = json[STATE_EVENT_DATES].toArray();
    for(int j=0; j<dates.size(); ++j)
    {
        QJsonObject date = dates[j].toObject();
        Date d = Date::fromJson(date);
        event.mDates.append(d);
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

#pragma mark Utilities
float Event::getMaxAlphaPhases(float defaultValue)
{
    float max = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        float alpha = mPhases[i]->mAlpha.mX;
        max = (alpha > max) ? alpha : max;
    }
    return max;
}

float Event::getMinBetaPhases(float defaultValue)
{
    float min = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        float beta = mPhases[i]->mBeta.mX;
        min = (beta < min) ? beta : min;
    }
    return min;
}

float Event::getMaxEventThetaBackward(float defaultValue)
{
    float thetaMax = defaultValue;
    
    // ------------------------------------------------------------------------------------
    //  Max des thetas des faits en contrainte directe antérieure
    // ------------------------------------------------------------------------------------
    for(int i=0; i<mConstraintsBwd.size(); ++i)
    {
        float thetaf = mConstraintsBwd[i]->mEventFrom->mTheta.mX;
        
        // Check if event constraint is respected :
        switch(mConstraintsBwd[i]->mPhiType)
        {
            // Pas utilisé pour l'instant
            /*case EventConstraint::ePhiFixed:
            {
                thetaf = mTheta.mX - (*it)->mPhiFixed;
                break;
            }*/
            case EventConstraint::ePhiRange:
            {
                thetaf = thetaf + mConstraintsBwd[i]->mPhiMin;
                break;
            }
            case EventConstraint::ePhiUnknown:
            default:
                break;
        }
        thetaMax = std::max(thetaMax, thetaf);
    }
    // TODO : respecter phi max avec les contraintes forward
    
    // ------------------------------------------------------------------------------------
    //  Itération sur les phases ??
    // ------------------------------------------------------------------------------------
    for(int i=0; i<mPhases.size(); ++i)
    {
        // ------------------------------------------------------------------------------------
        //  Itération sur les contraintes de phases antérieures
        // ------------------------------------------------------------------------------------
        for(int j=0; j<mPhases[i]->mConstraintsBwd.size(); ++j)
        {
            PhaseConstraint* constraint = mPhases[i]->mConstraintsBwd[j];
            // ------------------------------------------------------------------------------------
            //  On teste uniquement le plus grand theta f de la phase
            // ------------------------------------------------------------------------------------
            float thetaf = constraint->mPhaseFrom->getMaxThetaEvents();
            
            // Check if phase constraint is respected :
            switch(constraint->mGammaType)
            {
                case PhaseConstraint::eGammaFixed:
                {
                    thetaf = std::min(mTheta.mX - constraint->mGammaFixed, thetaf);
                    break;
                }
                case PhaseConstraint::eGammaRange:
                {
                    thetaf = std::min(mTheta.mX - constraint->mGammaMin, thetaf);
                    break;
                }
                case PhaseConstraint::eGammaUnknown:
                default:
                    break;
            }
            thetaMax = std::max(thetaMax, thetaf);
        }
    }
    return thetaMax;
}

float Event::getMinEventThetaForward(float defaultValue)
{
    float thetaMin = defaultValue;
    
    // ------------------------------------------------------------------------------------
    //  Min des thetas des faits en contrainte directe et qui nous suivent
    // ------------------------------------------------------------------------------------
    for(int i=0; i<mConstraintsFwd.size(); ++i)
    {
        EventConstraint* constraint = mConstraintsFwd[i];
        
        float thetaf = constraint->mEventTo->mTheta.mX;
        
        // Check if event constraint is respected :
        switch(constraint->mPhiType)
        {
            case EventConstraint::ePhiFixed:
            {
                thetaf = std::max(mTheta.mX + constraint->mPhiFixed, thetaf);
                break;
            }
            case EventConstraint::ePhiRange:
            {
                thetaf = std::max(mTheta.mX + constraint->mPhiMin, thetaf);
                break;
            }
            case EventConstraint::ePhiUnknown:
            default:
                break;
        }
        thetaMin = std::min(thetaMin, thetaf);
    }
    // ------------------------------------------------------------------------------------
    //  Itération sur les phases
    // ------------------------------------------------------------------------------------
    for(int i=0; i<mPhases.size(); ++i)
    {
        // ------------------------------------------------------------------------------------
        //  Itération sur les contraintes de phases postérieures
        // ------------------------------------------------------------------------------------
        for(int j=0; j<mPhases[i]->mConstraintsFwd.size(); ++j)
        {
            PhaseConstraint* constraint = mPhases[i]->mConstraintsFwd[j];
            // ------------------------------------------------------------------------------------
            //  On teste uniquement le plus petit theta f de la phase
            // ------------------------------------------------------------------------------------
            float thetaf = constraint->mPhaseTo->getMinThetaEvents();
            
            // Check if phase constraint is respected :
            switch(constraint->mGammaType)
            {
                case PhaseConstraint::eGammaFixed:
                {
                    thetaf = std::max(mTheta.mX + constraint->mGammaFixed, thetaf);
                    break;
                }
                case PhaseConstraint::eGammaRange:
                {
                    thetaf = std::max(mTheta.mX + constraint->mGammaMin, thetaf);
                    break;
                }
                case PhaseConstraint::eGammaUnknown:
                default:
                    break;
            }
            thetaMin = std::min(thetaMin, thetaf);
        }
    }
    return thetaMin;
}


#pragma mark MCMC
void Event::reset()
{
    mTheta.reset();
}

void Event::updateTheta(float tmin, float tmax)
{
    // -------------------------------------------------------------------------------------------------
    //  1 - Classic
    //  Méthode de rejet avec echantillonneur double_exp.
    //  - echantillonneur alternatif 1 : box_muller, couteux si on est en queue de distrib.
    //  - echantillonneur alternatif 2 : MH avec marcheur aléatoire gaussien adaptatif.
    //  Si les résultats sont "mauvais", on peut utiliser un autre échantillonneur.
    //  (Les utilisateurs seront formés à comprendre les résultats de calculs MCMC)
    //  => Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta (=phi) > 0.
    //
    //  TODO !!!! : Si le fait est en contrainte de type Fixed :
    //  - S'il est en début de contrainte, on l'évalue normalement
    //  en utilisant en plus les dates de l'autre fait avec des delta i = le phi fixed de la contrainte.
    //  - S'il est en fin de contrainte, on ne l'évalue pas : on prend juste le theta du fait en début de
    //  contrainte et on ajoute le phi fixed.
    // -------------------------------------------------------------------------------------------------
    
    float min = getMaxEventThetaBackward(tmin);
    float max = getMinEventThetaForward(tmax);
    
    float minPhases = getMaxAlphaPhases(tmin);
    float maxPhases = getMinBetaPhases(tmax);
    
    min = (minPhases > min) ? minPhases : min;
    max = (maxPhases < max) ? maxPhases : max;
    
    // ------------
    
    float sum_p = 0;
    float sum_t = 0;
    for(int i=0; i<mDates.size(); ++i)
    {
        sum_t += (mDates[i].mTheta.mX - mDates[i].mDelta.mX) / pow(mDates[i].mSigma.mX, 2);
        sum_p += 1 / pow(mDates[i].mSigma.mX, 2);
    }
    float theta_avg = sum_t / sum_p;
    float sigma = 1/sqrt(sum_p);
    
    switch(mMethod)
    {
        case eDoubleExp:
        {
            // Gibbs
            float theta = Generator::gaussByDoubleExp(theta_avg, sigma, min, max);
            mTheta.tryUpdate(theta, 1);
            break;
        }
        case eBoxMuller:
        {
            // Gibbs
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


