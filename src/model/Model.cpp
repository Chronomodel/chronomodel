#include "Model.h"
#include "Date.h"
#include "Project.h"
#include "EventKnown.h"
#include "MCMCLoopMain.h"
#include "MCMCProgressDialog.h"
#include <QJsonArray>
#include <QtWidgets>


Model::Model():QObject()
{
    
}

/*Model::Model(const Model& model):QObject()
{
    copyFrom(model);
}

Model& Model::operator=(const Model& model)
{
    copyFrom(model);
    return *this;
}

void Model::copyFrom(const Model& model)
{
    mSettings = model.mSettings;
    mMCMCSettings = model.mMCMCSettings;
    
    mEvents = model.mEvents;
    mPhases = model.mPhases;
    mEventConstraints = model.mEventConstraints;
    mPhaseConstraints = model.mPhaseConstraints;
}*/

Model::~Model()
{
    
}

bool sortEvents(Event* e1, Event* e2)
{
    return (e1->mItemY < e2->mItemY);
}

Model* Model::fromJson(const QJsonObject& json)
{
    Model* model = new Model();
    
    if(json.contains(STATE_SETTINGS))
    {
        QJsonObject settings = json[STATE_SETTINGS].toObject();
        model->mSettings = ProjectSettings::fromJson(settings);
    }
    
    if(json.contains(STATE_MCMC))
    {
        QJsonObject mcmc = json[STATE_MCMC].toObject();
        model->mMCMCSettings = MCMCSettings::fromJson(mcmc);
    }
    
    if(json.contains(STATE_PHASES))
    {
        QJsonArray phases = json[STATE_PHASES].toArray();
        for(int i=0; i<phases.size(); ++i)
        {
            QJsonObject phase = phases[i].toObject();
            Phase* p = new Phase(Phase::fromJson(phase));
            model->mPhases.append(p);
        }
    }
    
    if(json.contains(STATE_EVENTS))
    {
        QJsonArray events = json[STATE_EVENTS].toArray();
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            if(event[STATE_EVENT_TYPE].toInt() == Event::eDefault)
            {
                Event* e = new Event(Event::fromJson(event));
                model->mEvents.append(e);
            }
            else
            {
                EventKnown* e = new EventKnown(EventKnown::fromJson(event));
                model->mEvents.append(e);
            }
        }
    }
    
    // Sort events based on items y position
    std::sort(model->mEvents.begin(), model->mEvents.end(), sortEvents);
    
    if(json.contains(STATE_EVENTS_CONSTRAINTS))
    {
        QJsonArray constraints = json[STATE_EVENTS_CONSTRAINTS].toArray();
        for(int i=0; i<constraints.size(); ++i)
        {
            QJsonObject constraint = constraints[i].toObject();
            EventConstraint* c = new EventConstraint(EventConstraint::fromJson(constraint));
            model->mEventConstraints.append(c);
        }
    }
    
    if(json.contains(STATE_PHASES_CONSTRAINTS))
    {
        QJsonArray constraints = json[STATE_PHASES_CONSTRAINTS].toArray();
        for(int i=0; i<constraints.size(); ++i)
        {
            QJsonObject constraint = constraints[i].toObject();
            qDebug() << constraint;
            PhaseConstraint* c = new PhaseConstraint(PhaseConstraint::fromJson(constraint));
            model->mPhaseConstraints.append(c);
        }
    }
    
    // ------------------------------------------------------------
    //  Link objects to each other
    //  Must be done here !
    //  nb : Les data sont déjà linkées aux events à leur création
    // ------------------------------------------------------------
    for(int i=0; i<model->mEvents.size(); ++i)
    {
        int eventId = model->mEvents[i]->mId;
        QList<int> phasesIds = model->mEvents[i]->mPhasesIds;
        
        // Link des events / phases
        for(int j=0; j<model->mPhases.size(); ++j)
        {
            int phaseId = model->mPhases[j]->mId;
            if(phasesIds.contains(phaseId))
            {
                model->mEvents[i]->mPhases.append(model->mPhases[j]);
                model->mPhases[j]->mEvents.append(model->mEvents[i]);
            }
        }
        
        // Link des events / contraintes d'event
        for(int j=0; j<model->mEventConstraints.size(); ++j)
        {
            if(model->mEventConstraints[j]->mFromId == eventId)
            {
                model->mEventConstraints[j]->mEventFrom = model->mEvents[i];
                model->mEvents[i]->mConstraintsFwd.append(model->mEventConstraints[j]);
            }
            else if(model->mEventConstraints[j]->mToId == eventId)
            {
                model->mEventConstraints[j]->mEventTo = model->mEvents[i];
                model->mEvents[i]->mConstraintsBwd.append(model->mEventConstraints[j]);
            }
        }
    }
    // Link des phases / contraintes de phase
    for(int i=0; i<model->mPhases.size(); ++i)
    {
        int phaseId = model->mPhases[i]->mId;
        qDebug() << "Phase " << phaseId;
        for(int j=0; j<model->mPhaseConstraints.size(); ++j)
        {
            qDebug() << "constr. from " << model->mPhaseConstraints[j]->mFromId;
            qDebug() << "constr. to " << model->mPhaseConstraints[j]->mToId;
            
            if(model->mPhaseConstraints[j]->mFromId == phaseId)
            {
                model->mPhaseConstraints[j]->mPhaseFrom = model->mPhases[i];
                model->mPhases[i]->mConstraintsFwd.append(model->mPhaseConstraints[j]);
            }
            else if(model->mPhaseConstraints[j]->mToId == phaseId)
            {
                model->mPhaseConstraints[j]->mPhaseTo = model->mPhases[i];
                model->mPhases[i]->mConstraintsBwd.append(model->mPhaseConstraints[j]);
            }
        }
    }
    
    
    
    qDebug() << "===========================================";
    qDebug() << "MODEL CREATED";
    qDebug() << "===========================================";
    qDebug() << "=> Events : " << model->mEvents.size();
    for(int i=0; i<model->mEvents.size(); ++i)
    {
        QString objType = "Event";
        if(model->mEvents[i]->type() == Event::eKnown)
            objType = "Bound";
            
        qDebug() << "  => " << objType << " (id: " << model->mEvents[i]->mId << ", name: "<< model->mEvents[i]->mName <<") : " << model->mEvents[i]->mPhases.size() << " phases"
            << ", " << model->mEvents[i]->mDates.size() << " dates"
            << ", " << model->mEvents[i]->mConstraintsBwd.size() << " const. back."
            << ", " << model->mEvents[i]->mConstraintsFwd.size() << " const. fwd.";
    }
    qDebug() << "=> Phases : " << model->mPhases.size();
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
    qDebug() << "===========================================";
    
    return model;
}

