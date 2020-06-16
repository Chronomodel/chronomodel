/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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
    mItemX = 0.;//rand() % posDelta - posDelta/2;
    mItemY = 0.;//rand() % posDelta - posDelta/2;

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
    
    mY1 = event.mY1;
    mY2 = event.mY2;
    mY3 = event.mY3;
    
    mS1 = event.mS1;
    mS2 = event.mS2;
    mS3 = event.mS3;
}

Event::~Event()
{
    mTheta.reset();
    
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

    event.mY1 = json.value(STATE_EVENT_Y1).toDouble();
    event.mY2 = json.value(STATE_EVENT_Y2).toDouble();
    event.mY3 = json.value(STATE_EVENT_Y3).toDouble();
    
    event.mS1 = json.value(STATE_EVENT_S1).toDouble();
    event.mS2 = json.value(STATE_EVENT_S2).toDouble();
    event.mS3 = json.value(STATE_EVENT_S3).toDouble();

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
    event[STATE_EVENT_Y1] = mY1;
    event[STATE_EVENT_Y2] = mY2;
    event[STATE_EVENT_Y3] = mY3;
    event[STATE_EVENT_Y1] = mS1;
    event[STATE_EVENT_Y2] = mS2;
    event[STATE_EVENT_Y3] = mS3;

    QString eventIdsStr;
    if (mPhasesIds.size() > 0) {
        QStringList eventIds;
        for (int i=0; i<mPhasesIds.size(); ++i)
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

void Event::setChronocurveCsvDataToJsonEvent(QJsonObject& event, const QMap<QString, double>& chronocurveData)
{
    QMap<QString, double>::const_iterator i = chronocurveData.find("Y1");
    if(i != chronocurveData.end()){
        event[STATE_EVENT_Y1] = i.value();
    }
    i = chronocurveData.find("S1");
    if(i != chronocurveData.end()){
        event[STATE_EVENT_S1] = i.value();
    }
    i = chronocurveData.find("Y2");
    if(i != chronocurveData.end()){
        event[STATE_EVENT_Y2] = i.value();
    }
    i = chronocurveData.find("S2");
    if(i != chronocurveData.end()){
        event[STATE_EVENT_S2] = i.value();
    }
    i = chronocurveData.find("Y3");
    if(i != chronocurveData.end()){
        event[STATE_EVENT_Y3] = i.value();
    }
    i = chronocurveData.find("S3");
    if(i != chronocurveData.end()){
        event[STATE_EVENT_S3] = i.value();
    }
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

    //--
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela, puisqu'elle est échantillonnée ou fixée)




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
                                //circularEventName = startList + eventPhaseBwd->mName + " ?";
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

  //  qDebug() << mName << "startEvents : " << startList;
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

    //--
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela, puisqu'elle est échantillonnée ou fixée)




    if (noPhaseFwd && mConstraintsFwd.isEmpty()) {
        mNodeInitialized = true;
        return true;
    }

    else {
        // Check constraints in Events Scene
        if (!mConstraintsFwd.isEmpty())
            for (auto &&constFwd : mConstraintsFwd) {
                if (constFwd->mEventTo != originEvent ) {
                //if (!newStartEvents.contains (constFwd->mEventTo)) {
                   // qDebug() << " mConstraintsFwd" << constFwd->mEventTo->mName;
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
                               // qDebug() << " phase->mConstraintsFwd" << eventPhaseFwd->mName;
                                const bool _ok = eventPhaseFwd->getThetaMaxPossible ( originEvent, circularEventName, newStartEvents);
                                if (!_ok) {
                                    circularEventName = " (" + phase->mName + ")" + serieStr + eventPhaseFwd->mName + " (" + phaseFwd->mPhaseTo->mName + ")" +  circularEventName ;
                                    return false;
                                }


                            } //else {
                            if (eventPhaseFwd == originEvent ) {
                                circularEventName = " (" + phase->mName + ")" + serieStr + eventPhaseFwd->mName + " (" + phaseFwd->mPhaseTo->mName + ") !";
                                return false;
                            }
                        }

                    }
                 }
            }
        }

        // Check parallel constraints with the Events in the same phases -> useless
        //Il ne faut pas regarder à travers les autres Events de la même phase
/*
        for (auto &&phase : mPhases) {
                for (auto &&event : phase->mEvents) {
                    if (!newStartEvents.contains (event)) {

                           const bool _ok = event->getThetaMaxPossible (originEvent, circularEventName, newStartEvents);
                           if ( !_ok) {
                               circularEventName = " (" + phase->mName + ")" + parallelStr + event->mName +  " (" + phase->mName + ")" + circularEventName ;
                               return false;
                           }

                    }


                }
        }
*/
        mNodeInitialized = true;
        return true;
    }
}


