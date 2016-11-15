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
#include <QJsonArray>
#include <QtWidgets>
#include <QtCore/QStringList>

#pragma mark Constructor...
Model::Model():
mProject(0),
mNumberOfPhases(0),
mNumberOfEvents(0),
mNumberOfDates(0),
mNumberOfEventsInAllPhases(0),
mNumberOfDatesInAllPhases(0)
{
    
}

Model::~Model()
{

}

void Model::clear()
{
    this->clearTraces();
    if(!mEvents.isEmpty()) {
        foreach (Event* ev, mEvents) {
            if(ev)
                delete ev;
            ev = 0;
        }
        mEvents.clear();
     }

    if(!mPhases.isEmpty()) {
        foreach (Phase* ph, mPhases) {
            if(ph)
                delete ph;
            ph = 0;
        }
        mPhases.clear();
    }

    if(!mPhaseConstraints.isEmpty()) {
        /*foreach (PhaseConstraint* ph, mPhaseConstraints) {
            //if(ph) delete ph;
            ph = 0;
        }*/
        mPhaseConstraints.clear();
    }

    if(!mEventConstraints.isEmpty()) {
        /*foreach (EventConstraint* ev, mEventConstraints) {
            //if(ev) delete ev;
            ev = 0;
        }*/
        mEventConstraints.clear();
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
void Model::updateFormatSettings(const AppSettings* appSet)
{
    for (int i=0; i<this->mEvents.size(); i++) {
        Event* event= mEvents[i] ;
        event->mTheta.setFormat(appSet->mFormatDate);

        for (int j=0; j<event->mDates.size(); j++) {
            Date& date = event->mDates[j];
            date.mTheta.setFormat(appSet->mFormatDate);
            date.mSigma.setFormat(DateUtils::eNumeric);
            date.mWiggle.setFormat(appSet->mFormatDate);
        }
        event = 0;
    }
    for (int i=0; i<this->mPhases.size(); i++) {
        Phase* phase = mPhases[i] ;
        phase->mAlpha.setFormat(appSet->mFormatDate);
        phase->mBeta.setFormat(appSet->mFormatDate);
        phase->mDuration.setFormat(DateUtils::eNumeric);
        phase = 0;
    }
}

#pragma mark JSON conversion

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
            //p = 0;
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
                    Event* e = new Event(Event::fromJson(event));
                    e->mMixingLevel = mMCMCSettings.mMixingLevel;
                    mNumberOfDates += e->mDates.size();

                    for (int j=0; j<e->mDates.size(); ++j) {
                        e->mDates[j].mMixingLevel = e->mMixingLevel;
                        e->mDates[j].mColor = e->mColor;

                    }
                    mEvents.append(e);
                    //e = 0;
                }
                catch(QString error){
                    QMessageBox message(QMessageBox::Critical,
                                        qApp->applicationName() + " " + qApp->applicationVersion(),
                                        QObject::tr("Error : ") + error,
                                        QMessageBox::Ok,
                                        qApp->activeWindow(),
                                        Qt::Sheet);
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
            int phaseId = mPhases.at(j)->mId;
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
        int phaseId = mPhases.at(i)->mId;
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
        mNumberOfEventsInAllPhases += mPhases.at(i)->mEvents.size();
        for (int k=0; k<mPhases.at(i)->mEvents.size();++k) {
            mNumberOfDatesInAllPhases += mPhases.at(i)->mEvents.at(k)->mDates.size();
        }
    }
    //return model;
}

void Model::setProject( Project * project)
{
    mProject = project;
}

void Model::updateDesignFromJson(const QJsonObject& json)
{
    const QJsonArray phasesJSON = json.value(STATE_PHASES).toArray();
    if(mPhases.size() != phasesJSON.size())
        return;

    for (int i = 0; i < phasesJSON.size(); ++i) {
        const QJsonObject phaseJS = phasesJSON.at(i).toObject();
        Phase * p = mPhases[i];
        p->mName = phaseJS.value(STATE_NAME).toString();
        p->mColor = QColor(phaseJS.value(STATE_COLOR_RED).toInt(),phaseJS.value(STATE_COLOR_GREEN).toInt(),phaseJS.value(STATE_COLOR_BLUE ).toInt()) ;
    }

    const QJsonArray eventsJSON = json.value(STATE_EVENTS).toArray();
    if(mEvents.size() != eventsJSON.size())
        return;
    for (int i = 0; i < eventsJSON.size(); ++i) {
        const QJsonObject eventJS = eventsJSON.at(i).toObject();
        Event * e = mEvents[i];
        e->mName = eventJS.value(STATE_NAME).toString();
        e->mColor = QColor(eventJS.value(STATE_COLOR_RED).toInt(),eventJS.value(STATE_COLOR_GREEN).toInt(),eventJS.value(STATE_COLOR_BLUE ).toInt()) ;

        QJsonArray datesJS = eventJS.value(STATE_EVENT_DATES).toArray();
        if (e->mDates.size() != datesJS.size())
            return;
        for (int j = 0; j < datesJS.size(); ++j) {
            QJsonObject dateJS = datesJS.at(j).toObject();
            e->mDates[j].mName = dateJS.value(STATE_NAME).toString();
        }
    }
}

