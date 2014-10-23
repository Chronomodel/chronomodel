#include "Project.h"
#include "ProjectManager.h"
#include "Model.h"
#include "PluginManager.h"
#include "ProjectSettingsDialog.h"
#include "MCMCSettingsDialog.h"

#include "Event.h"
#include "EventKnown.h"
#include "EventDialog.h"
#include "EventConstraint.h"
#include "EventConstraintDialog.h"

#include "Phase.h"
#include "PhaseConstraint.h"
#include "PhaseDialog.h"
#include "PhaseConstraintDialog.h"

#include "DateDialog.h"
#include "Date.h"
#include "../PluginAbstract.h"
#include "../PluginFormAbstract.h"
#include "TrashDialog.h"
#include "ImportDataView.h"

#include "ModelUtilities.h"
#include "QtUtilities.h"

#include "MCMCLoopMain.h"
#include "MCMCProgressDialog.h"

#include "SetProjectState.h"
#include "StateEvent.h"

#include <iostream>
#include <QtWidgets>
#include <QThread>
#include <QJsonObject>



Project::Project():
mName(tr("Chronomodel Project")),
mProjectFileDir(""),
mProjectFileName(QObject::tr("Untitled"))
{
    initState();
    
    mAutoSaveTimer = new QTimer(this);
    connect(mAutoSaveTimer, SIGNAL(timeout()), this, SLOT(save()));
    mAutoSaveTimer->start(3000);
}

Project::~Project()
{
    mAutoSaveTimer->stop();
}


#pragma mark Project State

void Project::initState()
{
    mState = emptyState();
}

QJsonObject Project::emptyState() const
{
    QJsonObject state;
    
    ProjectSettings projectSettings;
    QJsonObject settings = projectSettings.toJson();
    state[STATE_SETTINGS] = settings;
    
    MCMCSettings mcmcSettings;
    QJsonObject mcmc = mcmcSettings.toJson();
    state[STATE_MCMC] = mcmc;
    
    QJsonArray events;
    state[STATE_EVENTS] = events;
    
    QJsonArray phases;
    state[STATE_PHASES] = phases;
    
    QJsonArray events_constraints;
    state[STATE_EVENTS_CONSTRAINTS] = events_constraints;
    
    QJsonArray phases_constraints;
    state[STATE_PHASES_CONSTRAINTS] = phases_constraints;
    
    QJsonArray dates_trash;
    state[STATE_DATES_TRASH] = dates_trash;
    
    QJsonArray events_trash;
    state[STATE_EVENTS_TRASH] = events_trash;
    
    return state;
}

QJsonObject Project::state() const
{
    return mState;
}

void Project::pushProjectState(const QJsonObject& state, const QString& reason, bool notify)
{
    if(mState != state)
    {
        SetProjectState* command = new SetProjectState(this, mState, state, reason, notify);
        ProjectManager::getUndoStack().push(command);
    }
}

void Project::sendUpdateState(const QJsonObject& state, const QString& reason, bool notify)
{
    qDebug() << "+++ Sending : " << reason;
    StateEvent* event = new StateEvent(state, reason, notify);
    QCoreApplication::postEvent(this, event, Qt::NormalEventPriority);
}

bool Project::event(QEvent* e)
{
    if(e->type() == QEvent::User)
    {
        StateEvent* se = static_cast<StateEvent*>(e);
        if(se)
        {
            updateState(se->state(), se->reason(), se->notify());
        }
        return true;
    }
    else
    {
        return QObject::event(e);
    }
}

void Project::updateState(const QJsonObject& state, const QString& reason, bool notify)
{
    qDebug() << "--- Receiving : " << reason;
    mState = state;
    if(notify)
        emit projectStateChanged();
}



#pragma mark Project File Management

bool Project::load(const QString& path)
{
    QFile file(path);
    if(file.open(QIODevice::ReadOnly))
    {
        QFileInfo info(path);
        ProjectManager::setCurrentPath(info.absolutePath());
        
        mProjectFileDir = info.absolutePath();
        mProjectFileName = info.fileName();
        
        QByteArray saveData = file.readAll();
        QJsonDocument jsonDoc(QJsonDocument::fromJson(saveData));
        mState = jsonDoc.object();
        
        pushProjectState(mState, "project loaded", true);
        
        file.close();
        
        return true;
    }
    return false;
}

