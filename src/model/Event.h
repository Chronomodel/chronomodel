#ifndef EVENT_H
#define EVENT_H

#include "Date.h"
#include "MHVariable.h"
#include "StateKeys.h"

#include <QMap>
#include <QColor>

class Phase;
class EventConstraint;


class Event
{
public:
    enum Type{
        eDefault = 0,
        eKnown = 1
    };
    enum Method{
        eDoubleExp = 0,
        eBoxMuller = 1,
        eMHAdaptGauss = 2
    };

    Event();
    Event(const Event& event);
    Event& operator=(const Event& event);
    void copyFrom(const Event& event);
    virtual ~Event();
    
    static Event fromJson(const QJsonObject& json);
    virtual QJsonObject toJson() const;
    
    Type type() const;
    
    void reset();
    float getThetaMin(float defaultValue);
    float getThetaMax(float defaultValue);
    virtual void updateTheta(float min, float max);
    
    // TODO : when creating an event from JSON, phases have to exists already to be able set pointers to them.
    // void initPhases(const QList<Phases*>& phases);
    
public:
    Type mType;
    int mId;
    QString mName;
    Method mMethod;
    QColor mColor;
    
    float mItemX;
    float mItemY;
    
    bool mIsCurrent;
    bool mIsSelected;
    
    QList<Date> mDates;
    
    QList<int> mPhasesIds;
    QList<int> mConstraintsFwdIds;
    QList<int> mConstraintsBwdIds;
    
    QList<Phase*> mPhases;
    QList<EventConstraint*> mConstraintsFwd;
    QList<EventConstraint*> mConstraintsBwd;
    
    MHVariable mTheta;
    float mS02;
    float mAShrinkage;
    bool mInitialized;
    
    int mLevel; // used to init mcmc
};

#endif
