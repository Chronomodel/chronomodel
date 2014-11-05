#ifndef EVENTCONSTRAINT_H
#define EVENTCONSTRAINT_H

#include <QJsonObject>

class Event;

#define STATE_EVENT_CONSTRAINT_ID "id"
#define STATE_EVENT_CONSTRAINT_BWD_ID "bwd_id"
#define STATE_EVENT_CONSTRAINT_FWD_ID "fwd_id"


class EventConstraint
{
public:
    enum PhiType{
        ePhiUnknown = 0,
        ePhiRange = 1,
        ePhiFixed = 2
    };
    
    EventConstraint();
    EventConstraint(const EventConstraint& ec);
    EventConstraint& operator=(const EventConstraint& ec);
    void copyFrom(const EventConstraint& ec);
    virtual ~EventConstraint();
    
    static EventConstraint fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
public:
    int mId;
    int mEventFromId;
    int mEventToId;
    
    Event* mEventFrom;
    Event* mEventTo;
};

#endif
