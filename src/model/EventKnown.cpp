/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "EventKnown.h"
#include "StdUtilities.h"
#include "GraphView.h"
#include "Generator.h"

#include <QObject>

EventKnown::EventKnown():Event(),
mFixed(0.)
{
    mType = eKnown;
    mMethod= eFixe;
    mTheta.mSigmaMH = 0.;
}

EventKnown::EventKnown(const QJsonObject& json):Event()
{
    mType = (Type)json[STATE_EVENT_TYPE].toInt();
    mId = json[STATE_ID].toInt();
    mName =  json[STATE_NAME].toString();
    mColor = QColor(json[STATE_COLOR_RED].toInt(),
                    json[STATE_COLOR_GREEN].toInt(),
                    json[STATE_COLOR_BLUE].toInt());
    mMethod = Event::eFixe;
    mItemX = json[STATE_ITEM_X].toDouble();
    mItemY = json[STATE_ITEM_Y].toDouble();
    mIsSelected = json[STATE_IS_SELECTED].toBool();
    mIsCurrent = json[STATE_IS_CURRENT].toBool();

    if (json.contains(STATE_EVENT_KNOWN_FIXED))
        mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();
    else
        mFixed = 0.;

    QString eventIdsStr = json[STATE_EVENT_PHASE_IDS].toString();
    if (!eventIdsStr.isEmpty()) {
        QStringList eventIds = eventIdsStr.split(",");
        for(auto &&evIds :eventIds)
            mPhasesIds.append(evIds.toInt());
    }
}


// JSON
// static function
EventKnown EventKnown::fromJson(const QJsonObject& json)
{
    EventKnown event;

    event.mType = (Type)json[STATE_EVENT_TYPE].toInt();
    event.mId = json[STATE_ID].toInt();
    event.mName =  json[STATE_NAME].toString();
    event.mColor = QColor(json[STATE_COLOR_RED].toInt(),
                           json[STATE_COLOR_GREEN].toInt(),
                           json[STATE_COLOR_BLUE].toInt());
    event.mMethod = Event::eFixe;
    event.mItemX = json[STATE_ITEM_X].toDouble();
    event.mItemY = json[STATE_ITEM_Y].toDouble();
    event.mIsSelected = json[STATE_IS_SELECTED].toBool();
    event.mIsCurrent = json[STATE_IS_CURRENT].toBool();

    if (json.contains(STATE_EVENT_KNOWN_FIXED))
        event.mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();
    else
        event.mFixed = 0.;

    QString eventIdsStr = json[STATE_EVENT_PHASE_IDS].toString();
    if (!eventIdsStr.isEmpty()) {
        QStringList eventIds = eventIdsStr.split(",");
        for(int i=0; i<eventIds.size(); ++i)
            event.mPhasesIds.append(eventIds[i].toInt());
    }

    return event;
}

QJsonObject EventKnown::toJson() const
{
    QJsonObject event;

    event[STATE_EVENT_TYPE] = mType;
    event[STATE_ID] = mId;

    event[STATE_NAME] = mName;

    event[STATE_COLOR_RED] = mColor.red();
    event[STATE_COLOR_GREEN] = mColor.green();
    event[STATE_COLOR_BLUE] = mColor.blue();

    event[STATE_EVENT_METHOD] = Event::eFixe;
    event[STATE_ITEM_X] = mItemX;
    event[STATE_ITEM_Y] = mItemY;
    event[STATE_IS_SELECTED] = mIsSelected;
    event[STATE_IS_CURRENT] = mIsCurrent;

    event[STATE_EVENT_KNOWN_FIXED] = mFixed;

    return event;
}


void EventKnown::setFixedValue(const double& value) {mFixed = value;}

double EventKnown::fixedValue() const
{
    return mFixed;
}

double EventKnown::formatedFixedValue() const
{
    return DateUtils::convertToAppSettingsFormat(mFixed);
}


void EventKnown::updateValues(const double& tmin, const double& tmax, const double& step)
{
    mValues.clear();

    for (double t=tmin; t<=tmax; t+=step)
        mValues[t] = 0.;
    mValues[mFixed] = 1.;

    if (mValues.size() == 0) {
        for (double t=tmin; t<=tmax; t+=step)
            mValues[t] = 0.;
    }
}

void EventKnown::updateTheta(const double& tmin, const double& tmax)
{
    (void) tmin;
    (void) tmax;
    mTheta.tryUpdate(mFixed, 1.);
}