bool Project::save()
{
    qDebug() << "Project::save() : " << mProjectFileDir + "/" + mProjectFileName;
    QFileInfo info(mProjectFileDir + "/" + mProjectFileName);
    return info.exists() ? saveProjectToFile() : saveAs();
}

bool Project::saveAs()
{
    QString path = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                tr("Save File"),
                                                ProjectManager::getCurrentPath(),
                                                tr("Chronomodel Project (*.chr)"));
    if(!path.isEmpty())
    {
        QFileInfo info(path);
        ProjectManager::setCurrentPath(info.absolutePath());
        
        mProjectFileDir = info.absolutePath();
        mProjectFileName = info.fileName();
        
        return saveProjectToFile();
    }
    return false;
}

bool Project::askToSave()
{
    // Check if modifs have been made
    QFile file(mProjectFileDir + "/" + mProjectFileName);
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray saveData = file.readAll();
        QJsonDocument jsonDoc(QJsonDocument::fromJson(saveData));
        QJsonObject fileState = jsonDoc.object();
    
        if(mState == fileState)
            return true;
    }
    
    // We have some modifications : ask to save :
    int result = QMessageBox::question(QApplication::activeWindow(),
                                       QApplication::applicationName(),
                                       tr("Do you want to save the project ?"),
                                       QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    
    if(result == QMessageBox::Yes)
    {
        // return true if saving is done correcty
        return save();
    }
    else if(result == QMessageBox::No)
    {
        // the user doesn't want to save : returning true to continue
        return true;
    }
    else if(result == QMessageBox::Cancel)
    {
        // the user canceled : return false to cancel any further operations
        return false;
    }
    return false;
}

bool Project::saveProjectToFile()
{
    QFile file(mProjectFileDir + "/" + mProjectFileName);
    if(file.open(QIODevice::ReadWrite))
    {
        QJsonDocument jsonDoc(mState);
        file.write(jsonDoc.toJson(QJsonDocument::Indented));
        file.resize(file.pos());
        file.close();
        
        return true;
    }
    return false;
}

// --------------------------------------------------------------------
//     Project Settings
// --------------------------------------------------------------------
#pragma mark Settings
void Project::setSettings(const ProjectSettings& settings)
{
    if(settings.mTmin >= settings.mTmax)
    {
        QMessageBox message(QMessageBox::Critical, tr("Inconsistent values"), tr("Start Date must be lower than End Date !"), QMessageBox::Ok, qApp->activeWindow(), Qt::Sheet);
    }
    else
    {
        QJsonObject stateNext = mState;
        stateNext[STATE_SETTINGS] = settings.toJson();
        pushProjectState(stateNext, tr("Settings updated"), true);
    }
}

void Project::setAppSettings(const Settings& settings)
{
    mAutoSaveTimer->setInterval(settings.mAutoSaveDelay * 1000);
    if(mAutoSaveTimer->isActive() && !settings.mAutoSave)
        mAutoSaveTimer->stop();
    else if(!mAutoSaveTimer->isActive() && settings.mAutoSave)
        mAutoSaveTimer->start();
}

void Project::mcmcSettings()
{
    MCMCSettingsDialog dialog(qApp->activeWindow(), Qt::Sheet);
    MCMCSettings settings = MCMCSettings::fromJson(mState[STATE_MCMC].toObject());
    dialog.setSettings(settings);
    dialog.setModal(true);
    
    if(dialog.exec() == QDialog::Accepted)
    {
        MCMCSettings settings = dialog.getSettings();
        
        QJsonObject stateNext = mState;
        stateNext[STATE_MCMC] = settings.toJson();
        pushProjectState(stateNext, tr("MCMC Settings updated"), true);
    }
}

// --------------------------------------------------------------------
//     Events
// --------------------------------------------------------------------
#pragma mark Events
int Project::getUnusedEventId(const QJsonArray& events)
{
    int id = -1;
    bool idIsFree = false;
    while(!idIsFree)
    {
        ++id;
        idIsFree = true;
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            if(event[STATE_EVENT_ID].toInt() == id)
                idIsFree = false;
        }
    }
    return id;
}

