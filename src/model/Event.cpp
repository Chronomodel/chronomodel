/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2020

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

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

    mTheta.mSupport = MetropolisVariable::eBounded;
    mTheta.mFormat = DateUtils::eUnknown;;

    // Item initial position :
    //int posDelta = 100;
    mItemX = 0.;
    mItemY = 0.;

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
    for (auto &&date : mDates) {
        date.mTheta.reset();
        date.mSigma.reset();
        date.mWiggle.reset();
    }

    mDates.clear();

    if (!mPhases.isEmpty())
        mPhases.clear();

    if (!mConstraintsFwd.isEmpty())
       mConstraintsFwd.clear();

    if (!mConstraintsBwd.isEmpty())
        mConstraintsBwd.clear();
}


// JSON
Event Event::fromJson(const QJsonObject& json)
{
    Event event;
    event.mType = Type (json.value(STATE_EVENT_TYPE).toInt());
    event.mId = json.value(STATE_ID).toInt();
    event.mName = json.value(STATE_NAME).toString();
    event.mColor = QColor(json.value(STATE_COLOR_RED).toInt(),
                          json.value(STATE_COLOR_GREEN).toInt(),
                          json.value(STATE_COLOR_BLUE).toInt());
    event.mMethod = Method (json.value(STATE_EVENT_METHOD).toInt());
    event.mItemX = json.value(STATE_ITEM_X).toDouble();
    event.mItemY = json.value(STATE_ITEM_Y).toDouble();
    event.mIsSelected = json.value(STATE_IS_SELECTED).toBool();
    event.mIsCurrent = json.value(STATE_IS_CURRENT).toBool();

    event.mTheta.mProposal = ModelUtilities::getEventMethodText(event.mMethod);
    event.mTheta.setName("Theta of Event : "+event.mName);

    event.mPhasesIds = stringListToIntList(json.value(STATE_EVENT_PHASE_IDS).toString());


    const QJsonArray dates = json.value(STATE_EVENT_DATES).toArray();

    for (auto && date : dates) {
        Date d;
        d.fromJson(date.toObject());
        d.autoSetTiSampler(true);
        d.mMixingLevel=event.mMixingLevel;

        if (!d.isNull())
            event.mDates.append(d);
        else
            throw QObject::tr("ERROR : data could not be created with plugin %1").arg(date.toObject().value(STATE_DATE_PLUGIN_ID).toString());

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
    if (mPhasesIds.size() > 0) {
        QStringList eventIds;
        for (int i(0); i<mPhasesIds.size(); ++i)
            eventIds.append(QString::number(mPhasesIds.at(i)));
        eventIdsStr = eventIds.join(",");
    }
    event[STATE_EVENT_PHASE_IDS] = eventIdsStr;

    QJsonArray dates;
    for (int i=0; i<mDates.size(); ++i) {
        QJsonObject date = mDates.at(i).toJson();
        dates.append(date);
    }
    event[STATE_EVENT_DATES] = dates;

    return event;
}

// Properties
Event::Type Event::type() const
{
    return mType;
}

// MCMC
void Event::reset()
{
    mTheta.reset();
    mInitialized = false;
    mNodeInitialized = false;
    mThetaNode = HUGE_VAL;//__builtin_inf();//INFINITY;
}

bool Event::getThetaMinPossible(const Event *originEvent, QString &circularEventName,  QList<Event *> &startEvents, QString & linkStr)
{
    QList<Event*> newStartEvents = startEvents;
    newStartEvents.append(this);

    if (linkStr.isEmpty())
        linkStr = " ➡︎ ";
    QString parallelStr  (" | ");
    QString serieStr  (" ➡︎ ");

    QString startList;
    for( Event *e : newStartEvents)
        startList += e->mName + linkStr;

    qDebug() << mName << "startList" << startList;

    if (mNodeInitialized)
        return true;

    // list of phase under
    bool noPhaseBwd (true);
    if (!mPhases.isEmpty())
        for (auto &&phase : mPhases)
            noPhaseBwd = noPhaseBwd && (phase->mConstraintsBwd.isEmpty());

    /*
     Le fait appartient à une ou plusieurs phases.
     Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
     Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
     (on utilise la valeur courante de la durée pour cela, puisqu'elle est échantillonnée ou fixée)
     */
    /*
     The fact belongs to one or more phases.
     If the phase has a duration constraint (! = Phase :: eTauUnknown),
     Make sure you are above the largest theta of the phase minus the duration
     (we use the current duration value for this, since it is sampled or fixed)
     */



    if (noPhaseBwd && mConstraintsBwd.isEmpty()) {
        mNodeInitialized = true;
        return true;
    }

    else {
        // Check constraints in Events Scene
        if (!mConstraintsBwd.isEmpty())
            for (auto &&constBwd : mConstraintsBwd) {
                if (constBwd->mEventFrom != originEvent ) {
                     const bool maxThetaOk = (constBwd->mEventFrom)->getThetaMinPossible (originEvent, circularEventName, newStartEvents, serieStr);
                     if ( !maxThetaOk) {
                         circularEventName =  serieStr + constBwd->mEventFrom->mName +  circularEventName ;
                         return false;
                    }

                } else {
                    circularEventName = serieStr + constBwd->mEventFrom->mName + " ?";
                    return false;
                }
            }



        if (!noPhaseBwd) {
            // Check constraints in Phases Scene
            for (auto &&phase : mPhases) {
                if (!phase->mConstraintsBwd.isEmpty()) {
                    for (auto &&phaseBwd : phase->mConstraintsBwd) {
                        for (auto &&eventPhaseBwd : phaseBwd->mPhaseFrom->mEvents) {
                            if (eventPhaseBwd != originEvent ) {
                                const bool tMinRecOk = eventPhaseBwd->getThetaMinPossible ( originEvent, circularEventName, newStartEvents, serieStr);
                                if (!tMinRecOk) {
                                    circularEventName = serieStr + eventPhaseBwd->mName +  circularEventName ;
                                    return false;
                                }


                            } else {
                                circularEventName = serieStr + eventPhaseBwd->mName + " !";
                                return false;
                            }
                        }

                    }
                 }
            }
        }

        // Check parallel constraints with the Events in the same phases

        for (auto &&phase : mPhases) {
                for (auto &&event : phase->mEvents) {
                    if (!newStartEvents.contains (event)) {
                           const bool thetaOk = event->getThetaMinPossible (originEvent, circularEventName, newStartEvents, parallelStr);
                           if ( !thetaOk) {
                               circularEventName = parallelStr + event->mName +  circularEventName ;
                               return false;
                           }

                    }

                }
        }

        mNodeInitialized = true;
        return true;
    }


}

bool Event::getThetaMaxPossible(const Event *originEvent, QString &circularEventName,  QList<Event *> &startEvents)
{
#ifdef DEBUG
    QString startList;
    for( Event *e : startEvents)
        startList += e->mName + "->";

#endif

    QList<Event*> newStartEvents = startEvents;
    newStartEvents.append(this);

    const QString parallelStr  (" | ");
    const QString serieStr  (" ➡︎ ");

    if (mNodeInitialized)
        return true;

    // list of phase under
    bool noPhaseFwd (true);
    if (!mPhases.isEmpty())
        for (auto &&phase : mPhases)
            noPhaseFwd = noPhaseFwd && (phase->mConstraintsFwd.isEmpty());

    /*
     Le fait appartient à une ou plusieurs phases.
     Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
     Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
     (on utilise la valeur courante de la durée pour cela, puisqu'elle est échantillonnée ou fixée)
     */
    
    /*
     The fact belongs to one or more phases.
     If the phase has a duration constraint (! = Phase :: eTauUnknown),
     Make sure you are above the largest theta of the phase minus the duration
     (we use the current duration value for this, since it is sampled or fixed)
     */


    if (noPhaseFwd && mConstraintsFwd.isEmpty()) {
        mNodeInitialized = true;
        return true;
    }

    else {
        // Check constraints in Events Scene
        if (!mConstraintsFwd.isEmpty())
            for (auto &&constFwd : mConstraintsFwd) {
                if (constFwd->mEventTo != originEvent ) {
                     const bool _ok = (constFwd->mEventTo)->getThetaMaxPossible (originEvent, circularEventName, newStartEvents);
                     if ( !_ok) {
                         circularEventName =  serieStr + constFwd->mEventTo->mName +  circularEventName ;
                         return false;
                    }

                } else {
                    circularEventName = serieStr + constFwd->mEventTo->mName + " ?";
                    return false;
                }
            }



        if (!noPhaseFwd) {
            // Check constraints in Phases Scene
            for (auto &&phase : mPhases) {
                if (!phase->mConstraintsFwd.isEmpty()) {
                    for (auto &&phaseFwd : phase->mConstraintsFwd) {
                        for (auto &&eventPhaseFwd : phaseFwd->mPhaseTo->mEvents) {
                            //if (eventPhaseFwd != originEvent ) {
                            if (!newStartEvents.contains (eventPhaseFwd)) {
                               
                                const bool _ok = eventPhaseFwd->getThetaMaxPossible ( originEvent, circularEventName, newStartEvents);
                                if (!_ok) {
                                    circularEventName = " (" + phase->mName + ")" + serieStr + eventPhaseFwd->mName + " (" + phaseFwd->mPhaseTo->mName + ")" +  circularEventName ;
                                    return false;
                                }


                            }
                            if (eventPhaseFwd == originEvent ) {
                                circularEventName = " (" + phase->mName + ")" + serieStr + eventPhaseFwd->mName + " (" + phaseFwd->mPhaseTo->mName + ") !";
                                return false;
                            }
                        }

                    }
                 }
            }
        }


        mNodeInitialized = true;
        return true;
    }
}


/**
@brief Only used in the init process
*/
double Event::getThetaMinRecursive(const double defaultValue, const QList<Event *> startEvents)
{
    // if the Event is initiated, constraints was controled previously
    if (mInitialized)
            return mTheta.mX;
    if (mNodeInitialized)
        return mThetaNode;

    if (startEvents.contains(this))
        return defaultValue;

    // list of phase under
    bool noPhaseBwd (true);
    if (!mPhases.isEmpty())
        for (auto &&phase : mPhases) {
            noPhaseBwd = noPhaseBwd && (phase->mConstraintsBwd.isEmpty());
        }

    /*
     Le fait appartient à une ou plusieurs phases.
        Si la phase a une contrainte de durée (== Phase::eTauFixed),
        Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
        (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
     */
    /*
     The fact belongs to one or more phases.
     If the phase has a duration constraint (== Phase :: eTauFixed),
     Make sure you are above the largest theta of the phase minus the duration
     (we use the current duration value for this since it is sampled)
     */
    QList<Event*> newStartEvents = startEvents;
    newStartEvents.append(this);

    double minInPhases (defaultValue);
    for (auto &&phase : mPhases) {
        if (phase->mTauType == Phase::eTauFixed || phase->mTauType == Phase::eTauOnly) {
            double thetaMax (defaultValue);
            for (auto &&event : phase->mEvents) {
                if (!newStartEvents.contains(event)) // to find the higher theta in the phase
                   thetaMax = std::max(thetaMax, event->getThetaMinRecursive(defaultValue, newStartEvents));
               
            }
            minInPhases = std::max(minInPhases, thetaMax - phase->mTau.mX);
 
        } //!= Phase::eTauUnknown // modif PhD
        
    }


    if (noPhaseBwd && mConstraintsBwd.isEmpty()) {
        mNodeInitialized = true;
        mThetaNode = minInPhases;
        return mThetaNode;
    }

    else {
        double maxThetaBwd (defaultValue);
        if (!mConstraintsBwd.isEmpty())
            for (auto &&constBwd : mConstraintsBwd) {
                if (!newStartEvents.contains(constBwd->mEventFrom)) {
                     maxThetaBwd = std::max(maxThetaBwd, (constBwd->mEventFrom)->getThetaMinRecursive(defaultValue, startEvents));
                }
            }

        double maxPhasesBwd (defaultValue);
        if (!noPhaseBwd) {

            for (auto &&phase : mPhases) {
                if (!phase->mConstraintsBwd.isEmpty()) {
                    double maxThetaBwd (defaultValue);
                    for (auto &&phaseBwd : phase->mConstraintsBwd) {

                        for (auto &&eventPhaseBwd : phaseBwd->mPhaseFrom->mEvents) {
                            if (!newStartEvents.contains(eventPhaseBwd)) {
                                maxThetaBwd = std::max(maxThetaBwd, eventPhaseBwd->getThetaMinRecursive(defaultValue, startEvents));
                            }
                        }
                        if (phaseBwd->mGammaType != PhaseConstraint::eGammaUnknown)
                            maxPhasesBwd = std::max(maxPhasesBwd, maxThetaBwd + phaseBwd->mGamma);
                        else
                            maxPhasesBwd = std::max(maxPhasesBwd, maxThetaBwd);
                    }
                 }
            }
        }
        mNodeInitialized = true;
        mThetaNode = std::max(maxThetaBwd, minInPhases);
        mThetaNode = std::max(maxPhasesBwd, mThetaNode);
        return mThetaNode;
    }


}
/**
 @brief Only used in the init process
 */
double Event::getThetaMaxRecursive(const double defaultValue, const QList<Event *> startEvents)
{
    // if the Event is initialized, constraints was controled previously
    if (mInitialized)
            return mTheta.mX;

    if (mNodeInitialized)
        return mThetaNode;

    // list of phase under
    bool noPhaseFwd (true);
    if (!mPhases.isEmpty()) {
        for (auto && phase : mPhases) {
            noPhaseFwd = noPhaseFwd && (phase->mConstraintsFwd.isEmpty());
        }
    }

    /*  Le fait appartient à une ou plusieurs phases.
        Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
        Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
        (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
     */
    QList<Event*> newStartEvents = startEvents;
    newStartEvents.append(this);

    double minPhases (defaultValue);
    for (auto &&phase :mPhases) {
        if (phase->mTauType == Phase::eTauFixed || phase->mTauType == Phase::eTauOnly) {
            double thetaMin (defaultValue);
            for (auto &&event : phase->mEvents) {
                if (!newStartEvents.contains(event) && event->mInitialized==true)
                    thetaMin = std::min(thetaMin, event->getThetaMaxRecursive(defaultValue, newStartEvents));
            }
            minPhases = std::min(minPhases, thetaMin + phase->mTau.mX);
        } // modif PhD
    }

    if (noPhaseFwd && mConstraintsFwd.isEmpty()) {
            mNodeInitialized = true;
            mThetaNode = minPhases;
            return mThetaNode;
        }


    else {
        double minTheta (defaultValue);
        if (!mConstraintsFwd.isEmpty())
            for (auto &&constFwd : mConstraintsFwd) {
                if (!newStartEvents.contains(constFwd->mEventTo))
                    minTheta = std::min(minTheta, (constFwd->mEventTo)->getThetaMaxRecursive(defaultValue, newStartEvents));

            }


        double minPhasesFwd (defaultValue);
        if (!noPhaseFwd) {

            for (auto phase : mPhases) {
                 if (!phase->mConstraintsFwd.isEmpty()) {
                    double minThetaFwd (defaultValue);
                    for (auto &&phaseFwd : phase->mConstraintsFwd) {

                        for (auto &&eventPhaseFwd : phaseFwd->mPhaseTo->mEvents) {
                            if (!newStartEvents.contains(eventPhaseFwd))
                                minThetaFwd = std::min(minThetaFwd, eventPhaseFwd->getThetaMaxRecursive(defaultValue, newStartEvents));
                        }

                        if (phaseFwd->mGammaType != PhaseConstraint::eGammaUnknown)
                            minPhasesFwd = std::min(minPhasesFwd, minThetaFwd - phaseFwd->mGamma);
                        else
                            minPhasesFwd = std::min(minPhasesFwd, minThetaFwd);
                    }
                  }
            }
        }
        mNodeInitialized = true;
        mThetaNode = std::min(minTheta, minPhases); 
        mThetaNode = std::min(minPhasesFwd, mThetaNode);
        return mThetaNode;
    }


}
/** ------------------------------------------------------------------
       Déterminer la borne min courante pour le tirage de theta
    ------------------------------------------------------------------ */
double Event::getThetaMin(double defaultValue)
{
    // Max des thetas des faits en contrainte directe antérieure
    double maxThetaBwd (defaultValue);
    for (auto &&constBwd : mConstraintsBwd) {
            maxThetaBwd = std::max(maxThetaBwd, constBwd->mEventFrom->mTheta.mX);
    }

    /*  Le fait appartient à une ou plusieurs phases.
        Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
        Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
        (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée) */
    
    double min3 (defaultValue);

    for (auto &&phase : mPhases) {
       if (phase->mTauType != Phase::eTauUnknown) {    // à utliser dans le cas d'un tirage de tau par Phase
        //if (phase->mTauType == Phase::eTauFixed) {    // à utliser dans le cas d'un tirage de tau par Event
            double thetaMax (defaultValue);
            for (auto &&event : phase->mEvents) {
                if (event != this)
                    thetaMax = std::max(event->mTheta.mX, thetaMax);
            }
            min3 = std::max(min3, thetaMax - phase->mTau.mX);
        } // modif PhD
    }

    // Contraintes des phases précédentes
    double maxPhasePrev (defaultValue);
    for (auto &&phase : mPhases) {
        //const double thetaMax = phase->getMaxThetaPrevPhases(defaultValue);
        //maxPhasePrev = std::max(maxPhasePrev, thetaMax);
        maxPhasePrev = std::max(maxPhasePrev, phase->getMaxThetaPrevPhases(defaultValue));
    }

    const double min_tmp1 = std::max(defaultValue, maxThetaBwd);
    const double min_tmp2 = std::max(min3, maxPhasePrev);
    const double min = std::max(min_tmp1, min_tmp2);

    return min;
}
/**  ------------------------------------------------------------------
      Déterminer la borne max
     ------------------------------------------------------------------
 */

double Event::getThetaMax(double defaultValue)
{
    double max1 = defaultValue;

    // Min des thetas des faits en contrainte directe et qui nous suivent
    double maxThetaFwd (defaultValue);
    for (auto &&constFwd : mConstraintsFwd) {
            maxThetaFwd = std::min(maxThetaFwd, constFwd->mEventTo->mTheta.mX);
    }

    /*  Le fait appartient à une ou plusieurs phases.
        Si la phase a une contrainte de durée (!= Phase::eTauUnknown),
        Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
        (on utilise la valeur courante de la durée pour cela, puisqu'elle est échantillonnée)
     */
    /*
     The fact belongs to one or more phases.
     If the phase has a duration constraint (!= Phase::eTauUnknown),
     Make sure you are below the smallest theta of the phase plus the duration
     (the current value of the duration is used for this, since it is sampled)
     */
    double max3 (defaultValue);
    for (auto &&phase : mPhases) {
       if (phase->mTauType != Phase::eTauUnknown) { // à utiliser dans le cas d'un tirage de tau par Phase
       // if (phase->mTauType == Phase::eTauFixed) { // à utiliser dans le cas d'un tirage de tau par Event
            double thetaMin (defaultValue);
             for (auto &&event : phase->mEvents) {
                if (event != this)
                    thetaMin = std::min(event->mTheta.mX, thetaMin);
            }
            max3 = std::min(max3, thetaMin + phase->mTau.mX);
         }    // modif PhD
    }

    // Contraintes des phases suivantes
    // Constraints of the following phases
    double maxPhaseNext (defaultValue);
    for (auto &&phase : mPhases) {
        // double thetaMin = phase->getMinThetaNextPhases(defaultValue);
        // maxPhaseNext = std::min(maxPhaseNext, thetaMin);
        maxPhaseNext = std::min(maxPhaseNext, phase->getMinThetaNextPhases(defaultValue));
    }

    const double max_tmp1 = std::min(max1, maxThetaFwd);
    const double max_tmp2 = std::min(max3, maxPhaseNext);
    const double max = std::min(max_tmp1, max_tmp2);

    return max;
}
/*
void Event::upDateTau(double& tmin, double& tmax)
{
    // définition du modèle  /
    double tau (0.);
    int n(1) ; // lui même
    bool inTauOnly (false);
    bool inTauFixed (false);
    double studyPeriod;
   
     double eAlpha(HUGE_VAL), eBeta(-HUGE_VAL);
    // union phase
    Phase unionPhase ;
    unionPhase.mTauFixed = HUGE_VAL;
    unionPhase.mTauType = Phase::eTauOnly;

    for (auto p : mPhases) {
        if (p->mTauType == Phase::eTauFixed || p->mTauType == Phase::eTauOnly) {
            for (auto ev: p->mEvents) {
                if (!unionPhase.mEvents.contains(ev)) {//ev!=this && on met quand même l'event concerné
                    unionPhase.mEvents.push_back(ev);
                }
            }
            studyPeriod = p->mStudyMax - p->mStudyMin;
            
        }
        if (p->mTauType ==  Phase::eTauFixed) {
            inTauFixed = true;
            //unionPhase.mTauType = Phase::eTauFixed;
            //ici on peut utiliser alpha et beta des phases, qui sont mis à jour aprés chaque échantillonnage d'un event dans MCMCLoopMain
            //auto ePhase = std::minmax_element(p->mEvents.cbegin(), p->mEvents.cend(), [](Event* e1, Event* e2) { return (e1->mTheta.mX < e2->mTheta.mX); });
            
            eAlpha = std::min(eAlpha, p->mBeta.mX - p->mTauFixed);
            eBeta = std::min(eBeta, p->mAlpha.mX + p->mTauFixed);
            
        } else if (p->mTauType == Phase::eTauOnly) {
            inTauOnly = true;
           // unionPhase.mTauFixed = std::min(unionPhase.mTauFixed, p->mTau.mX); // tau est dejà initialisé, ou echantillonné
        }
        
    }
    n = unionPhase.mEvents.length();
    
    if (n<1 || inTauOnly==false) //pas dans une phase eTauOnly
        return;
    
    auto sUnionPhase = std::minmax_element(unionPhase.mEvents.cbegin(), unionPhase.mEvents.cend(), [](Event* e1, Event* e2) { return (e1->mTheta.mX < e2->mTheta.mX); });
    
    //qDebug()<<"phase union pour event" << (*sUnionPhase.second)[0].mName << (*sUnionPhase.first)[0].mName << n;
    
 
    if (inTauFixed) {
        eAlpha = std::min(eAlpha, (*sUnionPhase.first)[0].mTheta.mX);
        eBeta = std::min(eBeta, (*sUnionPhase.second)[0].mTheta.mX );
    } else {
        eAlpha = (*sUnionPhase.first)[0].mTheta.mX;
        eBeta = (*sUnionPhase.second)[0].mTheta.mX;
    }
    
    if ( n>1 && inTauOnly) {
        double s = eBeta - eAlpha;
        

       // qDebug()<<"s trouvé"<<s <<"n trouvé"<<n <<" stuperiod"<<studyPeriod;
        const double precision (.0001);
        const double R (studyPeriod);
        const double Rp ( R/(n-1)*n );
           // variable à échantillonner
           
           const double u ( Generator::randomUniform(0.0, 1.0) );
          
             // Recherche solution équation F(x)-u=0
           const double FbMax = intFx(R, n, Rp, s);
           //double Fb = 1.0 - u;  // pour mémoire
           
                

               // Newton
           
           double xn (s);
           double  b1;
           double itF, p;
           if ( !isinf(FbMax)) {
               double Epsilon (R);
                           
               while ( Epsilon >= precision/100.) {

                   itF = intFx(xn, n, Rp, s);

                   itF = itF - u*FbMax;
                   p = Px(xn, n, Rp);
                
                   //b1 = itF != 0. ? (x - (itF / p )): x;
                   b1 = xn - (itF / p );
                   Epsilon = abs((xn - b1)/b1);
                   xn = std::move(b1);
                   
               }
                xn = std::max(s, std::min(xn, R));
               
           }
        tau = std::move(xn);
        qDebug()<< mName  <<"tau trouvé"<<tau;
    }
    tmax = std::min(tmax, eAlpha + tau);
    tmin = std::max(tmin, eBeta - tau);
    
           
         
}
*/
void Event::updateTheta(const double &tmin, const double &tmax)
{
    // check constraints
    double min ( getThetaMin(tmin) );
    double max ( getThetaMax(tmax) );
    
    if (min >= max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(mName, QString::number(min), QString::number(max));


    /*
      Evaluer theta.
      Le cas Wiggle est inclus ici car on utilise une formule générale.
      On est en "wiggle" si au moins une des mesures a un delta > 0.
     */
    /*
     Evaluate theta.
     The Wiggle case is included here because we are using a general formula.
     We are in "wiggle" if at least one of the measures has a delta> 0.
     */

    double sum_p (0.);
    double sum_t (0.);

    for (auto &&date: mDates) {
        const double variance (date.mSigma.mX * date.mSigma.mX);
        sum_t += (date.mTheta.mX + date.mDelta) / variance;
        sum_p += 1. / variance;
    }
    const double theta_avg (sum_t / sum_p);
    const double sigma (1. / sqrt(sum_p));

    switch(mMethod)
    {
        case eDoubleExp:
        {
            try{
                const double theta = Generator::gaussByDoubleExp(theta_avg, sigma, min, max);
                //qDebug() << "Event::updateTheta() case eDoubleExp rapport=1 ";
                mTheta.tryUpdate(theta, 1.);
            }
            catch(QString error){
                throw QObject::tr("Error for event : %1 : %2").arg(mName, error);
            }
            break;
        }
        case eBoxMuller:
        {
            double theta;
            long long counter (0.);
            do {
                theta = Generator::gaussByBoxMuller(theta_avg, sigma);
                ++counter;
                if (counter == 100000000)
                    throw QObject::tr("No MCMC solution could be found using event method %1 for event named %2 ( %3  trials done)").arg(ModelUtilities::getEventMethodText(mMethod), mName, QString::number(counter));

            } while(theta < min || theta > max);

            mTheta.tryUpdate(theta, 1.);
            break;
        }
        case eMHAdaptGauss:
        {
            /*
             MH : Seul cas où le taux d'acceptation a du sens car on utilise sigma MH
            */
            /*
             MH: Only case where the acceptance rate makes sense because we use sigma MH
            */
    
            const double theta = Generator::gaussByBoxMuller(mTheta.mX, mTheta.mSigmaMH);
            double rapport (0.);
            if (theta >= min && theta <= max)
                rapport = exp((-0.5/(sigma*sigma)) * (pow(theta - theta_avg, 2.) - pow(mTheta.mX - theta_avg, 2.)));

            //qDebug() << "Event::updateTheta() case eMHAdaptGauss rapport="<<rapport;
            mTheta.tryUpdate(theta, rapport);
            break;
        }
        default:
            break;
    }
  //qDebug() << "Event::updateTheta() name"<<mName<<mTheta.mX;
}

void Event::generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    if (type() != Event::eKnown)
        mTheta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);

    else {
        EventKnown* ek = dynamic_cast<EventKnown*>(this);
            // Nothing todo : this is just a Dirac !
            ek->mTheta.mHisto.clear();
            ek->mTheta.mChainsHistos.clear();

            ek->mTheta.mHisto.insert(ek->mFixed, 1);
            //generate fictifious chains
            for (int i =0 ;i<chains.size(); ++i)
                ek->mTheta.mChainsHistos.append(ek->mTheta.mHisto);

    }

}