//----
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

    //--
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    QList<Event*> newStartEvents = startEvents;
    newStartEvents.append(this);

    double maxPhases (defaultValue);
    for (auto &&phase : mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMax (defaultValue);
            for (auto &&event : phase->mEvents) {
                if (!newStartEvents.contains(event))
                   thetaMax = std::max(thetaMax, event->getThetaMinRecursive(defaultValue, newStartEvents));
            }
            maxPhases = std::max(maxPhases, thetaMax - phase->mTau);

        }
        //qDebug()<<"getThetaMin fin mTau maxPhases="<<maxPhases;
    }


    if (noPhaseBwd && mConstraintsBwd.isEmpty()) {
        mNodeInitialized = true;
        mThetaNode = maxPhases;
        return mThetaNode;
    }

    else {
        double maxTheta (defaultValue);
        if (!mConstraintsBwd.isEmpty())
            for (auto &&constBwd : mConstraintsBwd) {
                if (!newStartEvents.contains(constBwd->mEventFrom)) {
                     maxTheta = std::max(maxTheta, (constBwd->mEventFrom)->getThetaMinRecursive(defaultValue, startEvents));
                    //qDebug()<<" thetaMin "<< mName<<"in constBwd"<<constBwd->mEventFrom->mName;
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
                                //qDebug()<<" thetaMin "<< phase->mName<<"in phaseBwd->mPhaseFrom"<<phaseBwd->mPhaseFrom->mName<<eventPhaseBwd->mName;
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
        mThetaNode = std::max(maxTheta, maxPhases);
        mThetaNode = std::max(maxPhasesBwd, mThetaNode);
        return mThetaNode;
    }


}


double Event::getThetaMinRecursive_old(const double defaultValue,
                                   const QVector<QVector<Event*> >& eventBranches,
                                   const QVector<QVector<Phase*> >& phaseBranches)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne min courante pour le tirage de theta
    // ------------------------------------------------------------------
    double min1 (defaultValue);

    // Max des thetas des faits en contrainte directe antérieure
    double minBranchesInf (defaultValue);
    for (auto && branch : eventBranches) {

        double branchMin (defaultValue);
        for (auto && event: branch) {
            if (event == this) {
                minBranchesInf = qMax(minBranchesInf, branchMin);
                break;
            }
            if (event->mInitialized)
                branchMin = qMax(branchMin, event->mTheta.mX);
        }
    }

    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    double minPhases (defaultValue);

    for (auto && phases : mPhases) {
        if (phases->mTauType != Phase::eTauUnknown) {
            double thetaMax (defaultValue);
            for (auto && event : phases->mEvents) {
                if (event != this && event->mInitialized)
                    thetaMax = qMax(event->mTheta.mX, thetaMax);
            }
            minPhases = qMax(minPhases, thetaMax - phases->mTau);
        }
    }

    // Contraintes des phases précédentes
    double min4 (defaultValue);
    for (auto && branch : phaseBranches) {
        double branchMin = defaultValue;
        for( auto &&phase : branch) {
            if (phase->mEvents.contains(this)) {
                min4 = qMax(min4, branchMin);
                break;
            }
            double theta (defaultValue);

            for (QList<Event*>::const_iterator itEv = phase->mEvents.cbegin(); itEv != phase->mEvents.cend(); ++itEv) {
                    if ((*itEv)->mInitialized)
                        theta = std::max(theta, (*itEv)->mTheta.mX);

            }

            PhaseConstraint* c = nullptr;
            for (QList<PhaseConstraint*>::const_iterator itConstFwd = phase->mConstraintsFwd.cbegin(); itConstFwd != phase->mConstraintsFwd.cend(); ++itConstFwd) {
                if (branch.contains( (*itConstFwd)->mPhaseTo)) {
                    c = (*itConstFwd);
                    break;
                }
            }

            if (c && c->mGammaType != PhaseConstraint::eGammaUnknown)
                branchMin = std::max(branchMin, theta + c->mGamma);
            else
                branchMin = std::max(branchMin, theta);
        }
    }

    // Synthesize all
    const double min_tmp1 = qMax(min1, minBranchesInf);
    const double min_tmp2 = qMax(minPhases, min4);
    const double min = qMax(min_tmp1, min_tmp2);

    return min;
}

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

    //--
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    QList<Event*> newStartEvents = startEvents;
    newStartEvents.append(this);

    double minPhases (defaultValue);
    for (auto &&phase :mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMin (defaultValue);
            for (auto &&event : phase->mEvents) {
                if (!newStartEvents.contains(event))
                    thetaMin = std::min(thetaMin, event->getThetaMaxRecursive(defaultValue, newStartEvents));
            }
            minPhases = std::min(minPhases, thetaMin + phase->mTau);
        }
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