void Project::createEvent()
{
    EventDialog dialog(qApp->activeWindow(), Qt::Sheet);
    if(dialog.exec() == QDialog::Accepted)
    {
        Event event;
        event.mName = dialog.getName();
        event.mColor = dialog.getColor();
        
        addEvent(event.toJson(), tr("Event created"));
    }
}

void Project::createEventKnown()
{
    EventDialog dialog(qApp->activeWindow(), Qt::Sheet);
    if(dialog.exec() == QDialog::Accepted)
    {
        EventKnown event;
        event.mName = dialog.getName();
        event.mColor = dialog.getColor();
        
        addEvent(event.toJson(), tr("Bound created"));
    }
}

void Project::addEvent(QJsonObject event, const QString& reason)
{
    QJsonObject stateNext = mState;
    QJsonArray events = stateNext[STATE_EVENTS].toArray();
    
    event[STATE_EVENT_ID] = getUnusedEventId(events);
    events.append(event);
    stateNext[STATE_EVENTS] = events;
    
    pushProjectState(stateNext, reason, true);
}


void Project::deleteSelectedEvents()
{
    QJsonObject stateNext = mState;
    
    QJsonArray events = mState[STATE_EVENTS].toArray();
    QJsonArray events_constraints = mState[STATE_EVENTS_CONSTRAINTS].toArray();
    QJsonArray events_trash = mState[STATE_EVENTS_TRASH].toArray();
    
    for(int i=events.size()-1; i>=0; --i)
    {
        QJsonObject event = events[i].toObject();
        if(event[STATE_EVENT_IS_SELECTED].toBool())
        {
            int event_id = event[STATE_EVENT_ID].toInt();
            for(int j=events_constraints.size()-1; j>=0; --j)
            {
                QJsonObject constraint = events_constraints[j].toObject();
                int bwd_id = constraint[STATE_EVENT_CONSTRAINT_BWD_ID].toInt();
                int fwd_id = constraint[STATE_EVENT_CONSTRAINT_FWD_ID].toInt();
                if(bwd_id == event_id || fwd_id == event_id)
                {
                    events_constraints.removeAt(j);
                }
            }
            events.removeAt(i);
            events_trash.append(event);
        }
    }
    stateNext[STATE_EVENTS] = events;
    stateNext[STATE_EVENTS_CONSTRAINTS] = events_constraints;
    stateNext[STATE_EVENTS_TRASH] = events_trash;
    
    pushProjectState(stateNext, tr("Event(s) deleted"), true);
}

void Project::recycleEvents()
{
    TrashDialog dialog(TrashDialog::eEvent, qApp->activeWindow(), Qt::Sheet);
    if(dialog.exec() == QDialog::Accepted)
    {
        QList<int> indexes = dialog.getSelectedIndexes();
        qDebug() << indexes;
        
        QJsonObject stateNext = mState;
        QJsonArray events = mState[STATE_EVENTS].toArray();
        QJsonArray events_trash = mState[STATE_EVENTS_TRASH].toArray();
        
        for(int i=indexes.size()-1; i>=0; --i)
        {
            QJsonObject event = events_trash.takeAt(indexes[i]).toObject();
            event[STATE_EVENT_ID] = getUnusedEventId(events);
            events.append(event);
        }
        stateNext[STATE_EVENTS] = events;
        stateNext[STATE_EVENTS_TRASH] = events_trash;
        
        pushProjectState(stateNext, tr("Event(s) restored"), true);
    }
}

void Project::updateEvent(const QJsonObject& event, const QString& reason)
{
    QJsonObject stateNext = mState;
    QJsonArray events = mState[STATE_EVENTS].toArray();
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject evt = events[i].toObject();
        if(evt[STATE_EVENT_ID].toInt() == event[STATE_EVENT_ID].toInt())
        {
            events[i] = event;
            break;
        }
    }
    stateNext[STATE_EVENTS] = events;
    pushProjectState(stateNext, reason, true);
}


