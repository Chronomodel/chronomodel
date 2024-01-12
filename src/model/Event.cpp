/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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
#include "Model.h"
#include "Date.h"
#include "Phase.h"

#include "EventConstraint.h"
#include "PhaseConstraint.h"
#include "Generator.h"
#include "Bound.h"
#include "QtUtilities.h"

#include <QString>
#include <QJsonArray>
#include <QObject>
#include <QDebug>
#include <QJsonObject>


//Event::Event(const Model *model):
Event::Event(std::shared_ptr<Model> model):
    mType (eDefault),
    mId (0),
    //mModel (model),
    mName ("no Event Name"),
    mIsCurrent (false),
    mIsSelected (false),
    mBetaS02(0.),
    mInitialized (false),
    mLevel (0),
    mPointType (ePoint),
    mMixingCalibrations (nullptr)
{
    mModel = model;
    mTheta.mSupport = MetropolisVariable::eBounded;
    mTheta.mFormat = DateUtils::eUnknown;
    mTheta.mSamplerProposal = MHVariable::eDoubleExp;

    mS02Theta.mSupport = MetropolisVariable::eRpStar;
    mS02Theta.mFormat = DateUtils::eNumeric;

#ifdef S02_BAYESIAN
    mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;
#else
    mS02Theta.mSamplerProposal = MHVariable::eFixe;
#endif

    // Item initial position :
    mItemX = 0.;
    mItemY = 0.;

    // Note : setting an event in (0, 0) tells the scene that this item is new!
    // Thus the scene will move it randomly around the currently viewed center point.
    // --------------------------------------------------------
    //  Curve
    // --------------------------------------------------------

    // Valeurs entrées par l'utilisateur
    mXIncDepth = 0.;
    mYDec = 0.;
    mZField = 0.;

    mS_XA95Depth = 0.;
    mS_Y = 0.;
    mS_ZField = 0.;

    // Valeurs préparées (projetées)
    mYx = 0.;
    mYy = 0.;
    mYz = 0.;

    // Valeurs utilisée pour les calculs
    mThetaReduced = 0.;
    mSy = 0.;
    mW = 0.;

   // MHVariable mVg;
    mVg.mSupport = MetropolisVariable::eRpStar;
    mVg.mFormat = DateUtils::eNumeric;
    mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

}

//Event::Event (const QJsonObject& json, const Model *model):
Event::Event (const QJsonObject& json, std::shared_ptr<Model> model):
   // mModel (model),
    mMixingCalibrations (nullptr)
{
    mModel = model;
    mType = Type (json.value(STATE_EVENT_TYPE).toInt());
    mId = json.value(STATE_ID).toInt();
    mName = json.value(STATE_NAME).toString();
    mColor = QColor(json.value(STATE_COLOR_RED).toInt(),
                          json.value(STATE_COLOR_GREEN).toInt(),
                          json.value(STATE_COLOR_BLUE).toInt());
    //event.mMethod = Method (json.value(STATE_EVENT_METHOD).toInt());
    mItemX = json.value(STATE_ITEM_X).toDouble();
    mItemY = json.value(STATE_ITEM_Y).toDouble();
    mIsSelected = json.value(STATE_IS_SELECTED).toBool();
    mIsCurrent = json.value(STATE_IS_CURRENT).toBool();

    mTheta.mSamplerProposal = MHVariable::SamplerProposal (json.value(STATE_EVENT_SAMPLER).toInt());
    mTheta.setName("Theta of Event : " + mName);
    mTheta.mSupport = MetropolisVariable::eBounded;
    mTheta.mFormat = DateUtils::eUnknown;
    mTheta.mSigmaMH = 1.;

    mVg.setName("VG of Event : " + mName);
    mVg.mSupport = MetropolisVariable::eRpStar;
    mVg.mFormat = DateUtils::eNumeric;
    mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

    mS02Theta.setName("SO2Theta of Event : " + mName);
    mS02Theta.mSupport = MetropolisVariable::eRpStar;
    mS02Theta.mFormat = DateUtils::eNumeric;
#ifdef S02_BAYESIAN
    mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;
#else
    mS02Theta.mSamplerProposal = MHVariable::eFixe;
#endif

    mPhasesIds = stringListToIntList(json.value(STATE_EVENT_PHASE_IDS).toString());

    mPointType = PointType (json.value(STATE_EVENT_POINT_TYPE).toInt());
    mXIncDepth = json.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    mYDec = json.value(STATE_EVENT_Y_DEC).toDouble();
    mZField = json.value(STATE_EVENT_Z_F).toDouble();

    mS_XA95Depth = json.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    mS_Y = json.value(STATE_EVENT_SY).toDouble();
    mS_ZField = json.value(STATE_EVENT_SZ_SF).toDouble();

    const QJsonArray dates = json.value(STATE_EVENT_DATES).toArray();
    for (auto&& date : dates) {
        Date dat (date.toObject(), this);
        dat.autoSetTiSampler(true); // must be after fromJson()

        if (!dat.isNull())
            mDates.append(dat);
        else
            throw QObject::tr("ERROR : data could not be created with plugin %1").arg(date.toObject().value(STATE_DATE_PLUGIN_ID).toString());
    }

}

/**
 * @todo Check the copy of the color if mJson is not set
 */
