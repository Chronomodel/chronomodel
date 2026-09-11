/* ---------------------------------------------------------------------
Copyright or © or Copr. CNRS	2014 - 2026

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

#include "MCMCLoop.h"

#include "Bound.h"
#include "CalibrationCurve.h"
#include "Generator.h"
#include "Project.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
#include "StdUtilities.h"
#include "AppSettings.h"


#include <QDebug>
#include <QTime>

#include <QtWidgets>

#ifdef _WIN32
//#include "winbase.h"
#include <windows.h> //for Qt 6.7
#endif


MCMCLoop::MCMCLoop(std::shared_ptr<ModelCurve> model):
    mChainIndex (0),
    mState (State::eCalibrating)
{
    mModel = model;
    mAbortedReason = QString();
#ifdef _WIN32
    DWORD process_id =GetCurrentProcessId();
    HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (process_handle != NULL) {
        SetPriorityClass(process_handle, NORMAL_PRIORITY_CLASS);
        CloseHandle(process_handle);
    }
#endif

}

MCMCLoop::~MCMCLoop()
{
}

void MCMCLoop::setMCMCSettings(const MCMCSettings &s)
{
    mLoopChains.clear();
    for (int i = 0; i < s.mNumChains; ++i) {
        ChainSpecs chain;

        //if (i < s.mSeeds.size())
        //    chain.mSeed = s.mSeeds.at(i);
       // else // done in initialise_time
       //     chain.mSeed = Generator::createSeed();

        chain.mIterPerBurn = s.mIterPerBurn;
        chain.mBurnIterIndex = 0;
        chain.mMaxBatchs = s.mMaxBatches;
        chain.mIterPerBatch = s.mIterPerBatch;
        chain.mBatchIterIndex = 0;
        chain.mBatchIndex = 0;
        chain.mIterPerAquisition = s.mIterPerAquisition;
        chain.mAquisitionIterIndex = 0;
        chain.mTotalIter = 0;
        chain.mThinningInterval = s.mThinningInterval;
        chain.mRealyAccepted = 0;
        chain.mMixingLevel = s.mMixingLevel;
        mLoopChains.push_back(chain);
    }
}

const std::vector<ChainSpecs> &MCMCLoop::chains() const
{
    return mLoopChains;
}

QString MCMCLoop::initialize_time()
{
    tminPeriod = mModel->mSettings.mTmin;
    tmaxPeriod = mModel->mSettings.mTmax;

    std::vector<std::shared_ptr<Event>> &allEvents (mModel->mEvents);
    std::vector<std::shared_ptr<Phase>> &phases (mModel->mPhases);
    std::vector<std::shared_ptr<PhaseConstraint>> &phasesConstraints (mModel->mPhaseConstraints);

    if (isInterruptionRequested())
        return ABORTED_BY_USER;
    // initialisation des bornes
    // ---------------------- Reset Events ---------------------------
    for (std::shared_ptr<Event> &ev : allEvents) {
        ev->mInitialized = false;

        if (AppSettings::mEventModel == EventModelType::EDM2)
            ev->mS02Theta.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

        else
            ev->mS02Theta.mSamplerProposal = SamplerProposal::eFixe;

    }
    // -------------------------- Init gamma ------------------------------
    emit stepChanged(tr("Initializing Phase Gaps..."), 0, (int)phasesConstraints.size());
    int Ni = 0;
    try {
        for (auto&& phC : phasesConstraints) {
            phC->initGamma();
            if (isInterruptionRequested())
                return ABORTED_BY_USER;
            emit stepProgressed(++Ni);
        }
    }  catch (...) {
        qWarning() <<"Init Gamma ???";
        mAbortedReason = QString("Error in Init Gamma ???");
        return mAbortedReason;
    }

    // -------------------------- Init tau -----------------------------------------
    emit stepChanged(tr("Initializing Phase Durations..."), 0, (int)phases.size());
    Ni = 0;
    try {
        for (auto&& ph : phases) {
            ph->initTau(tminPeriod, tmaxPeriod);
            qDebug() << "[MCMCLoop::initialize_time] " << ph->getQStringName() << " init Tau =" << ph->mTau.value();
            if (isInterruptionRequested())
                return ABORTED_BY_USER;

            emit stepProgressed(++Ni);
        }
    }  catch (...) {
        qWarning() << "Init Tau ???";
        mAbortedReason = QString("Error in Init Tau ???");
        return mAbortedReason;
    }
    // --------------------------  Init Bounds --------------
    try {
        for (std::shared_ptr<Event> &ev : allEvents) {
            if (ev->mType == Event::eBound) {
                Bound* bound = dynamic_cast<Bound*>(ev.get());

                if (bound) {
                    bound->mTheta.setValue(bound->value());
                    bound->mThetaReduced = mModel->reduceTime(bound->mTheta.value());
                    bound->mTheta.mLastMHAccepts.clear();

                    bound->mTheta.recordBurnAdapt(); // utile pour creer FormatedTrace
                    bound->mTheta.acquire(); // non sauvegarder dans Loop.memo()
                    bound->mInitialized = true;
                    bound->mTheta.mSamplerProposal = SamplerProposal::eFixe;
                    qDebug() << QString("[MCMCLoop::initialize_time] Init for Bound : %1  ->theta = %4 thetaRed = %5-------").arg(bound->getQStringName(), QString::number(bound->mTheta.value(), 'f', 3), QString::number(bound->mThetaReduced, 'f', 3));
                    bound->mS02Theta.mSamplerProposal = SamplerProposal::eFixe;
                }
                bound = nullptr;
            }
        }

    }  catch (...) {
        qWarning() << "Init Bound ???";
        mAbortedReason = QString("Error in Init Bound ???");
        return mAbortedReason;
    }
    /* ----------------------------------------------------------------
     *  Init theta event, ti, ...
     * ---------------------------------------------------------------- */

    std::vector<std::shared_ptr<Event>> unsortedEvents = ModelUtilities::unsortEvents(allEvents);

    try {
        int Ni = 0;
        int N = unsortedEvents.size();


        emit stepChanged(tr("Initializing Events..."), 0, N);

        // Check Strati constraint
        for (std::shared_ptr<Event> &ev : unsortedEvents) {
            mModel->initNodeEvents();
            QString circularEventName = "";
            std::vector<Event*> startEvents = std::vector<Event*>();

            const bool ok (ev->getThetaMaxPossible (ev.get(), circularEventName, startEvents));
            if (!ok) {
                mAbortedReason = QString(tr("Warning : Find Circular Constraint Path %1  %2 ")).arg (ev->getQStringName(), circularEventName);
                return mAbortedReason;
            }

            // Controle la cohérence des contraintes strati-temporelle et des valeurs de profondeurs
            if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth ) {
                for (auto&& eForWard : ev->mConstraintsFwd) {
                    const bool notOk (ev->mXIncDepth > eForWard->mEventTo->mXIncDepth);
                    if (notOk) {
                        mAbortedReason = QString(tr("Warning: chronological constraint not in accordance with the stratigraphy: %1 - %2 path, control depth value!")).arg (ev->getQStringName(), eForWard->mEventTo->getQStringName());
                        return mAbortedReason;
                    }
                }
            }
        }

        // nouveau code 2024
        // initalize alpha beta phase
        // On regarde les gamma entre les phases, pour initialiser les alpha et beta
        for (auto p : phases) {
            p->init_alpha_beta_phase(phases);
            qDebug() << "[MCMCLoop::initialize_time] " << p->getQStringName() << " init alpha =" << p->mAlpha.value() << " beta=" << p->mBeta.value();
        }

        //---------------
