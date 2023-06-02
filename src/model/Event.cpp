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

Event::Event(const Model *model):
    mType (eDefault),
    mId (0),
    mModel (model),
    mName ("no Event Name"),
    mIsCurrent (false),
    //mMethod(Event::eDoubleExp),
    mIsSelected (false),
    mInitialized (false),
    mLevel (0),
    mPointType (ePoint)
{
    mTheta.mSupport = MetropolisVariable::eBounded;
    mTheta.mFormat = DateUtils::eUnknown;
    mTheta.mSamplerProposal = MHVariable::eDoubleExp;

    mS02.mSupport = MetropolisVariable::eRpStar;
    mS02.mFormat = DateUtils::eNumeric;

#ifdef S02_BAYESIAN
    mS02.mSamplerProposal = MHVariable::eMHAdaptGauss;
#else
    mS02.mSamplerProposal = MHVariable::eFixe;
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
    mVg.mSupport = MetropolisVariable::eRp;
    mVg.mFormat = DateUtils::eNumeric;
    mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

}
Event::Event (const QJsonObject& json, const Model *model):
    mModel (model)
{
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

    mVg.setName("VG of Event : " + mName);
    mVg.mSupport = MetropolisVariable::eRp;
    mVg.mFormat = DateUtils::eNumeric;
    mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

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

Event::Event(const Event& event)
{
    Event::copyFrom(event);
}

Event& Event::operator=(const Event& event)
{
    copyFrom(event);
    return *this;
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

    mS02.mX = event.mS02.mX;
    mS02.mSupport = event.mS02.mSupport;
    mS02.mFormat = event.mS02.mFormat;
    mS02.mSamplerProposal = event.mS02.mSamplerProposal;

    mAShrinkage = event.mAShrinkage;

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

    //event.mS02.mSamplerProposal = MHVariable::SamplerProposal (json.value(STATE_EVENT_SAMPLER).toInt());
    event.mS02.setName("S02 of Event : "+ event.mName);
    event.mS02.mSupport = MHVariable::eRpStar;

    event.mPhasesIds = stringListToIntList(json.value(STATE_EVENT_PHASE_IDS).toString());

    event.mPointType = PointType (json.value(STATE_EVENT_POINT_TYPE).toInt());
    event.mXIncDepth = json.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    event.mYDec = json.value(STATE_EVENT_Y_DEC).toDouble();
    event.mZField = json.value(STATE_EVENT_Z_F).toDouble();
    
    event.mS_XA95Depth = json.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    event.mS_Y = json.value(STATE_EVENT_SY).toDouble();
    event.mS_ZField = json.value(STATE_EVENT_SZ_SF).toDouble();

    const QJsonArray dates = json.value(STATE_EVENT_DATES).toArray();
    Date dat;
    for (auto&& date : dates) {
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
        //QStringList eventIds;
        QVector<QString> eventIds;
        for (int i=0; i<mPhasesIds.size(); ++i)
            eventIds.append(QString::number(mPhasesIds.at(i)));
        eventIdsStr = eventIds.join(",");
    }
    event[STATE_EVENT_PHASE_IDS] = eventIdsStr;

    QJsonArray dates;
    for (int i = 0; i<mDates.size(); ++i) {
        QJsonObject date = mDates.at(i).toJson();
        dates.append(date);
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

QString Event::curveDescriptionFromJsonEvent(QJsonObject &event, CurveSettings::ProcessType processType, CurveSettings::VariableType variableType )
{
    QString curveDescription = "";
    double xIncDepth = event.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    double s_XA95Depth = event.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    double yDec = event.value(STATE_EVENT_Y_DEC).toDouble();
    double s_Y = event.value(STATE_EVENT_SY).toDouble();
    double zField = event.value(STATE_EVENT_Z_F).toDouble();
    double s_ZField = event.value(STATE_EVENT_SZ_SF).toDouble();

    switch (processType) {
    case CurveSettings::eProcessTypeVector:
        curveDescription += QObject::tr(" [ Inclination : %1 ±  %2").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
        curveDescription += QObject::tr(" - Declination : %1").arg(stringForLocal(yDec));
        curveDescription += QObject::tr(" - Field : %1 ±  %2 ]").arg(stringForLocal(zField), stringForLocal(s_ZField));
        break;
    case CurveSettings::eProcessType2D:
        curveDescription += QObject::tr(" [ X : %1 ±  %2").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
        curveDescription += QObject::tr(" - Y : %1 ±  %2 ]").arg(stringForLocal(yDec), stringForLocal(s_Y));
        break;
    case CurveSettings::eProcessType3D:
        curveDescription += QObject::tr(" [ X : %1 ±  %2").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
        curveDescription += QObject::tr(" - Y : %1 ±  %2").arg(stringForLocal(yDec), stringForLocal(s_Y));
        curveDescription += QObject::tr(" - Z : %1 ±  %2 ]").arg(stringForLocal(zField), stringForLocal(s_ZField));
        break;
    case CurveSettings::eProcessTypeSpherical:
        curveDescription += QObject::tr(" [ Inclination : %1 ±  %2").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
        curveDescription += QObject::tr(" - Declination : %1 ]").arg(stringForLocal(yDec));
        break;
    default:
        switch (variableType) {
        case CurveSettings::eVariableTypeDepth:
            curveDescription += QObject::tr(" [ Depth : %1 ±  %2 ]").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            break;
        case CurveSettings::eVariableTypeField:
            curveDescription += line(textGreen(QObject::tr(" [ Field : %1 ±  %2 ]").arg(stringForLocal(zField), stringForLocal(s_ZField))));
            break;
        case CurveSettings::eVariableTypeInclination:
            curveDescription += QObject::tr(" [ Inclination : %1 ±  %2 ]").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            break;
        case CurveSettings::eVariableTypeDeclination:
            curveDescription += QObject::tr(" [ Declination : %1 ; Inclination %2 ±  %3 ]").arg(stringForLocal(yDec), stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            break;
        case CurveSettings::eVariableTypeOther:
            curveDescription += QObject::tr("[  Measure : %1 ±  %2 ]").arg(stringForLocal(xIncDepth), stringForLocal(s_XA95Depth));
            break;
        default:
            break;
        }

        break;
    }
    return curveDescription;
}

QList<double> Event::curveParametersFromJsonEvent(QJsonObject &event, CurveSettings::ProcessType processType, CurveSettings::VariableType variableType)
{
    QList<double> curveParameters;
    double xIncDepth = event.value(STATE_EVENT_X_INC_DEPTH).toDouble();
    double s_XA95Depth = event.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
    double yDec = event.value(STATE_EVENT_Y_DEC).toDouble();
    double s_Y = event.value(STATE_EVENT_SY).toDouble();
    double zField = event.value(STATE_EVENT_Z_F).toDouble();
    double s_ZField = event.value(STATE_EVENT_SZ_SF).toDouble();

    switch (processType) {
    case CurveSettings::eProcessTypeNone:
        return curveParameters;
        break;
    case CurveSettings::eProcessTypeVector:
        curveParameters.append({xIncDepth, s_XA95Depth, yDec, zField, s_ZField});
        break;
    case CurveSettings::eProcessType2D:
        curveParameters.append({xIncDepth, s_XA95Depth, yDec, s_Y});
        break;
    case CurveSettings::eProcessType3D:
        curveParameters.append({xIncDepth, s_XA95Depth, yDec, s_Y, zField, s_ZField});
        break;
    case CurveSettings::eProcessTypeSpherical:
        curveParameters.append({xIncDepth, s_XA95Depth, yDec});
        break;
    default:
        switch (variableType) {
        case CurveSettings::eVariableTypeDepth:
            curveParameters.append({xIncDepth, s_XA95Depth});
            break;
        case CurveSettings::eVariableTypeField:
            curveParameters.append({zField, s_ZField});
            break;
        case CurveSettings::eVariableTypeInclination:
            curveParameters.append({xIncDepth, s_XA95Depth});
            break;
        case CurveSettings::eVariableTypeDeclination:
            curveParameters.append({yDec, xIncDepth, s_XA95Depth});
            break;
        case CurveSettings::eVariableTypeOther:
            curveParameters.append({xIncDepth, s_XA95Depth});
            break;
        default:
            break;
        }

        break;
    }
    return curveParameters;
}

// Properties
Event::Type Event::type() const
{
    return mType;
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
    for (Event* e : newStartEvents)
        startList += e->mName + linkStr;

    //qDebug() << mName << "startList" << startList;

    if (mNodeInitialized)
        return true;

    // list of phase under
    bool noPhaseBwd (true);
    if (!mPhases.isEmpty())
        for (auto&& phase : mPhases)
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
                // On ne tient pas compte des theta non initailisés. Ils se mettront en place quand sera leur tour
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
        mThetaNode = std::max(maxTheta, maxPhases);
        mThetaNode = std::max(maxPhasesBwd, mThetaNode);
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

void Event::updateTheta(const double tmin, const double tmax)
{
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

void Event::generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
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
            qDebug()<< "[Event] updateW() mVg.mX + mSy * mSy < 1e-20";
        }
#endif
        mW = 1. / (mVg.mX + mSy * mSy);

#ifdef DEBUG
        if (mW < 1e-20) {
            qDebug()<< "[Event] updateW() mW < 1e-20"<< mW;

        } else if (mW > 1e+20) {
            qDebug()<< "[Event] updateW() mW > 1e+20"<< mW;
        }
 #endif


    }  catch (...) {
        qWarning() <<"[Event] updateW() mW = 0";
    }

}

void Event::updateS02()
{
    try {
        const double logVMin = -6.;
        const double logVMax = 100.;

        const double logV2 = Generator::gaussByBoxMuller(log10(mS02.mX) , mS02.mSigmaMH);
        const double V2 = pow(10, logV2);

        double rapport  = 0.;
        if (logV2 >= logVMin && logV2 <= logVMax) {
            const double current_h = h_S02(mS02.mX);
            const double try_h = h_S02(V2);
            rapport = (try_h / current_h) * (V2 / mS02.mX) ; // (V2 / V1) est le jacobien!

        }
#ifdef DEBUG
        else {
            //       qDebug()<<"void Event::updateS02() rapport rejet";
        }
#endif

        mS02.tryUpdate(V2, rapport);
       // qDebug()<<"SO2 ="<< mS02.mX<<" rapport = "<<rapport;
    }  catch (...) {
        qWarning() <<"S02() mW = 0";
    }

}


double Event::h_S02(const double S02)
{

    const double alp = 1. ;

    const double bet = 1. ;

    const double prior =   pow((1./S02), alp + 1) * exp(-bet/S02);
    const int a = 1 ;

    double prod_h = 1.;
    for (auto& d : mDates) {
        prod_h *= pow((S02/(S02 + pow(d.mSigmaTi.mX, 2))), a + 1) / S02;
    }

    return prior* prod_h;

}