/*void Project::mergeEvents(Event* srcEvent, Event* tgtEvent)
{
    if(srcEvent && tgtEvent)
    {
        for(int i=0; i<(int)srcEvent->mConstraintsBwd.size(); ++i)
            deleteEventConstraint(srcEvent->mConstraintsBwd[i]);
        
        for(int i=0; i<(int)srcEvent->mConstraintsFwd.size(); ++i)
            deleteEventConstraint(srcEvent->mConstraintsFwd[i]);
        
        for(int i=0; i<(int)srcEvent->mDates.size(); ++i)
            addDate(tgtEvent, srcEvent->mDates[i]);
        
        emit eventBeingDeleted(srcEvent);
        
        for(int i=0; i<(int)mEvents.size(); ++i)
        {
            if(mEvents[i] == srcEvent)
                mEvents.erase(mEvents.begin() + i);
        }
        delete srcEvent;
    }
}*/

// --------------------------------------------------------------------
//     Dates
// --------------------------------------------------------------------
#pragma mark Dates
int Project::getUnusedDateId(const QJsonArray& dates)
{
    int id = -1;
    bool idIsFree = false;
    while(!idIsFree)
    {
        ++id;
        idIsFree = true;
        for(int i=0; i<dates.size(); ++i)
        {
            QJsonObject date = dates[i].toObject();
            if(date[STATE_DATE_ID].toInt() == id)
                idIsFree = false;
        }
    }
    return id;
}

Date Project::createDateFromPlugin(PluginAbstract* plugin)
{
    Date date;
    if(plugin)
    {
        DateDialog dialog(qApp->activeWindow(), Qt::Sheet);
        PluginFormAbstract* form = plugin->getForm();
        dialog.setForm(form);
        dialog.setDataMethod(plugin->getDataMethod());
        
        if(dialog.exec() == QDialog::Accepted)
        {
            date.mPlugin = plugin;
            date.mData = form->getData();
            
            date.mName = dialog.getName();
            date.mMethod = dialog.getMethod();
            date.mDeltaType = dialog.getDeltaType();
            date.mDeltaFixed = dialog.getDeltaFixed();
            date.mDeltaMin = dialog.getDeltaMin();
            date.mDeltaMax = dialog.getDeltaMax();
            date.mDeltaAverage = dialog.getDeltaAverage();
            date.mDeltaError = dialog.getDeltaError();
        }
    }
    return date;
}

Date Project::createDateFromData(const QString& pluginName, const QStringList& dataStr)
{
    Date date;
    PluginAbstract* plugin = PluginManager::getPluginFromName(pluginName);
    if(plugin)
    {
        date.mName = dataStr[0];
        date.mPlugin = plugin;
        date.mMethod = plugin->getDataMethod();
        date.mData = plugin->dataFromList(dataStr);
        
        int minColNum = plugin->csvMinColumns();
        if(dataStr.size() > minColNum)
        {
            QString delta = dataStr[minColNum];
            if(!delta.contains("//"))
                date.mDeltaFixed = delta.toDouble();
        }
    }
    return date;
}

void Project::addDate(int eventId, QJsonObject date)
{
    QJsonObject stateNext = mState;
    QJsonArray events = mState[STATE_EVENTS].toArray();
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject evt = events[i].toObject();
        if(evt[STATE_EVENT_ID].toInt() == eventId)
        {
            date[STATE_DATE_ID] = getUnusedDateId(evt[STATE_EVENT_DATES].toArray());
            QJsonArray dates = evt[STATE_EVENT_DATES].toArray();
            dates.append(date);
            
            evt[STATE_EVENT_DATES] = dates;
            events[i] = evt;
            stateNext[STATE_EVENTS] = events;
            pushProjectState(stateNext, tr("Date created"), true);
            break;
        }
    }
}

