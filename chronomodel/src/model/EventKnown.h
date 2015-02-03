#ifndef EVENT_KNOWN_H
#define EVENT_KNOWN_H

#include "Event.h"


class EventKnown: public Event
{
public:
    enum KnownType{
        eFixed = 0,
        eUniform = 1
    };
    
    EventKnown();
    virtual ~EventKnown();
    
    static EventKnown fromJson(const QJsonObject& json);
    virtual QJsonObject toJson() const;
    
    void setKnownType(KnownType type);
    void setFixedValue(const double& value);
    void setUniformStart(const double& value);
    void setUniformEnd(const double& value);
    
    KnownType knownType() const;
    double fixedValue() const;
    double uniformStart() const;
    double uniformEnd() const;
    
    void updateValues(double tmin, double tmax, double step);
    
    virtual void updateTheta(double min, double max);
    
public:
    KnownType mKnownType;
    
    double mFixed;
    
    double mUniformStart;
    double mUniformEnd;
    
    QMap<double, double> mValues;
};

#endif