QJsonObject Model::toJson() const
{
    QJsonObject json;
    
    json["settings"] = mSettings.toJson();
    json["mcmc"] = mMCMCSettings.toJson();
    
    QJsonArray events;
    for (int i = 0; i < mEvents.size(); ++i)
        events.append(mEvents.at(i)->toJson());
    json["events"] = events;
    
    QJsonArray phases;
    for (int i = 0; i < mPhases.size(); ++i)
        phases.append(mPhases.at(i)->toJson());
    json["phases"] = phases;
    
    QJsonArray event_constraints;
    for (int i = 0; i < mEventConstraints.size(); ++i)
        event_constraints.append(mEventConstraints.at(i)->toJson());
    json["event_constraints"] = event_constraints;
    
    QJsonArray phase_constraints;
    for (int i = 0; i < mPhaseConstraints.size(); ++i)
        phase_constraints.append(mPhaseConstraints.at(i)->toJson());
    json["phase_constraints"] = phase_constraints;
    
    return json;
}

#pragma mark Logs
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
    
    for (int i = 0; i < mEvents.size(); ++i) {
        QString objType = "Event";
        if (mEvents.at(i)->type() == Event::eKnown) {
            log += line(textRed("Bound (" + QString::number(i+1) + "/" + QString::number(mEvents.size()) + ") : " + mEvents.at(i)->mName + " (" +
                                 QString::number(mEvents.at(i)->mPhases.size()) + " phases, " +
                                 QString::number(mEvents.at(i)->mConstraintsBwd.size()) + " const. back., " +
                                 QString::number(mEvents.at(i)->mConstraintsFwd.size()) + " const. fwd.)"));
        } else {
            log += line(textBlue("Event (" + QString::number(i+1) + "/" + QString::number(mEvents.size()) + ") : " + mEvents.at(i)->mName + " (" +
                                 QString::number(mEvents.at(i)->mDates.size()) + " data, " +
                                 QString::number(mEvents.at(i)->mPhases.size()) + " phases, " +
                                 QString::number(mEvents.at(i)->mConstraintsBwd.size()) + " const. back., " +
                                 QString::number(mEvents.at(i)->mConstraintsFwd.size()) + " const. fwd.)" +
                                 "<br>- Method : " + ModelUtilities::getEventMethodText(mEvents.at(i)->mMethod)));
        }
        
        for (int j = 0; j < mEvents.at(i)->mDates.size(); ++j) {
            log += "<br>";
            log += line(textBlack("Data (" + QString::number(j+1) + "/" + QString::number(mEvents.at(i)->mDates.size()) + ") : " + mEvents.at(i)->mDates.at(j).mName +
                                  "<br>- Type : " + mEvents.at(i)->mDates.at(j).mPlugin->getName() +
                                  "<br>- Method : " + ModelUtilities::getDataMethodText(mEvents.at(i)->mDates.at(j).mMethod) +
                                  "<br>- Params : " + mEvents.at(i)->mDates.at(j).getDesc()));
        }
        log += "<hr>";
        log += "<br>";
    }
    
    for (int i = 0; i < mPhases.size(); ++i) {
        log += line(textPurple("Phase (" + QString::number(i+1) + "/" + QString::number(mPhases.size()) + ") : " + mPhases[i]->mName + " (" + QString::number(mPhases[i]->mEvents.size()) + " events)"));
        log += "<br>";
        
        for(int j=0; j<mPhases.at(i)->mEvents.size(); ++j)
            log += line(textBlue("Event : " + mPhases.at(i)->mEvents.at(j)->mName));

        log += "<hr>";
        log += "<br>";
    }
    mLogModel = log;
    
    
    /*qDebug() << "=> Phases : " << model->mPhases.size();
     for(int i=0; i<model->mPhases.size(); ++i)
     {
     qDebug() << "  => Phase " << model->mPhases[i]->mId << " : " << model->mPhases[i]->mEvents.size() << " events"
     << " : " << model->mPhases[i]->mConstraintsBwd.size() << " const. back."
     << " : " << model->mPhases[i]->mConstraintsFwd.size() << " const. fwd.";
     }
     qDebug() << "=> Event Constraints : " << model->mEventConstraints.size();
     for(int i=0; i<model->mEventConstraints.size(); ++i)
     {
     qDebug() << "  => E. Const. " << model->mEventConstraints[i]->mId
     << " : event " << model->mEventConstraints[i]->mEventFrom->mId << "(" + model->mEventConstraints[i]->mEventFrom->mName + ")"
     << " to " << model->mEventConstraints[i]->mEventTo->mId << "(" + model->mEventConstraints[i]->mEventTo->mName + ")";
     }
     qDebug() << "=> Phase Constraints : " << model->mPhaseConstraints.size();
     for(int i=0; i<model->mPhaseConstraints.size(); ++i)
     {
     qDebug() << "  => P. Const. " << model->mPhaseConstraints[i]->mId
     << " : phase " << model->mPhaseConstraints[i]->mPhaseFrom->mId
     << " to " << model->mPhaseConstraints[i]->mPhaseTo->mId;
     }
     qDebug() << "===========================================";*/
}

QString Model::getResultsLog() const{
    return mLogResults;
}

void Model::generateResultsLog()
{
    QString log;
    for (int i = 0; i < mEvents.size(); ++i) {
        Event* event = mEvents.at(i);
        log += ModelUtilities::eventResultsHTML(event, true, this);
        log += "<hr>";
        event = 0;
    }
    for (int i = 0; i < mPhases.size(); ++i) {
        Phase* phase = mPhases.at(i);
        log += ModelUtilities::phaseResultsHTML(phase);
        log += "<hr>";
        phase = 0;
    }
    for (int i = 0; i < mPhaseConstraints.size(); ++i) {
        PhaseConstraint* phaseConstraint = mPhaseConstraints.at(i);
        log += ModelUtilities::constraintResultsHTML(phaseConstraint);
        log += "<hr>";
        phaseConstraint = 0;
    }
    mLogResults = log;
}

