/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "EventConstraint.h"
#include "Event.h"
#include "StateKeys.h"

EventConstraint::EventConstraint():
    Constraint(),
    mEventFrom(nullptr),
    mEventTo(nullptr)
{

}

EventConstraint::EventConstraint(const EventConstraint& ec):
    Constraint(static_cast<Constraint>(ec)),
    mEventFrom(ec.mEventFrom),
    mEventTo(ec.mEventTo)
{
}

EventConstraint::EventConstraint(const QJsonObject& json):
    Constraint(json),
    mEventFrom(nullptr),
    mEventTo(nullptr)
{

}

EventConstraint& EventConstraint::operator=(const EventConstraint& ec)
{
    EventConstraint::copyFrom(ec);
    return *this;
}

void EventConstraint::copyFrom(const Constraint& c)
{
    mId = c.mId;
    mFromId = c.mFromId;
    mToId = c.mToId;
    mEventFrom = nullptr;
    mEventTo = nullptr;
}

void EventConstraint::copyFrom(const EventConstraint& ec)
{
    mId = ec.mId;
    mFromId = ec.mFromId;
    mToId = ec.mToId;
    mEventFrom = ec.mEventFrom;
    mEventTo = ec.mEventTo;
}

EventConstraint::~EventConstraint()
{
   /* mEventFrom = nullptr;
    mEventTo = nullptr;*/
}

EventConstraint EventConstraint::fromJson(const QJsonObject& json)
{
    EventConstraint ec;
    ec.mId = json.value(STATE_ID).toInt();
    ec.mFromId = json.value(STATE_CONSTRAINT_BWD_ID).toInt();
    ec.mToId = json.value(STATE_CONSTRAINT_FWD_ID).toInt();
    return ec;
}

QJsonObject EventConstraint::toJson() const
{
    return Constraint::toJson();
}
