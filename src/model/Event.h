#ifndef EVENT_H
#define EVENT_H

#include "Date.h"
#include "MHVariable.h"
#include "StateKeys.h"

#include <QMap>
#include <QColor>
#include <QJsonObject>

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
    
    //Added by PhD
    void setModelJson(const QJsonObject & iModelJson, const int idxEvent);
    const QJsonObject* getModelJson();
    
    static Event fromJson(const QJsonObject& json, const int EventIdx);
    virtual QJsonObject toJson() const;
    
    Type type() const;
    
    void reset();
    
    
    // 2 fonctions utilisées pendant le MCMC (mais pas l'init!) :
    double getThetaMin(double defaultValue);
    double getThetaMax(double defaultValue);
    
    
    // 2 fonctions utilisées pour l'init du MCMC :
    double getThetaMinRecursive(double defaultValue,
                                const QVector<QVector<Event*> >& eventBranches,
                                const QVector<QVector<Phase*> >& phaseBranches);

    double getThetaMaxRecursive(double defaultValue,
                                const QVector<QVector<Event*> >& eventBranches,
                                const QVector<QVector<Phase*> >& phaseBranches);
    
    virtual void updateTheta(double min, double max);
    
    QColor getColor() const;
    QString getName() const;
    
public:
    Type mType;
    int mId;
    
    QString mInitName; //must be public, to be setting by dialogbox
    QColor mInitColor;

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

protected:
    
    
    const QJsonObject * mModelJson;
    int mJsonEventIdx;
};

#endif
