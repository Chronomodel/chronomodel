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

#include "Phase.h"

#include "Event.h"
#include "PhaseConstraint.h"
#include "Generator.h"
#include "QtUtilities.h"
#include "StateKeys.h"
#include "StdUtilities.h"

#include <QtWidgets>

Phase::Phase():
    mId (0),
    mAlpha(),
    mBeta(),
    mTimeRange(),
    mDuration(),
    mTempo(),
    mTempoInf(),
    mTempoSup(),
    mActivity(),
    mActivityInf(),
    mActivitySup(),
    mRawTempo(),
    mRawTempoInf(),
    mRawTempoSup(),
    mRawActivity(),
    mRawActivityInf(),
    mRawActivitySup(),
    mValueStack(),
    mTau(),
    mTauType (Phase::eTauUnknown),
    mTauFixed (0.),
    mTauMin (0.),
    mTauMax (0.),
    mIsSelected (false),
    mIsCurrent (false),
    mLevel (0),
    _name("no Phase Name")
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

    mValueStack.emplace("t_min", -INFINITY);
    mValueStack.emplace("t_max", INFINITY);


    mValueStack.emplace("Activity_TimeRange_Level", 0.);
    mValueStack.emplace("Activity_TimeRange_min", 0.);
    mValueStack.emplace("Activity_TimeRange_max", 0.);

    mValueStack.emplace("Activity_Significance_Score", 0.);
    mValueStack.emplace("R_etendue",  0);
    mValueStack.emplace("a_Unif", -INFINITY);
    mValueStack.emplace("b_Unif", INFINITY);

    mValueStack.emplace("Activity_min95", 0);
    mValueStack.emplace("Activity_max95", 0);
    mValueStack.emplace("Activity_mean95", 0.);
    mValueStack.emplace("Activity_std95", 0.);

    mValueStack.emplace("Activity_GridLength", 0.);
    mValueStack.emplace("Activity_h", 0.);
    mValueStack.emplace("Activity_Threshold", 0.);
    mValueStack.emplace("Activity_max", 0.);
    mValueStack.emplace("Activity_mode",  0.);
}

Phase::Phase(const Phase& phase)
{
    copyFrom(phase);
}

Phase::Phase (const QJsonObject& json):
    mId(json.value(STATE_ID).toInt()),
    mAlpha(),
    mBeta(),
    mTimeRange(),
    mDuration(),
    mTempo(),
    mTempoInf(),
    mTempoSup(),
    mActivity(),
    mActivityInf(),
    mActivitySup(),
    mRawTempo(),
    mRawTempoInf(),
    mRawTempoSup(),
    mRawActivity(),
    mRawActivityInf(),
    mRawActivitySup(),
    mValueStack(),
    mTau(),
    mTauType ((Phase::TauType)json.value(STATE_PHASE_TAU_TYPE).toInt()),
    mTauFixed (json.value(STATE_PHASE_TAU_FIXED).toDouble()),
    mTauMin (json.value(STATE_PHASE_TAU_MIN).toDouble()),
    mTauMax (json.value(STATE_PHASE_TAU_MAX).toDouble()),
    mIsSelected (json.value(STATE_IS_SELECTED).toBool()),
    mIsCurrent (json.value(STATE_IS_CURRENT).toBool()),
    mLevel (0),
    _name(json.value(STATE_NAME).toString().toStdString())