double Event::getThetaMaxRecursive_old(const double defaultValue,
                                   const QVector<QVector<Event*> >& eventBranches,
                                   const QVector<QVector<Phase*> >& phaseBranches)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne max courante pour le tirage de theta
    // ------------------------------------------------------------------

    double max1 (defaultValue);

    // Max des thetas des faits en contrainte directe antérieure
    double max2 (defaultValue);
    for (auto &&branch : eventBranches) {
        double branchMax = defaultValue;
        for (int j=branch.size()-1; j>=0; --j) {
            const Event* event = branch[j];
            if (event == this) {
                max2 = qMin(max2, branchMax);
                break;
            }
            if (event->mInitialized)
                branchMax = qMin(branchMax, event->mTheta.mX);
        }
    }

    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    double max3 (defaultValue);
    for (auto && phase :mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMin (defaultValue);
            for ( auto &&event : phase->mEvents) {
                if (event != this && event->mInitialized)
                    thetaMin = qMin(event->mTheta.mX, thetaMin);

            }
            max3 = qMin(max3, thetaMin + phase->mTau);
        }
    }

    // Contraintes des phases précédentes
    double max4 (defaultValue);
    for (auto &&branch : phaseBranches) {

        double branchMax (defaultValue);
        for (int j=branch.size()-1; j>=0; --j) {
            const Phase* phase = branch[j];
            if (phase->mEvents.contains(this)) {
                max4 = qMin(max4, branchMax);
                break;
            }
            double theta = defaultValue;
            for (auto && event : phase->mEvents) {
                if (event->mInitialized)
                    theta = std::min(theta, event->mTheta.mX);

            }
            const PhaseConstraint* c = nullptr;
            for (auto && constBwd: phase->mConstraintsBwd) {
                if (branch.contains(constBwd->mPhaseFrom)) {
                    c = constBwd;
                    break;
                }
            }
            if (c && c->mGammaType != PhaseConstraint::eGammaUnknown)
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

    // Max des thetas des faits en contrainte directe antérieure
    double maxThetaBwd (defaultValue);
    for (auto &&constBwd : mConstraintsBwd) {
       // if (constBwd->mEventFrom->mInitialized) { // les faits sont tous initialisés ici
            double thetaf (constBwd->mEventFrom->mTheta.mX);
            maxThetaBwd = std::max(maxThetaBwd, thetaf);
       // }
    }

    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    double min3 (defaultValue);

    for (auto &&phase : mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMax (defaultValue);
            for (auto &&event : phase->mEvents) {
                if (event != this) // && event->mInitialized)
                    thetaMax = std::max(event->mTheta.mX, thetaMax);
            }
            min3 = std::max(min3, thetaMax - phase->mTau);
        }
    }

    // Contraintes des phases précédentes
    double maxPhasePrev (defaultValue);
    for (auto &&phase : mPhases) {
        const double thetaMax = phase->getMaxThetaPrevPhases(defaultValue);
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
    double maxThetaFwd (defaultValue);
    for (int i=0; i<mConstraintsFwd.size(); ++i) {
       // if (mConstraintsFwd.at(i)->mEventTo->mInitialized) {
            double thetaf = mConstraintsFwd.at(i)->mEventTo->mTheta.mX;
            maxThetaFwd = std::min(maxThetaFwd, thetaf);
       // }
    }

    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    double max3 (defaultValue);
    for (auto &&phase : mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMin (defaultValue);
             for (auto &&event : phase->mEvents) {
                if (event != this) // && event->mInitialized)
                    thetaMin = std::min(event->mTheta.mX, thetaMin);
            }
            max3 = std::min(max3, thetaMin + phase->mTau);
        }
    }

    // Contraintes des phases suivantes
    double maxPhaseNext (defaultValue);
    for (auto &&phase : mPhases) {
        double thetaMin = phase->getMinThetaNextPhases(defaultValue);
        maxPhaseNext = std::min(maxPhaseNext, thetaMin);
    }

    double max_tmp1 = std::min(max1, maxThetaFwd);
    double max_tmp2 = std::min(max3, maxPhaseNext);
    double max = std::min(max_tmp1, max_tmp2);

    return max;
}

void Event::updateTheta(const double &tmin, const double &tmax)
{
    const double min ( getThetaMin(tmin) );
    const double max ( getThetaMax(tmax) );

    if (min >= max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(mName, QString::number(min), QString::number(max));

    // test no Event emboxed
  /*  const double theta = mDates[0].mTheta.mX;
    mTheta.tryUpdate(theta, 1.);
    return;
*/

    //qDebug() << "[" << min << ", " << max << "]";

    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------

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
            // MH : Seul cas où le taux d'acceptation a du sens car on utilise sigma MH :
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

            ek->mTheta.mHisto.insert(ek->mFixed,1);
            //generate fictifious chains
            for (int i =0 ;i<chains.size(); ++i)
                ek->mTheta.mChainsHistos.append(ek->mTheta.mHisto);
    }
}