#pragma mark Results CSV
QList<QStringList> Model::getStats(const QLocale locale, const bool withDateFormat)
{
    QList<QStringList> rows;
    
    int maxHpd = 0;
    
    // Phases
    for (int i = 0; i < mPhases.size(); ++i) {
        Phase* phase = mPhases.at(i);
        
        QStringList l = phase->mAlpha.getResultsList(locale);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(phase->mName + " alpha");
        rows << l;
        
        l = phase->mBeta.getResultsList(locale);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(phase->mName + " beta");
        rows << l;
        phase = 0;
    }
    
    // Events
    rows << QStringList();
    for (int i = 0; i < mEvents.size(); ++i) {
        Event* event = mEvents.at(i);
        
        QStringList l = event->mTheta.getResultsList(locale);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(event->mName);
        rows << l;
        event = 0;
    }
    
    // Dates
    rows << QStringList();
    for (int i = 0; i < mEvents.size(); ++i) {
        Event* event = mEvents.at(i);
        for (int j = 0; j < event->mDates.size(); ++j) {
            Date& date = event->mDates[j];
            
            QStringList l = date.mTheta.getResultsList(locale);
            maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
            l.prepend(date.mName);
            rows << l;
        }
        event = 0;
    }
    
    // Headers
    QStringList list;
    list << "" << "MAP" << "Mean" << "Std dev" << "Q1" << "Q2" << "Q3" << "Credibility %" << "Credibility start" << "Credibility end";
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
    
    int runSize = 0;
    for(int i=0; i<mChains.size(); ++i)
        runSize += mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
    
    
    QStringList headers;
    headers << "iter";
    for (int j = 0; j < mPhases.size(); ++j)
        headers << mPhases.at(j)->mName + " alpha" << mPhases.at(j)->mName + " beta";

    rows << headers;
    
    int shift = 0;
    for (int i = 0; i < mChains.size(); ++i) {
        int burnAdaptSize = mChains.at(i).mNumBurnIter + (mChains.at(i).mBatchIndex * mChains.at(i).mNumBatchIter);
        int runSize = mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
        
        for (int j = burnAdaptSize; j<burnAdaptSize + runSize; ++j) {
            QStringList l;
            l << QString::number(shift + j);
            for (int k = 0; k < mPhases.size(); ++k) {
                Phase* phase = mPhases.at(k);
                double valueAlpha = phase->mAlpha.mRawTrace->at(shift + j);
                
                if (withDateFormat)
                    valueAlpha = DateUtils::convertToAppSettingsFormat(valueAlpha);
                l << locale.toString(valueAlpha, 'g', 15);

                double valueBeta = phase->mBeta.mRawTrace->at(shift + j);
                
                if (withDateFormat)
                    valueBeta = DateUtils::convertToAppSettingsFormat(valueBeta);
                
                l << locale.toString(valueBeta, 'g', 15);
                phase = 0;

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
    
    Phase* phase = 0;
    if (phaseIdx >= 0 && phaseIdx < mPhases.size())
        phase = mPhases.value(phaseIdx);

    else
        return QList<QStringList>();

    
    int runSize = 0;
    for (int i = 0; i < mChains.size(); ++i)
        runSize += mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
    
    QStringList headers;
    headers << "iter" << phase->mName + " alpha" << phase->mName + " beta";
    for (int i = 0; i < phase->mEvents.size(); ++i) {
        Event* event = phase->mEvents.at(i);
        headers << event->mName;
        event = 0;
    }
    rows << headers;
    
    int shift = 0;
    for (int i = 0; i<mChains.size(); ++i) {
        int burnAdaptSize = mChains.at(i).mNumBurnIter + (mChains.at(i).mBatchIndex * mChains.at(i).mNumBatchIter);
        int runSize = mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
        
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

            for (int k = 0; k < phase->mEvents.size(); ++k) {
                Event* event = phase->mEvents.at(k);
                double value = event->mTheta.mRawTrace->at(shift + j);
                if (withDateFormat)
                    value = DateUtils::convertToAppSettingsFormat(value);
                
                l << locale.toString(value, 'g', 15);
                event = 0;
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
    for (int i = 0; i<mChains.size(); ++i)
        runSize += mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
    
    
    QStringList headers;
    headers << "iter";
    for (int i = 0; i < mEvents.size(); ++i) {
        Event* event = mEvents.at(i);
        headers << event->mName;
        event = 0;
    }
    rows << headers;
    
    int shift = 0;
    for (int i = 0; i < mChains.size(); ++i) {
        const int burnAdaptSize = mChains.at(i).mNumBurnIter + (mChains.at(i).mBatchIndex * mChains.at(i).mNumBatchIter);
        const int runSize = mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
        
        for (int j = burnAdaptSize; j < burnAdaptSize + runSize; ++j) {
            QStringList l;
            l << QString::number(shift + j) ;
            for (int k = 0; k < mEvents.size(); ++k) {
                Event* event = mEvents.at(k);
                double value = event->mTheta.mRawTrace->at(shift + j);
                if (withDateFormat)
                    value = DateUtils::convertToAppSettingsFormat(value);
                
                l << locale.toString(value, 'g', 10);
                event = 0;
            }
            rows << l;
        }
        shift += burnAdaptSize + runSize;
    }
    return rows;
}

#pragma mark Model validity
bool Model::isValid()
{
    // 1 - At least one event is required in a model
    if (mEvents.size() == 0)
        throw QObject::tr("At least one event is required");
    
    // 2 - The event must contain at least 1 data
    for (int i = 0; i < mEvents.size(); ++i) {
        if (mEvents.at(i)->type() == Event::eDefault) {
            if (mEvents.at(i)->mDates.size() == 0)
                throw QObject::tr(" The event") + " \"" + mEvents.at(i)->mName + "\" " + QObject::tr("must contain at least 1 data");
        }
    }
    
    // 3 - The phase must contain at least 1 event
    for (int i=0; i<mPhases.size(); ++i) {
        if (mPhases.at(i)->mEvents.size() == 0)
            throw QObject::tr("The phase") + " \"" + mPhases.at(i)->mName + "\" " + QObject::tr("must contain at least 1 event");
    }
    
    // 4 - Pas de circularité sur les contraintes de faits
    QVector<QVector<Event*> > eventBranches;
    try {
        eventBranches = ModelUtilities::getAllEventsBranches(mEvents);
    } catch(QString error){
        throw error;
    }
    
    // 5 - Pas de circularité sur les contraintes de phases
    // 6 - Gammas : sur toutes les branches, la somme des gamma min < plage d'étude :
    QVector<QVector<Phase*> > phaseBranches;
    try {
        phaseBranches = ModelUtilities::getAllPhasesBranches(mPhases, mSettings.mTmax - mSettings.mTmin);
    } catch(QString error){
        throw error;
    }
    
    // 7 - Un fait ne paut pas appartenir à 2 phases en contrainte
    for (int i = 0; i < phaseBranches.size(); ++i) {
        QVector<Event*> branchEvents;
        for (int j = 0; j < phaseBranches.at(i).size(); ++j) {
            Phase* phase = phaseBranches[i][j];
            for (int k = 0; k<phase->mEvents.size(); ++k) {
                if (!branchEvents.contains(phase->mEvents.at(k))) {
                    branchEvents.append(phase->mEvents.at(k));
                    //qDebug() << phase->mEvents[k]->mName << " in " << phase->mName;
                } else
                    throw QString("The event \"" + phase->mEvents.at(k)->mName + "\" cannot belong to several phases in a same branch!");
            }
            phase = 0;
        }
    }
    
    // 8 - Bounds : verifier cohérence des bornes en fonction des contraintes de faits (page 2)
    //  => Modifier les bornes des intervalles des bounds !! (juste dans le modèle servant pour le calcul)
    for(int i=0; i<eventBranches.size(); ++i)
    {
        for(int j=0; j<eventBranches.at(i).size(); ++j)
        {
            Event* event = eventBranches[i][j];
            if(event->type() == Event::eKnown)
            {
                EventKnown* bound = dynamic_cast<EventKnown*>(event);

                // --------------------
                // Check bound interval lower value
                // --------------------
                
                // On vérifie toutes les bornes avant et on prend le max
                // de leurs valeurs fixes ou du début de leur intervalle :
                double lower = (double) mSettings.mTmin;
                for(int k=0; k<j; ++k) {
                    Event* evt = eventBranches[i][k];
                    if(evt->mType == Event::eKnown) {
                        EventKnown* bd = dynamic_cast<EventKnown*>(evt);
                        if(bd->mKnownType == EventKnown::eFixed)
                            lower = qMax(lower, bd->mFixed);
                        else if(bd->mKnownType == EventKnown::eUniform)
                            lower = qMax(lower, bd->mUniformStart);
                    }
                }
                // Update bound interval
                if(bound->mKnownType == EventKnown::eFixed && bound->mFixed < lower)
                {
                    throw QString("The bound \"" + bound->mName + "\" has a fixed value inconsistent with previous bounds in chain!");
                }
                else if(bound->mKnownType == EventKnown::eUniform)
                    bound->mUniformStart = qMax(bound->mUniformStart, lower);

                
                // --------------------
                // Check bound interval upper value
                // --------------------
                double upper = (double) mSettings.mTmax;
                for(int k=j+1; k<eventBranches.at(i).size(); ++k) {
                    Event* evt = eventBranches[i][k];
                    if(evt->mType == Event::eKnown){
                        EventKnown* bd = dynamic_cast<EventKnown*>(evt);
                        if(bd->mKnownType == EventKnown::eFixed)
                            upper = qMin(upper, bd->mFixed);
                        else if(bd->mKnownType == EventKnown::eUniform)
                            upper = qMin(upper, bd->mUniformEnd);
                    }
                }
                // Update bound interval
                if(bound->mKnownType == EventKnown::eFixed && bound->mFixed > upper)
                {
                    throw QString("The bound \"" + bound->mName + "\" has a fixed value inconsistent with next bounds in chain!");
                }
                else if(bound->mKnownType == EventKnown::eUniform) {
                    bound->mUniformEnd = qMin(bound->mUniformEnd, upper);
                    if(bound->mUniformStart >= bound->mUniformEnd)
                    {
                        throw QString("The bound \"" + bound->mName + "\" has an inconsistent range with other related bounds!");
                    }
                }
            }
            event = 0;
        }
    }
    
    // 9 - Gamma min (ou fixe) entre 2 phases doit être inférieur à la différence entre : le min des sups des intervalles des bornes de la phase suivante ET le max des infs des intervalles des bornes de la phase précédente
    for(int i=0; i<mPhaseConstraints.size(); ++i)
    {
        double gammaMin = 0.;
        PhaseConstraint::GammaType gType = mPhaseConstraints.at(i)->mGammaType;
        if(gType == PhaseConstraint::eGammaFixed)
            gammaMin = mPhaseConstraints[i]->mGammaFixed;
        else if(gType == PhaseConstraint::eGammaRange)
            gammaMin = mPhaseConstraints.at(i)->mGammaMin;
        
        double lower = (double) mSettings.mTmin;
        Phase* phaseFrom = mPhaseConstraints.at(i)->mPhaseFrom;
        for(int j=0; j<phaseFrom->mEvents.size(); ++j)
        {
            EventKnown* bound = dynamic_cast<EventKnown*>(phaseFrom->mEvents[j]);
            if(bound) {
                if(bound->mKnownType == EventKnown::eFixed)
                    lower = qMax(lower, bound->mFixed);
                else if(bound->mKnownType == EventKnown::eUniform)
                    lower = qMax(lower, bound->mUniformStart);
            }
        }
        double upper = (double) mSettings.mTmax;
        Phase* phaseTo = mPhaseConstraints.at(i)->mPhaseTo;
        for(int j=0; j<phaseTo->mEvents.size(); ++j) {
            EventKnown* bound = dynamic_cast<EventKnown*>(phaseTo->mEvents[j]);
            if(bound) {
                if(bound->mKnownType == EventKnown::eFixed)
                    upper = qMin(upper, bound->mFixed);
                else if(bound->mKnownType == EventKnown::eUniform)
                    upper = qMin(upper, bound->mUniformEnd);
            }
            bound = 0;
        }
        if(gammaMin >= (upper - lower))
        {
            throw QString("The constraint between phases \"" + phaseFrom->mName + "\" and \"" + phaseTo->mName + "\" is not consistent with the bounds they contain!");
        }
        phaseFrom = 0;
        phaseTo = 0;
    }
    
    // 10 - Au sein d'une phase, tau max (ou fixe) doit être supérieur à la différence entre le max des infs des intervalles des bornes et le min des sups des intervalles des bornes.
    //  => Modifier les intervalles des bornes:
    //      - L'inf est le max entre : sa valeur courante ou (le max des infs des intervalles des bornes - tau max ou fixe)
    //      - Le sup est le min entre : sa valeur courante ou (le min des sups des intervalles des bornes + tau max ou fixe)
    
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases.at(i)->mTauType != Phase::eTauUnknown)
        {
            double tauMax = mPhases.at(i)->mTauFixed;
            if(mPhases.at(i)->mTauType == Phase::eTauRange)
                tauMax = mPhases.at(i)->mTauMax;
            
            double min = mSettings.mTmin;
            double max = mSettings.mTmax;
            bool boundFound = false;
            
            for(int j=0; j<mPhases.at(i)->mEvents.size(); ++j) {
                if(mPhases.at(i)->mEvents.at(j)->mType == Event::eKnown) {
                    EventKnown* bound = dynamic_cast<EventKnown*>(mPhases.at(i)->mEvents[j]);
                    if(bound) {
                        boundFound = true;
                        if(bound->mKnownType == EventKnown::eFixed) {
                            min = std::max(min, bound->mFixed);
                            max = std::min(max, bound->mFixed);
                        }
                        else if(bound->mKnownType == EventKnown::eUniform) {
                            min = std::max(min, bound->mUniformEnd);
                            max = std::min(max, bound->mUniformStart);
                        }
                    }
                    bound = 0;
                }
            }
            if(boundFound)
            {
                if(tauMax < (max - min))
                {
                    throw QString("The phase \"" + mPhases.at(i)->mName + "\" has a duration inconsistent with the bounds it contains!");
                }
                // Modify bounds intervals to match max phase duration
                for(int j=0; j<mPhases.at(i)->mEvents.size(); ++j)
                {
                    if(mPhases.at(i)->mEvents[j]->mType == Event::eKnown)
                    {
                        EventKnown* bound = dynamic_cast<EventKnown*>(mPhases.at(i)->mEvents[j]);
                        if(bound)
                        {
                            if(bound->mKnownType == EventKnown::eUniform)
                            {
                                bound->mUniformStart = std::max(bound->mUniformStart, max - tauMax);
                                bound->mUniformEnd = std::min(bound->mUniformEnd, min + tauMax);
                                
                                min = std::max(min, bound->mUniformEnd);
                                max = std::min(max, bound->mUniformStart);
                            }
                        }
                        bound = 0;
                    }
                }
            }
        }
    }
    
    // 11 - Vérifier la cohérence entre les contraintes de faits et de phase
    for(int i=0; i<phaseBranches.size(); ++i)
    {
        for(int j=0; j<phaseBranches.at(i).size(); ++j)
        {
            Phase* phase = phaseBranches[i][j];
            for(int k=0; k<phase->mEvents.size(); ++k)
            {
                Event* event = phase->mEvents[k];
                
                bool phaseFound = false;
                
                // On réinspecte toutes les phases de la branche et on vérifie que le fait n'a pas de contrainte en contradiction avec les contraintes de phase !
                for(int l=0; l<phaseBranches.at(i).size(); ++l)
                {
                    Phase* p = phaseBranches[i][l];
                    if(p == phase)
                    {
                        phaseFound = true;
                    }
                    else
                    {
                        for(int m=0; m<p->mEvents.size(); ++m)
                        {
                            Event* e = p->mEvents[m];
                            
                            // Si on regarde l'élément d'un phase d'avant, le fait ne peut pas être en contrainte vers un fait de cette phase
                            if(!phaseFound)
                            {
                                for(int n=0; n<e->mConstraintsBwd.size(); ++n)
                                {
                                    if(e->mConstraintsBwd[n]->mEventFrom == event)
                                    {
                                        throw "The event " + event->mName + " (in phase " + phase->mName + ") is before the event " + e->mName + " (in phase " + p->mName + "), BUT the phase " + phase->mName + " is after the phase " + p->mName + ".\r=> Contradiction !";
                                    }
                                }
                            }
                            else
                            {
                                for(int n=0; n<e->mConstraintsFwd.size(); ++n)
                                {
                                    if(e->mConstraintsFwd[n]->mEventTo == event)
                                    {
                                        throw "The event " + event->mName + " (in phase " + phase->mName + ") is after the event " + e->mName + " (in phase " + p->mName + "), BUT the phase " + phase->mName + " is before the phase " + p->mName + ".\r=> Contradiction !";
                                    }
                                }
                            }
                        }
                    }
                    p = 0;
                }
                event = 0;
            }
        }
    }
    
    return true;
}

#pragma mark Generate model data
void Model::generateCorrelations(const QList<ChainSpecs> &chains)
{
#ifdef DEBUG
    QTime t = QTime::currentTime();
#endif
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        (*iterEvent)->mTheta.generateCorrelations(chains);
        
        //for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
        for( Date& date : (*iterEvent)->mDates ) {
            //Date& date = (*iterEvent)->mDates[j];
            date.mTheta.generateCorrelations(chains);
            date.mSigma.generateCorrelations(chains);
        }
        ++iterEvent;
    }
    
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
        (*iterPhase)->mAlpha.generateCorrelations(chains);
        (*iterPhase)->mBeta.generateCorrelations(chains);
        ++iterPhase;
    }
#ifdef DEBUG
    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generateCorrelations done in " + QString::number(timeDiff) + " ms";
#endif
}

void Model::setBandwidth(const double bandwidth)
{
    qDebug()<<"Model::setBandwidth";
    if (mBandwidth != bandwidth) {
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
    qDebug()<<"Model::setFTLength";
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
 * @brief Make all densities and credibilities and time range and set mFFTLength, mBandwidth and mThreshold
 * @param[in] fftLength
 * @param[in] bandwidth
 * @param[in] threshold
 */
void Model::initDensities(const int fftLength, const double bandwidth, const double threshold)
{
    qDebug()<<"Model::initDensities"<<fftLength<<bandwidth<<threshold;
    mFFTLength = fftLength;
    mBandwidth = bandwidth;
    // memo the new value of the Threshold inside all the part of the model: phases, events and dates
    initThreshold(threshold);
    clearPosteriorDensities();
    generatePosteriorDensities(mChains, mFFTLength, mBandwidth);
    generateHPD(mThreshold);

    generateCredibility(mThreshold);
    generateNumericalResults(mChains);
    qDebug()<<"Model::initDensities";
 //   emit newCalculus();
}

void Model::updateDensities(const int fftLength, const double bandwidth, const double threshold)
{
    qDebug()<<"Model::updateDensities"<<fftLength<<bandwidth<<threshold;
    bool newPosteriorDensities = false;
    if ((mFFTLength != fftLength) || (mBandwidth != bandwidth)) {
        mFFTLength = fftLength;
        mBandwidth = bandwidth;

        clearPosteriorDensities();
        generatePosteriorDensities(mChains, mFFTLength, mBandwidth);
        newPosteriorDensities = true;
    }
    // memo the new value of the Threshold inside all the part of the model: phases, events and dates

    if (mThreshold != threshold) {
        initThreshold(threshold);

        generateCredibility(mThreshold);
    }

    if (newPosteriorDensities)
        generateHPD(mThreshold);

    generateNumericalResults(mChains);

}

void Model::generatePosteriorDensities(const QList<ChainSpecs> &chains, int fftLen, double bandwidth)
{
    QTime t = QTime::currentTime();
    
    const double tmin = mSettings.getTminFormated();
    const double tmax = mSettings.getTmaxFormated();
    
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        (*iterEvent)->mTheta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);

        for(int j=0; j<(*iterEvent)->mDates.size(); ++j)
            (*iterEvent)->mDates[j].generateHistos(chains, fftLen, bandwidth, tmin, tmax);

        ++iterEvent;
    }
    
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
        (*iterPhase)->generateHistos(chains, fftLen, bandwidth, tmin, tmax);
        ++iterPhase;
    }

    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generatePosteriorDensities done in " + QString::number(timeDiff) + " ms";
}

