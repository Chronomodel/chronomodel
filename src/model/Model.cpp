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

#include "Model.h"
#include "Date.h"
#include "Project.h"
#include "EventBound.h"
#include "MCMCLoopMain.h"
#include "MCMCProgressDialog.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "StdUtilities.h"
#include "DateUtils.h"
#include "MainWindow.h"
#include "../PluginAbstract.h"
#include "MetropolisVariable.h"

#include <QJsonArray>
#include <QtWidgets>
#include <QtCore/QStringList>
//#include <execution>

#if USE_FFT
#include "fftw3.h"
#endif

// Constructor...
Model::Model():
mProject(nullptr),
mNumberOfPhases(0),
mNumberOfEvents(0),
mNumberOfDates(0),
mThreshold(-1.),
mBandwidth(1.06),
mFFTLength(1024),
mHActivity(1)
{
    
}

Model::~Model()
{

}

void Model::clear()
{
    // Deleting an event executes these main following actions :
    // - The Event MH variables are reset (freeing trace memory)
    // - The Dates MH variables are reset (freeing trace memory)
    // - The Dates are cleared
    for (Event*& ev: mEvents) {
        // Event can be an Event or an EventCurve.
        // => do not delete it using ~Event(), because the appropriate destructor could be ~EventCurve().
        delete ev;
        ev = nullptr;
    }
    
    for (EventConstraint*& ec : mEventConstraints) {
        delete ec;
        ec = nullptr;
    }
    
    mEvents.clear();
    mEventConstraints.clear();

    if (!mPhases.isEmpty()) {
        for (Phase*& ph: mPhases) {
            if (ph) {
                delete ph;
                ph = nullptr;
            }
        }
        mPhases.clear();
    }

    if (!mPhaseConstraints.isEmpty()) {
        for (PhaseConstraint*& pc : mPhaseConstraints) {
            if (pc)
                pc->~PhaseConstraint();

        }
        mPhaseConstraints.clear();
    }

    mChains.clear();
    mLogModel.clear();
    mLogInit.clear();
    mLogAdapt.clear();
    mLogResults.clear();
}

/**
 * @brief Model::updateFormatSettings, set all date format according to the Application Preference, date format
 * @param appSet
 */
void Model::updateFormatSettings()
{
    for (const auto& event : mEvents) {
        event->mTheta.setFormat(AppSettings::mFormatDate);

        for (auto&& date : event->mDates) {
            date.mTi.setFormat(AppSettings::mFormatDate);
            date.mSigmaTi.setFormat(DateUtils::eNumeric);
            date.mWiggle.setFormat(AppSettings::mFormatDate);
        }

    }

    for (const auto& phase : mPhases) {
        phase->mAlpha.setFormat(AppSettings::mFormatDate);
        phase->mBeta.setFormat(AppSettings::mFormatDate);
        phase->mDuration.setFormat(DateUtils::eNumeric);
        phase->mTau.setFormat(DateUtils::eNumeric);

        // update Tempo and activity curves
        phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);
        phase->mTempoInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoInf);
        phase->mTempoSup = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoSup);

        phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);
        phase->mActivityInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityInf);
        phase->mActivitySup = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivitySup);

        phase->mActivityUnifMean = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifMean);
/*        phase->mActivityUnifInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifInf);
        phase->mActivityUnifSup = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifSup);
*/
    }
}

// JSON conversion

void Model::fromJson(const QJsonObject& json)
{
    if (json.contains(STATE_SETTINGS)) {
        const QJsonObject settings = json.value(STATE_SETTINGS).toObject();
        mSettings = ProjectSettings::fromJson(settings);
    }

    if (json.contains(STATE_MCMC)) {
        const QJsonObject mcmc = json.value(STATE_MCMC).toObject();
        mMCMCSettings = MCMCSettings::fromJson(mcmc);
        mChains = mMCMCSettings.getChains();
    }

    if (json.contains(STATE_PHASES)) {
        const QJsonArray phases = json.value(STATE_PHASES).toArray();
        mNumberOfPhases = phases.size();

        for (int i=0; i<phases.size(); ++i) {
            const QJsonObject phase = phases.at(i).toObject();
            Phase* p = new Phase(Phase::fromJson(phase));
            mPhases.append(p);
            p = nullptr;

        }
    }

    // Sort phases based on items y position
    std::sort(mPhases.begin(), mPhases.end(), sortPhases);

    if (json.contains(STATE_EVENTS)) {
        QJsonArray events = json.value(STATE_EVENTS).toArray();
        mNumberOfEvents = events.size();

        for (int i = 0; i < events.size(); ++i) {
            const QJsonObject event = events.at(i).toObject();

            if (event.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) {
                try {
                    Event* e = new Event();//Event::fromJson(event));
                    e->copyFrom(Event::fromJson(event));
                    e->mMixingLevel = mMCMCSettings.mMixingLevel;
                    mNumberOfDates += e->mDates.size();

                    for (auto&& d : e->mDates) {
                        d.mMixingLevel = e->mMixingLevel;
                        d.mColor = e->mColor;
                    }
                    mEvents.append(e);
                    e = nullptr;
                }
                catch(QString error){
                    QMessageBox message(QMessageBox::Critical,
                                        qApp->applicationName() + " " + qApp->applicationVersion(),
                                        QObject::tr("Error : %1").arg(error),
                                        QMessageBox::Ok,
                                        qApp->activeWindow());
                    message.exec();
                }
            } else {
                EventKnown* ek = new EventKnown();
                *ek = EventKnown::fromJson(event);
                ek->updateValues(mSettings.mTmin, mSettings.mTmax, mSettings.mStep);
                mEvents.append(ek);
                ek = nullptr;
            }
        }
    }


    // Sort events based on items y position
    std::sort(mEvents.begin(), mEvents.end(), sortEvents);

    if (json.contains(STATE_EVENTS_CONSTRAINTS)) {
        const QJsonArray constraints = json.value(STATE_EVENTS_CONSTRAINTS).toArray();
        for (int i=0; i<constraints.size(); ++i) {
            const QJsonObject constraint = constraints.at(i).toObject();
            EventConstraint* c = new EventConstraint(EventConstraint::fromJson(constraint));
            mEventConstraints.append(c);
        }
    }

    if (json.contains(STATE_PHASES_CONSTRAINTS)) {
        const QJsonArray constraints = json.value(STATE_PHASES_CONSTRAINTS).toArray();
        for (int i=0; i<constraints.size(); ++i) {
            const QJsonObject constraint = constraints.at(i).toObject();
            PhaseConstraint* c = new PhaseConstraint(PhaseConstraint::fromJson(constraint));
            mPhaseConstraints.append(c);
        }
    }

    // ------------------------------------------------------------
    //  Link objects to each other
    //  Must be done here !
    //  nb : Les data sont déjà linkées aux events à leur création
    // ------------------------------------------------------------
    for (int i=0; i<mEvents.size(); ++i) {
        int eventId = mEvents.at(i)->mId;
        QList<int> phasesIds = mEvents.at(i)->mPhasesIds;

        // Link des events / phases
        for (int j=0; j<mPhases.size(); ++j) {
            const int phaseId = mPhases.at(j)->mId;
            if (phasesIds.contains(phaseId)) {
                mEvents[i]->mPhases.append(mPhases[j]);
                mPhases[j]->mEvents.append(mEvents[i]);
            }
        }

        // Link des events / contraintes d'event
        for (int j=0; j<mEventConstraints.size(); ++j) {
            if (mEventConstraints[j]->mFromId == eventId) {
                mEventConstraints[j]->mEventFrom = mEvents[i];
                mEvents[i]->mConstraintsFwd.append(mEventConstraints[j]);
            } else if (mEventConstraints[j]->mToId == eventId) {
                mEventConstraints[j]->mEventTo = mEvents[i];
                mEvents[i]->mConstraintsBwd.append(mEventConstraints[j]);
            }
        }
    }
    // Link des phases / contraintes de phase
    for (int i=0; i<mPhases.size(); ++i) {
        const int phaseId = mPhases.at(i)->mId;
        for (int j=0; j<mPhaseConstraints.size(); ++j) {
            if (mPhaseConstraints.at(j)->mFromId == phaseId) {
                mPhaseConstraints[j]->mPhaseFrom = mPhases[i];
                mPhases[i]->mConstraintsFwd.append(mPhaseConstraints[j]);
            } else if (mPhaseConstraints.at(j)->mToId == phaseId) {
                mPhaseConstraints[j]->mPhaseTo = mPhases[i];
                mPhases[i]->mConstraintsBwd.append(mPhaseConstraints[j]);
            }
        }

    }

}

void Model::setProject( Project * project)
{
    mProject = project;
}


/**
 * @brief ResultsView::updateModel Update Design
 */
void Model::updateDesignFromJson()
{
    if (!mProject)
        return;

    const QJsonObject state = mProject->state();
    const QJsonArray events = state.value(STATE_EVENTS).toArray();
    const QJsonArray phases = state.value(STATE_PHASES).toArray();

    QJsonArray::const_iterator iterJSONEvent = events.constBegin();
    while (iterJSONEvent != events.constEnd()) {
        const QJsonObject eventJSON = (*iterJSONEvent).toObject();
        const int eventId = eventJSON.value(STATE_ID).toInt();
        const QJsonArray dates = eventJSON.value(STATE_EVENT_DATES).toArray();

        QList<Event *>::iterator iterEvent = mEvents.begin();
        while (iterEvent != mEvents.end()) {
            if ((*iterEvent)->mId == eventId) {
                (*iterEvent)->mName  = eventJSON.value(STATE_NAME).toString();
                (*iterEvent)->mItemX = eventJSON.value(STATE_ITEM_X).toDouble();
                (*iterEvent)->mItemY = eventJSON.value(STATE_ITEM_Y).toDouble();
                (*iterEvent)->mIsSelected = eventJSON.value(STATE_IS_SELECTED).toBool();
                (*iterEvent)->mColor = QColor(eventJSON.value(STATE_COLOR_RED).toInt(),
                                              eventJSON.value(STATE_COLOR_GREEN).toInt(),
                                              eventJSON.value(STATE_COLOR_BLUE).toInt());
                
                for (int k = 0; k<(*iterEvent)->mDates.size(); ++k) {
                    Date& d = (*iterEvent)->mDates[k];
                    for (auto &&dateVal : dates) {
                        const QJsonObject date = dateVal.toObject();
                        const int dateId = date.value(STATE_ID).toInt();

                        if (dateId == d.mId) {
                            d.mName = date.value(STATE_NAME).toString();
                            d.mColor = (*iterEvent)->mColor;
                            break;
                        }
                    }
                }
                break;
            }
            ++iterEvent;
        }
        ++iterJSONEvent;
    }

    QJsonArray::const_iterator iterJSONPhase = phases.constBegin();
    while (iterJSONPhase != phases.constEnd()) {
        const QJsonObject phaseJSON = (*iterJSONPhase).toObject();
        const int phaseId = phaseJSON.value(STATE_ID).toInt();

        for (const auto& p : mPhases) {
            if (p->mId == phaseId) {
                p->mName = phaseJSON.value(STATE_NAME).toString();
                p->mItemX = phaseJSON.value(STATE_ITEM_X).toDouble();
                p->mItemY = phaseJSON.value(STATE_ITEM_Y).toDouble();
                p->mColor = QColor(phaseJSON.value(STATE_COLOR_RED).toInt(),
                                   phaseJSON.value(STATE_COLOR_GREEN).toInt(),
                                   phaseJSON.value(STATE_COLOR_BLUE).toInt());
                p->mIsSelected = phaseJSON.value(STATE_IS_SELECTED).toBool();
                break;
            }
        }
        ++iterJSONPhase;
    }

    std::sort(mEvents.begin(), mEvents.end(), sortEvents);
    std::sort(mPhases.begin(), mPhases.end(), sortPhases);

    for (const auto& p : mPhases ) {
        std::sort(p->mEvents.begin(), p->mEvents.end(), sortEvents);
    }
}

