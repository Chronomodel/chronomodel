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

#include "Phase.h"
#include "Event.h"
#include "PhaseConstraint.h"
#include "Generator.h"
#include "QtUtilities.h"

#include <QtWidgets>

Phase::Phase():
mId(0),
mName("no Phase Name"),
mTauType(Phase::eTauUnknown),
mTauFixed(0.),
mTauMin(0.),
mTauMax(0.),
mIsSelected(false),
mIsCurrent(false),
mLevel(0)
{
    mColor = randomColor();
    mAlpha.mSupport = MetropolisVariable::eBounded;
    mAlpha.mFormat = DateUtils::eUnknown;

    mBeta.mSupport = MetropolisVariable::eBounded;
    mBeta.mFormat = DateUtils::eUnknown;

    mTau.mSupport = MetropolisVariable::eRp;
    mTau.mFormat = DateUtils::eNumeric;

    mDuration.mSupport = MetropolisVariable::eRp;
    mDuration.mFormat = DateUtils::eNumeric;
    // Item initial position :
    mItemX = 0.;
    mItemY = 0.;
}

Phase::Phase(const Phase& phase)
{
    copyFrom(phase);
}

Phase& Phase::operator=(const Phase& phase)
{
    copyFrom(phase);
    return *this;
}

void Phase::copyFrom(const Phase& phase)
{
    mId = phase.mId;
    mName = phase.mName;
    mColor = phase.mColor;

    mAlpha = phase.mAlpha;
    mBeta = phase.mBeta;
    mDuration = phase.mDuration;

    mTau = phase.mTau;

    mTauType = phase.mTauType;
    mTauFixed = phase.mTauFixed;
    mTauMin = phase.mTauMin;
    mTauMax = phase.mTauMax;

    mItemX = phase.mItemX;
    mItemY = phase.mItemY;

    mIsSelected = phase.mIsSelected;
    mIsCurrent = phase.mIsCurrent;

    mEvents = phase.mEvents;
    mConstraintsFwd = phase.mConstraintsFwd;
    mConstraintsBwd = phase.mConstraintsBwd;
}

Phase::~Phase()
{
   for (auto && ev: mEvents)
            ev = nullptr;

   mEvents.clear();

    if (!mConstraintsFwd.isEmpty()) {
        for (auto && pc : mConstraintsFwd)
            pc = nullptr;

        mConstraintsFwd.clear();
    }
    if (!mConstraintsBwd.isEmpty()) {
        for (auto && pc : mConstraintsBwd)
            pc = nullptr;

        mConstraintsBwd.clear();
    }
}

// Properties

/**
 * @todo Check the copy of the color if mJson is not set
 */
Phase Phase::fromJson(const QJsonObject& json)
{
    Phase p;
    p.mId = json.value(STATE_ID).toInt();
    p.mName = json.value(STATE_NAME).toString();
    p.mColor = QColor(json.value(STATE_COLOR_RED).toInt(), json.value(STATE_COLOR_GREEN).toInt(), json.value(STATE_COLOR_BLUE).toInt());

    p.mItemX = json.value(STATE_ITEM_X).toDouble();
    p.mItemY = json.value(STATE_ITEM_Y).toDouble();
    p.mTauType = (Phase::TauType)json.value(STATE_PHASE_TAU_TYPE).toInt();
    p.mTauFixed = json.value(STATE_PHASE_TAU_FIXED).toDouble();
    p.mTauMin = json.value(STATE_PHASE_TAU_MIN).toDouble();
    p.mTauMax = json.value(STATE_PHASE_TAU_MAX).toDouble();
    p.mIsSelected = json.value(STATE_IS_SELECTED).toBool();
    p.mIsCurrent = json.value(STATE_IS_CURRENT).toBool();

    p.mAlpha.setName("Begin of Phase : "+p.mName);
    p.mBeta.setName("End of Phase : "+p.mName);
    p.mTau.setName("Tau of Phase : " + p.mName);
    p.mDuration.setName("Duration of Phase : "+p.mName);

    return p;
}

QJsonObject Phase::toJson() const
{
    QJsonObject phase;

    phase[STATE_ID] = mId;
    phase[STATE_NAME] = mName;
    phase[STATE_COLOR_RED] = mColor.red();
    phase[STATE_COLOR_GREEN] = mColor.green();
    phase[STATE_COLOR_BLUE] = mColor.blue();
    phase[STATE_ITEM_X] = mItemX;
    phase[STATE_ITEM_Y] = mItemY;
    phase[STATE_PHASE_TAU_TYPE] = mTauType;
    phase[STATE_PHASE_TAU_FIXED] = mTauFixed;
    phase[STATE_PHASE_TAU_MIN] = mTauMin;
    phase[STATE_PHASE_TAU_MAX] = mTauMax;
    phase[STATE_IS_SELECTED] = mIsSelected;
    phase[STATE_IS_CURRENT] = mIsCurrent;

    return phase;
}

