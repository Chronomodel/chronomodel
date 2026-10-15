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

#include "PhaseConstraint.h"
#include "Phase.h"
#include "Generator.h"
#include "StateKeys.h"

PhaseConstraint::PhaseConstraint():
    Constraint(),
    mGamma(0.),
    mPhaseFrom(nullptr),
    mPhaseTo(nullptr),
    mGammaType(PhaseConstraint::eGammaUnknown),
    mGammaFixed(1),
    mGammaMin(0),
    mGammaMax(1)
{

}

PhaseConstraint::PhaseConstraint(const PhaseConstraint& pc):
    Constraint()
{
    mId = pc.mId;
    mFromId = pc.mFromId;
    mToId = pc.mToId;
    mGamma = pc.mGamma;

    mPhaseFrom = pc.mPhaseFrom;
    mPhaseTo = pc.mPhaseTo;

    mGammaType = pc.mGammaType;
    mGammaFixed = pc.mGammaFixed;
    mGammaMin = pc.mGammaMin;
    mGammaMax = pc.mGammaMax;
}

PhaseConstraint::PhaseConstraint (const QJsonObject& json):
    Constraint(json),
    mGammaType(PhaseConstraint::GammaType (json.value(STATE_CONSTRAINT_GAMMA_TYPE).toInt())),
    mGammaFixed(json.value(STATE_CONSTRAINT_GAMMA_FIXED).toDouble()),
    mGammaMin(json.value(STATE_CONSTRAINT_GAMMA_MIN).toDouble()),
    mGammaMax(json.value(STATE_CONSTRAINT_GAMMA_MAX).toDouble())
{
    mId = json.value(STATE_ID).toInt();
    mFromId = json.value(STATE_CONSTRAINT_BWD_ID).toInt();
    mToId = json.value(STATE_CONSTRAINT_FWD_ID).toInt();

}

PhaseConstraint& PhaseConstraint::operator=(const PhaseConstraint& pc)
{
    copyFrom(pc);
    return *this;
}

void PhaseConstraint::copyFrom(const Constraint& c)
{
    mId = c.mId;
    mFromId = c.mFromId;
    mToId = c.mToId;
}

void PhaseConstraint::copyFrom(const PhaseConstraint& pc)
{
    mId = pc.mId;
    mFromId = pc.mFromId;
    mToId = pc.mToId;
    mGamma = pc.mGamma;

    mPhaseFrom = pc.mPhaseFrom;
    mPhaseTo = pc.mPhaseTo;

    mGammaType = pc.mGammaType;
    mGammaFixed = pc.mGammaFixed;
    mGammaMin = pc.mGammaMin;
    mGammaMax = pc.mGammaMax;
}

PhaseConstraint::~PhaseConstraint()
{
   /* mPhaseFrom = nullptr;
    mPhaseTo = nullptr;*/
}

PhaseConstraint PhaseConstraint::fromJson(const QJsonObject& json)
{
    PhaseConstraint c;
    c.mId = json.value(STATE_ID).toInt();
    c.mFromId = json.value(STATE_CONSTRAINT_BWD_ID).toInt();
    c.mToId = json.value(STATE_CONSTRAINT_FWD_ID).toInt();
    c.mGammaType = PhaseConstraint::GammaType (json.value(STATE_CONSTRAINT_GAMMA_TYPE).toInt());
    c.mGammaFixed = json.value(STATE_CONSTRAINT_GAMMA_FIXED).toDouble();
    c.mGammaMin = json.value(STATE_CONSTRAINT_GAMMA_MIN).toDouble();
    c.mGammaMax = json.value(STATE_CONSTRAINT_GAMMA_MAX).toDouble();
    return c;
}

QJsonObject PhaseConstraint::toJson() const
{
    QJsonObject json = Constraint::toJson();
    json[STATE_ID] = mId;
    json[STATE_CONSTRAINT_BWD_ID] = mFromId;
    json[STATE_CONSTRAINT_FWD_ID] = mToId;
    json[STATE_CONSTRAINT_GAMMA_TYPE] = mGammaType;
    json[STATE_CONSTRAINT_GAMMA_FIXED] = mGammaFixed;
    json[STATE_CONSTRAINT_GAMMA_MIN] = mGammaMin;
    json[STATE_CONSTRAINT_GAMMA_MAX] = mGammaMax;
    return json;
}

void PhaseConstraint::initGamma()
{
    if(mGammaType == eGammaUnknown)
        mGamma = 0;

    else if (mGammaType == eGammaFixed && mGammaFixed != 0.)
        mGamma = mGammaFixed;

    else if (mGammaType == eGammaRange && mGammaMax > mGammaMin)
        mGamma = mGammaMin;
}

void PhaseConstraint::updateGamma()
{
    if (mGammaType == eGammaUnknown)
        mGamma = 0.;

    else if (mGammaType == eGammaFixed && mGammaFixed != 0.)
        mGamma = mGammaFixed;

    else if (mGammaType == eGammaRange && mGammaMax > mGammaMin) {
        double max = qMin(mGammaMax, mPhaseTo->mAlpha.value() - mPhaseFrom->mBeta.value());
        mGamma = Generator::randomUniform(mGammaMin, max);
    }
}

std::pair<double,double> PhaseConstraint::getFormatedGapRange() const
{
    const double t1 = DateUtils::convertToAppSettingsFormat(mGapRange.first);
    const double t2 = DateUtils::convertToAppSettingsFormat(mGapRange.second);

    if(t1<t2)
        return std::pair<double,double>(t1,t2);
    else
        return std::pair<double,double>(t2,t1);

}

std::pair<double,double> PhaseConstraint::getFormatedTransitionRange() const
{
    const double t1 = DateUtils::convertToAppSettingsFormat(mTransitionRange.first);
    const double t2 = DateUtils::convertToAppSettingsFormat(mTransitionRange.second);

    if (t1<t2)
        return std::pair<double,double>(t1,t2);
    else
        return std::pair<double,double>(t2,t1);

}
