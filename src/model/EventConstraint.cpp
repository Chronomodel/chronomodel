/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
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
    mEventFrom = nullptr;
    mEventTo = nullptr;
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
