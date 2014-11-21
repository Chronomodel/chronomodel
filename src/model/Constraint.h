#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include <QJsonObject>
#include "StateKeys.h"


class Constraint
{
public:
    Constraint();
    Constraint(const Constraint& ec);
    Constraint& operator=(const Constraint& ec);
    virtual void copyFrom(const Constraint& ec);
    virtual ~Constraint();
    
    static Constraint fromJson(const QJsonObject& json);
    virtual QJsonObject toJson() const;
    
public:
    int mId;
    int mFromId;
    int mToId;
};

#endif
