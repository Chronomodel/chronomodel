/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2026

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

#include "Event.h"

#include "CalibrationCurve.h"
#include "Date.h"
#include "Phase.h"

#include "PhaseConstraint.h"
#include "Generator.h"
#include "Bound.h"
#include "QtUtilities.h"
#include "StateKeys.h"
#include "StdUtilities.h"

#include <QString>
#include <QJsonArray>
#include <QObject>
#include <QDebug>
#include <QJsonObject>


Event::Event():
    mType (eDefault),
    mId (0),
    mItemX(0.),
    mItemY(0.),
    mIsCurrent (false),
    mIsSelected (false),
    mPhasesIds(),
    mConstraintsFwdIds(),
    mConstraintsBwdIds(),
    mPhases(),

    mConstraintsFwd(),
    mConstraintsBwd(),
    mTheta (),
    mS02Theta (),
    mAShrinkage(0.),
    mBetaS02 (0.),
    mInitialized (false),
    mIsNode (false),
    mLevel (0),
    mPointType (ePoint),
    mXIncDepth(0.),
    mYDec(0.),
    mZField(0.),
    mS_XA95Depth(0.),
    mS_Y(0.),
    mS_ZField(0.),
    mYx(0.),
    mYy(0.),
    mYz(0.),
    mGx(0.),
    mGy(0.),
    mGz(0.),
    mThetaReduced(0.),
    mSy(0.),
    mW(0.),
    mVg (),
    mMixingCalibrations (nullptr),
    mName ("no Event Name")
{
    mTheta.setName(std::string("Theta of Event : ") + mName);
    mTheta.mSupport = MetropolisVariable::eBounded;
    mTheta.mFormat = DateUtils::eUnknown;
    mTheta.mSamplerProposal = MHVariable::eDoubleExp;

    mS02Theta.setName(std::string("S02Theta of Event : ") + mName);
    mS02Theta.mSupport = MetropolisVariable::eRpStar;
    mS02Theta.mFormat = DateUtils::eNumeric;

#ifdef S02_BAYESIAN
    mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;
#else
    mS02Theta.mSamplerProposal = MHVariable::eFixe;
#endif

    // Item initial position :


    // Note : setting an event in (0, 0) tells the scene that this item is new!
    // Thus the scene will move it randomly around the currently viewed center point.
    // --------------------------------------------------------
    //  Curve
    // --------------------------------------------------------

    // Valeurs utilisée pour les calculs


   // MHVariable mVg;
    mVg.setName("Vg of Event : " + mName);
    mVg.mSupport = MetropolisVariable::eRpStar;
    mVg.mFormat = DateUtils::eNumeric;
    mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

}

Event::Event (const QJsonObject& json):
    mPhasesIds(),
    mConstraintsFwdIds(),
    mConstraintsBwdIds(),
    mPhases(),
    mConstraintsFwd(),
    mConstraintsBwd(),
    mAShrinkage(0.),
    mBetaS02 (0.),
    mIsNode(false),
    mYx(0.),
    mYy(0.),
    mYz(0.),
    mGx(0.),
    mGy(0.),
    mGz(0.),
    mW(0.),
    mMixingCalibrations(nullptr)
{
    mType = Type (json.value(STATE_EVENT_TYPE).toInt());
    mId = json.value(STATE_ID).toInt();
    setName(json.value(STATE_NAME).toString());

    mColor = QColor(json.value(STATE_COLOR_RED).toInt(),
                          json.value(STATE_COLOR_GREEN).toInt(),
                          json.value(STATE_COLOR_BLUE).toInt());

    mItemX = json.value(STATE_ITEM_X).toDouble();
    mItemY = json.value(STATE_ITEM_Y).toDouble();
    mIsSelected = json.value(STATE_IS_SELECTED).toBool();
    mIsCurrent = json.value(STATE_IS_CURRENT).toBool();

    mTheta.setName(std::string("Theta of Event : ") + mName);
    mTheta.mSamplerProposal = MHVariable::SamplerProposal (json.value(STATE_EVENT_SAMPLER).toInt());
    mTheta.mSupport = MetropolisVariable::eBounded;
    mTheta.mFormat = DateUtils::eUnknown;
    mTheta.mSigmaMH = 1.;

    mVg.setName(std::string("VG of Event : ") + mName);
    mVg.mSupport = MetropolisVariable::eRpStar;
    mVg.mFormat = DateUtils::eNumeric;
    mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

    mS02Theta.setName(std::string("SO2Theta of Event : ") + mName);
    mS02Theta.mSupport = MetropolisVariable::eRpStar;
    mS02Theta.mFormat = DateUtils::eNumeric;
#ifdef S02_BAYESIAN
    mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;
#else
    mS02Theta.mSamplerProposal = MHVariable::eFixe;
#endif

    mPhasesIds = QStringToStdVectorInt(json.value(STATE_EVENT_PHASE_IDS).toString());

    mPointType = PointType (json.value(STATE_EVENT_POINT_TYPE).toInt());
    mXIncDepth = json.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    mYDec = json.value(STATE_EVENT_Y_DEC).toDouble();
    mZField = json.value(STATE_EVENT_Z_F).toDouble();

    mS_XA95Depth = json.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    mS_Y = json.value(STATE_EVENT_SY).toDouble();
    mS_ZField = json.value(STATE_EVENT_SZ_SF).toDouble();

    const QJsonArray &dates = json.value(STATE_EVENT_DATES).toArray();
    for (auto&& date : dates) {
        try {
            mDates.emplace_back(date.toObject());

        } catch (...) {

            throw QObject::tr("ERROR : data %1 could not be created with plugin %2").arg(getQStringName(), date.toObject().value(STATE_DATE_PLUGIN_ID).toString());
        }


    }

}
/** Copy constructor */
Event::Event(const Event &origin):
    std::enable_shared_from_this<Event>()
{
    mType = origin.mType;
    mId = origin.mId;

    mName = origin.mName;

    mColor = origin.mColor;

    mItemX = origin.mItemX;
    mItemY = origin.mItemY;

    mIsCurrent = origin.mIsCurrent;
    mIsSelected = origin.mIsSelected;

    mDates = origin.mDates;

    mPhasesIds = origin.mPhasesIds;
    mConstraintsFwdIds = origin.mConstraintsFwdIds;
    mConstraintsBwdIds = origin.mConstraintsBwdIds;

    mPhases = origin.mPhases;
    mConstraintsFwd = origin.mConstraintsFwd;
    mConstraintsBwd = origin.mConstraintsBwd;

    mTheta = origin.mTheta;

    mS02Theta.mX = origin.mS02Theta.mX;

    mAShrinkage = origin.mAShrinkage;
    mBetaS02 = origin.mBetaS02;
    mInitialized = origin.mInitialized;

    mIsNode = origin.mIsNode;
    mThetaNode = origin.mThetaNode;

    mLevel = origin.mLevel;
    // Valeurs entrées par l'utilisateur
    mPointType = origin.mPointType;
    mXIncDepth = origin.mXIncDepth;
    mYDec = origin.mYDec;
    mZField = origin.mZField;

    mS_XA95Depth = origin.mS_XA95Depth;
    mS_Y = origin.mS_Y;
    mS_ZField = origin.mS_ZField;

    // Valeurs préparées (projetées)
    mYx = origin.mYx;
    mYy = origin.mYy;
    mYz = origin.mYz;

    // Valeurs utilisée pour les calculs
    mThetaReduced = origin.mThetaReduced;
    mSy = origin.mSy;
    mW = origin.mW;

    mVg = origin.mVg;

    mMixingCalibrations = origin.mMixingCalibrations;

}

/** Copy assignment operator */
Event& Event::operator=(const Event& origin)
{
    mType = origin.mType;
    mId = origin.mId;
    mItemX = origin.mItemX;
    mItemY = origin.mItemY;
    mIsCurrent = origin.mIsCurrent;
    mIsSelected = origin.mIsSelected;
    mPhasesIds = origin.mPhasesIds;
    mConstraintsFwdIds = origin.mConstraintsFwdIds;
    mConstraintsBwdIds = origin.mConstraintsBwdIds;
    mPhases = origin.mPhases;
    mConstraintsFwd = origin.mConstraintsFwd;
    mConstraintsBwd = origin.mConstraintsBwd;

    mTheta = origin.mTheta;
    mS02Theta = origin.mS02Theta;
    mAShrinkage = origin.mAShrinkage;
    mBetaS02 = origin.mBetaS02;
    mInitialized = origin.mInitialized;
    mIsNode = origin.mIsNode;
    mLevel = origin.mLevel;
    mPointType = origin.mPointType;
    mXIncDepth = origin.mXIncDepth;
    mYDec = origin.mYDec;
    mZField = origin.mZField;

    mS_XA95Depth = origin.mS_XA95Depth;
    mS_Y = origin.mS_Y;
    mS_ZField = origin.mS_ZField;
    mYx = origin.mYx;
    mYy = origin.mYy;
    mYz = origin.mYz;
    mGx = origin.mGx;
    mGy = origin.mGy;
    mGz = origin.mGz;
    mThetaReduced = origin.mThetaReduced;
    mSy = origin.mSy;

    mW = origin.mW;
    mVg = origin.mVg;
    mMixingCalibrations = origin.mMixingCalibrations;
    mName = origin.mName;

    mColor = origin.mColor;

    mDates = origin.mDates;

    mThetaNode = origin.mThetaNode;

    return *this;
}



void Event::copyFrom(const Event& event)
{
    mType = event.mType;
    mId = event.mId;

    mName = event.mName;
    mColor = event.mColor;

    mItemX = event.mItemX;
    mItemY = event.mItemY;

    mIsCurrent = event.mIsCurrent;
    mIsSelected = event.mIsSelected;

    mDates = event.mDates;

    mPhasesIds = event.mPhasesIds;
    mConstraintsFwdIds = event.mConstraintsFwdIds;
    mConstraintsBwdIds = event.mConstraintsBwdIds;

    mPhases = event.mPhases;
    mConstraintsFwd = event.mConstraintsFwd;
    mConstraintsBwd = event.mConstraintsBwd;

    mTheta = event.mTheta;

    mS02Theta.mX = event.mS02Theta.mX;

    mAShrinkage = event.mAShrinkage;
    mBetaS02 = event.mBetaS02;
    mInitialized = event.mInitialized;

    mIsNode = event.mIsNode;
    mThetaNode = event.mThetaNode;

    mLevel = event.mLevel;
    // --------------------------------------------------------
    //  Curve
    // --------------------------------------------------------

    // Values entered by the user
    mPointType = event.mPointType;
    mXIncDepth = event.mXIncDepth;
    mYDec = event.mYDec;
    mZField = event.mZField;
    
    mS_XA95Depth = event.mS_XA95Depth;
    mS_Y = event.mS_Y;
    mS_ZField = event.mS_ZField;

    // Prepared (projected) values
    mYx = event.mYx;
    mYy = event.mYy;
    mYz = event.mYz;

    // Splines values
    mGx = event.mGx;
    mGy = event.mGy;
    mGz = event.mGz;

    // Values used for the calculations
    mThetaReduced = event.mThetaReduced;
    mSy = event.mSy;
    mW = event.mW;

    mVg = event.mVg;

    mMixingCalibrations = event.mMixingCalibrations;

}

/** Move assignment operator */
Event& Event::operator=(Event&& origin) noexcept
{
    if (this != &origin) {
        mType = origin.mType;
        mId = origin.mId;
        mItemX = origin.mItemX;
        mItemY = origin.mItemY;
        mIsCurrent = origin.mIsCurrent;
        mIsSelected = origin.mIsSelected;
        mPhasesIds = std::move(origin.mPhasesIds);
        mConstraintsFwdIds = std::move(origin.mConstraintsFwdIds);
        mConstraintsBwdIds = std::move(origin.mConstraintsBwdIds);
        mPhases = std::move(origin.mPhases);
        mConstraintsFwd = std::move(origin.mConstraintsFwd);
        mConstraintsBwd = std::move(origin.mConstraintsBwd);

        mTheta = std::move(origin.mTheta);
        mS02Theta = std::move(origin.mS02Theta);
        mAShrinkage = origin.mAShrinkage;
        mBetaS02 = origin.mBetaS02;
        mInitialized = origin.mInitialized;
        mIsNode = origin.mIsNode;
        mLevel = origin.mLevel;
        mPointType = origin.mPointType;
        mXIncDepth = origin.mXIncDepth;
        mYDec = origin.mYDec;
        mZField = origin.mZField;

        mS_XA95Depth = origin.mS_XA95Depth;
        mS_Y = origin.mS_Y;
        mS_ZField = origin.mS_ZField;
        mYx = origin.mYx;
        mYy = origin.mYy;
        mYz = origin.mYz;
        mGx = origin.mGx;
        mGy = origin.mGy;
        mGz = origin.mGz;
        mThetaReduced = origin.mThetaReduced;
        mSy = origin.mSy;

        mW = origin.mW;
        mVg = origin.mVg;
        mMixingCalibrations = origin.mMixingCalibrations;
        mName = origin.mName;

        mColor = std::move(origin.mColor);

        mDates = std::move(origin.mDates);

        mThetaNode = origin.mThetaNode;

        origin.mMixingCalibrations.reset();
        origin.mTheta.clear_and_shrink();
        origin.mS02Theta.clear_and_shrink();
        origin.mPhases.clear();
        origin.mPhasesIds.clear();
        origin.mConstraintsBwd.clear();
        origin.mConstraintsBwdIds.clear();
        origin.mConstraintsFwd.clear();
        origin.mConstraintsFwdIds.clear();
        origin.mDates.clear();
    }
    return *this;
}


