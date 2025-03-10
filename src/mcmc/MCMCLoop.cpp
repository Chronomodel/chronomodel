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
    mModel = model;//.lock();
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
    //mModel.reset();
    //mProjectLock->mLoop = nullptr;
    //mProject = nullptr;
}

void MCMCLoop::setMCMCSettings(const MCMCSettings &s)
{
    mLoopChains.clear();
    for (int i=0; i<s.mNumChains; ++i) {
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
        //ev->mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;// not yet integrate within update_321

# else
        ev->mS02Theta.mSamplerProposal = MHVariable::eFixe;
#endif
    }
    // -------------------------- Init gamma ------------------------------
    emit stepChanged(tr("Initializing Phase Gaps..."), 0, (int)phasesConstraints.size());
    int i = 0;
    try {
        for (auto&& phC : phasesConstraints) {
            phC->initGamma();
            if (isInterruptionRequested())
                return ABORTED_BY_USER;
            ++i;
            emit stepProgressed(i);
        }
    }  catch (...) {
        qWarning() <<"Init Gamma ???";
        mAbortedReason = QString("Error in Init Gamma ???");
        return mAbortedReason;
    }

    // -------------------------- Init tau -----------------------------------------
    emit stepChanged(tr("Initializing Phase Durations..."), 0, (int)phases.size());
    i = 0;
    try {
        for (auto&& ph : phases) {
            ph->initTau(tminPeriod, tmaxPeriod);
            qDebug()<<"[MCMCLoop::initialize_time] " <<ph->getQStringName()<<" init Tau ="<<ph->mTau.mX;
            if (isInterruptionRequested())
                return ABORTED_BY_USER;
            ++i;
            emit stepProgressed(i);
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
        qWarning() <<"Init Bound ???";
        mAbortedReason = QString("Error in Init Bound ???");
        return mAbortedReason;
    }
    /* ----------------------------------------------------------------
     *  Init theta event, ti, ...
     * ---------------------------------------------------------------- */

    std::vector<std::shared_ptr<Event>> unsortedEvents = ModelUtilities::unsortEvents(allEvents);

    emit stepChanged(tr("Initializing Events..."), 0, (int)unsortedEvents.size());
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
            qDebug()<<"[MCMCLoop::initialize_time] " <<p->getQStringName()<<" init alpha ="<<p->mAlpha.mX<<" beta="<<p->mBeta.mX;
        }

        //---------------
        int i = 0;
        if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {

            for (std::shared_ptr<Event> uEvent : unsortedEvents) {
                if (uEvent->mType == Event::eDefault) {

                    mModel->initNodeEvents();
                    const double min = uEvent->getThetaMinRecursive_v3(tminPeriod);
                    mModel->initNodeEvents();
                    const double max = uEvent->getThetaMaxRecursive_v3(tmaxPeriod);

                    if (min > max) {
                        const int seed = mLoopChains.at(mChainIndex).mSeed;
                        qDebug()<<QString("[MCMCLoop::initialize_time] Error Init for event : %1 : min = %2 : max = %3-------Seed = %4").arg(uEvent->getQStringName(), QString::number(min, 'f', 30), QString::number(max, 'f', 30), QString::number(seed));
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
                        qDebug()<<QString("[MCMCLoop::initialize_time] Egality Init for event : %1 : min = %2 : max = %3-------Seed = %4").arg(uEvent->getQStringName(), QString::number(min, 'f', 30), QString::number(max, 'f', 30), QString::number(mLoopChains.at(mChainIndex).mSeed));

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
                        double sigma = double (data.std);
#ifdef DEBUG
                        if (sigma == 0.)
                            return "[MCMCLoop::initialize_time] sigma == 0";
#endif
                        if (is_wiggle) {
                            const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mWiggleCalibration->mRepartition);
                            date.mTi.mX = date.mWiggleCalibration->mTmin + idx * date.mWiggleCalibration->mStep;
                            // modif du 2021-06-16 pHd

                        } else if (!date.mCalibration->mRepartition.empty()) {
                            const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mCalibration->mRepartition);
                            date.mTi.mX = date.mCalibration->mTmin + idx * date.mCalibration->mStep;
                            // modif du 2021-06-16 pHd

                        } else { // in the case of mRepartion curve is null, we must init ti outside the study period
                            // For instance we use a gaussian random sampling
                            sigma = tmaxPeriod - tminPeriod;
                            qDebug()<<"[MCMCLoop::initialize_time] mRepartion curve is null for"<< date.getQStringName();
                            const double u = Generator::gaussByBoxMuller(0., sigma);
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
                        date.mSigmaTi.mSigmaMH = 1.0;//1.27;  //1.;

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
#ifdef CODE_KOMLAN \
                    // new code
                    //uEvent->mS02Theta.mX = 1.0 / Generator::gammaDistribution(1., uEvent->mBetaS02);
                    uEvent->mS02Theta.accept_update(1.0 / Generator::gammaDistribution(1., uEvent->mBetaS02));

#else
                    //uEvent->mS02Theta.mX = uEvent->mDates.size() / s02_sum;
                    uEvent->mS02Theta.accept_update(uEvent->mDates.size() / s02_sum);

#endif





                    // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
                    uEvent->mTheta.mSigmaMH = sqrt(uEvent->mS02Theta.mX);
                    uEvent->mAShrinkage = 1.;




                }

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i++);

            }

        } else {
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

                emit stepProgressed(i++);

            }
            // Check strati constraints , need alpha and beta Phase
            /*for (Event* ev : mModel->mEvents) {
                const double min = ev->getThetaMin(tminPeriod); // need alpha and beta Phase
                const double max = ev->getThetaMax(tmaxPeriod);


                qDebug() << QString("[MCMCLoop::initialize_time] Init for event theta fixed : %1 : min = %2 : max = %3  ->theta = %4 thetaRed = %5-------").arg(ev->mName, QString::number(min, 'f', 3), QString::number(max, 'f', 3), QString::number(ev->mTheta.mX, 'f', 3), QString::number(ev->mThetaReduced, 'f', 3));

                if (min > max) {
                    throw QObject::tr("Error for event theta fixed : %1 : min = %2 : max = %3").arg(ev->mName, QString::number(min), QString::number(max));
                }
            }*/


        }

    }  catch (const QString e) {
        qWarning() <<"Init theta event, ti,  ???"<<e;
        mAbortedReason = e;
        return mAbortedReason;
    }


    // --------------------------- Init alpha and beta phases ----------------------
    emit stepChanged(tr("Initializing Phases..."), 0, (int)phases.size());
    try {
        i = 0;
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

            emit stepProgressed(++i);
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

                qDebug() << QString("[MCMCLoop::initialize_time] Init for event theta fixed : %1 : min = %2 : max = %3  ->theta = %4 thetaRed = %5-------").arg(ev->getQStringName(), QString::number(min, 'f', 3), QString::number(max, 'f', 3), QString::number(ev->mTheta.mX, 'f', 3), QString::number(ev->mThetaReduced, 'f', 3));

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



    //----------------------- Chains --------------------------------------

    // initVariableForChain() reserve memory space

    mModel->initVariablesForChain();

    mModel->mLogInit += ModelUtilities::getMCMCSettingsLog(mModel);

    QStringList seeds;
    for (auto& chain : chains())
         seeds << QString::number(chain.mSeed);

    mModel->mLogInit += line(tr("List of used chain seeds (to be copied for re-use in MCMC Settings) : ") + seeds.join(";"));


     // copie la liste des pointeurs, pour garder l'ordre initiale des Events;
     // le mécanisme d'initialisation pour les courbes modifie cette liste, hors il faut la réablir pour les chaines suivantes
     std::vector<std::shared_ptr<Event>> initListEvents (mModel->mEvents.size());
     std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    unsigned estimatedTotalIter = (unsigned)((int)mLoopChains.size() *(mLoopChains.at(0).mIterPerBurn + mLoopChains.at(0).mIterPerBatch*mLoopChains.at(0).mMaxBatchs + mLoopChains.at(0).mIterPerAquisition));
    unsigned iterDone = 0;
    for (mChainIndex = 0; mChainIndex < mLoopChains.size(); ++mChainIndex) {

        log += "<hr>";

        ChainSpecs& chain = mLoopChains[mChainIndex];
        Generator::initGenerator(chain.mSeed);

        //----------------------- Initialization --------------------------------------

        if (isInterruptionRequested()) {
            mAbortedReason = ABORTED_BY_USER;
            return;
        }
        mState = eInit;

        emit stepChanged(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))  + " : " + tr("Initializing MCMC"), 0, 0);

        QElapsedTimer initTime;
        initTime.start();
        mAbortedReason = initialize();

        if (!mAbortedReason.isEmpty())
            return;

        memo();
        chain.mInitElapsedTime = initTime.elapsed();
        initTime.~QElapsedTimer();

        mModel->mLogInit += "<hr>";
        mModel->mLogInit += line(textBold(tr("INIT CHAIN %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))));
        mModel->mLogInit += line("Seed : " + QString::number(chain.mSeed));
        qDebug()<<" mLogInit Seed :  "<< QString::number(chain.mSeed);

        mModel->mLogInit += ModelUtilities:: modelStateDescriptionHTML(mModel);
        /*
// Save mLogInit for debug
#ifdef DEBUG
        mProject->mModel->mChains[mChainIndex].mInitElapsedTime = mChains[mChainIndex].mInitElapsedTime;// only to take the time
        QString dirPath = "../../../../..";//QFileInfo(".").absolutePath() ;
        QFile file(dirPath + "/Last_Project_Initialization_DEBUG.html");

        const QString projectName = tr("Project filename : %1").arg(mProject->mName);
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream output(&file);
            output<<"<!DOCTYPE html>"<< Qt::endl;
            output<<"<html>"<< Qt::endl;
            output<<"<body>"<< Qt::endl;

            output<<"<h2>"<< projectName+ "</h2>" << Qt::endl;
            output<<"<hr>";
            output<<mProject->mModel->getInitLog();

            output<<"</body>"<< Qt::endl;
            output<<"</html>"<< Qt::endl;
        }
        file.close();
#endif
*/
        //----------------------- Burnin --------------------------------------

        emit stepChanged(tr("Chain : %1 / %2").arg(QString::number(mChainIndex + 1), QString::number(mLoopChains.size()))  + " : " + tr("Burn-in"), 0, chain.mIterPerBurn);
        mState = eBurning;

        QElapsedTimer burningTime;
        burningTime.start();

        qint64 interTime = 0;
        while (chain.mBurnIterIndex < chain.mIterPerBurn) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }

            try {
                update();


                // ---test
                /*
                const auto& test_spline = mModel->mSpline.splineX;
                const auto& test_events = mModel->mEvents;


                for (auto event : test_events) {

                    auto it = std::find(test_spline.vecThetaReduced.begin(), test_spline.vecThetaReduced.end(), event->mThetaReduced);
                    if (it != test_spline.vecThetaReduced.end()) {
                        size_t thetaIdx = std::distance(test_spline.vecThetaReduced.begin(), it);
                        qDebug()<<"[MCMCLoop] burn-in" <<thetaIdx<< event->name();

                    }             else {
                        qDebug()<<"[MCMCLoop] burn-in errror" << event->name() << event->mThetaReduced;
                    }
                }
*/
                // -- fin test




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
            interTime = burningTime.elapsed() * (double)(estimatedTotalIter - iterDone) / (double)chain.mBurnIterIndex;
            emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()) + " : " + "Burn-in ; Total Estimated time left " + DHMS(interTime)));

            emit stepProgressed(chain.mBurnIterIndex);
        }
        chain.burnElapsedTime = burningTime.elapsed();
        burningTime.~QElapsedTimer();

        //----------------------- Adaptation --------------------------------------

        emit stepChanged(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))  + " : "  + "Adapting ; Total Estimated time left " + DHMS(interTime), 0, chain.mMaxBatchs * chain.mIterPerBatch);
        emit stepProgressed(0);
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


                // ---test