QJsonObject Model::toJson() const
{
    QJsonObject json;

    json["settings"] = mSettings.toJson();
    json["mcmc"] = mMCMCSettings.toJson();

    QJsonArray events;
    for (const auto& event : mEvents)
        events.append(event->toJson());

    json["events"] = events;

    QJsonArray phases;
    for (const auto& pPhase : mPhases)
        phases.append(pPhase->toJson());

    json["phases"] = phases;

    QJsonArray event_constraints;
    for (const auto& eventConstraint : mEventConstraints)
        event_constraints.append(eventConstraint->toJson());

    json["event_constraints"] = event_constraints;

    QJsonArray phase_constraints;

    for (const auto& pPhaseConstraint : mPhaseConstraints)
        phase_constraints.append(pPhaseConstraint->toJson());

    json["phase_constraints"] = phase_constraints;

    return json;
}

// Logs

QString Model::getModelLog() const{
    return mLogModel;
}

QString Model::getInitLog() const{
    QString log;
    int i = 1;
    for (const auto& chain : mChains) {
        log += line( tr("Elapsed init time %1 for chain %2").arg(DHMS(chain.mInitElapsedTime), QString::number(i)));
        ++i;
    }
    return log + mLogInit;
}

QString Model::getAdaptLog() const{
    QString log;
    int i = 1;
    for (const auto& chain : mChains) {
        log += line( tr("Elapsed adaptation time %1 for chain %2").arg(DHMS(chain.mAdaptElapsedTime), QString::number(i)));
        ++i;
    }
    return log + mLogAdapt;
}

/**
 * @brief Model::generateModelLog
 * @return Return a QString with the recall of the Model with the data MCMC Methode, and constraint
 */
void Model::generateModelLog()
{
    mLogModel = ModelUtilities::modelDescriptionHTML(static_cast<ModelCurve*>(this));
}

QString Model::getResultsLog() const{
    return mLogResults;
}

void Model::generateResultsLog()
{
    QString log;
    int i = 1;
    for (const auto& chain : mChains) {
        log += line( tr("Elapsed acquisition time %1 for chain %2").arg(DHMS(chain.mAcquisitionElapsedTime), QString::number(i)));
        ++i;
    }
    log += "<hr>";
    for (const auto& pPhase : mPhases) {
        log += ModelUtilities::phaseResultsHTML(pPhase);
        /** @todo delete repeted word phase */
         QString tempoStr = ModelUtilities::tempoResultsHTML(pPhase);
         tempoStr.remove(1, 41);
         log += tempoStr;
         log += "<hr>";
    }

    for (const auto& pPhaseConstraint : mPhaseConstraints) {
        log += ModelUtilities::constraintResultsHTML(pPhaseConstraint);
        log += "<hr>";
    }
    for (const auto& pEvent : mEvents) {
        log += ModelUtilities::eventResultsHTML(pEvent, true, this);
        log += "<hr>";
    }

    mLogResults = log;
}

// Results CSV
QList<QStringList> Model::getStats(const QLocale locale, const int precision, const bool withDateFormat)
{
    (void) withDateFormat;
    QList<QStringList> rows;

    int maxHpd = 0;

    // Phases

    for (const auto& pPhase : mPhases) {
        QStringList l = pPhase->mAlpha.getResultsList(locale, precision);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(pPhase->mName + " Begin");
        rows << l;

        l = pPhase->mBeta.getResultsList(locale, precision);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(pPhase->mName + " End");
        rows << l;

        /*
        l = pPhase->mTau.getResultsList(locale, precision);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(pPhase->mName + " Tau");
        rows << l;
        */
    }

    // Events
    rows << QStringList();
     for (Event*& event : mEvents) {
        QStringList l = event->mTheta.getResultsList(locale, precision);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(event->mName);
        rows << l;
    }

    // Dates
    rows << QStringList();
    for (Event*& event : mEvents) {
        for (int j = 0; j < event->mDates.size(); ++j) {
            Date& date = event->mDates[j];

            QStringList l = date.mTi.getResultsList(locale, precision);
            maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
            l.prepend(date.mName);
            rows << l;
        }
    }

    // Headers
    QStringList list;
    // list << "Item" << "MAP" << "Mean" << "Std dev" << "Q1" << "Q2" << "Q3" << "Credibility %" << "Credibility begin" << "Credibility end";
    list << "Item" << "Trace:Mean" << "Trace:Std"  << "Trace:Q1"<< "Trace:Q2"<< "Trace:Q3" << "Trace:min value" << "Trace:max value";
    list <<"Density:MAP"<< "Density:Mean" << "Density:Std"<< "Density:Q1"<< "Density:Q2"<< "Density:Q3" << "Credibility %" << "Credibility start" << "Credibility end";
    for (int i = 0; i < maxHpd; ++i) {
        list << "HPD" + QString::number(i + 1) + " %";
        list << "HPD" + QString::number(i + 1) + " begin";
        list << "HPD" + QString::number(i + 1) + " end";
    }
    rows.prepend(list);

    return rows;
}

QList<QStringList> Model::getPhasesTraces(const QLocale locale, const bool withDateFormat)
{
    QList<QStringList> rows;

    QStringList headers;
    headers << "iter";

    for (const auto& pPhase : mPhases)
        headers << pPhase->mName + " Begin" << pPhase->mName + " End";

    rows << headers;

    int shift = 0;
    for (int i = 0; i < mChains.size(); ++i) {
        int burnAdaptSize = 1 + mChains.at(i).mIterPerBurn + (mChains.at(i).mBatchIndex * mChains.at(i).mIterPerBatch);
        int runSize = mChains.at(i).mRealyAccepted;

        for (int j = burnAdaptSize; j<burnAdaptSize + runSize; ++j) {
            QStringList l;
            l << QString::number(shift + j);
            for (const auto& pPhase : mPhases) {
                double valueAlpha = pPhase->mAlpha.mRawTrace->at(shift + j);

                if (withDateFormat)
                    valueAlpha = DateUtils::convertToAppSettingsFormat(valueAlpha);
                l << locale.toString(valueAlpha, 'g', 15);

                double valueBeta = pPhase->mBeta.mRawTrace->at(shift + j);

                if (withDateFormat)
                    valueBeta = DateUtils::convertToAppSettingsFormat(valueBeta);

                l << locale.toString(valueBeta, 'g', 15);

                // double valueTau = pPhase->mTau.mRawTrace->at(shift + j);
                // l << locale.toString(valueTau, 'g', 15);

            }
            rows << l;
        }
        shift += burnAdaptSize + runSize;
    }
    return rows;
}

QList<QStringList> Model::getPhaseTrace(int phaseIdx, const QLocale locale, const bool withDateFormat)
{
    QList<QStringList> rows;

    Phase* phase = nullptr;
    if (phaseIdx >= 0 && phaseIdx < mPhases.size())
        phase = mPhases.value(phaseIdx);

    else
        return QList<QStringList>();


 /*   int runSize = 0;

    for (auto& chain : mChains)
        runSize += chain.mRealyAccepted;
*/
    QStringList headers;
    headers << "iter" << phase->mName + " Begin" << phase->mName + " End";
    for (const auto& event : phase->mEvents)
        headers << event->mName;

    rows << headers;

    int shift = 0;

    for (const ChainSpecs& chain : mChains) {
        int burnAdaptSize = 1 + chain.mIterPerBurn + (chain.mBatchIndex * chain.mIterPerBatch);
        int runSize = chain.mRealyAccepted;

        for (int j = burnAdaptSize; j < (burnAdaptSize + runSize); ++j) {
            QStringList l;
            l << QString::number(shift + j) ;
            double valueAlpha = phase->mAlpha.mRawTrace->at(shift + j);
            if (withDateFormat)
                valueAlpha = DateUtils::convertToAppSettingsFormat(valueAlpha);

            l << locale.toString(valueAlpha, 'g', 15);

            double valueBeta = phase->mBeta.mRawTrace->at(shift + j);
            if (withDateFormat)
                valueBeta = DateUtils::convertToAppSettingsFormat(valueBeta);

            l << locale.toString(valueBeta, 'g', 15);

           /* double valueTau = phase->mTau.mRawTrace->at(shift + j);
            l << locale.toString(valueTau, 'g', 15);
*/
            for (const auto& event : phase->mEvents) {
                double value = event->mTheta.mRawTrace->at(shift + j);
                if (withDateFormat)
                    value = DateUtils::convertToAppSettingsFormat(value);

                l << locale.toString(value, 'g', 15);
            }
            rows << l;
        }
        shift += burnAdaptSize + runSize;
    }
    return rows;
}

QList<QStringList> Model::getEventsTraces(QLocale locale,const bool withDateFormat)
{
    QList<QStringList> rows;

    QStringList headers;
    headers << "iter";
    for (const auto& event : mEvents)
        headers << event->mName;

    rows << headers;

    int shift = 0;
    for (const auto& chain : mChains)  {
        const int burnAdaptSize = 1+ chain.mIterPerBurn + (chain.mBatchIndex * chain.mIterPerBatch);
        const int runSize = chain.mRealyAccepted;

        for (int j = burnAdaptSize; j < burnAdaptSize + runSize; ++j) {
            QStringList l;
            l << QString::number(shift + j) ;

            for (const auto& event : mEvents) {
                double value = event->mTheta.mRawTrace->at(shift + j);
                if (withDateFormat)
                    value = DateUtils::convertToAppSettingsFormat(value);

                l << locale.toString(value, 'g', 15);
            }
            rows << l;
        }
        shift += burnAdaptSize + runSize;
    }
    return rows;
}