{
   mColor = QColor(json.value(STATE_COLOR_RED).toInt(), json.value(STATE_COLOR_GREEN).toInt(), json.value(STATE_COLOR_BLUE).toInt());

   mItemX = json.value(STATE_ITEM_X).toDouble();
   mItemY = json.value(STATE_ITEM_Y).toDouble();

   mAlpha.setName("Begin of Phase : " + _name);
   mAlpha.mSupport = MetropolisVariable::eBounded;
   mAlpha.mFormat = DateUtils::eUnknown;

   mBeta.setName("End of Phase : " + _name);
   mBeta.mSupport = MetropolisVariable::eBounded;
   mBeta.mFormat = DateUtils::eUnknown;

   mTau.setName("Tau of Phase : " + _name);
   mTau.mSupport = MetropolisVariable::eRp;
   mTau.mFormat = DateUtils::eNumeric;

   mDuration.setName("Duration of Phase : " + _name);
   mDuration.mSupport = MetropolisVariable::eRp;
   mDuration.mFormat = DateUtils::eNumeric;

   mValueStack.emplace("t_min", -INFINITY);
   mValueStack.emplace("t_max", INFINITY);

   mValueStack.emplace("Activity_TimeRange_Level", 0.);
   mValueStack.emplace("Activity_TimeRange_min", 0.);
   mValueStack.emplace("Activity_TimeRange_max", 0.);

   mValueStack.emplace("Activity_Significance_Score", 0.);
   mValueStack.emplace("R_etendue",  0.);
   mValueStack.emplace("a_Unif", -INFINITY);
   mValueStack.emplace("b_Unif", INFINITY);

   mValueStack.emplace("Activity_min95", 0);
   mValueStack.emplace("Activity_max95", 0);
   mValueStack.emplace("Activity_mean95", 0.);
   mValueStack.emplace("Activity_std95", 0.);

   mValueStack.emplace("Activity_GridLength", 0.);
   mValueStack.emplace("Activity_h", 0.);
   mValueStack.emplace("Activity_Threshold", 0.);
   mValueStack.emplace("Activity_max", 0.);
   mValueStack.emplace("Activity_mode",  0.);
}

Phase& Phase::operator=(const Phase& phase)
{
    copyFrom(phase);
    return *this;
}

void Phase::copyFrom(const Phase& phase)
{
    mId = phase.mId;

    _name = phase._name;
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
    mConstraintsNextPhases = phase.mConstraintsNextPhases;
    mConstraintsPrevPhases = phase.mConstraintsPrevPhases;
    mValueStack = phase.mValueStack;
}

Phase::~Phase()
{

}

void Phase::clear()
{
    mAlpha.clear();
    mBeta.clear();
    mTau.clear();
    mDuration.clear();

    mRawTempo.clear();
    mRawTempoInf.clear();
    mRawTempoSup.clear();

    mTempo.clear();
    mTempoInf.clear();
    mTempoSup.clear();

    mRawActivity.clear();
    mRawActivityInf.clear();
    mRawActivitySup.clear();

    mActivity.clear();
    mActivityInf.clear();
    mActivitySup.clear();

    mRawActivityUnifTheo.clear();
    mActivityUnifTheo.clear();
}

void Phase::shrink_to_fit()
{
    mAlpha.shrink_to_fit();
    mBeta.shrink_to_fit();
    mTau.shrink_to_fit();
    mDuration.shrink_to_fit();

}

void Phase::clear_and_shrink() noexcept
{
    mAlpha.clear();
    mAlpha.shrink_to_fit();

    mBeta.clear();
    mBeta.shrink_to_fit();

    mTau.clear();
    mTau.shrink_to_fit();

    mDuration.clear();
    mDuration.shrink_to_fit();

    mRawTempo.clear();
    mRawTempoInf.clear();
    mRawTempoSup.clear();

    mTempo.clear();
    mTempoInf.clear();
    mTempoSup.clear();

    mRawActivity.clear();
    mRawActivityInf.clear();
    mRawActivitySup.clear();

    mActivity.clear();
    mActivityInf.clear();
    mActivitySup.clear();

    mRawActivityUnifTheo.clear();
    mActivityUnifTheo.clear();
}
// Properties

/**
 * @todo Check the copy of the color if mJson is not set
 */
