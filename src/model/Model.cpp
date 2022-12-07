/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

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
#include "CalibrationCurve.h"
#include "Date.h"
#include "Project.h"
#include "Bound.h"
//#include "MCMCLoopChrono.h"
//#include "MCMCProgressDialog.h"

#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "StdUtilities.h"
#include "DateUtils.h"
//#include "MainWindow.h"
//#include "PluginAbstract.h"
#include "MetropolisVariable.h"

#include <QJsonArray>
#include <QtWidgets>
#include <QtCore/QStringList>
//#include <execution>
#include <thread>


#if USE_FFT
//#include "fftw3.h"
#endif

// Constructor...
Model::Model(QObject *parent):
    QObject(parent),
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
    for (Event* &ev: mEvents) {
        // Event can be an Event or an EventCurve.
        // => do not delete it using ~Event(), because the appropriate destructor could be ~EventCurve().
        delete ev;
        ev = nullptr;
    }
    
    for (EventConstraint* &ec : mEventConstraints) {
        delete ec;
        ec = nullptr;
    }
    
    mEvents.clear();
    mEventConstraints.clear();

    if (!mPhases.isEmpty()) {
        for (Phase* &ph: mPhases) {
            if (ph) {
                delete ph;
                ph = nullptr;
            }
        }
        mPhases.clear();
    }

    if (!mPhaseConstraints.isEmpty()) {
        for (PhaseConstraint* &pc : mPhaseConstraints) {
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

        phase->mActivityUnifTheo = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifTheo);

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

  /*      for (int i=0; i<phases.size(); ++i) {
            const QJsonObject JSONphase = phases.at(i).toObject();
            Phase* p = new Phase(Phase::fromJson(JSONphase));
            p->mModel = this;
            mPhases.append(p);
            p = nullptr;
        }*/

        for (auto &json : phases)
             mPhases.append(new Phase(json.toObject(), this));

    }

    // Sort phases based on items y position
    std::sort(mPhases.begin(), mPhases.end(), sortPhases);

    if (json.contains(STATE_EVENTS)) {
        QJsonArray events = json.value(STATE_EVENTS).toArray();
        mNumberOfEvents = events.size();

        for (int i = 0; i < events.size(); ++i) {
            const QJsonObject JSONevent = events.at(i).toObject();

            if (JSONevent.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) {
                try {
                  /*  Event* e = new Event(JSONevent, this);//Event::fromJson(event));
                   // e->copyFrom(Event::fromJson(event));
                    e->mMixingLevel = mMCMCSettings.mMixingLevel;
                    mNumberOfDates += e->mDates.size();

                    for (auto&& d : e->mDates) {
                        d.mMixingLevel = e->mMixingLevel;
                        d.mColor = e->mColor;
                    }
                    mEvents.append(e);
                    e = nullptr;
                    */
                    mEvents.append(new Event(JSONevent, this));
                    mNumberOfDates += JSONevent.value(STATE_EVENT_DATES).toArray().size();

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
                Bound* ek = new Bound(JSONevent, this);
                //*ek = Bound::fromJson(JSONevent);
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

    const QJsonObject *state = mProject->state_ptr();
    const QJsonArray events = state->value(STATE_EVENTS).toArray();
    const QJsonArray phases = state->value(STATE_PHASES).toArray();

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
        log += "<br>";
        log += line(textBold(textOrange(QObject::tr("Duration (posterior distrib.)"))));
        log += line(textOrange(pPhase->mDuration.resultsString("<br>", QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"), nullptr, false)));

       /*  QString tempoStr = ModelUtilities::tempoResultsHTML(pPhase);
         tempoStr.remove(1, 41);
         log += tempoStr;*/
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
                Bound* bound = dynamic_cast<Bound*>(event);

                // --------------------
                // Check bound interval lower value
                // --------------------

                // On vérifie toutes les bornes avant et on prend le max
                // de leurs valeurs fixes ou du début de leur intervalle :
                double lower = double (mSettings.mTmin);
                for (auto k = 0; k<j; ++k) {
                    Event* evt = eventBranches[i][k];
                    if (evt->mType == Event::eBound) {
                        Bound* bd = dynamic_cast<Bound*>(evt);
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
                        Bound* bd = dynamic_cast<Bound*>(evt);
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
            Bound* bound = dynamic_cast<Bound*>(phaseFrom->mEvents[j]);
            if (bound)
                lower = qMax(lower, bound->mFixed);

        }
        double upper = double (mSettings.mTmax);
        Phase* phaseTo = mPhaseConstraints.at(i)->mPhaseTo;
        for (int j=0; j<phaseTo->mEvents.size(); ++j) {
            Bound* bound = dynamic_cast<Bound*>(phaseTo->mEvents[j]);
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
                    Bound* bound = dynamic_cast<Bound*>(mPhases.at(i)->mEvents[j]);
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
        //event->mTheta.generateCorrelations(chains);
        std::thread thTheta ([event] (QList<ChainSpecs> ch) {event->mTheta.generateCorrelations(ch);}, chains);

        for (auto&& date : event->mDates ) {
            date.mTi.generateCorrelations(chains);
            date.mSigmaTi.generateCorrelations(chains);
           /* std::thread thTi ([date] (QList<ChainSpecs> ch) {date.mTi.generateCorrelations(ch);}, chains);
            std::thread thSigmaTi ([*date] (QList<ChainSpecs> ch) {date.mSigmaTi.generateCorrelations(ch);}, chains);*/
        }
        thTheta.join();
    }

    for (const auto& phase : mPhases ) {
       /* phase->mAlpha.generateCorrelations(chains);
        phase->mBeta.generateCorrelations(chains);*/

        std::thread thAlpha ([phase] (QList<ChainSpecs> ch) {phase->mAlpha.generateCorrelations(ch);}, chains);
        std::thread thBeta ([phase] (QList<ChainSpecs> ch) {phase->mBeta.generateCorrelations(ch);}, chains);
        thAlpha.join();
        thBeta.join();

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
            generateActivity(mFFTLength, h, mThreshold);
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
        std::thread thCred ([this] (double threshold)
        {
            generateCredibility(threshold);
        } , threshold);

        std::thread thHPD ([this] (double threshold)
        {
            generateHPD(threshold);
        } , threshold);

        thCred.join();
        thHPD.join();
        generateActivity(mFFTLength, mHActivity, threshold);
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

    if (mProject->mLoop)
        emit mProject->mLoop->setMessage(tr("Computing posterior distributions and numerical results - Posterior Densities"));

    generatePosteriorDensities(mChains, fftLen, bandwidth);

    if (mProject->mLoop)
        emit mProject->mLoop->setMessage(tr("Computing posterior distributions and numerical results - Credibility "));
    generateCredibility(threshold);

    if (mProject->mLoop)
        emit mProject->mLoop->setMessage(tr("Computing posterior distributions and numerical results - HPD "));

    generateHPD(threshold);

    setThresholdToAllModel(threshold);

    if (!mPhases.isEmpty()) {
        if (mProject->mLoop)
            emit mProject->mLoop->setMessage(tr("Computing posterior distributions and numerical results - Tempo"));
         generateTempo(fftLen);
    }

    if (!mPhases.isEmpty()) {
        if (mProject->mLoop)
            emit mProject->mLoop->setMessage(tr("Computing posterior distributions and numerical results - Activity"));
         generateActivity(fftLen, mHActivity, threshold);
    }

    if (mProject->mLoop)
        emit mProject->mLoop->setMessage(tr("Computing posterior distributions and numerical results - Numerical Results"));
    generateNumericalResults(mChains);


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
    std::thread thEvents ([this] (QList<ChainSpecs> chains)
    {
        for (const auto& event : mEvents) {
            if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
                event->mTheta.generateNumericalResults(chains);

                for (auto&& date : event->mDates) {
                    date.mTi.generateNumericalResults(chains);
                    date.mSigmaTi.generateNumericalResults(chains);
                }
            }
        }
    } , chains);

    std::thread thPhases ([this] (QList<ChainSpecs> chains)
    {
        for (const auto& phase : mPhases) {
            phase->mAlpha.generateNumericalResults(chains);
            phase->mBeta.generateNumericalResults(chains);
            // phase->mTau.generateNumericalResults(chains);
            phase->mDuration.generateNumericalResults(chains);
        }
    } , chains);

    thEvents.join();
    thPhases.join();

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

#define USE_THREAD_CRED
#ifdef USE_THREAD_CRED
    std::thread thEvents ([this] (double thresh)
    {
        for (const auto& event : mEvents) {
             if (event->type() != Event::eBound) {
                event->mTheta.generateCredibility(mChains, thresh);

                for (auto&& date : event->mDates )  {
                    date.mTi.generateCredibility(mChains, thresh);
                    date.mSigmaTi.generateCredibility(mChains, thresh);
                }
            }

        }
    } , thresh);

    std::thread thPhases ([this] (double thresh)
    {
        for (const auto& phase :mPhases) {
            // if there is only one Event in the phase, there is no Duration
            phase->mAlpha.generateCredibility(mChains, thresh);
            phase->mBeta.generateCredibility(mChains, thresh);
            //  pPhase->mTau.generateCredibility(mChains, thresh);
            phase->mDuration.generateCredibility(mChains, thresh);
            phase->mTimeRange = timeRangeFromTraces( phase->mAlpha.fullRunRawTrace(mChains),
                                                     phase->mBeta.fullRunRawTrace(mChains), thresh, "Time Range for Phase : " + phase->mName);
        }
    } , thresh);

    std::thread thPhasesConst ([this] (double thresh)
    {
        for (const auto& phaseConstraint : mPhaseConstraints) {

            Phase* phaseFrom = phaseConstraint->mPhaseFrom;
            Phase* phaseTo  = phaseConstraint->mPhaseTo;

            phaseConstraint->mGapRange = gapRangeFromTraces(phaseFrom->mBeta.fullRunRawTrace(mChains),
                                                            phaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Gap Range : "+ phaseFrom->mName+ " to "+ phaseTo->mName);

            phaseConstraint->mTransitionRange = transitionRangeFromTraces(phaseFrom->mBeta.fullRunRawTrace(mChains),
                                                                          phaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Transition Range : "+ phaseFrom->mName+ " to "+ phaseTo->mName);

        }
    } , thresh);

    thEvents.join();
    thPhases.join();
    thPhasesConst.join();
#else
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

    for (const auto& phase :mPhases) {
        // if there is only one Event in the phase, there is no Duration
        phase->mAlpha.generateCredibility(mChains, thresh);
        phase->mBeta.generateCredibility(mChains, thresh);
        //  pPhase->mTau.generateCredibility(mChains, thresh);
        phase->mDuration.generateCredibility(mChains, thresh);
        phase->mTimeRange = timeRangeFromTraces( phase->mAlpha.fullRunRawTrace(mChains),
                                                  phase->mBeta.fullRunRawTrace(mChains), thresh, "Time Range for Phase : " + phase->mName);
    }

    for (const auto& phaseConstraint : mPhaseConstraints) {

        Phase* phaseFrom = phaseConstraint->mPhaseFrom;
        Phase* phaseTo  = phaseConstraint->mPhaseTo;

        phaseConstraint->mGapRange = gapRangeFromTraces(phaseFrom->mBeta.fullRunRawTrace(mChains),
                                                        phaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Gap Range : "+ phaseFrom->mName+ " to "+ phaseTo->mName);

        qDebug()<<"Gap Range "<<phaseFrom->mName<<" to "<<phaseTo->mName;

        phaseConstraint->mTransitionRange = transitionRangeFromTraces(phaseFrom->mBeta.fullRunRawTrace(mChains),
                                                                      phaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Transition Range : "+ phaseFrom->mName+ " to "+ phaseTo->mName);

    }
#endif

#ifdef DEBUG

    qDebug() <<  "[Model::generateCredibility] done in " + DHMS(t.elapsed());
#endif

}

void Model::generateHPD(const double thresh)
{
#ifdef DEBUG
    QElapsedTimer t;
    t.start();
#endif

    for (const auto& event : mEvents) {
        if (event->type() != Event::eBound || (event->mTheta.mSamplerProposal != MHVariable::eFixe)) {
            event->mTheta.generateHPD(thresh);

            for (int j = 0; j<event->mDates.size(); ++j) {
                Date& date = event->mDates[j];
                date.mTi.generateHPD(thresh);
                date.mSigmaTi.generateHPD(thresh);
            }
        }
    };

    for (const auto& ph : mPhases) {
        ph->mAlpha.generateHPD(thresh);
        ph->mBeta.generateHPD(thresh);
        ph->mDuration.generateHPD(thresh);

    };


#ifdef DEBUG
    qDebug() <<  "[Model::generateHPD] done in " + DHMS(t.elapsed()) ;
#endif

}

//#define UNIT_TEST
void Model::generateTempoAndActivity(size_t gridLength, double h, const double threshold)
{
#ifdef DEBUG
    QElapsedTimer tClock;
    tClock.start();
#endif

// Avoid to redo calculation, when mActivity exist, it happen when the control is changed
    int activityToDo = 0;
    int tempoToDo = 0;
    for (const auto& phase : mPhases) {
        if (phase->mRawActivity.isEmpty() || gridLength != mFFTLength || h != mHActivity || threshold != mThreshold)
            ++activityToDo;

        if (phase->mRawTempo.isEmpty() || gridLength != mFFTLength)
            ++tempoToDo;
    }

    if (activityToDo == 0 && tempoToDo == 0) {// no computation
        return;

    } else if (activityToDo > 0 && tempoToDo == 0) {
        generateActivity(gridLength, h, threshold);
        return;
    }

    for (const auto& phase : mPhases) {
        if (phase->mEvents.size() < 2) {
            phase->mValueStack["Significance Score"] = TValueStack("Significance Score", 0);
            phase->mValueStack["R_etendue"] = TValueStack("R_etendue", mSettings.mTmax - mSettings.mTmin);
            phase->mValueStack["t_min"] = TValueStack("t_min", mSettings.mTmin);
            phase->mValueStack["t_max"] = TValueStack("t_max", mSettings.mTmax);
            phase->mValueStack["a_Unif"] = TValueStack("a_Unif", mSettings.mTmin);
            phase->mValueStack["b_Unif"] = TValueStack("b_Unif", mSettings.mTmax);
            phase->mValueStack["min95"] = TValueStack("min95", mSettings.mTmin);
            phase->mValueStack["max95"] = TValueStack("max95", mSettings.mTmax);
            continue;
        }
        phase->mValueStack["threshold"] = TValueStack("threshold", threshold);
        // Curves for error binomial
        const int n = phase->mEvents.size();
        phase->mRawActivityUnifTheo.clear();

        if (!mBinomiale_Gx.contains(n) || threshold != mThreshold) {
            const std::vector<double> &Rq = binomialeCurveByLog(n, 1.- threshold/100.); //  Détermine la courbe x = r (q)
            mBinomiale_Gx[n] = inverseCurve(Rq); // Pour qActivity, détermine la courbe p = g (x)
        }

        const std::vector<double> &Gx = mBinomiale_Gx[n];

        //---- timeRange
        std::pair<double, double> timeRange = phase->mTimeRange;
        if (timeRange.first == timeRange.second) {
             timeRange = timeRangeFromTraces( phase->mAlpha.fullRunRawTrace(mChains),
                                             phase->mBeta.fullRunRawTrace(mChains), threshold, "Time Range for Phase : " + phase->mName);

        }

        double TR_min = timeRange.first;
        double TR_max = timeRange.second;
        std::vector<double> concaTrace;

        // Description des données
        std::vector<double> concaAllTrace;

        for (const auto& ev : phase->mEvents) {
            const auto &rawtrace = ev->mTheta.fullRunRawTrace(mChains);
            concaAllTrace.resize(concaAllTrace.size() + rawtrace.size());
            std::copy_backward( rawtrace.begin(), rawtrace.end(), concaAllTrace.end() );

            std::copy_if(rawtrace.begin(), rawtrace.end(),
                        std::back_inserter(concaTrace),
                        [TR_min, TR_max](double x) { return (TR_min<= x && x<= TR_max); });

        }

        auto minmaxAll = std::minmax_element(concaAllTrace.begin(), concaAllTrace.end());
        const double t_min_data = *minmaxAll.first;
        const double t_max_data = *minmaxAll.second;
        phase->mValueStack["t_min"] = TValueStack("t_min", t_min_data);
        phase->mValueStack["t_max"] = TValueStack("t_max", t_max_data);

        // ---
/*
        std::pair<double, double> credDuration;
        if (phase->mDuration.mThresholdUsed != threshold) {
            double exactCredibilityThreshold;
            credDuration = credibilityForTrace(phase->mDuration.fullRunTrace(mChains), threshold, exactCredibilityThreshold);

        } else {
            credDuration = phase->mDuration.mCredibility;
        }

        auto &d_min_creDuration = credDuration.first;
        auto &d_max_creDuration = credDuration.second;



        std::vector<double> concaTrace;


        // Creation de la trace concatenée, des scénari qui vérifie, intervalle de date "durée" comprise entre
        // un minimum (t_min_creDuration) et un maximum (t_max_creDuration)
        // Create concatened trace for Mean and Variance estimation needed for uniform predict
        // tableau de toutes les traces des Events
        std::list<QList<double>::Iterator> lRawEvents;
        auto durationRawTrace = phase->mDuration.fullRunRawTrace(mChains);
        auto durationIter = durationRawTrace.begin();

        int chainIdx = 0;

        for (auto chain : mChains) {
            int realyAccepted = chain.mRealyAccepted;
            lRawEvents.clear();
            for ( auto event : phase->mEvents) {
                lRawEvents.push_back(event->mTheta.findIter_element(0, mChains, chainIdx) );
            }

            for (int iter = 0; iter < realyAccepted; ++iter) {
                //test durée dans intevalle
                if (d_min_creDuration <= *durationIter && *durationIter <= d_max_creDuration) {
                    for (auto idxTh : lRawEvents)
                        concaTrace.push_back(*(idxTh + iter));
                }
                durationIter++;
            }
            chainIdx++;
        }
*/
        // Timerange interpolates between the points, so the min and max are different
        auto minmax95 = std::minmax_element(concaTrace.begin(), concaTrace.end());
        double min95 = *minmax95.first;
        double max95 = *minmax95.second;

       // qDebug()<<"[Model::generateTempoAndActivity] min95 ; max95 : " <<phase->mName<< min95 << max95;



        const double nr = concaTrace.size();

        if (min95 == max95) { // hapen when there is only one bound in the phase ???

           qDebug()<<"[Model::generateTempoAndActivity] tmin == tmax : " <<phase->mName;
            phase->mRawTempo[t_min_data] = 1;
            phase->mRawTempo[mSettings.mTmax] = 1;

            phase->mRawActivity[t_min_data] = 1;
            // Convertion in the good Date format

            phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);
            phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);

            phase->mValueStack["Significance Score"] = TValueStack("Significance Score", 0);
            phase->mValueStack["R_etendue"] = TValueStack("R_etendue", 0);
            phase->mValueStack["a_Unif"] = TValueStack("a_Unif", 0);
            phase->mValueStack["b_Unif"] = TValueStack("b_Unif", 0);
            phase->mValueStack["min95"] = TValueStack("min95", min95);
            phase->mValueStack["max95"] = TValueStack("max95", max95);
            continue;

        } else {
            phase->mValueStack["min95"] = TValueStack("min95", min95);
            phase->mValueStack["max95"] = TValueStack("max95", max95);
        }


       // const double mu = -2;
        //const double R_etendue = (n+1)/(n-1)/(1.+ mu*sqrt(2./(double)((n-1)*(n+2))) )*(t_max_data-t_min_data);
        const double gamma =  (n>=500 ? 1. : gammaActivity[(int)n]);

        const double R_etendue =  (max95 - min95)/gamma;

        // prevent h=0 and h >R_etendue;
        h = std::min( std::max(mSettings.mStep, h),  R_etendue) ;
        const double h_2 = h/2.;

        const double fUnif = h / R_etendue;

        const double mid_R =  (max95 + min95)/2.;

        const double ActivityUnif = fUnif * n / h; //  remplace -> fUnif * n / std::min(h, R_etendue);

        const double half_etendue = R_etendue/2. ; //h /2.;


        // Recentrage de a_Unif et b_Unif
        // Variable pour courbe Unif Théo

        const double a_Unif = mid_R - half_etendue;
        const double b_Unif = mid_R + half_etendue;

        // L'unif théorique est défini par le trapéze correspondant à l'unif modifié par la fenètre mobile
        const double a_Unif_minus_h_2 = a_Unif - h_2;
        const double a_Unif_plus_h_2 = a_Unif + h_2;

        const double b_Unif_minus_h_2 = b_Unif - h_2;
        const double b_Unif_plus_h_2 = b_Unif + h_2;

        phase->mValueStack["a_Unif"] = TValueStack("a_Unif", a_Unif);
        phase->mValueStack["b_Unif"] = TValueStack("b_Unif", b_Unif);

        if (a_Unif_minus_h_2 < mSettings.mTmin)
            phase->mRawActivityUnifTheo.insert(mSettings.mTmin,  interpolate(mSettings.mTmin, a_Unif_minus_h_2, a_Unif_plus_h_2, 0., ActivityUnif));
        else
            phase->mRawActivityUnifTheo.insert(a_Unif_minus_h_2,  0.);

        phase->mRawActivityUnifTheo.insert(a_Unif_plus_h_2,  ActivityUnif);
        phase->mRawActivityUnifTheo.insert(b_Unif_minus_h_2,  ActivityUnif);

        if (b_Unif_plus_h_2 > mSettings.mTmax)
            phase->mRawActivityUnifTheo.insert(mSettings.mTmax,  interpolate(mSettings.mTmax, b_Unif_minus_h_2, b_Unif_plus_h_2, ActivityUnif, 0.));
        else
            phase->mRawActivityUnifTheo.insert(b_Unif_plus_h_2,  0.);


#ifdef DEBUG
        if (t_max_data > mSettings.mTmax) {
            qWarning("[Model::generateTempoAndActivity] tmax>mSettings.mTmax force tmax = mSettings.mTmax");
            //t_max_data = mSettings.mTmax;
        }

#endif

        /// \f$ \delta_t_min = (t_max - t_min)/(gridLength-1) \f$
        const double delta_t_min = (max95 - min95) / double(gridLength-1);

        /// \f$ \delta_t = (max95 - min95 + h)/(gridLenth-1) \f$
        double delta_t = (max95 - min95 + h) / double(gridLength-1);
        if (h < delta_t_min) {
             h = delta_t_min;
             delta_t = delta_t_min;
         }

         // overlaps

         const double t_min = std::max(min95 - h_2, mSettings.mTmin);
         const double t_max = std::min(max95 + h_2, mSettings.mTmax);
        // Loop
         std::vector<int> niActivity (gridLength);
         std::vector<int> niTempo (gridLength);
         const int iMax = gridLength-1;
   try {
        int iActivityGridMin, iActivityGridMax;
        int iTempoMin;

        for (const auto& t : concaTrace) {
            iActivityGridMin = inRange(0, (int) ceil((t - min95) / delta_t), (int)gridLength-1) ;
            iActivityGridMax = inRange(0, (int) ceil((t + h - min95) / delta_t), (int)gridLength-1) ;

            if (iActivityGridMax == iActivityGridMin) {
                ++*(niActivity.begin()+iActivityGridMin);

            } else {
                for (auto&& ni = niActivity.begin() + iActivityGridMin; ni != niActivity.begin() + iActivityGridMax + 1; ++ni) {
                    ++*ni ;
                }
            }

            iTempoMin = std::min(std::max (0, (int) ceil((t - t_min) / delta_t)), iMax);
            ++*(niTempo.begin()+iTempoMin); // one item per grid

        }
        // calculation of distribution on niTempo
        std::transform (niTempo.begin()+1, niTempo.end(), niTempo.begin(), niTempo.begin()+1, std::plus<double>());

       } catch (std::exception& e) {
        qWarning()<< "[Model::generateTempoAndActivity] exception caught: " << e.what() << '\n';

       } catch(...) {
        qWarning() << "[Model::generateTempoAndActivity] Caught Exception!\n";

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
                qDebug()<<"[Model::generateTempoAndActivity() ]QSup < QInf f="<<fA<< " QSup="<<QSup<<" QInf="<<QInf;
            }
#endif

            t = min95 + h/2. + nbIt * delta_t;

            if ((a_Unif_minus_h_2 <= t) && (t < a_Unif_plus_h_2)) {
                /* Delta(h) = somme sur theta de ( max(Aunif - Ainf) - min(Aunif, Asup) ) / nbre de theta de la grille, nbre de pas de la grille
                 */
                //const double dUnif = ActivityUnif * (t - a_Unif_minus_h_2)/(a_Unif_plus_h_2 - a_Unif_minus_h_2);
                const double dUnif =  interpolateValueInQMap(t, phase->mRawActivityUnifTheo);
                UnifScore += (std::max(dUnif, QInf) - std::min(dUnif, QSup)) / gridLength;


            }
            else if ((a_Unif_plus_h_2 <= t) && (t <= b_Unif_minus_h_2)) {
                /* Delta(h) = somme sur theta de ( max(Aunif - Ainf) - min(Aunif, Asup) ) / nbre de theta de la grille, nbre de pas de la grille
                 */
                UnifScore += (std::max(ActivityUnif, QInf) - std::min(ActivityUnif, QSup)) / gridLength;


            } else if ((b_Unif_minus_h_2 < t) && (t <= b_Unif_plus_h_2)) {
                /* Delta(h) = somme sur theta de ( max(Aunif - Ainf) - min(Aunif, Asup) ) / nbre de theta de la grille, nbre de pas de la grille
                 */
               // const double dUnif = ActivityUnif * (b_Unif_plus_h_2 - t)/(b_Unif_plus_h_2 - b_Unif_minus_h_2);
                const double dUnif =  interpolateValueInQMap(t, phase->mRawActivityUnifTheo);
                UnifScore += (std::max(dUnif, QInf) - std::min(dUnif, QSup)) / gridLength;


            }


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

            // ----

            nbIt++;

        }

        phase->mValueStack["Significance Score"] = TValueStack("Significance Score", UnifScore);
        phase->mValueStack["R_etendue"] = TValueStack("R_etendue", R_etendue);

        phase->mRawActivity = vector_to_map(espA, min95 - h_2, max95 + h_2, delta_t);
        phase->mRawActivityInf = vector_to_map(infA, min95 - h_2, max95 + h_2, delta_t);
        phase->mRawActivitySup = vector_to_map(supA, min95 - h_2, max95 + h_2, delta_t);

        // Prolongation de l'enveloppe au deçà de t_min, jusqu'à a_Unif_minus_h_2

        const double t_min_display = std::max(a_Unif_minus_h_2, mSettings.mTmin);
        const double t_max_display = std::min(b_Unif_plus_h_2, mSettings.mTmax);
        phase->mRawActivitySup.insert(t_min_display, phase->mRawActivitySup.first());
        phase->mRawActivitySup.insert(t_max_display, phase->mRawActivitySup.last());

        phase->mRawActivityInf.insert(t_min_display, phase->mRawActivityInf.first());
        phase->mRawActivityInf.insert(t_max_display, phase->mRawActivityInf.last());

   /*     if ( t_max  <= mSettings.mTmax) {
            phase->mRawActivity.last() = 0.;
            phase->mRawActivityInf.last() = 0.;
            phase->mRawActivitySup.last() = 0.;
        }

        if (t_min >= mSettings.mTmin) {
            phase->mRawActivity.first() = 0.;
            phase->mRawActivityInf.first() = 0.;
            phase->mRawActivitySup.first() = 0.;

        }
   */

 /*       auto handle_mActivity = std::async(std::launch::async, DateUtils::convertMapToAppSettingsFormat, phase->mRawActivity);
        auto handle_mActivityInf = std::async(std::launch::async, DateUtils::convertMapToAppSettingsFormat, phase->mRawActivityInf);
        auto handle_mActivitySup = std::async(std::launch::async, DateUtils::convertMapToAppSettingsFormat, phase->mRawActivitySup);
        auto handle_mActivityUnifTheo = std::async(std::launch::async, DateUtils::convertMapToAppSettingsFormat, phase->mRawActivityUnifTheo);
*/
// Ici plus lent que les threads
        phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);
        phase->mActivityInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityInf);
        phase->mActivitySup = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivitySup);
        phase->mActivityUnifTheo = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifTheo);
//

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

        // On  laisse le temps de faire les tempo
 /*       phase->mActivity = handle_mActivity.get();
        phase->mActivityInf = handle_mActivityInf.get();
        phase->mActivitySup = handle_mActivitySup.get();
        phase->mActivityUnifTheo = handle_mActivityUnifTheo.get();
*/
     } // Loop End on phase

#ifdef DEBUG
    qDebug() <<  QString("[Model::generateTempoAndActivity()] done in " + DHMS(tClock.elapsed()));

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

// Avoid to redo calculation, when mActivity exist, it happen when the control is changed

    int tempoToDo = 0;
    for (const auto& phase : mPhases) {
      if (phase->mRawTempo.isEmpty() || gridLength != mFFTLength)
            ++tempoToDo;
    }

    for (const auto& phase : mPhases) {
        const int n = phase->mEvents.size();

        // Description des données
        std::vector<double> concaAllTrace;

        for (const auto& ev : phase->mEvents) {
            const auto &rawtrace = ev->mTheta.fullRunRawTrace(mChains);
            concaAllTrace.resize(concaAllTrace.size() + rawtrace.size());
            std::copy_backward( rawtrace.begin(), rawtrace.end(), concaAllTrace.end() );
        }

        auto minmaxAll = std::minmax_element(concaAllTrace.begin(), concaAllTrace.end());
        const double t_min_data = *minmaxAll.first;
        const double t_max_data = *minmaxAll.second;
        phase->mValueStack["t_min"] = TValueStack("t_min", t_min_data);
        phase->mValueStack["t_max"] = TValueStack("t_max", t_max_data);


        const double nr = concaAllTrace.size();



#ifdef DEBUG
        if (t_max_data > mSettings.mTmax) {
            qWarning("[Model::generateTempo] tmax>mSettings.mTmax force tmax = mSettings.mTmax");
            //t_max_data = mSettings.mTmax;
        }

#endif

        /// \f$ \delta_t = (t_max_data - t_min_data) / (gridLenth-1) \f$
        const double delta_t = (t_max_data - t_min_data) / double(gridLength-1);

        // Loop
         std::vector<int> niTempo (gridLength);
         const int iMax = gridLength-1;
   try {
        for (const auto& t : concaAllTrace) {

            const int iTempoMin = std::min(std::max (0, (int) ceil((t - t_min_data) / delta_t)), iMax);
            ++*(niTempo.begin() + iTempoMin); // one item per grid

        }
        // calculation of distribution on niTempo
        std::transform (niTempo.begin()+1, niTempo.end(), niTempo.begin(), niTempo.begin()+1, std::plus<double>());

       } catch (std::exception& e) {
        qWarning()<< "[Model::generateTempo] exception caught: " << e.what() << '\n';

       } catch(...) {
        qWarning() << "[Model::generateTempo] Caught Exception!\n";

       }

       ///# Calculation of the mean and variance

        // Variable for Tempo
        QVector<double> infT;
        QVector<double> supT;

        QVector<double> espT;
        double pT, eT, vT, infpT;

       // auto niT = niTempo.begin();

        //double t;
        for (const auto& niT : niTempo) {

            // Compute Tempo
            pT = niT/ nr;
            eT =  n * pT ;
            vT = n * pT * (1-pT);

            espT.append(eT);
            // Forbidden negative error
            infpT = ( eT < 1.96 * sqrt(vT) ? 0. : eT - 1.96 * sqrt(vT) );
            infT.append( infpT );
            supT.append( eT + 1.96 * sqrt(vT));

        }


        phase->mRawTempo = vector_to_map(espT, t_min_data, t_max_data, delta_t);
        phase->mRawTempoInf = vector_to_map(infT, t_min_data, t_max_data, delta_t);
        phase->mRawTempoSup = vector_to_map(supT, t_min_data, t_max_data, delta_t);

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
    qDebug() <<  QString("[Model::generateTempo] done in " + DHMS(tClock.elapsed()));

#endif

}

void Model::generateTempo_old(size_t gridLength)
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
void Model::generateActivity(const size_t gridLength, const double h, const double threshold)
{
    for (const auto& phase : mPhases) {
        // Curves for error binomial
        const int n = phase->mEvents.size();
        if (!mBinomiale_Gx.contains(n) || threshold != mThreshold) {
            const std::vector<double> Rq = binomialeCurveByLog(n, 1. - threshold/100.); //  Détermine la courbe x = r (q)
            mBinomiale_Gx[n] = inverseCurve(Rq); // Pour qActivity, détermine la courbe p = g (x)
        }
        phase->generateActivity(gridLength, h, threshold);
    }


}

void Model::generateActivity_old(size_t gridLength, double h, const double threshold)
{
#ifdef DEBUG
   // qDebug()<<"Model::generateActivity() "<<mSettings.mTmin<<mSettings.mTmax;
    QElapsedTimer tClock;
    tClock.start();
#endif

// Avoid to redo calculation, when mActivity exist, it happen when the control is changed
    int activityToDo = 0;
    for (const auto& phase : mPhases) {
        if (phase->mRawActivity.isEmpty() || gridLength != mFFTLength || h != mHActivity || threshold != mThreshold)
            ++activityToDo;
    }

    if (activityToDo == 0) // no computation
        return;


    /// We want an interval bigger than the maximun finded value, we need a point on tmin, tmax and tmax+deltat


    for (const auto& phase : mPhases) {
        phase->mValueStack["threshold"] = TValueStack("threshold", threshold);

        if (phase->mEvents.size() < 2) {
            phase->mValueStack["Significance Score"] = TValueStack("Significance Score", 0);
            phase->mValueStack["R_etendue"] = TValueStack("R_etendue", mSettings.mTmax - mSettings.mTmin);
            phase->mValueStack["t_min"] = TValueStack("t_min", mSettings.mTmin);
            phase->mValueStack["t_max"] = TValueStack("t_max", mSettings.mTmax);
            phase->mValueStack["a_Unif"] = TValueStack("a_Unif", mSettings.mTmin);
            phase->mValueStack["b_Unif"] = TValueStack("b_Unif", mSettings.mTmax);
            phase->mValueStack["min95"] = TValueStack("min95", mSettings.mTmin);
            phase->mValueStack["max95"] = TValueStack("max95", mSettings.mTmax);
            continue;
        }

        // Curves for error binomial
        const int n = phase->mEvents.size();
        phase->mRawActivityUnifTheo.clear();

        if (!mBinomiale_Gx.contains(n) || threshold != mThreshold) {
            const std::vector<double> Rq = binomialeCurveByLog(n, 1. - threshold/100.); //  Détermine la courbe x = r (q)
            mBinomiale_Gx[n] = inverseCurve(Rq); // Pour qActivity, détermine la courbe p = g (x)
        }
        const std::vector<double>& Gx = mBinomiale_Gx[n];

        //---- timeRange
        std::pair<double, double> timeRange = phase->mTimeRange;
        if (timeRange.first == timeRange.second) {
             timeRange = timeRangeFromTraces( phase->mAlpha.fullRunRawTrace(mChains),
                                             phase->mBeta.fullRunRawTrace(mChains), threshold, "Time Range for Phase : " + phase->mName);

        }

        double TR_min = timeRange.first;
        double TR_max = timeRange.second;
        std::vector<double> concaTrace;

        for (const auto& ev : phase->mEvents) {
            const auto &rawtrace = ev->mTheta.fullRunRawTrace(mChains);

            std::copy_if(rawtrace.begin(), rawtrace.end(),
                        std::back_inserter(concaTrace),
                        [TR_min, TR_max](double x) { return (TR_min<= x && x<= TR_max); });

        }


        auto minmax95 = std::minmax_element(concaTrace.begin(), concaTrace.end());
        double min95 = *minmax95.first;
        double max95 = *minmax95.second;

        if (min95 == max95) { // hapen when there is only one bound in the phase ???

           qDebug()<<"[Model::generateTempoAndActivity] tmin == tmax : " <<phase->mName;

            phase->mRawActivity[min95] = 1;
            // Convertion in the good Date format

            phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);
            phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);

            phase->mValueStack["Significance Score"] = TValueStack("Significance Score", 0);
            phase->mValueStack["R_etendue"] = TValueStack("R_etendue", 0);
            phase->mValueStack["a_Unif"] = TValueStack("a_Unif", 0);
            phase->mValueStack["b_Unif"] = TValueStack("b_Unif", 0);
            phase->mValueStack["min95"] = TValueStack("min95", min95);
            phase->mValueStack["max95"] = TValueStack("max95", max95);
            continue;

        } else {
            phase->mValueStack["min95"] = TValueStack("min95", min95);
            phase->mValueStack["max95"] = TValueStack("max95", max95);

        }
        const double nr = concaTrace.size();

        // const double mu = -2;
         //const double R_etendue = (n+1)/(n-1)/(1.+ mu*sqrt(2./(double)((n-1)*(n+2))) )*(t_max_data-t_min_data);
         const double gamma =  (n>=500 ? 1. : gammaActivity[(int)n]);

         const double R_etendue =  (max95 - min95)/gamma;

         // prevent h=0 and h >R_etendue;
         h = std::min( std::max(mSettings.mStep, h),  R_etendue) ;
         const double h_2 = h/2.;

         const double fUnif = h / R_etendue;

         const double mid_R =  (max95 + min95)/2.;

         const double ActivityUnif = fUnif * n / h; //  remplace -> fUnif * n / std::min(h, R_etendue);

         const double half_etendue = R_etendue/2. ;


         // Recentrage de a_Unif et b_Unif
         // Variable pour courbe Unif Théo

         const double a_Unif = mid_R - half_etendue;
         const double b_Unif = mid_R + half_etendue;

         // L'unif théorique est défini par le trapéze correspondant à l'unif modifié par la fenètre mobile
         const double a_Unif_minus_h_2 = a_Unif - h_2;
         const double a_Unif_plus_h_2 = a_Unif + h_2;

         const double b_Unif_minus_h_2 = b_Unif - h_2;
         const double b_Unif_plus_h_2 = b_Unif + h_2;

         phase->mValueStack["a_Unif"] = TValueStack("a_Unif", a_Unif);
         phase->mValueStack["b_Unif"] = TValueStack("b_Unif", b_Unif);
         phase->mValueStack["R_etendue"] = TValueStack("R_etendue", R_etendue);

         if (a_Unif_minus_h_2 < mSettings.mTmin)
             phase->mRawActivityUnifTheo.insert(mSettings.mTmin,  interpolate(mSettings.mTmin, a_Unif_minus_h_2, a_Unif_plus_h_2, 0., ActivityUnif));
         else
             phase->mRawActivityUnifTheo.insert(a_Unif_minus_h_2,  0.);

         phase->mRawActivityUnifTheo.insert(a_Unif_plus_h_2,  ActivityUnif);
         phase->mRawActivityUnifTheo.insert(b_Unif_minus_h_2,  ActivityUnif);

         if (b_Unif_plus_h_2 > mSettings.mTmax)
             phase->mRawActivityUnifTheo.insert(mSettings.mTmax,  interpolate(mSettings.mTmax, b_Unif_minus_h_2, b_Unif_plus_h_2, ActivityUnif, 0.));
         else
             phase->mRawActivityUnifTheo.insert(b_Unif_plus_h_2,  0.);

        /// Look for the maximum span containing values \f$ x=2 \f$

        if (min95 == max95) {
            qDebug()<<"Model::generateActivity() min95 == max95 : " <<phase->mName;
            phase->mRawActivity[min95] = 1;

            // Convertion in the good Date format
            phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);

            continue;
        }


#ifdef DEBUG
        if (max95 > mSettings.mTmax) {
            qWarning("Model::generateActivity() max95 > mSettings.mTmax force max95 = mSettings.mTmax");
          }

#endif

        const double t_min_grid = std::max(min95 - h_2, mSettings.mTmin);
        const double t_max_grid = std::min(max95 + h_2, mSettings.mTmax);

        /// \f$ \delta_t_min = (max95 - min95)/(gridLength-1) \f$
     /*   const double delta_t_min = (t_max_grid - t_min_grid) / double(gridLength-1);
        if (h < delta_t_min) {
             h = delta_t_min;
         }
         */
        /// \f$ \delta_t = (max95 - min95 + h)/(gridLenth-1) \f$
        const double delta_t = (t_max_grid - t_min_grid) / double(gridLength-1);


         // overlaps


        // Loop
         std::vector<int> NiTot (gridLength);
   try {
        for (const auto& t : concaTrace) {

            int idxGridMin = inRange(0, (int) ceil((t - t_min_grid - h_2) / delta_t), (int)gridLength-1) ;
            int idxGridMax = inRange(0, (int) ceil((t - t_min_grid + h_2) / delta_t), (int)gridLength-1) ;

            if (idxGridMax == idxGridMin) {
                ++*(NiTot.begin()+idxGridMin);
            } else {
                for (auto&& ni = NiTot.begin() + idxGridMin; ni != NiTot.begin() + idxGridMax + 1; ++ni) {
                    ++*ni ;
                }
            }
        }

       } catch (std::exception& e) {
        qWarning()<< "[Model::generateActivity] exception caught: " << e.what() << '\n';

       } catch(...) {
        qWarning() << "[Model::generateActivity] Caught Exception!\n";

       }



       ///# Calculation of the mean and variance
        QVector<double> inf;
        QVector<double> sup;
        QVector<double> esp;

        double UnifScore = 0.;
        int nbIt = 0;

        for (const auto& ni : NiTot) {

            const double fA = ni / nr;
            const double eA =  fA * n / h;
            esp.append(eA);

            const double QSup = interpolate_value_from_curve(fA, Gx, 0, 1.)* n / h;
            sup.append(QSup);

            const double QInf = findOnOppositeCurve(fA, Gx)* n / h;
            inf.append(QInf);

#ifdef DEBUG
            if (QSup < QInf) {
                qDebug()<<"[Model::generateActivity] QSup < QInf ; f= "<<fA<< " ; QSup = "<<QSup<<" ; QInf = "<<QInf;
            }
#endif
            // Calcul du score
            /* Delta(h) = somme sur theta de ( max(Aunif - Ainf) - min(Aunif, Asup) ) / nbre de theta de la grille, nbre de pas de la grille
             */
            // La grille est définie entre min95-h/2 et max95+h/2 avec gridlength case
            const double t = nbIt * delta_t + t_min_grid ;
            if ((a_Unif_minus_h_2 <= t) && (t < a_Unif_plus_h_2)) {
                const double dUnif =  interpolateValueInQMap(t, phase->mRawActivityUnifTheo);
                UnifScore += (std::max(dUnif, QInf) - std::min(dUnif, QSup))/gridLength;

            }
            else if ((a_Unif_plus_h_2 <= t) && (t <= b_Unif_minus_h_2)) {
                UnifScore += (std::max(ActivityUnif, QInf) - std::min(ActivityUnif, QSup))/gridLength;

            } else if ((b_Unif_minus_h_2 < t) && (t <= b_Unif_plus_h_2)) {
                const double dUnif =  interpolateValueInQMap(t, phase->mRawActivityUnifTheo);
                UnifScore += (std::max(dUnif, QInf) - std::min(dUnif, QSup)) / gridLength;
            }
            nbIt++;
        }

        phase->mValueStack["Significance Score"] = TValueStack("Significance Score", UnifScore);

        phase->mRawActivity = vector_to_map(esp, t_min_grid, t_max_grid, delta_t);
        phase->mRawActivityInf = vector_to_map(inf, t_min_grid, t_max_grid, delta_t);
        phase->mRawActivitySup = vector_to_map(sup, t_min_grid, t_max_grid, delta_t);

        // Prolongation de l'enveloppe au deçà de t_min, jusqu'à a_Unif_minus_h_2

        const double t_min_display = std::max(a_Unif_minus_h_2, mSettings.mTmin);
        const double t_max_display = std::min(b_Unif_plus_h_2, mSettings.mTmax);

        const double QSup = interpolate_value_from_curve(0., Gx, 0, 1.)* n / h;
        const double QInf = findOnOppositeCurve(0., Gx)* n / h;

        phase->mRawActivitySup.insert(t_min_grid, QSup );
        phase->mRawActivitySup.insert(t_max_grid, QSup );
        phase->mRawActivitySup.insert(t_min_display, QSup );
        phase->mRawActivitySup.insert(t_max_display, QSup );

        phase->mRawActivityInf.insert(t_min_grid, QInf);
        phase->mRawActivityInf.insert(t_max_grid, QInf);
        phase->mRawActivityInf.insert(t_min_display, QInf);
        phase->mRawActivityInf.insert(t_max_display, QInf);

        phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);
        phase->mActivityInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityInf);
        phase->mActivitySup = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivitySup);

        phase->mActivityUnifTheo = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivityUnifTheo);


     } // Loop End on phase

