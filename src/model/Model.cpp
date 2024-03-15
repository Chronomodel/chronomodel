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

Model::Model(QObject *parent):
    QObject(parent),
    mProject(nullptr),
    mNumberOfPhases(0),
    mNumberOfEvents(0),
    mNumberOfDates(0),
    mThreshold(-1.),
    mBandwidth(0.9),
    mFFTLength(1024),
    mHActivity(1)
{
    
}

Model::Model(const QJsonObject &json, QObject* parent):
    QObject(parent),
    mProject(nullptr),
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
        mNumberOfPhases = phases.size();
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
                    mEvents.append(new Event(JSONevent, std::shared_ptr<Model>(this)));
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
                Bound* ek = new Bound(JSONevent, std::shared_ptr<Model>(this));
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
                mPhases[i]->mConstraintsNextPhases.append(mPhaseConstraints[j]);
            } else if (mPhaseConstraints.at(j)->mToId == phaseId) {
                mPhaseConstraints[j]->mPhaseTo = mPhases[i];
                mPhases[i]->mConstraintsPrevPhases.append(mPhaseConstraints[j]);
            }
        }

    }
}

Model::~Model()
{
    mProject = nullptr;
}

void Model::clear()
{
    mCurveName.clear();
    mCurveLongName.clear();
    // Deleting an event executes these main following actions :
    // - The Event MH variables are reset (freeing trace memory)
    // - The Dates MH variables are reset (freeing trace memory)
    // - The Dates are cleared
    //for (Event* &ev: mEvents) {
  /*  for (auto ev = mEvents.rbegin(); ev != mEvents.rend(); ev++) {
        // Event can be an Event or an EventCurve.
        // => do not delete it using ~Event(), because the appropriate destructor could be ~EventCurve().
        //delete ev;
        delete *ev;
    }
    
    for (EventConstraint* &ec : mEventConstraints) {
        delete ec;
        ec = nullptr;
    }
 */
    mEvents.clear();
    mEventConstraints.clear();

    mPhases.clear();
    mPhaseConstraints.clear();

    /*if (!mPhases.isEmpty()) {
        for (Phase* &ph: mPhases) {
           // if (ph) {
                delete ph;
                ph = nullptr;
           // }
        }
        mPhases.clear();
    }

    if (!mPhaseConstraints.isEmpty()) {
        for (PhaseConstraint* &pc : mPhaseConstraints) {
           // if (pc) {
                delete pc; //->~PhaseConstraint();
                pc = nullptr;
           // }
        }
        mPhaseConstraints.clear();
    }*/

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
        mSettings = StudyPeriodSettings::fromJson(settings);
    }

    if (json.contains(STATE_MCMC)) {
        const QJsonObject mcmc = json.value(STATE_MCMC).toObject();
        mMCMCSettings = MCMCSettings::fromJson(mcmc);
        mChains = mMCMCSettings.getChains();
    }

    if (json.contains(STATE_PHASES)) {
        const QJsonArray phases = json.value(STATE_PHASES).toArray();
        mNumberOfPhases = phases.size();
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
                    mEvents.append(new Event(JSONevent, std::shared_ptr<Model>(this)));
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
                Bound* ek = new Bound(JSONevent, std::shared_ptr<Model>(this));
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
                mPhases[i]->mConstraintsNextPhases.append(mPhaseConstraints[j]);
            } else if (mPhaseConstraints.at(j)->mToId == phaseId) {
                mPhaseConstraints[j]->mPhaseTo = mPhases[i];
                mPhases[i]->mConstraintsPrevPhases.append(mPhaseConstraints[j]);
            }
        }

    }

}