#pragma mark Init Theta Bayesian
        if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {

            for (std::shared_ptr<Event> uEvent : unsortedEvents) {
#ifdef FIXEDPRIOR
                uEvent->mTheta.mSamplerProposal = SamplerProposal::eEventPrior;
#endif
                emit stepProgressed(++Ni);
                emit setMessage(tr("Initializing Event : %1 / %2").arg(QString::number(Ni), QString::number(N)));

                if (uEvent->mType == Event::eDefault) {

                    mModel->initNodeEvents();
                    const double min = uEvent->getThetaMinRecursive_v3(tminPeriod);
                    mModel->initNodeEvents();
                    const double max = uEvent->getThetaMaxRecursive_v3(tmaxPeriod);

                    if (min > max) {
                        const int seed = mLoopChains.at(mChainIndex).mSeed;
                        qDebug() << QString("[MCMCLoop::initialize_time] Error Init for event : %1 : min = %2 : max = %3-------Seed = %4").arg(uEvent->getQStringName(), QString::number(min, 'f', 30), QString::number(max, 'f', 30), QString::number(seed));
                        mAbortedReason = QString(tr("Error Init for event : %1 \n min = %2 \n max = %3 \n Seed = %4").arg(uEvent->getQStringName(), QString::number(min, 'f', 6), QString::number(max, 'f', 6), QString::number(seed)));
                        return mAbortedReason;
                    }
                    // ----------------------------------------------------------------
                    // Curve init Theta event :
                    // On initialise les theta près des dates ti
                    // ----------------------------------------------------------------
                    CalibrationCurve mixCal = generate_mixingCalibration(uEvent->mDates);
                    uEvent->mMixingCalibrations = std::make_shared<CalibrationCurve>(mixCal);
                    double try_theta;

                    if (max == min) {
                        try_theta = min;
                        qDebug() << QString("[MCMCLoop::initialize_time] Egality Init for event : %1 : min = %2 : max = %3-------Seed = %4").arg(uEvent->getQStringName(), QString::number(min, 'f', 30), QString::number(max, 'f', 30), QString::number(mLoopChains.at(mChainIndex).mSeed));

                    } else {

                        try_theta = *sample_in_repartition(uEvent->mMixingCalibrations, min, max);

                    }
                    if (try_theta > max || try_theta < min) {
                        const int seed = mLoopChains.at(mChainIndex).mSeed;
#ifdef DEBUG
                        std::cerr << "[" << __func__ << "] "
                                  << "‼️ Error Init for event "
                                  << uEvent->name()
                                  << " : mTheta.mX > max || mTheta.mX < min : "
                                  << "min = " << std::fixed << std::setprecision(30) << min
                                  << " : max = " << std::fixed << std::setprecision(30) << max
                                  << "  🍇 Seed = " << seed
                                  << std::endl;

                        mAbortedReason = QString(tr("uEvent->mTheta.mX > max || uEvent->mTheta.mX < min Error Init for event : %1 \n min = %2 \n max = %3 \n Seed = %4").arg(uEvent->getQStringName(), QString::number(min, 'f', 6), QString::number(max, 'f', 6), QString::number(seed)));
#else
                        mAbortedReason = QString(tr("Error Init for event : %1 \n min = %2 \n max = %3 \n Seed = %4").arg(uEvent->getQStringName(), QString::number(min, 'f', 6), QString::number(max, 'f', 6), QString::number(seed)));
#endif
                        return mAbortedReason;
                    }
                    // 6- Clear mLastMHAccepts  array
                    uEvent->mTheta.mLastMHAccepts.clear();
                    //unsortedEvents.at(i)->mTheta.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                    uEvent->mTheta.accept_update(try_theta);

                    uEvent->mThetaReduced = mModel->reduceTime(uEvent->mTheta.value());
                    uEvent->mInitialized = true;

                    // ------- debug init
#ifdef DEBUG
                    constexpr int colW = 20;
                    constexpr int prec = 3;


                    std::cout << "[" << __func__ << "] Init for event : "
                              << std::setw(colW) << std::left
                              << uEvent->name()

                              << std::setw(colW) << std::left
                              << "min = "
                              << std::fixed << std::setprecision(prec)
                              << min

                              << std::setw(colW) << std::left
                              << "max = "
                              << std::fixed << std::setprecision(prec)
                              << max

                              << std::setw(colW) << std::left
                              << "theta = "
                              << std::fixed << std::setprecision(prec)
                              << uEvent->mTheta.value()

                              << std::setw(colW) << std::left
                              << "thetaRed = "
                              << std::fixed << std::setprecision(prec + 3)
                              << uEvent->mThetaReduced

                              << std::endl;
#endif

                    // ----------------------------------------------------------------


                    double s02_sum = 0.;
                    auto sigmaU = (tmaxPeriod - tminPeriod) / 2.0;
#pragma mark Init ti
                    double minVarianceIntra_ti = +INFINITY;
                    for (Date& date : uEvent->mDates) {

                        // 1 - Init ti
                        const bool is_wiggle = date.mDeltaType != Date::eDeltaNone;
#ifdef FIXEDPRIOR
                        date.mTi.mSamplerProposal = SamplerProposal::eLikelihood;
#endif
                        // Recherche de la variance intra calibration la plus petite
                        const double t_min = date.mCalibration->mTmin;
                        const double step = date.mCalibration->mStep;
                        const size_t size = date.mCalibration->mVector.size();
                        std::vector<double> x(size);
                        for (size_t i = 0; i < size; ++i) {
                            x[i] = t_min + i * step;
                        }

                        auto varianceIntra = computeIntraModeVarianceMin(x, date.mCalibration->mVector, 3);
                        if (!varianceIntra) {
                            std::cerr << "[" << __func__ << "] date : " << date.name() << " ❌ Pas de mode détecté – vérifier le seuil ou la densité des points.\n";

                        } else {
                            minVarianceIntra_ti = std::min(*varianceIntra, minVarianceIntra_ti);
                            // std::cout << "[" << __func__ << "] date : " << date.name() << " sqrt(min variance intra ti) = " << sqrt(*varianceIntra)  << std::endl ;
                        }

                        const DensityStat &data = analyseDensity(date.mCalibration->mMap);
                        double sigma = data.std;
#ifdef DEBUG
                        if (sigma == 0.)
                            return  QString("[ %1 ] date : %2 ❌ sigma == 0") .arg(QString::fromLatin1(__func__),
                                                                               date.getQStringName() );
#endif
#pragma mark Init Delta with wiggle
                        {
                            switch (date.mDeltaType) {
                            case Date::eDeltaNone:
                                date.mDelta = 0.0;
                                break;
                            case Date::eDeltaRange:
                                date.mDelta = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax);
                                break;
                            case Date::eDeltaGaussian:
                                date.mDelta = Generator::normalDistribution(date.mDeltaAverage, date.mDeltaError);
                                break;
                            case Date::eDeltaFixed:
                                date.mDelta = date.mDeltaFixed;
                                break;
                            }

                        }

                        if (!date.mCalibration->mRepartition.empty()) {
                            // On favorise les solutions proches de theta de l'Event
                            std::vector<double> repart_exp_theta (date.mCalibration->mVector.size());
                            double sum_exp = 0.0;
                            for ( size_t i = 0; i < date.mCalibration->mVector.size(); i++) {
                                double exp_theta = dnorm(date.mCalibration->mTmin + i * date.mCalibration->mStep, uEvent->mTheta.value(), sigmaU);
                                sum_exp += exp_theta * date.mCalibration->mVector[i];
                                repart_exp_theta[i] = sum_exp;
                            }

                            const double idx_ti = interpolate_index(Generator::randomUniform(0, sum_exp), repart_exp_theta);
                            double ti_init = std::clamp(date.mCalibration->mTmin + idx_ti * date.mCalibration->mStep,
                                                        date.mCalibration->mTmin,
                                                        date.mCalibration->mTmax);

                            date.mTi.setValue(ti_init);

                            //qWarning() << "[" << __func__ << "] with Repartition ➡️ ti = " << date.mTi.value();

                        } else { // Fallback
                            double mu = (mModel->mSettings.mTmax + mModel->mSettings.mTmin)/2.0;
                            double sig = (mModel->mSettings.mTmax - mModel->mSettings.mTmin)/2.0;
                            double ti_init = Generator::normalDistribution( mu, sig);
                            date.mTi.setValue(ti_init);
                            std::cout << "[" << __func__ << "] ⚠️ without Repartition ➡️ ti = " << date.mTi.value() << std::endl;

                        }


#ifdef FIXEDPRIOR
                        date.mSigmaTi.mSamplerProposal = SamplerProposal::ePrior;
#endif

                        // 3 - Init sigma MH adaptatif of each Data ti
                        // si on utilise un noyau Normalisé Centré
                        // Quand il y a 1 seul date, on revient sur un RW adptatif centré sur ti_old, donc non normalisé
                        if (uEvent->mDates.size() > 1 ) {
                            date.mTi.mSigmaMH = 2.38;

                        } else {
                            date.mTi.mSigmaMH = 2.38 * sigma; // optimum Roberts
                        }
                        // 4 - Clear mLastMHAccepts array and set this init at 100%
                        date.mTi.mLastMHAccepts.clear();
                        //date.mTheta.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                        date.mTi.accept_update(date.mTi.value());

#pragma mark Init SigmaTi and its Sigma_MH
                        // 5 - Init Sigma_i and its Sigma_MH
                        date.mSigmaTi.setValue(std::abs(date.mTi.value() - (uEvent->mTheta.value() - date.mDelta)));


                        if (date.mSigmaTi.value() <= 1.0E-6) {
                            date.mSigmaTi.setValue(1.0E-6); // Add control the 2015/06/15 with PhL

                        }
                        date.mSigmaTi.mSigmaMH = 0.1; // default = 1.0

                        date.mSigmaTi.mLastMHAccepts.clear();
                        date.mSigmaTi.accept_update(date.mSigmaTi.value());

                        // intermediary calculus for the harmonic average
                        s02_sum += 1.0 / (sigma * sigma);


                        // 2 - Init Wiggle matching and Clear mLastMHAccepts array
#pragma mark Init Wiggle
                        date.mWiggle.mLastMHAccepts.clear();
                        date.updateWiggle();
                        //date.mWiggle.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                        date.mWiggle.accept_update(date.mWiggle.value());

                        //constexpr double esp_gamma =  0.5/ 0.5; // mode de la loi gamma(0.5, 0.5)
                       // date.mXi = 1.;//esp_gamma; // Pour changement de variable
                    }

                    // 4 - Init S02 of each Event
#pragma mark Init mS02Theta
                    if (AppSettings::mEventModel == EventModelType::EDM2) {
                        uEvent->mS02Theta.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
                    } else {
                        uEvent->mS02Theta.mSamplerProposal = SamplerProposal::eFixe;
                    }

                    uEvent->mS02Theta.mSigmaMH = 0.1; // default = 1.0

                    uEvent->mS02Theta.mLastMHAccepts.clear();
                    const double S02_harmonique = uEvent->mDates.size() / s02_sum;


#pragma mark Init mBetaS02


                    // const double sqrt_S02_harmonique = sqrt(S02_harmonique);
                    // uEvent->mBetaS02 = 1.004680139*(1 - exp(- 0.0000847244 * pow(sqrt_S02_harmonique, 2.373548593))); // <- Formule de Komlan

                    //uEvent->mBetaS02 = 1.004680139*(1 - exp(- 0.0000847244 * pow(sqrt(minVarianceIntra_ti), 2.373548593)));
                    if (minVarianceIntra_ti > 0.0) {
                        uEvent->mBetaS02 = 1.004680139*(1 - exp(- 0.0000847244 * pow(minVarianceIntra_ti, 1.1867742965))); // simplification des puissance 1.1867742965 = 2.373548593 / 2.0

                    } else {
                        uEvent->mBetaS02 = 1.004680139*(1 - exp(- 0.0000847244 * pow(S02_harmonique, 1.1867742965)));
                    }

#ifdef CODE_KOMLAN
                    // new code
                    //uEvent->mS02Theta.mX = 1.0 / Generator::gammaDistribution(1., uEvent->mBetaS02);
                    uEvent->mS02Theta.accept_update(1.0 / Generator::gammaDistribution(1., uEvent->mBetaS02));

#else
                    //constexpr double scale = 100 * 100 ;
                    if (AppSettings::mEventModel == EventModelType::EDM2 ) {
                        // Protection NaN sur mBetaS02
                        if (std::isnan(uEvent->mBetaS02)) {
                            qWarning() << "[" << __func__ << "] uEvent->mBetaS02 is NaN – aborting update.";
                            mAbortedReason = QString("Init %& : mBetaS02 is NaN  ???").arg(uEvent->getQStringName());
                            return mAbortedReason;

                        }

                        uEvent->mS02Theta.accept_update(uEvent->mBetaS02 / 2.0); // mode de l'inverse gamma

                    } else {
                        uEvent->mS02Theta.accept_update(S02_harmonique);
                    }

#endif

                    // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
                    uEvent->mTheta.mSigmaMH = 2.38 * sqrt(S02_harmonique); // optimum Roberts
                    //uEvent->mAShrinkage = 1.0;


                }

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

            }

        }

