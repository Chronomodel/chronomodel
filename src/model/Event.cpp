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
    _name ("no Event Name")
{
    mTheta.setName(std::string("Theta of Event : ") + _name);
    mTheta.mSupport = MetropolisVariable::eBounded;
    mTheta.mFormat = DateUtils::eUnknown;
    mTheta.mSamplerProposal = MHVariable::eDoubleExp;

    mS02Theta.setName(std::string("S02Theta of Event : ") + _name);
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
    mVg.setName("Vg of Event : " + _name);
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

    mTheta.setName(std::string("Theta of Event : ") + _name);
    mTheta.mSamplerProposal = MHVariable::SamplerProposal (json.value(STATE_EVENT_SAMPLER).toInt());
    mTheta.mSupport = MetropolisVariable::eBounded;
    mTheta.mFormat = DateUtils::eUnknown;
    mTheta.mSigmaMH = 1.;

    mVg.setName(std::string("VG of Event : ") + _name);
    mVg.mSupport = MetropolisVariable::eRpStar;
    mVg.mFormat = DateUtils::eNumeric;
    mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

    mS02Theta.setName(std::string("SO2Theta of Event : ") + _name);
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

    _name = origin._name;

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
    _name = origin._name;

    mColor = origin.mColor;

    mDates = origin.mDates;

    mThetaNode = origin.mThetaNode;

    return *this;
}



void Event::copyFrom(const Event& event)
{
    mType = event.mType;
    mId = event.mId;

    _name = event._name;
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
        _name = origin._name;

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
// defaultValue = min period
double Event::getThetaMinRecursive_v3(const double defaultValue, const std::vector<Event* > &startEvents)
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
double Event::getThetaMaxRecursive_v3(const double defaultValue, const std::vector<Event*> &startEvents)
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



double Event::getThetaMin(double defaultValue)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne min courante pour le tirage de theta
    // ------------------------------------------------------------------

    // Max des thetas des faits en contrainte directe antérieure, ils sont déjà initialiés
    double maxThetaBwd = defaultValue;
    for (const auto& constBwd : mConstraintsBwd) {
        maxThetaBwd = std::max(maxThetaBwd, constBwd->mEventFrom->mTheta.mX);
    }

    /* Le fait appartient à une ou plusieurs phases.
     * Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
     * Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
     * (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
     */
    double min3 = defaultValue;

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

}

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

    /* Le fait appartient à une ou plusieurs phases.
     * Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
     * Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
     * (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
     */
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

void Event::updateTheta_v3(const double tmin, const double tmax)
{
    for (auto&& date : mDates )   {
        date.updateDate(mTheta.mX, mS02Theta.mX, mAShrinkage);
    }

    // Hack Date = Event
    //mTheta.tryUpdate(mDates[0].mTi.mX, 1.);
    //return;
    // Fin Hack ------

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

    double sum_p = 0.;
    double sum_t = 0.;

    for (auto&& date: mDates) {
        const double variance  = pow(date.mSigmaTi.mX, 2.);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1. / variance;
    }
    const double theta_avg = sum_t / sum_p;
    const double sigma = 1. / sqrt(sum_p);

    if (min == max) {
        const double theta = min;
        mTheta.accept_update(theta);

    } else {
        switch(mTheta.mSamplerProposal)
        {
        case MHVariable::eDoubleExp:
        {
            try {
                const double theta = Generator::gaussByDoubleExp(theta_avg, sigma, min, max);
                mTheta.accept_update(theta);

            }
            catch(QString error) {
                throw QObject::tr("Error for event : %1 : %2").arg(getQStringName(), error);
            }
            break;
        }

            // Event Prior
        case MHVariable::eBoxMuller:
        {
            double theta;
            long long counter = 0.;
            do {
                theta = Generator::normalDistribution(theta_avg, sigma);
                ++counter;
                if (counter == 100000000)
                    throw QObject::tr("No MCMC solution could be found using event method %1 for event named %2 ( %3  trials done)").arg(MHVariable::getSamplerProposalText(mTheta.mSamplerProposal), getQStringName(), QString::number(counter));

            } while(theta < min || theta > max);

            mTheta.accept_update(theta);
            break;
        }

        case MHVariable::eMHAdaptGauss:
        {
            // MH: The only case where the acceptance rate makes sense, since we use sigma MH :
            const double try_theta = Generator::normalDistribution(mTheta.mX, mTheta.mSigmaMH);
            double rapport = 0.0;
            if (try_theta >= min && try_theta <= max)
                rapport = exp((-0.5/(sigma*sigma)) * (pow(try_theta - theta_avg, 2.) - pow(mTheta.mX - theta_avg, 2.)));

            mTheta.try_update(try_theta, rapport);
            break;
        }

        default:
            break;
        }
    }
    //qDebug() << "[Event::updateTheta_v3]-----------> Event update : " << getQStringName() << " : " << mTheta.mX << " between" << "[" << min << " ; " << max << "]";

}

/* obsolete
void Event::updateTheta_v4(const double tmin, const double tmax, const double rate_theta)
{
    double rapport;

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
    const double mu = 1.0;
    const double u = Generator::randomUniform();
    double sum_p = 0.;
    double sum_t = 0.;

    for (auto&& date: mDates) {
        const double variance  = pow(date.mSigmaTi.mX, 2.);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1.0 / variance;
    }
    const double ti_avg = sum_t / sum_p;
    const double sigma = 1.0 / sqrt(sum_p);

    double theta_try ;
    if (u > mu) { // Q1
        const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), mMixingCalibrations->mRepartition);
        theta_try  = mMixingCalibrations->mTmin + idx * mMixingCalibrations->mStep;

    } else { //Q2
        theta_try = Generator::gaussByDoubleExp(ti_avg, sigma, min, max);
    }

    // Calcul rapport MH


    if (theta_try >= min && theta_try <= max) {

        const double q2_old = mMixingCalibrations->interpolate(mTheta.mX);
        const double q2_new = mMixingCalibrations->interpolate(theta_try);

        const double q1_new = dnorm(theta_try, ti_avg, sigma);
        const double q1_old = dnorm(mTheta.mX, ti_avg, sigma);

        const double rapport_Q = (mu * q1_old + (1 - mu) * q2_old) / (mu * q1_new + (1 - mu) * q2_new);
        const double rapport_P = q1_new / q1_old;
        rapport = rapport_P * rapport_Q;

    } else {
        rapport = -1.;
    }

    mTheta.try_update(theta_try, rapport * rate_theta);
    //qDebug() << "[Event::updateTheta_v4]-----------> Event update : " << getQStringName() << " : " << mTheta.mX << " between" << "[" << min << " ; " << max << "]";
}
*/

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

void Event::updateS02()
{
    try {
        const double logVMin = -100.0;
        const double logVMax = 100.0;

        const double logV2 = Generator::normalDistribution(log10(mS02Theta.mX) , mS02Theta.mSigmaMH);
        const double V2 = pow(10.0, logV2);

        double rapport  = -1.0; // Force reject
        if (logV2 >= logVMin && logV2 <= logVMax) {

            const double current_h = h_S02(mS02Theta.mX); // h_S02() comporte le jacobien!

            const double try_h = h_S02(V2);

            rapport = (try_h / current_h);

        }
#ifdef DEBUG
        else {
            //       qDebug()<<"[Event::updateS02] rapport rejet";
        }
#endif

        mS02Theta.try_update(V2, rapport);

    }  catch (...) {
        qWarning() << "[Event::updateS02] mW = 0";
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