void Model::generateNumericalResults(const QList<ChainSpecs> &chains)
{
    QTime t = QTime::currentTime();
    
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        (*iterEvent)->mTheta.generateNumericalResults(chains);
        
        for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
            Date& date = (*iterEvent)->mDates[j];
            date.mTheta.generateNumericalResults(chains);
            date.mSigma.generateNumericalResults(chains);
        }
        ++iterEvent;
    }
    
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
        (*iterPhase)->mAlpha.generateNumericalResults(chains);
        (*iterPhase)->mBeta.generateNumericalResults(chains);
        (*iterPhase)->mDuration.generateNumericalResults(chains);
        ++iterPhase;
    }
    
    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generateNumericalResults done in " + QString::number(timeDiff) + " ms";
}

void Model::clearThreshold()
{
    mThreshold = -1.;
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        (*iterEvent)->mTheta.mThresholdUsed = -1.;

        for (int j=0; j<(*iterEvent)->mDates.size(); ++j) {
            Date& date = (*iterEvent)->mDates[j];
            date.mTheta.mThresholdUsed = -1.;
            date.mSigma.mThresholdUsed = -1.;
        }
        ++iterEvent;
    }

    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
        (*iterPhase)->mAlpha.mThresholdUsed = -1.;
        (*iterPhase)->mBeta.mThresholdUsed = -1.;
        (*iterPhase)->mDuration.mThresholdUsed = -1.;
        ++iterPhase;
    }
}

