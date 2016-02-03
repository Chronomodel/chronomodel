#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include <QJsonObject>
#include <QObject>
#include "StateKeys.h"


class Constraint: public QObject
{
public:
    Constraint(QObject* parent = 0);
    Constraint(const Constraint& ec, QObject* parent = 0);
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
