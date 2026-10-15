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

#include "Bound.h"

#include "QtUtilities.h"
#include "StateKeys.h"

#include <QObject>


Bound::Bound():
    Event(),
    mFixed (0.)
{
    setName(std::string("no Bound Name"));
    mType = eBound;
    mPointType = ePoint;
    mTheta.mSamplerProposal = SamplerProposal::eFixe;
    mTheta.mSigmaMH = 1.;
    mS02Theta.mSamplerProposal = SamplerProposal::eFixe;
}

Bound::Bound(const Bound& origin):
    Event(origin),
    mFixed (origin.mFixed)
{
}


Bound::Bound(const QJsonObject& json):
    Event(json)
{
    mType = Type (json[STATE_EVENT_TYPE].toInt());
    mId = json[STATE_ID].toInt();
    setName(json[STATE_NAME].toString());
    mColor = QColor(json[STATE_COLOR_RED].toInt(),
                           json[STATE_COLOR_GREEN].toInt(),
                           json[STATE_COLOR_BLUE].toInt());
   
    mTheta.mSamplerProposal= SamplerProposal::eFixe;
    mItemX = json[STATE_ITEM_X].toDouble();
    mItemY = json[STATE_ITEM_Y].toDouble();
    mIsSelected = json[STATE_IS_SELECTED].toBool();
    mIsCurrent = json[STATE_IS_CURRENT].toBool();

    if (json.contains(STATE_EVENT_KNOWN_FIXED))
        mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();

    else
        mFixed = 0.;

    mPhasesIds = QStringToStdVectorInt(json.value(STATE_EVENT_PHASE_IDS).toString());

    mPointType = PointType (json.value(STATE_EVENT_POINT_TYPE).toInt());

    mXIncDepth = json.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    mYDec = json.value(STATE_EVENT_Y_DEC).toDouble();
    mZField = json.value(STATE_EVENT_Z_F).toDouble();

    mS_XA95Depth = json.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    mS_Y = json.value(STATE_EVENT_SY).toDouble();
    mS_ZField = json.value(STATE_EVENT_SZ_SF).toDouble();

    mVg.mSupport = Support::eRp;
    mVg.mFormat = DateUtils::eNumeric;
    mVg.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

    mS02Theta.mSamplerProposal = SamplerProposal::eFixe;
}

/** Copy assignment operator */
Bound& Bound::operator=(const Bound& origin)
{
    Event::operator=(origin);
    mFixed = origin.mFixed;
    return *this;
}
/** Move assignment operator */
Bound& Bound::operator=(Bound&& origin) noexcept
{
    Event::operator=(origin);
    mFixed = origin.mFixed;
    return *this;
}


Bound::~Bound()
{
}

// JSON
const Bound Bound::fromJson(const QJsonObject &json)
{
    Bound bound;
    bound.mType = Type (json[STATE_EVENT_TYPE].toInt());
    bound.mId = json[STATE_ID].toInt();
    bound.setName(json[STATE_NAME].toString());
    bound.mColor = QColor(json[STATE_COLOR_RED].toInt(),
                    json[STATE_COLOR_GREEN].toInt(),
                    json[STATE_COLOR_BLUE].toInt());

    bound.mTheta.mSamplerProposal = SamplerProposal::eFixe;
    bound.mItemX = json[STATE_ITEM_X].toDouble();
    bound.mItemY = json[STATE_ITEM_Y].toDouble();
    bound.mIsSelected = json[STATE_IS_SELECTED].toBool();
    bound.mIsCurrent = json[STATE_IS_CURRENT].toBool();

    if (json.contains(STATE_EVENT_KNOWN_FIXED))
       bound. mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();

    else
        bound.mFixed = 0.;

    bound.mPhasesIds = QStringToStdVectorInt(json.value(STATE_EVENT_PHASE_IDS).toString());

    bound.mPointType = PointType (json.value(STATE_EVENT_POINT_TYPE).toInt());

    bound.mXIncDepth = json.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    bound.mYDec = json.value(STATE_EVENT_Y_DEC).toDouble();
    bound.mZField = json.value(STATE_EVENT_Z_F).toDouble();

    bound.mS_XA95Depth = json.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    bound.mS_Y = json.value(STATE_EVENT_SY).toDouble();
    bound.mS_ZField = json.value(STATE_EVENT_SZ_SF).toDouble();

    bound.mVg.mSupport = Support::eRp;
    bound.mVg.mFormat = DateUtils::eNumeric;
    bound.mVg.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

    bound.mS02Theta.mSamplerProposal = SamplerProposal::eFixe;
    return bound;
}

QJsonObject Bound::toJson() const
{
    QJsonObject json;

    json[STATE_EVENT_TYPE] = mType;
    json[STATE_ID] = mId;

    json[STATE_NAME] = getQStringName();

    json[STATE_COLOR_RED] = mColor.red();
    json[STATE_COLOR_GREEN] = mColor.green();
    json[STATE_COLOR_BLUE] = mColor.blue();

    json[STATE_EVENT_SAMPLER] = static_cast<int>(SamplerProposal::eFixe);
    json[STATE_ITEM_X] = mItemX;
    json[STATE_ITEM_Y] = mItemY;
    json[STATE_IS_SELECTED] = mIsSelected;
    json[STATE_IS_CURRENT] = mIsCurrent;

    json[STATE_EVENT_KNOWN_FIXED] = mFixed;

    json[STATE_EVENT_POINT_TYPE] = mPointType;
    json[STATE_EVENT_X_INC_DEPTH] = mXIncDepth;
    json[STATE_EVENT_Y_DEC] = mYDec;
    json[STATE_EVENT_Z_F] = mZField;

    json[STATE_EVENT_SX_ALPHA95_SDEPTH] = mS_XA95Depth;
    json[STATE_EVENT_SY] = mS_Y;
    json[STATE_EVENT_SZ_SF] = mS_ZField;
    return json;
}





double Bound::formatedFixedValue() const
{
    return DateUtils::convertToAppSettingsFormat(mFixed);
}



