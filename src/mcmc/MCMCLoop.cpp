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

#include <QDebug>
#include <QTime>

#include <QtWidgets>

#ifdef _WIN32
//#include "winbase.h"
#include <windows.h> //for Qt 6.7
#endif


MCMCLoop::MCMCLoop(std::shared_ptr<ModelCurve> model):
    mChainIndex (0),
    mState (eBurning)
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

        if (i < s.mSeeds.size())
            chain.mSeed = s.mSeeds.at(i);
        else
            chain.mSeed = Generator::createSeed();

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

#ifdef S02_BAYESIAN
        ev->mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;// not yet integrate within update_321

# else
        ev->mS02Theta.mSamplerProposal = MHVariable::eFixe;
#endif
    }
    // -------------------------- Init gamma ------------------------------
   // emit stepChanged(tr("Initializing Phase Gaps..."), 0, (int)phasesConstraints.size());
    //int i = 0;
    try {
        for (auto&& phC : phasesConstraints) {
            phC->initGamma();
            if (isInterruptionRequested())
                return ABORTED_BY_USER;
            //++i;
            //emit stepProgressed(i);
        }
    }  catch (...) {
        qWarning() <<"Init Gamma ???";
        mAbortedReason = QString("Error in Init Gamma ???");
        return mAbortedReason;
    }

    // -------------------------- Init tau -----------------------------------------
    //emit stepChanged(tr("Initializing Phase Durations..."), 0, (int)phases.size());
    //i = 0;
    try {
        for (auto&& ph : phases) {
            ph->initTau(tminPeriod, tmaxPeriod);
            qDebug() << "[MCMCLoop::initialize_time] " << ph->getQStringName() << " init Tau =" << ph->mTau.mX;
            if (isInterruptionRequested())
                return ABORTED_BY_USER;
            //++i;
            //emit stepProgressed(i);
        }
    }  catch (...) {
        qWarning() <<"Init Tau ???";
        mAbortedReason = QString("Error in Init Tau ???");
        return mAbortedReason;
    }
    // --------------------------  Init Bounds --------------
    try {
        for (std::shared_ptr<Event> &ev : allEvents) {
            if (ev->mType == Event::eBound) {
                Bound* bound = dynamic_cast<Bound*>(ev.get());

                if (bound) {
                    bound->mTheta.mX = bound->mFixed;
                    bound->mThetaReduced = mModel->reduceTime(bound->mTheta.mX);
                    bound->mTheta.mLastAccepts.clear();
                    bound->mTheta.memo(); // non sauvegarder dans Loop.memo()
                    bound->mInitialized = true;
                    bound->mTheta.mSamplerProposal = MHVariable::eFixe;
                    qDebug() << QString("[MCMCLoop::initialize_time] Init for Bound : %1  ->theta = %4 thetaRed = %5-------").arg(bound->getQStringName(), QString::number(bound->mTheta.mX, 'f', 3), QString::number(bound->mThetaReduced, 'f', 3));

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

    //emit stepChanged(tr("Initializing Events..."), 0, (int)unsortedEvents.size());
    qDebug()<<"[MCMCLoop::initialize_time] mLoopChains seed = "<< mLoopChains[0].mSeed;
    try {

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
            qDebug() << "[MCMCLoop::initialize_time] " << p->getQStringName() << " init alpha =" << p->mAlpha.mX << " beta=" << p->mBeta.mX;
        }

        //---------------
        // int i = 0;
        if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {

            for (std::shared_ptr<Event> uEvent : unsortedEvents) {
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
                        try_theta = sample_in_repartition(uEvent->mMixingCalibrations, min, max);

                    }
                    if (try_theta > max || try_theta < min) {
                        const int seed = mLoopChains.at(mChainIndex).mSeed;
#ifdef DEBUG
                        qDebug()<<QString("[MCMCLoop::initialize_time] Error Init for eventuEvent->mTheta.mX > max || uEvent->mTheta.mX < min : %1 : min = %2 : max = %3-------Seed = %4").arg(uEvent->getQStringName(), QString::number(min, 'f', 30), QString::number(max, 'f', 30), QString::number(seed));
                        mAbortedReason = QString(tr("uEvent->mTheta.mX > max || uEvent->mTheta.mX < min Error Init for event : %1 \n min = %2 \n max = %3 \n Seed = %4").arg(uEvent->getQStringName(), QString::number(min, 'f', 6), QString::number(max, 'f', 6), QString::number(seed)));
#else
                        mAbortedReason = QString(tr("Error Init for event : %1 \n min = %2 \n max = %3 \n Seed = %4").arg(uEvent->getQStringName(), QString::number(min, 'f', 6), QString::number(max, 'f', 6), QString::number(seed)));
#endif
                        return mAbortedReason;
                    }
                    // 6- Clear mLastAccepts  array
                    uEvent->mTheta.mLastAccepts.clear();
                    //unsortedEvents.at(i)->mTheta.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                    uEvent->mTheta.accept_update(try_theta);


                    uEvent->mThetaReduced = mModel->reduceTime(uEvent->mTheta.mX);
                    uEvent->mInitialized = true;

                    // ------- debug init
                    qDebug() << QString("[MCMCLoop::initialize_time] Init for event : %1 : min = %2 : max = %3  ->theta = %4 thetaRed = %5-------").arg(uEvent->getQStringName(), QString::number(min, 'f', 3), QString::number(max, 'f', 3), QString::number(uEvent->mTheta.mX, 'f', 3), QString::number(uEvent->mThetaReduced, 'f', 3));
                    // ----------------------------------------------------------------


                    double s02_sum = 0.;

                    for (Date& date : uEvent->mDates) {

                        // 1 - Init ti
                        bool is_wiggle = date.mWiggleCalibration != nullptr;


                        const FunctionStat &data = analyseFunction(date.mCalibration->mMap);
                        double sigma = data.std;
#ifdef DEBUG
                        if (sigma == 0.)
                            return "[MCMCLoop::initialize_time] sigma == 0";
#endif
                        if (is_wiggle) {
                            // On favorise les solutions pretes de theta de l'Event
                            std::vector<double> repart_exp_theta (date.mWiggleCalibration->mVector.size());
                            double sum_exp = 0.0;
                            for ( size_t i = 0; i < date.mWiggleCalibration->mVector.size(); i++) {
                                double exp_theta = dnorm(date.mWiggleCalibration->mTmin + i * date.mWiggleCalibration->mStep, uEvent->mTheta.mX, sigma);
                                sum_exp += exp_theta * date.mWiggleCalibration->mVector[i];
                                repart_exp_theta[i] = sum_exp;
                            }
                            const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(0, sum_exp), repart_exp_theta);
                            date.mTi.mX = date.mWiggleCalibration->mTmin + idx * date.mWiggleCalibration->mStep;


                        } else if (!date.mCalibration->mRepartition.empty()) {
                            // On favorise les solutions pretes de theta de l'Event
                            std::vector<double> repart_exp_theta (date.mCalibration->mVector.size());
                            double sum_exp = 0.0;
                            for ( size_t i = 0; i < date.mCalibration->mVector.size(); i++) {
                                double exp_theta = dnorm(date.mCalibration->mTmin + i * date.mCalibration->mStep, uEvent->mTheta.mX, sigma);
                                sum_exp += exp_theta * date.mCalibration->mVector[i];
                                repart_exp_theta[i] = sum_exp;
                            }
                            const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(0, sum_exp), repart_exp_theta);

                            date.mTi.mX = date.mCalibration->mTmin + idx * date.mCalibration->mStep;

                        } else { // in the case of mRepartion curve is null, we must init ti outside the study period
                            // For instance we use a gaussian random sampling
                            sigma = tmaxPeriod - tminPeriod;
                            qDebug() << "[MCMCLoop::initialize_time] mRepartion curve is null for" << date.getQStringName();
                            const double u = Generator::normalDistribution(0., sigma);
                            if (u<0)
                                date.mTi.mX = tminPeriod + u;
                            else
                                date.mTi.mX = tmaxPeriod + u;

                            if (date.mTi.mSamplerProposal == MHVariable::eInversion) {
                                qDebug()<<"[MCMCLoop::initialize_time] Automatic sampling method exchange eInversion to eMHPrior for"<< date.getQStringName();
                                date.mTi.mSamplerProposal = MHVariable::eMHPrior;
                                date.autoSetTiSampler(true);
                            }

                        }

                        // 2 - Init Delta Wiggle matching and Clear mLastAccepts array
                        date.initDelta();
                        date.mWiggle.mLastAccepts.clear();
                        date.updateWiggle();
                        //date.mWiggle.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                        date.mWiggle.accept_update(date.mWiggle.mX);

                        // 3 - Init sigma MH adaptatif of each Data ti
                        date.mTi.mSigmaMH = sigma;

                        // 4 - Clear mLastAccepts array and set this init at 100%
                        date.mTi.mLastAccepts.clear();
                        //date.mTheta.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                        date.mTi.accept_update(date.mTi.mX);

                        // 5 - Init Sigma_i and its Sigma_MH
                        date.mSigmaTi.mX = std::abs(date.mTi.mX - (uEvent->mTheta.mX - date.mDelta));


                        if (date.mSigmaTi.mX <= 1.0E-6) {
                            date.mSigmaTi.mX = 1.0E-6; // Add control the 2015/06/15 with PhL
                            //log += line(date.mName + textBold("Sigma indiv. <=1E-6 set to 1E-6"));
                        }
                        date.mSigmaTi.mSigmaMH = 1.0;

                        date.mSigmaTi.mLastAccepts.clear();
                        date.mSigmaTi.accept_update(date.mSigmaTi.mX);

                        // intermediary calculus for the harmonic average
                        s02_sum += 1.0 / (sigma * sigma);

                    }

                    // 4 - Init S02 of each Event
                    uEvent->mS02Theta.mSigmaMH = 1.0;

                    uEvent->mS02Theta.mLastAccepts.clear();

                    const double sqrt_S02_harmonique = sqrt(uEvent->mDates.size() / s02_sum);


                    uEvent->mBetaS02 = 1.004680139*(1 - exp(- 0.0000847244 * pow(sqrt_S02_harmonique, 2.373548593)));
#ifdef CODE_KOMLAN
                    // new code
                    //uEvent->mS02Theta.mX = 1.0 / Generator::gammaDistribution(1., uEvent->mBetaS02);
                    uEvent->mS02Theta.accept_update(1.0 / Generator::gammaDistribution(1., uEvent->mBetaS02));

#else
                    uEvent->mS02Theta.accept_update(uEvent->mDates.size() / s02_sum);

#endif

                    // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
                    uEvent->mTheta.mSigmaMH = sqrt(uEvent->mS02Theta.mX);
                    uEvent->mAShrinkage = 1.;


                }

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                // emit stepProgressed(i++);

            }

        } else { // theta fixe
            for (std::shared_ptr<Event> &uEvent : unsortedEvents) {
                // ----------------------------------------------------------------
                // Curve init Theta event :
                // On initialise les theta près des dates ti
                // ----------------------------------------------------------------
                if (uEvent->mType == Event::eDefault)
                    sampleInCumulatedRepartition_thetaFixe(uEvent, mModel->mSettings);
                else
                    uEvent->mTheta.mX = static_cast<Bound*>(uEvent.get())->mFixed;
                // nous devons sauvegarder la valeur ici car dans loop.memo(), les variables fixes ne sont pas memorisées.
                // Pourtant, il faut récupèrer la valeur pour les affichages et les stats
                uEvent->mTheta.memo(); // il faut faire memo ici, c'est la seule fois. ce ne sera pas fait dans MCMCLoopChrono.memo()

                uEvent->mThetaReduced = mModel->reduceTime(uEvent->mTheta.mX);
                uEvent->mInitialized = true;
                uEvent->mTheta.mSamplerProposal = MHVariable::eFixe;

                for (auto&& date : uEvent->mDates) {
                    date.mTi.mSamplerProposal = MHVariable::eFixe;
                    date.mTi.mX = uEvent->mTheta.mX;
                    date.mTi.memo();

                    // 2 - Init Delta Wiggle matching and Clear mLastAccepts array
                    date.initDelta();
                    date.mWiggle.mSamplerProposal = MHVariable::eFixe;
                    date.mWiggle.mX = date.mTi.mX + date.mDelta;
                    date.mWiggle.memo();
                    date.mWiggle.mLastAccepts.clear();
                    //date.mWiggle.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                    // 3 - Init sigma MH adaptatif of each Data ti
                    date.mTi.mSigmaMH = 1;

                    // 4 - Clear mLastAccepts array and set this init at 100%
                    date.mTi.mLastAccepts.clear();
                    //date.mTheta.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                    // 5 - Init Sigma_i and its Sigma_MH
                    date.mSigmaTi.mSamplerProposal = MHVariable::eFixe;
                    date.mSigmaTi.mX = 0;
                    date.mSigmaTi.memo();

                    date.mSigmaTi.mSigmaMH = 1.;

                    date.mSigmaTi.mLastAccepts.clear();
                }

                // 4 - Init S02 of each Event fixed
                uEvent->mS02Theta.mX = 0;
                uEvent->mS02Theta.mLastAccepts.clear();
                uEvent->mS02Theta.mSamplerProposal = MHVariable::eFixe;

                uEvent->mS02Theta.memo();

                // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
                uEvent->mTheta.mSigmaMH = 1;
                uEvent->mAShrinkage = 1.;

                // 6- Clear mLastAccepts  array
                uEvent->mTheta.mLastAccepts.clear();
                //uEvent->mTheta.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                //emit stepProgressed(i++);

            }


        }

    }  catch (const QString e) {
        qWarning() <<"Init theta event, ti,  ???"<<e;
        mAbortedReason = e;
        return mAbortedReason;
    }


    // --------------------------- Init alpha and beta phases ----------------------
    // emit stepChanged(tr("Initializing Phases..."), 0, (int)phases.size());
    try {
        //i = 0;
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

            // emit stepProgressed(++i);
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

                qDebug() << QString("[MCMCLoop::initialize_time] Init for event theta fixed : %1 : min = %2 : max = %3  ->theta = %4 thetaRed = %5-------").arg(ev->getQStringName(), QString::number(min, 'f', 3), QString::number(max, 'f', 3), QString::number(ev->mTheta.mX, 'f', 5), QString::number(ev->mThetaReduced, 'f', 5));

                if (ev->mTheta.mX < min || ev->mTheta.mX > max) {
                    throw QObject::tr("Error for event theta fixed : %1 : min = %2 : max = %3 but Theta = %4" ).arg(ev->getQStringName(), QString::number(min), QString::number(max), QString::number(ev->mTheta.mX, 'f', 3));
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
double MCMCLoop::SMC_score()
{
    // Calcul de tau pour sigma ti
    double tau = 1.0;
    std::vector<double> sigmas;

    for (const auto& ev : mModel->mEvents) {
        if (ev->mType == Event::eDefault) {
            for (const Date& date : ev->mDates) {
                sigmas.push_back(date.mSigmaTi.mX);
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
            mean_date_score += std::exp(-date.mSigmaTi.mX / tau);
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
                var_residuel = ev->mVg.mX;
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

}


void MCMCLoop::run()
{
#if DEBUG
   // qDebug()<<"[MCMCLoop::run] run()";
#endif

    QElapsedTimer startTime;
    startTime.start();

    const QString mDate = QDateTime::currentDateTime().toString("dddd dd MMMM yyyy");
    QString log = "Start " + mDate + " -> " + QTime::currentTime().toString("hh:mm:ss.zzz");


    //----------------------- Calibrating --------------------------------------

    emit stepChanged(tr("Calibrating data..."), 0, 0);

    mAbortedReason = this->calibrate();
    if (!mAbortedReason.isEmpty())
        return;

    //----------------------- hybrid SMC Initialisation si plus de 5 chaines --------------------------------------
    bool hybrid_SMC = mLoopChains.size() > 5;
    // init chaines seed
    if (hybrid_SMC) {
        int N_particles = mLoopChains.size() * 10;

        emit stepChanged(tr("Hybrid SMC initializing ..."), 0, N_particles);

        mModel->mChains.resize(N_particles);
        mModel->initVariablesForChain();

        struct Particle {
            int init_seed;   // θ1…θN
            double weight;
        };

        // Génération des particules
        std::vector<Particle> particles;
        particles.reserve(N_particles);

        for (int i = 0; i < N_particles; ++i) {

            emit stepProgressed(i+1);

            int seed = Generator::createSeed();

            Generator::initGenerator(seed);
            initialize();

            Particle p{seed, SMC_score()};
            particles.push_back(p);
            std::cout << " seed : "<< seed << "; SMC score : " << p.weight << std::endl;


        }

        double sum_wi = std::accumulate(particles.begin(), particles.end(), 0.0,
                                        [](double acc, const Particle& p) { return acc + p.weight; });

#ifdef DEBUG
        // Vérifications après le calcul des poids :
        double max_weight = std::max_element(particles.begin(), particles.end(),
                                             [](const Particle& a, const Particle& b) { return a.weight < b.weight; })->weight;

        double min_weight = std::min_element(particles.begin(), particles.end(),
                                             [](const Particle& a, const Particle& b) { return a.weight < b.weight; })->weight;

        double avg_weight = sum_wi / N_particles;

        std::cout << "Poids max : " << max_weight << std::endl;
        std::cout << "Poids min : " << min_weight << std::endl;
        std::cout << "Poids moyen : " << avg_weight << std::endl;
        std::cout << "Ratio max/min : " << max_weight/min_weight << std::endl;

        // Vérifier que le ratio n'est pas trop élevé (indiquant des problèmes de variance)
        if (max_weight/min_weight > 1000) {
            std::cout << "⚠️ Attention : grande variance des poids" << std::endl;
        }
#endif
        // Normalisation des poids


        for (auto& p : particles) p.weight /= sum_wi;

        /*
         * Vous avez commenté la condition sur l'ESS (Effective Sample Size).
         * Ce que cela signifie : Dans un filtre SMC classique (pour suivre une trajectoire),
         *  on ne rééchantillonne que si les poids sont trop déséquilibrés (ESS faible) pour éviter d'appauvrir la diversité.
         * Dans votre cas (Initialisation MCMC) : Vous forcez le rééchantillonnage systématiquement.
         * C'est logique pour votre usage. Votre but ici n'est pas de maintenir une distribution sur le long terme, mais de sélectionner les N meilleurs points de départ parmi un nuage de 10×N candidats.
         * Vous faites une étape de sélection Darwinienne pure avant de lancer les MCMC. */

        /* // calcul ESS
         double sum_wi2 = 0.0;
        for (int i = 0; i < N_particles; ++i)
            sum_wi2 += particles[i].weight * particles[i].weight;

        double ESS = 1.0 / sum_wi2;

        std::cout << " ESS : "<< ESS << std::endl;
        if (ESS  < 0.6 * N_particles)
            particles = resample(particles);
        */



        // Construction CDF
        std::vector<double> cdf(particles.size());
        cdf[0] = particles[0].weight;
        for (size_t i = 1; i < particles.size(); ++i) {
            cdf[i] = cdf[i-1] + particles[i].weight;
        }
        cdf.back() = 1.0;  // 👈 CRUCIAL

        // Tirages
        int N_new = mLoopChains.size();

        std::vector<Particle> new_particles;
        new_particles.reserve(N_new);

        // Rééchantillonnage systématique sécurisé
        /** @ref Tille, Y., s. d. Theorie Des Sondages - 2E Ed. Dunod. ISBN 10:2100797956, Chap. 5.6 **/


        double u0 = Generator::randomUniform() / N_new;
        for (int n = 0; n < N_new; ++n) {
            double u = u0 + n / (double)N_new;

            // Par construction : u ∈ [u0, u0 + (N_new-1)/N_new]
            //                    u ∈ [0, 1]

            // Trouver l'index
            auto it = std::lower_bound(cdf.begin(), cdf.end(), u);
            size_t i = std::distance(cdf.begin(), it);
            // Sécurité (ne devrait jamais être nécessaire)
#ifdef DEBUG
            if (i >= particles.size()) {
                std::cerr << "⚠️  Erreur inattendue: i=" << i
                          << ", u=" << u << ", cdf.back()=" << cdf.back() << std::endl;
                i = particles.size() - 1;
            }
#endif
            new_particles.push_back(particles[i]);
        }



        // Graines sélectionnées
        for (int n = 0; n < N_new; ++n) {
            mLoopChains[n].mSeed = new_particles[n].init_seed;
        }
        // initVariableForChain() reserve memory space
        mModel->mChains.resize(N_new);
        mModel->mPosteriorMeanGByChain.resize(N_new);

        mModel->initVariablesForChain();

    } else {

        // initVariableForChain() reserve memory space
        mModel->mChains.resize(mLoopChains.size());

        mModel->initVariablesForChain();
    }

    //----------------------- Chains --------------------------------------



    mModel->mLogInit += ModelUtilities::getMCMCSettingsLog(mModel);

    QStringList seeds;
    for (auto& chain : chains())
         seeds << QString::number(chain.mSeed);

    mModel->mLogInit += line(tr("List of used chain seeds (to be copied for re-use in MCMC Settings) : ") + seeds.join(";"));


    // copie la liste des pointeurs, pour garder l'ordre initiale des Events;
    // le mécanisme d'initialisation pour les courbes modifie cette liste, hors il faut la réablir pour les chaines suivantes
    std::vector<std::shared_ptr<Event>> initListEvents (mModel->mEvents.size());
    std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    QElapsedTimer globalTimer;
    globalTimer.start();

    qint64 lastUpdateTime = 0;

    unsigned estimatedTotalIter = (unsigned)((int)mLoopChains.size() *(1 + mLoopChains.at(0).mIterPerBurn + mLoopChains.at(0).mIterPerBatch*mLoopChains.at(0).mMaxBatchs + mLoopChains.at(0).mIterPerAquisition));
    unsigned iterDone = 0;
    std::vector<int> update_seed;
    update_seed.reserve(mLoopChains.size());

    // Création d'un scope limité pour used_seeds
    if (hybrid_SMC) {
        int master_seed = 123456; // Seed principal (paramètre utilisateur), pour assurer la reproductibilité
        Generator::initGenerator(master_seed);

        std::set<int> used_seeds;

        for (mChainIndex = 0; mChainIndex < mLoopChains.size(); ++mChainIndex) {
            int seed;
            // Garantir que la graine est unique par rapport aux graines déjà utilisées
            // Si les graines sont uniques, on les utilise pour l'initialisation et le reste du run
            do {
                seed = mLoopChains[mChainIndex].mSeed + Generator::randomUniformInt(0, 1000);
            } while (used_seeds.find(seed) != used_seeds.end());

            used_seeds.insert(seed);
            update_seed.push_back(seed);
        }

    } else { // On garde le fonctionnement des versions précédentes de CM
        for (auto& chain : chains())
            update_seed.push_back(chain.mSeed);

    }

    auto updateTimeEstimate = [&](const QString& phaseName) {
        qint64 now = globalTimer.elapsed();
        if (now - lastUpdateTime > 1000 && iterDone > 10) {
            double avgTimePerIter = now / (double)iterDone;
            qint64 interTime = (qint64)(avgTimePerIter * (estimatedTotalIter - iterDone));

            emit setMessage(tr("Chain %1 / %2").arg(
                                QString::number(mChainIndex+1),
                                QString::number(mLoopChains.size())) +
                            " : " + phaseName + "\t ; Total Estimated time left " + DHMS(interTime));

            lastUpdateTime = now;
        }
    };
    auto updateTimeEstimateMessage = [&](const QString& phaseName) {
        qint64 now = globalTimer.elapsed();
        double avgTimePerIter = now / (double)iterDone;
        qint64 interTime = (qint64)(avgTimePerIter * (estimatedTotalIter - iterDone));

        emit setMessage(tr("Chain %1 / %2").arg(
                            QString::number(mChainIndex+1),
                            QString::number(mLoopChains.size())) +
                        " : " + phaseName + "\t ; Total Estimated time left " + DHMS(interTime));

        lastUpdateTime = now;

    };


    for (mChainIndex = 0; mChainIndex < mLoopChains.size(); ++mChainIndex) {

        log += "<hr>";

        ChainSpecs& chain = mLoopChains[mChainIndex];
        // Utiliser le seed d'initialisation (sélectionné par SMC)
        Generator::initGenerator(chain.mSeed); // 👈 Seed original

        //----------------------- Initialization --------------------------------------

        if (isInterruptionRequested()) {
            mAbortedReason = ABORTED_BY_USER;
            return;
        }
        mState = eInit;

        ++iterDone;
        emit stepChanged(tr("Chain : %1 / %2").arg(QString::number(mChainIndex + 1), QString::number(mLoopChains.size()))  + " : " + tr("Initializing"), 0, estimatedTotalIter);

        updateTimeEstimateMessage(tr("Initializing"));

        QElapsedTimer initTime;
        initTime.start();
        mAbortedReason = initialize();

        //std::cout << "SMC score : " << SMC_score() <<std::endl;
        if (!mAbortedReason.isEmpty())
            return;

        memo();
        chain.mInitElapsedTime = initTime.elapsed();
        initTime.~QElapsedTimer();

        mModel->mLogInit += "<hr>";
        mModel->mLogInit += line(textBold(tr("INIT CHAIN %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))));
        mModel->mLogInit += line("Init Seed : " + QString::number(chain.mSeed)) + " SMC score :" + QString::number(SMC_score());
        qDebug() << " mLogInit Seed :  " <<  QString::number(chain.mSeed);

        mModel->mLogInit += ModelUtilities:: modelStateDescriptionHTML(mModel);

        //----------------------- Burnin --------------------------------------
        //  Rejuvenation avant l'échantillonnage , pour éviter le risque d'avoir plusieurs fois la même graine d'initialisation
        if (hybrid_SMC) {
            qDebug() << " Update_seed :  " <<  QString::number(update_seed[mChainIndex]);
            mModel->mLogInit += line("Hybrid SMC");
            mModel->mLogInit += line("Update Seed : " + QString::number(update_seed[mChainIndex]));
            Generator::initGenerator(update_seed[mChainIndex]); // 👈 Nouvelle graine pour MCMC ou graine identique à l'initialisation
        }
        updateTimeEstimateMessage(tr("Burn-in"));

        mState = eBurning;

        QElapsedTimer burningTime;
        burningTime.start();

        while (chain.mBurnIterIndex < chain.mIterPerBurn) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }

            try {
                update();


#ifdef _WIN32
//    SetThreadExecutionState( ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);

#endif
            } catch (QString error) {
                mAbortedReason = error;
                return;
            }

            memo();
            ++chain.mBurnIterIndex;
            ++chain.mTotalIter;

            ++iterDone;

            updateTimeEstimate("Burn-in");  // 👈

            emit stepProgressed(iterDone);
        }
        chain.burnElapsedTime = burningTime.elapsed();
        burningTime.~QElapsedTimer();

        //----------------------- Adaptation --------------------------------------
// ici il faut supprimer les valeurs dans mLastAccept, qui parasite le début de l'adaptation
        updateTimeEstimateMessage(tr("Burn-in"));
        mState = eAdapting;

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
                    update();
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
#endif

                } catch (QString error) {
                    mAbortedReason = error;
                    return;
                }

                memo();
                ++chain.mBatchIterIndex;
                ++chain.mTotalIter;
                ++iterDone;

                emit stepProgressed(iterDone);
                qApp->processEvents(); //This function is especially useful if you have a long running operation and want to show its progress
            }
            ++chain.mBatchIndex;


            updateTimeEstimate("Adapting");  // 👈
            //qDebug()<<"[MCMCLoop::run] mBatchIndex -------"<< chain.mBatchIndex<<" ------------";
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

        //const bool refresh_process (chain.mAdaptElapsedTime == 10000); // force refresh progress loop bar if the model is complex
        //----------------------- Aquisition --------------------------------------

        updateTimeEstimateMessage(tr("Aquisition"));
        mState = eAquisition;
        QElapsedTimer aquisitionTime;
        aquisitionTime.start();

        int thinningIdx = 0;
        int batchIdx = 1;
        int totalBacth = chain.mBatchIndex;
        bool OkToMemo;

        int thinning_OkToMemo = 0; // compte le nombre entre chaque update = memo==true

        chain.mRealyAccepted = 0;

        while (chain.mAquisitionIterIndex < chain.mIterPerAquisition) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }
            ++iterDone;

            try {
                OkToMemo =  update();
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
                    memo();
                    thinning_OkToMemo = 0;

                    ++chain.mRealyAccepted;
                }

                mModel->memo_accept(mChainIndex);

            }

            if (batchIdx == chain.mIterPerBatch) {
                adapt(totalBacth);
                batchIdx = 1;
                totalBacth++;

            } else {
                batchIdx++;
            }

            ++chain.mAquisitionIterIndex;
            ++chain.mTotalIter;

            updateTimeEstimate("Acquisition");  // 👈
            if (!(chain.mAquisitionIterIndex % chain.mIterPerBatch)) {
              qApp->processEvents();
            }

            emit stepProgressed(iterDone);
        }
        chain.mAcquisitionElapsedTime = aquisitionTime.elapsed();
        aquisitionTime.~QElapsedTimer();
        mModel->mLogResults += line(tr("Acquisition time elapsed %1").arg(DHMS(chain.mAcquisitionElapsedTime)));

        // rétablissement de l'ordre des Events, indispensable en cas de calcul de courbe. Car le update modifie l'ordre des events et utile pour la sauvegarde de ChronoModel_Bash
        std::copy(initListEvents.begin(), initListEvents.end(), mModel->mEvents.begin() );
    }

    mModel->mChains = mLoopChains;

    //-----------------------------------------------------------------------

    emit stepChanged(tr("Computing posterior distributions and numerical results (HPD, credibility, ...)"), 0, 0);
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
    try {
        finalize();

    } catch (QString error) {
        mAbortedReason = error;
        return;
    }


#ifdef DEBUG
    QTime endTime = QTime::currentTime();

    qDebug()<<"[MCMCLoop::run] Model computed";
    qDebug()<<tr("finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug()<<tr("Total time elapsed %1").arg(QString(DHMS(startTime.elapsed())));
#endif


}