// --------------------------------------------------------------------------------

QPair<double,double> Phase::getFormatedTimeRange() const
{
    const double t1 = DateUtils::convertToAppSettingsFormat(mTimeRange.first);
    const double t2 = DateUtils::convertToAppSettingsFormat(mTimeRange.second);

    if (t1<t2)
        return QPair<double,double>(t1,t2);

    else
        return QPair<double,double>(t2,t1);

}

double Phase::getMaxThetaEvents(double tmax)
{
    Q_ASSERT_X(!mEvents.isEmpty(), "Phase::getMaxThetaEvents", QString("No Event in Phase :" + this->mName).toStdString().c_str());
    (void) tmax;
    double theta (mEvents[0]->mTheta.mX);

    // All Event must be Initialized
    std::for_each(mEvents.begin(), mEvents.end(), [&theta] (Event* ev) {theta= std::max(ev->mTheta.mX, theta);});
    return theta;

    // if we need to use this function with event not initalized we have to use the next code
 /*

    double theta;
    bool found = false;
    QList<Event*>::const_iterator iterEvent = mEvents.constBegin();
    while (iterEvent != mEvents.constEnd()) {
        if ((*iterEvent)->mInitialized)  {
            if (!found) {
                theta = (*iterEvent)->mTheta.mX;
                found = true;
            }
            else
                theta = qMax(theta, (*iterEvent)->mTheta.mX);

        }
        ++iterEvent;
    }

    return found ? theta : tmax;
*/
}
/**
 * @brief Phase::getMinThetaEvents
 * @param tmin
 * @return the min of Event inside the phase, used in Phase::updateAll() to set alpha and beta
 */
double Phase::getMinThetaEvents(double tmin)
{
    Q_ASSERT_X(!mEvents.isEmpty(), "Phase::getMinThetaEvents", QString("No Event in Phase :" + mName).toStdString().c_str());
    (void) tmin;
    double theta (mEvents[0]->mTheta.mX);

    // All Event must be Initialized
    std::for_each(mEvents.begin(), mEvents.end(), [&theta] (Event* ev){theta= std::min(ev->mTheta.mX, theta);});
    return theta;

    // if we need to use this function with event not initalized we have to use the next code
 /*
    bool found = false;
    QList<Event*>::const_iterator iterEvent = mEvents.constBegin();
    while(iterEvent != mEvents.constEnd()) {
        if ((*iterEvent)->mInitialized)  {
            if (!found) {
                theta = (*iterEvent)->mTheta.mX;
                found = true;
            } else
                theta = qMin(theta, (*iterEvent)->mTheta.mX);

        }
        ++iterEvent;
    }
    return found ? theta : tmin;
*/
}


double Phase::getMinThetaNextPhases(const double tmax)
{
    double minTheta = tmax;
    for (auto &&constFwd : mConstraintsFwd) {
        // we can juste look alpha and beta set in member mAlpha and mBeta
        //double theta= mConstraintsFwd[i]->mPhaseTo->getMinThetaEvents(tmax);
        double theta (constFwd->mPhaseTo->mAlpha.mX);

        if (constFwd->mGammaType != PhaseConstraint::eGammaUnknown)
            minTheta = std::min(minTheta, theta - constFwd->mGamma);
        else
            minTheta = std::min(minTheta, theta);
    }
    return minTheta;
}

double Phase::getMaxThetaPrevPhases(const double tmin)
{
    double maxTheta (tmin);

    for (auto &&constBwd : mConstraintsBwd) {
        const double theta (constBwd->mPhaseFrom->mBeta.mX);

        if (constBwd->mGammaType != PhaseConstraint::eGammaUnknown)
            maxTheta = std::max(maxTheta, theta + constBwd->mGamma);
        else
            maxTheta = std::max(maxTheta, theta);
    }
    return maxTheta;
}

// --------------------------------------------------------------------------------

void Phase::updateAll(const double tminPeriod, const double tmaxPeriod)
{
    //static bool initalized = false; // What is it??

    mAlpha.mX = getMinThetaEvents(tminPeriod);
    mBeta.mX = getMaxThetaEvents(tmaxPeriod);
    mDuration.mX = mBeta.mX - mAlpha.mX;

    updateTau(tminPeriod, tmaxPeriod);

    //initalized = true;
}

QString Phase::getTauTypeText() const
{
    switch (mTauType) {
            case eTauFixed:
                    return QObject::tr("Max Duration ≤ %1").arg( QString::number(mTauFixed));
                break;
            case eTauUnknown:
                    return QObject::tr("Max Duration Unknown");
                break;
            case eZOnly:
                    return QObject::tr("Span Uniform");
                break;
         /*   case eTauRange: // no longer used ->OBSOLETE
                return QObject::tr("Tau Range") + QString(" [ %1 ; %2 ]").arg(QString::number(mTauMin), QString::number(mTauMax));
            break;  */
            default:
                    return QObject::tr("Tau Undefined -> Error");
                break;
        }

}

