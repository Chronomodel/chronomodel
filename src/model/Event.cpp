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
#include <QJsonObject>

Event::Event():
mType(eDefault),
mId(0),
mName("no Event Name"),
mMethod(Event::eDoubleExp),
mIsCurrent(false),
mIsSelected(false),
mInitialized(false),
mLevel(0)
{
    //mTheta.mIsDate = true;
    mTheta.mSupport = MetropolisVariable::eBounded;
    mTheta.mFormat = DateUtils::eUnknown;;

    // Item initial position :
    //int posDelta = 100;
    mItemX = 0;//rand() % posDelta - posDelta/2;
    mItemY = 0;//rand() % posDelta - posDelta/2;
    
    // Note : setting an event in (0, 0) tells the scene that this item is new!
    // Thus the scene will move it randomly around the currently viewed center point.
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
/**
 * @todo Check the copy of the color if mJson is not set
 */
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
    mTheta.mSupport = event.mTheta.mSupport;

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
    
    mMixingLevel = event.mMixingLevel;
}

Event::~Event()
{
    mDates.clear();

    if(!mPhases.isEmpty()) {
        foreach (Phase* ph, mPhases) {
            ph = 0;
        }
        mPhases.clear();
     }

    if(!mConstraintsFwd.isEmpty()) {
        foreach (EventConstraint* ec, mConstraintsFwd) {
            if(ec) delete ec;
            ec = 0;
        }
        mConstraintsFwd.clear();
     }

    if(!mConstraintsBwd.isEmpty()) {
        foreach (EventConstraint* ec, mConstraintsBwd) {
            if(ec) ec->deleteLater();
            ec = 0;
        }
        mConstraintsBwd.clear();
     }
}


