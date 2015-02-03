#ifndef EVENTCONSTRAINT_H
#define EVENTCONSTRAINT_H

#include "Constraint.h"

class Event;


class EventConstraint: public Constraint
{
public:
    EventConstraint();
    EventConstraint(const EventConstraint& ec);
    EventConstraint& operator=(const EventConstraint& ec);
    void copyFrom(const Constraint& ec);
    virtual ~EventConstraint();
    
    static EventConstraint fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
public:
    Event* mEventFrom;
    Event* mEventTo;
};

#endif
