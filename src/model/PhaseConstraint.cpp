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

#include "PhaseConstraint.h"
#include "Phase.h"
#include "Generator.h"

PhaseConstraint::PhaseConstraint():Constraint(),
mGamma(0.f),
mPhaseFrom(nullptr),
mPhaseTo(nullptr),
mGammaType(PhaseConstraint::eGammaUnknown),
mGammaFixed(1),
mGammaMin(0),
mGammaMax(1)
{

}

PhaseConstraint::PhaseConstraint(const PhaseConstraint& pc):Constraint()
{
    copyFrom(pc);
}

PhaseConstraint& PhaseConstraint::operator=(const PhaseConstraint& pc)
{
    copyFrom(pc);
    return *this;
}

void PhaseConstraint::copyFrom(const Constraint& c)
{
    Constraint::copyFrom(c);

    const PhaseConstraint& pc = (PhaseConstraint&) c;

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
    mPhaseFrom = nullptr;
    mPhaseTo = nullptr;
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
        double max = qMin(mGammaMax, mPhaseTo->mAlpha.mX - mPhaseFrom->mBeta.mX);
        mGamma = Generator::randomUniform(mGammaMin, max);
    }
}

QPair<double,double> PhaseConstraint::getFormatedGapRange() const
{
    const double t1 = DateUtils::convertToAppSettingsFormat(mGapRange.first);
    const double t2 = DateUtils::convertToAppSettingsFormat(mGapRange.second);

    if(t1<t2)
        return QPair<double,double>(t1,t2);
    else
        return QPair<double,double>(t2,t1);

}

QPair<double,double> PhaseConstraint::getFormatedTransitionRange() const
{
    const double t1 = DateUtils::convertToAppSettingsFormat(mTransitionRange.first);
    const double t2 = DateUtils::convertToAppSettingsFormat(mTransitionRange.second);

    if (t1<t2)
        return QPair<double,double>(t1,t2);
    else
        return QPair<double,double>(t2,t1);

}
