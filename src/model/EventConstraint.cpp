#include "EventConstraint.h"
#include "Event.h"


EventConstraint::EventConstraint():
mId(-1),
mEventFromId(0),
mEventToId(0),
mEventFrom(0),
mEventTo(0)
{
    
}

EventConstraint::EventConstraint(const EventConstraint& ec)
{
    copyFrom(ec);
}

EventConstraint& EventConstraint::operator=(const EventConstraint& ec)
{
    copyFrom(ec);
    return *this;
}

void EventConstraint::copyFrom(const EventConstraint& ec)
{
    mId = ec.mId;
    mEventFromId = ec.mEventFromId;
    mEventToId = ec.mEventToId;
    
    mEventFrom = ec.mEventFrom;
    mEventTo = ec.mEventTo;
}

EventConstraint::~EventConstraint()
{
    
}

EventConstraint EventConstraint::fromJson(const QJsonObject& json)
{
    EventConstraint c;
    c.mId = json[STATE_EVENT_CONSTRAINT_ID].toInt();
    c.mEventFromId = json[STATE_EVENT_CONSTRAINT_BWD_ID].toInt();
    c.mEventToId = json[STATE_EVENT_CONSTRAINT_FWD_ID].toInt();
    return c;
}

QJsonObject EventConstraint::toJson() const
{
    QJsonObject json;
    json[STATE_EVENT_CONSTRAINT_ID] = mId;
    json[STATE_EVENT_CONSTRAINT_BWD_ID] = mEventFromId;
    json[STATE_EVENT_CONSTRAINT_FWD_ID] = mEventToId;
    return json;
}