//   Model validity
/**
 * @brief Model::isValid Check if the model is valid
 * @return
 */
bool Model::isValid()
{
    bool curveModel = mProject->isCurve();
    // 1 - At least one event is required in a model
    // 3 events is needed for a curve
    if (mEvents.isEmpty()) {
        throw QObject::tr("At least one event is required");
        return false;

     } else if (curveModel && mEvents.size() < 3) {
            throw tr("The model must contain at least 3 Events");
            return false;
    }

    // 2 - The event must contain at least 1 data
    for (int i = 0; i < mEvents.size(); ++i) {
        if (mEvents.at(i)->type() == Event::eDefault && mEvents.at(i)->mDates.size() == 0) {
                    throw tr("The event  \" %1 \" must contain at least 1 data").arg(mEvents.at(i)->mName);
                    return false;

        }
    }

    // 3 - The phase must contain at least 1 event
    for (int i = 0; i<mPhases.size(); ++i) {
        if (mPhases.at(i)->mEvents.size() == 0) {
            throw tr("The phase \" %1 \" must contain at least 1 event").arg(mPhases.at(i)->mName);
            return false;
        }
    }

    // 4 - Pas de circularité sur les contraintes des Event
    QVector<QVector<Event*> > eventBranches;
    try {
        eventBranches = ModelUtilities::getAllEventsBranches(mEvents);
    } catch(QString &error){
        throw &error;
    }

    // 5 - Pas de circularité sur les contraintes de phases
    // 6 - Gammas : sur toutes les branches, la somme des gamma min < plage d'étude :
    QVector<QVector<Phase*> > phaseBranches;
    try {
        phaseBranches = ModelUtilities::getAllPhasesBranches(mPhases, mSettings.mTmax - mSettings.mTmin);
    } catch(QString &error){
        throw &error;
    }

    // 7 - Un Event ne peut pas appartenir à 2 phases en contrainte
    for (int i = 0; i < phaseBranches.size(); ++i) {
        QVector<Event*> branchEvents;
        for (int j = 0; j < phaseBranches.at(i).size(); ++j) {
            Phase* phase = phaseBranches[i][j];
             for (const auto& pEvent : phase->mEvents) {
                if (!branchEvents.contains(pEvent)) {
                    branchEvents.append(pEvent);
                } else
                    throw QString(tr("The event \" %1 \" cannot belong to several phases in a same branch!").arg(pEvent->mName));
            }
        }
    }

    // 8 - Bounds : vérifier cohérence des bornes en fonction des contraintes de Events (page 2)
    //  => Modifier les bornes des intervalles des bounds !! (juste dans le modèle servant pour le calcul)
    for (int i = 0; i<eventBranches.size(); ++i) {
        for (auto j = 0; j<eventBranches.at(i).size(); ++j) {
            Event* event = eventBranches[i][j];
            if (event->type() == Event::eBound)  {
                EventKnown* bound = dynamic_cast<EventKnown*>(event);

                // --------------------
                // Check bound interval lower value
                // --------------------

                // On vérifie toutes les bornes avant et on prend le max
                // de leurs valeurs fixes ou du début de leur intervalle :
                double lower = double (mSettings.mTmin);
                for (auto k = 0; k<j; ++k) {
                    Event* evt = eventBranches[i][k];
                    if (evt->mType == Event::eBound) {
                        EventKnown* bd = dynamic_cast<EventKnown*>(evt);
                        //if(bd->mKnownType == EventKnown::eFixed)
                        lower = qMax(lower, bd->mFixed);
                        //else if(bd->mKnownType == EventKnown::eUniform)
                        //    lower = qMax(lower, bd->mUniformStart);
                    }
                }
                // Update bound interval

                if (bound->mFixed < lower)
                    throw QString(tr("The bound \" %1 \" has a fixed value inconsistent with previous bounds in chain!").arg(bound->mName));

              /*  else if (bound->mKnownType == EventKnown::eUniform)
                    bound->mUniformStart = qMax(bound->mUniformStart, lower);
                */

                // --------------------
                // Check bound interval upper value
                // --------------------
                double upper = mSettings.mTmax;
                for (auto k = j+1; k<eventBranches.at(i).size(); ++k) {
                    Event* evt = eventBranches[i][k];
                    if (evt->mType == Event::eBound) {
                        EventKnown* bd = dynamic_cast<EventKnown*>(evt);
                        // if (bd->mKnownType == EventKnown::eFixed)
                        upper = qMin(upper, bd->mFixed);

                    }
                }
                // Update bound interval
                if (bound->mFixed > upper)
                    throw QString(tr("The bound \" %1 \" has a fixed value inconsistent with next bounds in chain!").arg(bound->mName));

            }
            event = nullptr;
        }
    }

    // 9 - Gamma min (ou fixe) entre 2 phases doit être inférieur à la différence entre : le min des sups des intervalles des bornes de la phase suivante ET le max des infs des intervalles des bornes de la phase précédente
    for (int i = 0; i<mPhaseConstraints.size(); ++i) {
        double gammaMin = 0.;
        PhaseConstraint::GammaType gType = mPhaseConstraints.at(i)->mGammaType;
        if (gType == PhaseConstraint::eGammaFixed)
            gammaMin = mPhaseConstraints[i]->mGammaFixed;

        else if (gType == PhaseConstraint::eGammaRange)
            gammaMin = mPhaseConstraints.at(i)->mGammaMin;

        double lower = mSettings.mTmin;
        Phase* phaseFrom = mPhaseConstraints.at(i)->mPhaseFrom;
        for (int j = 0; j<phaseFrom->mEvents.size(); ++j) {
            EventKnown* bound = dynamic_cast<EventKnown*>(phaseFrom->mEvents[j]);
            if (bound)
                lower = qMax(lower, bound->mFixed);

        }
        double upper = double (mSettings.mTmax);
        Phase* phaseTo = mPhaseConstraints.at(i)->mPhaseTo;
        for (int j=0; j<phaseTo->mEvents.size(); ++j) {
            EventKnown* bound = dynamic_cast<EventKnown*>(phaseTo->mEvents[j]);
            if (bound)
                upper = qMin(upper, bound->mFixed);

            bound = nullptr;
        }
        if (gammaMin >= (upper - lower))
            throw QString(tr("The constraint between phases \" %1 \" and \" %2 \" is not consistent with the bounds they contain!").arg(phaseFrom->mName, phaseTo->mName));

        phaseFrom = nullptr;
        phaseTo = nullptr;
    }

    // 10 - Au sein d'une phase, tau max (ou fixe) doit être supérieur à la différence entre le max des infs des intervalles des bornes et le min des sups des intervalles des bornes.
    //  => Modifier les intervalles des bornes:
    //      - L'inf est le max entre : sa valeur courante ou (le max des infs des intervalles des bornes - tau max ou fixe)
    //      - Le sup est le min entre : sa valeur courante ou (le min des sups des intervalles des bornes + tau max ou fixe)

    for (int i = 0; i<mPhases.size(); ++i) {
        if (mPhases.at(i)->mTauType != Phase::eTauUnknown) {
            double tauMax = mPhases.at(i)->mTauFixed;
           // if (mPhases.at(i)->mTauType == Phase::eTauRange)
           //     tauMax = mPhases.at(i)->mTauMax;

            double min = mSettings.mTmin;
            double max = mSettings.mTmax;
            bool boundFound = false;

            for (int j = 0; j<mPhases.at(i)->mEvents.size(); ++j) {
                if (mPhases.at(i)->mEvents.at(j)->mType == Event::eBound) {
                    EventKnown* bound = dynamic_cast<EventKnown*>(mPhases.at(i)->mEvents[j]);
                    if (bound) {
                        boundFound = true;
                        min = std::max(min, bound->mFixed);
                        max = std::min(max, bound->mFixed);

                    }
                    bound = nullptr;
                }
            }
            if (boundFound){
                if (tauMax < (max - min))
                    throw QString(tr("The phase \" %1 \" has a duration inconsistent with the bounds it contains!").arg(mPhases.at(i)->mName));
            }
        }
    }

    // 11 - Vérifier la cohérence entre les contraintes de faits et de phase
    for (int i = 0; i<phaseBranches.size(); ++i) {
        for (int j = 0; j<phaseBranches.at(i).size(); ++j) {
            Phase* phase = phaseBranches[i][j];
            for (int k = 0; k<phase->mEvents.size(); ++k) {
                Event* event = phase->mEvents[k];

                bool phaseFound = false;

                // On réinspecte toutes les phases de la branche et on vérifie que le fait n'a pas de contrainte en contradiction avec les contraintes de phase !
                for (int l = 0; l<phaseBranches.at(i).size(); ++l) {
                    Phase* p = phaseBranches[i][l];
                    if (p == phase)
                        phaseFound = true;

                    else {
                        for (int m = 0; m<p->mEvents.size(); ++m) {
                            Event* e = p->mEvents[m];

                            // Si on regarde l'élément d'un phase d'avant, le fait ne peut pas être en contrainte vers un fait de cette phase
                            if (!phaseFound) {
                                for (int n = 0; n<e->mConstraintsBwd.size(); ++n) {
                                    if (e->mConstraintsBwd[n]->mEventFrom == event)
                                        throw tr("The event %1  (in phase %2 ) is before the event %3 (in phase %4), BUT the phase %5 is after the phase %6 .\r=> Contradiction !").arg(event->mName, phase->mName, e->mName, p->mName, phase->mName, p->mName) ;

                                }
                            } else {
                                for (int n = 0; n<e->mConstraintsFwd.size(); ++n) {
                                    if (e->mConstraintsFwd[n]->mEventTo == event)
                                        throw tr("The event %1 ( in phase %2 ) is after the event %3  ( in phase %4 ), BUT the phase %4 is before the phase .\r=> Contradiction !").arg(event->mName, phase->mName, e->mName, p->mName, phase->mName, p->mName);
                                }
                            }
                        }
                    }
                    p = nullptr;
                }
                event = nullptr;
            }
        }
    }

    return true;
}

// Generate model data
void Model::generateCorrelations(const QList<ChainSpecs> &chains)
{

#ifdef DEBUG
    qDebug()<<"Model::generateCorrelations()";
    QElapsedTimer t;
    t.start();
#endif

    for (const auto& event : mEvents ) {
        event->mTheta.generateCorrelations(chains);

        for (auto&& date : event->mDates ) {
            date.mTi.generateCorrelations(chains);
            date.mSigmaTi.generateCorrelations(chains);
        }
    }

    for (const auto& phase : mPhases ) {
        phase->mAlpha.generateCorrelations(chains);
        phase->mBeta.generateCorrelations(chains);
        //phase->mTau.generateCorrelations(chains); // ??? à voir avec PhL, est-ce utile ?
    }

#ifdef DEBUG
    qDebug() <<  "=> Model::generateCorrelations done in " + DHMS(t.elapsed());
#endif

}

