#include "EventKnown.h"
#include "StdUtilities.h"
#include "GraphView.h"
#include "Generator.h"
#include <QObject>


EventKnown::EventKnown():Event(),
//mKnownType(eFixed),
mFixed(0.)
//mUniformStart(0.),
//mUniformEnd(0.)
{
    mType = eKnown;
    mMethod= eFixe;
    mTheta.mSigmaMH = 0.;
}

EventKnown::~EventKnown()
{

}

// JSON

EventKnown EventKnown::fromJson(const QJsonObject& json)
{
    EventKnown event;
    
    event.mType = (Type)json[STATE_EVENT_TYPE].toInt();
    event.mId = json[STATE_ID].toInt();
    event.mName =  json[STATE_NAME].toString();
    event.mColor = QColor(json[STATE_COLOR_RED].toInt(),
                           json[STATE_COLOR_GREEN].toInt(),
                           json[STATE_COLOR_BLUE].toInt());
    event.mMethod = Event::eFixe; //(Method)json[STATE_EVENT_METHOD].toInt();
    event.mItemX = json[STATE_ITEM_X].toDouble();
    event.mItemY = json[STATE_ITEM_Y].toDouble();
    event.mIsSelected = json[STATE_IS_SELECTED].toBool();
    event.mIsCurrent = json[STATE_IS_CURRENT].toBool();
    
    //event.mKnownType = (EventKnown::KnownType)json[STATE_EVENT_KNOWN_TYPE].toInt();
    if (json.contains(STATE_EVENT_KNOWN_FIXED))
        event.mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();
    else
        event.mFixed = 0.;
    //event.mUniformStart = json[STATE_EVENT_KNOWN_START].toDouble();
    //event.mUniformEnd = json[STATE_EVENT_KNOWN_END].toDouble();
    
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
    
    //event[STATE_EVENT_KNOWN_TYPE] = mKnownType;
    event[STATE_EVENT_KNOWN_FIXED] = mFixed;
    //event[STATE_EVENT_KNOWN_START] = mUniformStart;
    //event[STATE_EVENT_KNOWN_END] = mUniformEnd;
    
    return event;
}

//void EventKnown::setKnownType(KnownType type) {mKnownType = type;}
void EventKnown::setFixedValue(const double& value) {mFixed = value;}
//void EventKnown::setUniformStart(const double& value) {mUniformStart = value;}
//void EventKnown::setUniformEnd(const double& value) {mUniformEnd = value;}

//EventKnown::KnownType EventKnown::knownType() const {return mKnownType;}
double EventKnown::fixedValue() const {return mFixed;}
//double EventKnown::uniformStart() const {return mUniformStart;}
//double EventKnown::uniformEnd() const {return mUniformEnd;}

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

