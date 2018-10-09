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
        eDefault = 0,   /**<  The classic type of Event with variance */
        eKnown = 1     /**< The Bound type */
    };
    enum Method{
        eFixe = -1,  /**<  use with Type==eKnown */
        eDoubleExp = 0, /**<  The default method */
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
    
    
    /// Functions used within the MCMC process ( not in the init part!) :
    double getThetaMin(double defaultValue);
    double getThetaMax(double defaultValue);
    
    
    ///  Functions used within the init MCMC process
    double getThetaMinRecursive_old(const double defaultValue,
                                const QVector<QVector<Event*> >& eventBranches,
                                const QVector<QVector<Phase*> >& phaseBranches);

    double getThetaMaxRecursive_old(const double defaultValue,
                                const QVector<QVector<Event*> >& eventBranches,
                                const QVector<QVector<Phase*> >& phaseBranches);

    bool getThetaMinPossible(const Event *originEvent, QString &circularEventName,  QList<Event *> &startEvents, QString &linkStr);
    bool getThetaMaxPossible(const Event *originEvent, QString &circularEventName,  QList<Event *> &startEvents);

    double getThetaMinRecursive(const double defaultValue, const QList<Event *> startEvents= QList<Event*>());
    double getThetaMaxRecursive(const double defaultValue, const QList<Event *> startEvents = QList<Event*>());
    
    virtual void updateTheta(const double& min, const double& max);

    void generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax);
    
public:
    Type mType;
    int mId;
    
    QString mName; //must be public, to be defined by dialogbox
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
    
    bool mNodeInitialized;
    double mThetaNode;
    int mLevel; // used to init mcmc
    
    double mMixingLevel;
};

#endif