#pragma mark JSON
Event Event::fromJson(const QJsonObject& json)
{
    Event event;
    event.mType = (Type)json.value(STATE_EVENT_TYPE).toInt();
    event.mId = json.value(STATE_ID).toInt();
    event.mName = json.value(STATE_NAME).toString();
    event.mColor = QColor(json.value(STATE_COLOR_RED).toInt(),
                          json.value(STATE_COLOR_GREEN).toInt(),
                          json.value(STATE_COLOR_BLUE).toInt());
    event.mMethod = (Method)(json.value(STATE_EVENT_METHOD).toInt());
    event.mItemX = json.value(STATE_ITEM_X).toDouble();
    event.mItemY = json.value(STATE_ITEM_Y).toDouble();
    event.mIsSelected = json.value(STATE_IS_SELECTED).toBool();
    event.mIsCurrent = json.value(STATE_IS_CURRENT).toBool();
    
    event.mTheta.mProposal = ModelUtilities::getEventMethodText(event.mMethod);
    event.mTheta.setName("Theta of Event : "+event.mName);
    
    event.mPhasesIds = stringListToIntList(json.value(STATE_EVENT_PHASE_IDS).toString());
    
    
    const QJsonArray dates = json.value(STATE_EVENT_DATES).toArray();
    for(int j=0; j<dates.size(); ++j)
    {
        QJsonObject jdate = dates.at(j).toObject();
        
        Date d = Date::fromJson(jdate);
        d.autoSetTiSampler(true);
        d.mMixingLevel=event.mMixingLevel;
        
        if(!d.isNull())
        {
            event.mDates.append(d);
        }
        else
        {
            throw QObject::tr("ERROR : data could not be created with plugin ") + jdate[STATE_DATE_PLUGIN_ID].toString();
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
            eventIds.append(QString::number(mPhasesIds.at(i)));
        eventIdsStr = eventIds.join(",");
    }
    event[STATE_EVENT_PHASE_IDS] = eventIdsStr;
    
    QJsonArray dates;
    for(int i=0; i<mDates.size(); ++i)
    {
        QJsonObject date = mDates.at(i).toJson();
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

double Event::getThetaMinRecursive(const double defaultValue,
                                   const QVector<QVector<Event*> >& eventBranches,
                                   const QVector<QVector<Phase*> >& phaseBranches)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne min courante pour le tirage de theta
    // ------------------------------------------------------------------
    double min1 = defaultValue;
    
    // Max des thetas des faits en contrainte directe antérieure
    double min2 = defaultValue;
    for(int i=0; i<eventBranches.size(); ++i)
    {
        const QVector<Event*>& branch = eventBranches.at(i);
        double branchMin = defaultValue;
        for(int j=0; j<branch.size(); ++j)
        {
            const Event* event = branch.at(j);
            if(event == this)
            {
                min2 = qMax(min2, branchMin);
                break;
            }
            if(event->mInitialized)
                branchMin = qMax(branchMin, event->mTheta.mX);
        }
    }
    
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    double min3 = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases.at(i)->mTauType != Phase::eTauUnknown)
        {
            double thetaMax = defaultValue;
            for(int j=0; j<mPhases.at(i)->mEvents.size(); ++j)
            {
                const Event* event = mPhases.at(i)->mEvents.at(j);
                if(event != this && event->mInitialized)
                {
                    thetaMax = qMax(event->mTheta.mX, thetaMax);
                }
            }
            min3 = qMax(min3, thetaMax - mPhases.at(i)->mTau);
        }
    }
    
    // Contraintes des phases précédentes
    double min4 = defaultValue;
    for(int i=0; i<phaseBranches.size(); ++i)
    {
        const QVector<Phase*>& branch = phaseBranches.at(i);
        double branchMin = defaultValue;
        for(int j=0; j<branch.size(); ++j)
        {
            const Phase* phase = branch.at(j);
            if(phase->mEvents.contains(this))
            {
                min4 = qMax(min4, branchMin);
                break;
            }
            double theta = defaultValue;
            for(int k=0; k<phase->mEvents.size(); ++k)
            {
                if(phase->mEvents.at(k)->mInitialized)
                {
                    theta = std::max(theta, phase->mEvents.at(k)->mTheta.mX);
                }
            }
            const PhaseConstraint* c = 0;
            for(int k=0; k<phase->mConstraintsFwd.size(); ++k)
            {
                if(branch.contains(phase->mConstraintsFwd.at(k)->mPhaseTo))
                {
                    c = phase->mConstraintsFwd.at(k);
                    break;
                }
            }
            if(c && c->mGammaType != PhaseConstraint::eGammaUnknown)
                branchMin = std::max(branchMin, theta + c->mGamma);
            else
                branchMin = std::max(branchMin, theta);
        }
    }
        
    // Synthesize all
    const double min_tmp1 = qMax(min1, min2);
    const double min_tmp2 = qMax(min3, min4);
    const double min = qMax(min_tmp1, min_tmp2);
    
    return min;
}