#pragma mark Init Theta Fixed
        else { // theta fixe

            for (std::shared_ptr<Event> &uEvent : unsortedEvents) {
                emit stepProgressed(++Ni);
                emit setMessage(tr("Initializing Event : %1 / %2").arg(QString::number(Ni), QString::number(N)));
                uEvent->mTheta.mSamplerProposal = SamplerProposal::eFixe;
                // ----------------------------------------------------------------
                // Curve init Theta event :
                // On initialise les theta près des dates ti
                // ----------------------------------------------------------------
                if (uEvent->mType == Event::eDefault)
                    sampleInCumulatedRepartition_thetaFixe(uEvent, mModel->mSettings);
                else
                    uEvent->mTheta.setValue(static_cast<Bound*>(uEvent.get())->value());
                // nous devons sauvegarder la valeur ici car dans loop.memo(), les variables fixes ne sont pas memorisées.
                // Pourtant, il faut récupèrer la valeur pour les affichages et les stats

                uEvent->mTheta.recordBurnAdapt();
                uEvent->mTheta.acquire(); // il faut faire memo ici, c'est la seule fois. ce ne sera pas fait dans MCMCLoopChrono.memo()

                uEvent->mThetaReduced = mModel->reduceTime(uEvent->mTheta.value());
                uEvent->mInitialized = true;


                for (auto&& date : uEvent->mDates) {
                    date.mTi.mSamplerProposal = SamplerProposal::eFixe;
                    date.mTi.setValue(uEvent->mTheta.value());
                    date.mTi.recordBurnAdapt();
                    date.mTi.acquire();

                    // 2 - Init Delta Wiggle matching and Clear mLastMHAccepts array
                    date.initDelta();
                    date.mWiggle.mSamplerProposal = SamplerProposal::eFixe;
                    date.mWiggle.setValue(date.mTi.value() + date.mDelta);
                    date.mWiggle.recordBurnAdapt();
                    date.mWiggle.acquire();
                    date.mWiggle.mLastMHAccepts.clear();
                    //date.mWiggle.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                    // 3 - Init sigma MH adaptatif of each Data ti
                    date.mTi.mSigmaMH = 0.1; // default = 1.0

                    // 4 - Clear mLastMHAccepts array and set this init at 100%
                    date.mTi.mLastMHAccepts.clear();
                    //date.mTheta.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                    // 5 - Init Sigma_i and its Sigma_MH
                    date.mSigmaTi.mSamplerProposal = SamplerProposal::eFixe;
                    date.mSigmaTi.setValue(0);
                    date.mSigmaTi.recordBurnAdapt();
                    date.mSigmaTi.acquire();

                    date.mSigmaTi.mSigmaMH = 0.1; // default = 1.0

                    date.mSigmaTi.mLastMHAccepts.clear();


                }

                // 4 - Init S02 of each Event fixed
                uEvent->mS02Theta.setValue(0);
                uEvent->mS02Theta.mLastMHAccepts.clear();
                uEvent->mS02Theta.mSamplerProposal = SamplerProposal::eFixe;
                uEvent->mS02Theta.recordBurnAdapt();
                uEvent->mS02Theta.acquire();

                // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
                uEvent->mTheta.mSigmaMH = 0.1; // default = 1.0
                //uEvent->mAShrinkage = 1.0;

                // 6- Clear mLastMHAccepts  array
                uEvent->mTheta.mLastMHAccepts.clear();
                //uEvent->mTheta.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

            }


        }


    }  catch (const QString e) {
        qWarning() <<"Init theta event, ti,  ???"<<e;
        mAbortedReason = e;
        return mAbortedReason;
    }


    // --------------------------- Init alpha and beta phases ----------------------
    emit stepChanged(tr("Initializing Phases..."), 0, (int)phases.size());
    try {
        int Ni = 0;
        for (auto&& phase : phases ) {
            phase->update_All(tminPeriod, tmaxPeriod);
            // tau is still initalize

            /*double tmp = phase->mEvents[0]->mTheta.mX;
            // All Event must be Initialized
            std::for_each(PAR phase->mEvents.begin(), phase->mEvents.end(), [&tmp] (Event* ev){tmp = std::min(ev->mTheta.mX, tmp);});
            phase->mAlpha.mX = tmp;

            tmp = phase->mEvents[0]->mTheta.mX;
            std::for_each(PAR phase->mEvents.begin(), phase->mEvents.end(), [&tmp] (Event* ev){tmp = std::max(ev->mTheta.mX, tmp);});
            phase->mBeta.mX = tmp;

            phase->mDuration.mX = phase->mBeta.mX - phase->mAlpha.mX;
            phase->mTau.mX = phase->mBeta.mX - phase->mAlpha.mX;
            */
            if (isInterruptionRequested())
                return ABORTED_BY_USER;

             emit stepProgressed(++Ni);
        }

    }  catch (...) {
        mAbortedReason = QString("Init alpha and beta phases  ???");
        return mAbortedReason;
    }

    // --------------------------- Check Event FIXED strati constraints----------------------
    if (mCurveSettings.mTimeType != CurveSettings::eModeBayesian) { // Theta fixed
        try {
            for (std::shared_ptr<Event> ev : mModel->mEvents) {
                const double min = ev->getThetaMin(tminPeriod); // need alpha and beta Phase
                const double max = ev->getThetaMax(tmaxPeriod);

                qDebug() << QString("[MCMCLoop::initialize_time] Init for event theta fixed : %1 : min = %2 : max = %3  ->theta = %4 thetaRed = %5-------").arg(ev->getQStringName(), QString::number(min, 'f', 3), QString::number(max, 'f', 3), QString::number(ev->mTheta.value(), 'f', 5), QString::number(ev->mThetaReduced, 'f', 6));

                if (ev->mTheta.value() < min || ev->mTheta.value() > max) {
                    throw QObject::tr("Error for event theta fixed : %1 : min = %2 : max = %3 but Theta = %4" ).arg(ev->getQStringName(), QString::number(min), QString::number(max), QString::number(ev->mTheta.value(), 'f', 3));
                }
            }
        }  catch (const QString e) {
            mAbortedReason = e;
            return mAbortedReason;
        }
    }


    return QString();
}
/**
 * @brief Calcule le score SMC pour une configuration donnée
 *
 * Le score SMC est une mesure de la qualité d'une configuration basée sur :
 * - Les écarts entre les dates observées et prédites
 * - L'ajustement de la courbe si applicable
 *
 * @return double Score SMC compris entre 10^(-15) et 1
 *
 * @details La fonction calcule un score probabiliste qui combine :
 * - Les scores des dates : exp(-distance/tau) où tau est une estimation robuste
 * - Le score de courbe : exp(-variance_résiduelle/variance_référence)
 *
 * Le score est borné pour éviter les valeurs extrêmes et garantir une
 * distribution de poids cohérente pour le rééchantillonnage SMC.
 *
 * @note Le score est utilisé pour la sélection des meilleures graines
 * lors de l'étape d'initialisation SMC.
 *
 * @mathematical_formula
 * \[
 * \text{SMC\_score} = \text{score}_{\text{dates}} \times \text{score}_{\text{curve}}
 * \]
 *
 * Où :
 * \[
 * \text{score}_{\text{dates}} = \sum_{i=1}^{N_{\text{events}}} \begin{cases}
 * 1 & \text{si } \text{event}_i \text{ n'est pas de type Default} \\
 * \frac{1}{|D_i|} \sum_{d \in D_i} \exp\left(-\frac{\sigma_{ti,d}}{\tau}\right) & \text{sinon}
 * \end{cases}
 * \]
 *
 * \[
 * \text{score}_{\text{curve}} = \exp\left(-\frac{\text{var}_{\text{residuel}}}{\text{var}_{\text{reference}}}\right)
 * \]
 *
 * \[
 * \text{SMC\_score} \in [0, 1]
 * \]
 *
 * @pre Les événements doivent être initialisés avec des données valides
 * @pre Les paramètres de variance doivent être positifs
 * @post Le score est toujours dans l'intervalle [0, 1]
 *
 * @warning Les valeurs extrêmes de variance peuvent entraîner des scores très faibles
 * @warning La médiane est utilisée pour une estimation robuste des paramètres
 */
