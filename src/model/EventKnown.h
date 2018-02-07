#ifndef EVENT_KNOWN_H
#define EVENT_KNOWN_H

#include "Event.h"


class EventKnown: public Event
{
public:

    EventKnown();
    explicit EventKnown(const QJsonObject& json);
    virtual ~EventKnown();
    
    static EventKnown fromJson(const QJsonObject& json);
    virtual QJsonObject toJson() const;

    void setFixedValue(const double& value);
    double fixedValue() const;
    
    void updateValues(const double &tmin, const double &tmax, const double &step);
    
    virtual void updateTheta(const double &min, const double &max);
    
    
public:
    double mFixed;
    QMap<double, double> mValues;
};

#endif