QJsonObject Model::toJson() const
{
    QJsonObject json;
    
    json["settings"] = mSettings.toJson();
    json["mcmc"] = mMCMCSettings.toJson();
    
    QJsonArray events;
    for(int i=0; i<mEvents.size(); ++i)
        events.append(mEvents[i]->toJson());
    json["events"] = events;
    
    QJsonArray phases;
    for(int i=0; i<mPhases.size(); ++i)
        phases.append(mPhases[i]->toJson());
    json["phases"] = phases;
    
    QJsonArray event_constraints;
    for(int i=0; i<mEventConstraints.size(); ++i)
        event_constraints.append(mEventConstraints[i]->toJson());
    json["event_constraints"] = event_constraints;
    
    QJsonArray phase_constraints;
    for(int i=0; i<mPhaseConstraints.size(); ++i)
        phase_constraints.append(mPhaseConstraints[i]->toJson());
    json["phase_constraints"] = phase_constraints;
    
    return json;
}

bool Model::isValid()
{
    if(mEvents.size() == 0)
        throw tr("At least one event is required");
    
    for(int i=0; i<mEvents.size(); ++i)
    {
        if(mEvents[i]->type() == Event::eDefault)
        {
            if(mEvents[i]->mDates.size() == 0)
                throw tr("Event") + " " + mEvents[i]->mName + " " + tr("must contain at least 1 data");
        }
    }
    /*for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases[i].mEventsIds.size() == 0)
            throw tr("Phase") + " " + mPhases[i].mName + " " + tr("must contain at least 1 event");
    }*/
    return true;
}