/*double MCMCLoop::SMC_score()
{
    // Calcul de tau pour sigma ti
    double tau = 1.0;
    std::vector<double> sigmas;

    for (const auto& ev : mModel->mEvents) {
        if (ev->mType == Event::eDefault) {
            for (const Date& date : ev->mDates) {
                sigmas.push_back(date.mSigmaTi.value());
            }
        }
    }

    if (!sigmas.empty()) {

        std::nth_element(sigmas.begin(),
                         sigmas.begin() + sigmas.size()/2,
                         sigmas.end());

        tau = sigmas[sigmas.size()/2];
    }

    double score = 0.0;

    for (const auto& ev : mModel->mEvents) {

        if (ev->mType != Event::eDefault) {
            score += 1.0;
            continue;
        }

        // Contribution des dates
        double mean_date_score = 0.0;
        for (const Date& date : ev->mDates) {
            mean_date_score += std::exp(-date.mSigmaTi.value() / tau);
        }
        mean_date_score /= ev->mDates.size();

        score += mean_date_score;

    }

    if (mModel->is_curve) {
        // Calcul de var_reference
        double var_reference = 0.0;
        const auto& events = mModel->mEvents;

        if (mModel->compute_X_only) {
            var_reference = variance_Knuth(get_vector<double>(get_Yx, events));
        } else if (mModel->compute_Y) {
            var_reference = (variance_Knuth(get_vector<double>(get_Yx, events)) +
                             variance_Knuth(get_vector<double>(get_Yy, events))) / 2.0;
        } else {
            var_reference = (variance_Knuth(get_vector<double>(get_Yx, events)) +
                             variance_Knuth(get_vector<double>(get_Yy, events)) +
                             variance_Knuth(get_vector<double>(get_Yz, events))) / 3.0;
        }

        // Vérification de la variance
        if (var_reference <= 1e-15) {
            var_reference = 1.0; // valeur par défaut
        }

        //
        double var_residuel = 0;


        for (const auto& ev : mModel->mEvents) {
            if (ev->mPointType == Event::ePoint) {
                var_residuel = ev->mVg.value();
                // On pourrait imaginer que var_reference soit ev->mInitialVariance
                break;
            }
        }

        // Le score de la courbe doit être entre 0 et 1
        // On divise des m² par des m², le résultat est sans unité.
        double curve_score = std::exp(-var_residuel / var_reference);

        // Maintenant on peut combiner avec le score des dates (qui est aussi sans unité)
        score *= curve_score;

    }
    return score;

}*/
double MCMCLoop::SMC_score()
{
    // --- Échelle de référence temporelle (tau) ---
    double tau = 1.0;
    {
        std::vector<double> sigmas;
        for (const auto& ev : mModel->mEvents) {
            if (ev->mType == Event::eDefault) {
                for (const Date& date : ev->mDates) {
                    sigmas.push_back(date.mSigmaTi.value());
                }
            }
        }
        if (!sigmas.empty()) {
            std::nth_element(sigmas.begin(), sigmas.begin() + sigmas.size() / 2, sigmas.end());
            tau = sigmas[sigmas.size() / 2];
            if (tau <= 1e-15) tau = 1.0;
        }
    }

    // --- Échelle de référence pour la courbe (var_reference) ---
    double var_reference = 1.0;
    if (mModel->is_curve) {
        const auto& events = mModel->mEvents;
        if (mModel->compute_X_only) {
            var_reference = variance_pop_Knuth(get_vector<double>(get_Yx, events));
        } else if (mModel->compute_Y) {
            var_reference = (variance_pop_Knuth(get_vector<double>(get_Yx, events)) +
                             variance_pop_Knuth(get_vector<double>(get_Yy, events))) / 2.0;
        } else {
            var_reference = (variance_pop_Knuth(get_vector<double>(get_Yx, events)) +
                             variance_pop_Knuth(get_vector<double>(get_Yy, events)) +
                             variance_pop_Knuth(get_vector<double>(get_Yz, events))) / 3.0;
        }
        if (var_reference <= 1e-15) var_reference = 1.0;
    }

    // --- Distance quadratique combinée, sommée sur tous les évènements ---
    double D_total = 0.0;

    for (const auto& ev : mModel->mEvents) {

        // Terme temporel : (sigmaTi_moyen / tau)^2
        if (ev->mType == Event::eDefault) {
            double mean_sigma = 0.0;
            for (const Date& date : ev->mDates) {
                mean_sigma += date.mSigmaTi.value();
            }
            mean_sigma /= ev->mDates.size();          // >=1 par construction
            const double t = mean_sigma / tau;
            D_total += t * t;
        }
        // eBound : sigmaTi = 0 -> contribution nulle

        // Terme courbe : mVg / var_reference
        if (mModel->is_curve && ev->mPointType == Event::ePoint) {
            D_total += ev->mVg.value() / var_reference;
        }
        // eNode : mVg = 0 -> contribution nulle
    }

    // score = produit des exp(-D_i) = exp(-somme des D_i)
    return std::exp(-D_total);
}
/**
 * @brief Calcule le log-score SMC d'une initialisation.
 *
 * @param Tmin, Tmax  Bornes de l'échelle temporelle de référence (années),
 *                     utilisées pour normaliser sigmaTi. Communes à toutes
 *                     les particules comparées.
 * @param Xmin, Xmax  Bornes de l'échelle spatiale de référence (unité de Y),
 *                     utilisées pour normaliser mVg. Communes à toutes
 *                     les particules comparées.
 *
 * @return double log_SMC_score = -D_total <= 0.
 *         Une chronologie/courbe parfaite (sigmaTi=0, mVg=0 partout)
 *         donne log_SMC_score = 0, soit le maximum atteignable.
 *
 * @details D_i = (sigmaTi_i / (Tmax-Tmin))^2 + mVg_i / (Xmax-Xmin)
 *          D_total = sum_i D_i
 *          Les eBound (sigmaTi=0) et eNode (mVg=0) contribuent 0 par construction.
 */