void Model::setProject( Project* project)
{
    mProject = project;
    mCurveName.clear();
    mCurveLongName.clear();

    if (!mProject || !mProject->isCurve()) {
        return;
    }

    //ModelCurve* curveModel = dynamic_cast<ModelCurve*>(mProject->mModel.get());
    const auto &curveModel = mProject->mModel;

    const QString xLabel = curveModel->mCurveSettings.XLabel();
    const QString yLabel = curveModel->mCurveSettings.YLabel();
    const QString zLabel = curveModel->mCurveSettings.ZLabel();

    const QString x_short_name = curveModel->mCurveSettings.X_short_name();
    const QString y_short_name = curveModel->mCurveSettings.Y_short_name();
    const QString z_short_name = curveModel->mCurveSettings.Z_short_name();

    switch (curveModel->mCurveSettings.mProcessType) {

    case CurveSettings::eProcess_2D:
    case CurveSettings::eProcess_Spherical:
    case CurveSettings::eProcess_Unknwon_Dec:
        mCurveName.append({x_short_name, y_short_name});
        mCurveLongName.append({xLabel, yLabel});
        break;

    case CurveSettings::eProcess_3D:
    case CurveSettings::eProcess_Vector:
        mCurveName.append({x_short_name, y_short_name, z_short_name});
        mCurveLongName.append({xLabel, yLabel, zLabel});
        break;

    case CurveSettings::eProcess_None:
    case CurveSettings::eProcess_Univariate:
    case CurveSettings::eProcess_Inclination:
    case CurveSettings::eProcess_Declination:
    case CurveSettings::eProcess_Field:
    case CurveSettings::eProcess_Depth:
    default:
        mCurveName.append(x_short_name);
        mCurveLongName.append(xLabel);
        break;
    }

}


/**
 * @brief ResultsView::updateModel Update Design
 */
