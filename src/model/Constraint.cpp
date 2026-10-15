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

#include "Constraint.h"

#include "StateKeys.h"

Constraint::Constraint():
    mId(-1),
    mFromId(0),
    mToId(0)
{

}

Constraint::Constraint(const Constraint& ec)
{
    mId = ec.mId;
    mFromId = ec.mFromId;
    mToId = ec.mToId;
}

Constraint::Constraint(const QJsonObject& json):
    mId(json.value(STATE_ID).toInt()),
    mFromId(json.value(STATE_CONSTRAINT_BWD_ID).toInt()),
    mToId(json.value(STATE_CONSTRAINT_FWD_ID).toInt())
{

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
    c.mId = json.value(STATE_ID).toInt();
    c.mFromId = json.value(STATE_CONSTRAINT_BWD_ID).toInt();
    c.mToId = json.value(STATE_CONSTRAINT_FWD_ID).toInt();
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
