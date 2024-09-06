/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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


#include <QColor>
#include <QJsonObject>

class Model;
class Phase;
class EventConstraint;

#define NotS02_BAYESIAN

class Event: std::enable_shared_from_this<Event>
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


    QColor mColor;

    double mItemX;
    double mItemY;

    bool mIsCurrent;
    bool mIsSelected;

    std::vector<Date> mDates;

    std::vector<int> mPhasesIds;
    std::vector<int> mConstraintsFwdIds;
    std::vector<int> mConstraintsBwdIds;

    std::vector<std::shared_ptr<Phase>> mPhases;
    std::vector<std::shared_ptr<EventConstraint>> mConstraintsFwd;
    std::vector<std::shared_ptr<EventConstraint>> mConstraintsBwd;

    MHVariable mTheta;
    MHVariable mS02Theta;

    double mAShrinkage;
    double mBetaS02;
    bool mInitialized;

    // Used with Event::getThetaMinRecursive_v2 and Event::getThetaMaxRecursive_v2 and Event::getThetaMaxPossible
    bool mIsNode; // Used with getThetaMinRecursive_v3 and getThetaMaxRecursive_v3
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

    MHVariable mVg; // sigma G of the event (relative to G(t) that we are trying to estimate)

    std::shared_ptr<CalibrationCurve> mMixingCalibrations;

#pragma mark Functions

    Event();

    explicit Event (const QJsonObject& json);
    Event(const Event &origin);
    virtual ~Event();

    inline QString getQStringName() const {return QString::fromStdString(_name);}
    inline std::string name() const {return _name;}
    void setName(const std::string name) {_name = name;}
    void setName(const QString name) {_name = name.toStdString();}

   // virtual Event& operator=(const Event& origin);
    virtual void copyFrom(const Event& event);

    static Event fromJson(const QJsonObject& json); // With no model
    virtual QJsonObject toJson() const;

    inline Type type() const { return mType;}

    void clear();
    
    static void setCurveCsvDataToJsonEvent(QJsonObject &event, const QMap<QString, double> &CurveData);
    static QString curveDescriptionFromJsonEvent(QJsonObject &event, CurveSettings::ProcessType processType = CurveSettings::eProcess_None);
    static QList<double> curveParametersFromJsonEvent(QJsonObject &event, CurveSettings::ProcessType processType = CurveSettings::eProcess_None);

#pragma mark Functions used within the MCMC process ( not in the init part!)

    double getThetaMin(double defaultValue);
    double getThetaMax(double defaultValue);

#pragma mark  Functions used within the init MCMC process

    // bool getThetaMinPossible(const Event* originEvent, QString &circularEventName,  const QList<Event*> &startEvents, QString &linkStr); // useless
    bool getThetaMaxPossible(Event *originEvent, QString &circularEventName,  const std::vector<Event *> &startEvents);

    bool is_direct_older(const Event &origin);
    bool is_direct_younger(const Event &origin);
    double getThetaMinRecursive_v2(const double defaultValue, const std::vector<Event* > &startEvents = std::vector<Event*>());
    double getThetaMaxRecursive_v2(const double defaultValue, const std::vector<Event* > &startEvents = std::vector<Event*>());

    double getThetaMinRecursive_v3(const double defaultValue, const std::vector<Event* > &startEvents = std::vector<Event*>());
    double getThetaMaxRecursive_v3(const double defaultValue, const std::vector<Event* > &startEvents = std::vector<Event*>());

    virtual void updateTheta(const double tmin, const double tmax) {updateTheta_v3(tmin, tmax);};

    void updateTheta_v3(const double tmin, const double tmax);

    void updateTheta_v4(const double tmin, const double tmax, const double rate_theta = 1.);
    /*obsolete
    void updateTheta_v41(const double tmin, const double tmax, const double rate_theta = 1.);
    void updateTheta_v42(const double tmin, const double tmax, const double rate_theta = 1.);
    */
    //void generate_mixingCalibration();

    void updateS02();
    double h_S02(const double S02);

    void generateHistos(const std::vector<ChainSpecs> &chains, const int fftLen, const double bandwidth, const double tmin, const double tmax);

    void updateW();

private:
    std::string _name; //must be public, to be defined by dialogbox

};

inline double get_Yx(std::shared_ptr<Event> e) {return e->mYx;};
inline double get_Yy(std::shared_ptr<Event> e) {return e->mYy;};
inline double get_Yz(std::shared_ptr<Event> e) {return e->mYz;};
inline double get_Sy(std::shared_ptr<Event> e) {return e->mSy;};

inline double get_Gx(std::shared_ptr<Event> e) {return e->mGx;};
inline double get_Gy(std::shared_ptr<Event> e) {return e->mGy;};
inline double get_Gz(std::shared_ptr<Event> e) {return e->mGz;};

inline double get_Theta(std::shared_ptr<Event> e) {return e->mTheta.mX;};
inline double get_ThetaReduced(std::shared_ptr<Event> e) {return e->mThetaReduced;};

std::vector<double> get_vector(const std::function <double (std::shared_ptr<Event>)> &fun, const std::vector<std::shared_ptr<Event>> &events);

#endif
