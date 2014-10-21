#include "EventKnown.h"
#include "StdUtilities.h"
#include "GraphView.h"
#include "Generator.h"
#include <QObject>


EventKnown::EventKnown():Event(),
mFixed(0),
mUniformStart(0),
mUniformEnd(0),
mGaussMeasure(0),
mGaussError(0)
{
    mType = eKnown;
}

EventKnown::~EventKnown()
{

}

EventKnown EventKnown::fromJson(const QJsonObject& json)
{
    EventKnown event;
    
    event.mType = (Type)json[STATE_EVENT_TYPE].toInt();
    event.mId = json[STATE_EVENT_ID].toInt();
    event.mName = json[STATE_EVENT_NAME].toString();
    event.mColor = QColor(json[STATE_EVENT_RED].toInt(),
                           json[STATE_EVENT_GREEN].toInt(),
                           json[STATE_EVENT_BLUE].toInt());
    event.mMethod = (Method)json[STATE_EVENT_METHOD].toInt();
    event.mItemX = json[STATE_EVENT_ITEM_X].toDouble();
    event.mItemY = json[STATE_EVENT_ITEM_Y].toDouble();
    event.mIsSelected = json[STATE_EVENT_IS_SELECTED].toBool();
    event.mIsCurrent = json[STATE_EVENT_IS_CURRENT].toBool();
    
    event.mKnownType = (EventKnown::KnownType)json[STATE_EVENT_KNOWN_TYPE].toInt();
    event.mFixed = json[STATE_EVENT_KNOWN_FIXED].toDouble();
    event.mUniformStart = json[STATE_EVENT_KNOWN_START].toDouble();
    event.mUniformEnd = json[STATE_EVENT_KNOWN_END].toDouble();
    event.mGaussMeasure = json[STATE_EVENT_KNOWN_MEASURE].toDouble();
    event.mGaussError = json[STATE_EVENT_KNOWN_ERROR].toDouble();
    
    return event;
}

QJsonObject EventKnown::toJson() const
{
    QJsonObject event;
    
    event[STATE_EVENT_TYPE] = mType;
    event[STATE_EVENT_ID] = mId;
    event[STATE_EVENT_NAME] = mName;
    event[STATE_EVENT_RED] = mColor.red();
    event[STATE_EVENT_GREEN] = mColor.green();
    event[STATE_EVENT_BLUE] = mColor.blue();
    event[STATE_EVENT_METHOD] = mMethod;
    event[STATE_EVENT_ITEM_X] = mItemX;
    event[STATE_EVENT_ITEM_Y] = mItemY;
    event[STATE_EVENT_IS_SELECTED] = mIsSelected;
    event[STATE_EVENT_IS_CURRENT] = mIsCurrent;
    
    event[STATE_EVENT_KNOWN_TYPE] = mKnownType;
    event[STATE_EVENT_KNOWN_FIXED] = mFixed;
    event[STATE_EVENT_KNOWN_START] = mUniformStart;
    event[STATE_EVENT_KNOWN_END] = mUniformEnd;
    event[STATE_EVENT_KNOWN_MEASURE] = mGaussMeasure;
    event[STATE_EVENT_KNOWN_ERROR] = mGaussError;
    
    return event;
}

void EventKnown::setKnownType(KnownType type) {mKnownType = type;}
void EventKnown::setFixedValue(const float& value) {mFixed = value;}
void EventKnown::setUniformStart(const float& value) {mUniformStart = value;}
void EventKnown::setUniformEnd(const float& value) {mUniformEnd = value;}
void EventKnown::setGaussMeasure(const float& value) {mGaussMeasure = value;}
void EventKnown::setGaussError(const float& value) {mGaussError = value;}

EventKnown::KnownType EventKnown::knownType() const {return mKnownType;}
float EventKnown::fixedValue() const {return mFixed;}
float EventKnown::uniformStart() const {return mUniformStart;}
float EventKnown::uniformEnd() const {return mUniformEnd;}
float EventKnown::gaussMeasure() const {return mGaussMeasure;}
float EventKnown::gaussError() const {return mGaussError;}

void EventKnown::updateValues(float tmin, float tmax, float step)
{
    mValues.clear();
    switch(mKnownType)
    {
        case eFixed:
        {
            for(int t=tmin; t<=tmax; t+=step)
            {
                float v = (t == mFixed) ? 1 : 0;
                mValues[t] = v;
            }
            break;
        }
        case eUniform:
        {
            if(mUniformStart < mUniformEnd)
            {
                for(int t=tmin; t<=tmax; t+=step)
                {
                    float v = (t > mUniformStart && t <= mUniformEnd) ? 1 / (mUniformEnd - mUniformStart) : 0;
                    mValues[t] = v;
                }
            }
            break;
        }
        case eGauss:
        {
            if(mGaussError != 0)
            {
                float factor1 = 1 / (mGaussError * sqrtf(2 * M_PI));
                float factor2 = -0.5f / (mGaussError * mGaussError);
                for(int t=tmin; t<=tmax; t+=step)
                {
                    float v = factor1 * expf(factor2 * (mGaussMeasure - t) * (mGaussMeasure - t));
                    mValues[t] = v;
                }
            }
            break;
        }
        default:
            break;
    }
    if(mValues.size() == 0)
    {
        for(int t=tmin; t<=tmax; t+=step)
            mValues[t] = 0.f;
    }
}

void EventKnown::updateTheta(float tmin, float tmax)
{
    float min = getMaxEventThetaBackward(tmin);
    float max = getMinEventThetaForward(tmax);
    
    float minPhases = getMaxAlphaPhases(tmin);
    float maxPhases = getMinBetaPhases(tmax);
    
    min = (minPhases > min) ? minPhases : min;
    max = (maxPhases < max) ? maxPhases : max;
    
    switch(mKnownType)
    {
        case eFixed:
        {
            mTheta.tryUpdate(mFixed, 1);
            break;
        }
        case eUniform:
        {
            min = (mUniformStart > min) ? mUniformStart : min;
            max = (mUniformEnd < max) ? mUniformEnd : max;
            float theta = max + Generator::randomUniform() * (max - min);
            mTheta.tryUpdate(theta, 1);
            break;
        }
        case eGauss:
        {
            float theta = Generator::gaussByDoubleExp(mGaussMeasure, mGaussError, min, max);
            float rapport = 0;
            if(theta >= min && theta <= max)
            {
                rapport = exp((-0.5/(mGaussError*mGaussError)) * (pow(theta - mGaussMeasure, 2) - pow(mTheta.mX - mGaussMeasure, 2)));
            }
            mTheta.tryUpdate(theta, rapport);
            break;
        }
        default:
            break;
    }
}

