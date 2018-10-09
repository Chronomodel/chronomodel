#include "EventKnown.h"
#include "StdUtilities.h"
#include "GraphView.h"
#include "Generator.h"
#include <QObject>


EventKnown::EventKnown():Event(),
mFixed(0.)
{
    mType = eKnown;
    mMethod= eFixe;
    mTheta.mSigmaMH = 0.;
}

EventKnown::EventKnown(const QJsonObject& json):Event()
{
    mType = (Type)json[STATE_EVENT_TYPE].toInt();
    mId = json[STATE_ID].toInt();
    mName =  json[STATE_NAME].toString();
    mColor = QColor(json[STATE_COLOR_RED].toInt(),
                    json[STATE_COLOR_GREEN].toInt(),
                    json[STATE_COLOR_BLUE].toInt());
    mMethod = Event::eFixe;
    mItemX = json[STATE_ITEM_X].toDouble();
    mItemY = json[STATE_ITEM_Y].toDouble();
    mIsSelected = json[STATE_IS_SELECTED].toBool();
    mIsCurrent = json[STATE_IS_CURRENT].toBool();

    if (json.contains(STATE_EVENT_KNOWN_FIXED))
        mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();
    else
        mFixed = 0.;

    QString eventIdsStr = json[STATE_EVENT_PHASE_IDS].toString();
    if (!eventIdsStr.isEmpty()) {
        QStringList eventIds = eventIdsStr.split(",");
        for(auto &&evIds :eventIds)
            mPhasesIds.append(evIds.toInt());
    }
}


// JSON
// static function
EventKnown EventKnown::fromJson(const QJsonObject& json)
{
    EventKnown event;
    
    event.mType = (Type)json[STATE_EVENT_TYPE].toInt();
    event.mId = json[STATE_ID].toInt();
    event.mName =  json[STATE_NAME].toString();
    event.mColor = QColor(json[STATE_COLOR_RED].toInt(),
                           json[STATE_COLOR_GREEN].toInt(),
                           json[STATE_COLOR_BLUE].toInt());
    event.mMethod = Event::eFixe;
    event.mItemX = json[STATE_ITEM_X].toDouble();
    event.mItemY = json[STATE_ITEM_Y].toDouble();
    event.mIsSelected = json[STATE_IS_SELECTED].toBool();
    event.mIsCurrent = json[STATE_IS_CURRENT].toBool();
    
    if (json.contains(STATE_EVENT_KNOWN_FIXED))
        event.mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();
    else
        event.mFixed = 0.;
    
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

    event[STATE_EVENT_KNOWN_FIXED] = mFixed;
    
    return event;
}


void EventKnown::setFixedValue(const double& value) {mFixed = value;}

double EventKnown::fixedValue() const
{
    return mFixed;
}

double EventKnown::formatedFixedValue() const
{
    return DateUtils::convertToAppSettingsFormat(mFixed);
}


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