Event::~Event()
{
    //qDebug() << "[Event::~Event] Event: ";//<< (mName.isNull()? " Deleted Name": mName);
    mMixingCalibrations = nullptr; //only the pointer
}


// JSON

Event const Event::fromJson(const QJsonObject& json)
{
    Event event;
    event.mType = Type (json.value(STATE_EVENT_TYPE).toInt());
    event.mId = json.value(STATE_ID).toInt();
    event.setName(json.value(STATE_NAME).toString());
    event.mColor = QColor(json.value(STATE_COLOR_RED).toInt(),
                          json.value(STATE_COLOR_GREEN).toInt(),
                          json.value(STATE_COLOR_BLUE).toInt());

    event.mItemX = json.value(STATE_ITEM_X).toDouble();
    event.mItemY = json.value(STATE_ITEM_Y).toDouble();
    event.mIsSelected = json.value(STATE_IS_SELECTED).toBool();
    event.mIsCurrent = json.value(STATE_IS_CURRENT).toBool();

    event.mTheta = MHVariable();
    event.mTheta.mSamplerProposal = MHVariable::SamplerProposal (json.value(STATE_EVENT_SAMPLER).toInt());
    event.mTheta.setName("Theta of Event : "+ event.name());

    event.mS02Theta = MHVariable();
    event.mS02Theta.setName("S02 of Event : "+ event.name());
    event.mS02Theta.mSupport = MHVariable::eRpStar;
#ifdef S02_BAYESIAN
    event.mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;
#else
    event.mS02Theta.mSamplerProposal = MHVariable::eFixe;
#endif

    event.mVg = MHVariable();
    event.mVg.setName("VG of Event : " + event.name());
    event.mVg.mSupport = MetropolisVariable::eRpStar;
    event.mVg.mFormat = DateUtils::eNumeric;
    event.mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

    event.mPhasesIds = QStringToStdVectorInt(json.value(STATE_EVENT_PHASE_IDS).toString());

    event.mPointType = PointType (json.value(STATE_EVENT_POINT_TYPE).toInt());
    event.mXIncDepth = json.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    event.mYDec = json.value(STATE_EVENT_Y_DEC).toDouble();
    event.mZField = json.value(STATE_EVENT_Z_F).toDouble();
    
    event.mS_XA95Depth = json.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    event.mS_Y = json.value(STATE_EVENT_SY).toDouble();
    event.mS_ZField = json.value(STATE_EVENT_SZ_SF).toDouble();

    const QJsonArray dates = json.value(STATE_EVENT_DATES).toArray();

    for (auto&& date : dates) {
        //Date dat (date.toObject());
       // if (!dat.isNull())
        try {
            event.mDates.emplace_back(date.toObject());

        } catch (...) {

            throw QObject::tr("ERROR : data %1 could not be created with plugin %2").arg(event.getQStringName(), date.toObject().value(STATE_DATE_PLUGIN_ID).toString());
        }

    }

    return event;
}


QJsonObject Event::toJson() const
{
    QJsonObject json;

    json[STATE_EVENT_TYPE] = mType;
    json[STATE_ID] = mId;
    json[STATE_NAME] = getQStringName();
    json[STATE_COLOR_RED] = mColor.red();
    json[STATE_COLOR_GREEN] = mColor.green();
    json[STATE_COLOR_BLUE] = mColor.blue();
    json[STATE_EVENT_SAMPLER] = mTheta.mSamplerProposal;

    json[STATE_ITEM_X] = mItemX;
    json[STATE_ITEM_Y] = mItemY;
    json[STATE_IS_SELECTED] = mIsSelected;
    json[STATE_IS_CURRENT] = mIsCurrent;

    json[STATE_EVENT_POINT_TYPE] = mPointType;
    json[STATE_EVENT_X_INC_DEPTH] = mXIncDepth;
    json[STATE_EVENT_Y_DEC] = mYDec;
    json[STATE_EVENT_Z_F] = mZField;
    json[STATE_EVENT_SX_ALPHA95_SDEPTH] = mS_XA95Depth;
    json[STATE_EVENT_SY] = mS_Y;
    json[STATE_EVENT_SZ_SF] = mS_ZField;

    QString eventIdsStr;
    if (mPhasesIds.size() > 0) {
        QList<QString> eventIds;
        for (auto &pId : mPhasesIds)
            eventIds.append(QString::number(pId));
        eventIdsStr = eventIds.join(",");
    }
    json[STATE_EVENT_PHASE_IDS] = eventIdsStr;

    QJsonArray dates;
    for (auto & d : mDates) {
        dates.append(d.toJson());
    }
    json[STATE_EVENT_DATES] = dates;

    return json;
}

void Event::setCurveCsvDataToJsonEvent(QJsonObject &event, const QMap<QString, double>& CurveData)
{
    QMap<QString, double>::const_iterator i;
    
    i = CurveData.find(STATE_EVENT_X_INC_DEPTH);
    if (i != CurveData.end()) {
        event[STATE_EVENT_X_INC_DEPTH] = i.value();
    }
    i = CurveData.find(STATE_EVENT_SX_ALPHA95_SDEPTH);
    if (i != CurveData.end()) {
        event[STATE_EVENT_SX_ALPHA95_SDEPTH] = i.value();
    }

    i = CurveData.find(STATE_EVENT_Y_DEC);
    if (i != CurveData.end()) {
        event[STATE_EVENT_Y_DEC] = i.value();
    }
    i = CurveData.find(STATE_EVENT_SY);
    if (i != CurveData.end()) {
        event[STATE_EVENT_SY] = i.value();
    }

    i = CurveData.find(STATE_EVENT_Z_F);
    if (i != CurveData.end()) {
        event[STATE_EVENT_Z_F] = i.value();
    }
    i = CurveData.find(STATE_EVENT_SZ_SF);
    if (i != CurveData.end()) {
        event[STATE_EVENT_SZ_SF] = i.value();
    }
}