double MCMCLoop::log_SMC_score(double Tmin, double Tmax, double Xmin, double Xmax)
{
    const double T_range = Tmax - Tmin;
    const double X_range = Xmax - Xmin;

    // Garde-fous : une échelle nulle ou invalide rendrait le terme correspondant
    // soit indéfini (division par 0), soit sans effet réel de normalisation.
    const double tau    = (T_range > 1e-15) ? T_range : 1.0;
    const double vg_ref = (X_range > 1e-15) ? X_range * X_range : 1.0;

    double D_total = 0.0;

    for (const auto& ev : mModel->mEvents) {

        // ------------------------------------------------------------
        // 1. Qualité temporelle
        // ------------------------------------------------------------
        if (ev->mType == Event::eDefault) {

            double sum_precision = 0.0;

            for (const Date& date : ev->mDates) {

                const double sigma =
                    date.mSigmaTi.value();

                if (sigma > 0.0)
                    sum_precision +=
                        1.0 / (sigma * sigma);
            }

            if (sum_precision > 0.0) {

                // Incertitude de la moyenne temporelle
                const double sigma_theta =
                    1.0 / std::sqrt(sum_precision);

                const double t =
                    sigma_theta / tau;

                D_total += t * t;
            }
        }

        // ------------------------------------------------------------
        // 2. Qualité de la courbe
        // ------------------------------------------------------------
        if (mModel->is_curve &&
            ev->mPointType == Event::ePoint) {

            const double Vg = ev->mVg.value();

            if (Vg >= 0.0)
                D_total += Vg / vg_ref;
        }
        // eNode : mVg = 0 -> contribution nulle
    }

    return -D_total;
}