#pragma mark FFTLength, Threshold, bandwidth

void Model::setBandwidth(const double bandwidth)
{  
    if (mBandwidth != bandwidth) {
        updateDensities(mFFTLength, bandwidth, mThreshold);
        mBandwidth = bandwidth;

        emit newCalculus();
    }
}

void Model::setFFTLength(size_t FFTLength)
{
    if (mFFTLength != FFTLength) {
        updateDensities(FFTLength, mBandwidth, mThreshold);
        emit newCalculus();
    }
}

void Model::setHActivity(const double h)
{
    if (mFFTLength != h) {
        if (!mPhases.isEmpty()) {
            generateActivity(mFFTLength, h);
            mHActivity = h;

            emit newCalculus();
        }
    }
}
/**
 * @brief Model::setThreshold this is a slot
 * @param threshold
 */
void Model::setThreshold(const double threshold)
{
    if (mThreshold != threshold) {
        generateCredibility(threshold);
        generateHPD(threshold);

        setThresholdToAllModel(threshold);

        emit newCalculus();
    }
}

void Model::setThresholdToAllModel(const double threshold)
{
    mThreshold = threshold;
    for (const auto& e : mEvents) {
        if (e->type() != Event::eBound){
          e->mTheta.mThresholdUsed = mThreshold;

          for (auto&& date : e->mDates ) {
                date.mTi.mThresholdUsed = mThreshold;
                date.mSigmaTi.mThresholdUsed = mThreshold;
            }
        }
    }
    for (const auto& p : mPhases) {
       p->mAlpha.mThresholdUsed = mThreshold;
       p->mBeta.mThresholdUsed = mThreshold;
       p->mTau.mThresholdUsed = mThreshold;
       p->mDuration.mThresholdUsed = mThreshold;
    }
}

double Model::getThreshold() const
{
    return mThreshold;
}

double Model::getBandwidth() const
{
    return mBandwidth;
}

int Model::getFFTLength() const
{
    return mFFTLength;
}

#pragma mark Densities

void Model::initNodeEvents()
{
    std::for_each( mEvents.begin(), mEvents.end(), [](Event* ev) {
        ev->mNodeInitialized = false;
        ev->mThetaNode = HUGE_VAL;
    });
}

/**
 * @brief Make all densities, credibilities and time range
 */
void Model::initDensities()
{
    mHActivity = std::max(1., abs(mSettings.mTmax - mSettings.mTmin) / 100.);
    clearThreshold();
    // memo the new value of the Threshold inside all the part of the model: phases, events and dates
    updateDensities(mFFTLength, mBandwidth, 95.);

}

void Model::updateDensities(int fftLen, double bandwidth, double threshold)
{
    clearPosteriorDensities();
    // memo the new value of the Threshold inside all the part of the model: phases, events and dates
  //  setThresholdToAllModel();

    updateFormatSettings();

    generatePosteriorDensities(mChains, fftLen, bandwidth);

    generateCredibility(threshold);
    generateHPD(threshold);

    if (!mPhases.isEmpty()) {
         generateTempoAndActivity(fftLen, mHActivity);
    }
    generateNumericalResults(mChains);

    setThresholdToAllModel(threshold);
    mBandwidth = bandwidth;
    mFFTLength = fftLen;

}

void Model::generatePosteriorDensities(const QList<ChainSpecs> &chains, int fftLen, double bandwidth)
{
#ifdef DEBUG
    QElapsedTimer t;
    t.start();
#endif

    const double tmin = mSettings.getTminFormated();
    const double tmax = mSettings.getTmaxFormated();

    for (const auto& event : mEvents) {
                event->mTheta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);

                for (auto&& d : event->mDates)
                    d.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
    }

    for (const auto& phase : mPhases)
         phase->generateHistos(chains, fftLen, bandwidth, tmin, tmax);

#ifdef DEBUG
    qDebug() <<  "=> Model::generatePosteriorDensities done in " + DHMS(t.elapsed());
#endif
}

void Model::generateNumericalResults(const QList<ChainSpecs> &chains)
{
#ifdef DEBUG
    QElapsedTimer t;
    t.start();
#endif

    for (const auto& event : mEvents) {
        event->mTheta.generateNumericalResults(chains);

        for (auto&& date : event->mDates) {
            date.mTi.generateNumericalResults(chains);
            date.mSigmaTi.generateNumericalResults(chains);
        }

    }

    for (const auto& phase : mPhases) {
        phase->mAlpha.generateNumericalResults(chains);
        phase->mBeta.generateNumericalResults(chains);
        // phase->mTau.generateNumericalResults(chains);
        phase->mDuration.generateNumericalResults(chains);
    }

#ifdef DEBUG

    qDebug() <<  "=> Model::generateNumericalResults done in " + DHMS(t.elapsed()) ;
#endif
}

void Model::clearThreshold()
{
   // mThreshold = -1.;

    for (const auto& event : mEvents) {
        event->mTheta.mThresholdUsed = -1.;

        for (auto&& date : event->mDates) {
            date.mTi.mThresholdUsed = -1.;
            date.mSigmaTi.mThresholdUsed = -1.;
        }
    }

    for (const auto& phase : mPhases) {
        phase->mAlpha.mThresholdUsed = -1.;
        phase->mBeta.mThresholdUsed = -1.;
        phase->mTau.mThresholdUsed = -1.;
        phase->mDuration.mThresholdUsed = -1.;
    }
}

void Model::generateCredibility(const double &thresh)
{
#ifdef DEBUG
    qDebug()<<QString("Model::generateCredibility( %1 %)").arg(thresh);
    QElapsedTimer t;
    t.start();
#endif
    if (mThreshold == thresh)
        return;

    for (const auto& pEvent : mEvents) {
        bool isFixedBound = false;
        if (pEvent->type() == Event::eBound)
                isFixedBound = true;

        if (!isFixedBound) {
            pEvent->mTheta.generateCredibility(mChains, thresh);

            for (auto&& date : pEvent->mDates )  {
                date.mTi.generateCredibility(mChains, thresh);
                date.mSigmaTi.generateCredibility(mChains, thresh);
            }
        }

    }

    // Diplay a progressBar if "long" set with setMinimumDuration()

/*    QProgressDialog *progress = new QProgressDialog(tr("Time range & credibilities generation"), tr("Wait") , 1, 10);
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(nullptr);
    progress->setMinimumDuration(5);
    progress->setMinimum(0);
    progress->setMaximum(mPhases.size()*4);
    //progress->setMinimumWidth(7 * AppSettings::widthUnit());
    progress->setMinimumWidth(int (progress->fontMetrics().boundingRect(progress->labelText()).width() * 1.5));
*/

    //int position = 0;

    for (const auto& pPhase :mPhases) {

        // if there is only one Event in the phase, there is no Duration
        pPhase->mAlpha.generateCredibility(mChains, thresh);
//        progress->setValue(++position);

        pPhase->mBeta.generateCredibility(mChains, thresh);
 //       progress->setValue(++position);
      //  pPhase->mTau.generateCredibility(mChains, thresh);
        pPhase->mDuration.generateCredibility(mChains, thresh);
 //       progress->setValue(++position);

        pPhase->mTimeRange = timeRangeFromTraces(pPhase->mAlpha.fullRunRawTrace(mChains),
                                                             pPhase->mBeta.fullRunRawTrace(mChains),thresh, "Time Range for Phase : "+pPhase->mName);

 //       progress->setValue(++position);

    }
 /*   progress->hide();
    progress->~QProgressDialog();

    QProgressDialog *progressGap = new QProgressDialog(tr("Gaps and transitions generation"), tr("Wait") , 1, 10);
    progressGap->setWindowModality(Qt::WindowModal);
    progressGap->setCancelButton(nullptr);
    progressGap->setMinimumDuration(5);
    progressGap->setMinimum(0);
    progressGap->setMaximum(mPhases.size()*4);
    progressGap->setMinimum(0);
    progressGap->setMaximum(mPhaseConstraints.size()*2);

    progressGap->setMinimumWidth(int (progressGap->fontMetrics().boundingRect(progressGap->labelText()).width() *1.5));
*/
    //position = 0;
    for (const auto& phaseConstraint : mPhaseConstraints) {

        Phase* phaseFrom = phaseConstraint->mPhaseFrom;
        Phase* phaseTo  = phaseConstraint->mPhaseTo;

        phaseConstraint->mGapRange = gapRangeFromTraces(phaseFrom->mBeta.fullRunRawTrace(mChains),
                                                             phaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Gap Range : "+phaseFrom->mName+ " to "+ phaseTo->mName);

        qDebug()<<"Gap Range "<<phaseFrom->mName<<" to "<<phaseTo->mName;
//        progressGap->setValue(++position);

        phaseConstraint->mTransitionRange = transitionRangeFromTraces(phaseFrom->mBeta.fullRunRawTrace(mChains),
                                                             phaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Transition Range : "+phaseFrom->mName+ " to "+ phaseTo->mName);

        qDebug()<<"Transition Range "<<phaseFrom->mName<<" to "<<phaseTo->mName;
//        progressGap->setValue(++position);

    }
//    delete progressGap;
#ifdef DEBUG

    qDebug() <<  "=> Model::generateCredibility done in " + DHMS(t.elapsed());
#endif

}

void Model::generateHPD(const double thresh)
{
#ifdef DEBUG
    QElapsedTimer t;
    t.start();
#endif

    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        bool isFixedBound = false;

        if ((*iterEvent)->type() == Event::eBound) {
            //EventKnown* ek = dynamic_cast<EventKnown*>(*iterEvent);
            //if(ek->knownType() == EventKnown::eFixed)
                isFixedBound = true;
        }

        if (!isFixedBound) {
            (*iterEvent)->mTheta.generateHPD(thresh);

            for (int j = 0; j<(*iterEvent)->mDates.size(); ++j) {
                Date& date = (*iterEvent)->mDates[j];
                date.mTi.generateHPD(thresh);
                date.mSigmaTi.generateHPD(thresh);
            }
        }
        ++iterEvent;
    }
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
       // if there is only one Event in the phase, there is no Duration
       (*iterPhase)->mAlpha.generateHPD(thresh);
       (*iterPhase)->mBeta.generateHPD(thresh);
       (*iterPhase)->mDuration.generateHPD(thresh);
        ++iterPhase;
    }
#ifdef DEBUG
    qDebug() <<  "=> Model::generateHPD done in " + DHMS(t.elapsed()) ;
#endif

}