void Project::updateDate(int eventId, int dateIndex)
{
    QJsonObject state = mState;
    QJsonArray events = mState[STATE_EVENTS].toArray();
    
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        if(event[STATE_EVENT_ID].toInt() == eventId)
        {
            QJsonArray dates = event[STATE_EVENT_DATES].toArray();
            if(dateIndex < dates.size())
            {
                QJsonObject date = dates[dateIndex].toObject();
                
                QString pluginId = date[STATE_DATE_PLUGIN_ID].toString();
                PluginAbstract* plugin = PluginManager::getPluginFromId(pluginId);
                
                DateDialog dialog(qApp->activeWindow(), Qt::Sheet);
                PluginFormAbstract* form = plugin->getForm();
                dialog.setForm(form);
                dialog.setDate(date);
                
                if(dialog.exec() == QDialog::Accepted)
                {
                    date[STATE_DATE_DATA] = form->getData();
                    date[STATE_DATE_NAME] = dialog.getName();
                    date[STATE_DATE_METHOD] = dialog.getMethod();
                    
                    date[STATE_DATE_DELTA_TYPE] = dialog.getDeltaType();
                    date[STATE_DATE_DELTA_FIXED] = dialog.getDeltaFixed();
                    date[STATE_DATE_DELTA_MIN] = dialog.getDeltaMin();
                    date[STATE_DATE_DELTA_MAX] = dialog.getDeltaMax();
                    date[STATE_DATE_DELTA_AVERAGE] = dialog.getDeltaAverage();
                    date[STATE_DATE_DELTA_ERROR] = dialog.getDeltaError();
                    
                    dates[dateIndex] = date;
                    event[STATE_EVENT_DATES] = dates;
                    events[i] = event;
                    state[STATE_EVENTS] = events;
                    
                    pushProjectState(state, tr("Date updated"), true);
                    
                    //date->calibrate(mSettings.mTmin, mSettings.mTmax, mSettings.mStep);
                }
            }
            break;
        }
    }
}

void Project::deleteDates(int eventId, const QList<int>& dateIndexes)
{
    QJsonObject state = mState;
    
    QJsonArray events = state[STATE_EVENTS].toArray();
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        if(event[STATE_EVENT_ID].toInt() == eventId)
        {
            QJsonArray dates_trash = state[STATE_DATES_TRASH].toArray();
            QJsonArray dates = event[STATE_EVENT_DATES].toArray();
            
            for(int j=dates.size()-1; j>=0; --j)
            {
                if(dateIndexes.contains(j))
                {
                    QJsonObject date = dates.takeAt(j).toObject();
                    dates_trash.append(date);
                }
            }
            event[STATE_EVENT_DATES] = dates;
            events[i] = event;
            state[STATE_EVENTS] = events;
            state[STATE_DATES_TRASH] = dates_trash;
            
            pushProjectState(state, QString::number(dateIndexes.size()) + tr(" date(s) deleted"), true);
            
            break;
        }
    }
}

void Project::recycleDates(int eventId)
{
    TrashDialog dialog(TrashDialog::eDate, qApp->activeWindow(), Qt::Sheet);
    if(dialog.exec() == QDialog::Accepted)
    {
        QList<int> indexes = dialog.getSelectedIndexes();
        qDebug() << indexes;
        
        QJsonObject stateNext = mState;
        QJsonArray events = mState[STATE_EVENTS].toArray();
        QJsonArray dates_trash = mState[STATE_DATES_TRASH].toArray();
        
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            if(event[STATE_EVENT_ID].toInt() == eventId)
            {
                QJsonArray dates = event[STATE_EVENT_DATES].toArray();
                for(int i=indexes.size()-1; i>=0; --i)
                {
                    QJsonObject date = dates_trash.takeAt(indexes[i]).toObject();
                    date[STATE_DATE_ID] = getUnusedDateId(dates);
                    dates.append(date);
                }
                event[STATE_EVENT_DATES] = dates;
                events[i] = event;
                stateNext[STATE_EVENTS] = events;
                stateNext[STATE_DATES_TRASH] = dates_trash;
                
                pushProjectState(stateNext, QString::number(indexes.size()) + tr(" date(s) restored"), true);
                
                break;
            }
        }
    }
}