void Model::initThreshold(const double threshold)
{
   // memo threshold used  value
    mThreshold = threshold;
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {

        if ((*iterEvent)->type() != Event::eKnown) {
          (*iterEvent)->mTheta.mThresholdUsed = threshold;

          for (int j = 0; j<(*iterEvent)->mDates.size(); ++j) {
                Date& date = (*iterEvent)->mDates[j];
                date.mTheta.mThresholdUsed = threshold;
                date.mSigma.mThresholdUsed = threshold;
            }

        }
        ++iterEvent;
    }
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
       (*iterPhase)->mAlpha.mThresholdUsed = threshold;
       (*iterPhase)->mBeta.mThresholdUsed = threshold;
       (*iterPhase)->mDuration.mThresholdUsed = threshold;
        ++iterPhase;
    }
}

/**
 * @brief Model::setThreshold this is a slot
 * @param threshold
 */
void Model::setThreshold(const double threshold)
{
    qDebug()<<"Model::setThreshold"<<threshold<<" mThreshold"<<mThreshold;
    if ( mThreshold != threshold) {
        mThreshold = threshold;
        generateCredibility(threshold);
        generateHPD(threshold);

        // memo threshold used  value

        QList<Event*>::iterator iterEvent = mEvents.begin();
        while (iterEvent!=mEvents.end()) {

            if ((*iterEvent)->type() != Event::eKnown) {
              (*iterEvent)->mTheta.mThresholdUsed = threshold;

              for (int j=0; j<(*iterEvent)->mDates.size(); ++j) {
                    Date& date = (*iterEvent)->mDates[j];
                    date.mTheta.mThresholdUsed = threshold;
                    date.mSigma.mThresholdUsed = threshold;
                }

            }
            ++iterEvent;
        }
        QList<Phase*>::iterator iterPhase = mPhases.begin();
        while (iterPhase!=mPhases.end()) {
           (*iterPhase)->mAlpha.mThresholdUsed = threshold;
           (*iterPhase)->mBeta.mThresholdUsed = threshold;
           (*iterPhase)->mDuration.mThresholdUsed = threshold;
            ++iterPhase;
        }

        emit newCalculus();
   }
}