void Event::copyFrom(const Event& event)
{
    mType = event.mType;
    mId = event.mId;
    mName = event.mName;
    //mMethod = event.mMethod;
    mColor = event.mColor;
    mLevel = event.mLevel;

    mDates = event.mDates;
    mPhases = event.mPhases;
    mConstraintsFwd = event.mConstraintsFwd;
    mConstraintsBwd = event.mConstraintsBwd;

    mTheta = event.mTheta;
    mTheta.mSupport = event.mTheta.mSupport;
    mTheta.mFormat = event.mTheta.mFormat;
    mTheta.mSamplerProposal = event.mTheta.mSamplerProposal;
    mTheta.mSigmaMH = event.mTheta.mSigmaMH;

    mS02Theta.mX = event.mS02Theta.mX;
    mS02Theta.mSupport = event.mS02Theta.mSupport;
    mS02Theta.mFormat = event.mS02Theta.mFormat;
    mS02Theta.mSamplerProposal = event.mS02Theta.mSamplerProposal;
    mS02Theta.mSigmaMH = event.mS02Theta.mSigmaMH;

    mAShrinkage = event.mAShrinkage;
    mBetaS02 = event.mBetaS02;

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

    //mMixingLevel = event.mMixingLevel;
    
    // Valeurs entrées par l'utilisateur
    mXIncDepth = event.mXIncDepth;
    mYDec = event.mYDec;
    mZField = event.mZField;
    
    mS_XA95Depth = event.mS_XA95Depth;
    mS_Y = event.mS_Y;
    mS_ZField = event.mS_ZField;

    // Valeurs préparées (projetées)
    mYx = event.mYx;
    mYy = event.mYy;
    mYz = event.mYz;

    // Valeurs utilisée pour les calculs
    mThetaReduced = event.mThetaReduced;
    mSy = event.mSy;
    mW = event.mW;

    mVg = event.mVg;
    mVg.mSupport = event.mVg.mSupport;
    mVg.mFormat = event.mVg.mFormat;
    mVg.mSamplerProposal = event.mVg.mSamplerProposal;

}

Event::~Event()
{
    mTheta.reset();
    
    for (auto&& date : mDates) {
        date.mTi.reset();
        date.mSigmaTi.reset();
        date.mWiggle.reset();
    }

    mDates.clear();

    if (!mPhases.isEmpty())
        mPhases.clear();

    if (!mConstraintsFwd.isEmpty())
       mConstraintsFwd.clear();

    if (!mConstraintsBwd.isEmpty())
        mConstraintsBwd.clear();

    mVg.reset();
}


// JSON

Event Event::fromJson(const QJsonObject& json)
{
    Event event = Event();
    event.mType = Type (json.value(STATE_EVENT_TYPE).toInt());
    event.mId = json.value(STATE_ID).toInt();
    event.mName = json.value(STATE_NAME).toString();
    event.mColor = QColor(json.value(STATE_COLOR_RED).toInt(),
                          json.value(STATE_COLOR_GREEN).toInt(),
                          json.value(STATE_COLOR_BLUE).toInt());
    //event.mMethod = Method (json.value(STATE_EVENT_METHOD).toInt());
    event.mItemX = json.value(STATE_ITEM_X).toDouble();
    event.mItemY = json.value(STATE_ITEM_Y).toDouble();
    event.mIsSelected = json.value(STATE_IS_SELECTED).toBool();
    event.mIsCurrent = json.value(STATE_IS_CURRENT).toBool();

    event.mTheta.mSamplerProposal = MHVariable::SamplerProposal (json.value(STATE_EVENT_SAMPLER).toInt());
    event.mTheta.setName("Theta of Event : "+ event.mName);

    event.mS02Theta.setName("S02 of Event : "+ event.mName);
    event.mS02Theta.mSupport = MHVariable::eRpStar;
#ifdef S02_BAYESIAN
    event.mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;
#else
    event.mS02Theta.mSamplerProposal = MHVariable::eFixe;
#endif


    event.mPhasesIds = stringListToIntList(json.value(STATE_EVENT_PHASE_IDS).toString());

    event.mPointType = PointType (json.value(STATE_EVENT_POINT_TYPE).toInt());
    event.mXIncDepth = json.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    event.mYDec = json.value(STATE_EVENT_Y_DEC).toDouble();
    event.mZField = json.value(STATE_EVENT_Z_F).toDouble();
    
    event.mS_XA95Depth = json.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    event.mS_Y = json.value(STATE_EVENT_SY).toDouble();
    event.mS_ZField = json.value(STATE_EVENT_SZ_SF).toDouble();

    const QJsonArray dates = json.value(STATE_EVENT_DATES).toArray();
    //Date dat;
    for (auto&& date : dates) {
        Date dat;
        dat.fromJson(date.toObject());
        dat.autoSetTiSampler(true); // must be after fromJson()
        //dat.mMixingLevel = event.mMixingLevel;

        if (!dat.isNull())
            event.mDates.append(dat);
        else
            throw QObject::tr("ERROR : data could not be created with plugin %1").arg(date.toObject().value(STATE_DATE_PLUGIN_ID).toString());

    }

    return event;
}


