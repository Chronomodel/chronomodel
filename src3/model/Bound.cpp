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

#include "Bound.h"
//#include "StdUtilities.h"
#include "QtUtilities.h"
//#include "GraphView.h"
//#include "Generator.h"

#include <QObject>

Bound::Bound():Event(),
mFixed(0.)
{
    Event::mType = eBound;
    Event::mTheta.mSamplerProposal= MHVariable::eFixe;
    Event::mTheta.mSigmaMH = 0.;
}

// JSON
Bound Bound::fromJson(const QJsonObject& json)
{
    Bound event;

    event.Event::mType = Type (json[STATE_EVENT_TYPE].toInt());
    event.Event::mId = json[STATE_ID].toInt();
    event.Event::mName =  json[STATE_NAME].toString();
    event.Event::mColor = QColor(json[STATE_COLOR_RED].toInt(),
                           json[STATE_COLOR_GREEN].toInt(),
                           json[STATE_COLOR_BLUE].toInt());
    //event.Event::mMethod = Event::eFixe;
    event.mTheta.mSamplerProposal= MHVariable::eFixe;
    event.Event::mItemX = json[STATE_ITEM_X].toDouble();
    event.Event::mItemY = json[STATE_ITEM_Y].toDouble();
    event.Event::mIsSelected = json[STATE_IS_SELECTED].toBool();
    event.Event::mIsCurrent = json[STATE_IS_CURRENT].toBool();

    if (json.contains(STATE_EVENT_KNOWN_FIXED))
        event.mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();

    else
        event.mFixed = 0.;

    event.Event::mPhasesIds = stringListToIntList(json.value(STATE_EVENT_PHASE_IDS).toString());

    event.Event::mXIncDepth = json.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    event.Event::mYDec = json.value(STATE_EVENT_Y_DEC).toDouble();
    event.Event::mZField = json.value(STATE_EVENT_Z_F).toDouble();

    event.Event::mS_XA95Depth = json.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    event.Event::mS_Y = json.value(STATE_EVENT_SY).toDouble();
    event.Event::mS_ZField = json.value(STATE_EVENT_SZ_SF).toDouble();

    event.mVG.mSamplerProposal= MHVariable::eMHAdaptGauss;
    return event;
}

QJsonObject Bound::toJson() const
{
    QJsonObject event;

    event[STATE_EVENT_TYPE] = Event::mType;
    event[STATE_ID] = Event::mId;

    event[STATE_NAME] = Event::mName;

    event[STATE_COLOR_RED] = Event::mColor.red();
    event[STATE_COLOR_GREEN] = Event::mColor.green();
    event[STATE_COLOR_BLUE] = Event::mColor.blue();

    event[STATE_EVENT_SAMPLER] = MHVariable::eFixe;
    event[STATE_ITEM_X] = Event::mItemX;
    event[STATE_ITEM_Y] = Event::mItemY;
    event[STATE_IS_SELECTED] = Event::mIsSelected;
    event[STATE_IS_CURRENT] = Event::mIsCurrent;

    event[STATE_EVENT_KNOWN_FIXED] = mFixed;

    event[STATE_EVENT_X_INC_DEPTH] = Event::mXIncDepth;
    event[STATE_EVENT_Y_DEC] = Event::mYDec;
    event[STATE_EVENT_Z_F] = Event::mZField;

    event[STATE_EVENT_SX_ALPHA95_SDEPTH] = Event::mS_XA95Depth;
    event[STATE_EVENT_SY] = Event::mS_Y;
    event[STATE_EVENT_SZ_SF] = Event::mS_ZField;
    return event;
}

/*
 * EventKnown& EventKnown::operator=(const EventKnown& event)
{
    copyFrom(event);
    return *this;
}

void EventKnown::copyFrom(const EventKnown& event)
{
    Event::copyFrom(event);
    mFixed = event.mFixed;
    mValues = event.mValues;
}
*/
void Bound::setFixedValue(const double& value) {mFixed = value;}

double Bound::fixedValue() const
{
    return mFixed;
}

double Bound::formatedFixedValue() const
{
    return DateUtils::convertToAppSettingsFormat(mFixed);
}


void Bound::updateValues(const double& tmin, const double& tmax, const double& step)
{
    mValues.clear();

    double t = tmin;
    while (t <= tmax) {
        mValues[t] = 0.;
        t +=step;
    }

    mValues[mFixed] = 1.;

    if (mValues.size() == 0) { // ???
        for (double t=tmin; t<=tmax; t+=step)
            mValues[t] = 0.;
    }
}

void Bound::updateTheta(const double& tmin, const double& tmax)
{
    (void) tmin;
    (void) tmax;
    mTheta.tryUpdate(mFixed, 1.);
}