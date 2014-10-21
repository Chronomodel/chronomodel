#ifndef EVENT_KNOWN_H
#define EVENT_KNOWN_H

#include "Event.h"

#define STATE_EVENT_KNOWN_TYPE "known_type"
#define STATE_EVENT_KNOWN_FIXED "known_fixed"
#define STATE_EVENT_KNOWN_START "known_unif_start"
#define STATE_EVENT_KNOWN_END "known_unif_end"
#define STATE_EVENT_KNOWN_MEASURE "known_gauss_measure"
#define STATE_EVENT_KNOWN_ERROR "known_gauss_error"


class EventKnown: public Event
{
public:
    enum KnownType{
        eFixed = 0,
        eUniform = 1,
        eGauss = 2
    };
    
    EventKnown();
    virtual ~EventKnown();
    
    static EventKnown fromJson(const QJsonObject& json);
    virtual QJsonObject toJson() const;
    
    void setKnownType(KnownType type);
    void setFixedValue(const float& value);
    void setUniformStart(const float& value);
    void setUniformEnd(const float& value);
    void setGaussMeasure(const float& value);
    void setGaussError(const float& value);
    
    KnownType knownType() const;
    float fixedValue() const;
    float uniformStart() const;
    float uniformEnd() const;
    float gaussMeasure() const;
    float gaussError() const;
    
    void updateValues(float tmin, float tmax, float step);
    
    virtual void updateTheta(float min, float max);
    
public:
    KnownType mKnownType;
    
    float mFixed;
    
    float mUniformStart;
    float mUniformEnd;
    
    float mGaussMeasure;
    float mGaussError;
    
    QMap<float, float> mValues;
    
    QPixmap mThumb;
};

#endif
