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
#include "Date.h"
#include "CurveSettings.h"

#include <QMap>
#include <QColor>
#include <QJsonObject>

class Model;
class Phase;
class EventConstraint;


#define NS02_BAYESIAN




class Event
{
public:
    enum Type {
        eDefault = 0,  /*  The classic/default type of Event with mS02*/
        eBound = 1     /* The Bound type, with no mS02 */
    };

    enum PointType {
        ePoint = 0,   /*  The classic type of Event with mVg != 0 */
        eNode = 1     /* The Node type with mVg = 0 */
    };

    Type mType;
    int mId;
    const Model *mModel;

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
    MHVariable mS02;

    double mAShrinkage;
    //double mS02harmonique;
    double mBetaS02;
    bool mInitialized;

    bool mNodeInitialized;
    double mThetaNode;
    int mLevel; // Used to init mcmc

    // --------------------------------------------------------
    //  Curve
    // --------------------------------------------------------

    // Values entered by the user
    PointType mPointType;
    double mXIncDepth;
    double mYDec;
    double mZField;

    double mS_XA95Depth;
    double mS_Y;
    double mS_ZField;

    // Prepared (projected) values
    double mYx;
    double mYy;
    double mYz;

    // Splines values
    double mGx;
    double mGy;
    double mGz;

    // Values used for the calculations
    t_reduceTime mThetaReduced;

    double mSy;
    double mW;

    MHVariable mVg; // sigma G de l'event (par rapport à G(t) qu'on cherche à estimer)

#pragma mark Functions

    Event (const Model * model = nullptr);
   // Event (const Event& event); // Deprecated
    explicit Event (const QJsonObject& json, const Model *model);
    virtual ~Event();

  //  Event& operator=(const Event& event);
    virtual void copyFrom(const Event& event);

    static Event fromJson(const QJsonObject& json);
    virtual QJsonObject toJson() const;

    inline Type type() const { return mType;}

    void reset();
    
    static void setCurveCsvDataToJsonEvent(QJsonObject &event, const QMap<QString, double> &CurveData);
    static QString curveDescriptionFromJsonEvent(QJsonObject &event, CurveSettings::ProcessType processType = CurveSettings::eProcess_None);
    static QList<double> curveParametersFromJsonEvent(QJsonObject &event, CurveSettings::ProcessType processType = CurveSettings::eProcess_None);

 #pragma mark Functions used within the MCMC process ( not in the init part!)

    double getThetaMin(double defaultValue);
    double getThetaMax(double defaultValue);


#pragma mark  Functions used within the init MCMC process

    bool getThetaMinPossible(const Event* originEvent, QString &circularEventName,  const QList<Event*> &startEvents, QString &linkStr);
    bool getThetaMaxPossible(const Event* originEvent, QString &circularEventName,  const QList<Event*> &startEvents);

    double getThetaMinRecursive(const double defaultValue, const QList<Event*> &startEvents = QList<Event*>());
    double getThetaMaxRecursive(const double defaultValue, const QList<Event*> &startEvents = QList<Event*>());

    virtual void updateTheta(const double tmin, const double tmax);

    void updateS02();
    double h_S02(const double S02);

    void generateHistos(const QList<ChainSpecs> &chains, const int fftLen, const double bandwidth, const double tmin, const double tmax);

    void updateW();

};


inline double get_Yx(Event* e) {return e->mYx;};
inline double get_Yy(Event* e) {return e->mYy;};
inline double get_Yz(Event* e) {return e->mYz;};

inline double get_Gx(Event* e) {return e->mGx;};
inline double get_Gy(Event* e) {return e->mGy;};
inline double get_Gz(Event* e) {return e->mGz;};

inline double get_Theta(Event* e) {return e->mTheta.mX;};
inline double get_ThetaReduced(Event* e) {return e->mThetaReduced;};

//std::vector<double> get_ThetaVector(const QList<Event *> &events);
//std::vector<double> get_ThetaReducedVector(const QList<Event *> &events);

std::vector<double> get_vector(const std::function <double (Event*)> &fun, const QList<Event *> &events);


#endif