QJsonObject Event::toJson() const
{
    QJsonObject event;

    event[STATE_EVENT_TYPE] = mType;
    event[STATE_ID] = mId;
    event[STATE_NAME] = mName;
    event[STATE_COLOR_RED] = mColor.red();
    event[STATE_COLOR_GREEN] = mColor.green();
    event[STATE_COLOR_BLUE] = mColor.blue();
    event[STATE_EVENT_SAMPLER] = mTheta.mSamplerProposal;
    event[STATE_EVENT_SAMPLER] = mTheta.mSamplerProposal;

    event[STATE_ITEM_X] = mItemX;
    event[STATE_ITEM_Y] = mItemY;
    event[STATE_IS_SELECTED] = mIsSelected;
    event[STATE_IS_CURRENT] = mIsCurrent;

    event[STATE_EVENT_POINT_TYPE] = mPointType;
    event[STATE_EVENT_X_INC_DEPTH] = mXIncDepth;
    event[STATE_EVENT_Y_DEC] = mYDec;
    event[STATE_EVENT_Z_F] = mZField;
    event[STATE_EVENT_SX_ALPHA95_SDEPTH] = mS_XA95Depth;
    event[STATE_EVENT_SY] = mS_Y;
    event[STATE_EVENT_SZ_SF] = mS_ZField;

    QString eventIdsStr;
    if (mPhasesIds.size() > 0) {
        QVector<QString> eventIds;
        for (auto &pId : mPhasesIds)
            eventIds.append(QString::number(pId));
        eventIdsStr = eventIds.join(",");
    }
    event[STATE_EVENT_PHASE_IDS] = eventIdsStr;

    QJsonArray dates;
    for (auto & d : mDates) {
        //QJsonObject date = d.toJson();
        dates.append(d.toJson());
    }
    event[STATE_EVENT_DATES] = dates;

    return event;
}

void Event::setCurveCsvDataToJsonEvent(QJsonObject& event, const QMap<QString, double>& CurveData)
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
    double xIncDepth = event.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    double s_XA95Depth = event.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    double yDec = event.value(STATE_EVENT_Y_DEC).toDouble();
    double s_Y = event.value(STATE_EVENT_SY).toDouble();
    double zField = event.value(STATE_EVENT_Z_F).toDouble();
    double s_ZField = event.value(STATE_EVENT_SZ_SF).toDouble();

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
void Event::reset()
{
    mTheta.reset();
    mVg.reset();
    mInitialized = false;
    mNodeInitialized = false;
    mThetaNode = HUGE_VAL;//__builtin_inf();//INFINITY;
}

bool Event::getThetaMinPossible(const Event* originEvent, QString& circularEventName, const QList<Event*> &startEvents, QString& linkStr)
{
    QList<Event*> newStartEvents = startEvents;
    newStartEvents.append(this);

    if (linkStr.isEmpty())
        linkStr = " ➡︎ ";

    QString parallelStr  (" | ");
    QString serieStr  (" ➡︎ ");

    QString startList;
    for (const Event* e : newStartEvents)
        startList += e->mName + linkStr;

    //qDebug() << mName << "startList" << startList;

    if (mNodeInitialized)
        return true;

    // list of phase under
    bool noPhaseBwd (true);
    if (!mPhases.isEmpty())
        for (const Phase* phase : mPhases)
            noPhaseBwd = noPhaseBwd && (phase->mConstraintsBwd.isEmpty());

    //--
    // L'Event appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela, puisqu'elle est échantillonnée ou fixée)




    if (noPhaseBwd && mConstraintsBwd.isEmpty()) {
        mNodeInitialized = true;
        return true;
    }

    else {
        // Check constraints in Events Scene
        if (!mConstraintsBwd.isEmpty())
            for (auto&& constBwd : mConstraintsBwd) {
                if (constBwd->mEventFrom != originEvent ) {
                     const bool maxThetaOk = (constBwd->mEventFrom)->getThetaMinPossible (originEvent, circularEventName, newStartEvents, serieStr);
                     if ( !maxThetaOk) {
                         circularEventName =  serieStr + constBwd->mEventFrom->mName +  circularEventName ;
                         return false;
                    }

                } else {
                    circularEventName = serieStr + constBwd->mEventFrom->mName + " ?";
                    return false;
                }
            }



        if (!noPhaseBwd) {
            // Check constraints in Phases Scene
            for (auto&& phase : mPhases) {
                if (!phase->mConstraintsBwd.isEmpty()) {
                    for (auto&& phaseBwd : phase->mConstraintsBwd) {
                        for (auto&& eventPhaseBwd : phaseBwd->mPhaseFrom->mEvents) {
                            if (eventPhaseBwd != originEvent ) {
                                const bool tMinRecOk = eventPhaseBwd->getThetaMinPossible ( originEvent, circularEventName, newStartEvents, serieStr);
                                if (!tMinRecOk) {
                                    circularEventName = serieStr + eventPhaseBwd->mName +  circularEventName ;
                                    return false;
                                }


                            } else {
                                circularEventName = serieStr + eventPhaseBwd->mName + " !";
                                //circularEventName = startList + eventPhaseBwd->mName + " ?";
                                return false;
                            }
                        }

                    }
                 }
            }
        }

        // Check parallel constraints with the Events in the same phases

        for (auto&& phase : mPhases) {
                for (auto&& event : phase->mEvents) {
                    if (!newStartEvents.contains (event)) {
                           const bool thetaOk = event->getThetaMinPossible (originEvent, circularEventName, newStartEvents, parallelStr);
                           if ( !thetaOk) {
                               circularEventName = parallelStr + event->mName +  circularEventName ;
                               return false;
                           }

                    }

                }
        }

        mNodeInitialized = true;
        return true;
    }


}

/**
 * @brief Event::getThetaMaxPossible
 * Vérifie si l'initialisation est possible, controle la circularité
 * @param originEvent
 * @param circularEventName
 * @param startEvents
 * @return
 */
