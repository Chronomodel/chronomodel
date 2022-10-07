/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */
#ifndef EVENT_H
#define EVENT_H

#include "MHVariable.h"
//#include "StateKeys.h"
#include "Date.h"
#include "CurveSettings.h"

#include <QMap>
#include <QColor>
#include <QJsonObject>

class Phase;
class EventConstraint;


class Event
{
public:
    enum Type {
        eDefault = 0,   /*  The classic type of Event with variance */
        eBound = 1     /* The Bound type */
    };

    Event();
    Event(const Event& event);
    Event& operator=(const Event& event);
    virtual void copyFrom(const Event& event);
    virtual ~Event();

    static Event fromJson(const QJsonObject& json);
    virtual QJsonObject toJson() const;

    Type type() const;

    void reset();
    
    static void setCurveCsvDataToJsonEvent(QJsonObject &event, const QMap<QString, double> &CurveData);
    static QString curveDescriptionFromJsonEvent(QJsonObject &event, CurveSettings::ProcessType processType = CurveSettings::eProcessTypeNone, CurveSettings::VariableType variableType = CurveSettings::eVariableTypeOther);

    /// Functions used within the MCMC process ( not in the init part!) :
    double getThetaMin(double defaultValue);
    double getThetaMax(double defaultValue);


    ///  Functions used within the init MCMC process
    bool getThetaMinPossible(const Event* originEvent, QString &circularEventName,  const QList<Event*> &startEvents, QString &linkStr);
    bool getThetaMaxPossible(const Event* originEvent, QString &circularEventName,  const QList<Event*> &startEvents);

    double getThetaMinRecursive(const double defaultValue, const QList<Event*> &startEvents = QList<Event*>());
    double getThetaMaxRecursive(const double defaultValue, const QList<Event*> &startEvents = QList<Event*>());

    virtual void updateTheta(const double tmin, const double tmax);

    void generateHistos(const QList<ChainSpecs> &chains, const int fftLen, const double bandwidth, const double tmin, const double tmax);

public:
    Type mType;
    int mId;

    QString mName; //must be public, to be defined by dialogbox
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

    bool mNodeInitialized;
    double mThetaNode;
    int mLevel; // Used to init mcmc
    
    double mMixingLevel;
    
    // --------------------------------------------------------
    //  Curve
    // --------------------------------------------------------
    
    // Values entered by the user
    double mXIncDepth; //mYInc;
    double mYDec;
    double mZField; // mYInt;

    double mS_XA95Depth; // mSInc;
    double mS_Y; //mSDec;
    double mS_ZField; //  mSInt;
    
    // Prepared (projected) values
    double mYx;
    double mYy;
    double mYz;
    
    // Values used for the calculations
    double mThetaReduced;
    double mY;
    double mSy;
    double mW;
    
    MHVariable mVG; // sigma G de l'event (par rapport à G(t) qu'on cherche à estimer)
    
    // A chaque mise à jour de VG, on doit aussi mettre w à jour :
    void updateW();
};

#endif