//#define UNIT_TEST
void Model::generateTempoAndActivity(size_t gridLength, double h)
{
#ifdef DEBUG
   // qDebug()<<"Model::generateTempoAndActivity() "<<mSettings.mTmin<<mSettings.mTmax;
    QElapsedTimer tClock;
    tClock.start();
#endif

// Avoid to redo calculation, when mActivity exist, it happen when the control is changed
    int activityToDo = 0;
    int tempoToDo = 0;
    for (const auto& phase : mPhases) {
        if (phase->mRawActivity.isEmpty() || gridLength != mFFTLength || h != mHActivity)
            ++activityToDo;

        if (phase->mRawTempo.isEmpty() || gridLength != mFFTLength)
            ++tempoToDo;
    }

    if (activityToDo == 0 && tempoToDo == 0) {// no computation
        return;

    } else if (activityToDo > 0 && tempoToDo == 0) {
        generateActivity(gridLength, h);
        return;
    }

    for (const auto& phase : mPhases) {
        if (phase->mEvents.size() < 2) {
            phase->mActivityValueStack["Significance Score"] = TValueStack("Significance Score", 0);
            phase->mActivityValueStack["R_etendue"] = TValueStack("R_etendue", mSettings.mTmax - mSettings.mTmin);
            phase->mActivityValueStack["t_min"] = TValueStack("t_min", mSettings.mTmin);
            phase->mActivityValueStack["t_max"] = TValueStack("t_max", mSettings.mTmax);
            phase->mActivityValueStack["t_min_R"] = TValueStack("t_min_R", mSettings.mTmin);
            phase->mActivityValueStack["t_max_R"] = TValueStack("t_max_R", mSettings.mTmax);
            continue;
        }

        // Curves for error binomial
        const double n = phase->mEvents.size();
        phase->mRawActivityUnifMean.clear();
       /* phase->mRawActivityUnifInf.clear();
        phase->mRawActivityUnifSup.clear();*/

        if (!mBinomiale_Gx.contains(n)){
            const std::vector<double> Rq = binomialeCurveByLog(n); //  Détermine la courbe x = r (q)
            mBinomiale_Gx[n] = inverseCurve(Rq); // Pour qActivity, détermine la courbe p = g (x)
        }

        const std::vector<double>& Gx = mBinomiale_Gx[n];


        // Create concatened trace for Mean and Variance estimation needed for uniform predict

        std::vector<double> concaTrace;

        double t_min_R = +INFINITY;
        double t_max_R = -INFINITY;

        for (const auto& ev : phase->mEvents) {
            const auto rawtrace = ev->mTheta.fullRunRawTrace(mChains);
            const Quartiles Q95 = quantilesType(rawtrace, 7, 0.025);
            t_min_R = std::min(t_min_R, Q95.Q1);
            t_max_R = std::max(t_max_R, Q95.Q3);

            concaTrace.resize(concaTrace.size() + rawtrace.size());
            std::copy_backward( rawtrace.begin(), rawtrace.end(), concaTrace.end() );
        }

        const double nr = concaTrace.size();

        const double t_min_data = *std::min_element(concaTrace.begin(), concaTrace.end());
        const double t_max_data = *std::max_element(concaTrace.begin(), concaTrace.end());

        if (t_min_data == t_max_data) { // hapen when there is only one bound in the phase ???

           qDebug()<<"Model::generateTempoAndActivity() tmin == tmax : " <<phase->mName;
            phase->mRawTempo[t_min_data] = 1;
            phase->mRawTempo[mSettings.mTmax] = 1;
            // Convertion in the good Date format
            phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);

            phase->mRawActivity[t_min_data] = 1;
            // Convertion in the good Date format
            phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);

            phase->mActivityValueStack["Significance Score"] = TValueStack("Significance Score", 0);
            phase->mActivityValueStack["R_etendue"] = TValueStack("R_etendue", 0);
            phase->mActivityValueStack["t_min"] = TValueStack("t_min", t_min_data);
            phase->mActivityValueStack["t_max"] = TValueStack("t_max", t_max_data);
            phase->mActivityValueStack["t_min_R"] = TValueStack("t_min_R", 0);
            phase->mActivityValueStack["t_max_R"] = TValueStack("t_max_R", 0);
            continue;
        }


       // const double mu = -2;
        //const double R_etendue = (n+1)/(n-1)/(1.+ mu*sqrt(2./(double)((n-1)*(n+2))) )*(t_max_data-t_min_data);
        const double gamma =  (n>=500 ? 1. : gammaActivity[(int)n]);

        const double R_etendue = (t_max_R - t_min_R)/gamma;

        // prevent h=0 and h >R_etendue;
        h = std::min( std::max(mSettings.mStep, h),  R_etendue) ;

        const double fUnif = h / R_etendue;

        const double mid_R = (t_max_R + t_min_R)/2.;

        const double ActivityUnif = fUnif * n / h; //  remplace -> fUnif * n / std::min(h, R_etendue);

        const double half_etendue = R_etendue/2. ; //h /2.;

        t_min_R = mid_R - half_etendue;
        t_max_R = mid_R + half_etendue;
        phase->mActivityValueStack["t_min_R"] = TValueStack("t_min_R", t_min_R);
        phase->mActivityValueStack["t_max_R"] = TValueStack("t_max_R", t_max_R);

        phase->mRawActivityUnifMean.insert(t_min_R,  ActivityUnif);
        phase->mRawActivityUnifMean.insert(t_max_R,  ActivityUnif);


#ifdef DEBUG
        if (t_max_data > mSettings.mTmax) {
            qWarning("Model::generateTempoAndActivity() tmax>mSettings.mTmax force tmax = mSettings.mTmax");
            //t_max_data = mSettings.mTmax;
        }

#endif

        /// \f$ \delta_t_min = (t_max - t_min)/(gridLength-1) \f$
        const double delta_t_min = (t_max_data-t_min_data) / double(gridLength-1);

        /// \f$ \delta_t = (t_max - t_min + h)/(gridLenth-1) \f$
        double delta_t = (t_max_data-t_min_data + h) / double(gridLength-1);
        if (h < delta_t_min) {
             h = delta_t_min;
             delta_t = delta_t_min;
         }

         // overlaps
         const double h_2 = h/2.;
         const double t_min = std::max(t_min_data - h_2, mSettings.mTmin);
         const double t_max = std::min(t_max_data + h_2, mSettings.mTmax);
        // Loop
         std::vector<int> niActivity (gridLength);
         std::vector<int> niTempo (gridLength);
         const int iMax = gridLength-1;
   try {
        int iActivityGridMin, iActivityGridMax;
        int iTempoMin;

        for (const auto& t : concaTrace) {
            iActivityGridMin = inRange(0, (int) ceil((t - h_2 - t_min) / delta_t), (int)gridLength-1) ;
            iActivityGridMax = inRange(0, (int) ceil((t + h_2 - t_min) / delta_t), (int)gridLength-1) ;

            if (iActivityGridMax == iActivityGridMin) {
                ++*(niActivity.begin()+iActivityGridMin);

            } else {
                for (auto&& ni = niActivity.begin() + iActivityGridMin; ni != niActivity.begin() + iActivityGridMax; ++ni) {
                    ++*ni ;
                }
            }

            iTempoMin = std::min(std::max (0, (int) ceil((t - t_min) / delta_t)), iMax);
            ++*(niTempo.begin()+iTempoMin); // one item per grid

        }
        // calculation of distribution on niTempo
        std::transform (niTempo.begin()+1, niTempo.end(), niTempo.begin(), niTempo.begin()+1, std::plus<double>());

       } catch (std::exception& e) {
        qWarning()<< "Model::generateTempoAndActivity exception caught: " << e.what() << '\n';

       } catch(...) {
        qWarning() << "Model::generateTempoAndActivity Caught Exception!\n";

       }


       ///# Calculation of the mean and variance
        // Variable for Activity
        QVector<double> infA;
        QVector<double> supA;
        QVector<double> espA;
        double fA, eA, QSup, QInf;
        double UnifScore = 0.;

        // Variable for Tempo
        QVector<double> infT;
        QVector<double> supT;

        QVector<double> espT;
        double pT, eT, vT, infpT;

        auto niT = niTempo.begin();

        double t;

        int nbIt = 0;

        for (const auto& niA : niActivity) {
            // Compute Activity
            fA = niA / nr;

            eA =  fA * n / h;

            espA.append(eA);

            QSup = interpolate_value_from_curve(fA, Gx, 0, 1.)* n / h;
            supA.append(QSup);

            QInf = findOnOppositeCurve(fA, Gx)* n / h;
            infA.append(QInf);

#ifdef DEBUG
            if (QSup < QInf) {
                qDebug()<<"generateTempoAndActivity() QSup < QInf f="<<fA<< " QSup="<<QSup<<" QInf="<<QInf;
            }
#endif

            // Compute Tempo
            pT = *niT/ nr;

            eT =  n * pT ;

            vT = n * pT * (1-pT);

            espT.append(eT);

            // Forbidden negative error
            infpT = ( eT < 1.96 * sqrt(vT) ? 0. : eT - 1.96 * sqrt(vT) );
            infT.append( infpT );
            supT.append( eT + 1.96 * sqrt(vT));
            ++niT;

            t = t_min + nbIt * delta_t;
            if ((t_min_R <= t) && (t <= t_max_R)) {
                /* Delta(h) = somme sur theta de ( max(Aunif - Ainf) - min(Aunif, Asup) ) / nbre de theta de la grille, nbre de pas de la grille
                 */
                UnifScore += (std::max(ActivityUnif, QInf) - std::min(ActivityUnif, QSup))/gridLength; // ??? à revoir avec Ph L


            }
            nbIt++;

        }

        phase->mActivityValueStack["Significance Score"] = TValueStack("Significance Score", UnifScore);
        phase->mActivityValueStack["R_etendue"] = TValueStack("R_etendue", R_etendue);
        phase->mActivityValueStack["t_min"] = TValueStack("t_min", t_min_data);
        phase->mActivityValueStack["t_max"] = TValueStack("t_max", t_max_data);

        phase->mRawActivity = vector_to_map(espA, t_min, t_max, delta_t);
        phase->mRawActivityInf = vector_to_map(infA, t_min, t_max, delta_t);
        phase->mRawActivitySup = vector_to_map(supA, t_min, t_max, delta_t);

        if ( t_max  <= mSettings.mTmax) {
            phase->mRawActivity.last() = 0.;
            phase->mRawActivityInf.last() = 0.;
            phase->mRawActivitySup.last() = 0.;
        }

        if (t_min >= mSettings.mTmin) {
            phase->mRawActivity.first() = 0.;
            phase->mRawActivityInf.first() = 0.;
            phase->mRawActivitySup.first() = 0.;

        }
        phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);
        phase->mActivityInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityInf);
        phase->mActivitySup = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivitySup);


        phase->mActivityUnifMean = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifMean);
     /*   phase->mActivityUnifInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifInf);
        phase->mActivityUnifSup = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifSup);
*/

        phase->mRawTempo = vector_to_map(espT, t_min, t_max, delta_t);
        phase->mRawTempoInf = vector_to_map(infT, t_min, t_max, delta_t);
        phase->mRawTempoSup = vector_to_map(supT, t_min, t_max, delta_t);

        // close the error curve on mean value
        const double tEnd = phase->mRawTempo.lastKey();
        const double vEnd = phase->mRawTempo[tEnd];

        if ( tEnd <= mSettings.mTmax) {
            phase->mRawTempoInf[tEnd] = vEnd;
            phase->mRawTempoSup[tEnd ] = vEnd;
        }
        phase->mRawTempo.insert(mSettings.mTmax, vEnd);

        const double tBegin = phase->mRawTempo.firstKey();

        // We need to add a point with the value 0 for the automatique Y scaling
        if ((tBegin) >= mSettings.mTmin) {
            phase->mRawTempo[tBegin] = 0;
            phase->mRawTempoInf[tBegin] = 0;
            phase->mRawTempoSup[tBegin] = 0;
        }
        phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);
        phase->mTempoInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoInf);
        phase->mTempoSup = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoSup);

     } // Loop End on phase

