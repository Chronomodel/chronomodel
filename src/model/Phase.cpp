/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#include "Model.h"
#include "Event.h"
#include "PhaseConstraint.h"
#include "Generator.h"
#include "QtUtilities.h"
#include "StdUtilities.h"

#include <QtWidgets>

Phase::Phase(const Model* model):
    mId (0),
    mModel (model),
    mName ("no Phase Name"),
    mTauType (Phase::eTauUnknown),
    mTauFixed (0.),
    mTauMin (0.),
    mTauMax (0.),
    mIsSelected (false),
    mIsCurrent (false),
    mLevel (0)
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

    mTimeRange = std::make_pair(- INFINITY, +INFINITY);
    mValueStack["Activity_TimeRange_Level"] = TValueStack("Activity_TimeRange_Level", 0.);
    mValueStack["Activity_TimeRange_min"] = TValueStack("Activity_TimeRange_min", 0);
    mValueStack["Activity_TimeRange_max"] = TValueStack("Activity_TimeRange_max", 0);
}

Phase::Phase(const Phase& phase)
{
    copyFrom(phase);
}

Phase::Phase (const QJsonObject& json, const Model* model):
    mId (json.value(STATE_ID).toInt()),
    mModel (model),
    mName (json.value(STATE_NAME).toString()),
    mTauType ((Phase::TauType)json.value(STATE_PHASE_TAU_TYPE).toInt()),
    mTauFixed (json.value(STATE_PHASE_TAU_FIXED).toDouble()),
    mTauMin (json.value(STATE_PHASE_TAU_MIN).toDouble()),
    mTauMax (json.value(STATE_PHASE_TAU_MAX).toDouble()),
    mIsSelected (json.value(STATE_IS_SELECTED).toBool()),
    mIsCurrent (json.value(STATE_IS_CURRENT).toBool()),
    mLevel (0)

{
   mColor = QColor(json.value(STATE_COLOR_RED).toInt(), json.value(STATE_COLOR_GREEN).toInt(), json.value(STATE_COLOR_BLUE).toInt());

   mItemX = json.value(STATE_ITEM_X).toDouble();
   mItemY = json.value(STATE_ITEM_Y).toDouble();

   mAlpha.setName("Begin of Phase : " + mName);
   mBeta.setName("End of Phase : " + mName);
   // mTau.setName("Tau of Phase : " + p.mName);
   mDuration.setName("Duration of Phase : " + mName);

   mValueStack["Activity_TimeRange_Level"] = TValueStack("Activity_TimeRange_Level", 0.);
   mValueStack["Activity_TimeRange_min"] = TValueStack("Activity_TimeRange_min", 0);
   mValueStack["Activity_TimeRange_max"] = TValueStack("Activity_TimeRange_max", 0);
}

Phase& Phase::operator=(const Phase& phase)
{
    copyFrom(phase);
    return *this;
}