double Model::getThreshold() const
{
    return mThreshold;
}

void Model::generateCredibility(const double thresh)
{
    qDebug()<<"Model::generateCredibility("<<thresh;
    QTime t = QTime::currentTime();

    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        bool isFixedBound = false;
        if ((*iterEvent)->type() == Event::eKnown) {
            EventKnown* ek = dynamic_cast<EventKnown*>(*iterEvent);
            if(ek->knownType() == EventKnown::eFixed)
                isFixedBound = true;
        }

        if (!isFixedBound) {
            (*iterEvent)->mTheta.generateCredibility(mChains, thresh);

            for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
                Date& date = (*iterEvent)->mDates[j];
                date.mTheta.generateCredibility(mChains, thresh);
                date.mSigma.generateCredibility(mChains, thresh);

            }
        }
        ++iterEvent;
    }

    QList<Phase*>::iterator iterPhase (mPhases.begin());
    // Diplay a progressBar if long
    QProgressDialog *progress = new QProgressDialog("Time range & credibilities generation","Wait" , 1, 10,qApp->activeWindow());
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(0);
    progress->setMinimumDuration(4);
    progress->setMinimum(0);
    progress->setMaximum(mPhases.size()*4);
    int position(1);
    while (iterPhase!=mPhases.end()) {
        progress->setValue(position);
        // if there is only one Event in the phase, there is no Duration
        (*iterPhase)->mAlpha.generateCredibility(mChains, thresh);
        ++position;
        (*iterPhase)->mBeta.generateCredibility(mChains, thresh);
        ++position;
        (*iterPhase)->mDuration.generateCredibility(mChains, thresh);
        ++position;
        (*iterPhase)->mTimeRange = timeRangeFromTraces((*iterPhase)->mAlpha.fullRunTrace(mChains),
                                                             (*iterPhase)->mBeta.fullRunTrace(mChains),thresh, "Time Range for Phase : "+(*iterPhase)->mName);

        ++position;
        //qDebug()<<"Time Range for Phase "<<(*iterPhase)->mName<<thresh;
        ++iterPhase;

    };
    progress->setMinimum(0);
    progress->setMaximum(mPhaseConstraints.size()*2);
    progress->setLabelText("Gaps and transitions generation");
    QList<PhaseConstraint*>::iterator iterPhaseConstraint = mPhaseConstraints.begin();
    position = 1;
    while (iterPhaseConstraint!=mPhaseConstraints.end()) {
        progress->setValue(position);
        Phase* phaseFrom = (*iterPhaseConstraint)->mPhaseFrom;
        Phase* phaseTo  = (*iterPhaseConstraint)->mPhaseTo;

        (*iterPhaseConstraint)->mGapRange = gapRangeFromTraces(phaseFrom->mBeta.fullRunTrace(mChains),
                                                             phaseTo->mAlpha.fullRunTrace(mChains), thresh, "Gap Range : "+phaseFrom->mName+ " to "+ phaseTo->mName);

        ++position;
        qDebug()<<"Gap Range "<<phaseFrom->mName<<" to "<<phaseTo->mName;

        (*iterPhaseConstraint)->mTransitionRange = transitionRangeFromTraces(phaseFrom->mBeta.fullRunTrace(mChains),
                                                             phaseTo->mAlpha.fullRunTrace(mChains), thresh, "Transition Range : "+phaseFrom->mName+ " to "+ phaseTo->mName);

        ++position;
        qDebug()<<"Transition Range "<<phaseFrom->mName<<" to "<<phaseTo->mName;
        ++iterPhaseConstraint;

    };
    delete progress;
    QTime t2 (QTime::currentTime());
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generateCredibility done in " + QString::number(timeDiff) + " ms";

}

