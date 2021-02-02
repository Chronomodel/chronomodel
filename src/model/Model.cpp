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
#include "EventKnown.h"
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

// Constructor...
Model::Model():
mProject(nullptr),
mNumberOfPhases(0),
mNumberOfEvents(0),
mNumberOfDates(0),
mFFTLength(1024),
mBandwidth(1.06),
mThreshold(95.0)
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
    for (Event* ev: mEvents) {
        // Event can be an Event or an EventChronocurve.
        // => do not delete it using ~Event(), because the appropriate destructor could be ~EventChronocurve().
        delete ev;
        ev = nullptr;
    }
    
    for (EventConstraint* ec : mEventConstraints) {
        delete ec;
        ec = nullptr;
    }
    
    mEvents.clear();
    mEventConstraints.clear();

    if (!mPhases.isEmpty()) {
        for (Phase* ph: mPhases) {
            if (ph) {
                ph->~Phase();
                ph = nullptr;
            }
        }
        mPhases.clear();
    }

    if (!mPhaseConstraints.isEmpty()) {
        for (PhaseConstraint* pc : mPhaseConstraints) {
            if (pc)
                pc->~PhaseConstraint();

        }
        mPhaseConstraints.clear();
    }

    mChains.clear();
    mLogModel.clear();
    mLogMCMC.clear();
    mLogResults.clear();
}

/**
 * @brief Model::updateFormatSettings, set all date format according to the Application Preference, date format
 * @param appSet
 */
