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

#include "Bound.h"

#include "QtUtilities.h"
#include "StateKeys.h"

#include <QObject>

//Bound::Bound(const Model *model):
Bound::Bound(std::shared_ptr<Model> model):
    Event (model),
    mFixed (0.)
{
    mType = eBound;
    mPointType = ePoint;
    mTheta.mSamplerProposal = MHVariable::eFixe;
    mTheta.mSigmaMH = 1.;
    mS02Theta.mSamplerProposal = MHVariable::eFixe;
}

//Bound::Bound(const QJsonObject &json, const Model* model):
Bound::Bound(const QJsonObject &json, std::shared_ptr<Model> model):
    Event (model)
{
    mType = Type (json[STATE_EVENT_TYPE].toInt());
    mId = json[STATE_ID].toInt();
    mName =  json[STATE_NAME].toString();
    mColor = QColor(json[STATE_COLOR_RED].toInt(),
                           json[STATE_COLOR_GREEN].toInt(),
                           json[STATE_COLOR_BLUE].toInt());
    //event.Event::mMethod = Event::eFixe;
    mTheta.mSamplerProposal= MHVariable::eFixe;
    mItemX = json[STATE_ITEM_X].toDouble();
    mItemY = json[STATE_ITEM_Y].toDouble();
    mIsSelected = json[STATE_IS_SELECTED].toBool();
    mIsCurrent = json[STATE_IS_CURRENT].toBool();

    if (json.contains(STATE_EVENT_KNOWN_FIXED))
        mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();

    else
        mFixed = 0.;

    mPhasesIds = stringListToIntList(json.value(STATE_EVENT_PHASE_IDS).toString());

    mPointType = PointType (json.value(STATE_EVENT_POINT_TYPE).toInt());

    mXIncDepth = json.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    mYDec = json.value(STATE_EVENT_Y_DEC).toDouble();
    mZField = json.value(STATE_EVENT_Z_F).toDouble();

    mS_XA95Depth = json.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    mS_Y = json.value(STATE_EVENT_SY).toDouble();
    mS_ZField = json.value(STATE_EVENT_SZ_SF).toDouble();

    mVg.mSupport = MetropolisVariable::eRp;
    mVg.mFormat = DateUtils::eNumeric;
    mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

    mS02Theta.mSamplerProposal = MHVariable::eFixe;
}


// JSON
Bound Bound::fromJson(const QJsonObject &json)
{
    Bound bound;

    bound.mType = Type (json[STATE_EVENT_TYPE].toInt());
    bound.mId = json[STATE_ID].toInt();
    bound.mName =  json[STATE_NAME].toString();
    bound.mColor = QColor(json[STATE_COLOR_RED].toInt(),
                           json[STATE_COLOR_GREEN].toInt(),
                           json[STATE_COLOR_BLUE].toInt());

    bound.mTheta.mSamplerProposal= MHVariable::eFixe;
    bound.mItemX = json[STATE_ITEM_X].toDouble();
    bound.mItemY = json[STATE_ITEM_Y].toDouble();
    bound.mIsSelected = json[STATE_IS_SELECTED].toBool();
    bound.mIsCurrent = json[STATE_IS_CURRENT].toBool();

    if (json.contains(STATE_EVENT_KNOWN_FIXED))
        bound.mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();

    else
        bound.mFixed = 0.;

    bound.mPhasesIds = stringListToIntList(json.value(STATE_EVENT_PHASE_IDS).toString());

    bound.mPointType = PointType (json.value(STATE_EVENT_POINT_TYPE).toInt());
    bound.mXIncDepth = json.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    bound.mYDec = json.value(STATE_EVENT_Y_DEC).toDouble();
    bound.mZField = json.value(STATE_EVENT_Z_F).toDouble();

    bound.mS_XA95Depth = json.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    bound.mS_Y = json.value(STATE_EVENT_SY).toDouble();
    bound.mS_ZField = json.value(STATE_EVENT_SZ_SF).toDouble();

    bound.mVg.mSupport = MetropolisVariable::eRp;
    bound.mVg.mFormat = DateUtils::eNumeric;
    bound.mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

    bound.mS02Theta.mSamplerProposal = MHVariable::eFixe;
    return bound;
}

QJsonObject Bound::toJson() const
{
    QJsonObject json;

    json[STATE_EVENT_TYPE] = mType;
    json[STATE_ID] = mId;

    json[STATE_NAME] = mName;

    json[STATE_COLOR_RED] = mColor.red();
    json[STATE_COLOR_GREEN] = mColor.green();
    json[STATE_COLOR_BLUE] = mColor.blue();

    json[STATE_EVENT_SAMPLER] = MHVariable::eFixe;
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

void Bound::setFixedValue(const double& value) {mFixed = value;}

double Bound::fixedValue() const
{
    return mFixed;
}

double Bound::formatedFixedValue() const
{
    return DateUtils::convertToAppSettingsFormat(mFixed);
}


void Bound::updateValues(const double tmin, const double tmax, const double step)
{
    mValues.clear();

    double t = tmin;
    while (t <= tmax) {
        mValues[t] = 0.;
        t += step;
    }

    mValues[mFixed] = 1.;

    if (mValues.size() == 0) { // ???
        t=tmin;
        while ( t<=tmax) {
            mValues[t] = 0.;
            t += step;
        }
    }
}

void Bound::updateTheta(const double , const double )
{
    mTheta.tryUpdate(mFixed, 2.);
}