#ifdef DEBUG
    qDebug() <<  QString("=> Model::generateTempoAndActivity() done in " + DHMS(tClock.elapsed()));

#endif

}


/**
 * @brief Model::generateTempo
 * The function check if the table mTempo exist. In this case, there is no calcul.
 * This calculus must be in date, not in age
 */

void Model::generateTempo(size_t gridLength)
{
#ifdef DEBUG
    QElapsedTimer tClock;
    tClock.start();
#endif

// Avoid to redo calculation, when mRawTempo exist, it happen when the control is changed
    int tempoToDo = 0;
    for (const auto& phase : mPhases) {
        if (phase->mRawTempo.isEmpty())
            ++tempoToDo;
    }

    if (tempoToDo == 0) // no computation
        return;

    // debut code
    /// We want an interval bigger than the maximun finded value, we need a point on tmin, tmax and tmax+deltat

    for (const auto& phase : mPhases) {
        // Avoid to redo calculation, when mTempo exist, it happen when the control is changed
        if (!phase->mRawTempo.isEmpty()) {
            continue;
        }


        // Create concatened trace for Mean and Variance estimation needed for uniform predict

        std::vector<double> concaTrace;
        for (const auto& ev : phase->mEvents) {
            const auto rawtrace = ev->mTheta.fullRunRawTrace(mChains);
            concaTrace.resize(concaTrace.size() + rawtrace.size());
            std::copy_backward( rawtrace.begin(), rawtrace.end(), concaTrace.end() );
        }

        double tmin = *std::min_element(concaTrace.begin(), concaTrace.end());
        double tmax = *std::max_element(concaTrace.begin(), concaTrace.end());

        const double nr = concaTrace.size();
        /*
        // create Empty containers
        std::vector<std::vector<int>> Ni (gridLenth);

         ///# 1 - Generate Event scenario
         // We suppose, it is the same iteration number for all chains
        std::vector<QVector<double>> listTrace;
        for (const auto& ev : phase->mEvents)
            listTrace.push_back(ev->mTheta.fullRunRawTrace(mChains));

        size_t totalIter = listTrace.at(0).size();

        /// Look for the maximum span containing values \f$ x=2 \f$
        tmin = mSettings.mTmax;
        tmax = mSettings.mTmin;

#ifndef UNIT_TEST
        for (const auto& l : listTrace) {
            const double lmin = *std::min_element(l.cbegin(), l.cend());
            tmin = std::min(tmin, lmin );
            const double lmax = *std::max_element(l.cbegin(), l.cend());
            tmax = std::max(tmax, lmax);
        }
        tmin = std::floor(tmin);
        tmax = std::ceil(tmax);
#endif
*/
        if (tmin == tmax) {

           qDebug()<<"Model::generateTempo() tmin == tmax : " <<phase->mName;
            phase->mRawTempo[tmin] = 1;
            phase->mRawTempo[mSettings.mTmax] = 1;

            // Convertion in the good Date format
            phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);

            continue;
        }
#ifdef DEBUG
        if (tmax > mSettings.mTmax) {
            qWarning("Model::generateActivity() tmax>mSettings.mTmax force tmax = mSettings.mTmax");
            tmax = mSettings.mTmax;
        }
#endif

        /// \f$ \delta_t = (t_max - t_min)/(gridLength-1) \f$
        const double delta_t = (tmax-tmin) / double(gridLength-1);


        /// Loop
/*
        std::vector<double> scenario;
        for (size_t i = 0; i<totalIter; ++i) {

            /// Create one scenario per iteration
            scenario.clear();
            for (const auto& lt : listTrace)
                scenario.push_back(lt.at(i));

            /// Insert the scenario dates in the activity grid
            std::vector<int> Nij (gridLenth);

            int idxGridMin;

            for (const auto& tScenario : scenario) {
                idxGridMin = std::max (0, (int) ceil((tScenario - tmin) / delta_t));

                for (auto&& nij = Nij.begin() + idxGridMin; nij < Nij.end() ; ++nij) {
                    ++*nij ;
                }

            }

            std::vector<int>::iterator itNij (Nij.begin());
            for (auto&& n : Ni) {
                n.push_back(*itNij);
                ++itNij;
            }

        }
        /// Loop End on totalIter
*/
        //new avec NiTot
        std::vector<int> NiTot (gridLength);
        int idxGridMin;
        for (const auto& t : concaTrace) {
            idxGridMin = std::min(std::max (0, (int) ceil((t - tmin) / delta_t)), (int)NiTot.size()-1);

            for (auto&& ni = NiTot.begin() + idxGridMin; ni != NiTot.end(); ++ni) {
                ++*ni ;
            }
        }
    ///# Calculation of the variance
        QVector<double> inf;
        QVector<double> sup;

        QVector<double> esp;

        double p, e, v, infp;
        const double n =  phase->mEvents.size();
        //const double nr = totalIter * phase->mEvents.size();

        /*for (const auto& vecNij : Ni) {
            p = std::accumulate(vecNij.begin(),  vecNij.end(), 0.);
            p /= nr;
        */
        for (const auto& ni : NiTot) {
            p = ni/ nr;

            e =  n * p ;

            v = n * p * (1-p);

            esp.append(e);

            // Forbidden negative error
            infp = ( e < 1.96 * sqrt(v) ? 0. : e - 1.96 * sqrt(v) );
            inf.append( infp );
            sup.append( e + 1.96 * sqrt(v));

        }

#ifndef UNIT_TEST
       // ++position;
//        progress->setValue(position);
#endif


        phase->mRawTempo = vector_to_map(esp, tmin, tmax, delta_t);
        phase->mRawTempoInf = vector_to_map(inf, tmin, tmax, delta_t);
        phase->mRawTempoSup = vector_to_map(sup, tmin, tmax, delta_t);

        // close the error curve on mean value
        const double tEnd = phase->mRawTempo.lastKey();
        const double vEnd = phase->mRawTempo[tEnd];

        if ( tEnd <= mSettings.mTmax) {
            phase->mRawTempoInf[tEnd] = vEnd;
            phase->mRawTempoSup[tEnd ] = vEnd;
        }
        phase->mRawTempo.insert(mSettings.mTmax, vEnd);

        const double tBegin = phase->mRawTempo.firstKey();

        // We need to add a point with the value 0 for the automatique Y scaling
        if ((tBegin) >= mSettings.mTmin) {
            phase->mRawTempo[tBegin] = 0;
            phase->mRawTempoInf[tBegin] = 0;
            phase->mRawTempoSup[tBegin] = 0;
        }
        phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);
        phase->mTempoInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoInf);
        phase->mTempoSup = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoSup);

     } // Loop End on phase

#ifdef DEBUG
    qDebug() <<  QString("=> Model::generateTempo() done in " + DHMS(tClock.elapsed()));

#endif

}




/**
 *  @brief Clear model data
 */
void Model::clearPosteriorDensities()
{
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        for (auto&& date : (*iterEvent)->mDates) {
            date.mTi.mHisto.clear();
            date.mSigmaTi.mHisto.clear();
            date.mTi.mChainsHistos.clear();
            date.mSigmaTi.mChainsHistos.clear();
        }
        (*iterEvent)->mTheta.mHisto.clear();
        (*iterEvent)->mTheta.mChainsHistos.clear();

        ++iterEvent;
    }

    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
        (*iterPhase)->mAlpha.mHisto.clear();
        (*iterPhase)->mBeta.mHisto.clear();
       // (*iterPhase)->mTau.mHisto.clear();
        (*iterPhase)->mDuration.mHisto.clear();

        (*iterPhase)->mAlpha.mChainsHistos.clear();
        (*iterPhase)->mBeta.mChainsHistos.clear();
        //(*iterPhase)->mTau.mChainsHistos.clear();
        (*iterPhase)->mDuration.mChainsHistos.clear();
        ++iterPhase;
    }
}

void Model::generateActivityBinomialeCurve(const int n, std::vector<double>& C1x, std::vector<double>& C2x, const double alpha)
{
    const double p_frac = 100.;
    const double x_frac = 100.;

    const double alpha2 = alpha/2.;
    const double one_alpha2 = (1-alpha/2.);

    double p, proba, sum_p = 0.;

    double x, qq;
    double k1, k2;

    /* Calcul de la courbe p=f(x) */
    std::vector<double> C1;
    std::vector<double> C2;



    for (int i = 0; i< (p_frac-1); ++i) {
           p = i/p_frac;

           qq = p/(1-p);
           k1 = 0;
           k2 = 0;
           double prev_sum = 0;
           sum_p = 0;
           for (int k = 0; k<n+1; ++k) {
               if (k == 0) {
                   proba = pow(1-p, n);

               } else {
                   proba *= (double)(n-k+1)/(double)k * qq;

               }
               sum_p += proba;

                if (sum_p == alpha2) {
                   k2 = k;

               } else  if (prev_sum < alpha2 && alpha2< sum_p) {
                   if ( k == 0) {
                       k2 = 0;

                   } else if (k<n) {
                     k2 = (alpha2-prev_sum)/proba + k -1;

                   } else {
                       k2 = n;
                   }
               }

               if (sum_p == one_alpha2) {
                   k1 = k;

               } else if (prev_sum < one_alpha2 && one_alpha2 < sum_p) {
                   if ( k == 0) {
                       k1 = 0;

                   } else if (k<n) {
                       k1 = (one_alpha2-prev_sum)/proba + k ;

                   } else {
                       k1 = n;
                   }

               }

               prev_sum = sum_p;

           }

           C1.push_back(k1);
           C2.push_back(k2);

       }
    // p = 100%
    C1.push_back(n);
    C2.push_back(n);

    /* Inversion de la courbe pour obtenir x=f(p)  */

    for (int i = 0; i<= x_frac; ++i) {
        x = (double)(i*n) / x_frac;
        p = 0.;
        unsigned long j = 0;
        while (C1[j] < x && j<C1.size()) {
            p = j / p_frac;
            ++j;
          }
        C1x.push_back(p);

        j = 0;
        p = 0;
         while (C2[j] <= x && j<C2.size()) {
            p = j /p_frac;
            ++j;
          }
        C2x.push_back(p);
    }
}