void Model::updateDesignFromJson()
{
    if (!mProject)
        return;
    setProject(mProject); // update mCurveName
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
    std::ranges::for_each(mPhases, [&log](Phase* phase) {
        log += ModelUtilities::phaseResultsHTML(phase);
        log += "<br>";
        log += line(textBold(textOrange(QObject::tr("Duration (posterior distrib.)"))));
        log += line(textOrange(phase->mDuration.resultsString(QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"))));
        log += "<hr>";
    });

    std::ranges::for_each(mPhaseConstraints, [&log](PhaseConstraint* phase_constraint) {
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
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(phase->mName + " Begin");
        rows << l;

        l = phase->mBeta.getResultsList(locale, precision);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(phase->mName + " End");
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
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
             QStringList l = event->mTheta.getResultsList(locale, precision);
             maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
             l.prepend(event->mName);
             rows << l;

        } else {
            QStringList l ;
            l << locale.toString(event->mTheta.mFormatedTrace->at(0), 'f', precision);
            l.prepend(event->mName);
            rows << l;
        }
    }

    // Dates
    rows << QStringList();
    for (Event*& event : mEvents) {
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
            for (int j = 0; j < event->mDates.size(); ++j) {
                Date& date = event->mDates[j];

                QStringList l = date.mTi.getResultsList(locale, precision);
                maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
                l.prepend(date.mName);
                rows << l;
            }
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
                double value;
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
            throw QObject::tr("The model must contain at least 3 Events");
            return false;
    }

    // 2 - The event must contain at least 1 data
    for (int i = 0; i < mEvents.size(); ++i) {
        if (mEvents.at(i)->type() == Event::eDefault && mEvents.at(i)->mDates.size() == 0) {
                    throw QObject::tr("The event  \" %1 \" must contain at least 1 data").arg(mEvents.at(i)->mName);
                    return false;

        }
    }

    // 3 - The phase must contain at least 1 event
    for (const Phase* ph: mPhases) {
        if (ph->mEvents.size() == 0) {
            throw QObject::tr("The phase \" %1 \" must contain at least 1 event").arg(ph->mName);
            return false;
        }
    }

    // 4 - Pas de circularité sur les contraintes des Event
    QList<QList<Event*> > eventBranches;
    try {
        eventBranches = ModelUtilities::getAllEventsBranches(mEvents);
    } catch(QString &error){
        throw &error;
    }

    // 5 - Pas de circularité sur les contraintes de phases
    // 6 - Gammas : sur toutes les branches, la somme des gamma min < plage d'étude :
    QList<QList<Phase*> > phaseBranches;
    try {
        phaseBranches = ModelUtilities::getAllPhasesBranches(mPhases, mSettings.mTmax - mSettings.mTmin);
    } catch(QString &error){
        throw &error;
    }

    // 7 - Un Event ne peut pas appartenir à 2 phases en contrainte
    for (const auto &branche_i :  phaseBranches) {
        QList<Event*> branchEvents;
        for (auto phase : branche_i ) {
             for (const auto& ev : phase->mEvents) {
                if (!branchEvents.contains(ev)) {
                    branchEvents.append(ev);
                } else
                    throw QString(QObject::tr("The event \" %1 \" cannot belong to several phases in a same branch!").arg(ev->mName));
            }
        }
    }

    // 8 - Bounds : vérifier cohérence des bornes en fonction des contraintes de Events (page 2)
    //  => Modifier les bornes des intervalles des bounds !! (juste dans le modèle servant pour le calcul)
    for (const auto &branche_i : eventBranches) {
        for (auto j = 0; j<branche_i.size(); j++) {
            Event* event = branche_i.at(j);
            if (event->type() == Event::eBound)  {
                Bound* bound = dynamic_cast<Bound*>(event);

                // --------------------
                // Check bound interval lower value
                // --------------------

                // On vérifie toutes les bornes avant et on prend le max
                // de leurs valeurs fixes ou du début de leur intervalle :
                double lower = double (mSettings.mTmin);
                for (auto k = 0; k<j; ++k) {
                    Event* evt = branche_i.at(k);
                    if (evt->type() == Event::eBound) {
                        lower = qMax(lower, dynamic_cast<Bound*>(evt)->mFixed);

                    }
                }
                // Update bound interval

                if (bound->mFixed < lower)
                    throw QString(QObject::tr("The bound \" %1 \" has a fixed value inconsistent with previous bounds in chain!").arg(bound->mName));

                // --------------------
                // Check bound interval upper value
                // --------------------
                double upper = mSettings.mTmax;
                for (auto k = j+1; k<branche_i.size(); ++k) {
                    Event* evt = branche_i.at(k);
                    if (evt->type() == Event::eBound) {
                        upper = qMin(upper, dynamic_cast<Bound*>(evt)->mFixed);
                    }
                }
                // Update bound interval
                if (bound->mFixed > upper)
                    throw QString(QObject::tr("The bound \" %1 \" has a fixed value inconsistent with next bounds in chain!").arg(bound->mName));

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
        const Phase* phaseFrom = phase_const->mPhaseFrom;
        for (const auto& ev : phaseFrom->mEvents) {
            Bound* bound = dynamic_cast<Bound*>(ev);
            if (bound)
                lower = qMax(lower, bound->mFixed);

        }
        double upper = double (mSettings.mTmax);
        Phase* phaseTo = phase_const->mPhaseTo;
        for (const auto& ev : phaseTo->mEvents) {
            Bound* bound = dynamic_cast<Bound*>(ev);
            if (bound)
                upper = qMin(upper, bound->mFixed);

            //bound = nullptr;
        }
        if (gammaMin >= (upper - lower))
            throw QString(QObject::tr("The constraint between phases \" %1 \" and \" %2 \" is not consistent with the bounds they contain!").arg(phaseFrom->mName, phaseTo->mName));

        phaseFrom = nullptr;
        phaseTo = nullptr;
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
                    Bound* bound = dynamic_cast<Bound*>(ev);
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
                    throw QString(QObject::tr("The phase \" %1 \" has a duration inconsistent with the bounds it contains!").arg(phase->mName));
            }
        }
    }

    // 11 - Vérifier la cohérence entre les contraintes d'Event et de Phase
    for (const auto &branche_i : phaseBranches) {
        for (const auto &phase : branche_i) {
            for (const auto &event : phase->mEvents) {
                bool phaseFound = false;

                // On réinspecte toutes les phases de la branche et on vérifie que le fait n'a pas de contrainte en contradiction avec les contraintes de phase !
                for (const Phase* p : branche_i) {
                    if (p == phase)
                        phaseFound = true;

                    else {
                        for (const auto &e : p->mEvents) {
                            // Si on regarde l'élément d'un phase d'avant, l'Event ne peut pas être en contrainte vers un Event de cette phase
                            if (!phaseFound) {
                                for (const auto & evBwd : e->mConstraintsBwd) {
                                    if (evBwd->mEventFrom == event)
                                        throw QObject::tr("The event %1  (in phase %2 ) is before the event %3 (in phase %4), BUT the phase %5 is after the phase %6 .\r=> Contradiction !").arg(event->mName, phase->mName, e->mName, p->mName, phase->mName, p->mName) ;

                                }
                            } else {
                                for (const auto &evFwd : e->mConstraintsFwd) {
                                    if (evFwd->mEventTo == event)
                                        throw QObject::tr("The event %1 (in phase %2) is after the event %3  (in phase %4), BUT the phase %5 is before the phase %6 .\r=> Contradiction !").arg(event->mName, phase->mName, e->mName, p->mName, phase->mName, p->mName);
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
void Model::generateCorrelations(const QList<ChainSpecs> &chains)
{
#ifdef DEBUG
    qDebug()<<"[Model::generateCorrelations] in progress";
    QElapsedTimer t;
    t.start();
#endif

    for (const auto& event : mEvents ) {

#ifdef USE_THREAD
        std::thread thTheta ([event] (QList<ChainSpecs> ch) {event->mTheta.generateCorrelations(ch);}, chains);
#else
        event->mTheta.generateCorrelations(chains);
#endif
        if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
            event->mS02Theta.generateCorrelations(chains);


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
    qDebug() <<  "=> [Model::generateCorrelations] done in " + DHMS(t.elapsed());
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

void Model::setHActivity(const double h, const double rangePercent)
{
   if (!mPhases.isEmpty()) {
        generateActivity(mFFTLength, h, mThreshold, rangePercent);
        mHActivity = h;

        emit newCalculus();
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
    int initReserve = 0;

    for (auto&& c: mChains)
       initReserve += ( 1 + (c.mMaxBatchs*c.mIterPerBatch) + c.mIterPerBurn + (c.mIterPerAquisition/c.mThinningInterval) );

    for (auto&& event : mEvents) {
       event->mTheta.reset();
       event->mTheta.reserve(initReserve);
       event->mTheta.mAllAccepts.resize(mChains.size());
       event->mTheta.mLastAccepts.reserve(acceptBufferLen);
       event->mTheta.mLastAcceptsLength = acceptBufferLen;

       event->mS02Theta.reset();
       event->mS02Theta.reserve(initReserve);
       event->mS02Theta.mAllAccepts.resize(mChains.size());
       event->mS02Theta.mLastAccepts.reserve(acceptBufferLen);
       event->mS02Theta.mLastAcceptsLength = acceptBufferLen;

       // event->mTheta.mAllAccepts.clear(); //don't clean, avalable for cumulate chain

       for (auto&& date : event->mDates) {
            date.mTi.reset();
            date.mTi.reserve(initReserve);
            date.mTi.mAllAccepts.resize(mChains.size());
            date.mTi.mLastAccepts.reserve(acceptBufferLen);
            date.mTi.mLastAcceptsLength = acceptBufferLen;

            date.mSigmaTi.reset();
            date.mSigmaTi.reserve(initReserve);
            date.mSigmaTi.mAllAccepts.resize(mChains.size());
            date.mSigmaTi.mLastAccepts.reserve(acceptBufferLen);
            date.mSigmaTi.mLastAcceptsLength = acceptBufferLen;

            date.mWiggle.reset();
            date.mWiggle.reserve(initReserve);
            date.mWiggle.mAllAccepts.resize(mChains.size());
            date.mWiggle.mLastAccepts.reserve(acceptBufferLen);
            date.mWiggle.mLastAcceptsLength = acceptBufferLen;
       }
    }

    for (auto&& phase : mPhases) {
       phase->mAlpha.reset();
       phase->mBeta.reset();
       //phase->mTau.reset();
       phase->mDuration.reset();

       phase->mAlpha.mRawTrace->reserve(initReserve);
       phase->mBeta.mRawTrace->reserve(initReserve);
       //phase->mTau.mRawTrace->reserve(initReserve);
       phase->mDuration.mRawTrace->reserve(initReserve);
    }
}



#pragma mark Densities

void Model::initNodeEvents()
{
    std::ranges::for_each( mEvents, [](Event* ev) {
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
    clearPosteriorDensities();

    if (mProject->mLoop)
        emit mProject->mLoop->setMessage(QObject::tr("Computing posterior distributions and numerical results - Credibility "));
    generateCredibility(threshold);

    updateFormatSettings(); // update formatedCredibility and formatedTrace

    if (mProject->mLoop)
        emit mProject->mLoop->setMessage(QObject::tr("Computing posterior distributions and numerical results - Posterior Densities"));

    generatePosteriorDensities(mChains, fftLen, bandwidth);


    if (mProject->mLoop)
        emit mProject->mLoop->setMessage(QObject::tr("Computing posterior distributions and numerical results - HPD "));

    generateHPD(threshold);

    // memo the new value of the Threshold inside all the part of the model: phases, events and dates
    setThresholdToAllModel(threshold);

    if (!mPhases.isEmpty()) {
        if (mProject->mLoop)
            emit mProject->mLoop->setMessage(QObject::tr("Computing posterior distributions and numerical results - Tempo"));
         generateTempo(fftLen);
    }

    if (!mPhases.isEmpty()) {
        if (mProject->mLoop)
            emit mProject->mLoop->setMessage(QObject::tr("Computing posterior distributions and numerical results - Activity"));
         generateActivity(fftLen, mHActivity, threshold);
    }

    if (mProject->mLoop)
        emit mProject->mLoop->setMessage(QObject::tr("Computing posterior distributions and numerical results - Numerical Results"));
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

        if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
            event->mS02Theta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);

        //if (event->mTheta.mSamplerProposal != MHVariable::eFixe)
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

    std::ranges::for_each( mEvents, [chains](Event* event) {
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


    std::ranges::for_each( mPhases, [chains](Phase* phase) {
         phase->mAlpha.generateNumericalResults(chains);
         phase->mBeta.generateNumericalResults(chains);
         // phase->mTau.generateNumericalResults(chains);
         phase->mDuration.generateNumericalResults(chains);
    });


#endif

#ifdef DEBUG
    qDebug() <<  "=> Model::generateNumericalResults done in " + DHMS(t.elapsed()) ;
#endif
}

void Model::clearThreshold()
{
    std::ranges::for_each( mEvents, [](Event* ev) {
         ev->mTheta.mThresholdUsed = -1.;
         ev->mS02Theta.mThresholdUsed = -1.;

         for (auto&& date : ev->mDates) {
            date.mTi.mThresholdUsed = -1.;
            date.mSigmaTi.mThresholdUsed = -1.;
         }
    });

    std::ranges::for_each( mPhases, [](Phase* phase) {
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
                                                  phase->mBeta.fullRunRawTrace(mChains), thresh, "Time Range for Phase : " + phase->mName);
    }


    for (const auto& phaseConstraint : mPhaseConstraints) {
        const QString str = phaseConstraint->mPhaseFrom->mName + " to " + phaseConstraint->mPhaseTo->mName;

        phaseConstraint->mGapRange = gapRangeFromTraces(phaseConstraint->mPhaseFrom->mBeta.fullRunRawTrace(mChains),
                                                        phaseConstraint->mPhaseTo->mAlpha.fullRunRawTrace(mChains), thresh, "Gap Range : "+ str);

        qDebug()<<"[Model::generateCredibility] Gap Range "<<str;

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

            for (int j = 0; j<event->mDates.size(); ++j) {
                Date& date = event->mDates[j];
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

void Model::generateTempo(size_t gridLength)
{
#ifdef DEBUG
    qDebug()<<QString("[Model::generateTempo] in progress");
    QElapsedTimer tClock;
    tClock.start();
#endif

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
        double t_min_data = *minmaxAll.first;
        double t_max_data = *minmaxAll.second;
        // cas d'une borne seule dans la phase
        if (t_min_data >= t_max_data) {
           // t_min_data = mSettings.mTmin;
            t_max_data = mSettings.mTmax;
        }
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
        QList<double> infT;
        QList<double> supT;

        QList<double> espT;
        double pT, eT, vT, infpT;

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


/**
 *  @brief Clear model data
 */
void Model::clearPosteriorDensities()
{
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent != mEvents.end()) {
        for (auto&& date : (*iterEvent)->mDates) {
            date.mTi.mFormatedHisto.clear();
            date.mSigmaTi.mFormatedHisto.clear();
            date.mTi.mChainsHistos.clear();
            date.mSigmaTi.mChainsHistos.clear();
        }
        (*iterEvent)->mTheta.mFormatedHisto.clear();
        (*iterEvent)->mTheta.mChainsHistos.clear();

        (*iterEvent)->mS02Theta.mFormatedHisto.clear();
        (*iterEvent)->mS02Theta.mChainsHistos.clear();

        ++iterEvent;
    }

    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase != mPhases.end()) {
        (*iterPhase)->mAlpha.mFormatedHisto.clear();
        (*iterPhase)->mBeta.mFormatedHisto.clear();
       // (*iterPhase)->mTau.mFormatedHisto.clear();
        (*iterPhase)->mDuration.mFormatedHisto.clear();

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
 * @brief Model::generateActivity. If there is only one Event, the activity is equal to it
 * @param gridLenth
 * @param h defined in year, if h<0 then h= delta_t \f$ \delta_t = (t_max - t_min + 4h )/(gridLenth) \f$
 */
void Model::generateActivity(const size_t gridLength, const double h, const double threshold, const double rangePercent)
{
    for (const auto& phase : mPhases) {
        // Curves for error binomial
        const int n = phase->mEvents.size();
        if (n<2) {
            phase->mActivity = phase->mEvents[0]->mTheta.mFormatedHisto;
            phase->mActivityInf = phase->mEvents[0]->mTheta.mFormatedHisto;
            phase->mActivitySup = phase->mEvents[0]->mTheta.mFormatedHisto;
            phase->mActivityUnifTheo = phase->mEvents[0]->mTheta.mFormatedHisto;
            continue;
        }
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

    out->setVersion(QDataStream::Qt_6_4);

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
    for (Phase*& phase : mPhases) {
        *out << phase->mAlpha;
        *out << phase->mBeta;
        *out << phase->mTau;
        *out << phase->mDuration;
    }
    // -----------------------------------------------------
    //  Writing event data
    // -----------------------------------------------------
    for (Event*& event : mEvents){
        *out << event->mTheta;
        *out << event->mS02Theta;
    }
    // -----------------------------------------------------
    //  Writing date data
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
void Model::restoreFromFile_v323(QDataStream *in)
{
    int QDataStreamVersion;
    *in >> QDataStreamVersion;
    in->setVersion(QDataStreamVersion);

    if (in->version()!= QDataStream::Qt_6_4)
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

    for (Phase* &p : mPhases) {
        *in >> p->mAlpha;
        *in >> p->mBeta;
        *in >> p->mTau;
        *in >> p->mDuration;
    }
    // -----------------------------------------------------
    //  Read events data
    // -----------------------------------------------------

    for (Event* &e : mEvents) {
        *in >> e->mTheta;
        e->mS02Theta.mSamplerProposal = MHVariable::eFixe;
        *in >> e->mS02Theta; // since 2023-06-01 v3.2.3
    }
    // -----------------------------------------------------
    //  Read dates data
    // -----------------------------------------------------

    for (Event*& event : mEvents) {
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

                if (d.mWiggleCalibration == nullptr || d.mWiggleCalibration->mVector.isEmpty()) {
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

}

void Model::restoreFromFile_v324(QDataStream *in)
{
    QList<QString> compatible_version;
    compatible_version<<"3.2.4"<<"3.2.6";
    int QDataStreamVersion;
    *in >> QDataStreamVersion;
    in->setVersion(QDataStreamVersion);

    if (in->version()!= QDataStream::Qt_6_4)
        return;

    //QString appliVersion;
    //res_file_version
    *in >> res_file_version;
    // prepare the future
    //QStringList projectVersionList = appliVersion.split(".");
#ifdef DEBUG
    if (res_file_version != qApp->applicationVersion())
        qDebug()<<" Different Model version ="<<res_file_version<<" actual ="<<qApp->applicationVersion();
    if (compatible_version.contains(res_file_version))
        qDebug()<<" Version compatible ";
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

    for (Phase* &p : mPhases) {
        *in >> p->mAlpha;
        *in >> p->mBeta;
        *in >> p->mTau;
        *in >> p->mDuration;
    }
    // -----------------------------------------------------
    //  Read events data
    // -----------------------------------------------------

    for (Event* &e : mEvents) {
        *in >> e->mTheta;
        *in >> e->mS02Theta; // since 2023-06-01 v3.2.3
    }
    // -----------------------------------------------------
    //  Read dates data
    // -----------------------------------------------------

    for (Event*& event : mEvents) {
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

                if (d.mWiggleCalibration == nullptr || d.mWiggleCalibration->mVector.isEmpty()) {
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

std::vector<t_reduceTime> Model::reduceTime(const std::vector<double> &vec_t) const
{
    const double tmin = mSettings.mTmin;
    const double tmax = mSettings.mTmax;

    std::vector<t_reduceTime> res;
    for (auto& t : vec_t)
        res.push_back((t - tmin) / (tmax - tmin));

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
