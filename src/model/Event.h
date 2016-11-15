#ifndef EVENT_H
#define EVENT_H

#include "MHVariable.h"
#include "Date.h"

#include "StateKeys.h"

#include <QMap>
#include <QColor>
#include <QJsonObject>


class Phase;
class EventConstraint;
class Date;

class Event
{
public:
    enum Type{
        eDefault = 0,
        eKnown = 1
    };
    enum Method{
        eFixe = -1, // use with Type==eKnown
        eDoubleExp = 0,
        eBoxMuller = 1,
        eMHAdaptGauss = 2,

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
    
    
    // 2 fonctions utilisées pendant le MCMC (mais pas l'init!) :
    double getThetaMin(double defaultValue);
    double getThetaMax(double defaultValue);
    
    
    // 2 fonctions utilisées pour l'init du MCMC :
    double getThetaMinRecursive(const double defaultValue,
                                const QVector<QVector<Event*> >& eventBranches,
                                const QVector<QVector<Phase*> >& phaseBranches);

    double getThetaMaxRecursive(const double defaultValue,
                                const QVector<QVector<Event*> >& eventBranches,
                                const QVector<QVector<Phase*> >& phaseBranches);
    
    virtual void updateTheta(const double min, const double max);

    void generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax);
    
public:
    Type mType;
    int mId;
    
    QString mName; //must be public, to be setting by dialogbox
    QColor mColor;

    Method mMethod;
    
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
    
    double mMixingLevel;
};

#endif
