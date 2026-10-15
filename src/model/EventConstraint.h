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

#ifndef EVENTCONSTRAINT_H
#define EVENTCONSTRAINT_H

#include "Constraint.h"
#include "Event.h"

class EventConstraint: public Constraint
{
public:
    std::shared_ptr<Event> mEventFrom;
    std::shared_ptr<Event> mEventTo;

    EventConstraint();
    EventConstraint(const EventConstraint& ec);
    explicit EventConstraint(const QJsonObject& json);

    EventConstraint& operator=(const EventConstraint& ec);

    void copyFrom(const Constraint& ec);
    void copyFrom(const EventConstraint& ec);
    virtual ~EventConstraint();

    static EventConstraint fromJson(const QJsonObject& json);
    QJsonObject toJson() const;

};

#endif