#ifdef DEBUG
    qDebug() <<  QString("[Model::generateActivity] done in " + DHMS(tClock.elapsed()));

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
            date.mTi.mCredibility = std::pair<double, double>();
            date.mSigmaTi.mHPD.clear();
            date.mSigmaTi.mCredibility= std::pair<double, double>();
        }
        (*iterEvent)->mTheta.mHPD.clear();
        (*iterEvent)->mTheta.mCredibility = std::pair<double, double>();
        ++iterEvent;
    }
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
        (*iterPhase)->mAlpha.mHPD.clear();
        (*iterPhase)->mAlpha.mCredibility = std::pair<double, double>();

        (*iterPhase)->mBeta.mHPD.clear();
        (*iterPhase)->mBeta.mCredibility = std::pair<double, double>();

        //(*iterPhase)->mTau.mHPD.clear();
       // (*iterPhase)->mTau.mCredibility = QPair<double, double>();

        (*iterPhase)->mDuration.mHPD.clear();
        (*iterPhase)->mDuration.mCredibility = std::pair<double, double>();
        //(*iterPhase)->mDuration.mThresholdOld = 0;
        (*iterPhase)->mTimeRange = std::pair<double, double>();
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

        ph->mRawActivityUnifTheo.clear();

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

              /* obsolete since 3.2.0
                *out << qint32 (d.mSettings.mTmin);
                *out << qint32 (d.mSettings.mTmax);
                *out <<  d.mSettings.mStep;
                *out << quint8 (d.mSettings.mStepForced==true? 1: 0);
*/

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

                    /* obsolete since 3.2.0
                    qint32 tmpInt32;
                    *in >> tmpInt32;
                    d.mSettings.mTmin = int (tmpInt32);
                    *in >> tmpInt32;
                    d.mSettings.mTmax = int (tmpInt32);
                    *in >> d.mSettings.mStep;
                    quint8 btmp;
                   * in >> btmp;
                    d.mSettings.mStepForced =(btmp==1);
                    */
                   // in >> d.mSubDates;
                    double tmp;
                    *in >> tmp;
                    d.setTminRefCurve(tmp);
                    *in >> tmp;
                    d.setTmaxRefCurve(tmp);

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
//#ifdef DEBUG

                     const QString toFind ("WID::"+ d.mUUID);

                     if (d.mWiggleCalibration == nullptr || d.mWiggleCalibration->mCurve.isEmpty()) {
                         qDebug()<<"[Model::restoreFromFile] mWiggleCalibration vide";
                         qDebug()<<"[Model::restoreFromFile] mWiggleCalibration vide";
                         qDebug()<<"[Model::restoreFromFile] mWiggleCalibration vide";

                     } else {
                         d.mWiggleCalibration = & (mProject->mCalibCurves[toFind]);
                     }
//#endif
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

t_reduceTime Model::reduceTime(double t) const
{
    const double tmin = mSettings.mTmin;
    const double tmax = mSettings.mTmax;
    return (t - tmin) / (tmax - tmin);
}

double Model::yearTime(t_reduceTime reduceTime)
{
    const double tmin = mSettings.mTmin;
    const double tmax = mSettings.mTmax;
    return reduceTime * (tmax - tmin) + tmin ;
}

QString Model::initializeTheta()
{
        return QString();
}