void MCMCLoop::run()
{
#if DEBUG
   // qDebug()<<"[MCMCLoop::run] run()";
#endif
    mState = State::eCalibrating;
    QElapsedTimer startTime;
    startTime.start();

    const QString mDate = QDateTime::currentDateTime().toString("dddd dd MMMM yyyy");
    QString log = "Start " + mDate + " -> " + QTime::currentTime().toString("hh:mm:ss.zzz");

#pragma mark Calibrating
    // ======================================================================
    // 1. Calibrating
    // ======================================================================

    emit stepChanged(tr("Calibrating data..."), 0, 0);

    mAbortedReason = this->calibrate();
    if (!mAbortedReason.isEmpty())
        return;

#pragma mark Hybrid SMC

    // ---------------------------------------------------------------------
    // 1️⃣  Déclarations et vérifications de base
    // ---------------------------------------------------------------------
    const qsizetype requiredChains = mModel->mMCMCSettings.mNumChains;   // nombre de chaînes demandé
    const qsizetype existingSeeds = mModel->mMCMCSettings.mSeeds.size();   // graines déjà stockées

    // ---------------------------------------------------------------------
    // 2️⃣  Copie des graines déjà présentes (si elles existent)
    // ---------------------------------------------------------------------
    // On copie d’abord les graines officielles qui se trouvent dans
    // mModel->mMCMCSettings.mSeeds.  Si ce vecteur est vide, on ne copie rien
    // (les nouvelles graines seront créées plus bas).
    std::size_t copyCount = 0;                     // nombre de graines réellement copiées
    if (!mModel->mMCMCSettings.mSeeds.empty())
    {
        copyCount = std::min<std::size_t>(mModel->mMCMCSettings.mSeeds.size(),
                                          static_cast<std::size_t>(requiredChains));
        // On s’assure que mLoopChains possède au moins `copyCount` éléments.
        if (mLoopChains.size() < copyCount)
            mLoopChains.resize(copyCount);
        for (std::size_t i = 0; i < copyCount; ++i)
            mLoopChains[i].mSeed = mModel->mMCMCSettings.mSeeds[i];
    }
    // ---------------------------------------------------------------------
    // 3️⃣  Détermination du mode hybrid SMC
    // ---------------------------------------------------------------------
    bool hybrid_SMC = (existingSeeds < requiredChains) && (requiredChains > 5);


    // ---------------------------------------------------------------------
    // 4️⃣  Initialisation des graines (et du reste) selon le mode
    // ---------------------------------------------------------------------

    // Lors du calcul de courbe l'ordre est modifié, puisqu'on trie les Event dans l'ordre croissant
    // Il faut donc le mémoriser pour le rétablir à chaque graine
    std::vector<std::shared_ptr<Event>> initialEventOrder = mModel->mEvents;
    const int N_missing = static_cast<int>(requiredChains - copyCount);

    if (hybrid_SMC) {


        mState = State::eSMC;
        // -----------------------------------------------------------------
        // 4. Nombre de chaînes et particules requises
        // -----------------------------------------------------------------
        const int N_missing = static_cast<int>(requiredChains - copyCount);
        if (N_missing <= 0) return; // Aucune chaîne à créer

        const int N_particles = N_missing * 20; // 20 particules par chaîne manquante
        emit stepChanged(tr("Hybrid SMC initializing …"), 0, N_particles);

        // -----------------------------------------------------------------
        // 4.1 Allocation temporaire pour l'évaluation des particules
        // -----------------------------------------------------------------
        mModel->mChains.resize(N_particles);
        mModel->setParametersForChain();

        struct Particle {
            int init_seed;
            double weight; // log-score puis poids normalisé
        };

        std::vector<Particle> particles;
        particles.reserve(N_particles);

        const double tmin = mModel->mSettings.mTmin;
        const double tmax = mModel->mSettings.mTmax;
        const double sigma_g = std::sqrt(mModel->mS02Vg);

        // -----------------------------------------------------------------
        // 4.2 Génération et calcul des log-scores
        // -----------------------------------------------------------------
        int master_seed = Generator::createSeed();//123456; // Seed principal (paramètre utilisateur), pour assurer la reproductibilité au besoin en DEBUG
        Generator::initGenerator(master_seed);
        std::vector<int> test_seed;

        for (int i = 0; i < N_particles; ++i) {
             test_seed.push_back(Generator::randomUniformInt(1, 1000));
            //test_seed.push_back(Generator::createSeed());
        }

        for (int i = 0; i < N_particles; ++i) {
            emit stepProgressed(i + 1);
            // const int seed = Generator::createSeed();
            const int seed = test_seed[i];
            Generator::initGenerator(seed);

            // Rétablissement de l'ordre initiale
            mModel->mEvents = initialEventOrder;

            initialize();

            const double logScore = log_SMC_score(tmin, tmax, 0, sigma_g);
            particles.push_back({seed, logScore});
#ifdef DEBUG
            std::cout << " seed : " << seed << "; log(SMC score) : " << logScore << std::endl;
#endif
        }

        // -----------------------------------------------------------------
        // 4.3 Ajustement dynamique de T par ESS (Effective Sample Size)
        // -----------------------------------------------------------------

        // Fonction lambda pour calculer l'ESS pour une température T donnée
        auto compute_ESS = [&](double T, std::vector<double>& out_normalized_weights) -> double {
            const double max_log_w = std::max_element(
                                         particles.begin(), particles.end(),
                                         [](const Particle& a, const Particle& b) { return a.weight < b.weight; }
                                         )->weight;

            double sum_exp = 0.0;
            for (const auto& p : particles) {
                sum_exp += std::exp((p.weight - max_log_w) / T);
            }
            const double log_sum_wi = (max_log_w / T) + std::log(sum_exp);

            double sum_sq_w = 0.0;
            out_normalized_weights.resize(particles.size());
            for (size_t i = 0; i < particles.size(); ++i) {
                const double w = std::exp((particles[i].weight / T) - log_sum_wi);
                out_normalized_weights[i] = w;
                sum_sq_w += w * w;
            }

            return 1.0 / sum_sq_w; // ESS = 1 / sum(w_i^2)
        };

        // Target ESS : 50% des particules (ex: 10 particules effectives si N_particles = 20)
        const double target_ESS = 0.50 * N_particles;

        std::vector<double> normalized_weights;
        double current_ESS = compute_ESS(1.0, normalized_weights);

        double T_opt = 1.0;

        // Si l'ESS à T = 1.0 est sous le seuil, on ajuste la température par dichotomie
        if (current_ESS < target_ESS) {
            double T_min = 1.0;
            double T_max = 10000.0; // Température max arbitraire
            const int max_iterations = 30;

            for (int iter = 0; iter < max_iterations; ++iter) {
                T_opt = 0.5 * (T_min + T_max);
                current_ESS = compute_ESS(T_opt, normalized_weights);

                if (std::abs(current_ESS - target_ESS) < 1e-2) {
                    break;
                }

                if (current_ESS < target_ESS) {
                    T_min = T_opt; // ESS trop faible -> augmenter T
                } else {
                    T_max = T_opt; // ESS trop élevé -> diminuer T
                }
            }
            std::cout << "[SMC] 🔥 Degeneracy detected! Adjusted Temperature T = "
                      << T_opt << " (ESS = " << current_ESS << " / " << target_ESS << ")\n";
        } else {
            std::cout << "[SMC] ✅ Good particle diversity (ESS = "
                      << current_ESS << " / " << target_ESS << ")\n";
        }

        // Réaffectation des poids ajustés dans la structure particles
        for (size_t i = 0; i < particles.size(); ++i) {
            particles[i].weight = normalized_weights[i];
        }
        // -----------------------------------------------------------------
        // 4.4 Sélection exacte des N_missing graines
        // -----------------------------------------------------------------
        const bool use_best_scores = false; // true = top-N glouton, false = tirage CDF
        const int N_new = N_missing;        // On ne tire QUE les graines manquantes

        std::vector<Particle> new_particles;
        new_particles.reserve(N_new);

        if (use_best_scores) {
            // --- Sélection élitiste (Top-N) ---
            if (N_new < static_cast<int>(particles.size())) {
                std::nth_element(particles.begin(), particles.begin() + N_new, particles.end(),
                                 [](const Particle& a, const Particle& b) { return a.weight > b.weight; });
            }
            new_particles.assign(particles.begin(), particles.begin() + N_new);

        } else {
            // --- Tirage systématique pondéré (CDF) ---
            std::vector<double> cdf(particles.size());
            cdf[0] = particles[0].weight;
            for (size_t i = 1; i < particles.size(); ++i) {
                cdf[i] = cdf[i - 1] + particles[i].weight;
            }
            cdf.back() = 1.0; // Sécurité numérique

            const double u0 = Generator::randomUniform() / N_new;
            for (int n = 0; n < N_new; ++n) {
                const double u = u0 + static_cast<double>(n) / N_new;
                auto it = std::lower_bound(cdf.begin(), cdf.end(), u);
                size_t idx = std::distance(cdf.begin(), it);

                if (idx >= particles.size()) {
                    idx = particles.size() - 1;
                }
                new_particles.push_back(particles[idx]);
            }
        }
        /*std::vector<int> test {35, 749, 752};
        for (auto  i=0 ; i< test.size(); i++) {
            new_particles[i].init_seed = test[i];
        }*/

        // -----------------------------------------------------------------
        // 4.5 Affectation des nouvelles graines dans mLoopChains
        // -----------------------------------------------------------------
        if (mLoopChains.size() < static_cast<std::size_t>(requiredChains)) {
            mLoopChains.resize(requiredChains);
        }

        // On remplit uniquement les emplacements manquants (à partir de copyCount)
        for (int n = 0; n < N_new; ++n) {
            mLoopChains[copyCount + n].mSeed = new_particles[n].init_seed;
        }
    } else {
        // On remplit uniquement les emplacements manquants (à partir de copyCount)
        for (int n = 0; n < N_missing; ++n) {
            mLoopChains[copyCount + n].mSeed = Generator::createSeed();
        }
    }

    // -----------------------------------------------------------------
    // 5️⃣ Redimensionnement final du modèle
    // -----------------------------------------------------------------
    mModel->mChains.resize(requiredChains);

    mModel->setParametersForChain();

    mModel->mLogInit += ModelUtilities::getMCMCSettingsLog(mModel);

    QStringList seeds;
    for (auto& chain : mLoopChains)
         seeds << QString::number(chain.mSeed);

    mModel->mLogInit += "<br>" + line(tr("List of used chain seeds (to be copied for re-use in MCMC Settings) : ") + seeds.join(";"));


    // copie la liste des pointeurs, pour garder l'ordre initiale des Events;
    // le mécanisme d'initialisation pour les courbes modifie cette liste, hors il faut la réablir pour les chaines suivantes
    std::vector<std::shared_ptr<Event>> initListEvents (mModel->mEvents.size());
    std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    QElapsedTimer globalTimer;
    globalTimer.start();

    qint64 lastUpdateTime = 0;

    unsigned estimatedTotalIter = (unsigned)((int)mLoopChains.size() *(1 + mLoopChains.at(0).mIterPerBurn + mLoopChains.at(0).mIterPerBatch*mLoopChains.at(0).mMaxBatchs + mLoopChains.at(0).mIterPerAquisition));
    unsigned iterDone = 0;


    const int annealSubSteps = computeAnnealSubSteps(mModel->mMCMCSettings.mAnnealTemp,
                                                     mModel->mMCMCSettings.mAnnealDwell);
    AnnealAwareEstimator est;      // 👈 partagé Burn-in / Adapting / Acquisition
    est.setSubSteps(annealSubSteps);
    const qint64 R = mModel->mMCMCSettings.mAnnealRecurrence;
    QElapsedTimer stepTimer;

    for (mChainIndex = 0; mChainIndex < mLoopChains.size(); ++mChainIndex) {

        log += "<hr>";

        ChainSpecs& chain = mLoopChains[mChainIndex];

        const bool annealingEnabled = (R > 0) && (R < chain.mIterPerAquisition);


        // Utiliser le seed d'initialisation (sélectionné par SMC)
        Generator::initGenerator(chain.mSeed); // 👈 Seed original

#pragma mark Initialisation
        // ======================================================================
        // 3. Initialization
        // ======================================================================

        if (isInterruptionRequested()) {
            mAbortedReason = ABORTED_BY_USER;
            return;
        }
        mState = State::eInit;

        ++iterDone;


        QElapsedTimer initTime;
        initTime.start();
        qDebug() << " seed used for init ⚠️ = " << Generator::seed();

        // Rétablissement de l'ordre d'origine
        mModel->mEvents = initialEventOrder;
        mAbortedReason = initialize();

        emit stepChanged(tr("Chain : %1 / %2").arg(QString::number(mChainIndex + 1), QString::number(mLoopChains.size()))  + " : " + tr("Initializing"), 0, estimatedTotalIter);

        if (!mAbortedReason.isEmpty())
            return;

        recordBurnAdapt();
        recordMH();

        chain.mInitElapsedTime = initTime.elapsed();
        initTime.~QElapsedTimer();

        mModel->mLogInit += "<hr>";
        mModel->mLogInit += line(textBold(tr("INIT CHAIN %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))));
        mModel->mLogInit += line("Init Seed : " + QString::number(chain.mSeed)) + " SMC score :" + QString::number(SMC_score());
        qDebug() << " mLogInit Seed :  " <<  QString::number(chain.mSeed);

        mModel->mLogInit += ModelUtilities:: modelStateDescriptionHTML(mModel);

#pragma mark Burn-in
        // ======================================================================
        // 4. Burn-in
        // ======================================================================


        emit stepChanged(tr("Chain : %1 / %2").arg(QString::number(mChainIndex + 1), QString::number(mLoopChains.size()))  + " : " + tr("Initializing"), 0, estimatedTotalIter);

        mState = State::eBurning;

        QElapsedTimer burningTime;
        burningTime.start();

        while (chain.mBurnIterIndex < chain.mIterPerBurn) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }

            try {

                stepTimer.start();
                update();


#ifdef _WIN32
//    SetThreadExecutionState( ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);

#endif
            } catch (QString error) {
                mAbortedReason = error;
                return;
            }

            recordBurnAdapt();
            recordMH();
            est.addSample(stepTimer.nsecsElapsed(), /*wasRegen=*/false, annealingEnabled);   // 👈



            ++chain.mBurnIterIndex;
            ++chain.mTotalIter;

            ++iterDone;

            qint64 now = globalTimer.elapsed();
            if (now - lastUpdateTime > 1000 && iterDone > 10) {
                qint64 interTime = estimateGlobalRemainingNs(mLoopChains, mChainIndex, mState, est, R) / 1000000;
                emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))
                                + " : Burn-in\t ; Total Estimated time left " + DHMS(interTime));
                lastUpdateTime = now;
            }
            emit stepProgressed(iterDone);
        }
        chain.burnElapsedTime = burningTime.elapsed();
        burningTime.~QElapsedTimer();