// --------------------------------------------------------------------
//     Event Constraints authorizations
// --------------------------------------------------------------------
/*#pragma mark Event Constraints authorizations

bool Project::isEventConstraintAllowed(Event* eventFrom, Event* eventTo, QString& message)
{
    if(eventFrom && eventTo && (eventFrom != eventTo))
    {
        // ------------------------------------------------------------
        //  Vérifier que la contrainte n'existe pas déjà, dans un sens ou dans l'autre
        // ------------------------------------------------------------
        for(int i=0; i<(int)mEventConstraints.size(); ++i)
        {
            if((mEventConstraints[i]->getEventTo() == eventTo &&
                mEventConstraints[i]->getEventFrom() == eventFrom))
            {
                message = tr("The constraint already exists");
                return false;
            }
            else if(mEventConstraints[i]->getEventTo() == eventFrom &&
                mEventConstraints[i]->getEventFrom() == eventTo)
            {
                message = tr("The constraint already exists in the other direction");
                return false;
            }
        }
        
        // ------------------------------------------------------------
        //  TODO : Vérifier que l'on ne créé pas de boucle
        // ------------------------------------------------------------
        //qDebug() << "Checking loops for " << QString::number(eventFrom->mId);
        QList<Event*> eventsFrom = getAllEventsFrom(eventFrom);
        for(int i=0; i<(int)eventsFrom.size(); ++i)
        {
            if(eventsFrom[i] == eventTo)
            {
                message = tr("You cannot create a loop");
                return false;
            }
        }
        
        // ------------------------------------------------------------
        //  TODO : Vérifier que la contrainte n'est pas redondante
        // ------------------------------------------------------------
        //qDebug() << "Checking duplicates for " << QString::number(eventFrom->mId);
        QList<Event*> eventsTo = getAllEventsTo(eventFrom);
        for(int i=0; i<(int)eventsFrom.size(); ++i)
        {
            QList<Event*> subEventsTo = getAllEventsTo(eventsFrom[i]);
            for(int j=0; j<(int)subEventsTo.size(); ++j)
            {
                if(std::find(eventsTo.begin(), eventsTo.end(), subEventsTo[j]) == eventsTo.end())
                {
                    eventsTo.push_back(subEventsTo[j]);
                }
            }
        }
        for(int i=0; i<(int)eventsTo.size(); ++i)
        {
            //qDebug() << eventsTo[i]->mId << " compared to " << eventTo->mId;
            if(eventsTo[i] == eventTo)
            {
                message = tr("This information already exists thanks to other constraints");
                return false;
            }
        }
        
        return true;
    }
    message = tr("Unvalid events for constraint creation");
    return false;
}

QList<Event*> Project::getAllEventsFrom(Event* event)
{
    QList<Event*> eventsFrom;
    for(int i=0; i<(int)event->mConstraintsBwd.size(); ++i)
    {
        Event* eventFrom = event->mConstraintsBwd[i]->getEventFrom();
        //qDebug() << "- from : " << eventFrom->mId;
        eventsFrom.push_back(eventFrom);
        
        QList<Event*> subEventsFrom = getAllEventsFrom(eventFrom);
        for(int i=0; i<(int)subEventsFrom.size(); ++i)
            eventsFrom.push_back(subEventsFrom[i]);
    }
    return eventsFrom;
}

QList<Event*> Project::getAllEventsTo(Event* event)
{
    QList<Event*> eventsTo;
    for(int i=0; i<(int)event->mConstraintsFwd.size(); ++i)
    {
        Event* eventTo = event->mConstraintsFwd[i]->getEventTo();
        //qDebug() << "- to : " << eventTo->mId;
        eventsTo.push_back(eventTo);
        
        QList<Event*> subEventsTo = getAllEventsTo(eventTo);
        for(int i=0; i<(int)subEventsTo.size(); ++i)
            eventsTo.push_back(subEventsTo[i]);
    }
    return eventsTo;
}*/