bool Event::getThetaMaxPossible(const Event* originEvent, QString &circularEventName,  const QList<Event *> &startEvents)
{
#ifdef DEBUG
    QString startList;
    for (const Event* e : startEvents)
        startList += e->mName + "->";
#endif

    QList<Event*> newStartEvents = startEvents;
    newStartEvents.append(this);

   // const QString parallelStr  (" | ");
    const QString serieStr  (" ➡︎ ");

    if (mNodeInitialized)
        return true;

    // list of phase under
    bool noPhaseFwd (true);
    if (!mPhases.isEmpty())
        for (const auto& phase : mPhases)
            noPhaseFwd = noPhaseFwd && (phase->mConstraintsFwd.isEmpty());

    //--
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela, puisqu'elle est échantillonnée ou fixée)




    if (noPhaseFwd && mConstraintsFwd.isEmpty()) {
        mNodeInitialized = true;
        return true;
    }

    else {
        // Check constraints in Events Scene
        if (!mConstraintsFwd.isEmpty())
            for (const auto& constFwd : mConstraintsFwd) {
                if (constFwd->mEventTo != originEvent ) {
                //if (!newStartEvents.contains (constFwd->mEventTo)) {
                   // qDebug() << " mConstraintsFwd" << constFwd->mEventTo->mName;
                     const bool _ok = (constFwd->mEventTo)->getThetaMaxPossible (originEvent, circularEventName, newStartEvents);
                     if ( !_ok) {
                         circularEventName =  serieStr + constFwd->mEventTo->mName +  circularEventName ;
                         return false;
                    }

                } else {
                    circularEventName = serieStr + constFwd->mEventTo->mName + " ?";
                    return false;
                }
            }



        if (!noPhaseFwd) {
            // Check constraints in Phases Scene
            for (const auto& phase : mPhases) {
                if (!phase->mConstraintsFwd.isEmpty()) {
                    for (const auto& phaseFwd : phase->mConstraintsFwd) {

                        for (const auto& eventPhaseFwd : phaseFwd->mPhaseTo->mEvents) {
                            if (!newStartEvents.contains (eventPhaseFwd)) {
                                const bool _ok = eventPhaseFwd->getThetaMaxPossible ( originEvent, circularEventName, newStartEvents);
                                if (!_ok) {
                                    circularEventName = " (" + phase->mName + ")" + serieStr + eventPhaseFwd->mName + " (" + phaseFwd->mPhaseTo->mName + ")" +  circularEventName ;
                                    return false;
                                }


                            }
                            if (eventPhaseFwd == originEvent ) {
                                circularEventName = " (" + phase->mName + ")" + serieStr + eventPhaseFwd->mName + " (" + phaseFwd->mPhaseTo->mName + ") !";
                                return false;
                            }
                        }

                    }
                 }
            }
        }

        // Check parallel constraints with the Events in the same phases -> useless
        //Il ne faut pas regarder à travers les autres Events de la même phase
/*
        for (auto &&phase : mPhases) {
                for (auto &&event : phase->mEvents) {
                    if (!newStartEvents.contains (event)) {

                           const bool _ok = event->getThetaMaxPossible (originEvent, circularEventName, newStartEvents);
                           if ( !_ok) {
                               circularEventName = " (" + phase->mName + ")" + parallelStr + event->mName +  " (" + phase->mName + ")" + circularEventName ;
                               return false;
                           }

                    }


                }
        }
*/
        mNodeInitialized = true;
        return true;
    }
}


