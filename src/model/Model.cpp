/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "StateKeys.h"
#include "StdUtilities.h"
#include "DateUtils.h"

#include "MetropolisVariable.h"

#include <QJsonArray>
#include <QtWidgets>
#include <QtCore/QStringList>

#define NO_USE_THREAD
#ifdef USE_THREAD
#include <thread>
#endif

extern QString res_file_version;

Model::Model():
    mEvents(),
    mPhases(),
    mEventConstraints(),
    mPhaseConstraints(),

    mNumberOfPhases(0),
    mNumberOfEvents(0),
    mNumberOfDates(0),
    mThreshold(-1.),
    mBandwidth(0.9),
    mFFTLength(1024),
    mHActivity(1)
{
    
}

Model::Model(const QJsonObject& json):
    mEvents(),
    mPhases(),
    mEventConstraints(),
    mPhaseConstraints(),

    mNumberOfPhases(0),
    mNumberOfEvents(0),
    mNumberOfDates(0),
    mThreshold(-1.),
    mBandwidth(0.9),
    mFFTLength(1024),
    mHActivity(1)
{
    // same code as fromJSON
    if (json.contains(STATE_SETTINGS)) {
        const QJsonObject settings = json.value(STATE_SETTINGS).toObject();
        mSettings = StudyPeriodSettings::fromJson(settings);
    }

    if (json.contains(STATE_MCMC)) {
        const QJsonObject mcmc = json.value(STATE_MCMC).toObject();
        mMCMCSettings = MCMCSettings::fromJson(mcmc);
        mChains = mMCMCSettings.getChains();
    }

    if (json.contains(STATE_PHASES)) {
        const QJsonArray phases = json.value(STATE_PHASES).toArray();
        mNumberOfPhases = (int) phases.size();

        for (const auto json : phases) {
            const QJsonObject& phaseObj = json.toObject();
            const auto& ph = std::make_shared<Phase>(phaseObj);
            mPhases.push_back(ph);

        }
    }

    // Sort phases based on items y position
    std::sort(mPhases.begin(), mPhases.end(), sortPhases);

    if (json.contains(STATE_EVENTS)) {
        QJsonArray events = json.value(STATE_EVENTS).toArray();
        mNumberOfEvents = (int) events.size();

        for (const auto event : events) {
            const QJsonObject& eventObj = event.toObject();

            if (eventObj.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) {
                try {
                   const auto& ev = std::make_shared<Event>(eventObj);
                   mEvents.push_back(ev);
                  
                   mNumberOfDates += eventObj.value(STATE_EVENT_DATES).toArray().size();

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
                const auto& ek = std::make_shared<Bound>(eventObj);
                mEvents.push_back(ek);


            }
        }
    }


    // Sort events based on items y position
    std::sort(mEvents.begin(), mEvents.end(), sortEvents);

    if (json.contains(STATE_EVENTS_CONSTRAINTS)) {
        const QJsonArray constraints = json.value(STATE_EVENTS_CONSTRAINTS).toArray();
        for (qsizetype i=0; i<constraints.size(); ++i) {
            const QJsonObject &constraintObj = constraints.at(i).toObject();
            const auto& c = std::make_shared<EventConstraint>(constraintObj);
            mEventConstraints.push_back(c);

        }
    }

    if (json.contains(STATE_PHASES_CONSTRAINTS)) {
        const QJsonArray constraints = json.value(STATE_PHASES_CONSTRAINTS).toArray();
        for (qsizetype i=0; i<constraints.size(); ++i) {
            const QJsonObject& constraintObj = constraints.at(i).toObject();
            const auto& c = std::make_shared<PhaseConstraint>(constraintObj);
            mPhaseConstraints.push_back(c);

        }
    }

    // ------------------------------------------------------------
    //  Link objects to each other
    //  Must be done here !
    //  nb : Les data sont déjà linkées aux events à leur création
    // ------------------------------------------------------------


    for (auto &event : mEvents) {
        int eventId = event->mId;
        std::vector<int> phasesIds = event->mPhasesIds;

        // Link des events / phases
        for (auto &phase : mPhases) {
            const int phaseId = phase->mId;
            if (container_contains(phasesIds, phaseId)) {
                event->mPhases.push_back(phase);
                phase->mEvents.push_back(event);
            }
        }

        // Link des events / contraintes d'event
        for (auto &ec : mEventConstraints) {
            if (ec->mFromId == eventId) {
                ec->mEventFrom = event;
                event->mConstraintsFwd.push_back(ec);
                
            } else if (ec->mToId == eventId) {
                ec->mEventTo = event;
                event->mConstraintsBwd.push_back(ec);
            }
        }
    }

    // Link des phases / contraintes de phase

    for (auto &phase : mPhases) {
        const int phaseId = phase->mId;

        for (auto &pc : mPhaseConstraints) {
            if (pc->mFromId == phaseId) {
                pc->mPhaseFrom = phase;
                phase->mConstraintsNextPhases.push_back(pc);
                
            } else if (pc->mToId == phaseId) {
                pc->mPhaseTo = phase;
                phase->mConstraintsPrevPhases.push_back(pc);
            }
        }

    }
}

Model::~Model()
{
    //qDebug() << "[Model::~Model]";
}

void Model::clear()
{
    mCurveName.clear();
    mCurveLongName.clear();

    for (auto &ev: mEvents) {
        ev->clear();
    }
    mEvents.clear();
    mEventConstraints.clear();

    for (auto &ph: mPhases) {
        ph->clear();
    }
    mPhases.clear();
    mPhaseConstraints.clear();

    mChains.clear();
    mLogModel.clear();
    mLogInit.clear();
    mLogAdapt.clear();
    mLogResults.clear();
}

void Model::shrink_to_fit()
{
    mCurveName.shrink_to_fit();
    mCurveLongName.shrink_to_fit();

    for (auto &ev: mEvents) {
        for (auto &dat: ev->mDates) {
            dat.shrink_to_fit();
        }
        ev->shrink_to_fit();
    }
    mEvents.shrink_to_fit();

    mEventConstraints.shrink_to_fit();

    for (auto &ph: mPhases) {
        ph->shrink_to_fit();
    }
    mPhases.shrink_to_fit();
    mPhaseConstraints.shrink_to_fit();

    mChains.shrink_to_fit();
    mLogModel.shrink_to_fit();
    mLogInit.shrink_to_fit();
    mLogAdapt.shrink_to_fit();
    mLogResults.shrink_to_fit();
}

void Model::clear_and_shrink() noexcept
{
    mCurveName.clear();
    mCurveName.shrink_to_fit();

    mCurveLongName.clear();
    mCurveLongName.shrink_to_fit();

    for (auto &ev: mEvents) {
        ev->clear_and_shrink();
    }
    mEvents.clear();
    mEvents.shrink_to_fit();

    mEventConstraints.clear();

    for (auto &ph: mPhases) {
        ph->clear_and_shrink();
    }
    mPhases.clear();
    mPhases.shrink_to_fit();

    mPhaseConstraints.clear();
    mPhaseConstraints.shrink_to_fit();

    mChains.clear();
    mChains.shrink_to_fit();

    mLogModel.clear();
    mLogModel.shrink_to_fit();

    mLogInit.clear();
    mLogInit.shrink_to_fit();

    mLogAdapt.clear();
    mLogAdapt.shrink_to_fit();

    mLogResults.clear();
    mLogResults.shrink_to_fit();
}


/**
 * @brief Model::updateFormatSettings, set all date format according to the Application Preference, date format
 * @param appSet
 */
void Model::updateFormatSettings()
{
    const auto appSetFormat = DateUtils::getAppSettingsFormat();

    for (const auto& event : mEvents) {
        event->mTheta.setFormat(appSetFormat);
        event->mS02Theta.setFormat(DateUtils::eNumeric);

        for (auto&& date : event->mDates) {
            date.mTi.setFormat(appSetFormat);
            date.mSigmaTi.setFormat(DateUtils::eNumeric);
            date.mWiggle.setFormat(appSetFormat);
        }

    }

    for (const auto& phase : mPhases) {
        phase->mAlpha.setFormat(DateUtils::getAppSettingsFormat());
        phase->mBeta.setFormat(DateUtils::getAppSettingsFormat());
        phase->mDuration.setFormat(DateUtils::eNumeric);
        //phase->mTau.setFormat(DateUtils::eNumeric);

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
        mSettings = StudyPeriodSettings::fromJson(settings);
    }

    if (json.contains(STATE_MCMC)) {
        const QJsonObject mcmc = json.value(STATE_MCMC).toObject();
        mMCMCSettings = MCMCSettings::fromJson(mcmc);
        mChains = mMCMCSettings.getChains();
    }

    if (json.contains(STATE_PHASES)) {
        const QJsonArray phases = json.value(STATE_PHASES).toArray();
        mNumberOfPhases = (int) phases.size();
        for (const auto json : phases)
            mPhases.push_back(std::make_shared<Phase>(json.toObject()));

    }

    // Sort phases based on items y position
    std::sort(mPhases.begin(), mPhases.end(), sortPhases);

    if (json.contains(STATE_EVENTS)) {
        QJsonArray events = json.value(STATE_EVENTS).toArray();
        mNumberOfEvents = (int) events.size();

        for (auto ev : events) {
            const QJsonObject JSONevent = ev.toObject();

            if (JSONevent.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) {
                try {
                    //Event* ev = new Event(JSONevent);
                    mEvents.push_back(std::make_shared<Event>(JSONevent));
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
                //Bound* ek = new Bound(JSONevent);
                mEvents.push_back(std::make_shared<Bound>(JSONevent));
                //ek = nullptr;
            }
        }
    }


    // Sort events based on items y position
    std::sort(mEvents.begin(), mEvents.end(), sortEvents);

    if (json.contains(STATE_EVENTS_CONSTRAINTS)) {
        const QJsonArray constraints = json.value(STATE_EVENTS_CONSTRAINTS).toArray();
        for (auto& co : constraints) {
            const QJsonObject& constraintObj = co.toObject();
            //EventConstraint* c = new EventConstraint(constraintObj);
            mEventConstraints.push_back(std::make_shared<EventConstraint>(constraintObj));
            //c = nullptr;
        }
    }

    if (json.contains(STATE_PHASES_CONSTRAINTS)) {
        const QJsonArray constraints = json.value(STATE_PHASES_CONSTRAINTS).toArray();
        for (auto& co : constraints) {
            const QJsonObject& constraintObj = co.toObject();
           // PhaseConstraint* c = new PhaseConstraint(constraintObj);
            mPhaseConstraints.push_back(std::make_shared<PhaseConstraint>(constraintObj));
           // c = nullptr;
        }
    }

    // ------------------------------------------------------------
    //  Link objects to each other
    //  Must be done here !
    //  nb : Les data sont déjà linkées aux events à leur création
    // ------------------------------------------------------------
    for (size_t i=0; i<mEvents.size(); ++i) {
        int eventId = mEvents.at(i)->mId;
        std::vector<int> phasesIds = mEvents.at(i)->mPhasesIds;

        // Link des events / phases
        for (size_t j=0; j<mPhases.size(); ++j) {
            const int phaseId = mPhases.at(j)->mId;
            if (container_contains(phasesIds, phaseId)) {
                mEvents[i]->mPhases.push_back(mPhases[j]);
                mPhases[j]->mEvents.push_back(mEvents[i]);
            }
        }

        // Link des events / contraintes d'event
        for (size_t j=0; j<mEventConstraints.size(); ++j) {
            if (mEventConstraints[j]->mFromId == eventId) {
                mEventConstraints[j]->mEventFrom = mEvents[i];
                mEvents[i]->mConstraintsFwd.push_back(mEventConstraints[j]);

            } else if (mEventConstraints[j]->mToId == eventId) {
                mEventConstraints[j]->mEventTo = mEvents[i];
                mEvents[i]->mConstraintsBwd.push_back(mEventConstraints[j]);
            }
        }
    }
    // Link des phases / contraintes de phase
    for (size_t i=0; i<mPhases.size(); ++i) {
        const int phaseId = mPhases.at(i)->mId;
        for (size_t j=0; j<mPhaseConstraints.size(); ++j) {
            if (mPhaseConstraints.at(j)->mFromId == phaseId) {
                mPhaseConstraints[j]->mPhaseFrom = mPhases[i];
                mPhases[i]->mConstraintsNextPhases.push_back(mPhaseConstraints[j]);

            } else if (mPhaseConstraints.at(j)->mToId == phaseId) {
                mPhaseConstraints[j]->mPhaseTo = mPhases[i];
                mPhases[i]->mConstraintsPrevPhases.push_back(mPhaseConstraints[j]);
            }
        }

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

QString Model::getModelLog() const
{
    return mLogModel;
}

QString Model::getInitLog() const
{
    QString log;
    int i = 1;
    for (const auto& chain : mChains) {
        log += line( QObject::tr("Elapsed init time %1 for chain %2").arg(DHMS(chain.mInitElapsedTime), QString::number(i)));
        ++i;
    }
    return log + mLogInit;
}

QString Model::getAdaptLog() const
{
    QString log;
    int i = 1;
    for (const auto& chain : mChains) {
        log += line( QObject::tr("Elapsed adaptation time %1 for chain %2").arg(DHMS(chain.mAdaptElapsedTime), QString::number(i)));
        ++i;
    }
    return log + mLogAdapt;
}

QString Model::getResultsLog() const
{
    return mLogResults;
}

void Model::generateResultsLog()
{
    QString log;
    int i = 1;
    for (const auto& chain : mChains) {
        log += line( QObject::tr("Elapsed acquisition time %1 for chain %2").arg(DHMS(chain.mAcquisitionElapsedTime), QString::number(i)));
        ++i;
    }
    log += "<hr>";
    std::ranges::for_each(mPhases, [&log](std::shared_ptr<Phase> phase) {
        log += ModelUtilities::phaseResultsHTML(phase);
        log += "<br>";
        log += line(textBold(textOrange(QObject::tr("Duration (posterior distrib.)"))));
        log += line(textOrange(phase->mDuration.resultsString(QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"))));
        log += "<hr>";
    });

    std::ranges::for_each(mPhaseConstraints, [&log](std::shared_ptr<PhaseConstraint> phase_constraint) {
        log += ModelUtilities::constraintResultsHTML(phase_constraint);
        log += "<hr>";
    });

    for (const auto& ev : mEvents) {
        log += ModelUtilities::eventResultsHTML(ev, true, mSettings.getTminFormated(), mSettings.getTmaxFormated(), !getCurvesName().isEmpty());
        //log += ModelUtilities::eventResultsHTML(ev, true, this);
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

    for (const auto& phase : mPhases) {
        QStringList l = phase->mAlpha.getResultsList(locale, precision);
        maxHpd = std::max(maxHpd, ((int)l.size() - 9) / 3);
        l.prepend(phase->getQStringName() + " Begin");
        rows << l;

        l = phase->mBeta.getResultsList(locale, precision);
        maxHpd = std::max(maxHpd, ((int)l.size() - 9) / 3);
        l.prepend(phase->getQStringName() + " End");
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
    for (std::shared_ptr<Event>& event : mEvents) {
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
             QStringList l = event->mTheta.getResultsList(locale, precision);
             maxHpd = std::max(maxHpd, ((int)l.size() - 9) / 3);
             l.prepend(event->getQStringName());
             rows << l;

        } else {
            QStringList l ;
            l << locale.toString(event->mTheta.mFormatedTrace->at(0), 'f', precision);
            l.prepend(event->getQStringName());
            rows << l;
        }
    }

    // Dates
    rows << QStringList();
    for (std::shared_ptr<Event>& event : mEvents) {
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
            for (size_t j = 0; j < event->mDates.size(); ++j) {
                const Date& date = event->mDates[j];

                QStringList l = date.mTi.getResultsList(locale, precision);
                maxHpd = std::max(maxHpd, ((int)l.size() - 9) / 3);
                l.prepend(date.getQStringName());
                rows << l;
            }
        }
    }

    // Headers
    QStringList list;
    // list << "Item" << "MAP" << "Mean" << "Std dev" << "Q1" << "Q2" << "Q3" << "Credibility %" << "Credibility begin" << "Credibility end";
    list << "Item" << "Trace:Mean" << "Trace:Std"  << "Trace:Q1"<< "Trace:Q2"<< "Trace:Q3" << "Trace:min value" << "Trace:max value";
    list <<"Density:MAP"<< "Density:Mean" << "Density:Std"<< "Density:Q1"<< "Density:Q2"<< "Density:Q3" << "Credibility %" << "Credibility begin" << "Credibility end";
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
        headers << pPhase->getQStringName() + " Begin" << pPhase->getQStringName() + " End";

    rows << headers;

    int shift = 0;
    for (const ChainSpecs& chain : mChains) {
        int burnAdaptSize = 1 + chain.mIterPerBurn + (chain.mBatchIndex * chain.mIterPerBatch);
        int runSize = chain.mRealyAccepted;

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

QList<QStringList> Model::getPhaseTrace(size_t phaseIdx, const QLocale locale, const bool withDateFormat)
{
    QList<QStringList> rows;

    std::shared_ptr<Phase> phase = nullptr;
    if (phaseIdx < mPhases.size())
        phase = mPhases.at(phaseIdx);

    else
        return QList<QStringList>();


 /*   int runSize = 0;

    for (auto& chain : mChains)
        runSize += chain.mRealyAccepted;
*/
    QStringList headers;
    headers << "iter" << phase->getQStringName() + " Begin" << phase->getQStringName() + " End";
    for (const auto& event : phase->mEvents)
        headers << event->getQStringName();

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
                double value;;
                if (event->mType == Event::eBound) {
                    value = event->mTheta.mRawTrace->at(0);
                } else {
                   value = event->mTheta.mRawTrace->at(shift + j);
                }

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
        headers << event->getQStringName();

    rows << headers;

    int shift = 0;
    for (const auto& chain : mChains)  {
        const int burnAdaptSize = 1+ chain.mIterPerBurn + (chain.mBatchIndex * chain.mIterPerBatch);
        const int runSize = chain.mRealyAccepted;

        for (int j = burnAdaptSize; j < burnAdaptSize + runSize; ++j) {
            QStringList l;
            l << QString::number(shift + j) ;

            for (const auto& event : mEvents) {
                double value;
                if (event->mType == Event::eBound || event->mTheta.mSamplerProposal == MHVariable::eFixe) {
                    value = event->mTheta.mRawTrace->at(0);

                } else {
                   value = event->mTheta.mRawTrace->at(shift + j);
                }

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
    bool curveModel = getProject_ptr()->isCurve();
    // 1 - At least one event is required in a model
    // 3 events is needed for a curve
    if (mEvents.empty()) {
        throw QObject::tr("At least one event is required");
        return false;

     } else if (curveModel && mEvents.size() < 3) {
            throw QObject::tr("The model must contain at least 3 Events");
            return false;
    }

    // 2 - The event must contain at least 1 data
    for (const auto& event : mEvents) {
        if (event->type() == Event::eDefault && event->mDates.size() == 0) {
                    throw QObject::tr("The event  \" %1 \" must contain at least 1 data").arg(event->getQStringName());
                    return false;

        }
    }

    // 3 - The phase must contain at least 1 event
    for (const std::shared_ptr<Phase> &ph: mPhases) {
        if (ph->mEvents.size() == 0) {
            throw QObject::tr("The phase \" %1 \" must contain at least 1 event").arg(ph->getQStringName());
            return false;
        }
    }

    // 4 - Pas de circularité sur les contraintes des Events
    std::vector<std::vector<std::shared_ptr<Event>> > eventBranches;
    try {
        eventBranches = ModelUtilities::getAllEventsBranches(mEvents);

    } catch(QString &error){
        throw &error;
    }

    // 5 - Pas de circularité sur les contraintes de phases
    // 6 - Gammas : sur toutes les branches, la somme des gamma min < plage d'étude :
    std::vector<std::vector<Phase*> > phaseBranches;
    try {
        phaseBranches = ModelUtilities::getAllPhasesBranches(mPhases, mSettings.mTmax - mSettings.mTmin);

    } catch(QString &error){
        throw &error;
    }

    // 7 - Un Event ne peut pas appartenir à 2 phases en contrainte
    for (const auto &branche_i :  phaseBranches) {
        QList<std::shared_ptr<Event>> branchEvents;
        for (auto phase : branche_i ) {
             for (const auto& ev : phase->mEvents) {
                if (!branchEvents.contains(ev)) {
                    branchEvents.append(ev);
                } else
                    throw QString(QObject::tr("The event \" %1 \" cannot belong to several phases in a same branch!").arg(ev->getQStringName()));
            }
        }
    }

    // 8 - Bounds : vérifier cohérence des bornes en fonction des contraintes de Events (page 2)
    //  => Modifier les bornes des intervalles des bounds !! (juste dans le modèle servant pour le calcul)
    for (const auto &branche_i : eventBranches) {
        for (size_t j = 0; j<branche_i.size(); j++) {
            std::shared_ptr<Event> event = branche_i.at(j);
            if (event->type() == Event::eBound)  {
                Bound* bound = dynamic_cast<Bound*>(event.get());

                // --------------------
                // Check bound interval lower value
                // --------------------

                // On vérifie toutes les bornes avant et on prend le max
                // de leurs valeurs fixes ou du début de leur intervalle :
                double lower = double (mSettings.mTmin);
                for (size_t k = 0; k<j; ++k) {
                    std::shared_ptr<Event> evt = branche_i.at(k);
                    if (evt->type() == Event::eBound) {
                        lower = qMax(lower, dynamic_cast<Bound*>(evt.get())->mFixed);

                    }
                }
                // Update bound interval

                if (bound->mFixed < lower)
                    throw QString(QObject::tr("The bound \" %1 \" has a fixed value inconsistent with previous bounds in chain!").arg(bound->getQStringName()));

                // --------------------
                // Check bound interval upper value
                // --------------------
                double upper = mSettings.mTmax;
                for (size_t k = j+1; k<branche_i.size(); ++k) {
                    std::shared_ptr<Event> evt = branche_i.at(k);
                    if (evt->type() == Event::eBound) {
                        upper = qMin(upper, dynamic_cast<Bound*>(evt.get())->mFixed);
                    }
                }
                // Update bound interval
                if (bound->mFixed > upper)
                    throw QString(QObject::tr("The bound \" %1 \" has a fixed value inconsistent with next bounds in chain!").arg(bound->getQStringName()));

            }
            event = nullptr;
        }
    }

    // 9 - Gamma min (ou fixe) entre 2 phases doit être inférieur à la différence entre : le min des sups des intervalles des bornes de la phase suivante ET le max des infs des intervalles des bornes de la phase précédente
    for (const auto &phase_const : mPhaseConstraints) {
        double gammaMin = 0.;
        const PhaseConstraint::GammaType gType = phase_const->mGammaType;
        if (gType == PhaseConstraint::eGammaFixed)
            gammaMin = phase_const->mGammaFixed;

        else if (gType == PhaseConstraint::eGammaRange)
            gammaMin = phase_const->mGammaMin;

        double lower = mSettings.mTmin;
        const std::shared_ptr<Phase> phaseFrom = phase_const->mPhaseFrom;
        for (const auto& ev : phaseFrom->mEvents) {
            Bound* bound = dynamic_cast<Bound*>(ev.get());
            if (bound)
                lower = qMax(lower, bound->mFixed);

        }
        double upper = double (mSettings.mTmax);
        std::shared_ptr<Phase> phaseTo = phase_const->mPhaseTo;
        for (const auto& ev : phaseTo->mEvents) {
            Bound* bound = dynamic_cast<Bound*>(ev.get());
            if (bound)
                upper = qMin(upper, bound->mFixed);

            //bound = nullptr;
        }
        if (gammaMin >= (upper - lower))
            throw QString(QObject::tr("The constraint between phases \" %1 \" and \" %2 \" is not consistent with the bounds they contain!").arg(phaseFrom->getQStringName(), phaseTo->getQStringName()));

        //phaseFrom = nullptr;
        //phaseTo = nullptr;
    }

    // 10 - Au sein d'une phase, tau max (ou fixe) doit être supérieur à la différence entre le max des infs des intervalles des bornes et le min des sups des intervalles des bornes.
    //  => Modifier les intervalles des bornes:
    //      - L'inf est le max entre : sa valeur courante ou (le max des infs des intervalles des bornes - tau max ou fixe)
    //      - Le sup est le min entre : sa valeur courante ou (le min des sups des intervalles des bornes + tau max ou fixe)

    for (const auto &phase : mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double tauMax = phase->mTauFixed;

            double min = mSettings.mTmin;
            double max = mSettings.mTmax;
            bool boundFound = false;

            for (const auto &ev : phase->mEvents) {
                if (ev->type() == Event::eBound) {
                    Bound* bound = dynamic_cast<Bound*>(ev.get());
                    if (bound) {
                        boundFound = true;
                        min = std::max(min, bound->mFixed);
                        max = std::min(max, bound->mFixed);

                    }
                    //bound = nullptr;
                }
            }
            if (boundFound){
                if (tauMax < (max - min))
                    throw QString(QObject::tr("The phase \" %1 \" has a duration inconsistent with the bounds it contains!").arg(phase->getQStringName()));
            }
        }
    }

    // 11 - Vérifier la cohérence entre les contraintes d'Event et de Phase
    for (const auto &branche_i : phaseBranches) {
        for (const auto &phase : branche_i) {
            for (const auto &event : phase->mEvents) {
                bool phaseFound = false;

                // On réinspecte toutes les phases de la branche et on vérifie que le fait n'a pas de contrainte en contradiction avec les contraintes de phase !
                for (auto &p : branche_i) {
                    if (p == phase)
                        phaseFound = true;

                    else {
                        for (const auto &e : p->mEvents) {
                            // Si on regarde l'élément d'un phase d'avant, l'Event ne peut pas être en contrainte vers un Event de cette phase
                            if (!phaseFound) {
                                for (const auto & evBwd : e->mConstraintsBwd) {
                                    if (evBwd->mEventFrom == event)
                                        throw QObject::tr("The event %1  (in phase %2 ) is before the event %3 (in phase %4), BUT the phase %5 is after the phase %6 .\r=> Contradiction !").arg(event->getQStringName(), phase->getQStringName(), e->getQStringName(), p->getQStringName(), phase->getQStringName(), p->getQStringName()) ;

                                }
                            } else {
                                for (const auto &evFwd : e->mConstraintsFwd) {
                                    if (evFwd->mEventTo == event)
                                        throw QObject::tr("The event %1 (in phase %2) is after the event %3  (in phase %4), BUT the phase %5 is before the phase %6 .\r=> Contradiction !").arg(event->getQStringName(), phase->getQStringName(), e->getQStringName(), p->getQStringName(), phase->getQStringName(), p->getQStringName());
                                }
                            }
                        }
                    }

                }

            }
        }
    }

    return true;
}

// Generate model data
void Model::generateCorrelations(const std::vector<ChainSpecs> &chains)
{
#ifdef DEBUG
    qDebug()<<"[Model::generateCorrelations] in progress";
    QElapsedTimer t;
    t.start();
#endif

    for (const auto& event : mEvents ) {

        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
#ifdef USE_THREAD
            std::thread thTheta ([event] (std::vector<ChainSpecs> ch) {event->mTheta.generateCorrelations(ch);}, chains);
#else
            event->mTheta.generateCorrelations(chains);
#endif
        }

        if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
            event->mS02Theta.generateCorrelations(chains);
        }

        for (auto&& date : event->mDates ) {
#ifdef USE_THREAD
            std::thread thTi ([date] (QList<ChainSpecs> ch) {date.mTi.generateCorrelations(ch);}, chains);
            std::thread thSigmaTi ([*date] (QList<ChainSpecs> ch) {date.mSigmaTi.generateCorrelations(ch);}, chains);
#else
            date.mTi.generateCorrelations(chains);
            date.mSigmaTi.generateCorrelations(chains);
#endif


        }
#ifdef USE_THREAD
        thTheta.join();
#endif
    }

    for (const auto& phase : mPhases ) {

#ifdef USE_THREAD
        std::thread thAlpha ([phase] (QList<ChainSpecs> ch) {phase->mAlpha.generateCorrelations(ch);}, chains);
        std::thread thBeta ([phase] (QList<ChainSpecs> ch) {phase->mBeta.generateCorrelations(ch);}, chains);
        thAlpha.join();
        thBeta.join();
#else
        phase->mAlpha.generateCorrelations(chains);
        phase->mBeta.generateCorrelations(chains);
#endif
        //phase->mTau.generateCorrelations(chains); // ??? is it useful?
    }

#ifdef DEBUG
    qDebug() <<  "[Model::generateCorrelations] done in " + DHMS(t.elapsed());
#endif

}

#pragma mark FFTLength, Threshold, bandwidth

void Model::setBandwidth(const double bandwidth)
{  
    if (mBandwidth != bandwidth) {
        updateDensities(mFFTLength, bandwidth, mThreshold);
        mBandwidth = bandwidth;

       // emit newCalculus();
    }
}

void Model::setFFTLength(int FFTLength)
{
    if (mFFTLength != FFTLength) {
        updateDensities(FFTLength, mBandwidth, mThreshold);
        //emit newCalculus();
    }
}

void Model::setHActivity(const double h, const double rangePercent)
{
   if (!mPhases.empty()) {
        generateActivity(mFFTLength, h, mThreshold, rangePercent);
        mHActivity = h;

       // emit newCalculus();
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

        generateActivity(mFFTLength, mHActivity, threshold);
        setThresholdToAllModel(threshold);

        //emit newCalculus();
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

#pragma mark Loop
void Model::memo_accept(const unsigned i_chain)
{
    for (const auto& event : mEvents) {
       //--------------------- Memo Events -----------------------------------------
       if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
            event->mTheta.memo_accept(i_chain);

            event->mS02Theta.memo_accept(i_chain);

            //--------------------- Memo Dates -----------------------------------------
            for (auto&& date : event->mDates )   {
                date.mTi.memo_accept(i_chain);
                date.mSigmaTi.memo_accept(i_chain);
                date.mWiggle.memo_accept(i_chain);

            }
       }
    }
}

void Model::initVariablesForChain()
{
    // Today we have the same acceptBufferLen for every chain
    const int acceptBufferLen =  mChains[0].mIterPerBatch;
    /*qsizetype initReserve = 0;

    for (auto&& c: mChains)
       initReserve += ( 1 + (c.mMaxBatchs*c.mIterPerBatch) + c.mIterPerBurn + (c.mIterPerAquisition/c.mThinningInterval) );
*/
    for (auto&& event : mEvents) {
       //event->mTheta.clear();
       //event->mTheta.reserve(initReserve);
       event->mTheta.mNbValuesAccepted.resize(mChains.size());
       //event->mTheta.mLastAccepts.reserve(acceptBufferLen);
       event->mTheta.mLastAcceptsLength = acceptBufferLen;

       //event->mS02Theta.clear();
       //event->mS02Theta.reserve(initReserve);
       event->mS02Theta.mNbValuesAccepted.resize(mChains.size());
       //event->mS02Theta.mLastAccepts.reserve(acceptBufferLen);
       event->mS02Theta.mLastAcceptsLength = acceptBufferLen;

       // event->mTheta.mNbValuesAccepted.clear(); //don't clean, avalable for cumulate chain

       for (auto&& date : event->mDates) {
            //date.mTi.clear();
            //date.mTi.reserve(initReserve);
            date.mTi.mNbValuesAccepted.resize(mChains.size());
            //date.mTi.mLastAccepts.reserve(acceptBufferLen);
            date.mTi.mLastAcceptsLength = acceptBufferLen;

            //date.mSigmaTi.clear();
            //date.mSigmaTi.reserve(initReserve);
            date.mSigmaTi.mNbValuesAccepted.resize(mChains.size());
            //date.mSigmaTi.mLastAccepts.reserve(acceptBufferLen);
            date.mSigmaTi.mLastAcceptsLength = acceptBufferLen;

            //date.mWiggle.clear();
            //.mWiggle.reserve(initReserve);
            date.mWiggle.mNbValuesAccepted.resize(mChains.size());
            //date.mWiggle.mLastAccepts.reserve(acceptBufferLen);
            date.mWiggle.mLastAcceptsLength = acceptBufferLen;
       }
    }

  /*  for (auto&& phase : mPhases) {
       phase->mAlpha.clear();
       phase->mBeta.clear();
       //phase->mTau.reset();
       phase->mDuration.clear();

       //phase->mAlpha.reserve(initReserve);
       //phase->mBeta.reserve(initReserve);
       //phase->mTau.mRawTrace->reserve(initReserve);
       //phase->mDuration.reserve(initReserve);
    }*/
}



#pragma mark Densities

void Model::initNodeEvents()
{
    std::ranges::for_each( mEvents, [](std::shared_ptr<Event> ev) {
        ev->mIsNode = false;
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
    remove_smoothed_densities();//clearPosteriorDensities();

    if (getProject_ptr()->mLoop)
        emit getProject_ptr()->mLoop->setMessage(QObject::tr("Computing posterior distributions and numerical results"));
    generateCredibility(threshold);

    updateFormatSettings(); // update formatedCredibility and formatedTrace

    generatePosteriorDensities(mChains, fftLen, bandwidth);

    generateHPD(threshold);

    // memo the new value of the Threshold inside all the part of the model: phases, events and dates
    setThresholdToAllModel(threshold);

    if (!mPhases.empty()) {
        generateTempo(fftLen);
        generateActivity(fftLen, mHActivity, threshold);
    }

    generateNumericalResults(mChains);

    mBandwidth = bandwidth;
    mFFTLength = fftLen;

}

void Model::generatePosteriorDensities(const std::vector<ChainSpecs> &chains, int fftLen, double bandwidth)
{
#ifdef DEBUG
    QElapsedTimer t;
    t.start();
#endif

    const double tmin = mSettings.getTminFormated();
    const double tmax = mSettings.getTmaxFormated();

    for (const auto& event : mEvents) {
        event->mTheta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);

        if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
            event->mS02Theta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);

        if (event->type() != Event::eBound) {
            for (auto&& d : event->mDates)
                d.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
        }
    }

    for (const auto& phase : mPhases)
         phase->generateHistos(chains, fftLen, bandwidth, tmin, tmax);

#ifdef DEBUG
    qDebug() <<  "=> Model::generatePosteriorDensities done in " + DHMS(t.elapsed());
#endif
}

void Model::generateNumericalResults(const std::vector<ChainSpecs> &chains)
{
#ifdef DEBUG
    QElapsedTimer t;
    t.start();
#endif


#ifdef USE_THREAD
    std::thread thEvents ([this] (QList<ChainSpecs> chains)
    {
        for (const auto& event : mEvents) {
            if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
                event->mTheta.generateNumericalResults(chains);

                if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                    event->mS02Theta.generateNumericalResults(chains);

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
#else

    std::ranges::for_each( mEvents, [chains](std::shared_ptr<Event> event) {
         if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
            event->mTheta.generateNumericalResults(chains);

            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                event->mS02Theta.generateNumericalResults(chains);

            for (auto&& date : event->mDates) {
                date.mTi.generateNumericalResults(chains);
                date.mSigmaTi.generateNumericalResults(chains);
            }
         } else {
            event->mTheta.MetropolisVariable::generateNumericalResults(chains);
         }
    });


    std::ranges::for_each( mPhases, [chains](std::shared_ptr<Phase> phase) {
         phase->mAlpha.MetropolisVariable::generateNumericalResults(chains);
         phase->mBeta.MetropolisVariable::generateNumericalResults(chains);
         // phase->mTau.generateNumericalResults(chains);
         phase->mDuration.MetropolisVariable::generateNumericalResults(chains);
    });


#endif

#ifdef DEBUG
    qDebug() <<  "[Model::generateNumericalResults] done in " + DHMS(t.elapsed()) ;
#endif
}

void Model::clearThreshold()
{
    mThreshold = -1.;
    std::ranges::for_each( mEvents, [](std::shared_ptr<Event> ev) {
         ev->mTheta.mThresholdUsed = -1.;
         ev->mS02Theta.mThresholdUsed = -1.;

         for (auto&& date : ev->mDates) {
            date.mTi.mThresholdUsed = -1.;
            date.mSigmaTi.mThresholdUsed = -1.;
         }
    });

    std::ranges::for_each( mPhases, [](std::shared_ptr<Phase> phase) {
         phase->mAlpha.mThresholdUsed = -1.;
         phase->mBeta.mThresholdUsed = -1.;
         phase->mTau.mThresholdUsed = -1.;
         phase->mDuration.mThresholdUsed = -1.;
    });
}

void Model::generateCredibility(const double thresh)
{
#ifdef DEBUG
    qDebug()<<QString("[Model::generateCredibility] Treshold = %1 %; in progress").arg(thresh);
    QElapsedTimer t;
    t.start();
#endif
    if (mThreshold == thresh)
        return;


#ifdef USE_THREAD
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
            phase->mAlpha.generateCredibility(mChains, thresh); // in formated Date
            phase->mBeta.generateCredibility(mChains, thresh);
            //  pPhase->mTau.generateCredibility(mChains, thresh);
            phase->mDuration.generateCredibility(mChains, thresh);
            // TimeRange in Raw Date
            phase->mTimeRange = timeRangeFromTraces( phase->mAlpha.fullRunRawTrace(mChains),
                                                     phase->mBeta.fullRunRawTrace(mChains), thresh, "Time Range for Phase : " + phase->mName);
            phase->mValueStack["TimeRange Threshold"] = TValueStack("TimeRange Threshold", thresh);
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
    for (const auto& ev : mEvents) {
        if (ev->type() != Event::eBound)//(ev->mTheta.mSamplerProposal != MHVariable::eFixe)
            ev->mTheta.generateCredibility(mChains, thresh);

        if (ev->mS02Theta.mSamplerProposal != MHVariable::eFixe)
            ev->mS02Theta.generateCredibility(mChains, thresh);

        if (ev->type() != Event::eBound) {
            for (auto&& date : ev->mDates )  {
                date.mTi.generateCredibility(mChains, thresh);
                date.mSigmaTi.generateCredibility(mChains, thresh);
            }
        }

    }

    for (const auto& phase :mPhases) {
        // If there is only one Event in the phase, there is no Duration
        phase->mAlpha.generateCredibility(mChains, thresh);
        phase->mBeta.generateCredibility(mChains, thresh);
        //  pPhase->mTau.generateCredibility(mChains, thresh);
        phase->mDuration.generateCredibility(mChains, thresh);
        phase->mTimeRange = timeRangeFromTraces( phase->mAlpha.fullRunRawTrace(mChains),
                                                  phase->mBeta.fullRunRawTrace(mChains), thresh, "Time Range for Phase : " + phase->getQStringName());
    }


    for (const auto& phaseConstraint : mPhaseConstraints) {
        const QString str = phaseConstraint->mPhaseFrom->getQStringName() + " to " + phaseConstraint->mPhaseTo->getQStringName();

        phaseConstraint->mGapRange = gapRangeFromTraces(phaseConstraint->mPhaseFrom->mBeta.fullRunRawTrace(mChains),
                                                        phaseConstraint->mPhaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Gap Range : "+ str);

        qDebug() << "[Model::generateCredibility] Gap Range " << str;

        phaseConstraint->mTransitionRange = transitionRangeFromTraces(phaseConstraint->mPhaseFrom->mBeta.fullRunRawTrace(mChains),
                                                                      phaseConstraint->mPhaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Transition Range : " + str);

    }

#endif

#ifdef DEBUG
    qDebug() <<  "[Model::generateCredibility] done in " + DHMS(t.elapsed());
#endif

}

void Model::generateHPD(const double thresh)
{
#ifdef DEBUG
    qDebug()<<QString("[Model::generateHPD] Treshold =  %1 %; in progress").arg(thresh);
    QElapsedTimer t;
    t.start();
#endif

    for (const auto& event : mEvents) {
        event->mTheta.generateHPD(thresh);

        if (event->type() != Event::eBound) {

            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                event->mS02Theta.generateHPD(thresh);

            for (auto&& date : event->mDates) {
                date.mTi.generateHPD(thresh);
                date.mSigmaTi.generateHPD(thresh);
            }
        }
    }

    for (const auto& ph : mPhases) {
        ph->mAlpha.generateHPD(thresh);
        ph->mBeta.generateHPD(thresh);
        ph->mDuration.generateHPD(thresh);
    }


#ifdef DEBUG
    qDebug() <<  "[Model::generateHPD] done in " + DHMS(t.elapsed()) ;
#endif

}

/**
 * @brief Model::generateTempo
 * The function check if the table mTempo exist. In this case, there is no calcul.
 * This calculus must be in date, not in age
 */

void Model::generateTempo(const size_t gridLength)
{
#ifdef DEBUG
    qDebug()<<QString("[Model::generateTempo] in progress");
    QElapsedTimer tClock;
    tClock.start();
#endif

    for (const auto& phase : mPhases) {
        const int n = (int)(phase->mEvents.size());

        // Description des données
        std::vector<double> concaAllTrace;

        const int nRealyAccepted = std::accumulate(mChains.begin(), mChains.end(), 0, [](double sum, ChainSpecs chain){return  sum + chain.mRealyAccepted;});


        for (const auto& ev : phase->mEvents) {
            if (ev->mTheta.mSamplerProposal != MHVariable::eFixe) {
                const auto &rawtrace = ev->mTheta.fullRunRawTrace(mChains);
                concaAllTrace.resize(concaAllTrace.size() + rawtrace.size());
                std::copy_backward( rawtrace.begin(), rawtrace.end(), concaAllTrace.end() );

            } else {

                const size_t begin_size = concaAllTrace.size();
                concaAllTrace.resize(concaAllTrace.size() + nRealyAccepted);
                std::fill_n( concaAllTrace.begin()+begin_size, nRealyAccepted, ev->mTheta.mRawTrace->at(0));
            }

        }


        auto minmaxAll = std::minmax_element(concaAllTrace.begin(), concaAllTrace.end());
        double t_min_data = *minmaxAll.first;
        double t_max_data = *minmaxAll.second;

        // cas d'une borne seule dans la phase
        if (t_min_data >= t_max_data) {
           t_max_data = mSettings.mTmax;
        }
        phase->mValueStack.insert_or_assign("t_min", t_min_data);
        phase->mValueStack.insert_or_assign("t_max", t_max_data);

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
         const int iMax = (int)(gridLength-1);
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
        std::vector<double> infT;
        std::vector<double> supT;

        std::vector<double> espT;
        double pT, eT;

        for (const auto& niT : niTempo) {
            // Compute Tempo
            pT = niT/ nr;
            eT =  n * pT ;
            espT.push_back(eT);

            // Calculation of 95% Gaussian error with z score=1.959
            /*
            double vT, infpT;
            vT = n * pT * (1-pT);
            infpT = ( eT < 1.96 * sqrt(vT) ? 0. : eT - 1.96 * sqrt(vT) );// Forbidden negative error
            infT.push_back( infpT );
            supT.push_back( eT + 1.96 * sqrt(vT));
            */
            // binomial (Clopper-Pearson) error  with 95% (or mThreshold) CI
            if (!mBinomiale_Gx.contains(n) ) {
                const std::vector<double> &Rq = binomialeCurveByLog(n, 1. - mThreshold/100.); //  Determine the curve x = r (q)
                mBinomiale_Gx[n] = inverseCurve(Rq);
            }

            const std::vector<double>& Gx = mBinomiale_Gx.at(n);
            auto QInf = interpolate_value_from_curve(pT, Gx, 0., 1.)*n ;
            auto QSup = findOnOppositeCurve(pT, Gx)*n;
            supT.push_back( QInf);
            infT.push_back( QSup );
            //qDebug()<<"generateTempo GX "<<phase->getQStringName()<< n <<" eT="<< eT <<eT+1.96*sqrt(vT)<<infpT<<" Gx:" <<QSup<<QInf;

        }

        phase->mRawTempo.clear();
        phase->mRawTempoInf.clear();
        phase->mRawTempoSup.clear();

        phase->mRawTempo = vector_to_map(espT, t_min_data, t_max_data, delta_t);

        if (n>1) {
            phase->mRawTempoInf = vector_to_map(infT, t_min_data, t_max_data, delta_t);
            phase->mRawTempoSup = vector_to_map(supT, t_min_data, t_max_data, delta_t);
        }


        // close the error curve on mean value
        const double tEnd = phase->mRawTempo.crbegin()->first;
        const double vEnd = phase->mRawTempo[tEnd];

        if ( tEnd <= mSettings.mTmax && n>1) {
            phase->mRawTempoInf[tEnd] = vEnd;
            phase->mRawTempoSup[tEnd ] = vEnd;
        }

        phase->mRawTempo.emplace(mSettings.mTmax, vEnd);

        const double tBegin = phase->mRawTempo.begin()->first;

        // We need to add a point with the value 0 for the automatique Y scaling
        if ((tBegin) >= mSettings.mTmin) {
            phase->mRawTempo[tBegin] = 0;
            if (n>1) {
                phase->mRawTempoInf[tBegin] = 0;
                phase->mRawTempoSup[tBegin] = 0;
            }
        }
        phase->mTempo.clear();
        phase->mTempoInf.clear();
        phase->mTempoSup.clear();

        phase->mTempo = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempo);

        if (n>1) {
            phase->mTempoInf = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoInf);
            phase->mTempoSup = DateUtils::convertMapToAppSettingsFormat(phase->mRawTempoSup);
        }

     } // Loop End on phase

#ifdef DEBUG
    qDebug() <<  QString("[Model::generateTempo] done in " + DHMS(tClock.elapsed()));
#endif

}


/**
 *  @brief Clear model data
 */
void Model::remove_smoothed_densities()
{
    for (auto &event : mEvents) {
        for (auto&& date : event->mDates) {
            date.mTi.remove_smoothed_densities();
            date.mSigmaTi.remove_smoothed_densities();
            date.mWiggle.remove_smoothed_densities();
        }
        event->mTheta.remove_smoothed_densities();
        event->mS02Theta.remove_smoothed_densities();
    }

    for (auto &phase : mPhases) {
        phase->mAlpha.remove_smoothed_densities();
        phase->mBeta.remove_smoothed_densities();
        phase->mDuration.remove_smoothed_densities();

        phase->mTempo.clear();
        phase->mTempoInf.clear();
        phase->mTempoSup.clear();
        phase->mActivity.clear();
        phase->mActivityInf.clear();
        phase->mActivitySup.clear();
        phase->mRawTempo.clear();
        phase->mRawTempoInf.clear();
        phase->mRawTempoSup.clear();
        phase->mRawActivity.clear();
        phase->mRawActivityInf.clear();
        phase->mRawActivitySup.clear();

        phase->mTau.clear();
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
 * @brief Model::generateActivity. If there is only one Event, the activity is equal to it
 * @param gridLenth
 * @param h defined in year, if h<0 then h= delta_t \f$ \delta_t = (t_max - t_min + 4h )/(gridLenth) \f$
 */
void Model::generateActivity(const size_t gridLength, const double h, const double threshold, const double rangePercent)
{
    for (const auto& phase : mPhases) {
        // Curves for error binomial
        const int n = phase->mEvents.size();
        /*if (n<2) {
            phase->mActivity = phase->mEvents[0]->mTheta.mFormatedHisto;
            phase->mActivityInf = phase->mEvents[0]->mTheta.mFormatedHisto;
            phase->mActivitySup = phase->mEvents[0]->mTheta.mFormatedHisto;
            phase->mActivityUnifTheo = phase->mEvents[0]->mTheta.mFormatedHisto;
            continue;
        }*/
        if (!mBinomiale_Gx.contains(n) || threshold != mThreshold) {
            const std::vector<double> &Rq = binomialeCurveByLog(n, 1. - threshold/100.); //  Détermine la courbe x = r (q)
            mBinomiale_Gx[n] = inverseCurve(Rq); // Pour qActivity, détermine la courbe p = g (x)
        }
        phase->generateActivity(gridLength, h, threshold, rangePercent);
    }

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
        const double idx_under = std::clamp(floor(idx), 0., numPts-1.);
        const double idx_upper = std::clamp(idx_under + 1., 0., numPts-1.);

        const double contrib_under = (idx_upper - idx) / denum;
        const double contrib_upper = (idx - idx_under) / denum;
#ifdef DEBUG
        if (std::isinf(contrib_under) || std::isinf(contrib_upper))
            qDebug() << "FFT input : infinity contrib!";

        if (idx_under < 0 || idx_under >= numPts || idx_upper < 0 || idx_upper > numPts)
            qDebug() << "FFT input : Wrong index";
#endif
        if (idx_under < numPts)
            input[(int)idx_under] += contrib_under;

        if (idx_upper < numPts) // This is to handle the case when matching the last point index !
            input[(int)idx_upper] += contrib_upper;
    }

}

void Model::clearCredibilityAndHPD()
{
    for (auto&& e : mEvents) {
        for (auto&& date : e->mDates) {
            date.mTi.mFormatedHPD.clear();
            date.mTi.mRawCredibility = std::pair<double, double>(1, -1);
            date.mTi.mFormatedCredibility = std::pair<double, double>(1, -1);
            date.mSigmaTi.mFormatedHPD.clear();
            date.mSigmaTi.mFormatedCredibility= std::pair<double, double>(1, -1);
        }
        e->mTheta.mFormatedHPD.clear();
        e->mTheta.mRawCredibility = std::pair<double, double>(1, -1);
        e->mTheta.mFormatedCredibility = std::pair<double, double>(1, -1);
    }

    for (auto&& p : mPhases) {
        p->mAlpha.mFormatedHPD.clear();
        p->mAlpha.mRawCredibility = std::pair<double, double>(1, -1);
        p->mAlpha.mFormatedCredibility = std::pair<double, double>(1, -1);

        p->mBeta.mFormatedHPD.clear();
        p->mBeta.mRawCredibility = std::pair<double, double>(1, -1);
        p->mBeta.mFormatedCredibility = std::pair<double, double>(1, -1);

        //(*iterPhase)->mTau.mFormatedHPD.clear();
       // (*iterPhase)->mTau.mFormatedCredibility = QPair<double, double>();

        p->mDuration.mFormatedHPD.clear();
        p->mDuration.mRawCredibility = std::pair<double, double>(1, -1);
        p->mDuration.mFormatedCredibility = std::pair<double, double>(1, -1);

        p->mTimeRange = std::pair<double, double>();

    }
}

void Model::clearTraces()
{
    for (const auto& ev : mEvents) {
        for (auto&& date : ev->mDates) {
            date.clear();
        }
        ev->clear();
    }

    for (const auto& ph : mPhases) {
        ph->clear();
    }
}


 // Date files read / write
/** @Brief Save .res file, the result of computation and compress it
 *
 * */
void Model::saveToFile(QDataStream *out)
{
    out->setVersion(QDataStream::Qt_6_7); // since v3.3.0 -> Qt_6_7

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
    //  Writing phase data
    // -----------------------------------------------------
    for (std::shared_ptr<Phase>& phase : mPhases) {
        *out << phase->mAlpha;
        *out << phase->mBeta;
        *out << phase->mTau;
        *out << phase->mDuration;
    }
    // -----------------------------------------------------
    //  Writing event data
    // -----------------------------------------------------
    for (std::shared_ptr<Event>& event : mEvents){
        *out << event->mTheta;
        *out << event->mS02Theta;
    }
    // -----------------------------------------------------
    //  Writing date data
    // -----------------------------------------------------
    for (std::shared_ptr<Event>& event : mEvents) {
        if (event->mType == Event::eDefault ) {
            for (auto&& d  : event->mDates) {
                *out << d.mTi;
                *out << d.mSigmaTi;
                if (d.mDeltaType != Date::eDeltaNone)
                    *out << d.mWiggle;

                *out << d.mDeltaFixed;
                *out << d.mDeltaMin;
                *out << d.mDeltaMax;
                *out << d.mDeltaAverage;
                *out << d.mDeltaError;

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
bool Model::restoreFromFile_v323(QDataStream *in)
{
    std::cout << "[Model::restoreFromFile_v323] Entering" << std::endl;

    if (in->version()!= QDataStream::Qt_6_4)
        return false;

    // -----------------------------------------------------
    //  Read info
    // -----------------------------------------------------

    quint32 tmp32;
    *in >> tmp32;

    mChains.clear();

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
        mChains.push_back(ch);
    }

    // -----------------------------------------------------
    //  Read phases data
    // -----------------------------------------------------

    for (std::shared_ptr<Phase> &p : mPhases) {
        *in >> p->mAlpha;
        *in >> p->mBeta;
        *in >> p->mTau;
        *in >> p->mDuration;
    }
    // -----------------------------------------------------
    //  Read events data
    // -----------------------------------------------------

    for (std::shared_ptr<Event> &e : mEvents) {
        *in >> e->mTheta;
        e->mS02Theta.mSamplerProposal = MHVariable::eFixe;
        *in >> e->mS02Theta; // since 2023-06-01 v3.2.3
    }
    // -----------------------------------------------------
    //  Read dates data
    // -----------------------------------------------------

    for (std::shared_ptr<Event>& event : mEvents) {
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

                d.mCalibration = & (getProject_ptr()->mCalibCurves[d.mUUID]);

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

                const std::string toFind = "WID::"+ d.mUUID;

                if (d.mWiggleCalibration == nullptr || d.mWiggleCalibration->mVector.empty()) {
                    qDebug()<<"[Model::restoreFromFile] mWiggleCalibration vide";

                } else {
                    d.mWiggleCalibration = & (getProject_ptr()->mCalibCurves[toFind]);
                }
                //#endif
            }
    }
    *in >> mLogModel;
    *in >> mLogInit;
    *in >> mLogAdapt;
    *in >> mLogResults;

    return true;
}

bool Model::restoreFromFile_v324(QDataStream *in)
{
    std::cout << "[Model::restoreFromFile_v324] Entering"  << std::endl;

    const QList<QString> compatible_version {"3.2.4", "3.2.6"};

    if (in->version()!= QDataStream::Qt_6_4)
        return false;

#ifdef DEBUG
    if (res_file_version != qApp->applicationVersion())
        qDebug() << "[Model::restoreFromFile_v324] Different Model version =" << res_file_version<<" actual =" << qApp->applicationVersion();

    if (compatible_version.contains(res_file_version))
        qDebug() << "[Model::restoreFromFile_v324] Version compatible 3.2.4";
#endif
    // -----------------------------------------------------
    //  Read info
    // -----------------------------------------------------

    quint32 tmp32;
    *in >> tmp32;

    mChains.clear();
    //mChains.reserve(int (tmp32));
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
        mChains.push_back(ch);
    }

    // -----------------------------------------------------
    //  Read phases data
    // -----------------------------------------------------

    for (std::shared_ptr<Phase> &p : mPhases) {
        *in >> p->mAlpha;
        *in >> p->mBeta;
        *in >> p->mTau;
        *in >> p->mDuration;
    }
    // -----------------------------------------------------
    //  Read events data
    // -----------------------------------------------------

    for (std::shared_ptr<Event> &e : mEvents) {
        *in >> e->mTheta;
        *in >> e->mS02Theta; // since 2023-06-01 v3.2.3
    }
    // -----------------------------------------------------
    //  Read dates data
    // -----------------------------------------------------

    for (std::shared_ptr<Event>& event : mEvents) {
        if (event->mType == Event::eDefault )
            for (auto&& d : event->mDates) {
                //d.mTi = MHVariable();
                //d.mSigmaTi = MHVariable();
                //d.mWiggle = MHVariable();
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

                d.mCalibration = & (getProject_ptr()->mCalibCurves[d.mUUID]);

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

                const std::string toFind ("WID::"+ d.mUUID);

                if (d.mWiggleCalibration == nullptr || d.mWiggleCalibration->mVector.empty()) {
                    qDebug()<<"[Model::restoreFromFile] mWiggleCalibration vide";

                } else {
                    d.mWiggleCalibration = & (getProject_ptr()->mCalibCurves[toFind]);
                }
                //#endif
            }
    }
    *in >> mLogModel;
    *in >> mLogInit;
    *in >> mLogAdapt;
    *in >> mLogResults;

    return true;
}

bool Model::restoreFromFile_v328(QDataStream *in)
{
    std::cout << "[Model::restoreFromFile_v328] Entering"  << std::endl;

    const QList<QString> compatible_version { "3.2.4", "3.2.6","3.2.9"};

    if (in->version()!= QDataStream::Qt_6_4)
        return false;

#ifdef DEBUG
    if (res_file_version != qApp->applicationVersion())
        qDebug() << "[Model::restoreFromFile_v328] Different Model version = " << res_file_version << " actual =" << qApp->applicationVersion();

    if (compatible_version.contains(res_file_version))
        qDebug()<<" [Model::restoreFromFile_v328] Version compatible 3.2.8";
#endif
    // -----------------------------------------------------
    //  Read info
    // -----------------------------------------------------

    quint32 tmp32;
    *in >> tmp32;

    mChains.clear();
    //mChains.reserve(int (tmp32));
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
        mChains.push_back(ch);
    }

    // -----------------------------------------------------
    //  Read phases data
    // -----------------------------------------------------

    for (std::shared_ptr<Phase> &p : mPhases) {
        p->mAlpha.load_stream(*in);
        p->mBeta.load_stream(*in);
        p->mTau.load_stream(*in);
        p->mDuration.load_stream(*in);

    }
    // -----------------------------------------------------
    //  Read events data
    // -----------------------------------------------------

    for (std::shared_ptr<Event> &e : mEvents) {
        e->mTheta.load_stream(*in);
        e->mS02Theta.load_stream(*in); // since 2023-06-01 v3.2.3
    }
    // -----------------------------------------------------
    //  Read dates data
    // -----------------------------------------------------

    for (std::shared_ptr<Event> &event : mEvents) {
        if (event->mType == Event::eDefault )
            for (auto&& d : event->mDates) {
                //d.mTi = MHVariable();
                //d.mSigmaTi = MHVariable();
                //d.mWiggle = MHVariable();
                d.mTi.load_stream(*in);
                d.mSigmaTi.load_stream(*in);
                if (d.mDeltaType != Date::eDeltaNone)
                    d.mWiggle.load_stream(*in);

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

                d.mCalibration = & (getProject_ptr()->mCalibCurves[d.mUUID]);

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

                const std::string toFind ("WID::"+ d.mUUID);

                if (d.mWiggleCalibration == nullptr || d.mWiggleCalibration->mVector.empty()) {
                    qDebug()<<"[Model::restoreFromFile_v328] mWiggleCalibration Empty";

                } else {
                    d.mWiggleCalibration = & (getProject_ptr()->mCalibCurves[toFind]);
                }
                //#endif
            }
    }
    *in >> mLogModel;
    *in >> mLogInit;
    *in >> mLogAdapt;
    *in >> mLogResults;

    return true;
}

bool Model::restoreFromFile_v330(QDataStream *in)
{
    std::cout << "[Model::restoreFromFile_v330] Entering" << std::endl;

    const QList<QString> compatible_version {"3.3.0", "3.3.5"};

    if (in->version()!= QDataStream::Qt_6_7)
        return false;


#ifdef DEBUG
    if (res_file_version != qApp->applicationVersion())
        qDebug()<<"[Model::restoreFromFile_v330] Different Model version =" << res_file_version << " actual =" << qApp->applicationVersion();

    if (compatible_version.contains(res_file_version))
        qDebug() << "[Model::restoreFromFile_v330] Version compatible";
#endif

    try {



        // -----------------------------------------------------
        //  Read info
        // -----------------------------------------------------

        quint32 tmp32;
        *in >> tmp32;

        mChains.clear();
        //mChains.reserve(int (tmp32));
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
            mChains.push_back(ch);
        }

        // -----------------------------------------------------
        //  Read phases data
        // -----------------------------------------------------

        for (std::shared_ptr<Phase> &p : mPhases) {
            p->mAlpha.load_stream(*in);
            p->mBeta.load_stream(*in);
            p->mTau.load_stream(*in);
            p->mDuration.load_stream(*in);

        }
        // -----------------------------------------------------
        //  Read events data
        // -----------------------------------------------------

        for (std::shared_ptr<Event> &e : mEvents) {
            e->mTheta.load_stream(*in);
            e->mS02Theta.load_stream(*in); // since 2023-06-01 v3.2.3
        }
        // -----------------------------------------------------
        //  Read dates data
        // -----------------------------------------------------

        for (std::shared_ptr<Event> &event : mEvents) {
            if (event->mType == Event::eDefault )
                for (auto&& d : event->mDates) {

                    d.mTi.load_stream(*in);
                    d.mSigmaTi.load_stream(*in);
                    if (d.mDeltaType != Date::eDeltaNone)
                        d.mWiggle.load_stream(*in);

                    *in >> d.mDeltaFixed;
                    *in >> d.mDeltaMin;
                    *in >> d.mDeltaMax;
                    *in >> d.mDeltaAverage;
                    *in >> d.mDeltaError;

                    double tmp;
                    *in >> tmp;
                    d.setTminRefCurve(tmp);
                    *in >> tmp;
                    d.setTmaxRefCurve(tmp);

                    d.mCalibration = & (getProject_ptr()->mCalibCurves[d.mUUID]);

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

                    const std::string toFind ("WID::"+ d.mUUID);

                    if (d.mWiggleCalibration == nullptr || d.mWiggleCalibration->mVector.empty()) {
                        qDebug()<<"[Model::restoreFromFile_v330] mWiggleCalibration vide";

                    } else {
                        d.mWiggleCalibration = & (getProject_ptr()->mCalibCurves[toFind]);
                    }
                    //#endif
                }
        }
        *in >> mLogModel;
        *in >> mLogInit;
        *in >> mLogAdapt;
        *in >> mLogResults;

        return true;

    } catch (...) {
        std::cout << "[Model::restoreFromFile_v330] Error" << std::endl;
        return false;
    }
}


bool Model::hasSelectedEvents()
{
   return std::any_of(mEvents.begin(), mEvents.end(), [](const std::shared_ptr<Event> e){return e->mIsSelected;});
}

bool Model::hasSelectedPhases()
{
    return std::any_of(mPhases.begin(), mPhases.end(), [](const std::shared_ptr<Phase> p){return p->mIsSelected;});
}

t_reduceTime Model::reduceTime(double t) const
{
    const long double tmin = static_cast<long double>(mSettings.mTmin);
    const long double tmax = static_cast<long double>(mSettings.mTmax);
    const long double tL = static_cast<long double>(t);
    const long double denominator = tmax - tmin;

#ifdef DEBUG
    // Vérification de la stabilité numérique
    if (std::abs(denominator) < std::numeric_limits<long double>::epsilon()) {
        throw std::runtime_error("Erreur : Division par une valeur trop petite !");
    }
#endif

    return static_cast<double>((tL - tmin) / denominator);
}




std::vector<t_reduceTime> Model::reduceTime(const std::vector<double> &vec_t) const
{
    const long double tmin = static_cast<long double>(mSettings.mTmin);
    const long double tmax = static_cast<long double>(mSettings.mTmax);

    const long double denominator = 1.0L / (tmax - tmin);

    std::vector<t_reduceTime> res;
    for (auto& t : vec_t) {
        const long double tL = static_cast<long double>(t);
        res.push_back(static_cast<double>((tL - tmin) * denominator));
    }
    return res;
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