// --------------------------------------------------------------------
//     Phase Constraints authorizations
// --------------------------------------------------------------------
/*#pragma mark Phase Constraints authorizations

bool Project::isPhaseConstraintAllowed(Phase* phaseFrom, Phase* phaseTo)
{
    if(phaseFrom && phaseTo && (phaseFrom != phaseTo))
    {
        // ------------------------------------------------------------
        //  Vérifier que la contrainte n'existe pas déjà, dans un sens ou dans l'autre
        // ------------------------------------------------------------
        for(int i=0; i<(int)mPhaseConstraints.size(); ++i)
        {
            if((mPhaseConstraints[i]->getPhaseTo() == phaseTo &&
                mPhaseConstraints[i]->getPhaseFrom() == phaseFrom) ||
               (mPhaseConstraints[i]->getPhaseTo() == phaseFrom &&
                mPhaseConstraints[i]->getPhaseFrom() == phaseTo))
            {
                return false;
            }
        }
        
        // ------------------------------------------------------------
        //  Vérifier que les phases n'ont pas de faits communs
        // ------------------------------------------------------------
        for(int i=0; i<(int)phaseFrom->mEvents.size(); ++i)
        {
            for(int j=0; j<(int)phaseTo->mEvents.size(); ++j)
            {
                if(phaseFrom->mEvents[i] == phaseTo->mEvents[j])
                    return false;
            }
        }
        // ------------------------------------------------------------
        //  TODO : Vérifier que l'on ne créé pas de boucle
        // ------------------------------------------------------------
        //qDebug() << "Checking loops for " << QString::number(phaseFrom->mId);
        QList<Phase*> phasesFrom = getAllPhasesFrom(phaseFrom);
        for(int i=0; i<(int)phasesFrom.size(); ++i)
        {
            if(phasesFrom[i] == phaseTo)
                return false;
        }
        
        // ------------------------------------------------------------
        //  TODO : Vérifier que la contrainte n'est pas redondante
        // ------------------------------------------------------------
        //qDebug() << "Checking duplicates for " << QString::number(phaseFrom->mId);
        QList<Phase*> phasesTo = getAllPhasesTo(phaseFrom);
        for(int i=0; i<(int)phasesFrom.size(); ++i)
        {
            QList<Phase*> subPhasesTo = getAllPhasesTo(phasesFrom[i]);
            for(int j=0; j<(int)subPhasesTo.size(); ++j)
            {
                if(std::find(phasesTo.begin(), phasesTo.end(), subPhasesTo[j]) == phasesTo.end())
                {
                    phasesTo.push_back(subPhasesTo[j]);
                }
            }
        }
        for(int i=0; i<(int)phasesTo.size(); ++i)
        {
            //qDebug() << phasesTo[i]->mId << " compared to " << phaseTo->mId;
            if(phasesTo[i] == phaseTo)
                return false;
        }
        
        return true;
    }
    return false;
    return true;
}

QList<Phase*> Project::getAllPhasesFrom(Phase* phase)
{
    QList<Phase*> phasesFrom;
    for(int i=0; i<(int)phase->mConstraintsBwd.size(); ++i)
    {
        Phase* phaseFrom = phase->mConstraintsBwd[i]->getPhaseFrom();
        //qDebug() << "- from : " << phaseFrom->mId;
        phasesFrom.push_back(phaseFrom);
        
        QList<Phase*> subPhasesFrom = getAllPhasesFrom(phaseFrom);
        for(int i=0; i<(int)subPhasesFrom.size(); ++i)
            phasesFrom.push_back(subPhasesFrom[i]);
    }
    return phasesFrom;
}

QList<Phase*> Project::getAllPhasesTo(Phase* phase)
{
    QList<Phase*> phasesTo;
    for(int i=0; i<(int)phase->mConstraintsFwd.size(); ++i)
    {
        Phase* phaseTo = phase->mConstraintsFwd[i]->getPhaseTo();
        //qDebug() << "- to : " << phaseTo->mId;
        phasesTo.push_back(phaseTo);
        
        QList<Phase*> subPhasesTo = getAllPhasesTo(phaseTo);
        for(int i=0; i<(int)subPhasesTo.size(); ++i)
            phasesTo.push_back(subPhasesTo[i]);
    }
    return phasesTo;
}*/




// -------