void Phase::copyFrom(const Phase& phase)
{
    mId = phase.mId;
    mModel = phase.mModel;
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
Phase Phase::fromJson(const QJsonObject &json)
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
   // p.mTau.setName("Tau of Phase : " + p.mName);
    p.mDuration.setName("Duration of Phase : "+p.mName);

    p.mValueStack["Activity_TimeRange_Level"] = TValueStack("Activity_TimeRange_Level", 0.);
    p.mValueStack["Activity_TimeRange_min"] = TValueStack("Activity_TimeRange_min", 0);
    p.mValueStack["Activity_TimeRange_max"] = TValueStack("Activity_TimeRange_max", 0);

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

std::pair<double,double> Phase::getFormatedTimeRange() const
{
    const double t1 = DateUtils::convertToAppSettingsFormat(mTimeRange.first);
    const double t2 = DateUtils::convertToAppSettingsFormat(mTimeRange.second);

    if (t1<t2)
        return std::pair<double,double>(t1, t2);

    else
        return std::pair<double,double>(t2, t1);

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

void Phase::update_AlphaBeta(const double tminPeriod, const double tmaxPeriod)
{
    mAlpha.mX = getMinThetaEvents(tminPeriod);
    mBeta.mX = getMaxThetaEvents(tmaxPeriod);
    mDuration.mX = mBeta.mX - mAlpha.mX;
}

void Phase::update_All(const double tminPeriod, const double tmaxPeriod)
{
    mAlpha.mX = getMinThetaEvents(tminPeriod);
    mBeta.mX = getMaxThetaEvents(tmaxPeriod);
    mDuration.mX = mBeta.mX - mAlpha.mX;

    update_Tau(tminPeriod, tmaxPeriod);
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
void Phase::update_Tau(const double tminPeriod, const double tmaxPeriod)
{
    if (mTauType == eTauFixed && mTauFixed != 0.) {
        mTau.mX = mTauFixed;

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

            mTau.mX = std::move(xn);

        }

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

void Phase::generateActivity(size_t gridLength, double h, const double threshold, const double timeRangeLevel)
{
#ifdef DEBUG
    QElapsedTimer tClock;
    tClock.start();
#endif

    // Avoid to redo calculation, when mActivity exist, it happen when the control is changed
    if (!mRawActivity.isEmpty() && gridLength == mValueStack.at("Activity_GridLength").mValue
            && h == mValueStack.at("Activity_h").mValue
            && threshold == mValueStack.at("Activity_Threshold").mValue
            && timeRangeLevel == mValueStack.at("Activity_TimeRange_Level").mValue)
       return;

    mValueStack["Activity_GridLength"] = TValueStack("Activity_GridLength", gridLength);
    mValueStack["Activity_h"] = TValueStack("Activity_h", h);
    mValueStack["Activity_Threshold"] = TValueStack("Activity_Threshold", threshold);
    mValueStack["Activity_max"] = TValueStack("Activity_max", 0.);
    mValueStack["Activity_mode"] = TValueStack("Activity_mode", 0.);
    mValueStack["Activity_TimeRange_Level"] = TValueStack("Activity_TimeRange_Level", 0.);

    const auto s = &mModel->mSettings;
    if (mEvents.size() < 2) {
        mValueStack["Significance Score"] = TValueStack("Significance Score", 0);
        mValueStack["R_etendue"] = TValueStack("R_etendue", s->mTmax - s->mTmin);
        mValueStack["t_min"] = TValueStack("t_min", s->mTmin);
        mValueStack["t_max"] = TValueStack("t_max", s->mTmax);


       // mValueStack["a_Unif"] = TValueStack("a_Unif", s->mTmin);
       // mValueStack["b_Unif"] = TValueStack("b_Unif", s->mTmax);
        mValueStack["Activity_TimeRange_min"] = TValueStack("Activity_TimeRange_min", s->mTmin);
        mValueStack["Activity_TimeRange_max"] = TValueStack("Activity_TimeRange_max", s->mTmax);

        mValueStack["Activity_min95"] = TValueStack("Activity_min95", s->mTmin);
        mValueStack["Activity_max95"] = TValueStack("Activity_max95", s->mTmax);
        mValueStack["Activity_mean95"] = TValueStack("Activity_mean95", (s->mTmax + s->mTmin)/2.);
        mValueStack["Activity_std95"] = TValueStack("Activity_std95", 0.);
        return;
    }

    // Curves for error binomial
    const int n = mEvents.size();
    mRawActivityUnifTheo.clear();

    const std::vector<double>& Gx = mModel->mBinomiale_Gx.at(n);

    //---- timeRange

    mValueStack["Activity_TimeRange_Level"] = TValueStack("Activity_TimeRange_Level", timeRangeLevel);


    if (mValueStack["Activity_TimeRange_min"].mValue == mValueStack["Activity_TimeRange_max"].mValue || mValueStack["TimeRange Threshold"].mValue != timeRangeLevel) {
        const std::pair<double, double> timeRange = timeRangeFromTraces( mAlpha.fullRunRawTrace(mModel->mChains),
                                                                         mBeta.fullRunRawTrace(mModel->mChains), timeRangeLevel, "Time Range for Phase : " + mName);
        mValueStack["Activity_TimeRange_min"] = TValueStack("Activity_TimeRange_min", timeRange.first);
        mValueStack["Activity_TimeRange_max"] = TValueStack("Activity_TimeRange_max", timeRange.second);
    }

    const double TimeRange_min = mValueStack["Activity_TimeRange_min"].mValue;
    const double TimeRange_max = mValueStack["Activity_TimeRange_max"].mValue;
    std::vector<double> concaTrace;

    double min95 = +INFINITY;
    double max95 = -INFINITY;
    for (const auto& ev : mEvents) {
        if (ev->mTheta.mSamplerProposal != MHVariable::eFixe) {
            const auto &rawtrace = ev->mTheta.fullRunRawTrace(mModel->mChains);

            std::copy_if(rawtrace.begin(), rawtrace.end(),
                         std::back_inserter(concaTrace),
                         [TimeRange_min, TimeRange_max](double x) { return (TimeRange_min<= x && x<= TimeRange_max); });
        } else {
            min95 = std::min( min95, ev->mTheta.mRawTrace->at(0));
            max95 = std::max( max95, ev->mTheta.mRawTrace->at(0));
        }

    }

    if (!concaTrace.empty()) {
        const auto &minmax95 = std::minmax_element(concaTrace.begin(), concaTrace.end());
        min95 = std::min( min95, *minmax95.first);
        max95 = std::max( max95, *minmax95.second);
    }

    if (min95 == max95) { // hapen when there is only one bound in the phase ???

        qDebug()<<"[Phase::generateActivity] tmin == tmax : " << mName;

        mRawActivity[min95] = 1;
        // Convertion in the good Date format

        mActivity = DateUtils::convertMapToAppSettingsFormat(mRawActivity);

        mValueStack["Significance Score"] = TValueStack("Significance Score", 0);
        mValueStack["R_etendue"] = TValueStack("R_etendue", 0);
        mValueStack["a_Unif"] = TValueStack("a_Unif", 0);
        mValueStack["b_Unif"] = TValueStack("b_Unif", 0);
        mValueStack["Activity_min95"] = TValueStack("Activity_min95", min95);
        mValueStack["Activity_max95"] = TValueStack("Activity_max95", max95);
        mValueStack["Activity_mean95"] = TValueStack("Activity_mean95", min95);
        mValueStack["Activity_std95"] = TValueStack("Activity_std95", 0.);
        return;

    } else {
        mValueStack["Activity_min95"] = TValueStack("Activity_min95", min95);
        mValueStack["Activity_max95"] = TValueStack("Activity_max95", max95);
        double mean95, var95;
        mean_variance_Knuth(concaTrace, mean95, var95);
        mValueStack["Activity_mean95"] = TValueStack("Activity_mean95", mean95);
        mValueStack["Activity_std95"] = TValueStack("Activity_std95", sqrt(var95));

    }

    /* const double mu = -2;
       const double R_etendue = (n+1)/(n-1)/(1.+ mu*sqrt(2./(double)((n-1)*(n+2))) )*(t_max_data-t_min_data);
       const double gamma =  (n>=500 ? 1. : gammaActivity[(int)n]);
       const double R_etendue =  (max95 - min95)/gamma;
    */

    const double R_etendue =  max95 - min95;

    // prevent h=0 and h >R_etendue;
    h = std::min( std::max(s->mStep, h),  R_etendue) ;
    mValueStack["Activity_h"] = TValueStack("Activity_h", h);

    const double h_2 = h/2.;

    const double fUnif = h / R_etendue;

    const double mid_R =  (max95 + min95)/2.;

    const double ActivityUnif = fUnif * n / h; //  remplace -> fUnif * n / std::min(h, R_etendue);

    const double half_etendue = R_etendue/2. ;

    // Recentrage de a_Unif et b_Unif
    // Variable pour courbe Unif Théo

    const double a_Unif = mid_R - half_etendue;
    const double b_Unif = mid_R + half_etendue;

    // L'unif théorique est définie par le trapéze correspondant à l'unif modifiée par la fenètre mobile
    const double a_Unif_minus_h_2 = a_Unif - h_2;
    const double a_Unif_plus_h_2 = a_Unif + h_2;

    const double b_Unif_minus_h_2 = b_Unif - h_2;
    const double b_Unif_plus_h_2 = b_Unif + h_2;

    // pour test theorique, sans le trapèze h=0
   /* const double a_Unif_minus_h_2 = a_Unif;
    const double a_Unif_plus_h_2 = a_Unif;

    const double b_Unif_minus_h_2 = b_Unif;
    const double b_Unif_plus_h_2 = b_Unif;
*/
    mValueStack["a_Unif"] = TValueStack("a_Unif", a_Unif);
    mValueStack["b_Unif"] = TValueStack("b_Unif", b_Unif);
    mValueStack["R_etendue"] = TValueStack("R_etendue", R_etendue);

  //  if (a_Unif_minus_h_2 < s->mTmin)
  //      mRawActivityUnifTheo.insert(s->mTmin,  interpolate(s->mTmin, a_Unif_minus_h_2, a_Unif_plus_h_2, 0., ActivityUnif));
  //  else
    mRawActivityUnifTheo.insert(a_Unif_minus_h_2,  0.);

    mRawActivityUnifTheo.insert(a_Unif_plus_h_2,  ActivityUnif);
    mRawActivityUnifTheo.insert(b_Unif_minus_h_2,  ActivityUnif);

 //   if (b_Unif_plus_h_2 > s->mTmax)
  //      mRawActivityUnifTheo.insert(s->mTmax,  interpolate(s->mTmax, b_Unif_minus_h_2, b_Unif_plus_h_2, ActivityUnif, 0.));
  //  else
    mRawActivityUnifTheo.insert(b_Unif_plus_h_2,  0.);

    /// Look for the maximum span containing values \f$ x=2 \f$

    if (min95 == max95) {
        qDebug()<<"[Phase::generateActivity] min95 == max95 : " << mName;
        mRawActivity[min95] = 1;

        // Convertion in the good Date format
        mActivity = DateUtils::convertMapToAppSettingsFormat( mRawActivity);

        return;
    }


#ifdef DEBUG
    if (max95 > s->mTmax) {
        qWarning("[Phase::generateActivity] max95 > mSettings.mTmax force max95 = mSettings.mTmax");
    }

#endif

    const double t_min_grid = TimeRange_min - h_2 - s->mStep;
    const double t_max_grid = TimeRange_max + h_2 + s->mStep;


    /// \f$ \delta_t_min = (max95 - min95)/(gridLength-1) \f$
    /*   const double delta_t_min = (t_max_grid - t_min_grid) / double(gridLength-1);
        if (h < delta_t_min) {
             h = delta_t_min;
         }
         */
    /// \f$ \delta_t = (max95 - min95 + h)/(gridLenth-1) \f$
    const double delta_t = (t_max_grid - t_min_grid) / double(gridLength-1);

    // overlaps

    double nr = concaTrace.size();
    const int maxGrid = (int)gridLength-1;
    // Loop
    std::vector<int> NiTot (gridLength);
    try {
        for (const auto& t : concaTrace) {

            int idxGridMin = inRange(0, (int) ceil((t - t_min_grid - h_2) / delta_t), maxGrid) ;

            if ((t - t_min_grid - h_2) / delta_t == (double) idxGridMin && (t - t_min_grid - h_2)>0) {
                ++idxGridMin;
            }
            const int idxGridMax = inRange(0, (int) floor((t - t_min_grid + h_2) / delta_t), maxGrid) ;

            for (auto&& ni = NiTot.begin() + idxGridMin; ni != NiTot.begin() + idxGridMax +1; ++ni) {
                ++*ni ;
            }

        }

        // Ajout artificiel des events et bornes fixes
        const int nRealyAccepted = std::accumulate(mModel->mChains.begin(), mModel->mChains.end(), 0, [] (int sum, ChainSpecs c) {return sum + c.mRealyAccepted;});
        for (const auto& ev : mEvents) {
            if (ev->mTheta.mSamplerProposal == MHVariable::eFixe) {
                auto t = ev->mTheta.mRawTrace->at(0);
                int idxGridMin = inRange(0, (int) ceil((t - t_min_grid - h_2) / delta_t), maxGrid) ;

                if ((t - t_min_grid - h_2) / delta_t == (double) idxGridMin && (t - t_min_grid - h_2)>0) {
                    ++idxGridMin;
                }

                int idxGridMax = inRange(0, (int) floor((t - t_min_grid + h_2) / delta_t), maxGrid) ;

                if ((t - t_min_grid + h_2) / delta_t == (double) idxGridMax && idxGridMax>0) {
                    --idxGridMax;
                }


                if (idxGridMax == idxGridMin) {
                    *(NiTot.begin()+idxGridMin) += nRealyAccepted;

                } else {
                    for (auto&& ni = NiTot.begin() + idxGridMin; ni != NiTot.begin() + idxGridMax + 1; ++ni) {
                        *ni += nRealyAccepted;
                    }
                }
                nr += nRealyAccepted;
            }
        }

    } catch (std::exception& e) {
        qWarning()<< "[Phase::generateActivity] exception caught: " << e.what() << '\n';

    } catch(...) {
        qWarning() << "[Phase::generateActivity] Caught Exception!\n";

    }

#ifdef DEBUG
       double somNi =std::accumulate(NiTot.begin(), NiTot.end(), 0.);
       qWarning() << "[Phase::generateActivity] somNi =" << somNi<< "\n";
#endif


    ///# Calculation of the mean and variance
    QVector<double> inf;
    QVector<double> sup;
    QVector<double> esp;
    double maxActivity = 0;
    double modeActivity = t_min_grid;

    double UnifScore = 0.;
    int nbIt = 0;
#ifdef DEBUG
    double somActivity = 0;
#endif
    for (const auto& ni : NiTot) {

        const double fA = ni / nr;
        const double eA =  fA * n / h;
        esp.append(eA);
        if (eA > maxActivity) {
            maxActivity =  eA;
            modeActivity = (nbIt * delta_t) + t_min_grid;
        }

#ifdef DEBUG
       somActivity += eA;
#endif
        const double QSup = interpolate_value_from_curve(fA, Gx, 0, 1.)* n / h;
        sup.append(QSup);

        const double QInf = findOnOppositeCurve(fA, Gx)* n / h;
        inf.append(QInf);

#ifdef DEBUG
        if (QSup < QInf) {
            qDebug()<<"[Model::generateActivity] QSup < QInf ; f= "<<fA<< " ; QSup = "<<QSup<<" ; QInf = "<<QInf;
        }
#endif
        // Calcul du score
        /* Delta(h) = somme sur theta de ( max(Aunif - Ainf) - min(Aunif, Asup) ) / nbre de theta de la grille, nbre de pas de la grille
             */
        // La grille est définie entre min95-h/2 et max95+h/2 avec gridlength case
        const double t = nbIt * delta_t + t_min_grid ;

        double dUnif;
        if ((a_Unif_minus_h_2 < t) && (t < a_Unif_plus_h_2)) {
            dUnif =  interpolateValueInQMap(t, mRawActivityUnifTheo);

        } else if ((a_Unif_plus_h_2 <= t) && (t <= b_Unif_minus_h_2)) {
            dUnif = ActivityUnif;

        } else if ((b_Unif_minus_h_2 < t) && (t < b_Unif_plus_h_2)) {
            dUnif =  interpolateValueInQMap(t, mRawActivityUnifTheo);

        } else
            dUnif = 0;

        //UnifScore += std::pow((dUnif - eA)/(QSup-QInf), 2.)/gridLength;
        //const auto nd = N(dUnif, eA, QSup-QInf);
        //std::cout<<"N(dUnif)"<<nd;
        //const double addUnif = exp(-0.5*pow((dUnif - eA)/(QSup-QInf), 2.)); // /(gridLength));
       // UnifScore +=  exp(-0.5*pow((dUnif - eA)/(QSup-QInf), 2.))/(gridLength); //N(dUnif, eA, QSup-QInf)/gridLength;//
       /* const double addUnif = std::abs(std::max(dUnif, QInf) - std::min(dUnif, QSup)) / gridLength;
        if (addUnif>0)
         qDebug()<<" t= "<<t<<" add="<< addUnif;
         */

        UnifScore += std::max(dUnif, QInf) - std::min(dUnif, QSup) / gridLength;
       // UnifScore += std::pow(dUnif - eA, 2.) / gridLength; // pour test

        nbIt++;
    }

#ifdef DEBUG
    qDebug()<<"[Model::generateActivity] somme Activity = "<< somActivity << " ; Phase = "<< mName <<"\n";
#endif
    mValueStack["Significance Score"] = TValueStack("Significance Score", UnifScore);
    mValueStack["Activity_max"] = TValueStack("Activity_max", maxActivity);
    mValueStack["Activity_mode"] = TValueStack("Activity_mode", modeActivity);

    mRawActivity = vector_to_map(esp, t_min_grid, t_max_grid, delta_t);
    mRawActivityInf = vector_to_map(inf, t_min_grid, t_max_grid, delta_t);
    mRawActivitySup = vector_to_map(sup, t_min_grid, t_max_grid, delta_t);

    const double QSup = interpolate_value_from_curve(0., Gx, 0, 1.)* n / h;
    const double QInf = findOnOppositeCurve(0., Gx)* n / h;

    mRawActivitySup.insert(t_min_grid, QSup );
    mRawActivitySup.insert(t_max_grid, QSup );


    mRawActivityInf.insert(t_min_grid, QInf);
    mRawActivityInf.insert(t_max_grid, QInf);


    mActivity = DateUtils::convertMapToAppSettingsFormat(mRawActivity);
    mActivityInf = DateUtils::convertMapToAppSettingsFormat(mRawActivityInf);
    mActivitySup = DateUtils::convertMapToAppSettingsFormat(mRawActivitySup);

    mActivityUnifTheo = DateUtils::convertMapToAppSettingsFormat(mRawActivityUnifTheo);


#ifdef DEBUG
    qDebug() <<  QString("[Phase::generateActivity] done in " + DHMS(tClock.elapsed()));

#endif

}
