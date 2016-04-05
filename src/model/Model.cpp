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
mNumberOfPhases(0),
mNumberOfEvents(0),
mNumberOfDates(0)
{
    
}

Model::~Model()
{
  /*  qDebug() << "Deleting old project model";
    
    for(int i=mEvents.size()-1; i>=0; --i)
    {
        Event* item = mEvents[i];
        delete item;
        item = 0;
        mEvents.removeAt(i);
    }
    for(int i=mPhases.size()-1; i>=0; --i)
    {
        Phase* item = mPhases[i];
        delete item;
        item = 0;
        mPhases.removeAt(i);
    }
    for(int i=mEventConstraints.size()-1; i>=0; --i)
    {
        EventConstraint* item = mEventConstraints[i];
        delete item;
        item = 0;
        mEventConstraints.removeAt(i);
    }
    for(int i=mPhaseConstraints.size()-1; i>=0; --i)
    {
        PhaseConstraint* item = mPhaseConstraints[i];
        delete item;
        item = 0;
        mPhaseConstraints.removeAt(i);
    }*/
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
        foreach (PhaseConstraint* ph, mPhaseConstraints) {
            //if(ph) delete ph;
            ph = 0;
        }
        mPhaseConstraints.clear();
    }

    if(!mEventConstraints.isEmpty()) {
        foreach (EventConstraint* ev, mEventConstraints) {
            //if(ev) delete ev;
            ev = 0;
        }
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
        Event* event = mEvents[i] ;
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
            p = 0;
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
                try{
                    Event* e = new Event(Event::fromJson(event));
                    e->mMixingLevel = mMCMCSettings.mMixingLevel;
                    mNumberOfDates += e->mDates.size();

                    for (int j=0; j<e->mDates.size(); ++j) {
                        e->mDates[j].mMixingLevel=e->mMixingLevel;
                        e->mDates[j].mColor=e->mColor;

                    }
                    mEvents.append(e);
                    e = 0;
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

    if(json.contains(STATE_EVENTS_CONSTRAINTS))
    {
        const QJsonArray constraints = json.value(STATE_EVENTS_CONSTRAINTS).toArray();
        for(int i=0; i<constraints.size(); ++i)
        {
            const QJsonObject constraint = constraints.at(i).toObject();
            EventConstraint* c = new EventConstraint(EventConstraint::fromJson(constraint));
            mEventConstraints.append(c);
        }
    }

    if(json.contains(STATE_PHASES_CONSTRAINTS))
    {
        const QJsonArray constraints = json.value(STATE_PHASES_CONSTRAINTS).toArray();
        for(int i=0; i<constraints.size(); ++i)
        {
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
    for(int i=0; i<mEvents.size(); ++i)
    {
        int eventId = mEvents.at(i)->mId;
        QList<int> phasesIds = mEvents.at(i)->mPhasesIds;

        // Link des events / phases
        for(int j=0; j<mPhases.size(); ++j)
        {
            int phaseId = mPhases.at(j)->mId;
            if(phasesIds.contains(phaseId))
            {
                mEvents[i]->mPhases.append(mPhases[j]);
                mPhases[j]->mEvents.append(mEvents[i]);
            }
        }

        // Link des events / contraintes d'event
        for(int j=0; j<mEventConstraints.size(); ++j)
        {
            if(mEventConstraints[j]->mFromId == eventId)
            {
                mEventConstraints[j]->mEventFrom = mEvents[i];
                mEvents[i]->mConstraintsFwd.append(mEventConstraints[j]);
            }
            else if(mEventConstraints[j]->mToId == eventId)
            {
                mEventConstraints[j]->mEventTo = mEvents[i];
                mEvents[i]->mConstraintsBwd.append(mEventConstraints[j]);
            }
        }
    }
    // Link des phases / contraintes de phase
    for(int i=0; i<mPhases.size(); ++i)
    {
        int phaseId = mPhases.at(i)->mId;
        for(int j=0; j<mPhaseConstraints.size(); ++j)
        {
            if(mPhaseConstraints.at(j)->mFromId == phaseId)
            {
                mPhaseConstraints[j]->mPhaseFrom = mPhases[i];
                mPhases[i]->mConstraintsFwd.append(mPhaseConstraints[j]);
            }
            else if(mPhaseConstraints.at(j)->mToId == phaseId)
            {
                mPhaseConstraints[j]->mPhaseTo = mPhases[i];
                mPhases[i]->mConstraintsBwd.append(mPhaseConstraints[j]);
            }
        }
    }
    //return model;
}

void Model::updateDesignFromJson(const QJsonObject& json)
{
    const QJsonArray phasesJSON = json.value(STATE_PHASES).toArray();
    if(mPhases.size() != phasesJSON.size()) return;

    for(int i=0; i<phasesJSON.size(); ++i)
    {
        const QJsonObject phaseJS = phasesJSON.at(i).toObject();
        Phase * p = mPhases[i];
        p->mName = phaseJS.value(STATE_NAME).toString();
        p->mColor = QColor(phaseJS.value(STATE_COLOR_RED).toInt(),phaseJS.value(STATE_COLOR_GREEN).toInt(),phaseJS.value(STATE_COLOR_BLUE ).toInt()) ;
    }

    const QJsonArray eventsJSON = json.value(STATE_EVENTS).toArray();
    if(mEvents.size() != eventsJSON.size()) return;
    for(int i=0; i<eventsJSON.size(); ++i)
    {
        const QJsonObject eventJS = eventsJSON.at(i).toObject();
        Event * e = mEvents[i];
        e->mName = eventJS.value(STATE_NAME).toString();
        e->mColor = QColor(eventJS.value(STATE_COLOR_RED).toInt(),eventJS.value(STATE_COLOR_GREEN).toInt(),eventJS.value(STATE_COLOR_BLUE ).toInt()) ;

        QJsonArray datesJS = eventJS.value(STATE_EVENT_DATES).toArray();
        if(e->mDates.size() != datesJS.size()) return;
        for(int j=0; j<datesJS.size(); ++j) {
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
    for(int i=0; i<mEvents.size(); ++i)
        events.append(mEvents.at(i)->toJson());
    json["events"] = events;
    
    QJsonArray phases;
    for(int i=0; i<mPhases.size(); ++i)
        phases.append(mPhases.at(i)->toJson());
    json["phases"] = phases;
    
    QJsonArray event_constraints;
    for(int i=0; i<mEventConstraints.size(); ++i)
        event_constraints.append(mEventConstraints.at(i)->toJson());
    json["event_constraints"] = event_constraints;
    
    QJsonArray phase_constraints;
    for(int i=0; i<mPhaseConstraints.size(); ++i)
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
    
    for(int i=0; i<mEvents.size(); ++i)
    {
        QString objType = "Event";
        if(mEvents.at(i)->type() == Event::eKnown)
        {
            log += line(textRed("Bound (" + QString::number(i+1) + "/" + QString::number(mEvents.size()) + ") : " + mEvents.at(i)->mName + " (" +
                                 QString::number(mEvents.at(i)->mPhases.size()) + " phases, " +
                                 QString::number(mEvents.at(i)->mConstraintsBwd.size()) + " const. back., " +
                                 QString::number(mEvents.at(i)->mConstraintsFwd.size()) + " const. fwd.)"));
        }
        else
        {
            log += line(textBlue("Event (" + QString::number(i+1) + "/" + QString::number(mEvents.size()) + ") : " + mEvents.at(i)->mName + " (" +
                                 QString::number(mEvents.at(i)->mDates.size()) + " data, " +
                                 QString::number(mEvents.at(i)->mPhases.size()) + " phases, " +
                                 QString::number(mEvents.at(i)->mConstraintsBwd.size()) + " const. back., " +
                                 QString::number(mEvents.at(i)->mConstraintsFwd.size()) + " const. fwd.)" +
                                 "<br>- Method : " + ModelUtilities::getEventMethodText(mEvents.at(i)->mMethod)));
        }
        
        for(int j=0; j<mEvents.at(i)->mDates.size(); ++j)
        {
            log += "<br>";
            log += line(textBlack("Data (" + QString::number(j+1) + "/" + QString::number(mEvents.at(i)->mDates.size()) + ") : " + mEvents.at(i)->mDates.at(j).mName +
                                  "<br>- Type : " + mEvents.at(i)->mDates.at(j).mPlugin->getName() +
                                  "<br>- Method : " + ModelUtilities::getDataMethodText(mEvents.at(i)->mDates.at(j).mMethod) +
                                  "<br>- Params : " + mEvents.at(i)->mDates.at(j).getDesc()));
        }
        log += "<hr>";
        log += "<br>";
    }
    
    for(int i=0; i<mPhases.size(); ++i)
    {
        log += line(textPurple("Phase (" + QString::number(i+1) + "/" + QString::number(mPhases.size()) + ") : " + mPhases[i]->mName + " (" + QString::number(mPhases[i]->mEvents.size()) + " events)"));
        log += "<br>";
        
        for(int j=0; j<mPhases.at(i)->mEvents.size(); ++j)
        {
            log += line(textBlue("Event : " + mPhases.at(i)->mEvents.at(j)->mName));
        }
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
    for(int i=0; i<mEvents.size(); ++i)
    {
        Event* event = mEvents.at(i);
        log += ModelUtilities::eventResultsHTML(event, true, this);
        log += "<hr>";
        event = 0;
    }
    for(int i=0; i<mPhases.size(); ++i)
    {
        Phase* phase = mPhases.at(i);
        log += ModelUtilities::phaseResultsHTML(phase);
        log += "<hr>";
        phase = 0;
    }
    for(int i=0; i<mPhaseConstraints.size(); ++i)
    {
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
    for(int i=0; i<mPhases.size(); ++i)
    {
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
    for(int i=0; i<mEvents.size(); ++i)
    {
        Event* event = mEvents.at(i);
        
        QStringList l = event->mTheta.getResultsList(locale);
        maxHpd = qMax(maxHpd, (l.size() - 9) / 3);
        l.prepend(event->mName);
        rows << l;
        event = 0;
    }
    
    // Dates
    rows << QStringList();
    for(int i=0; i<mEvents.size(); ++i)
    {
        Event* event = mEvents.at(i);
        for(int j=0; j<event->mDates.size(); ++j)
        {
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
    for(int i=0; i<maxHpd; ++i){
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
    for(int j=0; j<mPhases.size(); ++j)
        headers << mPhases.at(j)->mName + " alpha" << mPhases.at(j)->mName + " beta";

    rows << headers;
    
    int shift = 0;
    for(int i=0; i<mChains.size(); ++i)
    {
        int burnAdaptSize = mChains.at(i).mNumBurnIter + (mChains.at(i).mBatchIndex * mChains.at(i).mNumBatchIter);
        int runSize = mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
        
        for(int j = burnAdaptSize; j<burnAdaptSize + runSize; ++j) {
            QStringList l;
            l << QString::number(shift + j);
            for(int k=0; k<mPhases.size(); ++k) {
                Phase* phase = mPhases.at(k);
                double valueAlpha = phase->mAlpha.mRawTrace.at(shift + j);
                if(withDateFormat) valueAlpha = DateUtils::convertToAppSettingsFormat(valueAlpha);
                l << locale.toString(valueAlpha);

                double valueBeta = phase->mBeta.mRawTrace.at(shift + j);
                if(withDateFormat) valueBeta = DateUtils::convertToAppSettingsFormat(valueBeta);
                l << locale.toString(valueBeta);
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
    if(phaseIdx >= 0 && phaseIdx < mPhases.size()){
        phase = mPhases.value(phaseIdx);
    } else {
        return QList<QStringList>();
    }
    
    int runSize = 0;
    for(int i=0; i<mChains.size(); ++i)
        runSize += mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
    
    QStringList headers;
    headers << "iter" << phase->mName + " alpha" << phase->mName + " beta";
    for(int i=0; i<phase->mEvents.size(); ++i) {
        Event* event = phase->mEvents.at(i);
        headers << event->mName;
        event = 0;
    }
    rows << headers;
    
    int shift = 0;
    for(int i=0; i<mChains.size(); ++i) {
        int burnAdaptSize = mChains.at(i).mNumBurnIter + (mChains.at(i).mBatchIndex * mChains.at(i).mNumBatchIter);
        int runSize = mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
        
        for(int j=burnAdaptSize; j<burnAdaptSize + runSize; ++j) {
            QStringList l;
            l << QString::number(shift + j) ;
            double valueAlpha = phase->mAlpha.mRawTrace.at(shift + j);
            if(withDateFormat)
                valueAlpha = DateUtils::convertToAppSettingsFormat(valueAlpha);
            
            l << locale.toString(valueAlpha);

            double valueBeta = phase->mBeta.mRawTrace.at(shift + j);
            if(withDateFormat)
                valueBeta = DateUtils::convertToAppSettingsFormat(valueBeta);
            
            l << locale.toString(valueBeta);

            for(int k=0; k<phase->mEvents.size(); ++k) {
                Event* event = phase->mEvents.at(k);
                double value = event->mTheta.mRawTrace.at(shift + j);
                if(withDateFormat)
                    value = DateUtils::convertToAppSettingsFormat(value);
                
                l << locale.toString(value);
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
    for(int i=0; i<mChains.size(); ++i)
        runSize += mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
    
    
    QStringList headers;
    headers << "iter";
    for(int i=0; i<mEvents.size(); ++i) {
        Event* event = mEvents.at(i);
        headers << event->mName;
        event = 0;
    }
    rows << headers;
    
    int shift = 0;
    for(int i=0; i<mChains.size(); ++i) {
        const int burnAdaptSize = mChains.at(i).mNumBurnIter + (mChains.at(i).mBatchIndex * mChains.at(i).mNumBatchIter);
        const int runSize = mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval;
        
        for(int j=burnAdaptSize; j<burnAdaptSize + runSize; ++j) {
            QStringList l;
            l << QString::number(shift + j) ;
            for(int k=0; k<mEvents.size(); ++k) {
                Event* event = mEvents.at(k);
                double value = event->mTheta.mRawTrace.at(shift + j);
                if(withDateFormat)
                    value = DateUtils::convertToAppSettingsFormat(value);
                
                l << locale.toString(value);
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
    if(mEvents.size() == 0)
        throw QObject::tr("At least one event is required");
    
    // 2 - The event must contain at least 1 data
    for(int i=0; i<mEvents.size(); ++i)
    {
        if(mEvents.at(i)->type() == Event::eDefault)
        {
            if(mEvents.at(i)->mDates.size() == 0)
                throw QObject::tr(" The event") + " \"" + mEvents.at(i)->mName + "\" " + QObject::tr("must contain at least 1 data");
        }
    }
    
    // 3 - The phase must contain at least 1 event
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases.at(i)->mEvents.size() == 0)
            throw QObject::tr("The phase") + " \"" + mPhases.at(i)->mName + "\" " + QObject::tr("must contain at least 1 event");
    }
    
    // 4 - Pas de circularité sur les contraintes de faits
    QVector<QVector<Event*> > eventBranches;
    try{
        eventBranches = ModelUtilities::getAllEventsBranches(mEvents);
    }catch(QString error){
        throw error;
    }
    
    // 5 - Pas de circularité sur les contraintes de phases
    // 6 - Gammas : sur toutes les branches, la somme des gamma min < plage d'étude :
    QVector<QVector<Phase*> > phaseBranches;
    try{
        phaseBranches = ModelUtilities::getAllPhasesBranches(mPhases, mSettings.mTmax - mSettings.mTmin);
    }catch(QString error){
        throw error;
    }
    
    // 7 - Un fait ne paut pas appartenir à 2 phases en contrainte
    for(int i=0; i<phaseBranches.size(); ++i)
    {
        QVector<Event*> branchEvents;
        for(int j=0; j<phaseBranches.at(i).size(); ++j)
        {
            Phase* phase = phaseBranches[i][j];
            for(int k=0; k<phase->mEvents.size(); ++k)
            {
                if(!branchEvents.contains(phase->mEvents.at(k)))
                {
                    branchEvents.append(phase->mEvents.at(k));
                    //qDebug() << phase->mEvents[k]->mName << " in " << phase->mName;
                }
                else
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
                double lower = mSettings.mTmin;
                for(int k=0; k<j; ++k)
                {
                    Event* evt = eventBranches[i][k];
                    if(evt->mType == Event::eKnown)
                    {
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
                {
                    bound->mUniformStart = qMax(bound->mUniformStart, lower);
                }
                
                // --------------------
                // Check bound interval upper value
                // --------------------
                double upper = mSettings.mTmax;
                for(int k=j+1; k<eventBranches.at(i).size(); ++k)
                {
                    Event* evt = eventBranches[i][k];
                    if(evt->mType == Event::eKnown)
                    {
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
                else if(bound->mKnownType == EventKnown::eUniform)
                {
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
        double gammaMin = 0;
        PhaseConstraint::GammaType gType = mPhaseConstraints.at(i)->mGammaType;
        if(gType == PhaseConstraint::eGammaFixed)
            gammaMin = mPhaseConstraints[i]->mGammaFixed;
        else if(gType == PhaseConstraint::eGammaRange)
            gammaMin = mPhaseConstraints.at(i)->mGammaMin;
        
        double lower = mSettings.mTmin;
        Phase* phaseFrom = mPhaseConstraints.at(i)->mPhaseFrom;
        for(int j=0; j<phaseFrom->mEvents.size(); ++j)
        {
            EventKnown* bound = dynamic_cast<EventKnown*>(phaseFrom->mEvents[j]);
            if(bound)
            {
                if(bound->mKnownType == EventKnown::eFixed)
                    lower = qMax(lower, bound->mFixed);
                else if(bound->mKnownType == EventKnown::eUniform)
                    lower = qMax(lower, bound->mUniformStart);
            }
        }
        double upper = mSettings.mTmax;
        Phase* phaseTo = mPhaseConstraints.at(i)->mPhaseTo;
        for(int j=0; j<phaseTo->mEvents.size(); ++j)
        {
            EventKnown* bound = dynamic_cast<EventKnown*>(phaseTo->mEvents[j]);
            if(bound)
            {
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
            
            for(int j=0; j<mPhases.at(i)->mEvents.size(); ++j)
            {
                if(mPhases.at(i)->mEvents.at(j)->mType == Event::eKnown)
                {
                    EventKnown* bound = dynamic_cast<EventKnown*>(mPhases.at(i)->mEvents[j]);
                    if(bound)
                    {
                        boundFound = true;
                        if(bound->mKnownType == EventKnown::eFixed)
                        {
                            min = std::max(min, bound->mFixed);
                            max = std::min(max, bound->mFixed);
                        }
                        else if(bound->mKnownType == EventKnown::eUniform)
                        {
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
        
        for(int j=0; j<(*iterEvent)->mDates.size(); ++j)
        {
            Date& date = (*iterEvent)->mDates[j];
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
    if(mBandwidth != bandwidth) {
        mBandwidth = bandwidth;

        clearPosteriorDensities();
        generatePosteriorDensities(mChains, mFFTLength, mBandwidth);
        generateHPD(mThreshold);
        generateNumericalResults(mChains);
        emit newCalculus();
    }
}

void Model::setFFTLength(const double FFTLength)
{
    qDebug()<<"Model::setFTLength";
    if(mFFTLength != FFTLength) {
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
        
        for(int j=0; j<(*iterEvent)->mDates.size(); ++j)
        {
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
    mThreshold = -1;
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        (*iterEvent)->mTheta.mThresholdUsed = -1;
        for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
            Date& date = (*iterEvent)->mDates[j];
            date.mTheta.mThresholdUsed = -1;
            date.mSigma.mThresholdUsed = -1;
        }
        ++iterEvent;
    }

    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
        (*iterPhase)->mAlpha.mThresholdUsed = -1;
        (*iterPhase)->mBeta.mThresholdUsed = -1;
        (*iterPhase)->mDuration.mThresholdUsed = -1;
        ++iterPhase;
    }
}

void Model::initThreshold(const double threshold)
{
   // memo threshold used  value
    mThreshold = threshold;
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {

        if((*iterEvent)->type() != Event::eKnown) {
          (*iterEvent)->mTheta.mThresholdUsed = threshold;

          for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
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
    if( mThreshold != threshold) {
        mThreshold = threshold;
        generateCredibility(threshold);
        generateHPD(threshold);

        // memo threshold used  value

        QList<Event*>::iterator iterEvent = mEvents.begin();
        while (iterEvent!=mEvents.end()) {

            if((*iterEvent)->type() != Event::eKnown) {
              (*iterEvent)->mTheta.mThresholdUsed = threshold;

              for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
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
        if((*iterEvent)->type() == Event::eKnown)
        {
            EventKnown* ek = dynamic_cast<EventKnown*>(*iterEvent);
            if(ek->knownType() == EventKnown::eFixed)
                isFixedBound = true;
        }

        if(!isFixedBound)
        {
            (*iterEvent)->mTheta.generateCredibility(mChains, thresh);

            for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
                Date& date = (*iterEvent)->mDates[j];
                date.mTheta.generateCredibility(mChains, thresh);
                date.mSigma.generateCredibility(mChains, thresh);

            }
        }
        ++iterEvent;
    }

    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
        // if there is only one Event in the phase, there is no Duration
        (*iterPhase)->mAlpha.generateCredibility(mChains, thresh);
        (*iterPhase)->mBeta.generateCredibility(mChains, thresh);

        (*iterPhase)->mDuration.generateCredibility(mChains, thresh);
        (*iterPhase)->mTimeRange = timeRangeFromTraces((*iterPhase)->mAlpha.runRawTraceForChain(mChains,0),
                                                             (*iterPhase)->mBeta.runRawTraceForChain(mChains,0),thresh, "Time Range for Phase : "+(*iterPhase)->mName);

        qDebug()<<"Time Range for Phase "<<(*iterPhase)->mName<<thresh;
         ++iterPhase;
    };

    QList<PhaseConstraint*>::iterator iterPhaseConstraint = mPhaseConstraints.begin();
    while (iterPhaseConstraint!=mPhaseConstraints.end()) {
        Phase* phaseFrom = (*iterPhaseConstraint)->mPhaseFrom;
        Phase* phaseTo  = (*iterPhaseConstraint)->mPhaseTo;

        (*iterPhaseConstraint)->mGapRange = gapRangeFromTraces(phaseFrom->mBeta.runRawTraceForChain(mChains,0),
                                                             phaseTo->mAlpha.runRawTraceForChain(mChains,0), thresh, "Gap Range : "+phaseFrom->mName+ " to "+ phaseTo->mName);
        qDebug()<<"Gap Range "<<phaseFrom->mName<<" to "<<phaseTo->mName;
        ++iterPhaseConstraint;
    };

    QTime t2 = QTime::currentTime();
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

/*void Model::generateCredibilityAndHPD(const QList<ChainSpecs> &chains,const double thresh)
{
    QTime t = QTime::currentTime();
    //const int nbElement = mEvents.size()+mPhases.size();
   // int elementProgress = 0;
    const double threshold = qBound(0.0,thresh,100.0);
    
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.end()) {
        bool isFixedBound = false;
        //++elementProgress;
        //progress->setValue(elementProgress);
        if((*iterEvent)->type() == Event::eKnown)
        {
            EventKnown* ek = dynamic_cast<EventKnown*>(*iterEvent);
            if(ek->knownType() == EventKnown::eFixed)
                isFixedBound = true;
        }
        
        if(!isFixedBound)
        {
            if((*iterEvent)->mTheta.mThresholdUsed != threshold) {
                (*iterEvent)->mTheta.generateHPD(threshold);
                (*iterEvent)->mTheta.generateCredibility(chains, threshold);
                (*iterEvent)->mTheta.mThresholdUsed = threshold;

                for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
                    Date& date = (*iterEvent)->mDates[j];
                    date.mTheta.generateHPD(threshold);
                    date.mSigma.generateHPD(threshold);

                    date.mTheta.generateCredibility(chains, threshold);
                    date.mSigma.generateCredibility(chains, threshold);

                    date.mTheta.mThresholdUsed = threshold;
                    date.mSigma.mThresholdUsed = threshold;
                }
             }
        }
        ++iterEvent;
    }
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.end()) {
       // ++elementProgress;
       // progress->setValue(elementProgress);
        // if there is only one Event in the phase, there is no Duration
       if((*iterPhase)->mAlpha.mThresholdUsed != threshold) {
            (*iterPhase)->mAlpha.generateHPD(threshold);
            (*iterPhase)->mAlpha.generateCredibility(chains, threshold);
            (*iterPhase)->mAlpha.mThresholdUsed = threshold;
        }
        if((*iterPhase)->mBeta.mThresholdUsed != threshold){
            (*iterPhase)->mBeta.generateHPD(threshold);
            (*iterPhase)->mBeta.generateCredibility(chains, threshold);
            (*iterPhase)->mBeta.mThresholdUsed = threshold;
        }

       if((*iterPhase)->mDuration.mThresholdUsed != threshold) {
            (*iterPhase)->mDuration.generateHPD(threshold);
            (*iterPhase)->mDuration.generateCredibility(chains, threshold);
            (*iterPhase)->mDuration.mThresholdUsed = threshold;
            (*iterPhase)->mTimeRange = timeRangeFromTraces((*iterPhase)->mAlpha.runRawTraceForChain(chains,0),
                                                             (*iterPhase)->mBeta.runRawTraceForChain(chains,0),threshold, "Time Range for Phase : "+(*iterPhase)->mName);
            //qDebug()<<"Model::generateCredibilityAndHPD() timeRange"<<(*iterPhase)->mTimeRange.first<<(*iterPhase)->mTimeRange.second;
       }

        ++iterPhase;
    }
    
    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generateCredibilityAndHPD done in " + QString::number(timeDiff) + " ms";

    //delete progress;
}*/

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
            date.mTheta.mCredibility = QPair<double,double>();
            date.mSigma.mHPD.clear();
            date.mSigma.mCredibility= QPair<double,double>();
        }
        (*iterEvent)->mTheta.mHPD.clear();
        (*iterEvent)->mTheta.mCredibility = QPair<double,double>();
        ++iterEvent;
    }
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.cend()) {
        (*iterPhase)->mAlpha.mHPD.clear();
        (*iterPhase)->mAlpha.mCredibility = QPair<double,double>();
        //(*iterPhase)->mAlpha.mThresholdOld = 0;

        (*iterPhase)->mBeta.mHPD.clear();
        (*iterPhase)->mBeta.mCredibility = QPair<double,double>();
        //(*iterPhase)->mBeta.mThresholdOld = 0;

        (*iterPhase)->mDuration.mHPD.clear();
        (*iterPhase)->mDuration.mCredibility = QPair<double,double>();
        //(*iterPhase)->mDuration.mThresholdOld = 0;
        (*iterPhase)->mTimeRange = QPair<double,double>();
        ++iterPhase;
    }
}

void Model::clearTraces()
{
    QList<Event*>::iterator iterEvent = mEvents.begin();
    while (iterEvent!=mEvents.cend()) {
        foreach (Date date, (*iterEvent)->mDates) {
            date.mTheta.mRawTrace.clear();
            date.mTheta.mFormatedTrace.clear();
            date.mSigma.mRawTrace.clear();
            date.mSigma.mFormatedTrace.clear();
        }
        (*iterEvent)->mTheta.mRawTrace.clear();
        (*iterEvent)->mTheta.mFormatedTrace.clear();
        ++iterEvent;
    }
    QList<Phase*>::iterator iterPhase = mPhases.begin();
    while (iterPhase!=mPhases.cend()) {
        (*iterPhase)->mAlpha.mRawTrace.clear();
        (*iterPhase)->mAlpha.mFormatedTrace.clear();
        (*iterPhase)->mBeta.mRawTrace.clear();
        (*iterPhase)->mAlpha.mFormatedTrace.clear();
        (*iterPhase)->mDuration.mRawTrace.clear();
        (*iterPhase)->mDuration.mFormatedTrace.clear();
        ++iterPhase;
    }
}


#pragma mark Date files read / write
/** @Brief Save .dat file, the result of computation and compress it
 *
 * */
void Model::saveToFile(const QString& fileName)
{
    if(!mEvents.empty())
    //if(!mEvents[0]->mTheta.mTrace.empty())
    {
    // -----------------------------------------------------
    //  Create file
    // -----------------------------------------------------
    
    QFile file(fileName);
    QByteArray uncompresedData;
    if(file.open(QIODevice::WriteOnly))
    {
        QDataStream out(&uncompresedData, QIODevice::WriteOnly);
        
        // -----------------------------------------------------
        //  Write info
        // -----------------------------------------------------
        out << (qint32)mPhases.size();
        out << (qint32)mEvents.size();

        qint32 numDates = 0;
        for(int i=0; i<mEvents.size(); ++i)
            numDates += mEvents[i]->mDates.size();
        out << numDates;
        // -----------------------------------------------------
        //  Write phases data
        // -----------------------------------------------------
        foreach (Phase* phase, mPhases) {
            phase->mAlpha.saveToStream(&out);
            phase->mBeta.saveToStream(&out);
            phase->mDuration.saveToStream(&out);
        }

        // -----------------------------------------------------
        //  Write events data
        // -----------------------------------------------------
        foreach (Event* event, mEvents) {
           event->mTheta.saveToStream(&out);
        }

        // -----------------------------------------------------
        //  Write dates data
        // -----------------------------------------------------
        for(int i=0; i<mEvents.size(); ++i)
        {
            Event* event = mEvents[i];
            QList<Date>& dates = event->mDates;
            for(int j=0; j<dates.size(); ++j)
            {

                dates[j].mTheta.saveToStream(&out);
                dates[j].mSigma.saveToStream(&out);
                dates[j].mWiggle.saveToStream(&out);
                
               
                out << dates[j].mDeltaFixed;
                out << dates[j].mDeltaMin;
                out << dates[j].mDeltaMax;
                out << dates[j].mDeltaAverage;
                out << dates[j].mDeltaError;
                
                out << dates[j].mSettings.mTmin;
                out << dates[j].mSettings.mTmax;
                out << dates[j].mSettings.mStep;
                out << dates[j].mSettings.mStepForced;
                
              //  out << dates[j].mSubDates;
                
                out << dates[j].mCalibration;
                out << dates[j].mRepartition;
                out << dates[j].mCalibHPD;

            }
        }
        out << mLogModel;
        out << mLogMCMC;
        out << mLogResults;
        QByteArray compressedData = qCompress(uncompresedData);
        file.write(compressedData);
        file.close();
    }
  }
}
/** @Brief Read the .dat file, it's the result of the saved computation and uncompress it
 *
 * */
void Model::restoreFromFile(const QString& fileName)
{
    QFile file(fileName);

    if(file.exists() && file.open(QIODevice::ReadOnly))
    {
        QByteArray compressedData = file.readAll();

        QByteArray uncompresedData = qUncompress(compressedData);

/* #ifdef DEBUG
        qDebug() << "Lecture fichier :"<< fileName;
        qDebug() << "TAILLE compressedData :" << compressedData.size();
        qDebug() << "TAILLE uncompresedData :" << uncompresedData.size();
#endif */
        if(uncompresedData.size()!=0)
        {
            QDataStream in(&uncompresedData, QIODevice::ReadOnly);

            // -----------------------------------------------------
            //  Read info
            // -----------------------------------------------------

            qint32 numPhases = 0;
            in >> numPhases;

            qint32 numEvents = 0;
            in >> numEvents;

            qint32 numdates = 0;
            in >> numdates;

            // -----------------------------------------------------
            //  Read phases data
            // -----------------------------------------------------

            for(int i=0; i<mPhases.size(); ++i)
            {
                /*in >> mPhases[i]->mAlpha.mTrace;
                in >> mPhases[i]->mBeta.mTrace;
                in >> mPhases[i]->mDuration.mTrace;*/
                mPhases[i]->mAlpha.loadFromStream(&in);
                mPhases[i]->mBeta.loadFromStream(&in);
                mPhases[i]->mDuration.loadFromStream(&in);
            }

            // -----------------------------------------------------
            //  Read events data
            // -----------------------------------------------------

            for(int i=0; i<mEvents.size(); ++i)
            {
                /*in >> mEvents[i]->mTheta.mTrace;
                in >> mEvents[i]->mTheta.mHistoryAcceptRateMH;
                in >> mEvents[i]->mTheta.mAllAccepts;*/
                mEvents[i]->mTheta.loadFromStream(&in);
            }

            // -----------------------------------------------------
            //  Read dates data
            // -----------------------------------------------------

            for(int i=0; i<mEvents.size(); ++i)
            {
                for(int j=0; j<mEvents[i]->mDates.size(); ++j)
                {
                    /*in >> mEvents[i]->mDates[j].mTheta.mTrace;
                    in >> mEvents[i]->mDates[j].mTheta.mHistoryAcceptRateMH;
                    in >> mEvents[i]->mDates[j].mTheta.mAllAccepts;

                    in >> mEvents[i]->mDates[j].mSigma.mTrace;
                    in >> mEvents[i]->mDates[j].mSigma.mHistoryAcceptRateMH;
                    in >> mEvents[i]->mDates[j].mSigma.mAllAccepts;

                    in >> mEvents[i]->mDates[j].mWiggle.mTrace;
                    in >> mEvents[i]->mDates[j].mWiggle.mHistoryAcceptRateMH;
                    in >> mEvents[i]->mDates[j].mWiggle.mAllAccepts;*/

                    mEvents[i]->mDates[j].mTheta.loadFromStream(&in);
                    mEvents[i]->mDates[j].mSigma.loadFromStream(&in);
                    mEvents[i]->mDates[j].mWiggle.loadFromStream(&in);
                                                            
                    
                    in >> mEvents[i]->mDates[j].mDeltaFixed;
                    in >> mEvents[i]->mDates[j].mDeltaMin;
                    in >> mEvents[i]->mDates[j].mDeltaMax;
                    in >> mEvents[i]->mDates[j].mDeltaAverage;
                    in >> mEvents[i]->mDates[j].mDeltaError;
                    
                    in >> mEvents[i]->mDates[j].mSettings.mTmin;
                    in >> mEvents[i]->mDates[j].mSettings.mTmax;
                    in >> mEvents[i]->mDates[j].mSettings.mStep;
                    in >> mEvents[i]->mDates[j].mSettings.mStepForced;
                    
                   // in >> mEvents[i]->mDates[j].mSubDates;
                    
                    in >> mEvents[i]->mDates[j].mCalibration;
                    in >> mEvents[i]->mDates[j].mRepartition;
                    in >> mEvents[i]->mDates[j].mCalibHPD;
                    
                     if (mEvents[i]->mDates[j].mCalibration.isEmpty()) qDebug()<<"Model::restoreFromFile vide";
                    
                }
            }
            in >> mLogModel;
            in >> mLogMCMC;
            in >> mLogResults;
        
            generateCorrelations(mChains);
            generatePosteriorDensities(mChains, 1024, 1);
            generateNumericalResults(mChains);
        }
        file.close();
    }
    
}