#pragma mark Adaptation
        // ======================================================================
        // 5. Adaptation
        // ======================================================================


        mState = State::eAdapting;

        QElapsedTimer adaptTime;
        adaptTime.start();


        while ( chain.mBatchIndex < chain.mMaxBatchs) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }

            chain.mBatchIterIndex = 0;
            while (chain.mBatchIterIndex < chain.mIterPerBatch) {
                if (isInterruptionRequested()) {
                    mAbortedReason = ABORTED_BY_USER;
                    return;
                }

                try {
                    stepTimer.start();
                    update();

#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
#endif

                } catch (QString error) {
                    mAbortedReason = error;
                    return;
                }

                // memo();
                recordBurnAdapt();
                recordMH();
                est.addSample(stepTimer.nsecsElapsed(), /*wasRegen=*/false, annealingEnabled);

                ++chain.mBatchIterIndex;
                ++chain.mTotalIter;
                ++iterDone;

                qint64 now = globalTimer.elapsed();
                if (now - lastUpdateTime > 1000 && iterDone > 10) {
                    qint64 interTime = estimateGlobalRemainingNs(mLoopChains, mChainIndex, mState, est, R) / 1000000;
                    emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))
                                    + " : Adaptation\t ; Total Estimated time left " + DHMS(interTime));
                    lastUpdateTime = now;
                }

                emit stepProgressed(iterDone);
                qApp->processEvents(); //This function is especially useful if you have a long running operation and want to show its progress
            }
            ++chain.mBatchIndex;

            if (adapt(chain.mBatchIndex))
                    break;


        }
        // Fix Total iteration if adaptation break before the end
        estimatedTotalIter -= (chain.mMaxBatchs-chain.mBatchIndex)*chain.mIterPerBatch;

        mModel->mLogAdapt += "<hr>";
        mModel->mLogAdapt += line(textBold(tr("ADAPTATION FOR CHAIN %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))) );

        if (chain.mBatchIndex < chain.mMaxBatchs) {
            mModel->mLogAdapt += line("Adapt OK at batch : " + QString::number(chain.mBatchIndex) + "/" + QString::number(chain.mMaxBatchs));

        } else {
            mModel->mLogAdapt += line(textRed("Warning : Not adapted after " + QString::number(chain.mBatchIndex) + " batches"));
        }

        mModel->mLogAdapt += ModelUtilities::modelStateDescriptionHTML(mModel) ;
        mModel->mLogAdapt += "<hr>";

        chain.mAdaptElapsedTime = adaptTime.elapsed();
        adaptTime.~QElapsedTimer();

#pragma mark Acquisition
        // ======================================================================
        // 6. Acquisition
        // ======================================================================

        mState = State::eAcquisition;
        QElapsedTimer acquisitionTime;
        acquisitionTime.start();

        int thinningIdx = 0;
        int batchIdx = 1;
        int totalBacth = chain.mBatchIndex; // on continue le comptage du nombre de batch pour l'adaptation
        bool OkToMemo;

        int thinning_OkToMemo = 0; // compte le nombre entre chaque update = memo==true

        chain.mRealyAccepted = 0;

        while (chain.mAquisitionIterIndex < chain.mIterPerAquisition) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }
            ++iterDone;

            const qint64 curIter = chain.mTotalIter;
            const bool willRegen = annealingEnabled
                                   && (curIter > 0)
                                   && (curIter % R == 0)
                                   && (mState == State::eAcquisition);
            try {
                //  Ici le temps est aussi fonction du tempering qui est dans update(), quand il se déclenche le temps pour update augmente
                // mModel->mMCMCSettings.mAnnealRecurrence * mModel->mMCMCSettings.mAnnealTemp + mModel->mMCMCSettings.mAnnealRecurrence * mModel->mMCMCSettings.mAnnealDwell


                stepTimer.start();
                OkToMemo = update();

                thinningIdx++;
                if (OkToMemo) {
                    ++thinning_OkToMemo;
                }


#ifdef _WIN32
    SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
#endif
            } catch (QString error) {
                mAbortedReason = error;
                return;
            }

            if (thinningIdx == chain.mThinningInterval || thinning_OkToMemo >= chain.mThinningInterval ) {

                thinningIdx = 0;

                if (OkToMemo) {

                    recordMH();
                    acquire();
                    thinning_OkToMemo = 0;

                    ++chain.mRealyAccepted;
                }

            }

            est.addSample(stepTimer.nsecsElapsed(), willRegen, annealingEnabled);

            if (batchIdx == chain.mIterPerBatch) {
                adapt(totalBacth);
                //adapt(batchIdx);
                batchIdx = 1;
                totalBacth++;

            } else {
                batchIdx++;
            }


            ++chain.mAquisitionIterIndex;
            ++chain.mTotalIter;

            qint64 now = globalTimer.elapsed();
            if (now - lastUpdateTime > 1000 && iterDone > 10) {
                //qint64 interTime = estimateGlobalRemainingNs(chain, mState, est, R, annealingEnabled) / 1000000;
                qint64 interTime = estimateGlobalRemainingNs(mLoopChains, mChainIndex, mState, est, R) / 1000000;
                emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))
                                + " : Acquisition\t ; Total Estimated time left " + DHMS(interTime));
                lastUpdateTime = now;
            }

            if (!(chain.mAquisitionIterIndex % chain.mIterPerBatch)) {
              qApp->processEvents();
            }

            emit stepProgressed(iterDone);
        }

        chain.mIterDisplay = chain.mRealyAccepted;

        chain.mAcquisitionElapsedTime = acquisitionTime.elapsed();
        acquisitionTime.~QElapsedTimer();
        mModel->mLogResults += line(tr("Acquisition time elapsed %1").arg(DHMS(chain.mAcquisitionElapsedTime)));

        // rétablissement de l'ordre des Events, indispensable en cas de calcul de courbe. Car le update modifie l'ordre des events et utile pour la sauvegarde de ChronoModel_Bash
        std::copy(initListEvents.begin(), initListEvents.end(), mModel->mEvents.begin() );
    }

    mModel->mChains = mLoopChains;