Phase Phase::fromJson(const QJsonObject &json)
{
    Phase p;
    p.mId = json.value(STATE_ID).toInt();
    p.setName(json.value(STATE_NAME).toString());
    p.mColor = QColor(json.value(STATE_COLOR_RED).toInt(), json.value(STATE_COLOR_GREEN).toInt(), json.value(STATE_COLOR_BLUE).toInt());

    p.mItemX = json.value(STATE_ITEM_X).toDouble();
    p.mItemY = json.value(STATE_ITEM_Y).toDouble();
    p.mTauType = (Phase::TauType)json.value(STATE_PHASE_TAU_TYPE).toInt();
    p.mTauFixed = json.value(STATE_PHASE_TAU_FIXED).toDouble();
    p.mTauMin = json.value(STATE_PHASE_TAU_MIN).toDouble();
    p.mTauMax = json.value(STATE_PHASE_TAU_MAX).toDouble();
    p.mIsSelected = json.value(STATE_IS_SELECTED).toBool();
    p.mIsCurrent = json.value(STATE_IS_CURRENT).toBool();

    p.mAlpha.setName("Begin of Phase : " + p._name);
    p.mBeta.setName("End of Phase : " + p._name);
    p.mTau.setName("Tau of Phase : " + p._name);
    p.mDuration.setName("Duration of Phase : " + p._name);

    p.mValueStack.emplace("Activity_TimeRange_Level", 0.);
    p.mValueStack.emplace("Activity_TimeRange_min", 0.);
    p.mValueStack.emplace("Activity_TimeRange_max", 0.);

    return p;
}

