#include "Constraint.h"


Constraint::Constraint(QObject *parent): QObject( parent ),
mId(-1),
mFromId(0),
mToId(0)
{
    
}

Constraint::Constraint(const Constraint& ec, QObject *parent): QObject( parent )
{
    copyFrom(ec);
}

Constraint& Constraint::operator=(const Constraint& ec)
{
    copyFrom(ec);
    return *this;
}

void Constraint::copyFrom(const Constraint& ec)
{
    mId = ec.mId;
    mFromId = ec.mFromId;
    mToId = ec.mToId;
}

Constraint::~Constraint()
{
    
}

Constraint Constraint::fromJson(const QJsonObject& json)
{
    Constraint c;
    c.mId = json[STATE_ID].toInt();
    c.mFromId = json[STATE_CONSTRAINT_BWD_ID].toInt();
    c.mToId = json[STATE_CONSTRAINT_FWD_ID].toInt();
    return c;
}

QJsonObject Constraint::toJson() const
{
    QJsonObject json;
    json[STATE_ID] = mId;
    json[STATE_CONSTRAINT_BWD_ID] = mFromId;
    json[STATE_CONSTRAINT_FWD_ID] = mToId;
    return json;
}