void Model::generateHPD(const double thresh)
{
    QTime t = QTime::currentTime();

    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        bool isFixedBound = false;

        if((*iterEvent)->type() == Event::eKnown) {
            EventKnown* ek = dynamic_cast<EventKnown*>(*iterEvent);
            if(ek->knownType() == EventKnown::eFixed)
                isFixedBound = true;
        }

        if(!isFixedBound) {
            (*iterEvent)->mTheta.generateHPD(thresh);

            for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
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

    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generateHPD done in " + QString::number(timeDiff) + " ms";

}

#pragma mark Clear model data
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
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.cend()) {
        foreach (Date date, (*iterEvent)->mDates) {
            date.mTheta.mRawTrace->clear();
            date.mTheta.mFormatedTrace->clear();
            date.mSigma.mRawTrace->clear();
            date.mSigma.mFormatedTrace->clear();
        }
        (*iterEvent)->mTheta.mRawTrace->clear();
        (*iterEvent)->mTheta.mFormatedTrace->clear();
        ++iterEvent;
    }
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.cend()) {
        (*iterPhase)->mAlpha.mRawTrace->clear();
        (*iterPhase)->mAlpha.mFormatedTrace->clear();
        (*iterPhase)->mBeta.mRawTrace->clear();
        (*iterPhase)->mAlpha.mFormatedTrace->clear();
        (*iterPhase)->mDuration.mRawTrace->clear();
        (*iterPhase)->mDuration.mFormatedTrace->clear();
        ++iterPhase;
    }
}


