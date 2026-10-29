/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
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
#include "ParallelFor.h"

#include <QIODevice>

#include "MetropolisVariable.h"

#include <QJsonArray>
#include <QtWidgets>
#include <QtCore/QStringList>
#include <iostream>
#include <ranges>
#include <QDataStream>
#include <QFile>

#define NO_USE_THREAD
#ifdef USE_THREAD
#include <thread>
#endif
#include <unordered_map>

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
    mBandwidthType(BandwidthType::eBWSJ),
    mBandwidth(1),
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
    mThreshold(-1.0),
    mBandwidthType(BandwidthType::eBWSJ),
    mFFTLength(1024),
    mHActivity(1.0)
{
    // same code as fromJSON
    if (json.contains(STATE_SETTINGS)) {
        const QJsonObject settings = json.value(STATE_SETTINGS).toObject();
        mSettings = StudyPeriodSettings::fromJson(settings);
    }
    auto tmin = mSettings.getTminFormated();
    auto tmax = mSettings.getTmaxFormated();
    mBandwidth = (tmax - tmin) / 1000.0;

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
        mNumberOfEvents = static_cast<int>(events.size());

        for (auto it = events.cbegin(); it != events.cend(); ++it) {
            // it pointe vers un QJsonValue
            if (!it->isObject())
                continue;
            QJsonObject eventObj = it->toObject();

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
        for (qsizetype i = 0; i < constraints.size(); ++i) {
            const QJsonObject &constraintObj = constraints.at(i).toObject();
            const auto& c = std::make_shared<EventConstraint>(constraintObj);
            mEventConstraints.push_back(c);

        }
    }

    if (json.contains(STATE_PHASES_CONSTRAINTS)) {
        const QJsonArray constraints = json.value(STATE_PHASES_CONSTRAINTS).toArray();
        for (qsizetype i = 0; i < constraints.size(); ++i) {
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

    mCurveFilterMask.reset();
    mCurveFilterGP = {-std::numeric_limits<double>::infinity(), +std::numeric_limits<double>::infinity()};

    MainWindow::getInstance()->setConvergenceStatus(MCMCDiagnostic::ConvergenceStatus::eNone);
}

#pragma mark Curve filter mask

std::vector<MetropolisVariable*> Model::acquiredVariables()
{
    std::vector<MetropolisVariable*> vars;
    for (const auto& ev : mEvents) {
        vars.push_back(&ev->mTheta);
        vars.push_back(&ev->mS02Theta);
        for (auto&& d : ev->mDates) {
            vars.push_back(&d.mTi);
            vars.push_back(&d.mSigmaTi);
            vars.push_back(&d.mWiggle);
        }
    }
    for (const auto& ph : mPhases) {
        vars.push_back(&ph->mAlpha);
        vars.push_back(&ph->mBeta);
        vars.push_back(&ph->mDuration);
    }
    return vars;
}

void Model::setCurveFilterMask(std::vector<char> mask, std::pair<double, double> gpFilter)
{
    const bool allKept = std::all_of(mask.begin(), mask.end(), [](char c) { return c != 0; });
    if (mask.empty() || allKept) {
        mCurveFilterMask.reset();
        mCurveFilterGP = {-std::numeric_limits<double>::infinity(), +std::numeric_limits<double>::infinity()};
    } else {
        mCurveFilterMask = std::make_shared<const std::vector<char>>(std::move(mask));
        mCurveFilterGP = gpFilter;
    }
}

void Model::applyCurveFilterMask()
{
    std::size_t sumRaw = 0;
    for (const auto& ch : mChains)
        sumRaw += static_cast<std::size_t>(ch.mRealyAccepted);

    if (hasCurveFilter() && mCurveFilterMask->size() != sumRaw) {
        qWarning() << "[Model::applyCurveFilterMask] taille du masque incohérente ("
                   << mCurveFilterMask->size() << "au lieu de" << sumRaw << ") : filtre ignoré";
        mCurveFilterMask.reset();
    }

    // 1) mIterDisplay de chaque chaîne = nombre d'itérations retenues dans son segment
    std::size_t offset = 0;
    for (auto& ch : mChains) {
        const std::size_t n = static_cast<std::size_t>(ch.mRealyAccepted);
        if (hasCurveFilter()) {
            const auto first = mCurveFilterMask->begin() + offset;
            ch.mIterDisplay = static_cast<decltype(ch.mIterDisplay)>(std::count(first, first + n, char(1)));
        } else {
            ch.mIterDisplay = ch.mRealyAccepted;
        }
        offset += n;
    }

    // 2) Traces affichées de toutes les variables (mask nul -> filtre retiré)
    for (MetropolisVariable* v : acquiredVariables())
        v->applyCurveFilterMask(mCurveFilterMask);
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
        mNumberOfEvents = static_cast<int>(events.size());

        for (auto it = events.cbegin(); it != events.cend(); ++it) {
            // it pointe vers un QJsonValue
            if (!it->isObject())
                continue;
            QJsonObject JSONevent = it->toObject();

            if (JSONevent.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) {
                try {

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
                mEvents.push_back(std::make_shared<Bound>(JSONevent));

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

    // ------------------------------------------------------------
    // Rappel des conditions d'acquisition
    // ------------------------------------------------------------
    {
        const MCMCSettings& s = mMCMCSettings;

        long long totalAccepted = 0;
        long long totalDisplay  = 0;
        QStringList perChain;
        for (const auto& ch : mChains) {
            totalAccepted += ch.mRealyAccepted;
            totalDisplay  += ch.mIterDisplay;
            perChain << QString::number(ch.mRealyAccepted);
        }

        log += line(textBold(QObject::tr("Acquisition conditions")));
        log += line(QObject::tr("Number of chains : %1").arg(QString::number(mChains.size())));
        log += line(QObject::tr("Acquisition iterations per chain : %1").arg(QString::number(s.mIterPerAquisition)));
        log += line(QObject::tr("Thinning interval : %1").arg(QString::number(s.mThinningInterval)));
        log += line(QObject::tr("Retained samples : %1 (per chain : %2)")
                        .arg(QString::number(totalAccepted), perChain.join(" ; ")));
        if (hasCurveFilter())
            log += line(QObject::tr("Retained samples after curve filter : %1").arg(QString::number(totalDisplay)));

        if (s.mAnnealRecurrence <= 0) {
            log += line(QObject::tr("Annealing : disabled"));
        } else {
            log += line(QObject::tr("Annealing : Init. Temp. = %1 ; Recurrence = %2 ; Dwell = %3")
                            .arg(QString::number(s.mAnnealTemp),
                                 QString::number(s.mAnnealRecurrence),
                                 QString::number(s.mAnnealDwell)));
        }
        log += "<hr>";
    }

    auto totalTime = mChains[0].mAcquisitionElapsedTime
                     + mChains[0].mAdaptElapsedTime
                     + mChains[0].burnElapsedTime
                     + mChains[0].mInitElapsedTime;

    for (size_t j = 1 ; j < mChains.size(); j++) {
        totalTime += mChains[j].mAcquisitionElapsedTime
                     + mChains[j].mAdaptElapsedTime
                     + mChains[j].burnElapsedTime
                     + mChains[j].mInitElapsedTime;
    }
    log += line( QObject::tr("Total Elapsed time %1").arg(DHMS(totalTime)));
    log += "<hr>";

    int i = 1;
    for (const auto& chain : mChains) {
        log += line( QObject::tr("Elapsed acquisition time %1 for chain %2").arg(DHMS(chain.mAcquisitionElapsedTime), QString::number(i)));
        ++i;
    }

    log += "<hr>";
    QString convergenceColor;
    switch (mConvergenceSummary.status) {
    case MCMCDiagnostic::ConvergenceStatus::eGood:
        convergenceColor = QStringLiteral("#2e7d32"); // vert foncé
        break;
    case MCMCDiagnostic::ConvergenceStatus::eWarning:
        convergenceColor = QStringLiteral("#f9a825"); // orange/ambre
        break;
    case MCMCDiagnostic::ConvergenceStatus::eBad:
        convergenceColor = QStringLiteral("#c62828"); // Rouge foncé
        break;
    default:
        convergenceColor = QStringLiteral("#333333"); // Anthracite
        break;
    }

    log += line(QObject::tr("<span style=\"color:%1; font-weight:bold;\">%2</span>")
                    .arg(convergenceColor, mConvergenceSummary.label));

    log += line(QObject::tr("R\xCC\x82 : max = %1, mean = %2 (%3/%4 variable(s) ≥ Threshold, %5 %)")
                    .arg(mConvergenceSummary.maxRHat, 0, 'f', 4)
                    .arg(mConvergenceSummary.meanRHat, 0, 'f', 4)
                    .arg(mConvergenceSummary.nAboveGoodThreshold)
                    .arg(mConvergenceSummary.nVariables)
                    .arg(mConvergenceSummary.fractionAboveGoodThreshold * 100., 0, 'f', 2));

    if (mConvergenceSummary.hasEss) {
        log += line(QObject::tr("ESS : min = %1, mean = %2 (%3/%4 variable(s) &lt; Threshold,  %5 %)")
                        .arg(mConvergenceSummary.minESS, 0, 'f', 0)
                        .arg(mConvergenceSummary.meanESS, 0, 'f', 0)
                        .arg(mConvergenceSummary.nBelowGoodEssThreshold)
                        .arg(mConvergenceSummary.nEssVariables)
                        .arg(mConvergenceSummary.fractionBelowGoodEssThreshold * 100., 0, 'f', 2));
    }

    log += "<hr>";
    std::ranges::for_each(mPhases, [&log](std::shared_ptr<Phase> phase) {
        log += ModelUtilities::phaseResultsHTML(phase);
        log += "<br>";

        log += line(textBold(textOrange(QObject::tr("Duration (posterior distrib.)"))));

        log += line(textOrange(phase->mDuration.resultsString(QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"), false)));

        if (phase->mDuration.is_curve_filtering)
            log += line(textOrange(phase->mDuration.getFilteringString()));
        log += "<hr>";
    });

    std::ranges::for_each(mPhaseConstraints, [&log](std::shared_ptr<PhaseConstraint> phase_constraint) {
        log += ModelUtilities::constraintResultsHTML(phase_constraint);
        log += "<hr>";
    });

    for (const auto& ev : mEvents) {
        log += ModelUtilities::eventResultsHTML(ev, true, mSettings.getTminFormated(), mSettings.getTmaxFormated(), !getCurvesName().isEmpty());
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
        if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
             QStringList l = event->mTheta.getResultsList(locale, precision);
             maxHpd = std::max(maxHpd, ((int)l.size() - 9) / 3);
             l.prepend(event->getQStringName());
             rows << l;

        } else {
            QStringList l ;
            l << locale.toString(event->mTheta.mFormatedBurnAdaptTrace->at(0), 'f', precision);
            l.prepend(event->getQStringName());
            rows << l;
        }
    }

    // Dates
    rows << QStringList();
    for (std::shared_ptr<Event>& event : mEvents) {
        if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
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

namespace {
// Partie acquisition (trace brute, non filtrée) de la chaîne chainIdx, de taille mRealyAccepted.
// Depuis la séparation des traces, mBurnAdaptTrace ne contient QUE init + burn + adapt :
// l'acquisition est dans mAllAcquiredTrace (MetropolisVariable::acquire()).
std::vector<double> acquiredRunForChain(MetropolisVariable& var,
                                        const std::vector<ChainSpecs>& chains,
                                        std::size_t chainIdx)
{
    const std::size_t n = static_cast<std::size_t>(chains[chainIdx].mRealyAccepted);
    std::vector<double> tr = var.acquiredTraceforChain(chains, chainIdx);
    if (tr.size() != n) {
        qWarning() << "[Model::acquiredRunForChain]" << var.getQStringName()
                   << ": trace de taille" << tr.size() << "au lieu de" << n
                   << "(chaîne" << chainIdx << ")";
        tr.resize(n, std::numeric_limits<double>::quiet_NaN());
    }
    return tr;
}
} // namespace

QList<QStringList> Model::getPhasesTraces(const QLocale locale, const bool withDateFormat)
{
    QList<QStringList> rows;

    QStringList headers;
    headers << "iter";

    for (const auto& pPhase : mPhases)
        headers << pPhase->getQStringName() + " Begin" << pPhase->getQStringName() + " End";

    rows << headers;

    int shift = 0;
    std::size_t acqIndex = 0; // indice dans le masque du filtre de courbe
    for (std::size_t ci = 0; ci < mChains.size(); ++ci) {
        const ChainSpecs& chain = mChains[ci];
        const int burnAdaptSize = 1 + chain.mIterPerBurn + (chain.mBatchIndex * chain.mIterPerBatch);
        const int runSize = chain.mRealyAccepted;

        std::vector<std::vector<double>> alphas, betas;
        alphas.reserve(mPhases.size());
        betas.reserve(mPhases.size());
        for (const auto& pPhase : mPhases) {
            alphas.push_back(acquiredRunForChain(pPhase->mAlpha, mChains, ci));
            betas.push_back(acquiredRunForChain(pPhase->mBeta, mChains, ci));
        }

        // on parcourt la trace brute et on saute les itérations rejetées par le filtre de courbe
        for (int k = 0; k < runSize; ++k) {
            if (hasCurveFilter() && !(*mCurveFilterMask)[acqIndex++])
                continue;
            QStringList l;
            l << QString::number(shift + burnAdaptSize + k);
            for (std::size_t p = 0; p < mPhases.size(); ++p) {
                double valueAlpha = alphas[p][k];
                if (withDateFormat)
                    valueAlpha = DateUtils::convertToAppSettingsFormat(valueAlpha);
                l << locale.toString(valueAlpha, 'g', 15);

                double valueBeta = betas[p][k];
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

QList<QStringList> Model::getPhaseTrace(size_t phaseIdx, const QLocale locale, const bool withDateFormat)
{
    QList<QStringList> rows;

    if (phaseIdx >= mPhases.size())
        return QList<QStringList>();

    std::shared_ptr<Phase> phase = mPhases.at(phaseIdx);

    QStringList headers;
    headers << "iter" << phase->getQStringName() + " Begin" << phase->getQStringName() + " End";
    for (const auto& event : phase->mEvents)
        headers << event->getQStringName();

    rows << headers;

    int shift = 0;
    std::size_t acqIndex = 0; // indice dans le masque du filtre de courbe

    for (std::size_t ci = 0; ci < mChains.size(); ++ci) {
        const ChainSpecs& chain = mChains[ci];
        const int burnAdaptSize = 1 + chain.mIterPerBurn + (chain.mBatchIndex * chain.mIterPerBatch);
        const int runSize = chain.mRealyAccepted;

        const std::vector<double> alpha = acquiredRunForChain(phase->mAlpha, mChains, ci);
        const std::vector<double> beta  = acquiredRunForChain(phase->mBeta, mChains, ci);

        // theta des Events : variable fixe (Bound, theta fixé) -> valeur initiale constante
        std::vector<std::vector<double>> thetas;
        thetas.reserve(phase->mEvents.size());
        for (const auto& event : phase->mEvents) {
            if (event->mType == Event::eBound || event->mTheta.mSamplerProposal == SamplerProposal::eFixe) {
                const double v0 = (event->mTheta.mBurnAdaptTrace && !event->mTheta.mBurnAdaptTrace->empty())
                                      ? event->mTheta.mBurnAdaptTrace->front()
                                      : std::numeric_limits<double>::quiet_NaN();
                thetas.emplace_back(static_cast<std::size_t>(runSize), v0);
            } else {
                thetas.push_back(acquiredRunForChain(event->mTheta, mChains, ci));
            }
        }

        for (int k = 0; k < runSize; ++k) {
            if (hasCurveFilter() && !(*mCurveFilterMask)[acqIndex++])
                continue;
            QStringList l;
            l << QString::number(shift + burnAdaptSize + k);

            double valueAlpha = alpha[k];
            if (withDateFormat)
                valueAlpha = DateUtils::convertToAppSettingsFormat(valueAlpha);
            l << locale.toString(valueAlpha, 'g', 15);

            double valueBeta = beta[k];
            if (withDateFormat)
                valueBeta = DateUtils::convertToAppSettingsFormat(valueBeta);
            l << locale.toString(valueBeta, 'g', 15);

            for (const auto& th : thetas) {
                double value = th[k];
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

// créer un tableau avec les valeurs de tous les events, pendant l'acquisition.
// Même convention que getPhasesTraces / getPhaseTrace :
//   - trace brute (mAllAcquiredTrace, en BC/AD), itérations rejetées par le filtre de courbe sautées ;
//   - conversion au format de l'application seulement si withDateFormat ;
//   - "iter" = indice global dans la chaîne (init + burn + adapt + acquisition), décalé chaîne par chaîne.
QList<QStringList> Model::getEventsTraces(const QLocale& locale,
                                          bool withDateFormat) const
{
    QList<QStringList> rows;

    QStringList headers;
    headers << "iter";
    for (const auto& ev : mEvents)
        headers << ev->getQStringName();
    rows << headers;

    int shift = 0;
    std::size_t acqIndex = 0; // indice dans le masque du filtre de courbe

    for (std::size_t ci = 0; ci < mChains.size(); ++ci) {
        const ChainSpecs& chain = mChains[ci];
        const int burnAdaptSize = 1 + chain.mIterPerBurn + (chain.mBatchIndex * chain.mIterPerBatch);
        const int runSize = chain.mRealyAccepted;

        std::vector<std::vector<double>> thetas;
        thetas.reserve(mEvents.size());
        for (const auto& ev : mEvents) {
            if (ev->mType == Event::eBound || ev->mTheta.mSamplerProposal == SamplerProposal::eFixe) {
                // Bound / theta fixe : seule la valeur initiale est mémorisée
                const double v0 = (ev->mTheta.mBurnAdaptTrace && !ev->mTheta.mBurnAdaptTrace->empty())
                                      ? ev->mTheta.mBurnAdaptTrace->front()
                                      : std::numeric_limits<double>::quiet_NaN();
                thetas.emplace_back(static_cast<std::size_t>(runSize), v0);
            } else {
                thetas.push_back(acquiredRunForChain(ev->mTheta, mChains, ci));
            }
        }

        for (int k = 0; k < runSize; ++k) {
            if (hasCurveFilter() && !(*mCurveFilterMask)[acqIndex++])
                continue;
            QStringList l;
            l.reserve(1 + static_cast<qsizetype>(mEvents.size()));
            l << QString::number(shift + burnAdaptSize + k);

            for (const auto& th : thetas) {
                double value = th[k];
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

    // 3b - Un Event ne peut appartenir qu'à UNE SEULE phase à span uniforme (Phase::eZOnly) ;
    //      il peut en revanche appartenir en plus à d'autres phases sans span uniforme
    {
        std::unordered_map<const Event*, std::vector<const Phase*>> spanPhasesOfEvent;
        for (const auto& ph : mPhases) {
            if (ph->mTauType != Phase::eZOnly)
                continue;
            for (const auto& ev : ph->mEvents)
                spanPhasesOfEvent[ev.get()].push_back(ph.get());
        }

        for (const auto& ev : mEvents) { // ordre du modèle -> message reproductible
            const auto it = spanPhasesOfEvent.find(ev.get());
            if (it != spanPhasesOfEvent.end() && it->second.size() > 1) {
                QStringList names;
                for (const Phase* p : it->second)
                    names << "\" " + p->getQStringName() + " \"";

                throw QObject::tr("The event \" %1 \" cannot belong to several phases with a Uniform Span (%2)")
                    .arg(ev->getQStringName(), names.join(", "));
                return false;
            }
        }
    }

    // 4 - Pas de circularité sur les contraintes des Events
    std::vector<std::vector<std::shared_ptr<Event>> > eventBranches;
    try {
        eventBranches = ModelUtilities::getAllEventsBranches(mEvents);

    } catch(QString &error){
        throw; // relance le QString attrapé (throw &error lançait un QString*, non attrapé par les appelants)
    }

    // 5 - Pas de circularité sur les contraintes de phases
    // 6 - Gammas : sur toutes les branches, la somme des gamma min < plage d'étude :
    std::vector<std::vector<Phase*> > phaseBranches;
    try {
        phaseBranches = ModelUtilities::getAllPhasesBranches(mPhases, mSettings.mTmax - mSettings.mTmin);

    } catch(QString &error){
        throw; // relance le QString attrapé (throw &error lançait un QString*, non attrapé par les appelants)
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
                        lower = qMax(lower, dynamic_cast<Bound*>(evt.get())->value());

                    }
                }
                // Update bound interval

                if (bound->value() < lower)
                    throw QString(QObject::tr("The bound \" %1 \" has a fixed value inconsistent with previous bounds in chain!").arg(bound->getQStringName()));

                // --------------------
                // Check bound interval upper value
                // --------------------
                double upper = mSettings.mTmax;
                for (size_t k = j+1; k<branche_i.size(); ++k) {
                    std::shared_ptr<Event> evt = branche_i.at(k);
                    if (evt->type() == Event::eBound) {
                        upper = qMin(upper, dynamic_cast<Bound*>(evt.get())->value());
                    }
                }
                // Update bound interval
                if (bound->value() > upper)
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
                lower = qMax(lower, bound->value());

        }
        double upper = double (mSettings.mTmax);
        std::shared_ptr<Phase> phaseTo = phase_const->mPhaseTo;
        for (const auto& ev : phaseTo->mEvents) {
            Bound* bound = dynamic_cast<Bound*>(ev.get());
            if (bound)
                upper = qMin(upper, bound->value());

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
                        min = std::max(min, bound->value());
                        max = std::min(max, bound->value());

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

    // Parallélisé par variable (chaque appel n'écrit que dans sa variable)
    std::vector<std::function<void()>> tasks;

    for (const auto& event : mEvents ) {

        if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
            tasks.emplace_back([&chains, ev = event.get()] { ev->mTheta.generateCorrelations(chains); });
        }

        if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
            tasks.emplace_back([&chains, ev = event.get()] { ev->mS02Theta.generateCorrelations(chains); });
        }

        for (auto&& date : event->mDates ) {

            tasks.emplace_back([&chains, d = &date] { d->mTi.generateCorrelations(chains); });
            tasks.emplace_back([&chains, d = &date] { d->mSigmaTi.generateCorrelations(chains); });

        }

    }

    for (const auto& phase : mPhases ) {

        tasks.emplace_back([&chains, ph = phase.get()] { ph->mAlpha.generateCorrelations(chains); });
        tasks.emplace_back([&chains, ph = phase.get()] { ph->mBeta.generateCorrelations(chains); });

        //phase->mTau.generateCorrelations(chains); // ??? is it useful?
    }

    parallel_run(tasks);

#ifdef DEBUG
    qDebug() <<  "[Model::generateCorrelations] done in " + DHMS(t.elapsed());
#endif

}

#pragma mark FFTLength, Threshold, bandwidth

void Model::setBandwidth(BandwidthType bwt, const double bandwidth)
{

    if (mBandwidthType != bwt) {
        updateDensities(mFFTLength, bwt, bandwidth, mThreshold);
        // mBandwidth = bandwidth; // done in updateDensities
    }

    if (bwt == BandwidthType::eBWCustom && mBandwidth != bandwidth) {
        updateDensities(mFFTLength, bwt, bandwidth, mThreshold);
       // mBandwidth = bandwidth; // done in updateDensities
    }
}

void Model::setFFTLength(int FFTLength)
{
    if (mFFTLength != FFTLength) {
        updateDensities(FFTLength, mBandwidthType ,mBandwidth, mThreshold);
    }
}

void Model::setHActivity(const double h, const double rangePercent)
{
   if (!mPhases.empty()) {
        generateActivity(mFFTLength, h, mThreshold, rangePercent);
        mHActivity = h;
    }

}
/**
 * @brief Model::setThreshold this is a slot
 * @param threshold
 */
void Model::setThreshold(const double threshold)
{
    if (mThreshold != threshold) {
        updateDensities(mFFTLength, mBandwidthType ,mBandwidth, threshold);
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

                if (date.mDeltaType != Date::eDeltaNone)
                    date.mWiggle.mThresholdUsed = mThreshold;
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



#pragma mark Loop
/*
 * void Model::memo_accepted_state(const unsigned i_chain)
{
    for (const auto& event : mEvents) {
       //--------------------- Memo Events -----------------------------------------
       if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
            event->mTheta.memo_accepted_state(i_chain);

            event->mS02Theta.memo_accepted_state(i_chain);

            //--------------------- Memo Dates -----------------------------------------
            for (auto&& date : event->mDates )   {
                date.mTi.memo_accepted_state(i_chain);
                date.mSigmaTi.memo_accepted_state(i_chain);
                date.mWiggle.memo_accepted_state(i_chain);

            }
       }
    }
}
*/

/**
 * @brief Initialise les structures de stockage des paramètres pour chaque chaîne.
 *
 * Cette fonction prépare les conteneurs internes des différents objets
 * (`Event`, `Date`, …) afin qu’ils puissent accueillir les statistiques
 * d’acceptation de chaque chaîne MCMC.
 *
 * - **Taille du tampon d’acceptation** : le même `acceptBufferLen` est utilisé
 *   pour toutes les chaînes et correspond à `mChains[0].mIterPerBatch`.
 * - **Redimensionnement** : les vecteurs `mAcceptedStateCountByChain` sont
 *   redimensionnés à la taille actuelle de `mChains` pour chaque
 *   paramètre (`Theta`, `S02Theta`, `Ti`, `SigmaTi`, `Wiggle`).
 * - **Longueur du tampon** : la longueur du tampon d’acceptation
 *   (`mLastMHAcceptsLength`) est fixée à `acceptBufferLen` pour chaque
 *   paramètre afin d’éviter des réallocations pendant les itérations.
 *
 * @note
 *   Les appels à `clear()` et `reserve()` sont commentés car ils ne sont
 *   pas nécessaires : le redimensionnement ci‑dessus suffit à garantir que
 *   chaque chaîne possède son propre espace de comptage.
 *
 * @warning
 *   La fonction suppose que `mChains` n’est pas vide ; sinon l’accès
 *   `mChains[0]` provoquerait un comportement indéfini.
 *
 * @param[in,out] this  Instance du modèle (`Model`).
 *                     Les membres modifiés sont :
 *                     - `mEvents` (et leurs sous‑objets `mTheta`,
 *                       `mS02Theta`, `mDates`…)
 *                     - `mChains` (seulement la taille du tampon d’acceptation
 *                       est lue, aucune modification directe)
 *
 * @return Aucun (fonction `void`).
 *
 * @see Model::setParametersForChain() – implémentation détaillée.
 * @see Chain::mIterPerBatch – nombre d’itérations par lot, utilisé pour le
 *      calcul de `acceptBufferLen`.
 *
 */
void Model::setParametersForChain()
{
    // Today we have the same acceptBufferLen for every chain
    const int acceptBufferLen =  mChains[0].mIterPerBatch;

    for (auto&& event : mEvents) {
       //event->mTheta.clear();
       //event->mTheta.reserve(initReserve);
       event->mTheta.mAcceptedStateCountByChain.resize(mChains.size());

       //event->mTheta.mLastMHAccepts.reserve(acceptBufferLen);
       event->mTheta.mLastMHAcceptsLength = acceptBufferLen;

       //event->mS02Theta.clear();
       //event->mS02Theta.reserve(initReserve);
       event->mS02Theta.mAcceptedStateCountByChain.resize(mChains.size());
       //event->mS02Theta.mLastMHAccepts.reserve(acceptBufferLen);
       event->mS02Theta.mLastMHAcceptsLength = acceptBufferLen;

       // event->mTheta.mNbValuesAccepted.clear(); //don't clean, avalable for cumulate chain

       for (auto&& date : event->mDates) {
            //date.mTi.clear();
            //date.mTi.reserve(initReserve);
            date.mTi.mAcceptedStateCountByChain.resize(mChains.size());
            //date.mTi.mLastMHAccepts.reserve(acceptBufferLen);
            date.mTi.mLastMHAcceptsLength = acceptBufferLen;

            //date.mSigmaTi.clear();
            //date.mSigmaTi.reserve(initReserve);
            date.mSigmaTi.mAcceptedStateCountByChain.resize(mChains.size());
            //date.mSigmaTi.mLastMHAccepts.reserve(acceptBufferLen);
            date.mSigmaTi.mLastMHAcceptsLength = acceptBufferLen;

            //date.mWiggle.clear();
            //.mWiggle.reserve(initReserve);
            date.mWiggle.mAcceptedStateCountByChain.resize(mChains.size());
            //date.mWiggle.mLastMHAccepts.reserve(acceptBufferLen);
            date.mWiggle.mLastMHAcceptsLength = acceptBufferLen;
       }
    }

  /*  for (auto&& phase : mPhases) {
       phase->mAlpha.clear();
       phase->mBeta.clear();
       //phase->mTau.reset();
       phase->mDuration.clear();

       //phase->mAlpha.reserve(initReserve);
       //phase->mBeta.reserve(initReserve);
       //phase->mTau.mBurnAdaptTrace->reserve(initReserve);
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

    mHActivity = std::max(1.0, abs(mSettings.mTmax - mSettings.mTmin) / 100.0);
    clearThreshold();
    // memo the new value of the Threshold inside all the part of the model: phases, events and dates
    //updateDensities(mFFTLength, mBandwidth, 95.);
    remove_smoothed_densities();//clearPosteriorDensities();

    if (getProject_ptr()->mLoop)
        emit getProject_ptr()->mLoop->setMessage(QObject::tr("Computing posterior distributions and numerical results"));

    // Étapes de progression : fractions de la phase "calcul" du post-traitement,
    // [0, 0.15] étant pris par generateCorrelations() dans finalize().
    // Sans crochet installé (chargement d'un .res, etc.) ces appels sont sans effet.
    progressStage(QObject::tr("Computing trace statistics (R-hat, ESS, ...)"), 0.15, 0.55);
    updateFormatSettings(); // update mFormat for formatedCredibility and formatedTrace
    generateTraceNumericalResults(mChains); // stats pour formatedTrace

    progressStage(QObject::tr("Computing posterior densities"), 0.55, 0.80);
    generatePosteriorDensities(mChains, mFFTLength, mBandwidthType, mBandwidth);

    progressStage(QObject::tr("Computing HPD regions"), 0.80, 0.84);
    generateHPD(95.);
    progressStage(QObject::tr("Computing credibility intervals"), 0.84, 0.88);
    generateCredibility(95.);

    // memo the new value of the Threshold inside all the part of the model: phases, events and dates
    setThresholdToAllModel(95.);

    if (!mPhases.empty()) {
        progressStage(QObject::tr("Computing tempo"), 0.88, 0.91);
        generateTempo(mFFTLength);
        progressStage(QObject::tr("Computing activity"), 0.91, 0.94);
        generateActivity(mFFTLength, mHActivity, 95.);
    }

    progressStage(QObject::tr("Computing density statistics"), 0.94, 1.00);
    generateDensityNumericalResults();

}

void Model::updateDensities(int fftLen, BandwidthType bandwidthType, double bandwidth, double threshold)
{
    remove_smoothed_densities();//clearPosteriorDensities();

    if (getProject_ptr()->mLoop)
        emit getProject_ptr()->mLoop->setMessage(QObject::tr("Computing posterior distributions and numerical results"));


    updateFormatSettings(); // update mFormat for formatedCredibility and formatedTrace, reconstruit mFormatedAcquiredTrace

    // Vérifier si au moins une chaîne a mIterDisplay différent de mRealyAccepted, pour refaire les RHat et ESS
   /* bool is_curve_filtering = false;
    const auto& model = getModel_ptr();
    for (const auto &ch : model->mChains) {
        if (ch.mIterDisplay != ch.mRealyAccepted) {
            is_curve_filtering = true;
            break;
        }
    }
    if (is_curve_filtering)*/
    // Si les traces sont modifiés, il faut refaire le calcul
        generateTraceNumericalResults(mChains);


    generatePosteriorDensities(mChains, fftLen, bandwidthType, bandwidth);  // utilise les stats de Traces

    generateHPD(threshold); // utilise formatedAcquiredTrace
    generateCredibility(threshold);

    // memo the new value of the Threshold inside all the part of the model: phases, events and dates
    setThresholdToAllModel(threshold);

    if (!mPhases.empty()) {
        generateTempo(fftLen);
        generateActivity(fftLen, mHActivity, threshold);
    }

    generateDensityNumericalResults();

    mBandwidthType = bandwidthType;
    mBandwidth = bandwidth;
    mFFTLength = fftLen;

}

void Model::generatePosteriorDensities(const std::vector<ChainSpecs> &chains, int fftLen,  BandwidthType bwt, double bandwidth)
{
#ifdef DEBUG
    QElapsedTimer t;
    t.start();
#endif

    const double tmin = mSettings.getTminFormated();
    const double tmax = mSettings.getTmaxFormated();

    // Parallélisé : une tâche par variable d'Event, par Date (mTi, mSigmaTi, mWiggle)
    // et par Phase (alpha, beta, durée). Mêmes appels, même ordre à l'intérieur d'une tâche.
    std::vector<std::function<void()>> tasks;

    for (const auto& event : mEvents) {
        Event* ev = event.get();
        tasks.emplace_back([=, &chains] {
            ev->mTheta.setBandwidth(bwt, bandwidth);
            ev->mTheta.generateFormatedKDE(chains, fftLen, tmin, tmax);
        });

        tasks.emplace_back([=, &chains] {
            ev->mS02Theta.setBandwidth(bwt, bandwidth);
            const double S02Min = ev->mS02Theta.mResults.traceAnalysis.min;
            const double S02Max = ev->mS02Theta.mResults.traceAnalysis.max;
            ev->mS02Theta.generateFormatedKDE(chains, fftLen, S02Min, S02Max);
        });

       if (event->type() != Event::eBound) {
            for (auto&& date : event->mDates) {
                Date* d = &date;
                tasks.emplace_back([=, &chains] {
                    d->setBandwidth(bwt, bandwidth);
                    const double TiMin = d->mTi.mResults.traceAnalysis.min;
                    const double TiMax = d->mTi.mResults.traceAnalysis.max;

                    d->generateFormatedKDE(chains, fftLen, TiMin, TiMax);
                });
            }
       }

    }

    for (const auto& phase : mPhases) {
        Phase* ph = phase.get();
        tasks.emplace_back([=, &chains] {
            ph->setBandwidth(bwt, bandwidth);
            ph->generateFormatedKDE(chains, fftLen, tmin, tmax);
        });
    }

    parallel_run(tasks);

#ifdef DEBUG
    qDebug() <<  "=> Model::generatePosteriorDensities done in " + DHMS(t.elapsed());
#endif
}

/*
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
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
                event->mTheta.generateNumericalResults(chains);

                if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
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
         if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
            event->mTheta.generateNumericalResults(chains);

            if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
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
*/

void Model::generateDensityNumericalResults()
{
#ifdef DEBUG
    QElapsedTimer t;
    t.start();
#endif

    // Parallélisé : une tâche par Event (θ, S02, dates) et par Phase
    std::vector<std::function<void()>> tasks;

    for (const auto& event_sp : mEvents) {
        tasks.emplace_back([event = event_sp.get()] {
        if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
            event->mTheta.generateDensityNumericalResults();

            if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
                event->mS02Theta.generateDensityNumericalResults();

            for (auto&& date : event->mDates) {
                date.mTi.generateDensityNumericalResults();
                date.mSigmaTi.generateDensityNumericalResults();

                if (date.mDeltaType != Date::eDeltaNone)
                    date.mWiggle.generateDensityNumericalResults();
            }
        } else {
            event->mTheta.MetropolisVariable::generateDensityNumericalResults();
        }
        });
    }

    for (const auto& phase_sp : mPhases) {
        tasks.emplace_back([phase = phase_sp.get()] {
        phase->mAlpha.MetropolisVariable::generateDensityNumericalResults();
        phase->mBeta.MetropolisVariable::generateDensityNumericalResults();
        // phase->mTau.generateNumericalResults(chains);
        phase->mDuration.MetropolisVariable::generateDensityNumericalResults();
        });
    }

    parallel_run(tasks);


#ifdef DEBUG
    qDebug() <<  "[Model::generateDensityNumericalResults] done in " + DHMS(t.elapsed()) ;
#endif
}

void Model::generateTraceNumericalResults(const std::vector<ChainSpecs> &chains)
{
#ifdef DEBUG
    QElapsedTimer t;
    t.start();
#endif

    // Parallélisé à la granularité de la variable : c'est l'étape la plus coûteuse
    // (bw_SJ_ste, quartiles, R̂/ESS sur la trace globale + chaque chaîne).
    // `chains` n'est plus copié (capture par référence).
    std::vector<std::function<void()>> tasks;

    for (const auto& event : mEvents) {
        Event* ev = event.get();
        if (ev->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
            tasks.emplace_back([ev, &chains] { ev->mTheta.generateTraceNumericalResults(chains); });

            if (ev->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
                tasks.emplace_back([ev, &chains] { ev->mS02Theta.generateTraceNumericalResults(chains); });

            for (auto&& date : ev->mDates) {
                Date* d = &date;
                tasks.emplace_back([d, &chains] { d->mTi.generateTraceNumericalResults(chains); });
                tasks.emplace_back([d, &chains] { d->mSigmaTi.generateTraceNumericalResults(chains); });

                if (d->mDeltaType != Date::eDeltaNone)
                    tasks.emplace_back([d, &chains] { d->mWiggle.generateTraceNumericalResults(chains); });
            }
        } else {
            tasks.emplace_back([ev, &chains] { ev->mTheta.MetropolisVariable::generateTraceNumericalResults(chains); });
        }
    }

    for (const auto& phase : mPhases) {
        Phase* ph = phase.get();
        tasks.emplace_back([ph, &chains] { ph->mAlpha.generateTraceNumericalResults(chains); });
        tasks.emplace_back([ph, &chains] { ph->mBeta.generateTraceNumericalResults(chains); });
        // phase->mTau.generateNumericalResults(chains);
        tasks.emplace_back([ph, &chains] { ph->mDuration.generateTraceNumericalResults(chains); });
    }

    parallel_run(tasks);


    std::vector<double> Rhat;
    std::vector<double> ESS;
    for (size_t i = 0; i<mEvents.size(); i++) {
        const auto& event = mEvents[i];
        if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
            Rhat.push_back(event->mTheta.mResults.RhatESS.rHat);
            ESS.push_back(std::min(event->mTheta.mResults.RhatESS.bulkESS,
                                   event->mTheta.mResults.RhatESS.tailESS));

            if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
                Rhat.push_back( event->mS02Theta.mResults.RhatESS.rHat);
                ESS.push_back(std::min(event->mS02Theta.mResults.RhatESS.bulkESS,
                                       event->mS02Theta.mResults.RhatESS.tailESS));
            }
            for (auto&& date : event->mDates) {
                Rhat.push_back(date.mTi.mResults.RhatESS.rHat);
                ESS.push_back(std::min(date.mTi.mResults.RhatESS.bulkESS,
                                       date.mTi.mResults.RhatESS.tailESS));

                Rhat.push_back(date.mSigmaTi.mResults.RhatESS.rHat);
                ESS.push_back(std::min(date.mSigmaTi.mResults.RhatESS.bulkESS,
                                       date.mSigmaTi.mResults.RhatESS.tailESS));
            }
        }
    }

    // Les paramètres de phase ne sont pas échantillonnés
   /* for (size_t i = 0; i<mPhases.size(); i++) {
        const auto& phase = mPhases[i];
        Rhat.push_back(phase->mAlpha.mResults.RhatESS.rHat);
        ESS.push_back(std::min(phase->mAlpha.mResults.RhatESS.bulkESS,
                               phase->mAlpha.mResults.RhatESS.tailESS));

        Rhat.push_back(phase->mBeta.mResults.RhatESS.rHat);
        ESS.push_back(std::min(phase->mBeta.mResults.RhatESS.bulkESS,
                               phase->mBeta.mResults.RhatESS.tailESS));

        Rhat.push_back(phase->mDuration.mResults.RhatESS.rHat);
        ESS.push_back(std::min(phase->mDuration.mResults.RhatESS.bulkESS,
                               phase->mDuration.mResults.RhatESS.tailESS));

    }*/

    auto M = mChains.size();
    mConvergenceSummary = MCMCDiagnostic::computeConvergenceSummary(Rhat, ESS, M);
    std::cout << " Convergence Summary : " << mConvergenceSummary.label.toStdString() << std::endl;

#ifdef DEBUG
    qDebug() <<  "[Model::generateTraceNumericalResults] done in " + DHMS(t.elapsed()) ;
#endif
}

void Model::clearThreshold()
{
    mThreshold = -1.0;
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
    // Parallélisé : une tâche par Event et par Phase (tri des traces = coût dominant)
    std::vector<std::function<void()>> tasks;

    for (const auto& ev_sp : mEvents) {
        tasks.emplace_back([ev = ev_sp.get(), thresh] {
        if (ev->type() != Event::eBound)//(ev->mTheta.mSamplerProposal != SamplerProposal::eFixe)
            ev->mTheta.generateCredibility(thresh);

        if (ev->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
            ev->mS02Theta.generateCredibility(thresh);

        if (ev->type() != Event::eBound) {
            for (auto&& date : ev->mDates )  {
                date.mTi.generateCredibility(thresh);
                date.mSigmaTi.generateCredibility(thresh);

                if (date.mDeltaType != Date::eDeltaNone)
                    date.mWiggle.generateCredibility(thresh);
            }
        }
        });
    }

    for (const auto& phase_sp : mPhases) {
        tasks.emplace_back([phase = phase_sp.get(), thresh] {
        // If there is only one Event in the phase, there is no Duration
        phase->mAlpha.generateCredibility(thresh);
        phase->mBeta.generateCredibility(thresh);
        //  phase->mTau.generateCredibility(mChains, thresh);
        phase->mDuration.generateCredibility(thresh);

        // traces affichées (filtrées si filtre de courbe), appariées itération par itération
        const auto& alphaTrace = *phase->mAlpha.traceToDisplay();
        const auto& betaTrace = *phase->mBeta.traceToDisplay();
        phase->mTimeRange = timeRangeFromTraces( alphaTrace, betaTrace, thresh, "Time Range for Phase : " + phase->getQStringName());
        });
    }

    parallel_run(tasks);


    for (const auto& phaseConstraint : mPhaseConstraints) {
        const QString str = phaseConstraint->mPhaseFrom->getQStringName() + " to " + phaseConstraint->mPhaseTo->getQStringName();

        const auto& betaTrace = *phaseConstraint->mPhaseFrom->mBeta.traceToDisplay();
        const auto& alphaTrace = *phaseConstraint->mPhaseTo->mAlpha.traceToDisplay();

        phaseConstraint->mGapRange = gapRangeFromTraces(betaTrace, alphaTrace, thresh, "Gap Range : "+ str);

        qDebug() << "[Model::generateCredibility] Gap Range " << str;

        phaseConstraint->mTransitionRange = transitionRangeFromTraces(betaTrace, alphaTrace, thresh, "Transition Range : " + str);

    }


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

    // Parallélisé : une tâche par Event et par Phase
    std::vector<std::function<void()>> tasks;

    for (const auto& event_sp : mEvents
                                 | std::views::filter([](const auto& ev) {
                                       return ev->mTheta.mSamplerProposal != SamplerProposal::eFixe
                                              && ev->type() != Event::eBound;
                                   })) {
        tasks.emplace_back([event = event_sp.get(), thresh] {
        event->mTheta.generateHPD(thresh);
        if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
            event->mS02Theta.generateHPD(thresh);
        for (auto&& date : event->mDates) {
            date.mTi.generateHPD(thresh);
            date.mSigmaTi.generateHPD(thresh);

            if (date.mDeltaType != Date::eDeltaNone)
                date.mWiggle.generateHPD(thresh);
        }
        });
    }

    for (const auto& ph_sp : mPhases) {
        tasks.emplace_back([ph = ph_sp.get(), thresh] {
        ph->mAlpha.generateHPD(thresh);
        ph->mBeta.generateHPD(thresh);
        ph->mDuration.generateHPD(thresh);
        });
    }

    parallel_run(tasks);


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

        //const int nRealyAccepted = std::accumulate(mChains.begin(), mChains.end(), 0, [](double sum, ChainSpecs chain){return  sum + chain.mRealyAccepted;});
        const int nRealyAccepted = std::accumulate(mChains.begin(), mChains.end(), 0, [](double sum, ChainSpecs chain){return  sum + chain.mIterDisplay;});

        for (const auto& ev : phase->mEvents) {
            if (ev->mTheta.mSamplerProposal != SamplerProposal::eFixe) {

                const auto & rawtrace = *ev->mTheta.traceToDisplay();// .mAllAcquiredTrace;
                concaAllTrace.resize(concaAllTrace.size() + rawtrace.size());
                std::copy_backward( rawtrace.begin(), rawtrace.end(), concaAllTrace.end() );

            } else {

                const size_t begin_size = concaAllTrace.size();
                concaAllTrace.resize(concaAllTrace.size() + nRealyAccepted);
                std::fill_n( concaAllTrace.begin()+begin_size, nRealyAccepted, ev->mTheta.mBurnAdaptTrace->at(0));
            }

        }


        auto minmaxAll = std::minmax_element(concaAllTrace.begin(), concaAllTrace.end());
        double t_min_data = *minmaxAll.first;
        double t_max_data = *minmaxAll.second;

        // cas d'une borne seule dans la phase
        //if (t_min_data >= t_max_data) {
        //   t_max_data = mSettings.mTmax;
        //}
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
            double QInf = interpolate_value_from_curve(pT, Gx, 0.0, 1.0)*n ;
            double QSup = findOnOppositeCurve(pT, Gx)*n;
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
            phase->mActivity = phase->mEvents[0]->mTheta.mFormatedKDE;
            phase->mActivityInf = phase->mEvents[0]->mTheta.mFormatedKDE;
            phase->mActivitySup = phase->mEvents[0]->mTheta.mFormatedKDE;
            phase->mActivityUnifTheo = phase->mEvents[0]->mTheta.mFormatedKDE;
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
/**
 * @brief Model::peekResFormatTag
 * Regarde, sans les consommer, les 8 octets suivants du flux et indique s'ils
 * correspondent au marqueur @p tag. Les .res 4.0.0 écrits avant l'ajout du
 * compteur d'acceptation n'ont pas ce marqueur et sont relus en v338.
 */
bool Model::peekResFormatTag(QDataStream* in, quint64 tag)
{
    QIODevice* dev = in ? in->device() : nullptr;
    if (!dev || dev->isSequential())
        return false;

    const QByteArray raw = dev->peek(sizeof(quint64));
    if (raw.size() != static_cast<qsizetype>(sizeof(quint64)))
        return false;

    QDataStream probe(raw);
    probe.setByteOrder(in->byteOrder());
    probe.setVersion(in->version());
    quint64 v = 0;
    probe >> v;
    return v == tag;
}

void Model::saveToStream(QDataStream *out) const
{
   // Marqueur de format v4.0.0 : les MHVariable contiennent mMHAcceptcountSinceAquire
   *out << kResFormatTag_v400;

   // *out << quint32 (out->version());// we could add software version here << quint16(out.version());
   // *out << qApp->applicationVersion();

   // -------------------------------------------------
   // 2️⃣  Write info (chains)
   // -------------------------------------------------
   *out << static_cast<quint32>(mChains.size());
   for (const ChainSpecs &ch : mChains) {
       *out << ch.burnElapsedTime;
       *out << ch.mAdaptElapsedTime;
       *out << ch.mAcquisitionElapsedTime;
       *out << static_cast<quint32>(ch.mBatchIndex);
       *out << static_cast<quint32>(ch.mBatchIterIndex);
       *out << static_cast<quint32>(ch.mBurnIterIndex);
       *out << static_cast<quint32>(ch.mMaxBatchs);
       *out << ch.mMixingLevel;
       *out << static_cast<quint32>(ch.mIterPerBatch);
       *out << static_cast<quint32>(ch.mIterPerBurn);
       *out << static_cast<quint32>(ch.mIterPerAquisition);
       *out << static_cast<quint32>(ch.mAquisitionIterIndex);
       *out << static_cast<quint32>(ch.mSeed);
       *out << static_cast<quint32>(ch.mThinningInterval);
       *out << static_cast<quint32>(ch.mRealyAccepted);
       *out << static_cast<quint32>(ch.mTotalIter);
       // ch.mIterDisplay n’est pas persistant (dérivé)
   }
    // -----------------------------------------------------
    //  3️⃣ Writing phase data
    // -----------------------------------------------------
    for (const std::shared_ptr<Phase> &phase : mPhases) {
        *out << phase->mAlpha;
        *out << phase->mBeta;
        *out << phase->mTau;
        *out << phase->mDuration;
    }
    // -----------------------------------------------------
    //  Writing event data
    // -----------------------------------------------------
    for (const std::shared_ptr<Event>& event : mEvents) {
        *out << event->mTheta;
        *out << event->mS02Theta;
    }
    // -----------------------------------------------------
    //  Writing date data
    // -----------------------------------------------------
    for (const std::shared_ptr<Event>& event : mEvents) {
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

                *out << static_cast<quint32>(d.mCalibHPD.size());
                for (QMap<double, double>::const_iterator it = d.mCalibHPD.cbegin(); it!=d.mCalibHPD.cend();++it) {
                    *out << it.key();
                    *out << it.value();
                }
            }

        }
    }
    // -------------------------------------------------
    // 4️⃣  Logs
    // -------------------------------------------------
    *out << mLogModel;
    *out << mLogInit;
    *out << mLogAdapt;
    *out << mLogResults;
    // -------------------------------------------------
    // 4️⃣ bis  Masque du filtre de courbe (bloc optionnel, marqué)
    //   tag | quint64 taille | octets du masque | bornes G' du filtre
    //   Écrit seulement si un filtre est actif ; relu par peekResFormatTag.
    // -------------------------------------------------
    if (hasCurveFilter()) {
        *out << kResFilterTag_v400;
        const std::vector<char>& mask = *mCurveFilterMask;
        *out << static_cast<quint64>(mask.size());
        out->writeRawData(mask.data(), static_cast<int>(mask.size()));
        *out << mCurveFilterGP.first;
        *out << mCurveFilterGP.second;
    }
    // -------------------------------------------------
    // 5️⃣  Flush (optionnel mais explicite)
    // -------------------------------------------------
    // 2️⃣  Flush du périphérique sous‑jacent, uniquement s’il s’agit d’un QFile
    if (QFile *file = qobject_cast<QFile*>(out->device())) {
        file->flush();               // QFile::flush() existe
    }
}



/** @Brief Read the .res file, it's the result of the saved computation
 *
 * */
bool Model::loadFromStream_v323(QDataStream *in)
{
    std::cout << "[Model::loadFromStream_v323] Entering" << std::endl;

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
        e->mS02Theta.mSamplerProposal = SamplerProposal::eFixe;
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

bool Model::loadFromStream_v324(QDataStream *in)
{
    std::cout << "[Model::loadFromStream_v324] Entering"  << std::endl;

    const QList<QString> compatible_version {"3.2.4", "3.2.6"};

    if (in->version()!= QDataStream::Qt_6_4)
        return false;

#ifdef DEBUG
    if (res_file_version != qApp->applicationVersion())
        qDebug() << "[Model::loadFromStream_v324] Different Model version =" << res_file_version<<" actual =" << qApp->applicationVersion();

    if (compatible_version.contains(res_file_version))
        qDebug() << "[Model::loadFromStream_v324] Version compatible 3.2.4";
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
                    qDebug() << "[Model::loadFromStream_v324] mWiggleCalibration vide";

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

bool Model::loadFromStream_v328(QDataStream *in)
{
    std::cout << "[Model::loadFromStream_v328] Entering"  << std::endl;

    const QList<QString> compatible_version { "3.2.4", "3.2.6", "3.2.9"};

    if (in->version()!= QDataStream::Qt_6_4)
        return false;

#ifdef DEBUG
    if (res_file_version != qApp->applicationVersion())
        qDebug() << "[Model::loadFromStream_v328] Different Model version = " << res_file_version << " actual =" << qApp->applicationVersion();

    if (compatible_version.contains(res_file_version))
        qDebug() << " [Model::loadFromStream_v328] Version compatible 3.2.8";
#endif
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
        p->mAlpha.load_stream(*in);
        p->mBeta.load_stream(*in);
        p->mTau.load_stream(*in);
        p->mDuration.load_stream(*in);

    }
    // -----------------------------------------------------
    //  Read events data
    // -----------------------------------------------------

    for (std::shared_ptr<Event> &e : mEvents) {
        e->mTheta.load_stream_v338(*in);
        e->mS02Theta.load_stream_v338(*in); // since 2023-06-01 v3.2.3
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
                d.mTi.load_stream_v338(*in);
                d.mSigmaTi.load_stream_v338(*in);
                if (d.mDeltaType != Date::eDeltaNone)
                    d.mWiggle.load_stream_v338(*in);

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

bool Model::loadFromStream_v330(QDataStream *in)
{
    std::cout << "[Model::loadFromStream_v330] Entering" << std::endl;

    if (in->version()!= QDataStream::Qt_6_4)
        return false;

    try {

        // -----------------------------------------------------
        //  Read info
        // -----------------------------------------------------

        quint32 tmp32;
        *in >> tmp32;

        if (in->status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read variable tmp32");
        }
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
            ch.mIterDisplay = ch.mRealyAccepted;
            *in >> ch.mTotalIter;
            mChains.push_back(ch);
        }
        if (in->status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read variable Chain");
        }
        // -----------------------------------------------------
        //  Read phases data
        // -----------------------------------------------------

        for (std::shared_ptr<Phase> &p : mPhases) {
            p->mAlpha.load_stream_v337(*in);
            p->mBeta.load_stream_v337(*in);
            p->mTau.load_stream_v337(*in);
            p->mDuration.load_stream_v337(*in);

        }
        if (in->status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read variable Phase");
        }
        // -----------------------------------------------------
        //  Read events data
        // -----------------------------------------------------

        for (std::shared_ptr<Event> &e : mEvents) {
            e->mTheta.load_stream_v337(*in);
            e->mS02Theta.load_stream_v337(*in); // since 2023-06-01 v3.2.3
        }
        if (in->status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read variable Event");
        }
        // -----------------------------------------------------
        //  Read dates data
        // -----------------------------------------------------

        for (std::shared_ptr<Event> &event : mEvents) {
            if (event->mType == Event::eDefault )
                for (auto&& d : event->mDates) {

                    d.mTi.load_stream_v337(*in);
                    d.mSigmaTi.load_stream_v337(*in);
                    if (d.mDeltaType != Date::eDeltaNone)
                        d.mWiggle.load_stream_v337(*in);

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
        if (in->status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read variable Date");
        }

        *in >> mLogModel;
        *in >> mLogInit;
        *in >> mLogAdapt;
        *in >> mLogResults;

        if (in->status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read variable Log");
        }
        return true;

    }
    catch (const std::exception& e) {
        std::cout << "[Model::loadFromStream_v330]  " << e.what() << std::endl;
        return false;
    }
    /*catch (...) {
        std::cout << "[Model::loadFromStream_v330] Error " << std::endl;
        return false;
    }*/
}

bool Model::loadFromStream_v338(QDataStream *in)
{
    std::cout << "[Model::loadFromStream_v338] Entering" << std::endl;

    if (in->version()!= QDataStream::Qt_6_4)
        return false;

    try {

        // -----------------------------------------------------
        //  Read info
        // -----------------------------------------------------

        // -------------------------------------------------
        // 3️⃣  Lire les chaînes
        // -------------------------------------------------
        quint32 chainCount = 0;
        *in >> chainCount;
        if (in->status() != QDataStream::Ok) {
            qWarning() << "Erreur lecture du nombre de chaînes";
            return false;
        }
        mChains.clear();
        mChains.reserve(static_cast<size_t>(chainCount));
        for (quint32 i = 0; i < chainCount; ++i) {
            ChainSpecs ch;
            *in >> ch.burnElapsedTime;
            *in >> ch.mAdaptElapsedTime;
            *in >> ch.mAcquisitionElapsedTime;

            quint32 tmp;
            *in >> tmp; ch.mBatchIndex = static_cast<decltype(ch.mBatchIndex)>(tmp);
            *in >> tmp; ch.mBatchIterIndex = static_cast<decltype(ch.mBatchIterIndex)>(tmp);
            *in >> tmp; ch.mBurnIterIndex = static_cast<decltype(ch.mBurnIterIndex)>(tmp);
            *in >> tmp; ch.mMaxBatchs = static_cast<decltype(ch.mMaxBatchs)>(tmp);
            *in >> ch.mMixingLevel;
            *in >> tmp; ch.mIterPerBatch = static_cast<decltype(ch.mIterPerBatch)>(tmp);
            *in >> tmp; ch.mIterPerBurn = static_cast<decltype(ch.mIterPerBurn)>(tmp);
            *in >> tmp; ch.mIterPerAquisition = static_cast<decltype(ch.mIterPerAquisition)>(tmp);
            *in >> tmp; ch.mAquisitionIterIndex = static_cast<decltype(ch.mAquisitionIterIndex)>(tmp);
            *in >> tmp; ch.mSeed = static_cast<decltype(ch.mSeed)>(tmp);
            *in >> tmp; ch.mThinningInterval = static_cast<decltype(ch.mThinningInterval)>(tmp);
            *in >> tmp; ch.mRealyAccepted = static_cast<decltype(ch.mRealyAccepted)>(tmp);
            *in >> tmp; ch.mTotalIter = static_cast<decltype(ch.mTotalIter)>(tmp);
            // champ dérivé
            ch.mIterDisplay = ch.mRealyAccepted;
            if (in->status() != QDataStream::Ok) {
                qWarning() << "[Model::loadFromStream_v338] Erreur lecture chaîne n°" << i;
                return false;
            }
            mChains.push_back(ch);
        }
        if (in->status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read variable Chain");
        }
        // -----------------------------------------------------
        // 4️⃣ Read phases data
        // -----------------------------------------------------

        for (std::shared_ptr<Phase> &p : mPhases) {
            p->mAlpha.load_stream(*in);
            p->mBeta.load_stream(*in);
            p->mTau.load_stream(*in);
            p->mDuration.load_stream(*in);

        }
        if (in->status() != QDataStream::Ok) {
            qWarning() << "[Model::loadFromStream_v338] Erreur lecture phase";
            return false;

        }
        // -----------------------------------------------------
        //  5️⃣ Read events data
        // -----------------------------------------------------

        for (std::shared_ptr<Event> &e : mEvents) {
            e->mTheta.load_stream_v338(*in);
            e->mS02Theta.load_stream_v338(*in);
            if (in->status() != QDataStream::Ok) {
                qWarning() << "[Model::loadFromStream_v338] Erreur lecture event";
                return false;
            }
        }
        // -----------------------------------------------------
        //  6️⃣ Read dates data
        // -----------------------------------------------------

        for (std::shared_ptr<Event> &event : mEvents) {
            if (event->mType != Event::eDefault) continue;

            for (auto&& d : event->mDates) {

                d.mTi.load_stream_v338(*in);
                d.mSigmaTi.load_stream_v338(*in);
                if (d.mDeltaType != Date::eDeltaNone)
                    d.mWiggle.load_stream_v338(*in);

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

                // HPD map
                quint32 hpSize = 0;
                *in >> hpSize;
                d.mCalibHPD.clear();
                for (quint32 i = 0; i < hpSize; ++i) {
                    double key, value;
                    *in >> key >> value;
                    d.mCalibHPD.insert(key, value);
                }


                const std::string toFind = "WID::" + d.mUUID;

                if (d.mWiggleCalibration && !d.mWiggleCalibration->mVector.empty())
                    d.mWiggleCalibration = &(getProject_ptr()->mCalibCurves[toFind]);
                if (in->status() != QDataStream::Ok) {
                    qWarning() << "[Model::loadFromStream_v338] Erreur lecture date";
                    return false;
                }
            }
        }

        // -------------------------------------------------
        // 7️⃣  Logs
        // -------------------------------------------------
        *in >> mLogModel;
        *in >> mLogInit;
        *in >> mLogAdapt;
        *in >> mLogResults;

        if (in->status() != QDataStream::Ok) {
            qWarning() << "[Model::loadFromStream_v338] Erreur lecture logs";
            return false;
        }
        return true;

    }
    catch (const std::exception& e) {
        std::cout << "[Model::loadFromStream_v338]  " << e.what() << std::endl;
        return false;
    }
    /*catch (...) {
        std::cout << "[Model::loadFromStream_v338] Error " << std::endl;
        return false;
    }*/
}


/**
 * @brief Model::loadFromStream_v400
 * Identique à v338, mais les MHVariable sont lues avec load_stream_v400
 * (ajout de mMHAcceptcountSinceAquire). Fichiers .res écrits par la v4.0.0.
 */
bool Model::loadFromStream_v400(QDataStream *in)
{
    std::cout << "[Model::loadFromStream_v400] Entering" << std::endl;

    if (in->version()!= QDataStream::Qt_6_4)
        return false;

    try {
        // -----------------------------------------------------
        //  Marqueur de format v4.0.0
        // -----------------------------------------------------
        quint64 tag = 0;
        *in >> tag;
        if (in->status() != QDataStream::Ok || tag != kResFormatTag_v400) {
            qWarning() << "[Model::loadFromStream_v400] marqueur de format v4.0.0 absent ou invalide";
            return false;
        }


        // -----------------------------------------------------
        //  Read info
        // -----------------------------------------------------

        // -------------------------------------------------
        // 3️⃣  Lire les chaînes
        // -------------------------------------------------
        quint32 chainCount = 0;
        *in >> chainCount;
        if (in->status() != QDataStream::Ok) {
            qWarning() << "Erreur lecture du nombre de chaînes";
            return false;
        }
        mChains.clear();
        mChains.reserve(static_cast<size_t>(chainCount));
        for (quint32 i = 0; i < chainCount; ++i) {
            ChainSpecs ch;
            *in >> ch.burnElapsedTime;
            *in >> ch.mAdaptElapsedTime;
            *in >> ch.mAcquisitionElapsedTime;

            quint32 tmp;
            *in >> tmp; ch.mBatchIndex = static_cast<decltype(ch.mBatchIndex)>(tmp);
            *in >> tmp; ch.mBatchIterIndex = static_cast<decltype(ch.mBatchIterIndex)>(tmp);
            *in >> tmp; ch.mBurnIterIndex = static_cast<decltype(ch.mBurnIterIndex)>(tmp);
            *in >> tmp; ch.mMaxBatchs = static_cast<decltype(ch.mMaxBatchs)>(tmp);
            *in >> ch.mMixingLevel;
            *in >> tmp; ch.mIterPerBatch = static_cast<decltype(ch.mIterPerBatch)>(tmp);
            *in >> tmp; ch.mIterPerBurn = static_cast<decltype(ch.mIterPerBurn)>(tmp);
            *in >> tmp; ch.mIterPerAquisition = static_cast<decltype(ch.mIterPerAquisition)>(tmp);
            *in >> tmp; ch.mAquisitionIterIndex = static_cast<decltype(ch.mAquisitionIterIndex)>(tmp);
            *in >> tmp; ch.mSeed = static_cast<decltype(ch.mSeed)>(tmp);
            *in >> tmp; ch.mThinningInterval = static_cast<decltype(ch.mThinningInterval)>(tmp);
            *in >> tmp; ch.mRealyAccepted = static_cast<decltype(ch.mRealyAccepted)>(tmp);
            *in >> tmp; ch.mTotalIter = static_cast<decltype(ch.mTotalIter)>(tmp);
            // champ dérivé
            ch.mIterDisplay = ch.mRealyAccepted;
            if (in->status() != QDataStream::Ok) {
                qWarning() << "[Model::loadFromStream_v400] Erreur lecture chaîne n°" << i;
                return false;
            }
            mChains.push_back(ch);
        }
        if (in->status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read variable Chain");
        }
        // -----------------------------------------------------
        // 4️⃣ Read phases data
        // -----------------------------------------------------

        for (std::shared_ptr<Phase> &p : mPhases) {
            p->mAlpha.load_stream(*in);
            p->mBeta.load_stream(*in);
            p->mTau.load_stream(*in);
            p->mDuration.load_stream(*in);

        }
        if (in->status() != QDataStream::Ok) {
            qWarning() << "[Model::loadFromStream_v400] Erreur lecture phase";
            return false;

        }
        // -----------------------------------------------------
        //  5️⃣ Read events data
        // -----------------------------------------------------

        for (std::shared_ptr<Event> &e : mEvents) {
            e->mTheta.load_stream_v400(*in);
            e->mS02Theta.load_stream_v400(*in);
            if (in->status() != QDataStream::Ok) {
                qWarning() << "[Model::loadFromStream_v400] Erreur lecture event";
                return false;
            }
        }
        // -----------------------------------------------------
        //  6️⃣ Read dates data
        // -----------------------------------------------------

        for (std::shared_ptr<Event> &event : mEvents) {
            if (event->mType != Event::eDefault) continue;

            for (auto&& d : event->mDates) {

                d.mTi.load_stream_v400(*in);
                d.mSigmaTi.load_stream_v400(*in);
                if (d.mDeltaType != Date::eDeltaNone)
                    d.mWiggle.load_stream_v400(*in);

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

                // HPD map
                quint32 hpSize = 0;
                *in >> hpSize;
                d.mCalibHPD.clear();
                for (quint32 i = 0; i < hpSize; ++i) {
                    double key, value;
                    *in >> key >> value;
                    d.mCalibHPD.insert(key, value);
                }


                const std::string toFind = "WID::" + d.mUUID;

                if (d.mWiggleCalibration && !d.mWiggleCalibration->mVector.empty())
                    d.mWiggleCalibration = &(getProject_ptr()->mCalibCurves[toFind]);
                if (in->status() != QDataStream::Ok) {
                    qWarning() << "[Model::loadFromStream_v400] Erreur lecture date";
                    return false;
                }
            }
        }

        // -------------------------------------------------
        // 7️⃣  Logs
        // -------------------------------------------------
        *in >> mLogModel;
        *in >> mLogInit;
        *in >> mLogAdapt;
        *in >> mLogResults;

        if (in->status() != QDataStream::Ok) {
            qWarning() << "[Model::loadFromStream_v400] Erreur lecture logs";
            return false;
        }

        // -------------------------------------------------
        // 7️⃣ bis  Masque du filtre de courbe (optionnel)
        // -------------------------------------------------
        mCurveFilterMask.reset();
        mCurveFilterGP = {-std::numeric_limits<double>::infinity(), +std::numeric_limits<double>::infinity()};

        if (peekResFormatTag(in, kResFilterTag_v400)) {
            quint64 tag = 0, maskSize = 0;
            *in >> tag >> maskSize;
            std::vector<char> mask(static_cast<std::size_t>(maskSize));
            if (maskSize > 0 && in->readRawData(mask.data(), static_cast<int>(maskSize)) != static_cast<int>(maskSize)) {
                qWarning() << "[Model::loadFromStream_v400] Erreur lecture masque du filtre";
                return false;
            }
            std::pair<double, double> gp;
            *in >> gp.first >> gp.second;
            if (in->status() != QDataStream::Ok) {
                qWarning() << "[Model::loadFromStream_v400] Erreur lecture bornes du filtre";
                return false;
            }
            setCurveFilterMask(std::move(mask), gp);

        } else {
            // .res écrit avant la sauvegarde du masque : les traces filtrées des variables
            // ont été relues, mais pas les effectifs par chaîne -> le découpage par chaîne
            // serait faux. On retire le filtre (applyCurveFilterMask, appelé après le
            // chargement, remet les traces brutes) ; il suffit de relancer le filtrage.
            bool hadFilter = false;
            for (MetropolisVariable* v : acquiredVariables())
                hadFilter = hadFilter || v->is_curve_filtering;
            if (hadFilter) {
                qWarning() << "[Model::loadFromStream_v400] filtre de courbe sans masque sauvegardé : filtre retiré";
                mLogResults += "<br>" + QObject::tr("Warning: the curve filter saved with this file could not be restored; please rebuild the curve to filter again.") + "<br>";
            }
        }
        return true;

    }
    catch (const std::exception& e) {
        std::cout << "[Model::loadFromStream_v400]  " << e.what() << std::endl;
        return false;
    }
    /*catch (...) {
        std::cout << "[Model::loadFromStream_v400] Error " << std::endl;
        return false;
    }*/
}


bool Model::hasSelectedEvents()
{
   return std::any_of(mEvents.begin(), mEvents.end(), [](const std::shared_ptr<Event> e){return e->mIsSelected;});
}

bool Model::hasSelectedPhases()
{
    return std::any_of(mPhases.begin(), mPhases.end(), [](const std::shared_ptr<Phase> p){return p->mIsSelected;});
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



QString Model::initializeTheta()
{
        return QString();
}