QString Event::curveDescriptionFromJsonEvent(QJsonObject &event, CurveSettings::ProcessType processType)
{
    QString curveDescription = "";
    const double xIncDepth = event.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    const double s_XA95Depth = event.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    const double yDec = event.value(STATE_EVENT_Y_DEC).toDouble();
    const double s_Y = event.value(STATE_EVENT_SY).toDouble();
    const double zField = event.value(STATE_EVENT_Z_F).toDouble();
    const double s_ZField = event.value(STATE_EVENT_SZ_SF).toDouble();

    switch (processType) {
        case CurveSettings::eProcess_Univariate:
            curveDescription += QObject::tr("[  Measure : %1 ±  %2 ]").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            break;
        case CurveSettings::eProcess_Depth:
            curveDescription += QObject::tr(" [ Depth : %1 ±  %2 ]").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            break;

        case CurveSettings::eProcess_Inclination:
            curveDescription += QObject::tr(" [ Inclination : %1 ±  %2 ]").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            break;
        case CurveSettings::eProcess_Declination:
            curveDescription += QObject::tr(" [ Declination : %1 ; Inclination %2 ±  %3 ]").arg(stringForLocal(yDec), stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            break;
        case CurveSettings::eProcess_Field:
            curveDescription += line(textGreen(QObject::tr(" [ Field : %1 ±  %2 ]").arg(stringForLocal(zField), stringForLocal(s_ZField))));
            break;

        case CurveSettings::eProcess_2D:
            curveDescription += QObject::tr(" [ X : %1 ±  %2").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            curveDescription += QObject::tr(" - Y : %1 ±  %2 ]").arg(stringForLocal(yDec), stringForLocal(s_Y));
            break;
        case CurveSettings::eProcess_3D:
            curveDescription += QObject::tr(" [ X : %1 ±  %2").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            curveDescription += QObject::tr(" - Y : %1 ±  %2").arg(stringForLocal(yDec), stringForLocal(s_Y));
            curveDescription += QObject::tr(" - Z : %1 ±  %2 ]").arg(stringForLocal(zField), stringForLocal(s_ZField));
            break;

        case CurveSettings::eProcess_Unknwon_Dec:
            curveDescription += QObject::tr(" [ Inclination : %1 ±  %2").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            curveDescription += QObject::tr(" - Field : %1 ±  %2 ]").arg(stringForLocal(zField), stringForLocal(s_ZField));
            break;
        case CurveSettings::eProcess_Spherical:
            curveDescription += QObject::tr(" [ Inclination : %1 ±  %2").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            curveDescription += QObject::tr(" - Declination : %1 ]").arg(stringForLocal(yDec));
            break;

        case CurveSettings::eProcess_Vector:
            curveDescription += QObject::tr(" [ Inclination : %1 ±  %2").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            curveDescription += QObject::tr(" - Declination : %1").arg(stringForLocal(yDec));
            curveDescription += QObject::tr(" - Field : %1 ±  %2 ]").arg(stringForLocal(zField), stringForLocal(s_ZField));
            break;

        case CurveSettings::eProcess_None:
        default:
            break;

    }
    return curveDescription;
}

QList<double> Event::curveParametersFromJsonEvent(QJsonObject &event, CurveSettings::ProcessType processType)
{
    QList<double> curveParameters;
    double xIncDepth = event.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    double s_XA95Depth = event.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    double yDec = event.value(STATE_EVENT_Y_DEC).toDouble();
    double s_Y = event.value(STATE_EVENT_SY).toDouble();
    double zField = event.value(STATE_EVENT_Z_F).toDouble();
    double s_ZField = event.value(STATE_EVENT_SZ_SF).toDouble();

    switch (processType) {
    case CurveSettings::eProcess_Univariate:
        curveParameters.append({xIncDepth, s_XA95Depth});
        break;
    case CurveSettings::eProcess_Depth:
        curveParameters.append({xIncDepth, s_XA95Depth});
        break;
    case CurveSettings::eProcess_Inclination:
        curveParameters.append({xIncDepth, s_XA95Depth});
        break;
    case CurveSettings::eProcess_Declination:
        curveParameters.append({yDec, xIncDepth, s_XA95Depth});
        break;
    case CurveSettings::eProcess_Field:
        curveParameters.append({zField, s_ZField});
        break;
    case CurveSettings::eProcess_2D:
        curveParameters.append({xIncDepth, s_XA95Depth, yDec, s_Y});
        break;
    case CurveSettings::eProcess_3D:
        curveParameters.append({xIncDepth, s_XA95Depth, yDec, s_Y, zField, s_ZField});
        break;

    case CurveSettings::eProcess_Spherical:
        curveParameters.append({xIncDepth, s_XA95Depth, yDec});
        break;
    case CurveSettings::eProcess_Unknwon_Dec:
        curveParameters.append({xIncDepth, s_XA95Depth, zField, s_ZField});
        break;

    case CurveSettings::eProcess_Vector:
        curveParameters.append({xIncDepth, s_XA95Depth, yDec, zField, s_ZField});
        break;

    case CurveSettings::eProcess_None:
    default:
        break;
    }
    return curveParameters;
}


// MCMC
void Event::clear()
{
    for (auto &dat:mDates) {
        dat.clear();
    }
    mDates.clear();
    mTheta.clear();

    mVg.clear();
    mInitialized = false;
    mIsNode = false;
    mThetaNode = HUGE_VAL;//__builtin_inf();//INFINITY;
}

void Event::shrink_to_fit() noexcept
{
    for (auto &dat:mDates) {
        dat.shrink_to_fit();
    }
    mTheta.shrink_to_fit();
    mVg.shrink_to_fit();
}

void Event::clear_and_shrink() noexcept
{
    for (auto &dat:mDates) {
        dat.clear();
        dat.shrink_to_fit();
    }
    mDates.clear();
    mDates.shrink_to_fit();
    mTheta.clear();
    mTheta.shrink_to_fit();

    mVg.clear();
    mVg.shrink_to_fit();

    mInitialized = false;
    mIsNode = false;
    mThetaNode = HUGE_VAL;//__builtin_inf();//INFINITY;
}
/**
 * @brief Event::getThetaMaxPossible
 * Vérifie si l'initialisation est possible, controle la circularité,
 * Utilisé lors de l'initialisation pour controler la circularité, Ne controle pas les durées (tau), ni les hiatus (gamma)
 * @param originEvent
 * @param circularEventName indique le chemin en cas de circularité
 * @param startEvents
 * @return
 */

bool Event::getThetaMaxPossible(Event* originEvent, QString& circularEventName,  const std::vector<Event*> &startEvents)
{
#ifdef DEBUG
    QString startList;
    for (auto e : startEvents)
        startList += e->getQStringName() + "->";
#endif

    std::vector<Event*> newStartEvents = startEvents;
    newStartEvents.push_back(this);

   // const QString parallelStr  (" | ");
    const QString serieStr  (" ➡︎ ");

    if (mIsNode)
        return true;

    // list of phase under
    bool noPhaseFwd (true);
    if (!mPhases.empty())
        for (const auto& phase : mPhases)
            noPhaseFwd = noPhaseFwd && (phase->mConstraintsNextPhases.empty());

    //--
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela, puisqu'elle est échantillonnée ou fixée)




    if (noPhaseFwd && mConstraintsFwd.empty()) {
        mIsNode = true;
        return true;
    }

    else {
        // Check constraints in Events Scene
        if (!mConstraintsFwd.empty())
            for (const auto& constFwd : mConstraintsFwd) {
                if (constFwd->mEventTo.get() != originEvent ) {
                //if (!newStartEvents.contains (constFwd->mEventTo)) {
                   // qDebug() << " mConstraintsFwd" << constFwd->mEventTo->mName;
                     const bool _ok = (constFwd->mEventTo)->getThetaMaxPossible (originEvent, circularEventName, newStartEvents);
                     if ( !_ok) {
                         circularEventName =  serieStr + constFwd->mEventTo->getQStringName() +  circularEventName ;
                         return false;
                    }

                } else {
                    circularEventName = serieStr + constFwd->mEventTo->getQStringName() + " ?";
                    return false;
                }
            }



        if (!noPhaseFwd) {
            // Check constraints in Phases Scene
            for (const auto& phase : mPhases) {
                if (!phase->mConstraintsNextPhases.empty()) {
                    for (const auto& phaseFwd : phase->mConstraintsNextPhases) {

                        for (const auto& eventPhaseFwd : phaseFwd->mPhaseTo->mEvents) {
                            if (!container_contains(newStartEvents, eventPhaseFwd.get())) {
                                const bool _ok = eventPhaseFwd->getThetaMaxPossible ( originEvent, circularEventName, newStartEvents);
                                if (!_ok) {
                                    circularEventName = " (" + phase->getQStringName() + ")" + serieStr + eventPhaseFwd->getQStringName() + " (" + phaseFwd->mPhaseTo->getQStringName() + ")" +  circularEventName ;
                                    return false;
                                }


                            }
                            if (eventPhaseFwd.get() == originEvent ) {
                                circularEventName = " (" + phase->getQStringName() + ")" + serieStr + eventPhaseFwd->getQStringName() + " (" + phaseFwd->mPhaseTo->getQStringName() + ") !";
                                return false;
                            }
                        }

                    }
                 }
            }
        }

        mIsNode = true;
        return true;
    }
}

// vrai si origin est aprés this
bool Event::is_direct_older(const Event &origin)
{
    bool is_direct_link = false;
    if (!mConstraintsFwd.empty()) {
        for (const auto& constFwd : mConstraintsFwd) {
            if (constFwd->mEventTo.get() != &origin ) {
                is_direct_link = (constFwd->mEventTo)->is_direct_older(origin);

            } else {
                is_direct_link = true;
            }

            if ( is_direct_link) {
                return true;
            }
        }
    }
    // Liaison par les phases
    bool is_phase_link = false;
    if (!mPhases.empty()) {
        for (const auto& phase : mPhases) {
            if (!phase->mConstraintsNextPhases.empty()) {
                for (const auto& next_phase : phase->mConstraintsNextPhases) {
                    for (const auto& n_event : next_phase->mPhaseTo->mEvents) {
                        is_phase_link = n_event->is_direct_older(origin);
                        if ( is_phase_link) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}


// is_direct_younger ::vrai si origin est avant Event
bool Event::is_direct_younger(const Event &origin)
{
    bool is_direct_link = false;
    if (!mConstraintsBwd.empty()) {
        for (const auto& constBwd : mConstraintsBwd) {
            if (constBwd->mEventFrom.get() != &origin ) {
                is_direct_link = (constBwd->mEventFrom)->is_direct_younger(origin);

            } else {
                is_direct_link = true;
            }

            if ( is_direct_link) {
                return true;
            }
        }
    }
    // Liaison par les phases
    bool is_phase_link = false;
    if (!mPhases.empty()) {
        for (const auto& phase : mPhases) {
            if (!phase->mConstraintsPrevPhases.empty()) {
                for (const auto& prev_phase : phase->mConstraintsPrevPhases) {
                    for (const auto& p_event : prev_phase->mPhaseFrom->mEvents) {
                        is_phase_link = p_event->is_direct_younger(origin);
                        if ( is_phase_link) {
                            return true;;
                        }
                    }
                }
            }
        }
    }
    return false;
}

double Event::getThetaMinRecursive_v2(const double defaultValue, const std::vector<Event* > &startEvents)
{
     //qDebug()<<"rentre dans getThetaMinRecursive()"<< mName;
    // if the Event is initiated, constraints was controled previously
    if (mInitialized) {
            return mTheta.mX;

    } else if (mIsNode) {
        return mThetaNode;

    }

    // list of phase under
    bool noPhaseBwd = true;
    if (!mPhases.empty())
        for (const auto& phase : mPhases) {
            noPhaseBwd = noPhaseBwd && (phase->mConstraintsPrevPhases.empty());
        }

    //--
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    std::vector<Event*> newStartEvents = startEvents;
    newStartEvents.push_back(this);

    double maxPhases = defaultValue;
    for (const auto& phase : mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMax = defaultValue;
            for (auto&& event : phase->mEvents) {

                // On recherche la valeur de theta la plus grande de la phase et on soustrait Tau, il ne faut pas faire de récursif
                // On ne tient pas compte des theta non initialisés. Ils se mettront en place quand sera leur tour
                if (!container_contains(startEvents, event.get())) {
                    if (event->mInitialized) {
                       thetaMax = std::max(thetaMax, event-> mTheta.mX);

                   } else if (event->mIsNode) {
                       thetaMax = std::max(thetaMax, event-> mThetaNode);

                   }
                }
            }
            // Si aucune date initialisé maxPhase n'est pas évaluable donc égale à defaultValue
            if (thetaMax != defaultValue)
                maxPhases = std::max(maxPhases, thetaMax - phase->mTau.mX);

        }

    }


    if (noPhaseBwd && mConstraintsBwd.empty()) {
        mIsNode = true;
        mThetaNode = maxPhases;
        return mThetaNode;
    }

    else {
        double maxTheta = defaultValue;
        if (!mConstraintsBwd.empty())
            for (auto&& constBwd : mConstraintsBwd) {
                if (!container_contains(startEvents, constBwd->mEventFrom.get())) {
                    maxTheta = std::max(maxTheta, (constBwd->mEventFrom)->getThetaMinRecursive_v2(defaultValue, newStartEvents));
                    //qDebug()<<" thetaMin "<< mName <<"in constBwd"<<constBwd->mEventFrom->mName << maxTheta;
                }
            }

        double maxPhasesBwd = defaultValue;
        if (!noPhaseBwd) {

            for (auto&& phase : mPhases) {
                if (!phase->mConstraintsPrevPhases.empty()) {
                    double maxThetaBwd = defaultValue;
                    for (auto&& phaseBwd : phase->mConstraintsPrevPhases) {

                        for (auto&& eventPhaseBwd : phaseBwd->mPhaseFrom->mEvents) {
                            if (!container_contains(startEvents, eventPhaseBwd.get())) {
                                maxThetaBwd = std::max(maxThetaBwd, eventPhaseBwd->getThetaMinRecursive_v2(defaultValue, newStartEvents));
                            }
                        }
                        if (phaseBwd->mGammaType != PhaseConstraint::eGammaUnknown)
                            maxPhasesBwd = std::max(maxPhasesBwd, maxThetaBwd + phaseBwd->mGamma);
                        else
                            maxPhasesBwd = std::max(maxPhasesBwd, maxThetaBwd);
                    }
                 }
            }
        }
        mIsNode = true;
        mThetaNode = std::max({maxTheta, maxPhases,  maxPhasesBwd});
        return mThetaNode;
    }


}

// utiliser que dans initialize_time
double Event::getThetaMaxRecursive_v2(const double defaultValue, const std::vector<Event*> &startEvents)
{
    // if the Event is initialized, constraints was controled previously
    if (mInitialized) {
        return mTheta.mX;

    } else if (mIsNode) {
        return mThetaNode;

    }

    // list of phase under
    bool noPhaseNext = true;
    if (!mPhases.empty()) {
        for (const auto& phase : mPhases) {
            noPhaseNext = noPhaseNext && (phase->mConstraintsNextPhases.empty());
        }
    }
    // list of phase upper
    bool noPhaseBwd = true;
    if (!mPhases.empty()) {
        for (const auto& phase : mPhases) {
            noPhaseBwd = noPhaseBwd && (phase->mConstraintsPrevPhases.empty());
        }
    }

    //--
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    std::vector<Event*> newStartEvents = startEvents;
    newStartEvents.push_back(this);

    double minPhases = defaultValue;
    for (const auto& phase :mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMin = defaultValue - phase->mTau.mX;
            for (const auto& event : phase->mEvents) {
                if (event.get() != this) {

                    if (event->mInitialized) {
                        thetaMin = std::min(thetaMin, event-> mTheta.mX- phase->mTau.mX);

                    } else if (event->mIsNode) {
                        thetaMin = std::min(thetaMin, event-> mThetaNode);

                    }

                }
            }

            minPhases = thetaMin;

        }
     }

    // Pas de contrainte au dessus, ni de phase au dessus
     // Le Node comprends décallage en Tau
    if (noPhaseBwd && mConstraintsBwd.empty()) {
            mIsNode = true;
            mThetaNode = minPhases;
            return mThetaNode;
    }


    else {
        double minTheta = defaultValue;
        if (!mConstraintsFwd.empty())
            for (const auto& constFwd : mConstraintsFwd) {
                if (!container_contains(startEvents, constFwd->mEventTo.get())) {
                    minTheta = std::min(minTheta, constFwd->mEventTo->getThetaMaxRecursive_v2(defaultValue, newStartEvents));
                }
            }


        double minPhasesNext = defaultValue;
        if (!noPhaseNext) {

            for (const auto& phase : mPhases) {
                if (!phase->mConstraintsNextPhases.empty()) {
                    double minThetaFwd = defaultValue;
                    for (const auto& phase_next : phase->mConstraintsNextPhases) {

                        for (const auto& eventPhaseFwd : phase_next->mPhaseTo->mEvents) {
                            if (!container_contains(newStartEvents, eventPhaseFwd.get())) {
                                minThetaFwd = std::min(minThetaFwd, eventPhaseFwd->getThetaMaxRecursive_v2(defaultValue, newStartEvents));
                            }
                        }

                        if (phase_next->mGammaType != PhaseConstraint::eGammaUnknown)
                            minPhasesNext = std::min(minPhasesNext, minThetaFwd - phase_next->mGamma);
                        else
                            minPhasesNext = std::min(minPhasesNext, minThetaFwd);
                    }
                }
                if (phase->mTauType != Phase::eTauUnknown) {
                    double thetaMin = minPhasesNext;// - phase->mTau.mX;
                    for (const auto& event : phase->mEvents) {

                        if (event.get() != this) {
                            if (event->mInitialized) {
                                thetaMin = std::min(thetaMin, event-> mTheta.mX - phase->mTau.mX);

                            } else if (event->mIsNode) {
                                thetaMin = std::min(thetaMin, event-> mThetaNode);

                            }

                        }
                    }

                }

            }
        }
        mIsNode = true;
        mThetaNode = std::min(minTheta, minPhases);
        mThetaNode = std::min(minPhasesNext, mThetaNode);
        return mThetaNode;
    }


}


// Utiliser que dans initialize_time

/* ------------------------------------------------------------------ */
/*  Wrapper public – appel simple depuis l’extérieur                  */
/* ------------------------------------------------------------------ */
double Event::getThetaMinRecursive_v3(double defaultValue,
                                      const std::vector<Event*>& startEvents)
{
    std::unordered_set<Event*> visited;
    visited.reserve(startEvents.size() + 64); // pré‑allocation légère
    // On insère les événements de départ (ils sont déjà « visités »)
    for (Event* e : startEvents)
        visited.insert(e);
    std::unordered_map<Event*, double> memo; // cache local à cet appel
    memo.reserve(256); // estimation, évite re‑hashes
    return getThetaMinRecursive_v3_impl(defaultValue, visited, memo);
}

/* ------------------------------------------------------------------ */
/*  Fonction principale – version optimisée                           */
/* ------------------------------------------------------------------ */
double Event::getThetaMinRecursive_v3_impl(double defaultValue,
                                           std::unordered_set<Event*>& visited,
                                           std::unordered_map<Event*, double>& memo)
{
    // 1️⃣ Mémoïsation locale (déjà calculé dans un autre chemin)
    auto itMemo = memo.find(this);
    if (itMemo != memo.end())
        return itMemo->second;
    // 2️⃣ Cas déjà initialisé ou déjà traité (mIsNode)
    if (mInitialized || mIsNode)
        return mTheta.mX;
    // 3️⃣ Protection contre les cycles (déjà présent dans le set)
    if (!visited.insert(this).second)   // insertion échouée → déjà dans le set
        return defaultValue;            // on sort rapidement du cycle
    // ------------------------------------------------------------------
    // 4️⃣ 1 – Propagation des contraintes « strati »
    // ------------------------------------------------------------------
    double max_theta_strati = defaultValue;

    for (const auto& constBwd_sp : mConstraintsBwd) {          // constBwd_sp : const std::shared_ptr<EventConstraint>&
        const EventConstraint* constBwd = constBwd_sp.get(); // pointeur brut
        double v = constBwd->mEventFrom->getThetaMinRecursive_v3_impl(defaultValue,
                                                                 visited,
                                                                 memo);
        max_theta_strati = std::max(max_theta_strati, v);
    }
    // ------------------------------------------------------------------
    // 5️⃣ 2 – Contraintes de phase (gamma)
    // ------------------------------------------------------------------
    double max_theta_phase_strati = defaultValue;
    for (const auto& phasePtr : mPhases) {               // phasePtr = std::shared_ptr<Phase>
        const Phase* phase = phasePtr.get();             // pointeur brut pour la suite

        for (const auto& bwd_phase_sp : phase->mConstraintsPrevPhases) {
            const PhaseConstraint* bwd_phase = bwd_phase_sp.get();
            double candidate = bwd_phase->mPhaseFrom->init_max_theta(defaultValue) + bwd_phase->mGamma;
            max_theta_phase_strati = std::max(max_theta_phase_strati, candidate);
        }
    }
    // ------------------------------------------------------------------
    // 6️⃣ 3 – Contraintes de durée (tau) à l’intérieur des phases
    // ------------------------------------------------------------------
    double max_tau_phases = defaultValue;
    for (const auto& phasePtr : mPhases) {               // phasePtr = std::shared_ptr<Phase>
        const Phase* phase = phasePtr.get();             // pointeur brut pour la suite

        if (phase->mTauType == Phase::eTauUnknown)
            continue;                     // rien à faire pour cette phase
        double th_max_phase = defaultValue;
        for (const auto& th_friend_ptr : phase->mEvents) {
            Event* th_friend = th_friend_ptr.get();
            if (th_friend == this) continue;
            if (th_friend->mInitialized) {
                th_max_phase = std::max(th_max_phase, th_friend->mTheta.mX);
            }
            else {
                // On ne descend que si le friend est « plus jeune » que *this
                if (!this->is_direct_older(*th_friend)) {
                    for (const auto& constBwd_sp : th_friend->mConstraintsBwd) {
                        const EventConstraint* constBwd = constBwd_sp.get();
                        // évite les boucles déjà parcourues
                        if (visited.find(constBwd->mEventFrom.get()) == visited.end()) {
                            double v = constBwd->mEventFrom->getThetaMinRecursive_v3_impl(
                                defaultValue, visited, memo);
                            th_max_phase = std::max(th_max_phase, v);
                        }
                    }
                }
            }
        }
        // tau = th_max_phase - tau_offset
        max_tau_phases = std::max(max_tau_phases, th_max_phase - phase->mTau.mX);
    }
    // ------------------------------------------------------------------
    // 7️⃣  Consolidation du résultat
    // ------------------------------------------------------------------
    double result = std::max(max_theta_strati, std::max(max_theta_phase_strati, max_tau_phases));
    // Marquage « node » + mémorisation globale
    mIsNode = true;
    mTheta.mX = result;
    memo[this] = result;
    // Nettoyage du set de visite (pop‑like)
    visited.erase(this);
    return result;
}

// defaultValue = min period
double Event::getThetaMinRecursive_v3_old(const double defaultValue, const std::vector<Event* > &startEvents)
{
    if (mInitialized) {
        return mTheta.mX;

    } else  if (mIsNode) {
        return mTheta.mX;

    } else   {
        // 1 - Descendre les contraintes pour retrouver la valeur la plus grande
        std::vector<Event*> newStartEvents = startEvents;
        newStartEvents.push_back(this);
        double max_theta_strati = defaultValue;
        if (!mConstraintsBwd.empty()) {
            for (auto&& constBwd : mConstraintsBwd) {
                max_theta_strati = std::max(max_theta_strati, constBwd->mEventFrom->getThetaMinRecursive_v3(defaultValue, newStartEvents));
            }
        }
        // 2 - Si l'Event appartient à une phase, descendre les contraintes de phase en ajoutant les gamma
        double max_theta_phase_strati = defaultValue;
        if (!mPhases.empty()) {
            for (const auto& phase : mPhases) { // clazy:skip
                for (const auto& bwd_phase : phase->mConstraintsPrevPhases) { // clazy:skip
                    max_theta_phase_strati = std::max(max_theta_phase_strati, bwd_phase->mPhaseFrom->init_max_theta(defaultValue) + bwd_phase->mGamma);
                }
            }
        }


        // 3 - Si dans une phase avec durée, il y a des Events initialisés. voir la contrainte de durée
        double max_tau_phases = defaultValue;
        if (!mPhases.empty()) {
            // C'est la date max-tau
            for (const auto& phase : mPhases) {
                double th_max_phase = defaultValue;
                if (phase->mTauType != Phase::eTauUnknown) {
                    for (const auto& th_friend : phase->mEvents) { // clazy:skip
                        if (th_friend->mInitialized == true && th_friend.get() != this ) {
                            th_max_phase = std::max(th_max_phase, th_friend->mTheta.mX);
                        }
                        // ---
                        else if (th_friend->mInitialized == false && th_friend.get() != this) {
                            bool friend_is_under = !this->is_direct_older(*th_friend);

                            //qDebug()<<"min recursive fiend="<<th_friend->mName<<" this="<<this->mName<<" je fais"<<friend_is_under;
                            if (friend_is_under) {
                                for (auto&& constBwd : th_friend->mConstraintsBwd) {
                                    if (!container_contains(startEvents, constBwd->mEventFrom.get())) {
                                        th_max_phase = std::max(th_max_phase, constBwd->mEventFrom->getThetaMinRecursive_v3(defaultValue, newStartEvents));
                                    }
                                }
                            }
                        }
                        // ---
                    }
                    max_tau_phases = std::max(max_tau_phases, th_max_phase - phase->mTau.mX);
                }
            }
        }

        mIsNode = true;
        mTheta.mX = std::max({max_theta_strati, max_theta_phase_strati, max_tau_phases});

        return mTheta.mX;
    }

}

// Utilisé que dans initialize_time
// defaultValue = max period
double Event::getThetaMaxRecursive_v3_old(const double defaultValue, const std::vector<Event*> &startEvents)
{
    if (mInitialized) {
        return mTheta.mX;

    } else  if (mIsNode) {
        return mTheta.mX;

    } else  {
        // 1 - remonter les contraintes pour retrouver la valeur la plus grande
        std::vector<Event*> newStartEvents = startEvents;
        newStartEvents.push_back(this);

        double min_theta_strati = defaultValue;
        if (!mConstraintsFwd.empty()) {
            for (auto&& constFwd : mConstraintsFwd) {
                min_theta_strati = std::min(min_theta_strati, constFwd->mEventTo->getThetaMaxRecursive_v3(defaultValue, newStartEvents));
            }
        }
        // 2 - Si l'Event appartient à une phase, remonter les contraintes de phase en soustrayant les gamma
        double min_theta_phase_strati = defaultValue;
        if (!mPhases.empty()) {
            for (const auto& phase : mPhases) {
                for (const auto& fwd_phase : phase->mConstraintsNextPhases) { // clazy:skip
                    //min_theta_phase_strati = std::min(min_theta_phase_strati, fwd_phase->mPhaseTo->init_min_theta(defaultValue) - fwd_phase->mGamma);
                    // aucune copie de shared_ptr n’est faite ici
                    double candidate = fwd_phase->mPhaseTo->init_min_theta(defaultValue)
                                       - fwd_phase->mGamma;
                    min_theta_phase_strati = std::min(min_theta_phase_strati, candidate);

                }
            }
        }

        // 3 - Si dans une phase avec durée, voir la contrainte de durée
        double min_tau_phases = defaultValue;
        if (!mPhases.empty()) {
            // C'est la date min+tau
            for (const auto& phase : mPhases) {
                double th_min_phase = defaultValue;
                if (phase->mTauType != Phase::eTauUnknown) {
                    for (const auto& th_friend : phase->mEvents) { // clazy:skip
                        if (th_friend->mInitialized == true && th_friend.get() != this ) {
                            th_min_phase = std::min(th_min_phase, th_friend->mTheta.mX);
                        }
                        // ----
                        else if (th_friend->mInitialized == false && th_friend.get() != this) {
                            bool friend_is_upper = !this->is_direct_younger(*th_friend);

                            //qDebug()<<"max recursive friend="<<th_friend->mName<<" this="<<this->mName<<" je fais"<<friend_is_upper;
                            if (friend_is_upper) {
                                for (auto&& constFwd : th_friend->mConstraintsFwd) {
                                    if (!container_contains(startEvents, constFwd->mEventTo.get())) {
                                        th_min_phase = std::min(th_min_phase, constFwd->mEventTo->getThetaMaxRecursive_v3(defaultValue, newStartEvents));
                                    }
                                }
                            }
                        }
                        // ---
                    }
                    min_tau_phases = std::min(min_tau_phases, th_min_phase + phase->mTau.mX);
                }
            }
        }


        mIsNode = true;

        mTheta.mX = std::min({min_theta_strati, min_theta_phase_strati, min_tau_phases});

        return mTheta.mX;
    }

}

/* ------------------------------------------------------------------ */
/*  Wrapper public – appel simple depuis l’extérieur                 */
/* ------------------------------------------------------------------ */
double Event::getThetaMaxRecursive_v3(double defaultValue,
                                          const std::vector<Event*>& startEvents)
{
    std::unordered_set<Event*> visited;
    visited.reserve(startEvents.size() + 64);
    for (Event* e : startEvents) visited.insert(e);
    std::unordered_map<Event*, double> memo;
    memo.reserve(256);
    return getThetaMaxRecursive_v3_impl(defaultValue, visited, memo);
}
/* ------------------------------------------------------------------ */
/*  Implémentation interne – version optimisée                       */
/* ------------------------------------------------------------------ */
double Event::getThetaMaxRecursive_v3_impl(double defaultValue,
                                           std::unordered_set<Event*>& visited,
                                           std::unordered_map<Event*, double>& memo)
{
    // --------------------------------------------------------------
    // 1️⃣ Mémoïsation locale
    // --------------------------------------------------------------
    auto itMemo = memo.find(this);
    if (itMemo != memo.end())
        return itMemo->second;
    // --------------------------------------------------------------
    // 2️⃣ Cas déjà initialisé ou déjà calculé
    // --------------------------------------------------------------
    if (mInitialized || mIsNode)
        return mTheta.mX;
    // --------------------------------------------------------------
    // 3️⃣ Protection contre les cycles
    // --------------------------------------------------------------
    if (!visited.insert(this).second)          // déjà présent → cycle
        return defaultValue;
    // --------------------------------------------------------------
    // 4️⃣ 1 – Remonter les contraintes « forward » (max → min)
    // --------------------------------------------------------------
    double min_theta_strati = defaultValue;
    for (const auto& constFwd_sp : mConstraintsFwd) {
        const EventConstraint* constFwd = constFwd_sp.get();
        double v = constFwd->mEventTo->getThetaMaxRecursive_v3_impl(
            defaultValue, visited, memo);
        min_theta_strati = std::min(min_theta_strati, v);
    }
    // --------------------------------------------------------------
    // 5️⃣ 2 – Contraintes de phase (soustraction du gamma)
    // --------------------------------------------------------------
    double min_theta_phase_strati = defaultValue;
    for (const auto& phasePtr : mPhases) {
        const Phase* phase = phasePtr.get();   // pointeur brut
        for (const auto& fwd_phase_sp : phase->mConstraintsNextPhases) {
            const PhaseConstraint* fwd_phase = fwd_phase_sp.get();
            double candidate = fwd_phase->mPhaseTo->init_min_theta(defaultValue)
                               - fwd_phase->mGamma;
            min_theta_phase_strati = std::min(min_theta_phase_strati, candidate);
        }
    }
    // --------------------------------------------------------------
    // 6️⃣ 3 – Contraintes de durée (tau) à l’intérieur des phases
    // --------------------------------------------------------------
    double min_tau_phases = defaultValue;
    for (const auto& phasePtr : mPhases) {
        const Phase* phase = phasePtr.get();
        if (phase->mTauType == Phase::eTauUnknown)
            continue;                                   // aucune contrainte de durée
        double th_min_phase = defaultValue;
        for (const auto& th_friend_sp : phase->mEvents) {   // mEvents = vector<shared_ptr<Event>>
            Event* th_friend = th_friend_sp.get();
            if (th_friend == this) continue;
            if (th_friend->mInitialized) {
                // Event déjà initialisé → on prend directement sa valeur
                th_min_phase = std::min(th_min_phase, th_friend->mTheta.mX);
            }
            else {
                // Event non initialisé : on ne descend que si le friend est « plus haut »
                if (!this->is_direct_younger(*th_friend)) {
                    for (const auto& constFwd_sp : th_friend->mConstraintsFwd) {
                        const EventConstraint* constFwd = constFwd_sp.get();
                        // évite de revisiter un nœud déjà présent dans le chemin
                        if (visited.find(constFwd->mEventTo.get()) == visited.end()) {
                            double v = constFwd->mEventTo->getThetaMaxRecursive_v3_impl(
                                defaultValue, visited, memo);
                            th_min_phase = std::min(th_min_phase, v);
                        }
                    }
                }
            }
        }
        // tau = th_min_phase + tau_offset
        min_tau_phases = std::min(min_tau_phases,
                                  th_min_phase + phase->mTau.mX);
    }
    // --------------------------------------------------------------
    // 7️⃣ Consolidation du résultat
    // --------------------------------------------------------------
    double result = std::min(min_theta_strati,
                             std::min(min_theta_phase_strati, min_tau_phases));
    mIsNode   = true;
    mTheta.mX = result;
    memo[this] = result;
    // --------------------------------------------------------------
    // 8️⃣ Nettoyage du set de visite (pop‑like)
    // --------------------------------------------------------------
    visited.erase(this);
    return result;
}

double Event::getThetaMin(double defaultValue)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne min courante pour le tirage de theta
    // ------------------------------------------------------------------

    // --------------------------------------------------------------
    // 1️⃣  Max des thetas des contraintes directes antérieures
    // --------------------------------------------------------------
    double maxThetaBwd = defaultValue;
    for (const auto& csp : mConstraintsBwd) {          // csp = shared_ptr<EventConstraint>
        const EventConstraint* c = csp.get();          // pointeur brut
        maxThetaBwd = std::max(maxThetaBwd, c->mEventFrom->mTheta.mX);
    }

    /* Le fait appartient à une ou plusieurs phases.
     * Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
     * Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
     * (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
     */
    /*double min3 = defaultValue;

    for (const auto& phase : mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMax = defaultValue;
            for (const auto& event : phase->mEvents) {
                if (event.get() != this)
                    thetaMax = std::max(event->mTheta.mX, thetaMax);
            }
            min3 = std::max(min3, thetaMax - phase->mTau.mX);
        }
    }

    // Contraintes des phases précédentes
    double maxPhasePrev = defaultValue;
    for (const auto& phase : mPhases) {
        const double thetaMax = phase->getMaxThetaPrevPhases(defaultValue);
        maxPhasePrev = std::max(maxPhasePrev, thetaMax);
    }

    return std::max({defaultValue, maxThetaBwd, min3, maxPhasePrev});
*/
    // --------------------------------------------------------------
    // 2️⃣  Parcours des phases (calcul combiné de min3 et maxPhasePrev)
    // --------------------------------------------------------------
    double min3          = defaultValue;   // max(theta) - tau  (pour les phases avec durée)
    double maxPhasePrev  = defaultValue;   // max des thetas des phases précédentes
    // cache local : phase → max(theta) déjà calculé dans la phase
    std::unordered_map<const Phase*, double> phaseThetaCache;
    phaseThetaCache.reserve(mPhases.size());

    for (const auto& psp : mPhases) {               // psp = shared_ptr<Phase>
        const Phase* phase = psp.get();
        // ----- 2a – max(theta) dans la phase (utilisé deux fois) -----
        double thetaMaxInPhase = defaultValue;
        auto cacheIt = phaseThetaCache.find(phase);
        if (cacheIt != phaseThetaCache.end()) {
            thetaMaxInPhase = cacheIt->second;
        } else {
            for (const auto& esp : phase->mEvents) {   // esp = shared_ptr<Event>
                const Event* ev = esp.get();
                if (ev != this) {
                    thetaMaxInPhase = std::max(thetaMaxInPhase, ev->mTheta.mX);
                }
            }
            phaseThetaCache.emplace(phase, thetaMaxInPhase);
        }
        // ----- 2b – contrainte de durée (tau) -----
        if (phase->mTauType != Phase::eTauUnknown) {
            // on veut être au‑dessus du plus grand theta de la phase moins la durée
            min3 = std::max(min3, thetaMaxInPhase - phase->mTau.mX);
        }
        // ----- 2c – contraintes des phases précédentes -----
        double thetaPrev = phase->getMaxThetaPrevPhases(defaultValue);
        maxPhasePrev = std::max(maxPhasePrev, thetaPrev);
    }
    // --------------------------------------------------------------
    // 3️⃣  Consolidation du résultat (sans initializer_list)
    // --------------------------------------------------------------
    double result = std::max(defaultValue, maxThetaBwd);
    result = std::max(result, min3);
    result = std::max(result, maxPhasePrev);
    return result;
}

/*
double Event::getThetaMax(double defaultValue)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne max
    // ------------------------------------------------------------------

    // Min des thetas des faits en contrainte directe et qui nous suivent
    double minThetaFwd = defaultValue;
    for (const auto& cFwd : mConstraintsFwd) {
        minThetaFwd = std::min(minThetaFwd, cFwd->mEventTo->mTheta.mX);
    }

    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    //
    double max3 = defaultValue;
    for (const auto& phase : mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMin = defaultValue;
             for (const auto& event : phase->mEvents) {
                if (event.get() != this)
                    thetaMin = std::min(event->mTheta.mX, thetaMin);
            }
            max3 = std::min(max3, thetaMin + phase->mTau.mX);
        }
    }

    // Contraintes des phases suivantes
    double maxPhaseNext = defaultValue;
    for (auto&& phase : mPhases) {
        maxPhaseNext = std::min(maxPhaseNext, phase->getMinThetaNextPhases(defaultValue));
    }

    return std::min({minThetaFwd, max3, maxPhaseNext});

}
*/

    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    //
double Event::getThetaMax(double defaultValue)
{
    // ---- 1. contraintes forward ----
    double minThetaFwd = defaultValue;
    for (const auto& cFwd_sp : mConstraintsFwd) {
        minThetaFwd = std::min(minThetaFwd, cFwd_sp->mEventTo->mTheta.mX);
    }
    // ---- 2. boucle unique sur les phases ----
    double max3 = defaultValue;
    double maxPhaseNext = defaultValue;
    std::unordered_map<const Phase*, double> thetaMinCache;
    thetaMinCache.reserve(mPhases.size());
    for (const auto& psp : mPhases) {
        const Phase* phase = psp.get();
        // thetaMin de la phase (mise en cache)
        double thetaMinInPhase = defaultValue;
        auto it = thetaMinCache.find(phase);
        if (it != thetaMinCache.end()) {
            thetaMinInPhase = it->second;
        } else {
            for (const auto& esp : phase->mEvents) {
                const Event* ev = esp.get();
                if (ev != this)
                    thetaMinInPhase = std::min(thetaMinInPhase, ev->mTheta.mX);
            }
            thetaMinCache.emplace(phase, thetaMinInPhase);
        }
        // contrainte de durée
        if (phase->mTauType != Phase::eTauUnknown)
            max3 = std::min(max3, thetaMinInPhase + phase->mTau.mX);
        // contrainte des phases suivantes
        maxPhaseNext = std::min(maxPhaseNext,
                                phase->getMinThetaNextPhases(defaultValue));
    }
    // ---- 3. résultat final ----
    double result = std::min(minThetaFwd, max3);
    result = std::min(result, maxPhaseNext);
    return result;
}




void Event::updateTheta_v3(const double tmin, const double tmax)
{
    for (auto&& date : mDates )   {
        date.updateDate(mTheta.mX, mS02Theta.mX, mAShrinkage);

    }

    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);
    //qDebug() << "----------->      in Event::updateTheta(): Event update : " << this->mName << " : " << this->mTheta.mX << " between" << "[" << min << " ; " << max << "]";

    if (min > max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(getQStringName(), QString::number(min), QString::number(max));

    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------

    double sum_p = 0.0;
    double sum_t = 0.0;

    for (auto&& date: mDates) {
        const double variance  = pow(date.mSigmaTi.mX, 2);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1.0 / variance;
    }
    const double ti_avg = sum_t / sum_p;
    const double sigma = 1.0 / sqrt(sum_p);

    if (min == max) {
        double try_theta = min;
        mTheta.accept_update(try_theta);

    } else {
        switch(mTheta.mSamplerProposal)
        {
        case MHVariable::eDoubleExp:
        {
            try {
                double try_theta = Generator::gaussByDoubleExp(ti_avg, sigma, min, max);
                mTheta.accept_update(try_theta);

            }
            catch(QString error) {
                throw QObject::tr("Error for event : %1 : %2").arg(getQStringName(), error);
            }
            break;
        }

            // Event Prior
        case MHVariable::eBoxMuller:
        {
            /* double theta;

            long long counter = 0.;
            do {
                theta = Generator::normalDistribution(ti_avg, sigma);
                ++counter;
                if (counter == 100000000)
                    throw QObject::tr("No MCMC solution could be found using event method %1 for event named %2 ( %3  trials done)").arg(MHVariable::getSamplerProposalText(mTheta.mSamplerProposal), getQStringName(), QString::number(counter));

            } while(theta < min || theta > max);
            */

            double try_theta = Generator::truncatedNormal(ti_avg, sigma, min, max);
            mTheta.accept_update(try_theta);
            break;
        }

        case MHVariable::eMHAdaptGauss:
        {
            // MH: The only case where the acceptance rate makes sense, since we use sigma MH :
            double try_theta = Generator::normalDistribution(mTheta.mX, mTheta.mSigmaMH);
            double rate = 0.0;
            if (try_theta >= min && try_theta <= max) {
                double diff1 = try_theta - ti_avg;
                double diff2 = mTheta.mX - ti_avg;
                rate = -0.5 * (diff1*diff1 - diff2*diff2) / (sigma*sigma);

            }
#ifdef DEBUG
            if (try_theta <min  || max < try_theta) {
                std::cout << "[updateTheta_v3] 🔄 " << mName
                          << " min = " << min << " max = " << max
                          << ", theta = " << try_theta
                          << ((min < try_theta && try_theta < max) ? "✅ YES" : "❌ NO")
                          << std::endl;
            }
#endif
            mTheta.try_update_log(try_theta, rate);
            break;
        }

        default:
            break;
        }
    }


}

// identique à la updateTheta_v3
// mais à la place de date prior,
void Event::updateTheta_v4(const double tmin, const double tmax)
{
    for (auto&& date : mDates )   {
        date.updateDate(mTheta.mX, mS02Theta.mX, mAShrinkage);
    }

    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);
    //qDebug() << "----------->      in Event::updateTheta(): Event update : " << this->mName << " : " << this->mTheta.mX << " between" << "[" << min << " ; " << max << "]";

    if (min > max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(getQStringName(), QString::number(min), QString::number(max));

    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------

    double sum_p = 0.0;
    double sum_t = 0.0;

    for (auto&& date: mDates) {
        const double variance  = pow(date.mSigmaTi.mX, 2.);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1.0 / variance;
    }
    const double ti_avg = sum_t / sum_p;
    const double sigma = 1.0 / sqrt(sum_p);

    if (min == max) {
        const double theta = min;
        mTheta.accept_update(theta);

    } else {
            const double u1 = Generator::randomUniform();
            double try_value;
            double current_value = mTheta.mX;
            double mu = 1;

            if (u1 < mu) {
                    // prior Event = updateTheta_v3
                    try_value = Generator::truncatedNormal(ti_avg, sigma, min, max);
                    mTheta.accept_update(try_value);

            } else {
                const double s = 10000;
                try_value = Generator::truncatedNormal(ti_avg, s* sigma, min, max);

                // Composante de prior/vraisemblance
                double pi_y = dnorm(try_value, ti_avg, sigma);
                double pi_x = dnorm(current_value, ti_avg, sigma);

                // q(θ|θ') / q(θ'|θ)
                double q_y = dnorm(try_value, ti_avg, s* sigma);
                double q_x = dnorm(current_value, ti_avg, s* sigma);

                double rate = (pi_y*q_x) / (pi_x*q_y);

                mTheta.try_update(try_value, rate);

            }


    }


}

// ne marche pas, trop de rejet
/*
void Event::updateThetaAndTiSigma(const double tmin, const double tmax)
{
    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);

    if (min > max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3")
            .arg(getQStringName(), QString::number(min), QString::number(max));

    // ----------------------------------------
    // 1️⃣ Proposer un nouveau theta_j
    // ----------------------------------------

    double theta_current = mTheta.mX;
    double w1 = 0.5;

    const double u = Generator::randomUniform();
    double theta_try;
    if (u<w1) {
    theta_try = Generator::truncatedNormal(theta_current, mTheta.mSigmaMH, min, max);

    } else {
        theta_try = Generator::truncatedNormal(theta_current, 50, min, max);
    }

    if (theta_try <= min || theta_try >= max) {
        mTheta.reject_update();
        for (auto&& date : mDates) {
            date.mTi.reject_update();
            date.mSigmaTi.reject_update();
            date.mZi.reject_update();
        }
        return;
    }

    double log_target_current = 0.0;
    double log_target_try = 0.0;

    // Stockage temporaire pour t_i, sigma_i, z_i
    std::vector<double> try_ti(mDates.size());
    std::vector<double> try_si(mDates.size());
    std::vector<double> try_zi(mDates.size());


    // ----------------------------------------
    // 2️⃣ Boucle sur toutes les Dates
    // ----------------------------------------
    for (size_t idx = 0; idx < mDates.size(); ++idx) {
        auto& date = mDates[idx];

        // --- Tirage t_i dans la CDF L(t_i)
        const double u1 = Generator::randomUniform();
        const double idx_interp = interpolate_index(u1, date.mCalibration->mRepartition);
        const double t_try_i = date.mCalibration->mTmin + idx_interp * date.mCalibration->mStep;

        const double lik_try = date.getLikelihood(t_try_i);
        if (lik_try <= 0.0) { // rejet si t_i impossible
            mTheta.reject_update();
            for (auto&& d : mDates) {
                d.mTi.reject_update();
                d.mSigmaTi.reject_update();
                d.mZi.reject_update();
            }
            return;
        }

        // --- Tirage de z_i positif (RW sur log z_i)
        const double logz_current = std::log(date.mZi.mX);
        const double logz_try = Generator::normalDistribution(logz_current, date.mZi.mSigmaMH);
        const double z_i = std::exp(logz_try);

        // --- Reconstruction sigma_i
        const double sigma_try = (t_try_i - theta_try) / z_i;
        const double sigma_current = (date.mTi.mX - theta_current) / date.mZi.mX;

        if (sigma_try <= 0.0) { // rejet si sigma négatif
            mTheta.reject_update();
            for (auto&& d : mDates) {
                d.mTi.reject_update();
                d.mSigmaTi.reject_update();
                d.mZi.reject_update();
            }
            return;
        }

        const double V_try = sigma_try * sigma_try;
        const double V_current = sigma_current * sigma_current;

        const double S02 = mS02Theta.mX;
        const double AShrink = mAShrinkage;

        // --- Log-target MH
        log_target_current += std::log(date.getLikelihood(date.mTi.mX))
                              - (AShrink + 1.0) * std::log(S02 + V_current)
                              + std::log(std::abs(date.mZi.mX));

        log_target_try += std::log(lik_try)
                          - (AShrink + 1.0) * std::log(S02 + V_try)
                          + std::log(std::abs(z_i)); // Jacobien

        // --- Stockage temporaire
        try_ti[idx] = t_try_i;
        try_si[idx] = sigma_try;
        try_zi[idx] = z_i;
    }

    // ----------------------------------------
    // 3️⃣ Metropolis-Hastings via mTheta.try_update_log
    // ----------------------------------------
    if (mTheta.try_update_log(theta_try, log_target_try - log_target_current)) {
        // Acceptation : mettre à jour t_i, sigma_i et z_i
        for (size_t idx = 0; idx < mDates.size(); ++idx) {
            auto& date = mDates[idx];
            date.mTi.mX = try_ti[idx];
            date.mSigmaTi.mX = try_si[idx];
            date.mZi.mX = try_zi[idx];

            date.mTi.accept_update(date.mTi.mX);
            date.mSigmaTi.accept_update(date.mSigmaTi.mX);
            date.mZi.accept_update(date.mZi.mX);
        }
    } else {
        // Rejet : restaurer les anciens t_i, sigma_i et z_i
        for (auto&& date : mDates) {
            date.mTi.reject_update();
            date.mSigmaTi.reject_update();
            date.mZi.reject_update();
        }
    }
}
*/
/*
void Event::updateTheta_v4_mixing0(const double tmin, const double tmax, const double rate_theta)
{
    for (auto&& date : mDates )   {
       date.updateDate(mTheta.mX, mS02Theta.mX, mAShrinkage);
    }

    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);
    //qDebug() << "----------->      in Event::updateTheta(): Event update : " << this->mName << " : " << this->mTheta.mX << " between" << "[" << min << " ; " << max << "]";

    if (min >= max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(getQStringName(), QString::number(min), QString::number(max));

    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------
    // tirage de theta
    constexpr double mu = 0.9;
    const double u = Generator::randomUniform();
    double sum_p = 0.0;
    double sum_t = 0.0;

    for (auto&& date: mDates) {
        const double variance  = pow(date.mSigmaTi.mX, 2.);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1.0 / variance;
    }
    const double ti_avg = sum_t / sum_p;
    const double sigma = 1.0 / sqrt(sum_p);

    double theta_try ;
    if (u > mu) { // Q1
        //const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), mMixingCalibrations->mRepartition);
        const double idx = interpolate_index(Generator::randomUniform(), mMixingCalibrations->mRepartition);
        theta_try  = mMixingCalibrations->mTmin + idx * mMixingCalibrations->mStep;

    } else { //Q2

        theta_try = Generator::truncatedNormal(ti_avg, sigma, min, max);
    }

    // Calcul rapport MH


    if (theta_try >= min && theta_try <= max) {

        const double q2_old = mMixingCalibrations->interpolate(mTheta.mX);
        const double q2_new = mMixingCalibrations->interpolate(theta_try);

        const double q1_new = dnorm(theta_try, ti_avg, sigma);
        const double q1_old = dnorm(mTheta.mX, ti_avg, sigma);

        const double rate_Q = (mu * q1_old + (1 - mu) * q2_old) / (mu * q1_new + (1 - mu) * q2_new);
        const double rate_P = q1_new / q1_old;
        double rate = rate_P * rate_Q;

        mTheta.try_update(theta_try, rate * rate_theta);

    } else {

        mTheta.reject_update();
    }
}



void Event::updateTheta_v4_mixing(const double tmin, // test avec mélange de densités
                                  const double tmax,
                                  const double rate_theta)
{
    for (auto&& date : mDates) {
        date.updateDate(mTheta.mX, mS02Theta.mX, mAShrinkage);
    }

    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);

    if (min >= max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3")
            .arg(getQStringName(), QString::number(min), QString::number(max));

    // ---------------------------
    // Calcul π = q1
    // ---------------------------
    double sum_p = 0.0;
    double sum_t = 0.0;

    for (auto&& date: mDates) {
        const double variance = pow(date.mSigmaTi.mX, 2.);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1.0 / variance;
    }

    const double ti_avg = sum_t / sum_p;
    const double sigma  = 1.0 / std::sqrt(sum_p);

    // ---------------------------
    // Paramètres mélange
    // ---------------------------
    constexpr double w1 = 0.33;  // locale
    constexpr double w2 = 0.33;  // calibration
    constexpr double w3 = 1 - w1 - w2;  // large

    constexpr double k  = 50.0;   // élargissement
    const double sigma_large = k * sigma;

    const double u = Generator::randomUniform();
    double theta_try = 0.0;

    // ---------------------------
    // Tirage mélange
    // ---------------------------
    if (u < w1) {
        // q1 : locale (≈ cible)
        theta_try = Generator::truncatedNormal(ti_avg, sigma, min, max);
    }
    else if (u < w1 + w2) {
        // q2 : calibration
        const double idx =
            interpolate_index(Generator::randomUniform(),
                              mMixingCalibrations->mRepartition);

        theta_try = mMixingCalibrations->mTmin
                    + idx * mMixingCalibrations->mStep;
    }
    else {
        // q3 : gaussienne large
        theta_try = Generator::truncatedNormal(ti_avg,
                                               sigma_large,
                                               min,
                                               max);
    }

    // ---------------------------
    // Vérification support
    // ---------------------------
    if (theta_try < min || theta_try > max) {
        mTheta.reject_update();
        return;
    }

    // ---------------------------
    // Densités pour MH
    // ---------------------------
    const double q1_new = dnorm(theta_try, ti_avg, sigma);
    const double q1_old = dnorm(mTheta.mX, ti_avg, sigma);

    const double q2_new = mMixingCalibrations->interpolate(theta_try);
    const double q2_old = mMixingCalibrations->interpolate(mTheta.mX);

    const double q3_new = dnorm(theta_try, ti_avg, sigma_large);
    const double q3_old = dnorm(mTheta.mX, ti_avg, sigma_large);

    const double q_new =
        w1*q1_new + w2*q2_new + w3*q3_new;

    const double q_old =
        w1*q1_old + w2*q2_old + w3*q3_old;

    // π = q1
    const double rate_P = q1_new / q1_old;
    const double rate_Q = q_old / q_new;

    const double rate = rate_P * rate_Q;

    mTheta.try_update(theta_try, rate * rate_theta);
}*/



/**
 * @brief Met à jour le paramètre \f$\theta\f$ d’un événement en mode *tempering*.
 *
 * Cette fonction implémente une étape de **tempering** (ou *mixing move*) qui ne
 * cherche pas à respecter exactement la distribution cible.
 * L’objectif est d’« secouer » la chaîne de Markov afin d’accélérer le
 * mélange et d’explorer plus largement l’espace des paramètres.
 * Aucun test de Metropolis‑Hastings n’est effectué ; il ne s’agit donc pas d’un
 * pas MCMC rigoureux.
 *
 * @details
 * 1.  Les bornes admissibles de \f$\theta\f$ sont calculées à partir de
 *     \p tmin et \p tmax via les méthodes \c getThetaMin() et \c getThetaMax().
 *     Si la borne inférieure est supérieure ou égale à la borne supérieure,
 *     une exception Qt est levée.
 *
 * 2.  Pour chaque date associée à l’événement :
 *     - les variables internes \c Delta, \c Ti et \c SigmaShrinkage_K sont
 *       actualisées de façon déterministe ;
 *     - la fonction \c updateWiggle() rafraîchit les variables dépendantes.
 *
 * 3.  Le poids moyen \f$\bar{t}_i\f$ et son incertitude \f$\sigma\f$ sont
 *     estimés à partir des variances \f$\sigma_{t_i}^2\f$.
 *
 * 4.  Le paramètre de température \p T est utilisé pour élargir l’écart‑type
 *     de la loi normale tronquée :
 *     \f[ \sigma_T = \sigma \times 2^{T} \f]
 *
 * 5.  Un nouveau \f$\theta\f$ est tiré d’une loi normale tronquée
 *     \f$\mathcal{N}(\bar{t}_i,\sigma_T^2)\f$ sur l’intervalle \f$[{\tt min},
 *     {\tt max}]\f$ puis stocké via \c mTheta.setValue().
 *
 * @param tmin  Borne inférieure du temps de référence (définit \c min).
 * @param tmax  Borne supérieure du temps de référence (définit \c max).
 * @param k     Niveau de *tempering* (entier ou réel) ; contrôle l’amplification
 *              de l’écart‑type (\f$2^{T}\f$). Plus \p T est grand, plus le tirage
 *              est large.
 *
 * @return Toujours \c true (la fonction ne retourne pas d’indicateur d’échec
 *         autre que l’exception décrite ci‑dessus).
 *
 * @throw QObject::tr Si \c min >= \c max, avec le message :
 *        « Error for event : %1 : min = %2 : max = %3 ».
 *
 * @note Cette mise à jour ne comporte aucun rejet ni aucune acceptation de type
 *       Metropolis‑Hastings ; elle est donc **non‑rigoureuse** du point de vue
 *       du MCMC classique, mais elle favorise le *mixing* de la chaîne.
 *
 * @warning L’appelant doit s’assurer que les vecteurs \c mDates et les
 *          paramètres internes (\c mTheta, \c mS02Theta, \c mAShrinkage) sont
 *          correctement initialisés avant l’invocation.
 *
 * @see Event::updateTheta_v3()               // mise à jour standard (sans tempering)
 * @see Generator::truncatedNormal()          // génération d’une loi normale tronquée
 * @see Event::getThetaMin(), Event::getThetaMax()
 *
 * @since 3.3.7
 */

void Event::applyTheta_v6_regenering_with_tempering(const double tmin,
                                                    const double tmax,
                                                    const double T)
{
    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);
    if (min >= max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3")
            .arg(getQStringName(), QString::number(min), QString::number(max));

    for (auto&& date : mDates) {

        const double u1 = Generator::randomUniform();

        const double tminCalib = date.mCalibration->mTmin;
        const double idx = interpolate_index(u1, date.mCalibration->mRepartition);
        double tiNew = tminCalib + idx * date.mCalibration->mStep;
        date.mTi.setValue(tiNew);

        date.updateDelta(mTheta.value()); // pas de memo

        date.applySigmaShrinkage_K_tempering(mTheta.value(), mS02Theta.value(), mAShrinkage, 1.0);

        date.updateWiggle(); // mise à jour déterministe, pas de tiragedate.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);

    }

    double sum_t = 0.0;
    double sum_p = 0.0;
    for (auto&& date : mDates) {
        const double var = pow(date.mSigmaTi.mX, 2.0);
        sum_t += (date.mTi.mX + date.mDelta) / var;
        sum_p += 1.0 / var;
    }
    const double ti_avg = sum_t / sum_p;
    const double sigma  = 1.0 / std::sqrt(sum_p);

    double sigma_T = schedule_exp(sigma, T); // Exponentiel doux (k > 0)

    //double sigma_T   = sigma * std::pow(2.0, T);
    double theta_try = Generator::truncatedNormal(ti_avg, sigma_T, min, max);
#ifdef DEBUG
    if (theta_try < min || max < theta_try) {
        std::cout << "[applyTheta_v6_Tempering] 🔄 " << mName
                  << " min = " << min << " max = " << max
                  << ", theta = " << theta_try
                  << ((min < theta_try && theta_try < max) ? "✅ YES" : "❌ NO")
                  << std::endl;
    }
#endif
    mTheta.setValue(theta_try);

}

void Event::applyTheta_v6_MH_Tempering(const double tmin,
                                       const double tmax,
                                       const double T)
{
    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);
    if (min >= max)
        throw QObject::tr("[Event::applyTheta_v6_MH_Tempering] Error for event : %1 : min = %2 : max = %3")
            .arg(getQStringName(), QString::number(min), QString::number(max));

    for (auto&& date : mDates) {

        const double u1 = Generator::randomUniform();

        const double tminCalib = date.mCalibration->mTmin;
        const double idx = interpolate_index(u1, date.mCalibration->mRepartition);
        double tiNew = tminCalib + idx * date.mCalibration->mStep;
        date.mTi.setValue(tiNew);

        date.updateDelta(mTheta.value()); // pas de memo

        date.applySigmaShrinkage_K_tempering(mTheta.value(), mS02Theta.value(), mAShrinkage, 1.0);

        date.updateWiggle(); // mise à jour déterministe, pas de tiragedate.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);

    }

    double sum_t = 0.0;
    double sum_p = 0.0;
    for (auto&& date : mDates) {
        const double var = pow(date.mSigmaTi.value(), 2.0);
        sum_t += (date.mTi.value() + date.mDelta) / var;
        sum_p += 1.0 / var;
    }
    const double ti_avg = sum_t / sum_p;
    const double sigma  = 1.0 / std::sqrt(sum_p);

    //double sigma_T = schedule_exp(sigma, T); // Exponentiel doux (k > 0)
    double sigma_T = schedule_exp_pow(sigma, T,  2.0); // a) Exponentiel décroit (α > 1)

    double theta_try = Generator::truncatedNormal(ti_avg, sigma_T, min, max);

    double log_alpha =
        (log_dnorm(theta_try, ti_avg, sigma)
         - log_dnorm(mTheta.value(), ti_avg, sigma)) / T;


#ifdef DEBUG
    if (theta_try < min || max < theta_try) {

        std::cout << "[applyTheta_v6_MH_Tempering] 🔄 " << mName
                  << " min = " << min << " max = " << max
                  << ", theta = " << theta_try
                  << ((min < theta_try && theta_try < max) ? "✅ YES" : "❌ NO")
                  << std::endl;
    }
    //std::cout << "[applyTheta_v6_MH_Tempering] 🔄 " << mName
      //        << " rate = " << exp(log_alpha)
        //      << std::endl;
#endif

    if (MHAcceptanceTest_log(log_alpha))
        mTheta.setValue(theta_try);

   // return true;
}



// mauvais nom updateTheta_v3, ne fait qu'un tirage avec la courbe de répartition
// et n'enregistre pas le tirage, juste une affectation
void Event::applyThetaProposal_v3(const double tmin, const double tmax)
{
    for (auto&& date : mDates )   {
        const double u1 = Generator::randomUniform();
        double tiNew;

        const double tminCalib = date.mCalibration->mTmin;

        if (u1 <  date.mMixingLevel) { // tiNew always in the study period
            const double idx = interpolate_index(u1, date.mCalibration->mRepartition);
            tiNew = tminCalib + idx * date.mCalibration->mStep;

        } else {
            // -- gaussian
            const double t0 = date.mTi.mX;
            const double s = (tmax - tmin) / 2.0;

            tiNew = Generator::normalDistribution(t0, s);
        }

        const double rate_1 = date.getLikelihood(tiNew) / date.getLikelihood(date.mTi.mX);

        const double rate_2 = exp((-0.5 / (date.mSigmaTi.mX * date.mSigmaTi.mX)) *
                                  (pow(tiNew - (mTheta.value() - date.mDelta), 2) -
                                   pow(date.mTi.mX - (mTheta.value() - date.mDelta), 2))
                                  );

        const double rate_3 = date.fProposalDensity(date.mTi.mX, tiNew) / date.fProposalDensity(tiNew, date.mTi.mX);

        double rate = rate_1 * rate_2 * rate_3;
        if (MHAcceptanceTest(rate)) {
            date.mTi.setValue(tiNew);
        }


        date.updateDelta(mTheta.value()); // pas de memo

        date.applySigmaShrinkage_K_tempering(mTheta.value(), mS02Theta.value(), mAShrinkage, 1.0);

        date.updateWiggle(); // mise à jour déterministe, pas de tiragedate.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);

    }

    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);

    if (min > max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(getQStringName(), QString::number(min), QString::number(max));

    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------

    double sum_p = 0.0;
    double sum_t = 0.0;

    for (auto&& date: mDates) {
        const double variance  = pow(date.mSigmaTi.mX, 2.);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1. / variance;
    }
    const double ti_avg = sum_t / sum_p;
    const double sigma = 1.0 / sqrt(sum_p);

    if (min == max) {
        double theta_try = min;
        mTheta.setValue(theta_try);

    } else {
        switch(mTheta.mSamplerProposal)
        {
        case MHVariable::eDoubleExp:
        {
            try {
                double theta_try = Generator::gaussByDoubleExp(ti_avg, sigma, min, max);
                mTheta.setValue(theta_try);

            }
            catch(QString error) {
                throw QObject::tr("Error for event : %1 : %2").arg(getQStringName(), error);
            }
            break;
        }

            // Event Prior
        case MHVariable::eBoxMuller:
        {
            double theta_try = Generator::truncatedNormal(ti_avg, sigma, min, max);
            mTheta.setValue(theta_try);
            break;
        }

        case MHVariable::eMHAdaptGauss:
        {
            // MH: The only case where the acceptance rate makes sense, since we use sigma MH :
            double theta_try = Generator::normalDistribution(mTheta.mX, mTheta.mSigmaMH);
            double rate = 0.0;
            if (theta_try >= min && theta_try <= max) {
                double diff1 = theta_try - ti_avg;
                double diff2 = mTheta.mX - ti_avg;
                rate = std::exp(-0.5 * (diff1*diff1 - diff2*diff2) / (sigma*sigma));

            }
#ifdef DEBUG
           /* if (theta_try <min  || max < theta_try) {
                std::cout << "[applyThetaProposal_v3] 🔄 " << mName
                          << " min = " << min << " max = " << max
                          << ", theta = " << theta_try
                          << ((min < theta_try && theta_try < max) ? "✅ YES" : "❌ NO")
                          << std::endl;
            }*/
#endif
            // test MH
            const bool accepted = MHAcceptanceTest(rate);

            if (accepted)
                mTheta.setValue(theta_try);

            break;
        }

        default:
            break;
        }
    }


}

void Event::applyThetaPriorCDE(const double tmin, const double tmax)
{
    for (auto&& date : mDates )   {
        const double u1 = Generator::randomUniform();
        double tiNew;

        const double tminCalib = date.mCalibration->mTmin;

        if (u1 <  date.mMixingLevel) { // tiNew always in the study period
            const double idx = interpolate_index(u1, date.mCalibration->mRepartition);
            tiNew = tminCalib + idx * date.mCalibration->mStep;

        } else {
            // -- gaussian
            const double t0 = date.mTi.mX;
            const double s = (tmax - tmin) / 2.0;

            tiNew = Generator::normalDistribution(t0, s);
        }

        const double rate_1 = date.getLikelihood(tiNew) / date.getLikelihood(date.mTi.mX);

        const double rate_2 = exp((-0.5 / (date.mSigmaTi.mX * date.mSigmaTi.mX)) *
                                  (pow(tiNew - (mTheta.value() - date.mDelta), 2) -
                                   pow(date.mTi.mX - (mTheta.value() - date.mDelta), 2))
                                  );

        const double rate_3 = date.fProposalDensity(date.mTi.mX, tiNew) / date.fProposalDensity(tiNew, date.mTi.mX);

        double rate = rate_1 * rate_2 * rate_3;
        if (MHAcceptanceTest(rate)) {
            date.mTi.setValue(tiNew);
        }


        date.updateDelta(mTheta.value()); // pas de memo

        date.applySigmaShrinkage_K_tempering(mTheta.value(), mS02Theta.value(), mAShrinkage, 1.0);

        date.updateWiggle(); // mise à jour déterministe, pas de tiragedate.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);

    }

    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);

    if (min > max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(getQStringName(), QString::number(min), QString::number(max));

    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------

    double sum_p = 0.0;
    double sum_t = 0.0;

    for (auto&& date: mDates) {
        const double variance  = pow(date.mSigmaTi.mX, 2.);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1. / variance;
    }
    const double ti_avg = sum_t / sum_p;
    const double sigma = 1.0 / sqrt(sum_p);

    if (min == max) {
        double theta_try = min;
        mTheta.setValue(theta_try);

    } else {
        // Tirage d'une valeur candidate directement depuis l'a priori empirique
        const double theta_try = mTheta.sampleFromEmpiricalPrior(min, max);

        // Ratio MH : la proposition n'est pas symétrique → corriger par q(x|x')/q(x'|x)
        const double q_forward  = mTheta.evalEmpiricalPrior(theta_try);  // q(x'|x)
        const double q_backward = mTheta.evalEmpiricalPrior(mTheta.value()); // q(x|x')

        const double rate = (dnorm(theta_try, ti_avg, sigma) / dnorm(mTheta.value(), ti_avg, sigma))
                             * (q_backward / q_forward);                   // correction
            // test MH
        const bool accepted = MHAcceptanceTest(rate);

        if (accepted)
            mTheta.setValue(theta_try);

    }

}

void Event::updateThetaPriorCDE(const double tmin, const double tmax)
{
    /*for (auto&& date : mDates )   {
        date.updateDate(mTheta.mX, mS02Theta.mX, mAShrinkage);

    }*/
    for (auto&& date : mDates) {

        const double u1 = Generator::randomUniform();
        double tiNew;

        const double tminCalib = date.mCalibration->mTmin;

        if (u1 <  date.mMixingLevel) { // tiNew always in the study period
            const double idx = interpolate_index(u1, date.mCalibration->mRepartition);
            tiNew = tminCalib + idx * date.mCalibration->mStep;

        } else {
            // -- gaussian
            const double t0 = date.mTi.mX;
            const double s = (tmax - tmin) / 2.0;

            tiNew = Generator::normalDistribution(t0, s);
        }

        const double rate_1 = date.getLikelihood(tiNew) / date.getLikelihood(date.mTi.mX);

        const double rate_2 = exp((-0.5 / (date.mSigmaTi.mX * date.mSigmaTi.mX)) *
                                  (pow(tiNew - (mTheta.value() - date.mDelta), 2) -
                                   pow(date.mTi.mX - (mTheta.value() - date.mDelta), 2))
                                  );

        const double rate_3 = date.fProposalDensity(date.mTi.mX, tiNew) / date.fProposalDensity(tiNew, date.mTi.mX);

        double rate = rate_1 * rate_2 * rate_3;
        date.mTi.try_update(tiNew, rate);

        // ____

        date.updateDelta(mTheta.value()); // pas de memo
        // ______

        double V1 = date.mSigmaTi.value()*date.mSigmaTi.value();

        const double mu = pow(date.mTi.mX - (mTheta.value() - date.mDelta), 2.) * 0.5;


        //const double logV2 = Generator::truncatedNormal(log10(V1), mSigmaTi.mSigmaMH, logVMin, logVMax);
        //const double mu_centre = 2. * mu;
        //const double logV2 = log10(mu_centre) + Generator::normalDistribution(0, 5); // test
        //double V2 = pow(10, logV2);

        double V_try = date.mSigmaTi.sampleFromEmpiricalPrior(0, 100000);

        const double x1 = exp(-mu * (V1 - V_try) / (V1 * V_try));
        // Likelihood term
        //const double log_x1 =  -mu * (V1 - V2) / (V1 * V2);

        const double x2 = pow((mS02Theta.value() + V1) / (mS02Theta.value() + V_try), mAShrinkage + 1.0);// a priori shrinkage

        rate = x1 * sqrt(V1/V_try) * x2;// * V_try / V1 ; // (V2 / V1) est le jacobien!


        // Proposition : y* ~ empiricalPrior, x* = y*²
        //const double y_proposed  = sampleFromEmpiricalPrior(min, max);
        const double x_proposed  = V_try * V_try;

        // Valeur courante
        const double y_current   = std::sqrt(V1);

        // Densités de proposition dans l'espace de y
        const double q_forward   = date.mSigmaTi.evalEmpiricalPrior(V_try);  // q(y*|y)
        const double q_backward  = date.mSigmaTi.evalEmpiricalPrior(V1);   // q(y|y*)

        // Jacobiens : dx/dy = 2y
        const double jac_proposed = 2.0 * V_try;   // |dx*/dy*|
        const double jac_current  = 2.0 * V1;    // |dx/dy|

        //         L(x*)      prior(x*)     q(y|y*)         jac_current
        // α =  ────────── × ──────────── × ─────────── × ───────────────
        //         L(x)        prior(x)     q(y*|y)        jac_proposed
        //
        // Les jacobiens convertissent q de l'espace y vers l'espace x

        const double alpha = (rate)
                             * (date.mSigmaTi.evalEmpiricalPrior(x_proposed) / date.mSigmaTi.evalEmpiricalPrior(V1))
                             * (q_backward / q_forward)
                             * (jac_current / jac_proposed);



        date.mSigmaTi.try_update(sqrt(V_try), rate);

        //date.applySigmaShrinkage_K_tempering(mTheta.value(), mS02Theta.value(), mAShrinkage, 1);

        date.updateWiggle(); // mise à jour déterministe, pas de tirage date.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);

    }
    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);

    if (min > max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(getQStringName(), QString::number(min), QString::number(max));

    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------

    double sum_p = 0.0;
    double sum_t = 0.0;

    for (auto&& date: mDates) {
        const double variance  = pow(date.mSigmaTi.mX, 2.);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1. / variance;
    }
    const double ti_avg = sum_t / sum_p;
    const double sigma = 1.0 / sqrt(sum_p);

    if (min == max) {
        double theta_try = min;
        mTheta.accept_update(theta_try);

    } else {
        // Tirage d'une valeur candidate directement depuis l'a priori empirique
        const double theta_try = mTheta.sampleFromEmpiricalPrior(min, max);
        //std::cout << mTheta.mName << " " << theta_try << std::endl;

        // Ratio MH : la proposition n'est pas symétrique → corriger par q(x|x')/q(x'|x)
        const double q_forward  = mTheta.evalEmpiricalPrior(theta_try);  // q(x'|x)
        const double q_backward = mTheta.evalEmpiricalPrior(mTheta.value()); // q(x|x')

        const double rate = (dnorm(theta_try, ti_avg, sigma) / dnorm(mTheta.value(), ti_avg, sigma))
                            * (q_backward / q_forward);                   // correction
        // test MH
        mTheta.try_update(theta_try, rate);


    }

}
void Event::generateHistos(const std::vector<ChainSpecs> &chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    if (type() != Event::eBound)
        mTheta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);

    else {
        Bound* ek = dynamic_cast<Bound*>(this);
        // Nothing todo : this is just a Dirac !
        ek->mTheta.mFormatedHisto.clear();
        ek->mTheta.mChainsHistos.clear();

        ek->mTheta.mFormatedHisto.emplace(ek->mFixed, 1);
        // Generate fictifious chains
        for (size_t i =0 ;i<chains.size(); ++i)
            ek->mTheta.mChainsHistos.push_back(ek->mTheta.mFormatedHisto);
    }
}

void Event::updateW()
{
#ifdef DEBUG
    try {

        if ((mVg.mX + mSy * mSy) < 1e-20) {
            qDebug()<< "[Event::updateW] mVg.mX + mSy * mSy < 1e-20";
        }
#endif
        mW = 1.0 / (mVg.mX + mSy * mSy);

#ifdef DEBUG
        if (mW < 1e-20) {
            qDebug()<< "[Event::updateW] mW < 1e-20"<< mW;

        } else if (mW > 1e+20) {
            qDebug()<< "[Event::updateW] mW > 1e+20"<< mW;
        }


    }  catch (...) {
        qWarning() <<"[Event::updateW] mW = 0";
    }
#endif

}

void Event::updateS02Theta()
{
    try {
        const double logVMin = -100.0;
        const double logVMax = 100.0;

        const double logV2 = Generator::truncatedNormal(log10(mS02Theta.mX) , mS02Theta.mSigmaMH, logVMin, logVMax );
        const double V2 = pow(10.0, logV2);

        const double current_h = h_S02(mS02Theta.mX); // h_S02() comporte le jacobien!

        const double try_h = h_S02(V2);

        double rate = try_h / current_h;


        mS02Theta.try_update(V2, rate);

    }  catch (...) {
        qWarning() << "[Event::updateS02Theta] mW = 0";
    }

}

void Event::applyS02Theta()
{
    try {
        const double logVMin = -100.0;
        const double logVMax = 100.0;

        const double logV2 = Generator::truncatedNormal(log10(mS02Theta.mX) , mS02Theta.mSigmaMH, logVMin, logVMax );
        const double V2 = pow(10.0, logV2);

        const double current_h = h_S02(mS02Theta.mX); // h_S02() comporte le jacobien!

        const double try_h = h_S02(V2);

        double rate = try_h / current_h;

        if (MHAcceptanceTest(rate)) {
            mS02Theta.setValue(V2);
        }


    }  catch (...) {
        qWarning() << "[Event::applyS02Theta] Error";
    }

}

void Event::applyS02Theta(double T)
{
    try {
        const double logVMin = -100.0;
        const double logVMax = 100.0;

        const double logV2 = Generator::truncatedNormal(log10(mS02Theta.mX) , mS02Theta.mSigmaMH, logVMin, logVMax );
        const double V2 = pow(10.0, logV2);

        const double current_h = h_S02(mS02Theta.mX); // h_S02() comporte le jacobien!

        const double try_h = h_S02(V2);

        double log_rate = log(try_h) -log(current_h);


        //if (MHAcceptanceTest_log(log_rate/T)) {
        if (MHAcceptanceTest(exp(log_rate/T))) {
            mS02Theta.setValue(V2);
        }


    }  catch (...) {
        qWarning() << "[Event::applyS02Theta(double T)] Error";
    }

}

double Event::h_S02(const double S02)
{
     /* schoolbook algo*/
 /*   const double alpha = 1. ;

    const double beta = 1.004680139*(1 - exp(- 0.0000847244 * pow(sqrt_S02_harmonique, 2.373548593)));

    const double prior = pow(1. / S02, alpha) * expl(- beta / S02);

    const double a = 1. ;


    double prod_h = 1.;
    for (auto& d : mDates) {
        prod_h *= pow((S02/(S02 + pow(d.mSigmaTi.mX, 2))), a + 1.) / S02;
    }

    // memory leak and slower !! ??
    // const double prod_h (std::accumulate(mDates.begin(), mDates.end(), 1., [S02, a] (double prod, Date d){return prod * (pow((S02/(S02 + pow(d.mSigmaTi.mX, 2.))), a + 1.) / S02);}));

    return prior * prod_h;
 */

// Code optimization

    //const double beta = 1.004680139*(1 - exp(- 0.0000847244 * pow(sqrt_S02_harmonique, 2.373548593)));

   /* double h = exp(- mBetaS02 / S02) / S02;

    for (auto& d : mDates) {
        h *= pow((S02/(S02 + pow(d.mSigmaTi.mX, 2))), 2.0);
    }
   return std::move(h / pow(S02, mDates.size()));
*/

 //--
   const double S02_squared = S02 * S02;
   // 1. Initialisation simplifiée de h
   double h = exp(-mBetaS02 / S02);

   // 2. Précalcul du terme de division final pour l'inclure dans la boucle
   // L'expression finale est : h * (1 / (S02 * S02)^N)
   // On initie h = exp(-mBetaS02 / S02) * (1 / S02) * (1 / S02)^(N)
   // On peut inclure (1/S02) dans la boucle pour simplifier l'étape de post-calcul

   // On utilise S02_inv pour la division finale
   const double S02_inv = 1.0 / S02;
   h *= S02_inv; // h = exp(...) / S02

   for (auto& d : mDates) {
       // 3. Calcul de la variance individuelle au carré une seule fois
       const double sigmaTi_squared = d.mSigmaTi.mX * d.mSigmaTi.mX;

       // 4. Simplification : le terme (S02/(S02 + sigma^2))^2 est équivalent à :
       // (S02 * S02) / ((S02 + sigma^2) * (S02 + sigma^2))

       // Calcul du dénominateur (S02 + sigma^2)^2
       const double denominator_sum = S02 + sigmaTi_squared;
       const double denominator_squared = denominator_sum * denominator_sum;

       // Multiplication par le terme complet (S02^2 / denominator_squared)
       h *= S02_squared / denominator_squared;

       // 5. Ajout de la division finale (1 / S02^N) en multipliant par 1/S02
       // à chaque itération (1 / S02)^N = (1 / S02) * (1 / S02) ... N fois
       h *= S02_inv;
   }

   // 6. Pas de post-calcul nécessaire.
   return h;
}