QJsonObject Phase::toJson() const
{
    QJsonObject phase;

    phase[STATE_ID] = mId;
    phase[STATE_NAME] = getQStringName();
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

#pragma mark INIT
double Phase::sum_gamma_prev_phases()
{
    double max_prev = 0.;
    if (mConstraintsPrevPhases.empty())
        return 0.;
    else {
        for(const auto &prev_c : mConstraintsPrevPhases) {
            max_prev = std::max(max_prev, prev_c->mGamma + prev_c->mPhaseFrom->sum_gamma_prev_phases());
        }
        return max_prev;
    }

}

double Phase::sum_gamma_next_phases()
{
    double max_next = 0.;
    if (mConstraintsNextPhases.empty())
        return 0.;
    else {
        for(const auto &prev_c : mConstraintsNextPhases) {
            max_next = std::max(max_next, prev_c->mGamma + prev_c->mPhaseTo->sum_gamma_next_phases());
        }
        return max_next;
    }
}


void Phase::init_alpha_beta_phase(std::vector<std::shared_ptr<Phase>> &phases)
{
    auto model = getModel_ptr();
    for (auto phase : phases) {
        phase->mAlpha.mX = model->mSettings.mTmin + phase->sum_gamma_prev_phases();
        phase->mBeta.mX = model->mSettings.mTmax - phase->sum_gamma_next_phases();
    }
}

void Phase::init_update_alpha_phase(double theta_max_phase_prev)
{
    if (mConstraintsNextPhases.empty())
        return;
    else {
        for (const auto &prev_c : mConstraintsNextPhases) {
            if (mTauType != Phase::TauType::eTauUnknown) {
                prev_c->mPhaseTo->mAlpha.mX = std::max(prev_c->mPhaseTo->mAlpha.mX, theta_max_phase_prev  + prev_c->mGamma);
                qDebug()<<"[Phase::init_update_alpha_phase] mise à jour alpha des phases Sup " <<prev_c->mPhaseTo->getQStringName()<<" init alpha ="<<prev_c->mPhaseTo->mAlpha.mX;
            }
        }
        return;
    }
}

void Phase::init_update_beta_phase(double beta_sup)
{
    if (mConstraintsPrevPhases.empty())
        return;
    else {
        for(const auto &prev_c : mConstraintsPrevPhases) {
            if (mTauType != Phase::TauType::eTauUnknown) {
                prev_c->mPhaseFrom->mBeta.mX = std::max(prev_c->mPhaseFrom->mBeta.mX, beta_sup - prev_c->mGamma);
                qDebug()<<"[Phase::init_update_beta_phase] mise à jour beta des phases Inf " <<prev_c->mPhaseFrom->getQStringName()<<" init beta ="<<prev_c->mPhaseFrom->mBeta.mX;
            }
        }
        return;
    }
}

/**
 * @brief Phase::init_max_theta, permet de retrouver la valeur la plus grande.
 * La plus grande valeur peut venir des Events inf dans la strati
 * @param max_default : C'est la valeur la plus petite
 * @return
 */
double Phase::init_max_theta(const double max_default)
{
    double theta = max_default;
    for (auto ev : mEvents) {
        if (ev->mInitialized)  {
            theta = std::max(theta, ev->mTheta.mX);
        } else {
            theta = std::max(theta, ev->getThetaMinRecursive_v3(max_default));
        }
    }

    return theta;
}

/**
 * @brief Phase::init_min_theta, utilisé dans Event::getThetaMaxRecursive_v3
 * @param min_default
 * @return
 */
double Phase::init_min_theta(const double min_default)
{
    double theta = min_default;
    for (auto ev : mEvents) {
        if (ev->mInitialized)  {
            theta = std::min(theta, ev->mTheta.mX);
        } else {
            theta = std::min(theta, ev->getThetaMaxRecursive_v3(min_default));
        }
    }

    return theta;
}


#pragma mark RUN
double Phase::getMaxThetaEvents(double tmax)
{
    Q_ASSERT_X(!mEvents.empty(), "[Phase::getMaxThetaEvents]", QString("No Event in Phase :" + this->getQStringName()).toStdString().c_str());
    (void) tmax;
    double theta (mEvents[0]->mTheta.mX);

    // All Event must be Initialized
    std::for_each(mEvents.begin(), mEvents.end(), [&theta] (std::shared_ptr<Event> ev) {theta= std::max(ev->mTheta.mX, theta);});
    return theta;

}
/**
 * @brief Phase::getMinThetaEvents
 * @param tmin
 * @return the min of Event inside the phase, used in Phase::updateAll() to set alpha and beta
 */
double Phase::getMinThetaEvents(double tmin)
{
    Q_ASSERT_X(!mEvents.empty(), "[Phase::getMinThetaEvents]", QString("No Event in Phase :" + getQStringName()).toStdString().c_str());
    (void) tmin;
    double theta (mEvents[0]->mTheta.mX);

    // All Event must be Initialized
    std::for_each(mEvents.begin(), mEvents.end(), [&theta] (std::shared_ptr<Event> ev){theta= std::min(ev->mTheta.mX, theta);});
    return theta;
}


double Phase::getMinThetaNextPhases(const double tmax)
{
    double minTheta = tmax;
    for (auto &&constFwd : mConstraintsNextPhases) {
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

    for (auto &&constBwd : mConstraintsPrevPhases) {
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
                    return QObject::tr("Uniform Span");
                break;
            default:
                    return QObject::tr("Tau Undefined -> Error");
                break;
        }

}

void Phase::initTau(const double tminPeriod, const double tmaxPeriod)
{
    if (mTauType == eTauFixed && mTauFixed != 0.)
        mTau.mX = mTauFixed;

    else if (mTauType == eZOnly) {
            // Modif PhD ; initialisation arbitraire
            mTau.mX = tmaxPeriod - tminPeriod;
            // nothing to do
        }
    else if (mTauType == eTauUnknown) {
        mTau.mX = 0.;
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
#ifdef DEBUG
    if (isnan(P2+som)) {
        qDebug()<<"Px is Nan "<<x<<" "<<n<<" "<<Rp;
    }
#endif
    return(std::move(P2+som) );

}
void Phase::update_Tau(const double tminPeriod, const double tmaxPeriod)
{
    if (mTauType == eTauFixed && mTauFixed != 0.) {
        mTau.mX = mTauFixed;

    } else if (mTauType == eZOnly) {
            // Modif PhD 2022
            // model definition

        const int n = (int) mEvents.size();
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
    mDuration.memo();
   // if (mTauType == eZOnly)
     //   mTau.memo();


#ifdef DEBUG
    if (mBeta.mX - mAlpha.mX < 0.)
        qDebug()<<"[Phase::memoAll] : "<<getQStringName()<<" Warning mBeta.mX - mAlpha.mX<0";
#endif
}

void Phase::generateHistos(const std::vector<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{   
    mAlpha.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
    mBeta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
    // if (mTauType == eZOnly)
    //   mTau.generateHistos(chains, fftLen, bandwidth);
    mDuration.generateHistos(chains, fftLen, bandwidth);
}



void Phase::generateActivity(size_t gridLength, double h, const double threshold, const double timeRangeLevel)
{
#ifdef DEBUG
    QElapsedTimer tClock;
    tClock.start();
#endif

    auto model = getModel_ptr();

    const auto& s = model->mSettings;

    // prevent h=0
    if (h <= 0) {
        h = ( s.mTmax - s.mTmin) / 1000.0;
    }

    // Avoid to redo calculation, when mActivity exist, it happen when the control is changed
    if (!mRawActivity.empty() && gridLength == mValueStack.at("Activity_GridLength")
            && h == mValueStack.at("Activity_h")
            && threshold == mValueStack.at("Activity_Threshold")
            && timeRangeLevel == mValueStack.at("Activity_TimeRange_Level"))
       return;

    mValueStack.insert_or_assign("Activity_GridLength", gridLength);
    mValueStack.insert_or_assign("Activity_h", h);
    mValueStack.insert_or_assign("Activity_Threshold", threshold);
    mValueStack.insert_or_assign("Activity_max", 0.0);
    mValueStack.insert_or_assign("Activity_mode", 0.0);

    const int n = static_cast<int>(mEvents.size());

    if (n < 1) {
        mValueStack.insert_or_assign("Activity_Significance_Score", 0.0);
        mValueStack.insert_or_assign("R_etendue", ( s.mTmax - s.mTmin));
        mValueStack.insert_or_assign("t_min", s.mTmin);
        mValueStack.insert_or_assign("t_max", s.mTmax);

        mValueStack.insert_or_assign("Activity_TimeRange_min", s.mTmin);
        mValueStack.insert_or_assign("Activity_TimeRange_max", s.mTmax);

        mValueStack.insert_or_assign("Activity_min95", s.mTmin);
        mValueStack.insert_or_assign("Activity_max95", s.mTmax);
        mValueStack.insert_or_assign("Activity_mean95", (s.mTmax + s.mTmin)/2.0);
        mValueStack.insert_or_assign("Activity_std95", 0.0);
        return;
    }

    // Curves for error binomial


    // clear old results
    mRawActivityUnifTheo.clear();
    mRawActivity.clear();
    mRawActivityInf.clear();
    mRawActivitySup.clear();

    const std::vector<double>& Gx = model->mBinomiale_Gx.at(n);

    //---- timeRange

    if (mValueStack.at("Activity_TimeRange_min") == mValueStack.at("Activity_TimeRange_max")
        || mValueStack.at("Activity_TimeRange_Level") != timeRangeLevel) {
        const std::pair<double, double> timeRange = timeRangeFromTraces( mAlpha.fullRunRawTrace(model->mChains),
                                                                         mBeta.fullRunRawTrace(model->mChains), timeRangeLevel, "Time Range for Phase : " + getQStringName());
        mValueStack.insert_or_assign("Activity_TimeRange_min", timeRange.first);
        mValueStack.insert_or_assign("Activity_TimeRange_max", timeRange.second);
    }

    mValueStack.insert_or_assign("Activity_TimeRange_Level", timeRangeLevel);

    const double TimeRange_min = mValueStack.at("Activity_TimeRange_min");
    const double TimeRange_max = mValueStack.at("Activity_TimeRange_max");
    std::vector<double> concaTrace;

    double min95 = +INFINITY;
    double max95 = -INFINITY;
    // Ajout artificiel des events et bornes fixes
    const int nRealyAccepted = std::accumulate(model->mChains.begin(), model->mChains.end(), 0, [] (int sum, ChainSpecs c) {return sum + c.mRealyAccepted;});

    for (const auto& ev : mEvents) {
        if (ev->mTheta.mSamplerProposal != MHVariable::eFixe) {
            const auto &rawtrace = ev->mTheta.fullRunRawTrace(model->mChains);

            std::copy_if(rawtrace.begin(), rawtrace.end(),
                         std::back_inserter(concaTrace),
                         [TimeRange_min, TimeRange_max](double x) { return (TimeRange_min<= x && x<= TimeRange_max); });
        } else {
            const size_t begin_size = concaTrace.size();
            concaTrace.resize(concaTrace.size() + nRealyAccepted);
            std::fill_n( concaTrace.begin()+begin_size, nRealyAccepted, ev->mTheta.mRawTrace->at(0));
        }

    }


    double nr = static_cast<double>(concaTrace.size());
    if (nr <= 0.0) {
        qWarning() << "[Phase::generateActivity] concaTrace empty after filtering -> no data in time range";
        mValueStack.insert_or_assign("Activity_Significance_Score", 0.0);
        mValueStack.insert_or_assign("Activity_max", 0.0);
        mValueStack.insert_or_assign("Activity_mode", TimeRange_min);
        // vider & retourner
        mRawActivity.clear(); mRawActivityInf.clear(); mRawActivitySup.clear(); mRawActivityUnifTheo.clear();
        mActivity.clear(); mActivityInf.clear(); mActivitySup.clear(); mActivityUnifTheo.clear();
        return;
    }

    const auto &minmax95 = std::minmax_element(concaTrace.begin(), concaTrace.end());
    min95 = *minmax95.first;
    max95 = *minmax95.second;

    mValueStack.insert_or_assign("Activity_min95", min95);
    mValueStack.insert_or_assign("Activity_max95", max95);

    if (min95 == max95) { // happen when there is only one bound in the phase ???

        mValueStack.insert_or_assign("Activity_Significance_Score", 0.0);
        mValueStack.insert_or_assign("R_etendue",  0.0);
        mValueStack.insert_or_assign("a_Unif", min95);
        mValueStack.insert_or_assign("b_Unif", max95);

        mValueStack.insert_or_assign("Activity_mean95", min95);
        mValueStack.insert_or_assign("Activity_std95", 0.);


    } else {

        double mean95, var95;
        mean_variance_Knuth(concaTrace, mean95, var95);
        mValueStack.insert_or_assign("Activity_mean95", mean95);
        mValueStack.insert_or_assign("Activity_std95", sqrt(var95));

    }

    const double R_etendue =  max95 - min95;

    // prevent h > R_etendue;
    if (R_etendue > 0) {
        h = std::min( h,  R_etendue) ;
    }

    mValueStack.insert_or_assign("Activity_h", h);

    const double h_2 = h/2.0;

    const double fUnif = n > 1 ? h / R_etendue : 1.0;

    const double mid_R =  (max95 + min95) /2.0;

    const double ActivityUnif = fUnif * n / h; //  remplace -> fUnif * n / std::min(h, R_etendue);

    const double half_etendue = R_etendue /2.0 ;

    // Recentrage de a_Unif et b_Unif
    // Variable pour courbe Unif Théo

    const double a_Unif = mid_R - half_etendue;
    const double b_Unif = mid_R + half_etendue;

    // L'unif théorique est définie par le trapéze correspondant à l'unif modifiée par la fenètre mobile
    const double a_Unif_minus_h_2 = a_Unif - h_2;
    const double a_Unif_plus_h_2 = a_Unif + h_2;

    const double b_Unif_minus_h_2 = b_Unif - h_2;
    const double b_Unif_plus_h_2 = b_Unif + h_2;

    mValueStack.insert_or_assign("a_Unif", a_Unif);
    mValueStack.insert_or_assign("b_Unif", b_Unif);
    mValueStack.insert_or_assign("R_etendue", R_etendue);


    /// Look for the maximum span containing values \f$ x=2 \f$

    if (min95 == max95) { // happen when there is only one bound in the phase ???
        qDebug()<<"[Phase::generateActivity] min95 == max95 : " << getQStringName();
        mRawActivity.emplace(a_Unif_minus_h_2,  1.0/h);
        mRawActivity.emplace(b_Unif_plus_h_2, 1.0/h);

        mRawActivityInf = mRawActivity;
        mRawActivitySup = mRawActivity;

        // Convertion in the good Date format
        mActivity = DateUtils::convertMapToAppSettingsFormat( mRawActivity);
        mActivityInf = mActivity;
        mActivitySup = mActivity;

        mRawActivityUnifTheo = mRawActivity;

        mActivityUnifTheo = mActivity;

        mValueStack.insert_or_assign("Activity_Significance_Score", 0.0);
        mValueStack.insert_or_assign("Activity_max",  1.0/h);
        mValueStack.insert_or_assign("Activity_mode", a_Unif);

        return;
    }

    mRawActivityUnifTheo.emplace(a_Unif_minus_h_2,  0.0);
    mRawActivityUnifTheo.emplace(a_Unif_plus_h_2,  ActivityUnif);

    mRawActivityUnifTheo.emplace(b_Unif_minus_h_2,  ActivityUnif);
    mRawActivityUnifTheo.emplace(b_Unif_plus_h_2,  0.0);

#ifdef DEBUG
    if (max95 > s.mTmax) {
        qWarning("[Phase::generateActivity] max95 > mSettings.mTmax force max95 = mSettings.mTmax");
    }

#endif

    const double t_min_grid = TimeRange_min - h_2;
    const double t_max_grid = TimeRange_max + h_2;

    /// \f$ \delta_t = (max95 - min95 + h)/(gridLenth-1) \f$
    const double delta_t = (t_max_grid - t_min_grid) / double(gridLength-1);

    // overlaps

    const int maxGrid = (int)gridLength-1;
    // Loop
    std::vector<int> NiTot (gridLength);
    try {
        for (const auto& t : concaTrace) {

            int idxGridMin = std::clamp((int) ceil((t - t_min_grid - h_2) / delta_t), 0, maxGrid) ;

            if ((t - t_min_grid - h_2) / delta_t == (double) idxGridMin && (t - t_min_grid - h_2)>0) {
                ++idxGridMin;
            }
            const int idxGridMax = std::clamp((int) floor((t - t_min_grid + h_2) / delta_t), 0, maxGrid) ;

            for (auto&& ni = NiTot.begin() + idxGridMin; ni != NiTot.begin() + idxGridMax +1; ++ni) {
                ++*ni ;
            }

        }

    } catch (std::exception& e) {
        qWarning()<< "[Phase::generateActivity] exception caught: " << e.what() << '\n';

    } catch(...) {
        qWarning() << "[Phase::generateActivity] Caught Exception!\n";

    }


    ///# Calculation of the mean and variance

    std::vector<double> esp;
    std::vector<double> esp_inf;
    std::vector<double> esp_sup;
    double maxActivity = 0.0;
    double modeActivity = t_min_grid;

    double UnifScore = 0.;
    int nbIt = 0;

    // Preallocate memory for esp, esp_inf, and esp_sup if the size is known
    esp.reserve(gridLength);
    esp_inf.reserve(gridLength);
    esp_sup.reserve(gridLength);

    for (const auto& ni : NiTot) {
        // La grille est définie entre TimeRange_min - h_2 et TimeRange_max + h_2 avec gridlength case
        //
        const double t = nbIt * delta_t + t_min_grid ;

        const double fA = static_cast<double>(ni) / nr;
        const double eA =  fA * n / h;
        esp.push_back(eA);
        if (eA > maxActivity) {
            maxActivity =  eA;
            modeActivity = t;
        }

        double QInf, QSup;
        if (n == 1) {
            QInf = eA;
            QSup = eA;

        } else {
            QSup = interpolate_value_from_curve(fA, Gx, 0, 1.)* n / h;
            QInf = findOnOppositeCurve(fA, Gx)* n / h;
        }
#ifdef DEBUG
        if (QSup < QInf) {
            qDebug()<<"[Phase::generateActivity] QSup < QInf ; f= "<<fA<< " ; QSup = "<<QSup<<" ; QInf = "<<QInf;
        }
#endif
        esp_inf.push_back(QInf);
        esp_sup.push_back(QSup);
        // Calcul du score


        double dUnif = 0.0;
        if ((a_Unif_minus_h_2 < t) && (t < a_Unif_plus_h_2)) {
            dUnif = interpolateValueInStdMap(t, mRawActivityUnifTheo);

        } else if ((a_Unif_plus_h_2 <= t) && (t <= b_Unif_minus_h_2)) {
            dUnif = ActivityUnif;

        } else if ((b_Unif_minus_h_2 < t) && (t < b_Unif_plus_h_2)) {
            dUnif = interpolateValueInStdMap(t, mRawActivityUnifTheo);

        }
       /* auto addunif = std::max(dUnif, QInf) - std::min(dUnif, QSup);
        if (addunif>0) {
            qDebug() <<" t="<< t << " dUnif=" <<dUnif << " QInf=" <<QInf << " QSup="<< QSup << " t_min_grid="<< t_min_grid << "t_max_grid=" << t_max_grid << " a_Unif_minus_h_2=" << a_Unif_minus_h_2 << " b_Unif_plus_h_2=" << b_Unif_plus_h_2;
        }*/

        UnifScore += std::max(dUnif, QInf) - std::min(dUnif, QSup);

        nbIt++;
    }

    // nouvelle formule: Transformation du calcul du score en utilisant le log, pour se retrouver entre 0 et 1
    // UnifScore peut être enter n et 0.
    // Utilisation du log permet une meilleur lecture des valeurs
    // Score = ln(1+UnifScore) / ln(n+1)
    /**
     * @brief Calcule le score de signification de l'activité ("Activity_Significance_Score").
     *
     * Le calcul s'effectue en deux étapes :
     *
     * 1. **Accumulation de l'écart avec l'uniforme** :
     *    \f[
     *    \text{UnifScore} \;=\;
     *    \sum_{i}
     *    \left[
     *        \max\!\bigl(d_{\text{Unif}}(t_i), Q_{\inf}(t_i)\bigr)
     *        \;-\;
     *        \min\!\bigl(d_{\text{Unif}}(t_i), Q_{\sup}(t_i)\bigr)
     *    \right]
     *    \f]
     *
     *    où :
     *    - \f$d_{\text{Unif}}(t_i)\f$ est la valeur théorique de la loi uniforme au temps \f$t_i\f$,
     *    - \f$Q_{\inf}(t_i)\f$ et \f$Q_{\sup}(t_i)\f$ sont respectivement les bornes
     *      inférieure et supérieure de l'activité estimée.
     *
     * 2. **Score final normalisé** :
     *    \f[
     *    \text{Activity\_Significance\_Score} \;=\;
     *    \frac{\ln\!\left(1 + \Delta t \cdot \text{UnifScore}\right)}{\ln(n+1)}
     *    \f]
     *
     *    avec :
     *    - \f$\Delta t\f$ le pas de discrétisation,
     *    - \f$n\f$ le nombre d'événements de la phase considérée.
     *
     * @note La normalisation par \f$\ln(n+1)\f$ garantit que le score reste dans [0,1].
     */

    mValueStack.insert_or_assign("Activity_Significance_Score", log(1+ UnifScore * delta_t)/log(static_cast<double>(n+1) ));

    mValueStack.insert_or_assign("Activity_max", maxActivity);
    mValueStack.insert_or_assign("Activity_mode", modeActivity);

    auto acti = vector_to_map(esp, t_min_grid, t_max_grid, delta_t);

    mRawActivity.swap(acti);
    acti = vector_to_map(esp_inf, t_min_grid, t_max_grid, delta_t);
    mRawActivityInf.swap(acti);
    acti = vector_to_map(esp_sup, t_min_grid, t_max_grid, delta_t);
    mRawActivitySup.swap(acti);

    const double QSup = interpolate_value_from_curve(0., Gx, 0, 1.)* n / h;
    const double QInf = findOnOppositeCurve(0., Gx)* n / h;

    mRawActivitySup.emplace(t_min_grid, QSup );
    mRawActivitySup.emplace(t_max_grid, QSup );


    mRawActivityInf.emplace(t_min_grid, QInf);
    mRawActivityInf.emplace(t_max_grid, QInf);


    acti = DateUtils::convertMapToAppSettingsFormat(mRawActivity);
    mActivity.swap(acti);

    acti = DateUtils::convertMapToAppSettingsFormat(mRawActivityInf);
    mActivityInf.swap(acti);

    acti = DateUtils::convertMapToAppSettingsFormat(mRawActivitySup);
    mActivitySup.swap(acti);

    acti = DateUtils::convertMapToAppSettingsFormat(mRawActivityUnifTheo);
    mActivityUnifTheo.swap(acti);

#ifdef DEBUG
    qDebug() <<  QString("[Phase::generateActivity] done in " + DHMS(tClock.elapsed()));

#endif

}