/**
 * @brief Model::generateActivity
 * @param gridLenth
 * @param h defined in year, if h<0 then h= delta_t \f$ \delta_t = (t_max - t_min + 4h )/(gridLenth) \f$
 */
void Model::generateActivity(size_t gridLength, double h)
{
#ifdef DEBUG
   // qDebug()<<"Model::generateActivity() "<<mSettings.mTmin<<mSettings.mTmax;
    QElapsedTimer tClock;
    tClock.start();
#endif

// Avoid to redo calculation, when mActivity exist, it happen when the control is changed
    int activityToDo = 0;
    for (const auto& phase : mPhases) {
        if (phase->mRawActivity.isEmpty() || gridLength != mFFTLength || h != mHActivity)
            ++activityToDo;
    }

    if (activityToDo == 0) // no computation
        return;


    /// We want an interval bigger than the maximun finded value, we need a point on tmin, tmax and tmax+deltat


    for (const auto& phase : mPhases) {
        if (phase->mEvents.size() < 2)
            continue;

        // Curves for error binomial
        const double n = phase->mEvents.size();
        phase->mRawActivityUnifMean.clear();

        const std::vector<double>& Gx = mBinomiale_Gx[n];

        // Create concatened trace for Mean and Variance estimation needed for uniform predict

        std::vector<double> concaTrace;

        for (const auto& ev : phase->mEvents) {
            const auto rawtrace = ev->mTheta.fullRunRawTrace(mChains);
            concaTrace.resize(concaTrace.size() + rawtrace.size());
            std::copy_backward( rawtrace.begin(), rawtrace.end(), concaTrace.end() );
        }
        const double nr = concaTrace.size();

        // Do not change, it is the same sampling.
        // The values of the previous calculation are recovered directly.
        const double t_min_data = phase->mActivityValueStack["t_min"].mValue;
        const double t_max_data = phase->mActivityValueStack["t_max"].mValue;

        const double R_etendue = phase->mActivityValueStack["R_etendue"].mValue;
        const double t_min_R = phase->mActivityValueStack["t_min_R"].mValue;
        const double t_max_R = phase->mActivityValueStack["t_max_R"].mValue;

        // Prevent h = 0 and h > R_etendue;
        h = inRange(mSettings.mStep, h, R_etendue );

        const double fUnif = h / R_etendue;

        const double ActivityUnif = fUnif * n / h;

        phase->mRawActivityUnifMean.insert(t_min_R,  ActivityUnif);
        phase->mRawActivityUnifMean.insert(t_max_R,  ActivityUnif);

        //double unifInf, unifSup;
       // if (n*qUnif*(1-qUnif) > 10.) {
        // cas approximation gaussienne // inutile avec binomialeCurveByLog, testé avec n=5000
     /*   if (pow(1-qUnif, n) <= 0) { // over Range in binimialConfidence95()
                unifInf = (n / M_m) - 1.96 * sqrt(n / M_m) * ((1./ h) - (1. / M_m)) ;
                unifSup = (n / M_m) + 1.96 * sqrt(n / M_m) * ((1./ h) - (1. / M_m)) ;

        } else {
        */


        /// Look for the maximum span containing values \f$ x=2 \f$

        if (t_min_data == t_max_data) {
            qDebug()<<"Model::generateActivity() tmin == tmax : " <<phase->mName;
            phase->mRawActivity[t_min_data] = 1;

            // Convertion in the good Date format
            phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);

            continue;
        }


#ifdef DEBUG
        if (t_max_data > mSettings.mTmax) {
            qWarning("Model::generateActivity() tmax>mSettings.mTmax force tmax = mSettings.mTmax");
           // t_max_data = mSettings.mTmax;
        }

#endif

        /// \f$ \delta_t_min = (t_max - t_min)/(gridLength-1) \f$
        const double delta_t_min = (t_max_data-t_min_data) / (gridLength - 1);

        /// \f$ \delta_t = (t_max - t_min + h)/(gridLenth-1) \f$
        double delta_t = (t_max_data - t_min_data + h) / (gridLength - 1);
        if (h < delta_t_min) {
             h = delta_t_min;
             delta_t = delta_t_min;
         }

         // overlaps
         const double h_2 = h/2.;
         const double t_min = std::max(t_min_data - h_2, mSettings.mTmin);
         const double t_max = std::min(t_max_data + h_2, mSettings.mTmax);
        // Loop
         std::vector<int> NiTot (gridLength);
   try {
        int idxGridMin, idxGridMax;

        for (const auto& t : concaTrace) {

            idxGridMin = inRange(0, (int) ceil((t - h_2 - t_min) / delta_t), (int)gridLength-1) ;
            idxGridMax = inRange(0, (int) ceil((t + h_2 - t_min) / delta_t), (int)gridLength-1) ;

            if (idxGridMax == idxGridMin) {
                ++*(NiTot.begin()+idxGridMin);
            } else {
                for (auto&& ni = NiTot.begin() + idxGridMin; ni != NiTot.begin() + idxGridMax; ++ni) {
                    ++*ni ;
                }
            }
        }

       } catch (std::exception& e) {
        qWarning()<< "Model::generateActivity exception caught: " << e.what() << '\n';

       } catch(...) {
        qWarning() << "Model::generateActivity Caught Exception!\n";

       }
       ///# Calculation of the mean and variance
        QVector<double> inf;
        QVector<double> sup;
        QVector<double> esp;
        double fA, eA, QSup, QInf;
        double UnifScore = 0.;
        double t;
        int nbIt = 0;

        for (const auto& ni : NiTot) {

            fA = ni / nr;
            eA =  fA * n / h;
            esp.append(eA);

            QSup = interpolate_value_from_curve(fA, Gx, 0, 1.)* n / h;
            sup.append(QSup);

            QInf = findOnOppositeCurve(fA, Gx)* n / h;
            inf.append(QInf);

#ifdef DEBUG
            if (QSup < QInf) {
                qDebug()<<"generateActivity() QSup<QInf f="<<fA<< " QSup="<<QSup<<" QInf="<<QInf;
            }
#endif

            t = t_min + nbIt * delta_t;
            if ((t_min_R <= t) && (t <= t_max_R)) {
                UnifScore += (std::max(ActivityUnif, QInf) - std::min(ActivityUnif, QSup))/gridLength;
            }
            nbIt++;
        }

        phase->mActivityValueStack["Significance Score"] = TValueStack("Significance Score", UnifScore);

        phase->mRawActivity = vector_to_map(esp, t_min, t_max, delta_t);
        phase->mRawActivityInf = vector_to_map(inf, t_min, t_max, delta_t);
        phase->mRawActivitySup = vector_to_map(sup, t_min, t_max, delta_t);

        if ( t_max <= mSettings.mTmax) {
            phase->mRawActivity.last() = 0.;
            phase->mRawActivityInf.last() = 0.;
            phase->mRawActivitySup.last() = 0.;
        }

        if (t_min >= mSettings.mTmin) {
            phase->mRawActivity.first() = 0.;
            phase->mRawActivityInf.first() = 0.;
            phase->mRawActivitySup.first() = 0.;
        }

        phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);
        phase->mActivityInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityInf);
        phase->mActivitySup = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivitySup);

        phase->mActivityUnifMean = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifMean);
        /*phase->mActivityUnifInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifInf);
        phase->mActivityUnifSup = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifSup);
*/

     } // Loop End on phase

#ifdef DEBUG
    qDebug() <<  QString("=> Model::generateActivity() done in " + DHMS(tClock.elapsed()));

#endif

}

void generateBufferForHisto(double *input, const std::vector<double> &dataSrc, const int numPts, const double a, const double b)
{
    // Work with "double" precision here !
    // Otherwise, "denum" can be very large and lead to infinity contribs!

    const double delta = (b - a) / (numPts - 1);

    const double denum = dataSrc.size();

    for (int i=0; i<numPts; ++i)
        input[i]= 0.;

    std::vector<double>::const_iterator iter = dataSrc.cbegin();
    for (; iter != dataSrc.cend(); ++iter) {
        const double t = *iter;

        const double idx = (t - a) / delta;
        const double idx_under = floor(idx);
        const double idx_upper = idx_under + 1.;

        const double contrib_under = (idx_upper - idx) / denum;
        const double contrib_upper = (idx - idx_under) / denum;

        if (std::isinf(contrib_under) || std::isinf(contrib_upper))
            qDebug() << "FFT input : infinity contrib!";

        if (idx_under < 0 || idx_under >= numPts || idx_upper < 0 || idx_upper > numPts)
            qDebug() << "FFT input : Wrong index";

        if (idx_under < numPts)
            input[(int)idx_under] += contrib_under;

        if (idx_upper < numPts) // This is to handle the case when matching the last point index !
            input[(int)idx_upper] += contrib_upper;
    }

}

void Model::clearCredibilityAndHPD()
{
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        for (auto&& date : (*iterEvent)->mDates) {
            date.mTi.mHPD.clear();
            date.mTi.mCredibility = QPair<double, double>();
            date.mSigmaTi.mHPD.clear();
            date.mSigmaTi.mCredibility= QPair<double, double>();
        }
        (*iterEvent)->mTheta.mHPD.clear();
        (*iterEvent)->mTheta.mCredibility = QPair<double, double>();
        ++iterEvent;
    }
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
        (*iterPhase)->mAlpha.mHPD.clear();
        (*iterPhase)->mAlpha.mCredibility = QPair<double, double>();
        //(*iterPhase)->mAlpha.mThresholdOld = 0;

        (*iterPhase)->mBeta.mHPD.clear();
        (*iterPhase)->mBeta.mCredibility = QPair<double, double>();
        //(*iterPhase)->mBeta.mThresholdOld = 0;

        //(*iterPhase)->mTau.mHPD.clear();
       // (*iterPhase)->mTau.mCredibility = QPair<double, double>();

        (*iterPhase)->mDuration.mHPD.clear();
        (*iterPhase)->mDuration.mCredibility = QPair<double, double>();
        //(*iterPhase)->mDuration.mThresholdOld = 0;
        (*iterPhase)->mTimeRange = QPair<double, double>();
        ++iterPhase;
    }
}

