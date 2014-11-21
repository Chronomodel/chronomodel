#ifndef EVENT_KNOWN_H
#define EVENT_KNOWN_H

#include "Event.h"


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
};

#endif
