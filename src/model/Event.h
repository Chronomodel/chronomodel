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
    
    double getThetaMin(double defaultValue);
    double getThetaMax(double defaultValue);
    
    double getThetaMinRecursive(double defaultValue,
                                const QVector<QVector<Event*>>& eventBranches,
                                const QVector<QVector<Phase*>>& phaseBranches);

    double getThetaMaxRecursive(double defaultValue,
                                const QVector<QVector<Event*>>& eventBranches,
                                const QVector<QVector<Phase*>>& phaseBranches);
    
    virtual void updateTheta(double min, double max);
    
public:
    Type mType;
    int mId;
    QString mName;
    Method mMethod;
    QColor mColor;
    
    double mItemX;
    double mItemY;
    
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
    double mS02;
    double mAShrinkage;
    bool mInitialized;
    
    int mLevel; // used to init mcmc
};

#endif