void Phase::initTau(const double tminPeriod, const double tmaxPeriod)
{
    if (mTauType == eTauFixed && mTauFixed != 0.)
        mTau.mX = mTauFixed;

   /* else if (mTauType == eTauRange && mTauMax > mTauMin) // no longer used
        mTau.mX = mTauMax;
*/
    else if (mTauType == eZOnly) {
            // Modif PhD ; initialisation arbitraire
            mTau.mX = tmaxPeriod - tminPeriod;
            // nothing to do
        }
    else if (mTauType == eTauUnknown) {
        // nothing to do
    }
}

// Formule du 25 mars 2020

double somKn (double x, int n, double Rp, double s)
{
  double som  = 0.;

  if (n >= 3) {
      for (int np1 = 1.; np1 <= (n-2); ++np1 ) { // memory np1 = n - p - 1.;
          som +=  (pow(Rp/x, np1) - pow(Rp/s, np1) )/np1;
      }
  }
  return(std::move(som));
}

double intFx (double x, int n, double Rp, double s)
{

  if (n < 2) {
    return(0.);

  } else if (n < 3) {
    return( log( x/(Rp-x) * (Rp-s)/s ) );

  } else {
      return( log( x/(Rp-x) * (Rp-s)/s )- somKn(x, n, Rp, s) );
  }

}

double Px(double x, int n, double Rp)
{
    const double P2 = 1./(Rp-x) + (1./x) ;

    double som = 0.;
    for (int p = 1; p <=(n-2); ++p ) {
      //som +=  (pow(Rp, n - p -1) / pow(x, n-p) );
      som += pow(Rp/x, n-p)/Rp;
    }
if (isnan(P2+som)) {
    qDebug()<<"Px is Nan "<<x<<" "<<n<<" "<<Rp;
}
    return(std::move(P2+som) );

}
void Phase::updateTau(const double tminPeriod, const double tmaxPeriod)
{
    if (mTauType == eTauFixed && mTauFixed != 0.) {
        mTau.mX = mTauFixed;

  /*  } else if (mTauType == eTauRange && mTauMax > mTauMin) {
        mTau.mX = Generator::randomUniform(qMax(mTauMin, mBeta.mX - mAlpha.mX), mTauMax);
*/
   } else if (mTauType == eZOnly) {
            // Modif PhD 2022
            // model definition

            const int n = mEvents.length();
            double s = mBeta.mX - mAlpha.mX;
            const double precision = .0001;
            const double R = tmaxPeriod - tminPeriod;
            const double Rp = R/(n-1)*n;

            const double u = Generator::randomUniform(0.0, 1.0);

            // solve equation F(x)-u=0
            const double FbMax = intFx(R, n, Rp, s);
            //double Fb = 1.0 - u;  // pour mémoire



                // Newton

            double xn = s;
            double  b1;
            double itF, p;
            if ( !isinf(FbMax)) {
                double Epsilon = R;

                while ( Epsilon >= precision/100.) {

                    itF = intFx(xn, n, Rp, s);

                    itF = itF - u*FbMax;
                    p = Px(xn, n, Rp);

                    //b1 = itF != 0. ? (x - (itF / p )): x;
                    b1 = xn - (itF / p );
                    Epsilon = abs((xn - b1)/b1);
                    xn = std::move(b1);

                }
                 xn = std::max(s, std::min(xn, R));

            }



         //  mTau.mX = std::move(xd);

       /*  if (xn<mTau.mX) {
               qDebug()<<"-----------precision------> U = "<< u << " R = "<< R <<  "xd = "<<xn <<  "xn = "<< xn <<" s = "<< s;
          }
 */ // } else


            mTau.mX = std::move(xn);

        }
   /* else if (mTauType == eTauUnknown) {
        // Nothing to do!
    }*/
}

void Phase::memoAll()
{
    mAlpha.memo();
    mBeta.memo();
   // if (mTauType == eZOnly)
     //   mTau.memo();
    mDuration.memo();
#ifdef DEBUG
    if (mBeta.mX - mAlpha.mX < 0.)
        qDebug()<<"in Phase::memoAll : "<<mName<<" Warning mBeta.mX - mAlpha.mX<0";
#endif
}

void Phase::generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
  //  if (mAlpha.HistoWithParameter(fftLen, bandwidth, tmin, tmax) == false) {
        mAlpha.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
        mBeta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
       // if (mTauType == eZOnly)
         //   mTau.generateHistos(chains, fftLen, bandwidth);
        mDuration.generateHistos(chains, fftLen, bandwidth);
  //  }
}