double Event::getThetaMinRecursive(const double defaultValue, const QList<Event*> &startEvents)
{
     //qDebug()<<"rentre dans getThetaMinRecursive()"<< mName;
    // if the Event is initiated, constraints was controled previously
    if (mInitialized) {
            return mTheta.mX;

    } else if (mNodeInitialized) {
        return mThetaNode;

    }
    /* else if (startEvents.contains(this)) {
        return defaultValue;
    }
    */
    // list of phase under
    bool noPhaseBwd = true;
    if (!mPhases.isEmpty())
        for (const auto& phase : mPhases) {
            noPhaseBwd = noPhaseBwd && (phase->mConstraintsBwd.isEmpty());
        }

    //--
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être au-dessus du plus grand theta de la phase moins la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    QList<Event*> newStartEvents = startEvents;
    newStartEvents.append(this);

    double maxPhases = defaultValue;
    for (const auto& phase : mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMax = defaultValue;
            for (auto&& event : phase->mEvents) {

                // On recherche la valeur de theta la plus grande de la phase et on soustrait Tau, il ne faut pas faire de récursif
                // On ne tient pas compte des theta non initialisés. Ils se mettront en place quand sera leur tour
                if (!startEvents.contains(event)) {
                   if (event->mInitialized) {
                       thetaMax = std::max(thetaMax, event-> mTheta.mX);

                   } else if (event->mNodeInitialized) {
                       thetaMax = std::max(thetaMax, event-> mThetaNode);

                   } /*else {
                       thetaMax = std::max(thetaMax, event->getThetaMinRecursive(defaultValue, newStartEvents));
                   }*/
                }
            }
            // Si aucune date initialisé maxPhase n'est pas évaluable donc égale à defaultValue
            if (thetaMax != defaultValue)
                maxPhases = std::max(maxPhases, thetaMax - phase->mTau.mX);

        }

    }


    if (noPhaseBwd && mConstraintsBwd.isEmpty()) {
        mNodeInitialized = true;
        mThetaNode = maxPhases;
        return mThetaNode;
    }

    else {
        double maxTheta = defaultValue;
        if (!mConstraintsBwd.isEmpty())
            for (auto&& constBwd : mConstraintsBwd) {
                if (!startEvents.contains(constBwd->mEventFrom)) {
                     maxTheta = std::max(maxTheta, (constBwd->mEventFrom)->getThetaMinRecursive(defaultValue, newStartEvents));
                    //qDebug()<<" thetaMin "<< mName <<"in constBwd"<<constBwd->mEventFrom->mName << maxTheta;
                }
            }

        double maxPhasesBwd = defaultValue;
        if (!noPhaseBwd) {

            for (auto&& phase : mPhases) {
                if (!phase->mConstraintsBwd.isEmpty()) {
                    double maxThetaBwd = defaultValue;
                    for (auto&& phaseBwd : phase->mConstraintsBwd) {

                        for (auto&& eventPhaseBwd : phaseBwd->mPhaseFrom->mEvents) {
                            if (!startEvents.contains(eventPhaseBwd)) {
                                maxThetaBwd = std::max(maxThetaBwd, eventPhaseBwd->getThetaMinRecursive(defaultValue, newStartEvents));
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
        mNodeInitialized = true;
       // mThetaNode = std::max(maxTheta, maxPhases);
        mThetaNode = std::max({maxTheta, maxPhases,  maxPhasesBwd});
        return mThetaNode;
    }


}

double Event::getThetaMaxRecursive(const double defaultValue, const QList<Event *> &startEvents)
{
    // if the Event is initialized, constraints was controled previously
    if (mInitialized) {
        return mTheta.mX;

    } else if (mNodeInitialized) {
        return mThetaNode;

    }
    /* else if (startEvents.contains(this)) {
        return defaultValue;
    }
    */


    // list of phase under
    bool noPhaseFwd = true;
    if (!mPhases.isEmpty()) {
        for (const auto& phase : mPhases) {
            noPhaseFwd = noPhaseFwd && (phase->mConstraintsFwd.isEmpty());
        }
    }

    //--
    // Le fait appartient à une ou plusieurs phases.
    // Si la phase à une contrainte de durée (!= Phase::eTauUnknown),
    // Il faut s'assurer d'être en-dessous du plus petit theta de la phase plus la durée
    // (on utilise la valeur courante de la durée pour cela puisqu'elle est échantillonnée)
    QList<Event*> newStartEvents = startEvents;
    newStartEvents.append(this);

    double minPhases = defaultValue;
    for (const auto& phase :mPhases) {
        if (phase->mTauType != Phase::eTauUnknown) {
            double thetaMin = defaultValue;
            for (const auto& event : phase->mEvents) {
                //if (!startEvents.contains(event)) {
                    //thetaMin = std::min(thetaMin, event->getThetaMaxRecursive(defaultValue, newStartEvents));
               // if (event != this) {
                if (!startEvents.contains(event)) {
                    if (event->mInitialized) {
                        thetaMin = std::min(thetaMin, event-> mTheta.mX);

                    } else if (event->mNodeInitialized) {
                        thetaMin = std::min(thetaMin, event-> mThetaNode);

                    } /* else {
                        thetaMin = std::min(thetaMin, event->getThetaMaxRecursive(defaultValue, newStartEvents));
                    } */

                }
            }
            // Si aucune date initialisé minPhase n'est pas évaluable donc égale à defaultValue
            if (thetaMin != defaultValue)
                minPhases = std::min(minPhases, thetaMin + phase->mTau.mX);
        }
     }

    if (noPhaseFwd && mConstraintsFwd.isEmpty()) {
            mNodeInitialized = true;
            mThetaNode = minPhases;
            return mThetaNode;
        }


    else {
        double minTheta = defaultValue;
        if (!mConstraintsFwd.isEmpty())
            for (const auto& constFwd : mConstraintsFwd) {
                if (!startEvents.contains(constFwd->mEventTo)) {
                    minTheta = std::min(minTheta, (constFwd->mEventTo)->getThetaMaxRecursive(defaultValue, newStartEvents));
                }
            }


        double minPhasesFwd = defaultValue;
        if (!noPhaseFwd) {

            for (const auto& phase : mPhases) {
                 if (!phase->mConstraintsFwd.isEmpty()) {
                    double minThetaFwd = defaultValue;
                    for (const auto& phaseFwd : phase->mConstraintsFwd) {

                        for (const auto& eventPhaseFwd : phaseFwd->mPhaseTo->mEvents) {
                            if (!newStartEvents.contains(eventPhaseFwd))
                                minThetaFwd = std::min(minThetaFwd, eventPhaseFwd->getThetaMaxRecursive(defaultValue, newStartEvents));
                        }

                        if (phaseFwd->mGammaType != PhaseConstraint::eGammaUnknown)
                            minPhasesFwd = std::min(minPhasesFwd, minThetaFwd - phaseFwd->mGamma);
                        else
                            minPhasesFwd = std::min(minPhasesFwd, minThetaFwd);
                    }
                  }
            }
        }
        mNodeInitialized = true;
        mThetaNode = std::min(minTheta, minPhases);
        mThetaNode = std::min(minPhasesFwd, mThetaNode);
        return mThetaNode;
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
            double thetaf (constBwd->mEventFrom->mTheta.mX);
            maxThetaBwd = std::max(maxThetaBwd, thetaf);
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
                if (event != this)
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

    double min_tmp1 = std::max(defaultValue, maxThetaBwd);
    double min_tmp2 = std::max(min3, maxPhasePrev);
    double min = std::max(min_tmp1, min_tmp2);

    return min;
}

double Event::getThetaMax(double defaultValue)
{
    // ------------------------------------------------------------------
    //  Déterminer la borne max
    // ------------------------------------------------------------------

    // Min des thetas des faits en contrainte directe et qui nous suivent
    double maxThetaFwd = defaultValue;
    for (const auto& cFwd : mConstraintsFwd) {
            maxThetaFwd = std::min(maxThetaFwd, cFwd->mEventTo->mTheta.mX);
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
                if (event != this)
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

    const double max_tmp2 = std::min(max3, maxPhaseNext);
    const double max = std::min(maxThetaFwd, max_tmp2);

    return max;
}

void Event::updateTheta_v3(const double tmin, const double tmax)
{
    for (auto&& date : mDates )   {
        date.updateDate(this);
    }

    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);
    //qDebug() << "----------->      in Event::updateTheta(): Event update : " << this->mName << " : " << this->mTheta.mX << " between" << "[" << min << " ; " << max << "]";

    if (min >= max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(mName, QString::number(min), QString::number(max));

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

    switch(mTheta.mSamplerProposal)
    {
        case MHVariable::eDoubleExp:
        {
            try{
                const double theta = Generator::gaussByDoubleExp(theta_avg, sigma, min, max);
                mTheta.tryUpdate(theta, 1.);

            }
            catch(QString error){
                throw QObject::tr("Error for event : %1 : %2").arg(mName, error);
            }
            break;
        }

        case MHVariable::eBoxMuller:
        {
            double theta;
            long long counter = 0.;
            do {
                theta = Generator::gaussByBoxMuller(theta_avg, sigma);
                ++counter;
                if (counter == 100000000)
                    throw QObject::tr("No MCMC solution could be found using event method %1 for event named %2 ( %3  trials done)").arg(MHVariable::getSamplerProposalText(mTheta.mSamplerProposal), mName, QString::number(counter));

            } while(theta < min || theta > max);

            mTheta.tryUpdate(theta, 1.);
            break;
        }

        case MHVariable::eMHAdaptGauss:
        {
            // MH : Seul cas où le taux d'acceptation a du sens car on utilise sigma MH :
            const double theta = Generator::gaussByBoxMuller(mTheta.mX, mTheta.mSigmaMH);
            double rapport = 0.;
            if (theta >= min && theta <= max)
                rapport = exp((-0.5/(sigma*sigma)) * (pow(theta - theta_avg, 2.) - pow(mTheta.mX - theta_avg, 2.)));

            mTheta.tryUpdate(theta, rapport);
            break;
        }

        default:
            break;
    }

}


void Event::updateTheta_v4(const double tmin, const double tmax, const double rate_theta)
{
    double rapport;

   for (auto&& date : mDates )   {
       date.updateDate(this);
   }

    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);
    //qDebug() << "----------->      in Event::updateTheta(): Event update : " << this->mName << " : " << this->mTheta.mX << " between" << "[" << min << " ; " << max << "]";

    if (min >= max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(mName, QString::number(min), QString::number(max));

    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------
    // tirage de theta
    const double mu = 1;
    const double u = Generator::randomUniform();
    double sum_p = 0.;
    double sum_t = 0.;

    for (auto&& date: mDates) {
        const double variance  = pow(date.mSigmaTi.mX, 2.);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1. / variance;
    }
    const double ti_avg = sum_t / sum_p;
    const double sigma = 1. / sqrt(sum_p);

    double theta_try ;
    if (u > mu) { // Q1

        const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), mMixingCalibrations->mRepartition);
        theta_try  = mMixingCalibrations->mTmin + idx * mMixingCalibrations->mStep;
//qDebug()<<"Mixing theta_try= "<< theta_try;

    } else { //Q2
        // -- gaussian

        theta_try = Generator::gaussByDoubleExp(ti_avg, sigma, min, max);
    }

    //qDebug()<<"theta_try="<< theta_try;

    // Calcul rapport MH


    if (theta_try >= min && theta_try <= max) {

        const double q2_old = mMixingCalibrations->interpolate(mTheta.mX);
        const double q2_new = mMixingCalibrations->interpolate(theta_try);

        const double q1_new = dnorm(theta_try, ti_avg, sigma);
        const double q1_old = dnorm(mTheta.mX, ti_avg, sigma);

        const double rapport_Q = (mu * q1_old + (1 - mu) * q2_old) / (mu * q1_new + (1 - mu) * q2_new);
        const double rapport_P = q1_new / q1_old;
        rapport = rapport_P * rapport_Q;
        //qDebug()<<"theta_try="<< theta_try<<"rate="<< rapport;

    } else {
        rapport = -1.;
    }
//qDebug()<<"theta_try="<< theta_try<<"rate="<< rapport;
    mTheta.tryUpdate(theta_try, rapport*rate_theta);


}


/*
void Event::updateTheta_v41(const double tmin, const double tmax, const double rate_theta)
{

    double rapport;

    for (auto&& date : mDates )   {
        date.updateDelta(this); // à faire avec xi
        date.updateWiggle();
    }

    // màj sigma
    for (auto&& date : mDates )   {
        date.updateSigma_v4(this);
    }


    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);
    //qDebug() << "----------->      in Event::updateTheta(): Event update : " << this->mName << " : " << this->mTheta.mX << " between" << "[" << min << " ; " << max << "]";

    if (min >= max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(mName, QString::number(min), QString::number(max));

    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------
    // tirage de theta
    const double mu = 0.999;
    const double u1 = Generator::randomUniform();
    const double t_mid = (mMixingCalibrations->mTmax + mMixingCalibrations->mTmin)/ 2.;
    const double s_p = (mMixingCalibrations->mTmax - mMixingCalibrations->mTmin)/ 2.;

    double theta_try ;
    if (u1 < mu) { // tiNew always in the study period
        theta_try  = sample_in_repartition(mMixingCalibrations, min, max);

    } else {
        // -- gaussian
        theta_try = Generator::gaussByBoxMuller(t_mid, s_p);
    }


    //const double theta_try  = Generator::gaussByBoxMuller(mTheta.mX, mTheta.mSigmaMH);

    qDebug()<<"theta_try="<< theta_try;
    double rapport_G = 1.;

    if (theta_try >= min && theta_try <= max) {
        for (auto&& date : mDates )   {
            auto ti1 = date.xi_current + theta_try;
            auto ti2 = date.xi_current + mTheta.mX;
            auto ri1 = date.mCalibration->interpolate(date.xi_current + theta_try);
            auto ri2 = date.mCalibration->interpolate(date.xi_current + mTheta.mX);
            //qDebug()<<"ti1="<< ti1<<" ri1="<<ri1<<"ti2="<<ti2<<"ri2="<< ri2;

            rapport_G *= date.mCalibration->interpolate(date.xi_current + theta_try) / date.mCalibration->interpolate(date.xi_current + mTheta.mX);
            //rapport *= date.getLikelihood(date.xi_current*date.mSigmaTi.mX + theta_try) / date.getLikelihood(date.xi_current*date.mSigmaTi.mX + mTheta.mX);
        }
        // si sample répartition
        auto i1 = mMixingCalibrations->interpolate(mTheta.mX);
        auto i2 = mMixingCalibrations->interpolate(theta_try);

        double q1_old = mMixingCalibrations->interpolate(mTheta.mX);
        double q1_new = mMixingCalibrations->interpolate(theta_try);

        const double q2_new =  dnorm(theta_try, t_mid, s_p);// exp(-0.5* pow((theta_try - t_mid)/ s_p, 2)) / (s_p*sqrt(2*M_PI));
        const double q2_old = dnorm(mTheta.mX, t_mid, s_p);//exp(-0.5* pow((mTheta.mX - t_mid)/ s_p, 2)) / (s_p*sqrt(2*M_PI));

        const double rapport_Q = (mu * q1_old + (1. - mu) * q2_old) / (mu * q1_new + (1. - mu) * q2_new);

        rapport = rapport_G * rapport_Q;
        qDebug()<<"theta_try="<< theta_try<<"rate="<< rapport;

    } else {
        rapport = -1.;
    }

    mTheta.tryUpdate(theta_try, rapport*rate_theta);

    for (auto&& date : mDates )   {
        //date.updateSigma_v4(this);
        date.updateDelta(this);
        date.updateWiggle();

        //date.updateTi_v4(this);
    }


    for (auto&& date : mDates )   {
        const double xi_try  = Generator::gaussByBoxMuller(0., 1.);
        const double ti_try = xi_try  + mTheta.mX - date.mDelta;

        const auto ti_revalued = date.xi_current  + mTheta.mX - date.mDelta;

        const double rapport_exp = dnorm(date.xi_current, 0., date.mSigmaTi.mX) / dnorm(xi_try, 0., date.mSigmaTi.mX);

        const double rapport = rapport_exp * date.mCalibration->interpolate(ti_try) / date.mCalibration->interpolate(ti_revalued);

        if (rapport < 1.) {
            const double uniform = Generator::randomUniform();
            date.xi_current = (uniform <= rapport)? xi_try : date.xi_current;
        }

        date.mTi.mX =  date.xi_current  + mTheta.mX - date.mDelta; //((date.mTi.mX + date.mDelta) - mTheta.mX ) / date.mSigmaTi.mX = xi;
        date.mTi.mLastAccepts.append(true);
        // date.updateTi_v4(this);
    }

}

void Event::updateTheta_v42(const double tmin, const double tmax, const double rate_theta)
{

    double rapport;


    double sum_p = 0.;
    double sum_t = 0.;

    for (auto&& date: mDates) {
        const double variance  = pow(date.mSigmaTi.mX, 2.);
        sum_t += (date.mTi.mX + date.mDelta) / variance;
        sum_p += 1. / variance;
    }
    const double theta_avg = sum_t / sum_p;
    const double sigma_avg = 1. / sqrt(sum_p);

    //const double x_event_current = (mTheta.mX - theta_avg)/sigma_avg;

    for (auto&& date : mDates )   {
        date.updateDelta(this); // à faire avec xi
        date.updateWiggle();
    }



    // -------------------------------------------------------------------------------------------------
    //  Evaluer theta.
    //  Le cas Wiggle est inclus ici car on utilise une formule générale.
    //  On est en "wiggle" si au moins une des mesures a un delta > 0.
    // -------------------------------------------------------------------------------------------------
    // tirage de theta

    const double min = getThetaMin(tmin);
    const double max = getThetaMax(tmax);
    //qDebug() << "----------->      in Event::updateTheta(): Event update : " << this->mName << " : " << this->mTheta.mX << " between" << "[" << min << " ; " << max << "]";

    if (min >= max)
        throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(mName, QString::number(min), QString::number(max));

    const double x_try  = Generator::gaussByBoxMuller(0., 1.);
    const double theta_try = x_try*sigma_avg + theta_avg;

    if (min <= theta_try && theta_try<= max) {
        rapport = 1.;

    } else {
        rapport =-1.;
    }


    mTheta.tryUpdate(theta_try, rapport*rate_theta);

    for (auto&& date : mDates )   {
        date.updateSigma_v4(this);
        date.updateDelta(this);
        date.updateWiggle();


        //double ti = sample_in_repartition(date.mCalibration, date.mCalibration->mTmin, date.mCalibration->mTmax);


        const double tiNew = Generator::gaussByBoxMuller(mTheta.mX - date.mDelta, date.mSigmaTi.mX);
        rapport = date.getLikelihood(tiNew) / date.getLikelihood(date.mTi.mX);
        rapport *= exp(-0.5 *(pow(tiNew - mTheta.mX, 2.) - pow(date.mTi.mX - mTheta.mX, 2.))/pow(date.mSigmaTi.mX, 2.));

        date.mTi.tryUpdate(tiNew, rapport);
        //date.mTi.mLastAccepts.append(true);
        //date.updateTi(this);

    }


}
*/

/**
 * @brief Event::generate_mixingCalibration  Creation of the cumulative date distribution
 */
/*
void Event::generate_mixingCalibration()
{
    if (mMixingCalibrations != nullptr) {
        mMixingCalibrations->~CalibrationCurve();
    }

    if (mDates.size() == 1) {
        mMixingCalibrations = new CalibrationCurve(*mDates.at(0).mCalibration);
        mMixingCalibrations->mDescription = QString("Mixing Calibrations of Event %1").arg(mName);
        mMixingCalibrations->mPlugin = nullptr;
        mMixingCalibrations->mPluginId = "";

    } else {

        mMixingCalibrations = new CalibrationCurve();
        mMixingCalibrations->mName = QString("Mixing Calibrations of Event %1").arg(mName);
        mMixingCalibrations->mDescription = QString("Mixing Calibrations of Event %1").arg(mName);
        mMixingCalibrations->mPlugin = nullptr;
        mMixingCalibrations->mPluginId = "";

        // 1 - Search for tmin and tmax, distribution curves, identical to the calibration.
        double unionTmin = +INFINITY;
        double unionTmax = -INFINITY;
        double unionStep = INFINITY;

        for (auto&& d : mDates) {
            if (d.mCalibration != nullptr && !d.mCalibration->mVector.isEmpty() ) {
                unionTmin = std::min(unionTmin, d.mCalibration->mTmin);
                unionTmax = std::max(unionTmax, d.mCalibration->mTmax);
                unionStep = std::min(unionStep, d.mCalibration->mStep);

            }

        }
        // 2 - Creation of the cumulative distribution curves in the interval

        mMixingCalibrations->mTmin = unionTmin;
        mMixingCalibrations->mTmax = unionTmax;
        mMixingCalibrations->mStep = unionStep;

        double t = unionTmin;
        long double sum = 0.;
        long double sum_old = 0.;
        const double n = mDates.size();
        while (t <= unionTmax) {
            sum= 0.;
            for (auto&& d : mDates) {
                sum += d.mCalibration->repartition_interpolate(t);
            }
            mMixingCalibrations->mVector.push_back((sum - sum_old)/(unionStep*n));
            mMixingCalibrations->mRepartition.push_back(sum/n);
            t += unionStep;
            sum_old = sum;
        }

        mMixingCalibrations->mMap = vector_to_map(mMixingCalibrations->mVector, unionTmin, unionTmax, unionStep);
    }

}
*/



void Event::generateHistos(const QList<ChainSpecs> &chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    if (type() != Event::eBound)
        mTheta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);

    else {
        Bound* ek = dynamic_cast<Bound*>(this);
        // Nothing todo : this is just a Dirac !
        ek->mTheta.mFormatedHisto.clear();
        ek->mTheta.mChainsHistos.clear();

        ek->mTheta.mFormatedHisto.insert(ek->mFixed,1);
        //generate fictifious chains
        for (int i =0 ;i<chains.size(); ++i)
            ek->mTheta.mChainsHistos.append(ek->mTheta.mFormatedHisto);
    }
}

void Event::updateW()
{
    try {
#ifdef DEBUG
        if ((mVg.mX + mSy * mSy) < 1e-20) {
            qDebug()<< "[Event::updateW] mVg.mX + mSy * mSy < 1e-20";
        }
#endif
        mW = 1. / (mVg.mX + mSy * mSy);

#ifdef DEBUG
        if (mW < 1e-20) {
            qDebug()<< "[Event::updateW] mW < 1e-20"<< mW;

        } else if (mW > 1e+20) {
            qDebug()<< "[Event::updateW] mW > 1e+20"<< mW;
        }
 #endif


    }  catch (...) {
        qWarning() <<"[Event::updateW] mW = 0";
    }

}

void Event::updateS02()
{
    try {
        const double logVMin = -100.;
        const double logVMax = 100.;

        const double logV2 = Generator::gaussByBoxMuller(log10(mS02Theta.mX) , mS02Theta.mSigmaMH);
        const double V2 = pow(10., logV2);

        double rapport  = -1.; // Force reject
        if (logV2 >= logVMin && logV2 <= logVMax) {

            const double current_h = h_S02(mS02Theta.mX);

            const double try_h = h_S02(V2);

            rapport = (try_h / current_h) ; // (V2 / mS02Theta.mX) ; // (V2 / V1) est le jacobien!

        }
#ifdef DEBUG
        else {
            //       qDebug()<<"[Event::updateS02] rapport rejet";
        }
#endif

        mS02Theta.tryUpdate(V2, rapport);
        //qDebug()<<"SO2 ="<< mS02Theta.mX<<" rapport = "<<rapport;
    }  catch (...) {
        qWarning() <<"[Event::updateS02] mW = 0";
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

    double h = exp(- mBetaS02 / S02) / S02;
    //qDebug()<<"mBetaS02="<<mName<< mBetaS02<<S02;

    for (auto& d : mDates) {
        h *= pow((S02/(S02 + pow(d.mSigmaTi.mX, 2))), 2.);
    }
   return std::move(h / pow(S02, mDates.size()));


}

std::vector<double> get_vector(const std::function<double (Event*)> &fun, const QList<Event*> &events)
{
    std::vector<double> vec (events.size());
    std::transform(events.begin(), events.end(), vec.begin(), fun);
    return vec;
}
