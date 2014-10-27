#include "Model.h"
#include "Date.h"
#include "EventKnown.h"
#include "MCMCLoopMain.h"
#include "MCMCProgressDialog.h"
#include <QJsonArray>
#include <QtWidgets>


Model::Model()
{
    
}

Model::Model(const Model& model)
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
}

Model::~Model()
{
    
}


Model Model::fromJson(const QJsonObject& json)
{
    Model model;
    
    if(json.contains("settings"))
    {
        QJsonObject settings = json["settings"].toObject();
        model.mSettings = ProjectSettings::fromJson(settings);
    }
    
    if(json.contains("mcmc"))
    {
        QJsonObject mcmc = json["mcmc"].toObject();
        model.mMCMCSettings = MCMCSettings::fromJson(mcmc);
    }
    
    if(json.contains("events"))
    {
        QJsonArray events = json["events"].toArray();
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            if(event[STATE_EVENT_TYPE].toInt() == Event::eDefault)
            {
                Event e = Event::fromJson(event);
                model.mEvents.append(e);
            }
            else
            {
                EventKnown e = EventKnown::fromJson(event);
                model.mEvents.append(e);
            }
        }
    }
    
    if(json.contains("phases"))
    {
        QJsonArray phases = json["phases"].toArray();
        for(int i=0; i<phases.size(); ++i)
        {
            QJsonObject phase = phases[i].toObject();
            Phase p = Phase::fromJson(phase);
            model.mPhases.append(p);
        }
    }
    
    if(json.contains("event_constraints"))
    {
        QJsonArray constraints = json["event_constraints"].toArray();
        for(int i=0; i<constraints.size(); ++i)
        {
            QJsonObject constraint = constraints[i].toObject();
            EventConstraint c = EventConstraint::fromJson(constraint);
            model.mEventConstraints.append(c);
        }
    }
    
    if(json.contains("phase_constraints"))
    {
        QJsonArray constraints = json["phase_constraints"].toArray();
        for(int i=0; i<constraints.size(); ++i)
        {
            QJsonObject constraint = constraints[i].toObject();
            PhaseConstraint c = PhaseConstraint::fromJson(constraint);
            model.mPhaseConstraints.append(c);
        }
    }
    return model;
}

QJsonObject Model::toJson() const
{
    QJsonObject json;
    
    json["settings"] = mSettings.toJson();
    json["mcmc"] = mMCMCSettings.toJson();
    
    QJsonArray events;
    for(int i=0; i<mEvents.size(); ++i)
        events.append(mEvents[i].toJson());
    json["events"] = events;
    
    QJsonArray phases;
    for(int i=0; i<mPhases.size(); ++i)
        phases.append(mPhases[i].toJson());
    json["phases"] = phases;
    
    QJsonArray event_constraints;
    for(int i=0; i<mEventConstraints.size(); ++i)
        event_constraints.append(mEventConstraints[i].toJson());
    json["event_constraints"] = event_constraints;
    
    QJsonArray phase_constraints;
    for(int i=0; i<mPhaseConstraints.size(); ++i)
        phase_constraints.append(mPhaseConstraints[i].toJson());
    json["phase_constraints"] = phase_constraints;
    
    return json;
}

bool Model::validate()
{
    if(mEvents.size() == 0)
        throw tr("At least one event is required");
    
    for(int i=0; i<mEvents.size(); ++i)
    {
        if(mEvents[i].type() == Event::eDefault)
        {
            if(mEvents[i].mDates.size() == 0)
                throw tr("Event") + " " + mEvents[i].mName + " " + tr("must contain at least 1 data");
        }
    }
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases[i].mEvents.size() == 0)
            throw tr("Phase") + " " + mPhases[i].mName + " " + tr("must contain at least 1 event");
    }
    return true;
}