void Model::clearTraces()
{
    for (const auto& ev : mEvents) {
        for (auto&& date : ev->mDates) {
            date.reset();
        }
        ev->reset();
    }

    for (const auto& ph : mPhases) {
        ph->mAlpha.reset();
        ph->mBeta.reset();
        //ph->mTau.reset();
        ph->mDuration.reset();

        ph->mRawTempo.clear();
        ph->mRawTempoInf.clear();
        ph->mRawTempoSup.clear();

        ph->mRawActivity.clear();
        ph->mRawActivityInf.clear();
        ph->mRawActivitySup.clear();

        ph->mRawActivityUnifMean.clear();
        /*ph->mRawActivityUnifInf.clear();
        ph->mRawActivityUnifSup.clear();
        */
    }
}


 // Date files read / write
/** @Brief Save .res file, the result of computation and compress it
 *
 * */
void Model::saveToFile(QDataStream *out)
{

    out->setVersion(QDataStream::Qt_6_1);

    *out << quint32 (out->version());// we could add software version here << quint16(out.version());
    *out << qApp->applicationVersion();
    // -----------------------------------------------------
    //  Write info
    // -----------------------------------------------------
    *out << quint32 (mChains.size());
    for (ChainSpecs& ch : mChains) {
        *out << ch.burnElapsedTime;
        *out << ch.mAdaptElapsedTime;
        *out << ch.mAcquisitionElapsedTime;

        *out << quint32 (ch.mBatchIndex);
        *out << quint32 (ch.mBatchIterIndex);
        *out << quint32 (ch.mBurnIterIndex);
        *out << quint32 (ch.mMaxBatchs);
        *out << ch.mMixingLevel;
        *out << quint32 (ch.mIterPerBatch);
        *out << quint32 (ch.mIterPerBurn);
        *out << quint32 (ch.mIterPerAquisition);
        *out << quint32 (ch.mAquisitionIterIndex);
        *out << unsigned (ch.mSeed);
        *out << quint32 (ch.mThinningInterval);
        *out << quint32 (ch.mRealyAccepted);
        *out << quint32 (ch.mTotalIter);
    }
    // -----------------------------------------------------
    //  Write phases data
    // -----------------------------------------------------
    for (Phase*& phase : mPhases) {
        *out << phase->mAlpha;
        *out << phase->mBeta;
        *out << phase->mTau;
        *out << phase->mDuration;
    }
    // -----------------------------------------------------
    //  Write events data
    // -----------------------------------------------------
    for (Event*& event : mEvents)
        *out << event->mTheta;

    // -----------------------------------------------------
    //  Write dates data
    // -----------------------------------------------------
    for (Event*& event : mEvents) {
        if (event->mType == Event::eDefault ) {
            QList<Date> dates (event->mDates);
            for (auto&& d  : dates) {
                *out << d.mTi;
                *out << d.mSigmaTi;
                if (d.mDeltaType != Date::eDeltaNone)
                    *out << d.mWiggle;

                *out << d.mDeltaFixed;
                *out << d.mDeltaMin;
                *out << d.mDeltaMax;
                *out << d.mDeltaAverage;
                *out << d.mDeltaError;

                *out << qint32 (d.mSettings.mTmin);
                *out << qint32 (d.mSettings.mTmax);
                *out <<  d.mSettings.mStep;
                *out << quint8 (d.mSettings.mStepForced==true? 1: 0);


                *out << d.getTminRefCurve();
                *out << d.getTmaxRefCurve();

                //mCalibration and mWiggleCalibration are saved in to *.cal file

                *out << quint32 (d.mCalibHPD.size());
                for (QMap<double, double>::const_iterator it = d.mCalibHPD.cbegin(); it!=d.mCalibHPD.cend();++it) {
                    *out << it.key();
                    *out << it.value();
                }
            }

        }
    }
    *out << mLogModel;
    *out << mLogInit;
    *out << mLogAdapt;
    *out << mLogResults;

}
/** @Brief Read the .res file, it's the result of the saved computation
 *
 * */
void Model::restoreFromFile(QDataStream *in)
{
/*    QFile fileDat(fileName);
    fileDat.open(QIODevice::ReadOnly);
    QByteArray compressedData (fileDat.readAll());
    fileDat.close();

    QByteArray uncompressedData (qUncompress(compressedData));
#ifdef DEBUG
       qDebug() << "Lecture fichier :"<< fileName;
       qDebug() << "TAILLE compressedData :" << compressedData.size();
       qDebug() << "TAILLE uncompresedData :" << uncompressedData.size();
#endif
    compressedData.clear();
*/
/*    QFileInfo info(fileName);
    QFile file(info.path() + info.baseName() + ".~dat"); // when we could compress the file

    file.open(QIODevice::WriteOnly);
    file.write(uncompressedData);
    file.close();
*/
   // QFileInfo info(fileName);
   // QFile file(info.path() + info.baseName() + ".res");

   // QFile file(in);
  //  if (file.exists() && file.open(QIODevice::ReadOnly)){

    //    if ( file.size()!=0 /* uncompressedData.size()!=0*/ ) {
 //           QDataStream in(&uncompressedData, QIODevice::ReadOnly);
   // QDataStream in(&file);

    int QDataStreamVersion;
    *in >> QDataStreamVersion;
    in->setVersion(QDataStreamVersion);

    if (in->version()!= QDataStream::Qt_6_1)
            return;

    QString appliVersion;
    *in >> appliVersion;
    // prepare the future
    //QStringList projectVersionList = appliVersion.split(".");
#ifdef DEBUG
    if (appliVersion != qApp->applicationVersion())
        qDebug()<<" Different Model version ="<<appliVersion<<" actual ="<<qApp->applicationVersion();

#endif
    // -----------------------------------------------------
    //  Read info
    // -----------------------------------------------------

    quint32 tmp32;
    *in >> tmp32;

    mChains.clear();
    mChains.reserve(int (tmp32));
    for (quint32 i=0 ; i<tmp32; ++i) {
        ChainSpecs ch;
        *in >> ch.burnElapsedTime;
        *in >> ch.mAdaptElapsedTime;
        *in >> ch.mAcquisitionElapsedTime;

        *in >> ch.mBatchIndex;
        *in >> ch.mBatchIterIndex;
        *in >> ch.mBurnIterIndex;
        *in >> ch.mMaxBatchs;
        *in >> ch.mMixingLevel;
        *in >> ch.mIterPerBatch;
        *in >> ch.mIterPerBurn;
        *in >> ch.mIterPerAquisition;
        *in >> ch.mAquisitionIterIndex;
        *in >> ch.mSeed;
        *in >> ch.mThinningInterval;
        *in >> ch.mRealyAccepted;
        *in >> ch.mTotalIter;
        mChains.append(ch);
    }

        // -----------------------------------------------------
        //  Read phases data
        // -----------------------------------------------------

        for (const auto& p : mPhases) {
               *in >> p->mAlpha;
               *in >> p->mBeta;
               *in >> p->mTau;
               *in >> p->mDuration;
            }
        // -----------------------------------------------------
        //  Read events data
        // -----------------------------------------------------

        for (const auto& e:mEvents)
            *in >> e->mTheta;

        // -----------------------------------------------------
        //  Read dates data
        // -----------------------------------------------------

        for (const auto& event : mEvents) {
            if (event->mType == Event::eDefault )
                 for (auto&& d : event->mDates) {
                    *in >> d.mTi;
                    *in >> d.mSigmaTi;
                    if (d.mDeltaType != Date::eDeltaNone)
                       *in >> d.mWiggle;

                    *in >> d.mDeltaFixed;
                    *in >> d.mDeltaMin;
                    *in >> d.mDeltaMax;
                    *in >> d.mDeltaAverage;
                    *in >> d.mDeltaError;
                    qint32 tmpInt32;
                    *in >> tmpInt32;
                    d.mSettings.mTmin = int (tmpInt32);
                    *in >> tmpInt32;
                    d.mSettings.mTmax = int (tmpInt32);
                    *in >> d.mSettings.mStep;
                    quint8 btmp;
                   * in >> btmp;
                    d.mSettings.mStepForced =(btmp==1);

                   // in >> d.mSubDates;
                    double tmp;
                    *in >> tmp;
                    d.setTminRefCurve(tmp);
                    *in >> tmp;
                    d.setTmaxRefCurve(tmp);

                    /* Check if the Calibration Curve exist*/

         //           QMap<QString, CalibrationCurve>::const_iterator it = mProject->mCalibCurves.find (toFind);

                    // if no curve Create a new instance in mProject->mCalibration
       //             if ( it == mProject->mCalibCurves.end())
       //                 mProject->mCalibCurves.insert(toFind, CalibrationCurve());

                    //mProject->mCalibCurves.insert(toFind, CalibrationCurve());
       //     qDebug()<<"Model:restoreFromFile insert a new mCalibration "<<toFind;

                    d.mCalibration = & (mProject->mCalibCurves[d.mUUID]);

                    quint32 tmpUint32;
                    *in >> tmpUint32;
                    double tmpKey;
                    double tmpValue;
                    for (quint32 i= 0; i<tmpUint32; i++) {
                       *in >> tmpKey;
                       *in >> tmpValue;
                        d.mCalibHPD[tmpKey]= tmpValue;
                    }
#ifdef DEBUG

                     const QString toFind ("WID::"+ d.mUUID);

                     if (d.mWiggleCalibration==nullptr || d.mWiggleCalibration->mCurve.isEmpty()) {
                         qDebug()<<"Model::restoreFromFile mWiggleCalibration vide";

                     } else {
                         d.mWiggleCalibration = & (mProject->mCalibCurves[toFind]);
                     }
#endif
                }
         }
        *in >> mLogModel;
        *in >> mLogInit;
        *in >> mLogAdapt;
        *in >> mLogResults;

        //generateCorrelations(mChains);
       // generatePosteriorDensities(mChains, 1024, 1);
       // generateNumericalResults(mChains);

        // -----------------------------------------------------
        //  Read curve data
        // -----------------------------------------------------

    //    file.close();
       // file.remove(); // delete the temporary file with uncompressed data
  //  }
}

bool Model::hasSelectedEvents()
{
   return std::any_of(mEvents.begin(), mEvents.end(), [](Event * e){return e->mIsSelected;});
}

bool Model::hasSelectedPhases()
{
    return std::any_of(mPhases.begin(), mPhases.end(), [](Phase * p){return p->mIsSelected;});
}

void Model::reduceEventsTheta(QList<Event *> &lEvent)
{
    for (const auto& e : lEvent)
        e->mTheta.mX = reduceTime( e->mTheta.mX );
}

long double Model::reduceTime(double t)
{
    const double tmin = mSettings.mTmin;
    const double tmax = mSettings.mTmax;
    return (long double) (t - tmin) / (tmax - tmin);
}

long double Model::yearTime(double reduceTime)
{
    const double tmin = mSettings.mTmin;
    const double tmax = mSettings.mTmax;
    return (long double)  reduceTime * (tmax - tmin) + tmin ;
}

QString Model::initializeTheta()
{
        return QString();
}
