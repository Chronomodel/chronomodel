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

#include "MCMCLoop.h"

#include "Bound.h"
#include "CalibrationCurve.h"
#include "Generator.h"
#include "Project.h"
#include "QtCore/qcoreapplication.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"

#include <QDebug>
#include <QTime>

#include <QtWidgets>

#ifdef _WIN32
#include "winbase.h"
#include "windows.h"
#endif

MCMCLoop::MCMCLoop():
    mChainIndex (0),
    mState (eBurning),
    mProject (nullptr)
{
    mAbortedReason = QString();
}

MCMCLoop::MCMCLoop(Project *project):
    mChainIndex (0),
    mState (eBurning),
    mProject (project)
{
    mProject->mLoop = this;
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
    mProject->mLoop = nullptr;
    mProject = nullptr;
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
        mLoopChains.append(chain);
    }
}

const QList<ChainSpecs> &MCMCLoop::chains() const
{
    return mLoopChains;
}


QString MCMCLoop::initialize_time(Model* model)
{
    tminPeriod = model->mSettings.mTmin;
    tmaxPeriod = model->mSettings.mTmax;

    QList<Event*>& allEvents (model->mEvents);
    QList<Phase*>& phases (model->mPhases);
    QList<PhaseConstraint*>& phasesConstraints (model->mPhaseConstraints);

    if (isInterruptionRequested())
        return ABORTED_BY_USER;
    // initialisation des bornes
    // ---------------------- Reset Events ---------------------------
    for (Event* ev : allEvents) {
        ev->mInitialized = false;

#ifdef S02_BAYESIAN
        //ev->mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;// not yet integrate within update_321

# else
        ev->mS02Theta.mSamplerProposal = MHVariable::eFixe;
#endif
    }
    // -------------------------- Init gamma ------------------------------
    emit stepChanged(tr("Initializing Phase Gaps..."), 0, phasesConstraints.size());
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
    emit stepChanged(tr("Initializing Phase Durations..."), 0, phases.size());
    i = 0;
    try {
        for (auto&& ph : phases) {
            ph->initTau(tminPeriod, tmaxPeriod);

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
        for (Event* ev : allEvents) {
            if (ev->mType == Event::eBound) {
                Bound* bound = dynamic_cast<Bound*>(ev);

                if (bound) {
                    bound->mTheta.mX = bound->mFixed;
                    bound->mThetaReduced = model->reduceTime(bound->mTheta.mX);
                    bound->mTheta.mLastAccepts.clear();
                    bound->mTheta.memo(); // non sauvegarder dans Loop.memo()
                    bound->mInitialized = true;

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

    QList<Event*> unsortedEvents = ModelUtilities::unsortEvents(allEvents);

    emit stepChanged(tr("Initializing Events..."), 0, unsortedEvents.size());
    qDebug()<<" mLoopChains seed = "<< mLoopChains[0].mSeed;
    try {

        // Check Strati constraint
        for (Event* ev : unsortedEvents) {
            model->initNodeEvents();
            QString circularEventName = "";
            QList<Event*> startEvents = QList<Event*>();

            const bool ok (ev->getThetaMaxPossible (ev, circularEventName, startEvents));
            if (!ok) {
                mAbortedReason = QString(tr("Warning : Find Circular Constraint Path %1  %2 ")).arg (ev->mName, circularEventName);
                return mAbortedReason;
            }

            // Controle la cohérence des contraintes strati-temporelle et des valeurs de profondeurs
            if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth ) {
                for (auto&& eForWard : ev->mConstraintsFwd) {
                    const bool notOk (ev->mXIncDepth > eForWard->mEventTo->mXIncDepth);
                    if (notOk) {
                        mAbortedReason = QString(tr("Warning: chronological constraint not in accordance with the stratigraphy: %1 - %2 path, control depth value!")).arg (ev->mName, eForWard->mEventTo->mName);
                        return mAbortedReason;
                    }
                }
            }
        }
        int i = 0;
        if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {

            for (Event* uEvent : unsortedEvents) {
                if (uEvent->mType == Event::eDefault) {

                    model->initNodeEvents(); // Doit être réinitialisé pour toute recherche getThetaMinRecursive et getThetaMaxRecursive
                    QString circularEventName = "";

                    const double min = uEvent->getThetaMinRecursive (tminPeriod);

                    // ?? Comment circularEventName peut-il être pas vide ?
                    if (!circularEventName.isEmpty()) {
                        mAbortedReason = QString(tr("Warning : Find Circular constraint with %1  bad path  %2 ")).arg(uEvent->mName, circularEventName);
                        return mAbortedReason;
                    }

                    model->initNodeEvents();
                    const double max = uEvent->getThetaMaxRecursive(tmaxPeriod);

                    if (min >= max) {
                        mAbortedReason = QString(tr("Error Init for event : %1 : min = %2 : max = %3-------").arg(uEvent->mName, QString::number(min, 'f', 30), QString::number(max, 'f', 30)));
                        return mAbortedReason;
                    }
                    // ----------------------------------------------------------------
                    // Curve init Theta event :
                    // On initialise les theta près des dates ti
                    // ----------------------------------------------------------------

                    sampleInCumulatedRepartition(uEvent, model->mSettings, min, max);

                    uEvent->mThetaReduced = model->reduceTime(uEvent->mTheta.mX);
                    uEvent->mInitialized = true;

                    qDebug() << tr("----- Init for event : %1 : min = %2 : max = %3 ->theta = %4 thetaRed = %5-------").arg(uEvent->mName, QString::number(min, 'f', 30), QString::number(max, 'f', 30), QString::number(uEvent->mTheta.mX, 'f', 30), QString::number(uEvent->mThetaReduced, 'f', 30));
                    // ----------------------------------------------------------------

                    double s02_sum = 0.;

                    for (Date& date : uEvent->mDates) {

                        // 1 - Init ti

                        // modif du 2021-06-16 pHd
                        const FunctionStat &data = analyseFunction(date.mCalibration->mMap);
                        double sigma = double (data.std);
#ifdef DEBUG
                        if (sigma == 0.)
                            return "sigma == 0";
#endif
                        //
                        if (!date.mCalibration->mRepartition.isEmpty()) {
                            const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mCalibration->mRepartition);
                            date.mTi.mX = date.mCalibration->mTmin + idx * date.mCalibration->mStep;
                            // modif du 2021-06-16 pHd

                        } else { // in the case of mRepartion curve is null, we must init ti outside the study period
                            // For instance we use a gaussian random sampling
                            sigma = tmaxPeriod - tminPeriod;
                            qDebug()<<"mRepartion curve is null for"<< date.mName;
                            const double u = Generator::gaussByBoxMuller(0., sigma);
                            if (u<0)
                                date.mTi.mX = tminPeriod + u;
                            else
                                date.mTi.mX = tmaxPeriod + u;

                            if (date.mTi.mSamplerProposal == MHVariable::eInversion) {
                                qDebug()<<"Automatic sampling method exchange eInversion to eMHSymetric for"<< date.mName;
                                date.mTi.mSamplerProposal = MHVariable::eMHSymetric;
                                date.autoSetTiSampler(true);
                            }

                        }

                        // 2 - Init Delta Wiggle matching and Clear mLastAccepts array
                        date.initDelta(uEvent);
                        date.mWiggle.mLastAccepts.clear();
                        //date.mWiggle.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
                        date.mWiggle.tryUpdate(date.mWiggle.mX, 2.);

                        // 3 - Init sigma MH adaptatif of each Data ti
                        date.mTi.mSigmaMH = sigma;

                        // 4 - Clear mLastAccepts array and set this init at 100%
                        date.mTi.mLastAccepts.clear();
                        //date.mTheta.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
                        date.mTi.tryUpdate(date.mTi.mX, 2.);

                        // 5 - Init Sigma_i and its Sigma_MH
                        date.mSigmaTi.mX = std::abs(date.mTi.mX - (uEvent->mTheta.mX - date.mDelta));

                        if (date.mSigmaTi.mX <= 1E-6) {
                            date.mSigmaTi.mX = 1E-6; // Add control the 2015/06/15 with PhL
                            //log += line(date.mName + textBold("Sigma indiv. <=1E-6 set to 1E-6"));
                        }
                        date.mSigmaTi.mSigmaMH = 1.;//1.27;  //1.;

                        date.mSigmaTi.mLastAccepts.clear();
                        date.mSigmaTi.tryUpdate(date.mSigmaTi.mX, 2.);

                        // intermediary calculus for the harmonic average
                        s02_sum += 1. / (sigma * sigma);
                    }

                    // 4 - Init S02 of each Event

                    const double sqrt_S02_harmonique = sqrt(uEvent->mDates.size() / s02_sum);
                    uEvent->mBetaS02 = 1.004680139*(1 - exp(- 0.0000847244 * pow(sqrt_S02_harmonique, 2.373548593)));
#ifdef CODE_KOMLAN
                    // new code
                    uEvent->mS02Theta.mX = 1. / Generator::gammaDistribution(1., uEvent->mBetaS02);
                    uEvent->mS02Theta.mSigmaMH = 1.;
#else
                    uEvent->mS02Theta.mX = uEvent->mDates.size() / s02_sum;
                    uEvent->mS02Theta.mSigmaMH = 1.;

#endif

                    uEvent->mS02Theta.mLastAccepts.clear();
                    uEvent->mS02Theta.tryUpdate(uEvent->mS02Theta.mX, 2.);


                    // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
                    uEvent->mTheta.mSigmaMH = sqrt(uEvent->mS02Theta.mX);
                    uEvent->mAShrinkage = 1.;

                    // 6- Clear mLastAccepts  array
                    uEvent->mTheta.mLastAccepts.clear();
                    //unsortedEvents.at(i)->mTheta.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
                    uEvent->mTheta.tryUpdate(uEvent->mTheta.mX, 2.);

                }

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i++);

            }

        } else {
            for (Event* uEvent : unsortedEvents) {
                // ----------------------------------------------------------------
                // Curve init Theta event :
                // On initialise les theta près des dates ti
                // ----------------------------------------------------------------
                if (uEvent->mType == Event::eDefault)
                    sampleInCumulatedRepartition_thetaFixe(uEvent, model->mSettings);
                else
                    uEvent->mTheta.mX = static_cast<Bound*>(uEvent)->mFixed;
                // nous devons sauvegarder la valeur ici car dans loop.memo(), les variables fixes ne sont pas memorisées.
                // Pourtant, il faut récupèrer la valeur pour les affichages et les stats
                uEvent->mTheta.memo();

                uEvent->mThetaReduced = model->reduceTime(uEvent->mTheta.mX);
                uEvent->mInitialized = true;
                uEvent->mTheta.mSamplerProposal = MHVariable::eFixe;

                for (auto&& date : uEvent->mDates) {
                    date.mTi.mSamplerProposal = MHVariable::eFixe;
                    date.mTi.mX = uEvent->mTheta.mX;
                    date.mTi.memo();

                    // 2 - Init Delta Wiggle matching and Clear mLastAccepts array
                    date.initDelta(uEvent);
                    date.mWiggle.mLastAccepts.clear();
                    //date.mWiggle.mAllAccepts->clear(); //don't clean, avalable for cumulate chain

                    // 3 - Init sigma MH adaptatif of each Data ti
                    date.mTi.mSigmaMH = 1;

                    // 4 - Clear mLastAccepts array and set this init at 100%
                    date.mTi.mLastAccepts.clear();
                    //date.mTheta.mAllAccepts->clear(); //don't clean, avalable for cumulate chain

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
                //uEvent->mTheta.mAllAccepts->clear(); //don't clean, avalable for cumulate chain

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i++);

            }
            // Check strati constraints .
            for (Event* ev : model->mEvents) {
                const double min = ev->getThetaMin(tminPeriod);
                const double max = ev->getThetaMax(tmaxPeriod);
                if (min >= max) {
                    throw QObject::tr("Error for event theta fixed: %1 : min = %2 : max = %3").arg(ev->mName, QString::number(min), QString::number(max));
                }
            }


        }

    }  catch (const QString e) {
        qWarning() <<"Init theta event, ti,  ???"<<e;
        //mAbortedReason = QString("Error in Init theta event, ti,  ???");
        mAbortedReason = e;
        return mAbortedReason;
    }


    // --------------------------- Init alpha and beta phases ----------------------
    emit stepChanged(tr("Initializing Phases..."), 0, phases.size());
    try {
        i = 0;
        for (auto&& phase : phases ) {

            // tau is still initalize

            double tmp = phase->mEvents[0]->mTheta.mX;
            // All Event must be Initialized
            std::for_each(PAR phase->mEvents.begin(), phase->mEvents.end(), [&tmp] (Event* ev){tmp = std::min(ev->mTheta.mX, tmp);});
            phase->mAlpha.mX = tmp;

            tmp = phase->mEvents[0]->mTheta.mX;
            std::for_each(PAR phase->mEvents.begin(), phase->mEvents.end(), [&tmp] (Event* ev){tmp = std::max(ev->mTheta.mX, tmp);});
            phase->mBeta.mX = tmp;

            phase->mDuration.mX = phase->mBeta.mX - phase->mAlpha.mX;

            if (isInterruptionRequested())
                return ABORTED_BY_USER;

            emit stepProgressed(++i);
        }

    }  catch (...) {
        mAbortedReason = QString("Init alpha and beta phases  ???");
        return mAbortedReason;
    }
    return QString();
}



void MCMCLoop::run()
{
#if DEBUG
    qDebug()<<"[MCMCLoop] run()";
#endif

    QElapsedTimer startTime;
    startTime.start();

    QString mDate = QDateTime::currentDateTime().toString("dddd dd MMMM yyyy");


    QString log = "Start " + mDate + " -> " + QTime::currentTime().toString("hh:mm:ss.zzz");


    //----------------------- Calibrating --------------------------------------

    emit stepChanged(tr("Calibrating data..."), 0, 0);

    mAbortedReason = this->calibrate();
    if (!mAbortedReason.isEmpty())
        return;



    //----------------------- Chains --------------------------------------

    // initVariableForChain() reserve memory space

    mProject->mModel->initVariablesForChain();

    mProject->mModel->mLogInit += ModelUtilities::getMCMCSettingsLog(mProject->mModel);

    QStringList seeds;
    for (auto& chain : chains())
         seeds << QString::number(chain.mSeed);

     mProject->mModel->mLogInit += line(tr("List of used chain seeds (to be copied for re-use in MCMC Settings) : ") + seeds.join(";"));


     // copie la liste des pointeurs, pour garder l'ordre initiale des Events;
     // le mécanisme d'initialisation pour les courbes modifie cette liste, hors il faut la réablir pour les chaines suivantes
     std::vector<Event*> initListEvents (mProject->mModel->mEvents.size());
     std::copy(mProject->mModel->mEvents.begin(), mProject->mModel->mEvents.end(), initListEvents.begin() );

    unsigned estimatedTotalIter = mLoopChains.size() *(mLoopChains.at(0).mIterPerBurn + mLoopChains.at(0).mIterPerBatch*mLoopChains.at(0).mMaxBatchs + mLoopChains.at(0).mIterPerAquisition);
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

        emit stepChanged(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))  + " : " + tr("Initialising MCMC"), 0, 0);

        QElapsedTimer initTime;
        initTime.start();
        mAbortedReason = initialize();

        if (!mAbortedReason.isEmpty())
            return;

        memo();
        chain.mInitElapsedTime = initTime.elapsed();
        initTime.~QElapsedTimer();

        mProject->mModel->mLogInit += "<hr>";
        mProject->mModel->mLogInit += line(textBold(tr("INIT CHAIN %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))));
        mProject->mModel->mLogInit += line("Seed : " + QString::number(chain.mSeed));
    qDebug()<<" mLogInit Seed :  "<< QString::number(chain.mSeed);
        mProject->mModel->mLogInit += ModelUtilities:: modelStateDescriptionHTML(static_cast<ModelCurve*>(mProject->mModel) );
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


        mProject->mModel->mLogAdapt += "<hr>";
        mProject->mModel->mLogAdapt += line(textBold(tr("ADAPTATION FOR CHAIN %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mLoopChains.size()))) );

        if (chain.mBatchIndex < chain.mMaxBatchs) {
            mProject->mModel->mLogAdapt += line("Adapt OK at batch : " + QString::number(chain.mBatchIndex) + "/" + QString::number(chain.mMaxBatchs));

        } else {
            mProject->mModel->mLogAdapt += line(textRed("Warning : Not adapted after " + QString::number(chain.mBatchIndex) + " batches"));
        }

        mProject->mModel->mLogAdapt += ModelUtilities::modelStateDescriptionHTML(static_cast<ModelCurve*>(mProject->mModel) );
        mProject->mModel->mLogAdapt += "<hr>";

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

                mProject->mModel->memo_accept(mChainIndex);

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
        mProject->mModel->mLogResults += line(tr("Acquisition time elapsed %1").arg(DHMS(chain.mAcquisitionElapsedTime)));

        // rétablissement de l'ordre des Events, indispensable en cas de calcul de courbe. Car le update modifie l'ordre des events et utile pour la sauvegarde de ChronoModel_Bash
        std::copy(initListEvents.begin(), initListEvents.end(), mProject->mModel->mEvents.begin() );
    }

    mProject->mModel->mChains = mLoopChains;

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

    qDebug()<<"Model computed";
    qDebug()<<tr("finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug()<<tr("Total time elapsed %1").arg(QString(DHMS(startTime.elapsed())));
#endif


}
