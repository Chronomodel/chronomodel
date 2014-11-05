#ifndef MODEL_H
#define MODEL_H

#include "ProjectSettings.h"
#include "MCMCSettings.h"
#include "Event.h"
#include "Phase.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"

#include <QObject>
#include <QJsonObject>


class Model: public QObject
{
    Q_OBJECT
public:
    explicit Model();
    Model(const Model& model);
    Model& operator=(const Model& model);
    void copyFrom(const Model& model);
    virtual ~Model();

    static Model* fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    bool isValid();
    
public:
    ProjectSettings mSettings;
    MCMCSettings mMCMCSettings;
    
    QList<Event> mEvents;
    QList<Phase> mPhases;
    QList<EventConstraint> mEventConstraints;
    QList<PhaseConstraint> mPhaseConstraints;
};

#endif