double Event::getThetaMaxRecursive(const double defaultValue,
                                   const QVector<QVector<Event*> >& eventBranches,
                                   const QVector<QVector<Phase*> >& phaseBranches)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne max courante pour le tirage de theta
    // ------------------------------------------------------------------
    
    double max1 = defaultValue;
    
    // Max des thetas des faits en contrainte directe antérieure
    double max2 = defaultValue;
    for(int i=0; i<eventBranches.size(); ++i)
    {
        const QVector<Event*>& branch = eventBranches[i];
        double branchMax = defaultValue;
        for(int j=branch.size()-1; j>=0; --j)
        {
            const Event* event = branch[j];
            if(event == this)
            {
                max2 = qMin(max2, branchMax);
                break;
            }
            if(event->mInitialized)
                branchMax = qMin(branchMax, event->mTheta.mX);
        }
    }
    
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    double max3 = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases[i]->mTauType != Phase::eTauUnknown)
        {
            double thetaMin = defaultValue;
            for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
            {
                const Event* event = mPhases[i]->mEvents[j];
                if(event != this && event->mInitialized)
                {
                    thetaMin = qMin(event->mTheta.mX, thetaMin);
                }
            }
            max3 = qMin(max3, thetaMin + mPhases[i]->mTau);
        }
    }
    
    // Contraintes des phases précédentes
    double max4 = defaultValue;
    for(int i=0; i<phaseBranches.size(); ++i)
    {
        const QVector<Phase*>& branch = phaseBranches[i];
        double branchMax = defaultValue;
        for(int j=branch.size()-1; j>=0; --j)
        {
            const Phase* phase = branch[j];
            if(phase->mEvents.contains(this))
            {
                max4 = qMin(max4, branchMax);
                break;
            }
            double theta = defaultValue;
            for(int k=0; k<phase->mEvents.size(); ++k)
            {
                if(phase->mEvents[k]->mInitialized)
                {
                    theta = std::min(theta, phase->mEvents[k]->mTheta.mX);
                }
            }
            const PhaseConstraint* c = 0;
            for(int k=0; k<phase->mConstraintsBwd.size(); ++k)
            {
                if(branch.contains(phase->mConstraintsBwd[k]->mPhaseFrom))
                {
                    c = phase->mConstraintsBwd[k];
                    break;
                }
            }
            if(c && c->mGammaType != PhaseConstraint::eGammaUnknown)
                branchMax = std::min(branchMax, theta - c->mGamma);
            else
                branchMax = std::min(branchMax, theta);
        }
    }
    
    // Synthesize all
    double max_tmp1 = qMin(max1, max2);
    double max_tmp2 = qMin(max3, max4);
    double max = qMin(max_tmp1, max_tmp2);
    
    return max;
}

double Event::getThetaMin(double defaultValue)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne min courante pour le tirage de theta
    // ------------------------------------------------------------------
    
    //double min1 = defaultValue;
    
    // Max des thetas des faits en contrainte directe antérieure
    double maxThetaBwd = defaultValue;
    for(int i=0; i<mConstraintsBwd.size(); ++i)
    {
        if(mConstraintsBwd[i]->mEventFrom->mInitialized)
        {
            double thetaf = mConstraintsBwd[i]->mEventFrom->mTheta.mX;
            maxThetaBwd = std::max(maxThetaBwd, thetaf);
        }
    }
    
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    double min3 = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases[i]->mTauType != Phase::eTauUnknown)
        {
            double thetaMax = defaultValue;
            for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
            {
                Event* event = mPhases[i]->mEvents[j];
                if(event != this && event->mInitialized)
                {
                    thetaMax = std::max(event->mTheta.mX, thetaMax);
                }
            }
            min3 = std::max(min3, thetaMax - mPhases[i]->mTau);
        }
    }
    
    // Contraintes des phases précédentes
    double maxPhasePrev = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        double thetaMax = mPhases[i]->getMaxThetaPrevPhases(defaultValue);
        maxPhasePrev = std::max(maxPhasePrev, thetaMax);
    }
    
    double min_tmp1 = std::max(defaultValue, maxThetaBwd);
    double min_tmp2 = std::max(min3, maxPhasePrev);
    double min = std::max(min_tmp1, min_tmp2);
    
    return min;
}

double Event::getThetaMax(double defaultValue)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne max
    // ------------------------------------------------------------------
    
    double max1 = defaultValue;
    
    // Min des thetas des faits en contrainte directe et qui nous suivent
    double maxThetaFwd = defaultValue;
    for(int i=0; i<mConstraintsFwd.size(); ++i)
    {
        if(mConstraintsFwd[i]->mEventTo->mInitialized)
        {
            double thetaf = mConstraintsFwd[i]->mEventTo->mTheta.mX;
            maxThetaFwd = std::min(maxThetaFwd, thetaf);
        }
    }
    
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    double max3 = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases[i]->mTauType != Phase::eTauUnknown)
        {
            double thetaMin = defaultValue;
            for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
            {
                Event* event = mPhases[i]->mEvents[j];
                if(event != this && event->mInitialized)
                {
                    thetaMin = std::min(event->mTheta.mX, thetaMin);
                }
            }
            max3 = std::min(max3, thetaMin + mPhases[i]->mTau);
        }
    }
    
    // Contraintes des phases suivantes
    double maxPhaseNext = defaultValue;
    for(int i=0; i<mPhases.size(); ++i)
    {
        double thetaMin = mPhases[i]->getMinThetaNextPhases(defaultValue);
        maxPhaseNext = std::min(maxPhaseNext, thetaMin);
    }
    
    double max_tmp1 = std::min(max1, maxThetaFwd);
    double max_tmp2 = std::min(max3, maxPhaseNext);
    double max = std::min(max_tmp1, max_tmp2);
    
    return max;
}