void Model::updateFormatSettings()
{
    for (auto&& event : mEvents) {
        event->mTheta.setFormat(AppSettings::mFormatDate);

        for (auto && date : event->mDates) {
            date.mTheta.setFormat(AppSettings::mFormatDate);
            date.mSigma.setFormat(DateUtils::eNumeric);
            date.mWiggle.setFormat(AppSettings::mFormatDate);
        }

    }

    for (auto && phase : mPhases) {
        phase->mAlpha.setFormat(AppSettings::mFormatDate);
        phase->mBeta.setFormat(AppSettings::mFormatDate);
        phase->mDuration.setFormat(DateUtils::eNumeric);

        // update Tempo and activity curves
        phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);
        phase->mTempoInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoInf);
        phase->mTempoSup = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoSup);
        phase->mTempoCredibilityInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoCredibilityInf);
        phase->mTempoCredibilitySup = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoCredibilitySup);

        phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);

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

        for (int i=0; i<events.size(); ++i) {
            const QJsonObject event = events.at(i).toObject();

            if (event.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) {
                try {
                    Event* e = new Event();//Event::fromJson(event));
                    e->copyFrom(Event::fromJson(event));
                    e->mMixingLevel = mMCMCSettings.mMixingLevel;
                    mNumberOfDates += e->mDates.size();

                    for (int j=0; j<e->mDates.size(); ++j) {
                        e->mDates[j].mMixingLevel = e->mMixingLevel;
                        e->mDates[j].mColor = e->mColor;
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
                EventKnown* e = new EventKnown(EventKnown::fromJson(event));
                e->updateValues(mSettings.mTmin, mSettings.mTmax, mSettings.mStep);
                mEvents.append(e);
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
       // int mNumberOfEventsInAllPhases;
       // int mNumberOfDatesInAllPhases;
        //mNumberOfEventsInAllPhases += mPhases.at(i)->mEvents.size();
        //for (int k=0; k<mPhases.at(i)->mEvents.size();++k)
        //    mNumberOfDatesInAllPhases += mPhases.at(i)->mEvents.at(k)->mDates.size();

    }
    //return model;
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
    while (iterJSONEvent != events.constEnd())
    {
        const QJsonObject eventJSON = (*iterJSONEvent).toObject();
        const int eventId = eventJSON.value(STATE_ID).toInt();
        const QJsonArray dates = eventJSON.value(STATE_EVENT_DATES).toArray();

        QList<Event *>::iterator iterEvent = mEvents.begin();
        while (iterEvent != mEvents.cend())
        {
            if ((*iterEvent)->mId == eventId)
            {
                (*iterEvent)->mName  = eventJSON.value(STATE_NAME).toString();
                (*iterEvent)->mItemX = eventJSON.value(STATE_ITEM_X).toDouble();
                (*iterEvent)->mItemY = eventJSON.value(STATE_ITEM_Y).toDouble();
                (*iterEvent)->mIsSelected = eventJSON.value(STATE_IS_SELECTED).toBool();
                (*iterEvent)->mColor = QColor(eventJSON.value(STATE_COLOR_RED).toInt(),
                                              eventJSON.value(STATE_COLOR_GREEN).toInt(),
                                              eventJSON.value(STATE_COLOR_BLUE).toInt());
                
                for (int k=0; k<(*iterEvent)->mDates.size(); ++k)
                {
                    Date& d = (*iterEvent)->mDates[k];
                    for (auto &&dateVal : dates)
                    {
                        const QJsonObject date = dateVal.toObject();
                        const int dateId = date.value(STATE_ID).toInt();

                        if (dateId == d.mId)
                        {
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
    while(iterJSONPhase != phases.constEnd())
    {
        const QJsonObject phaseJSON = (*iterJSONPhase).toObject();
        const int phaseId = phaseJSON.value(STATE_ID).toInt();

        for(auto &&p : mPhases)
        {
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

    for ( auto &&p : mPhases ) {
        std::sort(p->mEvents.begin(), p->mEvents.end(), sortEvents);
    }
}

QJsonObject Model::toJson() const
{
    QJsonObject json;

    json["settings"] = mSettings.toJson();
    json["mcmc"] = mMCMCSettings.toJson();

    QJsonArray events;
    for (auto&& event : mEvents)
        events.append(event->toJson());

    json["events"] = events;

    QJsonArray phases;
    for (auto&& pPhase : mPhases)
        phases.append(pPhase->toJson());

    json["phases"] = phases;

    QJsonArray event_constraints;
    for (auto&& eventConstraint : mEventConstraints)
        event_constraints.append(eventConstraint->toJson());

    json["event_constraints"] = event_constraints;

    QJsonArray phase_constraints;

    for (auto&& pPhaseConstraint : mPhaseConstraints)
        phase_constraints.append(pPhaseConstraint->toJson());

    json["phase_constraints"] = phase_constraints;

    return json;
}

// Logs
QString Model::getMCMCLog() const{
    return mLogMCMC;
}

QString Model::getModelLog() const{
    return mLogModel;
}

/**
 * @brief Model::generateModelLog
 * @return Return a QString with the recall of the Model with the data MCMC Methode, and constraint
 */
void Model::generateModelLog()
{
    QString log;
    // Study period
    QLocale locale = QLocale();
    log += line(textBold(textBlack(tr("Prior Study Period : [ %1 : %2 ] %3").arg(locale.toString(mSettings.getTminFormated()), locale.toString(mSettings.getTmaxFormated()), DateUtils::getAppSettingsFormatStr() ))));
    log += "<br>";

    int i(0);
    for (auto&& pEvent : mEvents) {
        if (pEvent->type() == Event::eKnown) {
            log += line(textRed(tr("Bound ( %1 / %2 ) : %3 ( %4  phases, %5 const. back., %6 const.fwd.)").arg(QString::number(i+1), QString::number(mEvents.size()), pEvent->mName,
                                                                                                               QString::number(pEvent->mPhases.size()),
                                                                                                               QString::number(pEvent->mConstraintsBwd.size()),
                                                                                                               QString::number(pEvent->mConstraintsFwd.size()))));
        } else {
            log += line(textBlue(tr("Event ( %1 / %2 ) : %3 ( %4 data, %5 phases, %6 const. back., %7 const. fwd.)").arg(QString::number(i+1), QString::number(mEvents.size()), pEvent->mName,
                                                                                                                         QString::number(pEvent->mDates.size()),
                                                                                                                         QString::number(pEvent->mPhases.size()),
                                                                                                                         QString::number(pEvent->mConstraintsBwd.size()),
                                                                                                                         QString::number(pEvent->mConstraintsFwd.size()))
                                 + "<br>" + tr("- Method : %1").arg(ModelUtilities::getEventMethodText(pEvent->mMethod))));
        }

        int j(0);
        for (auto&& date : pEvent->mDates) {
            log += "<br>";
            log += line(textBlack(tr("Data ( %1 / %2 ) : %3").arg(QString::number(j+1), QString::number(pEvent->mDates.size()), date.mName)
                                  + "<br>" + tr("- Type : %1").arg(date.mPlugin->getName())
                                  + "<br>" + tr("- Method : %1").arg(ModelUtilities::getDataMethodText(date.mMethod))
                                  + "<br>" + tr("- Params : %1").arg(date.getDesc())));
            ++j;
        }
        log += "<hr>";
        log += "<br>";
        ++i;
    }

    i = 0;
    for (auto &&pPhase : mPhases) {
        log += line(textPurple(tr("Phase ( %1 / %2 ) : %3 ( %4 events, %5 const. back., %6 const. fwd.)").arg(QString::number(i+1), QString::number(mPhases.size()), pPhase->mName,
                                                                                                              QString::number(pPhase->mEvents.size()),
                                                                                                              QString::number(pPhase->mConstraintsBwd.size()),
                                                                                                              QString::number(pPhase->mConstraintsFwd.size()))
                               + "<br>" + tr("- Type : %1").arg(pPhase->getTauTypeText())));
        log += "<br>";

        for (auto &&pEvent : pPhase->mEvents)
            log += line(textBlue(tr("Event : %1").arg(pEvent->mName)));

        log += "<hr>";
        log += "<br>";
        ++i;
    }

   // i = 0;
    for (auto&& pPhaseConst : mPhaseConstraints) {
        log += "<hr>";
        log += line(textBold(textGreen( QObject::tr("Succession from %1 to %2").arg(pPhaseConst->mPhaseFrom->mName, pPhaseConst->mPhaseTo->mName))));

        switch(pPhaseConst->mGammaType) {
            case PhaseConstraint::eGammaFixed :
                log += line(textBold(textGreen( QObject::tr("Min Hiatus fixed = %1").arg(pPhaseConst->mGammaFixed))));
                break;
            case PhaseConstraint::eGammaUnknown :
                log += line(textBold(textGreen( QObject::tr("Min Hiatus unknown") )));
                break;
            case PhaseConstraint::eGammaRange : //no longer used
                 log += line(textBold(textGreen( QObject::tr("Min Hiatus between %1 and %2").arg(pPhaseConst->mGammaMin, pPhaseConst->mGammaMax))));
                break;
            default:
                log += "Hiatus undefined -> ERROR";
            break;
        }

        log += "<hr>";
        log += "<br>";
    }

    mLogModel = log;

}

QString Model::getResultsLog() const{
    return mLogResults;
}

void Model::generateResultsLog()
{
    QString log;

    for (auto &&pPhase : mPhases) {
        log += ModelUtilities::phaseResultsHTML(pPhase);
        /** @todo delete repeted word phase */
         QString tempoStr = ModelUtilities::tempoResultsHTML(pPhase);
         tempoStr.remove(1, 41);
         log += tempoStr;
        log += "<hr>";
    }

    for (auto &&pPhaseConstraint : mPhaseConstraints) {
        log += ModelUtilities::constraintResultsHTML(pPhaseConstraint);
        log += "<hr>";
    }
    for (auto &&pEvent : mEvents) {
        log += ModelUtilities::eventResultsHTML(pEvent, true, this);
        log += "<hr>";
    }

    mLogResults += log;
}

// Results CSV
QList<QStringList> Model::getStats(const QLocale locale, const int precision, const bool withDateFormat)
{
    (void) withDateFormat;
    QList<QStringList> rows;

    int maxHpd (0);

    // Phases

    for (auto& pPhase : mPhases) {
        QStringList l = pPhase->mAlpha.getResultsList(locale, precision);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(pPhase->mName + " Begin");
        rows << l;

        l = pPhase->mBeta.getResultsList(locale, precision);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(pPhase->mName + " End");
        rows << l;
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

            QStringList l = date.mTheta.getResultsList(locale, precision);
            maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
            l.prepend(date.mName);
            rows << l;
        }
    }

    // Headers
    QStringList list;
    list << "Item" << "MAP" << "Mean" << "Std dev" << "Q1" << "Q2" << "Q3" << "Credibility %" << "Credibility start" << "Credibility end";
    for (int i = 0; i < maxHpd; ++i) {
        list << "HPD" + QString::number(i + 1) + " %";
        list << "HPD" + QString::number(i + 1) + " start";
        list << "HPD" + QString::number(i + 1) + " end";
    }
    rows.prepend(list);

    return rows;
}

QList<QStringList> Model::getPhasesTraces(const QLocale locale, const bool withDateFormat)
{
    QList<QStringList> rows;

    int runSize (0);

    for (auto& ch :mChains)
        runSize += ch.mNumRunIter / ch.mThinningInterval;

    QStringList headers;
    headers << "iter";

    for (auto &&pPhase : mPhases)
        headers << pPhase->mName + " Begin" << pPhase->mName + " End";

    rows << headers;

    int shift (0);
    for (int i = 0; i < mChains.size(); ++i) {
        int burnAdaptSize = 1 + mChains.at(i).mNumBurnIter + (mChains.at(i).mBatchIndex * mChains.at(i).mNumBatchIter);
        int runSize = mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;

        for (int j = burnAdaptSize; j<burnAdaptSize + runSize; ++j) {
            QStringList l;
            l << QString::number(shift + j);
            for (auto& pPhase : mPhases) {
                double valueAlpha = pPhase->mAlpha.mRawTrace->at(shift + j);

                if (withDateFormat)
                    valueAlpha = DateUtils::convertToAppSettingsFormat(valueAlpha);
                l << locale.toString(valueAlpha, 'g', 15);

                double valueBeta = pPhase->mBeta.mRawTrace->at(shift + j);

                if (withDateFormat)
                    valueBeta = DateUtils::convertToAppSettingsFormat(valueBeta);

                l << locale.toString(valueBeta, 'g', 15);

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


    int runSize (0);

    for (auto& chain : mChains)
        runSize += chain.mNumRunIter / chain.mThinningInterval;

    QStringList headers;
    headers << "iter" << phase->mName + " Begin" << phase->mName + " End";
    for (auto& event : phase->mEvents)
        headers << event->mName;

    rows << headers;

    int shift (0);

    for (ChainSpecs& chain : mChains) {
        int burnAdaptSize = 1 + chain.mNumBurnIter + (chain.mBatchIndex * chain.mNumBatchIter);
        int runSize = chain.mNumRunIter / chain.mThinningInterval;

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

            for (auto& event : phase->mEvents) {
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

    int runSize = 0;
    for (auto& chain : mChains)
        runSize += chain.mNumRunIter / chain.mThinningInterval;


    QStringList headers;
    headers << "iter";
    for (auto& event : mEvents)
        headers << event->mName;

    rows << headers;

    int shift (0);
    for (auto& chain : mChains)  {
        const int burnAdaptSize = 1+ chain.mNumBurnIter + (chain.mBatchIndex * chain.mNumBatchIter);
        const int runSize = chain.mNumRunIter / chain.mThinningInterval;

        for (int j = burnAdaptSize; j < burnAdaptSize + runSize; ++j) {
            QStringList l;
            l << QString::number(shift + j) ;

            for (auto& event : mEvents) {
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
    // 1 - At least one event is required in a model
    if (mEvents.isEmpty())
        throw QObject::tr("At least one event is required");

    // 2 - The event must contain at least 1 data
    for (int i = 0; i < mEvents.size(); ++i) {
        if (mEvents.at(i)->type() == Event::eDefault) {
            if (mEvents.at(i)->mDates.size() == 0)
                throw QObject::tr("The event  \" %1 \" must contain at least 1 data").arg(mEvents.at(i)->mName);
        }
    }

    // 3 - The phase must contain at least 1 event
    for (int i=0; i<mPhases.size(); ++i) {
        if (mPhases.at(i)->mEvents.size() == 0)
            throw QObject::tr("The phase \" %1 \" must contain at least 1 event").arg(mPhases.at(i)->mName);
    }

    // 4 - Pas de circularité sur les contraintes de faits
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

    // 7 - Un fait ne paut pas appartenir à 2 phases en contrainte
    for (int i = 0; i < phaseBranches.size(); ++i) {
        QVector<Event*> branchEvents;
        for (int j = 0; j < phaseBranches.at(i).size(); ++j) {
            Phase* phase = phaseBranches[i][j];
             for (auto&& pEvent : phase->mEvents) {
                if (!branchEvents.contains(pEvent)) {
                    branchEvents.append(pEvent);
                } else
                    throw QString(tr("The event \" %1 \" cannot belong to several phases in a same branch!").arg(pEvent->mName));
            }
        }
    }

    // 8 - Bounds : verifier cohérence des bornes en fonction des contraintes de faits (page 2)
    //  => Modifier les bornes des intervalles des bounds !! (juste dans le modèle servant pour le calcul)
    for (int i=0; i<eventBranches.size(); ++i) {
        for (int j=0; j<eventBranches.at(i).size(); ++j) {
            Event* event = eventBranches[i][j];
            if (event->type() == Event::eKnown)  {
                EventKnown* bound = dynamic_cast<EventKnown*>(event);

                // --------------------
                // Check bound interval lower value
                // --------------------

                // On vérifie toutes les bornes avant et on prend le max
                // de leurs valeurs fixes ou du début de leur intervalle :
                double lower = double (mSettings.mTmin);
                for (int k(0); k<j; ++k) {
                    Event* evt = eventBranches[i][k];
                    if (evt->mType == Event::eKnown) {
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
                for (int k=j+1; k<eventBranches.at(i).size(); ++k) {
                    Event* evt = eventBranches[i][k];
                    if (evt->mType == Event::eKnown) {
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
    for (int i=0; i<mPhaseConstraints.size(); ++i) {
        double gammaMin = 0.;
        PhaseConstraint::GammaType gType = mPhaseConstraints.at(i)->mGammaType;
        if (gType == PhaseConstraint::eGammaFixed)
            gammaMin = mPhaseConstraints[i]->mGammaFixed;

        else if (gType == PhaseConstraint::eGammaRange)
            gammaMin = mPhaseConstraints.at(i)->mGammaMin;

        double lower = mSettings.mTmin;
        Phase* phaseFrom = mPhaseConstraints.at(i)->mPhaseFrom;
        for (int j=0; j<phaseFrom->mEvents.size(); ++j) {
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

    for (int i=0; i<mPhases.size(); ++i) {
        if (mPhases.at(i)->mTauType != Phase::eTauUnknown) {
            double tauMax = mPhases.at(i)->mTauFixed;
            if (mPhases.at(i)->mTauType == Phase::eTauRange)
                tauMax = mPhases.at(i)->mTauMax;

            double min = mSettings.mTmin;
            double max = mSettings.mTmax;
            bool boundFound = false;

            for (int j=0; j<mPhases.at(i)->mEvents.size(); ++j) {
                if (mPhases.at(i)->mEvents.at(j)->mType == Event::eKnown) {
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
    for (int i=0; i<phaseBranches.size(); ++i) {
        for (int j=0; j<phaseBranches.at(i).size(); ++j) {
            Phase* phase = phaseBranches[i][j];
            for (int k=0; k<phase->mEvents.size(); ++k) {
                Event* event = phase->mEvents[k];

                bool phaseFound = false;

                // On réinspecte toutes les phases de la branche et on vérifie que le fait n'a pas de contrainte en contradiction avec les contraintes de phase !
                for (int l=0; l<phaseBranches.at(i).size(); ++l) {
                    Phase* p = phaseBranches[i][l];
                    if (p == phase)
                        phaseFound = true;

                    else {
                        for (int m=0; m<p->mEvents.size(); ++m) {
                            Event* e = p->mEvents[m];

                            // Si on regarde l'élément d'un phase d'avant, le fait ne peut pas être en contrainte vers un fait de cette phase
                            if (!phaseFound) {
                                for (int n=0; n<e->mConstraintsBwd.size(); ++n) {
                                    if (e->mConstraintsBwd[n]->mEventFrom == event)
                                        throw tr("The event %1  (in phase %2 ) is before the event %3 (in phase %4), BUT the phase %5 is after the phase %6 .\r=> Contradiction !").arg(event->mName, phase->mName, e->mName, p->mName, phase->mName, p->mName) ;

                                }
                            } else {
                                for (int n=0; n<e->mConstraintsFwd.size(); ++n) {
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

#ifndef UNIT_TEST
    // we can't use a progressBar here becasue ther is the finalyze bar on the top, and it's bug
    // Display a progressBar if "long" set with setMinimumDuration()
 /*   QProgressDialog *progress = new QProgressDialog(tr("Correlation generation"), tr("Wait") , 1, 10);
    progress->setWindowModality(Qt::NonModal);
    progress->setCancelButton(0);
    progress->setMinimumDuration(4);
    progress->setMinimum(0);

    int sum (0);
    for (auto && event : mEvents )
        sum += (event->mDates.size() + 1);

    progress->setMaximum(mPhases.size() + sum);
    int position(0);*/
#endif


    for (auto&& event : mEvents ) {
        event->mTheta.generateCorrelations(chains);
#ifndef UNIT_TEST
        //progress->setValue(++position);
#endif
        for (auto&& date : event->mDates ) {
            date.mTheta.generateCorrelations(chains);
            date.mSigma.generateCorrelations(chains);

#ifndef UNIT_TEST
            //progress->setValue(++position);
#endif
        }
    }

    for (auto&& phase : mPhases ) {
        phase->mAlpha.generateCorrelations(chains);
        phase->mBeta.generateCorrelations(chains);

#ifndef UNIT_TEST
    //    progress->setValue(++position);
#endif
    }

#ifndef UNIT_TEST
//    progress->~QProgressDialog();
#endif

#ifdef DEBUG
    //QTime t2 = QTime::currentTime();
    QTime timeDiff(0,0,0, (int)t.elapsed());
   // timeDiff = timeDiff.addMSecs(t.elapsed()).addMSecs(-1);
//    qint64 timeDiff = t.msecsTo(t2);

    qDebug() <<  QString("=> Model::generateCorrelations done in  %1 h %2 m %3 s %4 ms").arg(QString::number(timeDiff.hour()),
                                                                QString::number(timeDiff.minute()),
                                                                QString::number(timeDiff.second()),
                                                                QString::number(timeDiff.msec()) );
#endif
}

#pragma mark FFTLength, Threshold, bandwidth

void Model::setBandwidth(const double bandwidth)
{
    if (mBandwidth != bandwidth)
    {
        mBandwidth = bandwidth;

        clearPosteriorDensities();
        generatePosteriorDensities(mChains, mFFTLength, mBandwidth);
        generateHPD(mThreshold);
        generateNumericalResults(mChains);
        
        emit newCalculus();
    }
}

void Model::setFFTLength(const int FFTLength)
{
    if (mFFTLength != FFTLength) {
        mFFTLength = FFTLength;

        clearPosteriorDensities();
        generatePosteriorDensities(mChains, mFFTLength, mBandwidth);
        generateHPD(mThreshold);
        generateNumericalResults(mChains);

        emit newCalculus();
    }
}

/**
 * @brief Model::setThreshold this is a slot
 * @param threshold
 */
void Model::setThreshold(const double threshold)
{
    if (threshold != mThreshold) {
        mThreshold = threshold;
        
        generateCredibility(mThreshold);
        generateHPD(mThreshold);
        setThresholdToAllModel();
        
        emit newCalculus();
    }
}

void Model::setThresholdToAllModel()
{
    for (auto&& pEvent : mEvents) {
        if (pEvent->type() != Event::eKnown){
          pEvent->mTheta.mThresholdUsed = mThreshold;

          for (auto&& date : pEvent->mDates ) {
                date.mTheta.mThresholdUsed = mThreshold;
                date.mSigma.mThresholdUsed = mThreshold;
            }
        }
    }
    for (auto && pPhase :mPhases) {
       pPhase->mAlpha.mThresholdUsed = mThreshold;
       pPhase->mBeta.mThresholdUsed = mThreshold;
       pPhase->mDuration.mThresholdUsed = mThreshold;
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
    std::for_each(mEvents.begin(), mEvents.end(), [](Event* ev) {
        ev->mNodeInitialized = false;
        ev->mThetaNode = HUGE_VAL;
    });
}

/**
 * @brief Make all densities, credibilities and time range
 */
void Model::initDensities()
{
    // memo the new value of the Threshold inside all the part of the model: phases, events and dates
    clearPosteriorDensities();
    generatePosteriorDensities(mChains, mFFTLength, mBandwidth);
    generateHPD(mThreshold);

    generateCredibility(mThreshold);
    generateNumericalResults(mChains);
    
    if (!mPhases.isEmpty()) {
        generateTempo();
    }
}

void Model::updateDensities()
{
    clearPosteriorDensities();
    generatePosteriorDensities(mChains, mFFTLength, mBandwidth);
    setThresholdToAllModel();
    generateCredibility(mThreshold);
    generateHPD(mThreshold);

    if (!mPhases.isEmpty()) {
        generateTempo();

    }
    generateNumericalResults(mChains);
}

void Model::generatePosteriorDensities(const QList<ChainSpecs> &chains, int fftLen, double bandwidth)
{
#ifdef DEBUG
    QTime t = QTime::currentTime();
#endif
    const double tmin = mSettings.getTminFormated();
    const double tmax = mSettings.getTmaxFormated();

    for (auto&& event : mEvents) {
        event->mTheta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);

        for (auto&& d : event->mDates)
            d.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
    }

    for (auto && phase : mPhases)
        phase->generateHistos(chains, fftLen, bandwidth, tmin, tmax);
#ifdef DEBUG
    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generatePosteriorDensities done in " + QString::number(timeDiff) + " ms";
#endif
}

void Model::generateNumericalResults(const QList<ChainSpecs> &chains)
{
#ifdef DEBUG
    QTime t = QTime::currentTime();
#endif

    for (auto && event : mEvents) {
        event->mTheta.generateNumericalResults(chains);

        for (auto&& date : event->mDates) {
            date.mTheta.generateNumericalResults(chains);
            date.mSigma.generateNumericalResults(chains);
        }

    }

    for (auto && phase : mPhases) {
        phase->mAlpha.generateNumericalResults(chains);
        phase->mBeta.generateNumericalResults(chains);
        phase->mDuration.generateNumericalResults(chains);
    }

#ifdef DEBUG
    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generateNumericalResults done in " + QString::number(timeDiff) + " ms";
#endif
}

void Model::clearThreshold()
{
    mThreshold = -1.;

    for (auto && event : mEvents) {
        event->mTheta.mThresholdUsed = -1.;

        for (auto && date : event->mDates) {
            date.mTheta.mThresholdUsed = -1.;
            date.mSigma.mThresholdUsed = -1.;
        }
    }

    for (auto&& phase : mPhases) {
        phase->mAlpha.mThresholdUsed = -1.;
        phase->mBeta.mThresholdUsed = -1.;
        phase->mDuration.mThresholdUsed = -1.;
    }
}

void Model::generateCredibility(const double thresh)
{
#ifdef DEBUG
    qDebug()<<QString("Model::generateCredibility( %1 %)").arg(thresh);
    QTime t = QTime::currentTime();
#endif
    for (auto && pEvent : mEvents) {
        bool isFixedBound = false;
        if (pEvent->type() == Event::eKnown)
                isFixedBound = true;

        if (!isFixedBound) {
            pEvent->mTheta.generateCredibility(mChains, thresh);

            for (auto && date : pEvent->mDates )  {
                date.mTheta.generateCredibility(mChains, thresh);
                date.mSigma.generateCredibility(mChains, thresh);
            }
        }

    }

    // Diplay a progressBar if "long" set with setMinimumDuration()

    QProgressDialog *progress = new QProgressDialog(tr("Time range & credibilities generation"), tr("Wait") , 1, 10);
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(nullptr);
    progress->setMinimumDuration(5);
    progress->setMinimum(0);
    progress->setMaximum(mPhases.size()*4);
    //progress->setMinimumWidth(7 * AppSettings::widthUnit());
    progress->setMinimumWidth(int (progress->fontMetrics().boundingRect(progress->labelText()).width() * 1.5));


    int position(0);

    for (auto && pPhase :mPhases) {

        // if there is only one Event in the phase, there is no Duration
        pPhase->mAlpha.generateCredibility(mChains, thresh);
        progress->setValue(++position);

        pPhase->mBeta.generateCredibility(mChains, thresh);
        progress->setValue(++position);

        pPhase->mDuration.generateCredibility(mChains, thresh);
        progress->setValue(++position);

        pPhase->mTimeRange = timeRangeFromTraces(pPhase->mAlpha.fullRunRawTrace(mChains),
                                                             pPhase->mBeta.fullRunRawTrace(mChains),thresh, "Time Range for Phase : "+pPhase->mName);

        progress->setValue(++position);

    }
    progress->hide();
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

    position = 0;
    for (auto && phaseConstraint : mPhaseConstraints) {

        Phase* phaseFrom = phaseConstraint->mPhaseFrom;
        Phase* phaseTo  = phaseConstraint->mPhaseTo;

        phaseConstraint->mGapRange = gapRangeFromTraces(phaseFrom->mBeta.fullRunRawTrace(mChains),
                                                             phaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Gap Range : "+phaseFrom->mName+ " to "+ phaseTo->mName);

        qDebug()<<"Gap Range "<<phaseFrom->mName<<" to "<<phaseTo->mName;
        progressGap->setValue(++position);

        phaseConstraint->mTransitionRange = transitionRangeFromTraces(phaseFrom->mBeta.fullRunRawTrace(mChains),
                                                             phaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Transition Range : "+phaseFrom->mName+ " to "+ phaseTo->mName);

        qDebug()<<"Transition Range "<<phaseFrom->mName<<" to "<<phaseTo->mName;
        progressGap->setValue(++position);

    }
    delete progressGap;
#ifdef DEBUG
    QTime t2 (QTime::currentTime());
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generateCredibility done in " + QString::number(timeDiff) + " ms";
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

        if ((*iterEvent)->type() == Event::eKnown) {
            //EventKnown* ek = dynamic_cast<EventKnown*>(*iterEvent);
            //if(ek->knownType() == EventKnown::eFixed)
                isFixedBound = true;
        }

        if (!isFixedBound) {
            (*iterEvent)->mTheta.generateHPD(thresh);

            for (int j=0; j<(*iterEvent)->mDates.size(); ++j) {
                Date& date = (*iterEvent)->mDates[j];
                date.mTheta.generateHPD(thresh);
                date.mSigma.generateHPD(thresh);
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
    //QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.elapsed();
    qDebug() <<  "=> Model::generateHPD done in " + QString::number(timeDiff) + " ms";
#endif

}

//#define UNIT_TEST
/**
 * @brief Model::generateTempo
 * The function check if the table mTempo exist. In this case, there is no calcul.
 * This calculus must be in date, not in age
 */
void Model::generateTempo()
{
#ifdef DEBUG
    qDebug()<<"Model::generateTempo()"<<mSettings.mTmin<<mSettings.mTmax;
    QElapsedTimer t;
    t.start();
#endif

// Avoid to redo calculation, when mTempo exist, it happen when the control is changed
    int tempoToDo (0);
    for (auto &&phase : mPhases) {
        if (phase->mRawTempo.isEmpty())
            ++tempoToDo;
    }

    if(tempoToDo == 0) // no computation
        return;


#ifndef UNIT_TEST
    // Display a progressBar if "long" set with setMinimumDuration()
    QProgressDialog *progress = new QProgressDialog(tr("Tempo Plot generation"), tr("Wait") , 1, 10);//, qApp->activeWindow(), Qt::Window);
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(nullptr);
    progress->setMinimumDuration(5);
    progress->setMinimum(0);
    progress->setMaximum(mPhases.size() * 4);

    progress->setMinimumWidth(int (progress->fontMetrics().boundingRect(progress->labelText()).width() * 1.5));
    progress->show();
    int position(0);
#endif

    double tmin (mSettings.mTmin);
    double tmax (mSettings.mTmax);

#ifdef UNIT_TEST
    const int nbStep (20);
#else
    const int nbStep (1000); ///&< Number of point for the calulation table, if the size is too important, there is problem with the memory place
#endif

    /// We want an interval bigger than the maximun finded value, we need a point on tmin, tmax and tmax+deltat
    const int nbPts (nbStep + 1);

    const int totalIter = int (std::ceil(mChains[0].mNumRunIter / mChains[0].mThinningInterval));

    // create Empty containers
    QVector<int> N ;  // Tempo
    N.resize(nbPts);

    QVector<int> N2 ;
    N2.resize(nbPts);

    QVector<int> activity ; //mActivity
    activity.resize(nbPts);

    QVector<int> previousN ;
    previousN.resize(nbPts);

    QVector<int> previousN2 ;
    previousN2.resize(nbPts);

    QMap<double, QVector<int>> Ni; ///&< mTempoCredibility t, QVector(N)

    for (auto &&phase : mPhases) {
        // Avoid to redo calculation, when mTempo exist, it happen when the control is changed
        if (!phase->mRawTempo.isEmpty()) {
#ifndef UNIT_TEST
            position += 4;
            progress->setValue(position);
#endif
            continue;
        }

         ///# 1 - Generate Event scenario
         // We suppose, it is the same iteration number for all chains
        QList<QVector<double>> listTrace;
        for (auto &&ev : phase->mEvents)
            listTrace.append(ev->mTheta.fullRunRawTrace(mChains));


        /// Look for the maximum span containing values \f$ x=2 \f$
        tmin = mSettings.mTmax;
        tmax = mSettings.mTmin;

#ifndef UNIT_TEST
        for (auto &&l:listTrace) {
            const double lmin = *std::min_element(l.cbegin(), l.cend());
            tmin = std::min(tmin, lmin );
            const double lmax = *std::max_element(l.cbegin(), l.cend());
            tmax = std::max(tmax, lmax);
        }
        tmin = std::floor(tmin);
        tmax = std::ceil(tmax);
#endif
        if (tmin == tmax) {

           qDebug()<<"Model::generateTempo() tmin == tmax : " <<phase->mName;

            phase->mRawTempoCredibilityInf[tmax] = 1;

            phase->mRawTempo[tmax] = 1;
            phase->mRawTempoInf[tmax] = 1;
            phase->mRawTempoSup[tmax] = 1;

            phase->mRawTempo.insert(mSettings.mTmax, 1);

            phase->mRawTempoCredibilityInf[tmax] = 1;
            phase->mRawTempoCredibilitySup[tmax] = 1;

            phase->mRawActivity[tmin] = 1;

            // Convertion in the good Date format
            phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);
            phase->mTempoInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoInf);
            phase->mTempoSup = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoSup);
            phase->mTempoCredibilityInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoCredibilityInf);
            phase->mTempoCredibilitySup = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoCredibilitySup);

            phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);

#ifndef UNIT_TEST
            position += 4;
            progress->setValue(position);
#endif

            continue;
        }
#ifdef DEBUG
        if (tmax > mSettings.mTmax) {
            qWarning("Model::generateTempo() tmax>mSettings.mTmax force tmax = mSettings.mTmax");
            tmax = mSettings.mTmax;
        }
#endif

        /// \f$ \delta_t = (t_max - t_min)/(nbStep) \f$
        const double deltat = (tmax-tmin)/ double(nbStep);


        /// Erase containers
        N.fill(0);
        N2.fill(0);
#ifdef ACTIVITY // used if we want to compute with the some vs derivative function
        activity .fill(0); //mActivity
#endif

        previousN .fill(0);
        previousN2 .fill(0);

        Ni.clear();
        /// Loop
        for (int i(0); i<totalIter; ++i) {
            /// Create one scenario per iteration
            QVector<double> scenario;

            for (auto &&t : listTrace)
                scenario.append(t.at(i));

            /// Sort scenario trace
            std::sort(scenario.begin(),scenario.end());

            QVector<double>::const_iterator itScenario (scenario.begin());

            QVector<int>::iterator itN (N.begin()); // for Tempo
            QVector<int>::iterator itPrevN (previousN.begin());

            QVector<int>::iterator itN2 (N2.begin()); // for Tempo Error
            QVector<int>::iterator itPrevN2 (previousN2.begin());
#ifdef ACTIVITY
            QVector<int>::iterator itactivity (activity.begin()); // for Intensity/Activity
#endif
            int index (0); // index of table N corresponding to the first value // faster than used of Distance
            double t (tmin);
            int memoScenarioIdx (0);
            int scenarioIdx (1);

            while (itScenario != scenario.cend()) {

                if (*itScenario > t && t <= tmax ) {
                     if (itN != N.end()) {
                             (*itN2) = (*itPrevN2) + (memoScenarioIdx * memoScenarioIdx);

                            ++itN2;
                            ++itPrevN2;
                            (*itN) = (*itPrevN) + memoScenarioIdx;
                            ++itN;
                            ++itPrevN;
                     }
#ifdef ACTIVITY
                     if (itactivity!= activity.end())
                         ++itactivity;
#endif
                    Ni[t].append(memoScenarioIdx);

                    ++index;
                    t = tmin + index*deltat;

                } else {
                    (*itN2) = (*itPrevN2) + (scenarioIdx * scenarioIdx);

                    (*itN) = (*itPrevN) + scenarioIdx;
#ifdef ACTIVITY
                    (*itactivity) = (*itactivity) + 1;
#endif

                   memoScenarioIdx = scenarioIdx;

                   if (itScenario != scenario.cend()) {
                        ++itScenario;
                       ++scenarioIdx;
                    }
                }
            }

            while (index < nbPts) {
                if (itN != N.end()) {
                     (*itN) = (*itPrevN) + memoScenarioIdx;
                    ++itN;
                    ++itPrevN;
                }
                if (itN2 != N2.end()) {
                    (*itN2) = (*itPrevN2) + (memoScenarioIdx * memoScenarioIdx);
                    ++itN2;
                    ++itPrevN2;
                }

                Ni[t].append(memoScenarioIdx);
                ++index;
                t = tmin + index*deltat;

            }
            std::copy(N2.begin(), N2.end(), previousN2.begin());
            std::copy(N.begin(), N.end(), previousN.begin());
        }
        /// Loop End


    ///# Calculation of the variance
        QVector<double> inf;
        QVector<double> sup;
        QVector<double> mean;
        QVector<int>::iterator itN2 (N2.begin());
        const double di (totalIter);
        for (auto &&x : N) {

            const double m = x/di;
            const double s2 = std::sqrt( (*itN2)/di - std::pow(m, 2.) );

            mean.append(m);
            // Forbidden negative error
            const double infp = ( m < 1.96*s2 ? 0. : m - 1.96*s2 );
            inf.append( infp);
            sup.append( m + 1.96*s2);
            ++itN2;

        }
#ifndef UNIT_TEST
        ++position;
        progress->setValue(position);
#endif

       ///# 2 - Cumulate Nj and Nj2
        /*QMap<double, double> tempo;
        QMap<double, double> tempoInf;
        QMap<double, double> tempoSup;
*/
        phase->mRawTempo = vector_to_map(mean, tmin, tmax, deltat);
        phase->mRawTempoInf = vector_to_map(inf, tmin, tmax, deltat);
        phase->mRawTempoSup = vector_to_map(sup, tmin, tmax, deltat);

        // close the error curve on mean value
        const double tEnd (phase->mRawTempo.lastKey());
        const double vEnd (phase->mRawTempo[tEnd]);

        if ( (tEnd ) <= mSettings.mTmax) {
            phase->mRawTempoInf[tEnd] = vEnd;
            phase->mRawTempoSup[tEnd ] = vEnd;
        }
        phase->mRawTempo.insert(mSettings.mTmax, vEnd);

        const double tBegin (phase->mRawTempo.firstKey());
        const double vBegin (0.);
        // We need to add a point with the value 0 for the automatique Y scaling
        if ((tBegin - deltat) >= mSettings.mTmin) {
            phase->mRawTempo[tBegin - deltat] = vBegin;
            phase->mRawTempoInf[tBegin - deltat] = vBegin;
            phase->mRawTempoSup[tBegin - deltat] = vBegin;
        }

        // Convertion in the good Date format
        phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);
        phase->mTempoInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoInf);
        phase->mTempoSup = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoSup);

#ifndef UNIT_TEST
        ++position;
        progress->setValue(position);
#endif
#ifndef ACTIVITY
        ///# 3 - Derivative function
        /**  compute the slope of a nearby secant line through the points (x-h,f(x-h)) and (x+h,f(x+h)).
         *   https://en.wikipedia.org/wiki/Numerical_differentiation
         */

        QVector<double> dN;
        // first value, we can't use the same formula for the beginning because we don't have f(x-h)
        int Np (N[0]);
        int Nm (0);
        dN.append( (Np - Nm)/2. );
        Nm = Np;
        for (QVector<int>::const_iterator x = N.cbegin()+1;  x != N.cend()-1; ++x) {
            Np = (* (x+1));
            dN.append((Np - Nm )/2. );
            Nm = (*x);
        }
        //last value
        Np = N.last();
        dN.append( (Np - Nm )/2. );

        //QMap<double, double> activity;
        phase->mRawActivity = vector_to_map(dN, tmin, tmax, deltat);

        if ( tEnd  <= mSettings.mTmax)
            phase->mRawActivity[tEnd] = 0.;

        if ((tmin - deltat) >= mSettings.mTmin)
            phase->mRawActivity[tmin - deltat] = 0.;

        // Convertion in the good Date format
        phase->mActivity = DateUtils::convertMapToAppSettingsFormat(phase->mRawActivity);

 #else
       //phase->phase->mRawActivity = vector_to_map(activity, tmin, tmax, deltat);
       // Convertion in the good Date format
       phase->mActivity = DateUtils::convertMapToAppSettingsFormat(vector_to_map(phase->mRawActivity, tmin, tmax, deltat));

#endif
#ifndef UNIT_TEST
        ++position;
        progress->setValue(position);
#endif
        ///# 4 - Credibility
        QVector<double> credInf (nbPts);
        QVector<double> credSup (nbPts);

        QVector<double>::iterator itCredInf (credInf.begin());
        QVector<double>::iterator itCredSup (credSup.begin());
        int index (0); // index of table N corresponding to the first value // faster than used of Distance
        double t (tmin);
        while (t <= tmax ) {
            /* quartile for 95%,
             * 2.5% on inferior curve
             * and 2.5% on the superior curve */
            Quartiles cred = quartilesType(Ni[t], 8, 0.025);
            *itCredInf = cred.Q1;
            *itCredSup = cred.Q3;
            ++itCredInf;
            ++itCredSup;

            ++index;
            t = tmin + index*deltat;
        }
        phase->mRawTempoCredibilityInf = vector_to_map(credInf, tmin, tmax, deltat);
        phase->mRawTempoCredibilitySup = vector_to_map(credSup, tmin, tmax, deltat);

        // Joining curves on the curve Tempo
        if ( tmax  <= mSettings.mTmax) {
            phase->mRawTempoCredibilityInf[tmax] = vEnd;
            phase->mRawTempoCredibilitySup[tmax] = vEnd;
        }

        if ((tBegin - deltat) >= mSettings.mTmin) {
            phase->mRawTempoCredibilityInf[tBegin - deltat] = vBegin;
            phase->mRawTempoCredibilitySup[tBegin - deltat] = vBegin;
        }
        // Convertion in the good Date format
        phase->mTempoCredibilityInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoCredibilityInf);
        phase->mTempoCredibilitySup = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoCredibilitySup);

#ifndef UNIT_TEST
        ++position;
        progress->setValue(position);
#endif

    }

#ifndef UNIT_TEST
    progress->~QProgressDialog();
#endif

#ifdef DEBUG
    //QTime t2 = QTime::currentTime();
    //qint64 timeDiff = t.msecsTo(t2);

    QTime timeDiff(0, 0, 0, (int) t.elapsed());
    //timeDiff = timeDiff.addMSecs(t.elapsed()).addMSecs(-1);

    qDebug() <<  QString("=> Model::generateTempo() done in  %1 h %2 m %3 s %4 ms").arg(QString::number(timeDiff.hour()),
                                                                QString::number(timeDiff.minute()),
                                                                QString::number(timeDiff.second()),
                                                                QString::number(timeDiff.msec()) );
#endif


}


/**
 *  @brief Clear model data
 */
void Model::clearPosteriorDensities()
{
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.cend()) {
        for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
            Date& date = (*iterEvent)->mDates[j];
            date.mTheta.mHisto.clear();
            date.mSigma.mHisto.clear();
            date.mTheta.mChainsHistos.clear();
            date.mSigma.mChainsHistos.clear();
        }
        (*iterEvent)->mTheta.mHisto.clear();
        (*iterEvent)->mTheta.mChainsHistos.clear();

        ++iterEvent;
    }

    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.cend()) {
        (*iterPhase)->mAlpha.mHisto.clear();
        (*iterPhase)->mBeta.mHisto.clear();
        (*iterPhase)->mDuration.mHisto.clear();

        (*iterPhase)->mAlpha.mChainsHistos.clear();
        (*iterPhase)->mBeta.mChainsHistos.clear();
        (*iterPhase)->mDuration.mChainsHistos.clear();
        ++iterPhase;
    }
}

void Model::clearCredibilityAndHPD()
{
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.cend()) {
        foreach (Date date, (*iterEvent)->mDates) {
            date.mTheta.mHPD.clear();
            date.mTheta.mCredibility = QPair<double, double>();
            date.mSigma.mHPD.clear();
            date.mSigma.mCredibility= QPair<double, double>();
        }
        (*iterEvent)->mTheta.mHPD.clear();
        (*iterEvent)->mTheta.mCredibility = QPair<double, double>();
        ++iterEvent;
    }
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.cend()) {
        (*iterPhase)->mAlpha.mHPD.clear();
        (*iterPhase)->mAlpha.mCredibility = QPair<double, double>();
        //(*iterPhase)->mAlpha.mThresholdOld = 0;

        (*iterPhase)->mBeta.mHPD.clear();
        (*iterPhase)->mBeta.mCredibility = QPair<double, double>();
        //(*iterPhase)->mBeta.mThresholdOld = 0;

        (*iterPhase)->mDuration.mHPD.clear();
        (*iterPhase)->mDuration.mCredibility = QPair<double, double>();
        //(*iterPhase)->mDuration.mThresholdOld = 0;
        (*iterPhase)->mTimeRange = QPair<double, double>();
        ++iterPhase;
    }
}

void Model::clearTraces()
{
    for (auto ev : mEvents) {
        for (Date date : ev->mDates) {
            date.reset();
        }
        ev->reset();
    }

    for (auto ph : mPhases) {
        ph->mAlpha.reset();
        ph->mBeta.reset();
        ph->mDuration.reset();

        ph->mRawTempo.clear();
        ph->mRawTempoInf.clear();
        ph->mRawTempoSup.clear();
        ph->mRawTempoCredibilityInf.clear();
        ph->mRawTempoCredibilitySup.clear();
    }
}


 // Date files read / write
/** @Brief Save .res file, the result of computation and compress it
 *
 * */
void Model::saveToFile(const QString& fileName)
{
    if (!mEvents.empty()) {
    // -----------------------------------------------------
    //  Create file
    // -----------------------------------------------------
    //QFileInfo info(fileName);
   // QFile file(info.path() + info.baseName() + ".~res"); // when we could do a compressed file
    //QFile file(info.path() + info.baseName() + ".res");
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {

        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_5_5);

        int reserveInit = 0;
        // compute reserve to estim the size of uncompresedData and magnify
        foreach (Phase* phase, mPhases) {
            reserveInit += phase->mAlpha.mRawTrace->size();
            reserveInit += phase->mBeta.mRawTrace->size();
            reserveInit += phase->mDuration.mRawTrace->size();
        }

        int numDates = 0;
        foreach (Event* event, mEvents) {
            reserveInit += event->mTheta.mRawTrace->size();
            reserveInit += event->mTheta.mAllAccepts->size();
            reserveInit += event->mTheta.mHistoryAcceptRateMH->size();
            reserveInit += 4;

            QList<Date>& dates = event->mDates;
            numDates += event->mDates.size();
            for (int j=0; j<dates.size(); ++j) {
                reserveInit += dates.at(j).mTheta.mRawTrace->size();
                reserveInit += dates.at(j).mTheta.mAllAccepts->size();
                reserveInit += dates.at(j).mTheta.mHistoryAcceptRateMH->size();
                reserveInit += 4;

                reserveInit += dates.at(j).mSigma.mRawTrace->size();
                reserveInit += dates.at(j).mSigma.mAllAccepts->size();
                reserveInit += dates.at(j).mSigma.mHistoryAcceptRateMH->size();
                reserveInit += 4;

                if (dates.at(j).mWiggle.mRawTrace) {
                    reserveInit += dates.at(j).mWiggle.mRawTrace->size();
                    reserveInit += dates.at(j).mWiggle.mAllAccepts->size();
                    reserveInit += dates.at(j).mWiggle.mHistoryAcceptRateMH->size();
                    reserveInit += 4;
                }

             }
        }
        //uncompressedData.reserve(reserveInit);
     /*   QBuffer buffer(&uncompressedData);
            buffer.open(QIODevice::WriteOnly);

            QDataStream out(&buffer);*/

       // QDataStream out(&uncompressedData, QIODevice::WriteOnly);


        out << quint32 (out.version());// we could add software version here << quint16(out.version());
        out << qApp->applicationVersion();
        // -----------------------------------------------------
        //  Write info
        // -----------------------------------------------------
        out << quint32 (mPhases.size());
        out << quint32 (mEvents.size());
        out << quint32 (numDates);

        out << quint32 (mChains.size());
        for (ChainSpecs& ch : mChains) {
            out << quint32 (ch.mBatchIndex);
            out << quint32 (ch.mBatchIterIndex);
            out << quint32 (ch.mBurnIterIndex);
            out << quint32 (ch.mMaxBatchs);
            out << ch.mMixingLevel;
            out << quint32 (ch.mNumBatchIter);
            out << quint32 (ch.mNumBurnIter);
            out << quint32 (ch.mNumRunIter);
            out << quint32 (ch.mRunIterIndex);
            out << qint32 (ch.mSeed);
            out << quint32 (ch.mThinningInterval);
            out << quint32 (ch.mTotalIter);
        }
        // -----------------------------------------------------
        //  Write phases data
        // -----------------------------------------------------
        foreach (Phase* phase, mPhases) {
            out << phase->mAlpha;
            out << phase->mBeta;
            out << phase->mDuration;
        }
        // -----------------------------------------------------
        //  Write events data
        // -----------------------------------------------------
        foreach (Event* event, mEvents)
            out << event->mTheta;

        // -----------------------------------------------------
        //  Write dates data
        // -----------------------------------------------------
        for (const Event* event : mEvents) {
            if (event->mType == Event::eDefault ){
                QList<Date> dates (event->mDates);
                 for ( Date d : dates) {
                     out << d.mTheta;
                     out << d.mSigma;
                      if (d.mDeltaType != Date::eDeltaNone)
                          out << d.mWiggle;

                      out << d.mDeltaFixed;
                      out << d.mDeltaMin;
                      out << d.mDeltaMax;
                      out << d.mDeltaAverage;
                      out << d.mDeltaError;

                      out << qint32 (d.mSettings.mTmin);
                      out << qint32 (d.mSettings.mTmax);
                      out <<  d.mSettings.mStep;
                      out << quint8 (d.mSettings.mStepForced==true? 1: 0);


                      out << d.getTminRefCurve();
                      out << d.getTmaxRefCurve();

                      //mCalibration and mWiggleCalibration are saved in to *.cal file

                      out << quint32 (d.mCalibHPD.size());
                      for (QMap<double, double>::const_iterator it = d.mCalibHPD.cbegin(); it!=d.mCalibHPD.cend();++it) {
                          out << it.key();
                          out << it.value();
                       }
                    }

            }
        }
        out << mLogModel;
        out << mLogMCMC;
        out << mLogResults;

      //  QByteArray compressedData (uncompressedData);// (qCompress(uncompressedData, 2));
      //  file.write(compressedData);

      /*  QDataStream outComp(&file);
        outComp.setVersion(QDataStream::Qt_4_3);
        outComp << qCompress(uncompressedData, 2);*/

        //file.write(qCompress(uncompressedData, 2));
        file.close();

        // compress file
      //  QFile infile(fileName+"_tmp");

      /*     file.open(QIODevice::ReadOnly);

           QByteArray uncompressedData (file.readAll());
           file.close();
           file.remove();

          // QByteArray compressedData (qCompress(uncompressedData, 9));

           QFile fileDat(fileName);
           fileDat.open(QIODevice::WriteOnly);
           fileDat.write(qCompress(uncompressedData, 9));

           fileDat.close();
           uncompressedData.clear();
        */
    }
  }
}
/** @Brief Read the .res file, it's the result of the saved computation and uncompress it
 *
 * */
void Model::restoreFromFile(const QString& fileName)
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

    QFile file(fileName);
    if (file.exists() && file.open(QIODevice::ReadOnly)){

    //    if ( file.size()!=0 /* uncompressedData.size()!=0*/ ) {
 //           QDataStream in(&uncompressedData, QIODevice::ReadOnly);
    QDataStream in(&file);

    int QDataStreamVersion;
    in >> QDataStreamVersion;
    in.setVersion(QDataStreamVersion);

    if (in.version()!= QDataStream::Qt_5_5)
            return;

    QString appliVersion;
    in >> appliVersion;
    // prepare the future
    //QStringList projectVersionList = appliVersion.split(".");
    if (appliVersion != qApp->applicationVersion())
        qDebug()<<file.fileName()<<" different version="<<appliVersion<<" actual="<<qApp->applicationVersion();


    // -----------------------------------------------------
    //  Read info
    // -----------------------------------------------------

    quint32 tmp32;
    in >> tmp32;
    //const int numPhases = (int)tmp32;
    in >> tmp32;
    //const int numEvents = (int)tmp32;
    in >> tmp32;
    //const int numdates = (int)tmp32;

    in >> tmp32;
    mChains.clear();
    mChains.reserve(int (tmp32));
    for (quint32 i=0 ; i<tmp32; ++i) {
        ChainSpecs ch;
        in >> ch.mBatchIndex;
        in >> ch.mBatchIterIndex;
        in >> ch.mBurnIterIndex;
        in >> ch.mMaxBatchs;
        in >> ch.mMixingLevel;
        in >> ch.mNumBatchIter;
        in >> ch.mNumBurnIter;
        in >> ch.mNumRunIter;
        in >> ch.mRunIterIndex;
        in >> ch.mSeed;
        in >> ch.mThinningInterval;
        in >> ch.mTotalIter;
        mChains.append(ch);
    }

        // -----------------------------------------------------
        //  Read phases data
        // -----------------------------------------------------

        for (auto &&p : mPhases) {
               in >> p->mAlpha;
               in >> p->mBeta;
               in >> p->mDuration;
            }
        // -----------------------------------------------------
        //  Read events data
        // -----------------------------------------------------

        for (auto &&e:mEvents)
            in >> e->mTheta;

        // -----------------------------------------------------
        //  Read dates data
        // -----------------------------------------------------

        for (auto &&event : mEvents) {
            if (event->mType == Event::eDefault )
                 for (auto &&d : event->mDates) {
                    in >> d.mTheta;
                    in >> d.mSigma;
                    if (d.mDeltaType != Date::eDeltaNone)
                        in >> d.mWiggle;

                    in >> d.mDeltaFixed;
                    in >> d.mDeltaMin;
                    in >> d.mDeltaMax;
                    in >> d.mDeltaAverage;
                    in >> d.mDeltaError;
                    qint32 tmpInt32;
                    in >> tmpInt32;
                    d.mSettings.mTmin = int (tmpInt32);
                    in >> tmpInt32;
                    d.mSettings.mTmax = int (tmpInt32);
                    in >> d.mSettings.mStep;
                    quint8 btmp;
                    in >> btmp;
                    d.mSettings.mStepForced =(btmp==1);

                   // in >> d.mSubDates;
                    double tmp;
                    in >> tmp;
                    d.setTminRefCurve(tmp);
                    in >> tmp;
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
                    in >> tmpUint32;

                    for (quint32 i= 0; i<tmpUint32; i++) {
                        double tmpKey;
                        double tmpValue;
                        in >> tmpKey;
                        in >> tmpValue;
                        d.mCalibHPD[tmpKey]= tmpValue;
                    }
#ifdef DEBUG

                     const QString toFind ("WID::"+ d.mUUID);

                     if (d.mWiggleCalibration==nullptr || d.mWiggleCalibration->mCurve.isEmpty())
                         qDebug()<<"Model::restoreFromFile vide";
                     else {
                         d.mWiggleCalibration = & (mProject->mCalibCurves[toFind]);
                     }
#endif
                }
         }
        in >> mLogModel;
        in >> mLogMCMC;
        in >> mLogResults;

        generateCorrelations(mChains);
       // generatePosteriorDensities(mChains, 1024, 1);
       // generateNumericalResults(mChains);

        file.close();
       // file.remove(); // delete the temporary file with uncompressed data
    }

}

bool Model::hasSelectedEvents()
{
    for(auto && event : mEvents) {
        if (event->mIsSelected) {
            return true;
        }
    }
    return false;
}

bool Model::hasSelectedPhases()
{
    for(auto && phase : mPhases) {
        if (phase->mIsSelected) {
            return true;
        }
    }
    return false;
}