#pragma mark Date files read / write
/** @Brief Save .dat file, the result of computation and compress it
 *
 * */
void Model::saveToFile(const QString& fileName)
{
    if (!mEvents.empty()) {
    // -----------------------------------------------------
    //  Create file
    // -----------------------------------------------------
    //QFileInfo info(fileName);
   // QFile file(info.path() + info.baseName() + ".~dat"); // when we could do a compressed file
    //QFile file(info.path() + info.baseName() + ".dat");
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly)) {
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
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_5_5);

        //out << quint32(MagicNumber);// we could add software version here << quint16(out.version());
        
        // -----------------------------------------------------
        //  Write info
        // -----------------------------------------------------
        out << quint32 (mPhases.size());
        out << quint32 (mEvents.size());
        out << quint32 (numDates);

        out << (quint32) mChains.size();
        for (ChainSpecs& ch : mChains) {
            out << (quint32) ch.mBatchIndex;
            out << (quint32) ch.mBatchIterIndex;
            out << (quint32)ch.mBurnIterIndex;
            out << (quint32) ch.mMaxBatchs;
            out << ch.mMixingLevel;
            out << (quint32) ch.mNumBatchIter;
            out << (quint32) ch.mNumBurnIter;
            out << (quint32) ch.mNumRunIter;
            out << (quint32)ch.mRunIterIndex;
            out << (qint32) ch.mSeed;
            out << (quint32)ch.mThinningInterval;
            out << (quint32) ch.mTotalIter;
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

                      out << (qint32) d.mSettings.mTmin;
                      out << (qint32) d.mSettings.mTmax;
                      out <<  d.mSettings.mStep;
                      out << (quint8)(d.mSettings.mStepForced==true? 1: 0);

                    //  out << d.mSubDates;
                      out << d.getTminRefCurve();
                      out << d.getTmaxRefCurve();

                      /*out << d.getTminCalib();
                      out << d.getTmaxCalib();
                      */
                      out <<d.mCalibration;
                      //out <<d.mRepartition;


                      out << (quint32)d.mCalibHPD.size();
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
/** @Brief Read the .dat file, it's the result of the saved computation and uncompress it
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
   // QFile file(info.path() + info.baseName() + ".dat");

    QFile file(fileName);
    if (file.exists() && file.open(QIODevice::ReadOnly)){

    //    if ( file.size()!=0 /* uncompressedData.size()!=0*/ ) {
 //           QDataStream in(&uncompressedData, QIODevice::ReadOnly);
    QDataStream in(&file);
   /* if (in.version()!= QDataStream::Qt_5_5)
        return;
    in.setVersion(QDataStream::Qt_5_5);
    */
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
            mChains.reserve((int) tmp32);
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

            for (int i=0; i<mPhases.size(); ++i) {
               in >> mPhases[i]->mAlpha;
               in >> mPhases[i]->mBeta;
               in >> mPhases[i]->mDuration;
            }

            // -----------------------------------------------------
            //  Read events data
            // -----------------------------------------------------

            for (Event* e:mEvents)
                in >> e->mTheta;

            // -----------------------------------------------------
            //  Read dates data
            // -----------------------------------------------------

            for (int i=0; i<mEvents.size(); ++i) {
                if (mEvents[i]->mType == Event::eDefault )
                     for (int j=0; j<mEvents[i]->mDates.size(); ++j) {
                        Date& d = mEvents[i]->mDates[j];
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
                        d.mSettings.mTmin = (int)tmpInt32;
                        in >> tmpInt32;
                        d.mSettings.mTmax = (int)tmpInt32;
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

                       /* in >> tmp;
                        d.setTminCalib(tmp);
                        in >>tmp;
                        d.setTmaxCalib(tmp);
                       */
                        quint32 tmpUint32;
                        in >>*(d.mCalibration);
                        //in >>d.mRepartition;

                        in >> tmpUint32;

                        for (quint32 i= 0; i<tmpUint32; i++) {
                            double tmpKey;
                            double tmpValue;
                            in >> tmpKey;
                            in >> tmpValue;
                            d.mCalibHPD[tmpKey]= tmpValue;
                        }
#ifdef DEBUG
                         if (d.mCalibration->mCurve.isEmpty())
                             qDebug()<<"Model::restoreFromFile vide";
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