void Project::exportAsText()
{
    /*QString currentDir = ProjectManager::getCurrentPath();
    
    QString path = QFileDialog::getExistingDirectory(qApp->activeWindow(), tr("Location"), currentDir);
    if(!path.isEmpty())
    {
        ProjectManager::setCurrentPath(path);
        
        QString folderBaseName = mName.simplified().toLower().replace(" ", "_");
        QString folderName = folderBaseName;
        int index = 1;
        while(QFileInfo::exists(path + "/" + folderName))
        {
            folderName = folderBaseName + " (" + QString::number(index) + ")";
            ++index;
        }
        QDir dir(path);
        if(dir.mkdir(folderName))
        {
            dir = QDir(path + "/" + folderName);
            
            QString descFileName = "description.txt";
            QFile descFile(dir.absoluteFilePath(descFileName));
            if(descFile.open(QIODevice::ReadWrite))
            {
                QTextStream stream(&descFile);
                
                stream << "Project name : " << mName << endl;
                
                stream << endl << "****************************************" << endl;
                stream << "EVENTS" << endl;
                stream << "****************************************" << endl;
                
                for(int i=0; i<mEvents.size(); ++i)
                {
                    Event* event = mEvents[i];
                    stream << endl << "++++++++++++++++++++++++++++++++++++++++" << endl;
                    stream << "EVENT : " << endl;
                    stream << "- Name : " << event->mName << endl;
                    stream << "- Method : " << ModelUtilities::getEventMethodText(event->mMethod) << endl;
                    stream << "- Num data : " << QString::number(event->mDates.size()) << endl;
                    stream << "- Num phases : " << QString::number(event->mPhases.size()) << endl;
                    stream << "++++++++++++++++++++++++++++++++++++++++" << endl;
                    
                    for(int j=0; j<event->mDates.size(); ++j)
                    {
                        Date* date = (Date*) event->mDates[j];
                        stream << "DATA : " << endl;
                        stream << "- Name : " << date->mName << endl;
                        stream << "- Type : " << date->mPlugin->getName() << endl;
                        stream << "---------------" << endl;
                    }
                }
                
                stream << endl << "****************************************" << endl;
                stream << "PHASES" << endl;
                stream << "****************************************" << endl;
                
                for(int i=0; i<mPhases.size(); ++i)
                {
                    Phase* phase = mPhases[i];
                    stream << endl << "++++++++++++++++++++++++++++++++++++++++" << endl;
                    stream << "PHASE : " << endl;
                    stream << "- Name : " << phase->mName << endl;
                    stream << "- Num events : " << QString::number(phase->mEvents.size()) << endl;
                    stream << "++++++++++++++++++++++++++++++++++++++++" << endl;
                }
            }
        }
    }*/
    
    /*QMessageBox message(QMessageBox::Critical, tr("TODO"), tr("create readable textual representation of XML project file"), QMessageBox::Ok, qApp->activeWindow(), Qt::Sheet);
    message.exec();*/
}

// --------------------------------------------------------------------
//     Project Run
// --------------------------------------------------------------------
#pragma mark Run Project
void Project::run()
{
    mModels.clear();
    Model model = Model::fromJson(mState);
    mModels.append(model);
    
    MCMCLoopMain loop(mModels[0]);
    MCMCProgressDialog dialog(&loop, qApp->activeWindow(), Qt::Sheet);
    if(dialog.startMCMC() == QDialog::Accepted)
    {
        emit mcmcFinished(mModels[0]);
    }
}



/*void Project::calibrateAllDates()
 {
 QList<Date*> dates;
 for(int i=0; i<mEvents.size(); ++i)
 {
 Event* event = mEvents[i];
 for(int j=0; j<event->mDates.size(); ++j)
 {
 dates.push_back(event->mDates[j]);
 }
 }
 
 QProgressDialog progress(qApp->activeWindow(), Qt::Sheet);
 progress.setLabelText(tr("Calibrating data..."));
 progress.setRange(0, dates.size());
 progress.setModal(true);
 progress.setCancelButton(0);
 progress.setValue(0);
 progress.setMinimumDuration(0);
 progress.show();
 
 for(int i=0; i<dates.size(); ++i)
 {
 progress.setValue(i);
 if(progress.wasCanceled())
 break;
 
 dates[i]->calibrate(mSettings.mTmin, mSettings.mTmax, mSettings.mStep);
 }
 
 progress.setValue(dates.size());
 }*/