#pragma mark Finalize
    // ======================================================================
    // 7. Finalize
    // ======================================================================

    emit stepChanged(tr("Computing posterior distributions and numerical results (HPD, credibility, ...)"), 0, 0);
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
    try {
        mState = State::eFinalize;
        finalize();

    } catch (QString error) {
        mAbortedReason = error;
        return;
    } catch(...) {
        std::cerr << "[" << __func__ << "] Caught Exception ‼️" << std::endl;
        mAbortedReason = " Run Error";
        return;
    }


#ifdef DEBUG
    QTime endTime = QTime::currentTime();

    qDebug() << "[" << __func__ << "]  Model computed";
    qDebug() << "[" << __func__ << "] " << tr("finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug() << "[" << __func__ << "] " << tr("Total time elapsed %1").arg(QString(DHMS(startTime.elapsed())));
#endif


}


qint64 estimateGlobalRemainingNs(const std::vector<ChainSpecs> &chains,   // ou le type réel de mLoopChains
                                 int currentChainIndex,
                                 MCMCLoop::State state,
                                 const AnnealAwareEstimator& est,
                                 qint64 R)
{
    qint64 total = 0;

    // ---- 1. Reste de la chaîne courante ----
    {
        const ChainSpecs& chain = chains[currentChainIndex];
        const bool annealingEnabled = (R > 0) && (R <= chain.mIterPerAquisition);
        qint64 t = chain.mTotalIter;

        if (state == MCMCLoop::State::eInit || state == MCMCLoop::State::eBurning) {
            const qint64 remain = (state == MCMCLoop::State::eBurning)
            ? (chain.mIterPerBurn - chain.mBurnIterIndex)
            : chain.mIterPerBurn;
            total += est.estimateRemainingNs(t, remain, R, /*regenApplies=*/false);
            t += remain;
        }

        if (state == MCMCLoop::State::eInit || state == MCMCLoop::State::eBurning || state == MCMCLoop::State::eAdapting) {
            const qint64 totalAdaptIter = (qint64)chain.mMaxBatchs * chain.mIterPerBatch;
            qint64 remain;
            if (state == MCMCLoop::State::eAdapting) {
                const qint64 doneInAdapt = (qint64)chain.mBatchIndex * chain.mIterPerBatch + chain.mBatchIterIndex;
                remain = std::max<qint64>(0, totalAdaptIter - doneInAdapt);
            } else {
                remain = totalAdaptIter;
            }
            total += est.estimateRemainingNs(t, remain, R, /*regenApplies=*/false);
            t += remain;
        }

        {
            const qint64 remain = (state == MCMCLoop::State::eAcquisition)
            ? (chain.mIterPerAquisition - chain.mAquisitionIterIndex)
            : chain.mIterPerAquisition;
            total += est.estimateRemainingNs(t, remain, R, annealingEnabled);
            t += remain;
        }
    }

    // ---- 2. Chaînes futures, pas encore démarrées : temps complet ----
    for (size_t c = currentChainIndex + 1; c < chains.size(); ++c) {
        const ChainSpecs& chain = chains[c];
        const bool annealingEnabled = (R > 0) && (R <= chain.mIterPerAquisition);
        qint64 t = 0;   // 👈 chaîne pas encore démarrée, mTotalIter repart de 0

        total += est.estimateRemainingNs(t, chain.mIterPerBurn, R, /*regenApplies=*/false);
        t += chain.mIterPerBurn;

        const qint64 totalAdaptIter = (qint64)chain.mMaxBatchs * chain.mIterPerBatch;
        total += est.estimateRemainingNs(t, totalAdaptIter, R, /*regenApplies=*/false);
        t += totalAdaptIter;

        total += est.estimateRemainingNs(t, chain.mIterPerAquisition, R, annealingEnabled);
        t += chain.mIterPerAquisition;
    }

    return total;
}