void Event::updateTheta(const double tmin, const double tmax)
{
    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);
    
    if(min >= max)
    {
        throw QObject::tr("Error for event : ") + mName + " : min = " + QString::number(min) + " : max = " + QString::number(max);
    }
    
    //qDebug() << "[" << min << ", " << max << "]";
    
    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------
    
    double sum_p = 0.;
    double sum_t = 0.;

    // with const type variable foreeach is speed
    foreach (const Date date, mDates) {
        const double variance = (date.mSigma.mX * date.mSigma.mX);
        sum_t += (date.mTheta.mX + date.mDelta) / variance;
        sum_p += 1.f / variance;
    }
    double theta_avg = sum_t / sum_p;
    double sigma = 1.f / sqrt(sum_p);
    
    switch(mMethod)
    {
        case eDoubleExp:
        {
            try{
                double theta = Generator::gaussByDoubleExp(theta_avg, sigma, min, max);
                mTheta.tryUpdate(theta, 1);
            }
            catch(QString error){
                throw QObject::tr("Error for event : ") + mName + " : " + error;
            }
            break;
        }
        case eBoxMuller:
        {
            double theta;
            long long counter = 0;
            do{
                theta = Generator::gaussByBoxMuller(theta_avg, sigma);
                ++counter;
                if(counter == 100000000)
                {
                    throw QObject::tr("No MCMC solution could be found using event method ") + ModelUtilities::getEventMethodText(mMethod) + " for event named " + mName + ". (" + QString::number(counter) + " trials done)";
                }
            }while(theta < min || theta > max);
            
            //qDebug() << "Event update num trials : " << counter;
            mTheta.tryUpdate(theta, 1);
            break;
        }
        case eMHAdaptGauss:
        {
            // MH : Seul cas où le taux d'acceptation a du sens car on utilise sigma MH :
            const double theta = Generator::gaussByBoxMuller(mTheta.mX, mTheta.mSigmaMH);
            
            double rapport = 0;
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

void Event::generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{

   /* if(type() == Event::eKnown && ek && (ek->knownType() == EventKnown::eFixed)) {
        // Nothing todo : this is just a Dirac !
        ek->mTheta.mHisto.clear();
        ek->mTheta.mChainsHistos.clear();

        ek->mTheta.mHisto.insert(ek->mFixed,1);
        for(int i =0 ;i<chains.size(); ++i) {
            //generate fictifious chains
            ek->mTheta.mChainsHistos.append(ek->mTheta.mHisto);
        }
    }
    else {
        (*iterEvent)->mTheta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
    }
    */

    if (type() != Event::eKnown)
        mTheta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);

    else {
        EventKnown* ek = dynamic_cast<EventKnown*>(this);
        if (ek && (ek->knownType() == EventKnown::eFixed)) {
            // Nothing todo : this is just a Dirac !
            ek->mTheta.mHisto.clear();
            ek->mTheta.mChainsHistos.clear();

            ek->mTheta.mHisto.insert(ek->mFixed,1);
            //generate fictifious chains
            for(int i =0 ;i<chains.size(); ++i)
                ek->mTheta.mChainsHistos.append(ek->mTheta.mHisto);

        }
    }

}