/*
                const auto& test_spline = mModel->mSpline;
                for (auto event : mModel->mEvents) {

                    auto it = std::find(test_spline.splineX.vecThetaReduced.begin(), test_spline.splineX.vecThetaReduced.end(), event->mThetaReduced);
                    if (it != test_spline.splineX.vecThetaReduced.end()) {
                        size_t thetaIdx = std::distance(test_spline.splineX.vecThetaReduced.begin(), it);
                        qDebug()<<"[MCMCLoop] memo adapt" <<thetaIdx;

                    }             else {
                        qDebug()<<"[MCMCLoop] errror";
                    }
                }
*/
                // -- fin test



                memo();
                ++chain.mBatchIterIndex;
                ++chain.mTotalIter;

                emit stepProgressed(chain.mBatchIndex * chain.mIterPerBatch + chain.mBatchIterIndex);
                qApp->processEvents(); //This function is especially useful if you have a long running operation and want to show its progress
            }
            ++chain.mBatchIndex;

            iterDone += chain.mIterPerBatch;
            interTime = adaptTime.elapsed() * (double)(estimatedTotalIter - iterDone)/ (double)(chain.mIterPerBatch*chain.mBatchIndex);

            emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()) + " : " + "Adapting ; Total Estimated time left " + DHMS(interTime)));

            //qDebug()<<"[MCMCLoop::run] mBatchIndex -------"<< chain.mBatchIndex<<" ------------";
            if (adapt(chain.mBatchIndex))
                    break;


        }
        // Fix Total iteration if adaptation break before the end
        estimatedTotalIter -= (chain.mMaxBatchs-chain.mBatchIndex)*chain.mIterPerBatch;
        interTime = adaptTime.elapsed() * (double)(estimatedTotalIter - iterDone)/ (double)(chain.mIterPerBatch*chain.mBatchIndex);

        emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()) + " : " + "Adapting ; Total Estimated time left " + DHMS(interTime)));


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

        const bool refresh_process (chain.mAdaptElapsedTime == 10000); // force refresh progress loop bar if the model is complex
        //----------------------- Aquisition --------------------------------------

        emit stepChanged(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size())) + " : Aquisition ; Total Estimated time left " + DHMS(interTime), 0, chain.mIterPerAquisition);
        emit stepProgressed(0);
        mState = eAquisition;
        QElapsedTimer aquisitionTime;
        aquisitionTime.start();

        int thinningIdx = 1;
        int batchIdx = 1;
        int totalBacth = chain.mBatchIndex;
        bool OkToMemo;
        chain.mRealyAccepted = 0;

        while (chain.mAquisitionIterIndex < chain.mIterPerAquisition) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }
            ++iterDone;



            try {
                OkToMemo =  update();
#ifdef _WIN32
    SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
#endif
            } catch (QString error) {
                mAbortedReason = error;
                return;
            }

            if (thinningIdx == chain.mThinningInterval) {

                thinningIdx = 1;

                if (OkToMemo) {
                    memo();
                     ++chain.mRealyAccepted;
                }

                mModel->memo_accept(mChainIndex);

            } else {
                    thinningIdx++;
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

            if (!(chain.mAquisitionIterIndex % chain.mIterPerBatch) || refresh_process) {

                interTime = aquisitionTime.elapsed() * (double) (estimatedTotalIter - iterDone) / (double) chain.mAquisitionIterIndex;

                emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()) + " : Aquisition ; Total Estimated time left " + DHMS(interTime)));
                qApp->processEvents();
            }


            emit stepProgressed(chain.mAquisitionIterIndex);
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
        this->finalize();

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
