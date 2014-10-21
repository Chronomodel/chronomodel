#include "EventConstraint.h"
#include "Event.h"


EventConstraint::EventConstraint():
mId(-1),
mEventFromId(0),
mEventToId(0),
mPhiType(EventConstraint::ePhiUnknown),
mPhiFixed(0),
mPhiMin(0),
mPhiMax(0),
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
    
    mPhiType = ec.mPhiType;
    mPhiFixed = ec.mPhiFixed;
    mPhiMin = ec.mPhiMin;
    mPhiMax = ec.mPhiMax;
    
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
    c.mPhiType = (PhiType)json[STATE_EVENT_CONSTRAINT_PHI_TYPE].toInt();
    c.mPhiFixed = json[STATE_EVENT_CONSTRAINT_PHI_FIXED].toDouble();
    c.mPhiMin = json[STATE_EVENT_CONSTRAINT_PHI_MIN].toDouble();
    c.mPhiMax = json[STATE_EVENT_CONSTRAINT_PHI_MAX].toDouble();
    return c;
}

QJsonObject EventConstraint::toJson() const
{
    QJsonObject json;
    json[STATE_EVENT_CONSTRAINT_ID] = mId;
    json[STATE_EVENT_CONSTRAINT_BWD_ID] = mEventFromId;
    json[STATE_EVENT_CONSTRAINT_FWD_ID] = mEventToId;
    json[STATE_EVENT_CONSTRAINT_PHI_TYPE] = mPhiType;
    json[STATE_EVENT_CONSTRAINT_PHI_FIXED] = mPhiFixed;
    json[STATE_EVENT_CONSTRAINT_PHI_MIN] = mPhiMin;
    json[STATE_EVENT_CONSTRAINT_PHI_MAX] = mPhiMax;
    json[STATE_EVENT_CONSTRAINT_BWD_ID] = mEventFrom ? mEventFrom->mId : -1;
    json[STATE_EVENT_CONSTRAINT_FWD_ID] = mEventTo ? mEventTo->mId : -1;
    return json;
}

