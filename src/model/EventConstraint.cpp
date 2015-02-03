#include "EventConstraint.h"
#include "Event.h"


EventConstraint::EventConstraint():Constraint(),
mEventFrom(0),
mEventTo(0)
{
    
}

EventConstraint::EventConstraint(const EventConstraint& ec):Constraint()
{
    copyFrom(ec);
}

EventConstraint& EventConstraint::operator=(const EventConstraint& ec)
{
    copyFrom(ec);
    return *this;
}

void EventConstraint::copyFrom(const Constraint& c)
{
    Constraint::copyFrom(c);
    
    const EventConstraint& ec = ((EventConstraint&)c);
    
    mEventFrom = ec.mEventFrom;
    mEventTo = ec.mEventTo;
}

EventConstraint::~EventConstraint()
{
    
}

EventConstraint EventConstraint::fromJson(const QJsonObject& json)
{
    EventConstraint c;
    c.mId = json[STATE_ID].toInt();
    c.mFromId = json[STATE_CONSTRAINT_BWD_ID].toInt();
    c.mToId = json[STATE_CONSTRAINT_FWD_ID].toInt();
    return c;
}

QJsonObject EventConstraint::toJson() const
{
    QJsonObject json = Constraint::toJson();
    return json;
}

