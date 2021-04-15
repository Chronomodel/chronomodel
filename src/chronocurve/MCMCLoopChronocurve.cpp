/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "MCMCLoopChronocurve.h"
#include "ModelChronocurve.h"

#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "Date.h"
#include "Project.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "../PluginAbstract.h"
#include "CalibrationCurve.h"

#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QTime>

#include <errno.h>      /* errno, EDOM */
#include <fenv.h>
#include <exception>

MCMCLoopChronocurve::MCMCLoopChronocurve(ModelChronocurve* model, Project* project):MCMCLoop(),
mModel(model)
{
    mProject = project;
    if (mModel){
        setMCMCSettings(mModel->mMCMCSettings);
    }
    
    QJsonObject state = project->mState;
    mChronocurveSettings = ChronocurveSettings::fromJson(state.value(STATE_CHRONOCURVE).toObject());
}

MCMCLoopChronocurve::~MCMCLoopChronocurve()
{
    mModel = nullptr;
    mProject = nullptr;
}

#pragma mark MCMC Loop Overloads

/**
 * Idem Chronomodel + prepareEventsY() qui sert à corriger les données d'entrées de Chronocurve.
 * (Calcul de Yx, Yy, Yz et de Sy)
 */
QString MCMCLoopChronocurve::calibrate()
{
    if (mModel) {
        QList<Event*>& events = mModel->mEvents;
        events.reserve(mModel->mEvents.size());
        
        //----------------- Calibrate measurements --------------------------------------

        QList<Date*> dates;
        // find number of dates, to optimize memory space
        int nbDates = 0;
        for (auto&& ev : events)
            nbDates += ev->mDates.size();

        dates.reserve(nbDates);
        for (auto&& ev : events) {
            int num_dates = ev->mDates.size();
            for (int j = 0; j<num_dates; ++j) {
                Date* date = &ev->mDates[j];
                dates.push_back(date);
            }
        }
        

        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepChanged(tr("Calibrating..."), 0, dates.size());

        int i = 0;
        for (auto&& date : dates) {
              if (date->mCalibration) {
                if (date->mCalibration->mCurve.isEmpty())
                    date->calibrate(mModel->mSettings, mProject);
                } else
                    return (tr("Invalid Model -> No Calibration on Data %1").arg(date->mName));


            if (isInterruptionRequested())
                return ABORTED_BY_USER;

            emit stepProgressed(i);
            ++i;

        }
        dates.clear();
        
        prepareEventsY(mModel->mEvents);
        
        return QString();
    }
    return tr("Invalid model");
}

/**
 * Idem Chronomodel + initialisation des variables aléatoires VG (events) et Alpha Lissage (global)
 * TODO : initialisation des résultats g(t), g'(t), g"(t)
 */
void MCMCLoopChronocurve::initVariablesForChain()
{
    // today we have the same acceptBufferLen for every chain
    const int acceptBufferLen =  mChains.at(0).mNumBatchIter;
    int initReserve (0);
    
    for (auto& c: mChains) {
       initReserve += ( 1 + (c.mMaxBatchs*c.mNumBatchIter) + c.mNumBurnIter + (c.mNumRunIter/c.mThinningInterval) );
    }
    
    for (Event*& event : mModel->mEvents) {
        
        event->mTheta.reset();
        event->mTheta.reserve(initReserve);
        event->mTheta.mLastAccepts.reserve(acceptBufferLen);
        event->mTheta.mLastAcceptsLength = acceptBufferLen;
        
        event->mVG.reset();
        event->mVG.reserve(initReserve);
        event->mVG.mLastAccepts.reserve(acceptBufferLen);
        event->mVG.mLastAcceptsLength = acceptBufferLen;

        // event->mTheta.mAllAccepts.clear(); //don't clean, avalable for cumulate chain

        for (Date& date : event->mDates) {
            date.mTheta.reset();
            date.mTheta.reserve(initReserve);
            date.mTheta.mLastAccepts.reserve(acceptBufferLen);
            date.mTheta.mLastAcceptsLength = acceptBufferLen;

            date.mSigma.reset();
            date.mSigma.reserve(initReserve);
            date.mSigma.mLastAccepts.reserve(acceptBufferLen);
            date.mSigma.mLastAcceptsLength = acceptBufferLen;

            date.mWiggle.reset();
            date.mWiggle.reserve(initReserve);
            date.mWiggle.mLastAccepts.reserve(acceptBufferLen);
            date.mWiggle.mLastAcceptsLength = acceptBufferLen;
        }
    }
    
    mModel->mAlphaLissage.reset();
    mModel->mAlphaLissage.reserve(initReserve);
    mModel->mAlphaLissage.mLastAccepts.reserve(acceptBufferLen);
    mModel->mAlphaLissage.mLastAcceptsLength = acceptBufferLen;
    
    // Ré-initialisation du stockage des splines
    mModel->mMCMCSplines.clear();

    // Ré-initialisation des résultats
    mModel->mPosteriorMeanGByChain.clear();
    mModel->mPosteriorMeanG.gx.vecG.clear();
    mModel->mPosteriorMeanG.gy.vecG.clear();
    mModel->mPosteriorMeanG.gz.vecG.clear();

    mModel->mPosteriorMeanG.gx.vecGErr.clear();
    mModel->mPosteriorMeanG.gy.vecGErr.clear();
    mModel->mPosteriorMeanG.gz.vecGErr.clear();

    mModel->mPosteriorMeanG.gx.vecGP.clear();
    mModel->mPosteriorMeanG.gy.vecGP.clear();
    mModel->mPosteriorMeanG.gz.vecGP.clear();

    mModel->mPosteriorMeanG.gx.vecGS.clear();
    mModel->mPosteriorMeanG.gy.vecGS.clear();
    mModel->mPosteriorMeanG.gz.vecGS.clear();
}

/**
 * Idem Chronomodel + initialisation de VG (events) et Alpha Lissage (global)
 */
QString MCMCLoopChronocurve::initMCMC()
{
    QList<Event*>& events(mModel->mEvents);
    QList<Phase*>& phases (mModel->mPhases);
    QList<PhaseConstraint*>& phasesConstraints (mModel->mPhaseConstraints);

    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;

    if (isInterruptionRequested())
        return ABORTED_BY_USER;
// initialisation des bornes
    // ---------------------- Reset Events ---------------------------
    for (Event* ev : events) {
        ev->mInitialized = false;
    }
    // -------------------------- Init gamma ------------------------------
    emit stepChanged(tr("Initializing Phase Gaps..."), 0, phasesConstraints.size());
    int i = 0;
    for (auto&& phC : phasesConstraints) {
        phC->initGamma();
        if (isInterruptionRequested())
            return ABORTED_BY_USER;
        ++i;
        emit stepProgressed(i);
    }

    // ----------------------- Init tau -----------------------------------------
    emit stepChanged(tr("Initializing Phase Durations..."), 0, phases.size());
    i = 0;
    for (auto&& ph : phases) {
        ph->initTau();

        if (isInterruptionRequested())
            return ABORTED_BY_USER;
        ++i;
        emit stepProgressed(i);
    }
    /* -------------- Init Bounds --------------
    * - Définir des niveaux pour les faits
    * - Initialiser les bornes (uniquement, pas les faits) par niveaux croissants
    * => Init borne :
    *  - si valeur fixe, facile!
    *  - si intervalle : random uniform sur l'intervalle (vérifier si min < max pour l'intervalle qui a été modifié par la validation du modèle)
    * ---------------------------------------------------------------- */
    QVector<Event*> eventsByLevel = ModelUtilities::sortEventsByLevel(mModel->mEvents);
    int curLevel (0);
    double curLevelMaxValue = mModel->mSettings.mTmin;

    for (int i = 0; i < eventsByLevel.size(); ++i) {
        if (eventsByLevel.at(i)->mType == Event::eKnown) {
            EventKnown* bound = dynamic_cast<EventKnown*>(eventsByLevel[i]);

            if (bound) {
                if (curLevel != bound->mLevel) {
                    curLevel = bound->mLevel;
                    curLevelMaxValue = mModel->mSettings.mTmin;
                }

                bound->mTheta.mX = bound->mFixed;
                bound->mThetaReduced = reduceTime(bound->mTheta.mX);

                curLevelMaxValue = qMax(curLevelMaxValue, bound->mTheta.mX);

                bound->mTheta.memo();
                bound->mTheta.mLastAccepts.clear();
                bound->mTheta.mLastAccepts.push_back(1.);
                bound->mTheta.saveCurrentAcceptRate();
                bound->mInitialized = true;
                prepareEventY(bound);
            }
            bound = nullptr;
        }
    }

    // ----------------------------------------------------------------
    //  Init theta event, ti, ...
    // ----------------------------------------------------------------
    QVector<Event*> unsortedEvents = ModelUtilities::unsortEvents(events);
    
    emit stepChanged(tr("Initializing Events..."), 0, unsortedEvents.size());
    
    for (auto&& e : unsortedEvents) {
        mModel->initNodeEvents();
        QString circularEventName = "";
        QList<Event*> startEvents = QList<Event*>();

        const bool ok (e->getThetaMaxPossible (e, circularEventName, startEvents));
        if (!ok) {
            mAbortedReason = QString(tr("Warning : Find Circular Constraint Path %1  %2 ")).arg (e->mName, circularEventName);
            return mAbortedReason;
        }
    }

    for (int i = 0; i<unsortedEvents.size(); ++i) {
        if (unsortedEvents.at(i)->mType == Event::eDefault) {

            mModel->initNodeEvents(); // ?? FAIT AU-DESSUS ?
            QString circularEventName = "";
          //  QList< Event*> startEvents = QList<Event*>();
            const double min (unsortedEvents.at(i)->getThetaMinRecursive (tmin));
            
            // ?? Comment circularEventName peut-il être pas vide ?
            if (!circularEventName.isEmpty()) {
                mAbortedReason = QString(tr("Warning : Find Circular constraint with %1  bad path  %2 ")).arg(unsortedEvents.at(i)->mName, circularEventName);
                return mAbortedReason;
            }
            
            mModel->initNodeEvents();
            const double max ( unsortedEvents.at(i)->getThetaMaxRecursive(tmax) );
#ifdef DEBUG
            if (min >= max)
                qDebug() << tr("-----Error Init for event : %1 : min = %2 : max = %3-------").arg(unsortedEvents.at(i)->mName, QString::number(min), QString::number(max));
#endif
            
            // ----------------------------------------------------------------
            // Chronocurve init Theta event :
            // On initialise les theta près des dates ti
            // ----------------------------------------------------------------
            if (true) {//mChronocurveSettings.mTimeType == ChronocurveSettings::eModeFixed)
       /*     {
                // Dans le cas theta fixe (pas de Bayésien),
                // On initialise les theta event à la valeur médiane de la calibration de leur première date.
                // La valeur médiane est retrouvée grâce à Q2
                
                const Date& firstDate = unsortedEvents.at(i)->mDates[0];
                
                Quartiles quartiles = quartilesForRepartition(
                    firstDate.mCalibration->mRepartition,
                    firstDate.mCalibration->mTmin,
                    firstDate.mCalibration->mStep
                );
                
                unsortedEvents.at(i)->mTheta.mX = quartiles.Q2;

            }*/
                sampleInCumulatedRepartition(unsortedEvents.at(i), mModel->mSettings,min, max);

               } else {
                // Idem Chronomodel, mais jamais utilisé ici :
                unsortedEvents.at(i)->mTheta.mX = Generator::randomUniform(min, max);
            }
            unsortedEvents.at(i)->mThetaReduced = reduceTime(unsortedEvents.at(i)->mTheta.mX);
            unsortedEvents.at(i)->mInitialized = true;

            // ----------------------------------------------------------------

            double s02_sum (0.);
            for (int j = 0; j < unsortedEvents.at(i)->mDates.size(); ++j) {
                Date& date = unsortedEvents.at(i)->mDates[j];

                // 1 - Init ti
                double sigma;
                if (!date.mCalibration->mRepartition.isEmpty()) {
                    const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mCalibration->mRepartition);
                    date.mTheta.mX = date.mCalibration->mTmin + idx * date.mCalibration->mStep;

                    FunctionAnalysis data = analyseFunction(vector_to_map(date.mCalibration->mCurve, tmin, tmax, date.mCalibration->mStep));
                    sigma = double (data.stddev);

                } else { // in the case of mRepartion curve is null, we must init ti outside the study period
                       // For instance we use a gaussian random sampling
                    sigma = mModel->mSettings.mTmax - mModel->mSettings.mTmin;
                    const double u = Generator::gaussByBoxMuller(0., sigma);
                    if (u<0)
                        date.mTheta.mX = mModel->mSettings.mTmin + u;
                    else
                        date.mTheta.mX = mModel->mSettings.mTmax + u;

                    if (date.mMethod == Date::eInversion) {
                        qDebug()<<"Automatic sampling method exchange eInversion to eMHSymetric for"<< date.mName;
                        date.mMethod = Date::eMHSymetric;
                        date.autoSetTiSampler(true);
                    }

                }
                
                // 2 - Init Delta Wiggle matching and Clear mLastAccepts array
                date.initDelta(unsortedEvents.at(i));
                date.mWiggle.memo();
                date.mWiggle.mLastAccepts.push_back(true);
                date.mWiggle.saveCurrentAcceptRate();

                // 3 - Init sigma MH adaptatif of each Data ti
                date.mTheta.mSigmaMH = sigma;

                // 4 - Clear mLastAccepts array and set this init at 100%
                date.mTheta.mLastAccepts.clear();
                date.mTheta.mLastAccepts.push_back(true);

                // 5 - Memo
                date.mTheta.memo();

                date.mTheta.saveCurrentAcceptRate();

                // intermediary calculus for the harmonic average
                s02_sum += 1. / (sigma * sigma);
            }

            // 4 - Init S02 of each Event
            unsortedEvents.at(i)->mS02 = unsortedEvents.at(i)->mDates.size() / s02_sum;// /100;

            // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
            unsortedEvents.at(i)->mTheta.mSigmaMH = sqrt(unsortedEvents.at(i)->mS02);
            unsortedEvents.at(i)->mAShrinkage = 1.;
            
            // 6- Clear mLastAccepts array
            unsortedEvents.at(i)->mTheta.mLastAccepts.clear();
            unsortedEvents.at(i)->mTheta.mLastAccepts.push_back(true);
            
            // 7 - Memo
            unsortedEvents.at(i)->mTheta.memo();
            unsortedEvents.at(i)->mTheta.saveCurrentAcceptRate();
        }
        // ----------------------------------------------------------------
        // Chronocurve init VG :
        // ----------------------------------------------------------------
        Event* event = unsortedEvents.at(i);
        if (mChronocurveSettings.mVarianceType == ChronocurveSettings::eModeFixed) {
            event->mVG.mX = mChronocurveSettings.mVarianceFixed;

        } else {
            event->mVG.mX = event->mSy;// 1e+0; //1e20;
        }

        // On stocke la racine de VG, qui est une variance pour afficher l'écart-type
        double memoVG = sqrt(event->mVG.mX);
        event->mVG.memo(&memoVG);

        event->mVG.mSigmaMH = 1.;
        event->mVG.mLastAccepts.clear();
        event->mVG.mLastAccepts.push_back(true);

        event->mVG.saveCurrentAcceptRate();

        // ----------------------------------------------------------------
        //  Les W des events ne dépendant que de leur VG
        //  Lors de l'update, on a besoin de W pour les calculs de mise à jour de theta, VG et Alpha lissage
        //  On sera donc amenés à remettre le W à jour à chaque modification de VG
        //  On le calcul ici lors de l'initialisation pour avoir sa valeur de départ
        // ----------------------------------------------------------------
        event->updateW();


        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepProgressed(i);
    }

    // ----------------------------------------------------------------
    //  Init sigma i and its sigma MH
    // ----------------------------------------------------------------
    QString log;
    emit stepChanged(tr("Initializing Variances..."), 0, events.size());

    for (auto& ev : events) {
        for (auto & date : ev->mDates) {
            // date.mSigma.mX = sqrt(shrinkageUniform(events[i]->mS02)); // modif the 2015/05/19 with PhL
            date.mSigma.mX = std::abs(date.mTheta.mX - (ev->mTheta.mX - date.mDelta));

            if (date.mSigma.mX <= 1E-6) {
               date.mSigma.mX = 1E-6; // Add control the 2015/06/15 with PhL
               log += line(date.mName + textBold("Sigma indiv. <=1E-6 set to 1E-6"));
            }
            date.mSigma.mSigmaMH = 1.;

            date.mSigma.mLastAccepts.clear();
            date.mSigma.mLastAccepts.push_back(true);

            date.mSigma.memo();
            date.mSigma.saveCurrentAcceptRate();
        }
        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Init alpha lissage
    // ----------------------------------------------------------------
    if (mChronocurveSettings.mCoeffLissageType == ChronocurveSettings::eModeFixed){
        mModel->mAlphaLissage.mX = mChronocurveSettings.mAlphaLissage;

    } else {
        mModel->mAlphaLissage.mX = 1e-60;//1e-6; in the middle of the log interval
    }
    
    mModel->mAlphaLissage.mSigmaMH = 1.;
    mModel->mAlphaLissage.mLastAccepts.clear();
    mModel->mAlphaLissage.mLastAccepts.push_back(true);

    // On stocke le log10 de alpha pour afficher les résultats a posteriori
    double memoAlpha = log10(mModel->mAlphaLissage.mX);
    mModel->mAlphaLissage.memo(&memoAlpha);

    mModel->mAlphaLissage.saveCurrentAcceptRate();

    // --------------------------- memo spline ----------------------
    
    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------

    MCMCSpline spline;

    // prepareCalculSpline : ne fait pas intervenir les valeurs Y(x,y,z) des events :mais utilise les theta réduits
    // => On le fait une seule fois pour les 3 composantes

    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);
    std::vector<long double> vecH = calculVecH(mModel->mEvents);

    SplineMatrices matrices = prepareCalculSpline(mModel->mEvents, vecH);

    // calculSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    std::vector<std::vector<long double>> matB;// = matrices.matR; //matR;
    const double alpha = mModel->mAlphaLissage.mX;
    if (alpha != 0) {
        std::vector<std::vector<long double>> tmp = multiConstParMat(matrices.matQTW_1Q, alpha, 5);
        matB = addMatEtMat(matrices.matR, tmp, 5);
    } else {
        matB = matrices.matR;
    }

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matB, 5, 1);
    std::vector<long double> vecTheta = getThetaEventVector(mModel->mEvents);
    std::vector<long double> vecErrG = calculSplineError(matrices, decomp, alpha);

    // Tout le calcul précédent ne change pas

    // std::vector<long double> vecH = calculVecH(mModel->mEvents);
    SplineResults s = calculSplineX(matrices, vecH, decomp, matB, alpha);

    MCMCSplineComposante splineX;

    splineX.vecG = std::move(s.vecG);
    splineX.vecGamma = std::move(s.vecGamma);

    splineX.vecThetaEvents = vecTheta; //getThetaEventVector(mModel->mEvents);
    splineX.vecErrG = vecErrG; //calculSplineError(matrices, decomp);

    spline.splineX = std::move(splineX);

    if ( mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique ||
         mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel||
         mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D ) {
        // calculSpline utilise les Y des events
        // => On le calcule ici pour la seconde composante (y)

        s = calculSplineY(matrices, vecH, decomp, matB, alpha);

        MCMCSplineComposante splineY;
        splineY.vecG = std::move(s.vecG);
        splineY.vecGamma = std::move(s.vecGamma);

        splineY.vecErrG = vecErrG; //calculSplineError(matrices, decomp);
        splineY.vecThetaEvents = vecTheta; //getThetaEventVector(mModel->mEvents);

        spline.splineY = std::move(splineY);
    }

    if ( mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique ||
         mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel ||
         mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D) {
        // Dans le future, ne sera pas utile pour le mode spherique
        // => On le calcule ici pour la troisième composante (z)

        s = calculSplineZ(matrices, vecH, decomp, matB, alpha);

        MCMCSplineComposante splineZ;

        splineZ.vecG = std::move(s.vecG);
        splineZ.vecGamma = std::move(s.vecGamma);

        splineZ.vecThetaEvents = vecTheta; //getThetaEventVector(mModel->mEvents);
        splineZ.vecErrG = vecErrG; //calculSplineError(matrices, decomp);

        spline.splineZ = std::move(splineZ);
    }

    mModel->mMCMCSplines.push_back(spline);



    // --------------------------- Init phases ----------------------
    emit stepChanged(tr("Initializing Phases..."), 0, phases.size());

    i = 0;
    for (auto&& phase : phases ) {
        phase->updateAll(tmin, tmax);
        phase->memoAll();

        if (isInterruptionRequested())
            return ABORTED_BY_USER;
        ++i;

        emit stepProgressed(i);
    }

    // --------------------------- Log Init ---------------------
    log += "<hr>";
    log += textBold("Events Initialization (with their data)");

    i = 0;
    for (auto& event : events) {
        ++i;
        log += "<hr><br>";

        if (event->type() == Event::eKnown) {
             const EventKnown* bound = dynamic_cast<const EventKnown*>(event);
            if (bound) {
                log += line(textRed(tr("Bound ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(events.size()), bound->mName)));
                log += line(textRed(tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(bound->mTheta.mX), DateUtils::getAppSettingsFormatStr())));
                log += line(textRed(tr(" - Sigma_MH on Theta : %1").arg(stringForLocal(bound->mTheta.mSigmaMH))));
            }
        }
        else {
            log += line(textBlue(tr("Event ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(events.size()), event->mName)));
            log += line(textBlue(tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(event->mTheta.mX), DateUtils::getAppSettingsFormatStr())));
            log += line(textBlue(tr(" - Sigma_MH on Theta : %1").arg(stringForLocal(event->mTheta.mSigmaMH))));
            log += line(textBlue(tr(" - S02 : %1").arg(stringForLocal(event->mS02))));
        }
        int j = 0;
        for (auto& date : event->mDates) {
            ++j;
            log += "<br>";
            
            log += line(textBlack(tr("Data ( %1 / %2 ) : %3").arg(QString::number(j), QString::number(event->mDates.size()), date.mName)));
            log += line(textBlack(tr(" - ti : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(date.mTheta.mX), DateUtils::getAppSettingsFormatStr())));
            if (date.mMethod == Date::eMHSymGaussAdapt)
                log += line(textBlack(tr(" - Sigma_MH on ti : %1").arg(stringForLocal(date.mTheta.mSigmaMH))));

            log += line(textBlack(tr(" - Sigma_i : %1").arg(stringForLocal(date.mSigma.mX))));
            log += line(textBlack(tr(" - Sigma_MH on Sigma_i : %1").arg(stringForLocal(date.mSigma.mSigmaMH))));
            if (date.mDeltaType != Date::eDeltaNone)
                log += line(textBlack(tr(" - Delta_i : %1").arg(stringForLocal(date.mDelta))));

        }
    }

    if (phases.size() > 0) {
        log += "<hr>";
        log += textBold(tr("Phases Initialization"));
        log += "<hr>";

        int i = 0;
        for (auto& phase : phases) {
            ++i;
            log += "<br>";
            log += line(textPurple(tr("Phase ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(phases.size()), phase->mName)));
            log += line(textPurple(tr(" - Begin : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(phase->mAlpha.mX), DateUtils::getAppSettingsFormatStr())));
            log += line(textPurple(tr(" - End : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(phase->mBeta.mX), DateUtils::getAppSettingsFormatStr())));
            log += line(textPurple(tr(" - Tau : %1").arg(stringForLocal(phase->mTau))));
        }
    }

    if (phasesConstraints.size() > 0) {
        log += "<hr>";
        log += textBold(textGreen(tr("Phases Constraints Initialization"))) ;
        log += "<hr>";

        int i = 0;
        for (auto& constraint : phasesConstraints) {
            ++i;
            log += "<br>";
            log += line(textGreen(tr("Succession ( %1 / %2) : from %3 to %4").arg(QString::number(i), QString::number(phasesConstraints.size()),constraint->mPhaseFrom->mName, constraint->mPhaseTo->mName)));
            log += line(textGreen(tr(" - Gamma : %1").arg(stringForLocal(constraint->mGamma))));
        }
    }

    mInitLog += "<hr>";
    mInitLog += textBold(tr("INIT CHAIN %1").arg(QString::number(mChainIndex+1)));
    mInitLog += "<hr>";
    mInitLog += log;

    return QString();
}

/**
 * Idem Chronomodel pour les Dates
 * Pour les events, theta a une nouvelle composante et est donc entièrement revu
 * Les events doivent aussi mettre VG à jour (sur le même principe que theta)
 * Idem pour le alpha lissage global
 * Ces 3 mises à jour font intervenir le calcul matriciel (cravate, spline, etc...)
 */
void MCMCLoopChronocurve::update()
{
 try {
    const double t_max (mModel->mSettings.mTmax);
    const double t_min (mModel->mSettings.mTmin);

    ChainSpecs& chain = mChains[mChainIndex];
    const bool doMemo = (mState == eBurning) || (mState == eAdapting) || (chain.mTotalIter % chain.mThinningInterval == 0);

    // --------------------------------------------------------------
    //  Update Dates (idem chronomodel)
    // --------------------------------------------------------------
    try {


    for (Event*& event : mModel->mEvents) {
        for (auto&& date : event->mDates) {
            date.updateDelta(event);
            date.updateTheta(event);
            //date.updateSigma(event);
            //date.updateSigmaJeffreys(event);
            date.updateSigmaShrinkage(event);
            //date.updateSigmaReParam(event);
            date.updateWiggle();

            if (doMemo) {
                date.mTheta.memo();
                date.mSigma.memo();
                date.mWiggle.memo();

                date.mTheta.saveCurrentAcceptRate();
                date.mSigma.saveCurrentAcceptRate();
            }
        }
    }
    
    }  catch (...) {
        qWarning() <<"update Date ???";
    }
    // --------------------------------------------------------------
    //  Update theta Events
    // --------------------------------------------------------------


    // copie la liste des pointeurs
    std::vector<Event*> initListEvents (mModel->mEvents.size());
    std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    // find minimal step;
   // long double minStep = minimalThetaReducedDifference(mModel->mEvents)/10.;

    if (mChronocurveSettings.mTimeType == ChronocurveSettings::eModeBayesian) {
        try {
            orderEventsByThetaReduced(mModel->mEvents);

            spreadEventsThetaReduced0(mModel->mEvents);
            std::vector<long double> vecH_current = calculVecH(mModel->mEvents);
            SplineMatrices matrices = prepareCalculSpline(mModel->mEvents, vecH_current);

            // pHd : pour h_theta mTheta doit être en année, et h_YWI_AY utilise mThetaReduced
             //long double h_current= h_YWI_AY(matrices, mModel->mEvents, mModel->mAlphaLissage.mX) * h_alpha(matrices, mModel->mEvents.size(), mModel->mAlphaLissage.mX) * h_theta(mModel->mEvents);

            long double h_YWI = h_YWI_AY(matrices, mModel->mEvents, mModel->mAlphaLissage.mX, vecH_current);
            long double h_al = h_alpha(matrices, mModel->mEvents.size(), mModel->mAlphaLissage.mX);
            long double h_thet = h_theta(mModel->mEvents);

            long double h_current = h_YWI * h_al * h_thet;

            long double h_new;
            std::vector<long double> vecH_new;

            //  restoreEventsTheta(mModel->mEvents);

            /* ----------------------------------------------------------------------
             *  Dans Chronomodel, on appelle simplement : event->updateTheta(t_min,t_max); sur tous les events.
             *  Pour mettre à jour un theta d'event dans Chronocurve, on doit accéder aux thetas des autres events.
             *  => on effectue donc la mise à jour directement ici, sans passer par une fonction
             *  de la classe event (qui n'a pas accès aux autres events)
             * ---------------------------------------------------------------------- */
            // qDebug()<<"mChronocurveSettings.mTimeType == ChronocurveSettings::eModeBayesian ici memo = "<<doMemo<<chain.mBatchIterIndex<<chain.mBatchIndex;

            for (Event*& event : initListEvents) {
                if (event->mType == Event::eDefault) {
                    const double min = event->getThetaMin(t_min);
                    const double max = event->getThetaMax(t_max);

                    if (min >= max) {
                        throw QObject::tr("Error for event theta : %1 : min = %2 : max = %3").arg(event->mName, QString::number(min), QString::number(max));
                    }

                    // On stocke l'ancienne valeur :
                    const double theta_current = event->mTheta.mX;

                    // On tire une nouvelle valeur :
                    const double theta_new = Generator::gaussByBoxMuller(theta_current, event->mTheta.mSigmaMH);

                    long double rapport = 0.;


                    if (theta_new >= min && theta_new <= max) {
                        // On force la mise à jour de la nouvelle valeur pour calculer h_new

                        event->mTheta.mX = theta_new; // pHd : Utile pour h_theta
                        event->mThetaReduced = reduceTime(theta_new);

                        orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                        //spreadEventsThetaReduced(mModel->mEvents, minStep); // On espace les temps si il y a égalité de date
                        spreadEventsThetaReduced0(mModel->mEvents);

                        vecH_new = calculVecH(mModel->mEvents);
                        matrices = prepareCalculSpline(mModel->mEvents, vecH_new);

                        h_YWI = h_YWI_AY(matrices, mModel->mEvents, mModel->mAlphaLissage.mX, vecH_new);
                        h_al = h_alpha(matrices, mModel->mEvents.size(), mModel->mAlphaLissage.mX);
                        h_thet = h_theta(mModel->mEvents);

                        h_new = h_YWI * h_al * h_thet;

                    /*    if (h_new == 0.) {
                            qWarning("update() h_new==0");
                        }
*/
                        // h_new = h_YWI_AY(matrices, mModel->mEvents, mModel->mAlphaLissage.mX) * h_alpha(matrices, mModel->mEvents.size(), mModel->mAlphaLissage.mX) * h_theta(mModel->mEvents);
                        //  restoreEventsTheta(mModel->mEvents); // On supprime les décalages introduits par spreadEventsTheta pour les calculs de h_new

                        // Calcul du rapport :
                        rapport = (h_current == 0.) ? 1. : (h_new / h_current);

                    }
                    // restore Theta
                    event->mTheta.mX = theta_current;
                    event->mTheta.tryUpdate(theta_new, rapport);

                    if ( event->mTheta.mLastAccepts.last() == true) {
                        // Pour l'itération suivante :
                          h_current = h_new;
                    }


                } else { // this is a bound, nothing to sample. Always the same value
                    event->updateTheta(t_min, t_max);
                    //event->mTheta.tryUpdate(event->mTheta.mX, 1.); // always accepted
                }
                // update after tryUpdate or updateTheta
                event->mThetaReduced = reduceTime(event->mTheta.mX);

                if (doMemo) {
                    event->mTheta.memo();
                    event->mTheta.saveCurrentAcceptRate();
                }

                //--------------------- Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
                for (auto&& phInEv : event->mPhases)
                    phInEv->updateAll(t_min, t_max);
            }


            // Rétablissement de l'ordre initial. Est-ce nécessaire ?
            std::copy(initListEvents.begin(), initListEvents.end(), mModel->mEvents.begin() );

        } catch(...) {
            qDebug() << "MCMCLoopChronocurve::update mChronocurveSettings.mTimeType == ChronocurveSettings::eModeBayesian : Caught Exception!\n";
        }

    } else { // Pas bayésien : on sauvegarde la valeur constante dans la trace
        for (Event*& event : mModel->mEvents) {
            event->mTheta.tryUpdate(event->mTheta.mX, 1);
            if (doMemo) {
                event->mTheta.memo();
                event->mTheta.saveCurrentAcceptRate();
            }
        }
    }

    //--------------------- Memo Phases -----------------------------------------
    if (doMemo) {
        for (auto&& ph : mModel->mPhases)
            ph->memoAll();
    }

    //--------------------- Update Phases constraints -----------------------------------------
    for (auto&& phConst : mModel->mPhaseConstraints )
        phConst->updateGamma();


    // --------------------------------------------------------------
    //  Remarque : à ce stade, tous les theta events sont à jour et ordonnés.
    //  On va à présent mettre à jour tous les VG, puis Alpha lissage.
    //  Pour cela, nous devons espacer les thetas pour permettre les calculs.
    //  Nous le faisons donc ici, et restaurerons les vrais thetas à la fin.
    // --------------------------------------------------------------
    std::vector<long double> vecH;
    try {
        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents); // don't modify theta->mX
        vecH = calculVecH(mModel->mEvents);

       // orderEventsByThetaReduced(mModel->mEvents);
        //  saveEventsTheta(mModel->mEvents);
        //  reduceEventsTheta(mModel->mEvents); // On passe en temps réduit entre 0 et 1
        // find minimal step;
        // long double minStep = minimalThetaReducedDifference(mModel->mEvents);
        //spreadEventsThetaReduced(mModel->mEvents, minStep/10.);
       // spreadEventsThetaReduced0(mModel->mEvents);


    } catch(...) {
        qDebug() << "MCMCLoopChronocurve::update order Event : Caught Exception!\n";
    }
    // --------------------------------------------------------------
    //  Update VG Events
    // --------------------------------------------------------------
    if (mChronocurveSettings.mVarianceType == ChronocurveSettings::eModeBayesian) {
        try {

            long double current_h, current_value, try_h, try_value_log, try_value, rapport;
            long double h_YWI, h_al, h_VGij;

          /*  orderEventsByThetaReduced(mModel->mEvents);
            spreadEventsThetaReduced0(mModel->mEvents);
            std::vector<long double> vecH = calculVecH(mModel->mEvents);
*/
            SplineMatrices current_matrices = prepareCalculSpline(mModel->mEvents, vecH);
            h_YWI = h_YWI_AY(current_matrices, mModel->mEvents, mModel->mAlphaLissage.mX, vecH);

            if (mChronocurveSettings.mCoeffLissageType == ChronocurveSettings::eModeBayesian)
                 h_al = h_alpha(current_matrices, mModel->mEvents.size(), mModel->mAlphaLissage.mX);
            else
               h_al = 1.;

            h_VGij = h_VG(mModel->mEvents);

            current_h = h_YWI * h_al * h_VGij;


            const double logMin = -100.; //log10(1e-10);
            const double logMax = +30.; //log10(1e+20);

            if (!mChronocurveSettings.mUseVarianceIndividual) {
                // On stocke l'ancienne valeur :
                const double current_value = mModel->mEvents.at(0)->mVG.mX;

                // On tire une nouvelle valeur :

                try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mEvents.at(0)->mVG.mSigmaMH);
                try_value = pow(10, try_value_log);

                // affectation temporaire pour evaluer la nouvelle proba
                for (Event*& ev : mModel->mEvents) {
                    ev->mVG.mX = try_value;
                    ev->updateW();
                }

                double rapport = 0.;
                if (try_value_log >= logMin && try_value_log <= logMax) {

                    // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG
                    SplineMatrices try_matrices = prepareCalculSpline(mModel->mEvents, vecH);

                    h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mAlphaLissage.mX, vecH);

                    if (mChronocurveSettings.mCoeffLissageType == ChronocurveSettings::eModeBayesian)
                        h_al = h_alpha(try_matrices, mModel->mEvents.size(), mModel->mAlphaLissage.mX);
                    else
                        h_al = 1.;

                   // h_al = h_alpha(try_matrices, mModel->mEvents.size(), mModel->mAlphaLissage.mX);
                    h_VGij = h_VG (mModel->mEvents);

                    try_h = h_YWI * h_al * h_VGij;

                    rapport = (current_h == 0) ? 1 : ((try_h * try_value) / (current_h * current_value));

                    // On remet l'ancienne valeur, qui sera éventuellement mise à jour dans ce qui suit (Metropolis Hastings)
                    // A chaque fois qu'on modifie VG, W change !
                    //event->mVG.mX = value_current;
                    //event->updateW();

                    // Pour l'itération suivante :
                    //h_current = h_new;

                } else {
                    rapport = 0.;
                }

                // ON fait le test avec le premier event
                mModel->mEvents.at(0)->mVG.mX = current_value;
                mModel->mEvents.at(0)->mVG.tryUpdate(try_value, rapport);


                if ( mModel->mEvents.at(0)->mVG.mLastAccepts.last() == true) {
                    // la nouvelle valeur est acceptée
                    for (Event*& ev : mModel->mEvents) {
                        ev->mVG.mX = try_value;
                        ev->updateW();
                    }


                }

                if (doMemo) {
                     for (Event*& ev : mModel->mEvents) {
                         // On stocke la racine de VG, qui est une variance pour afficher l'écart-type
                         double memoVG = sqrt(ev->mVG.mX);
                         ev->mVG.memo(&memoVG);

                         ev->mVG.saveCurrentAcceptRate();
                     }

                }




            } else {
                // Variance individuelle

                 Event* event;
                //for (Event* event : mModel->mEvents)   {
                for (int i = 0; i < mModel->mEvents.size(); ++i) {
                    event = mModel->mEvents.at(i);

                    current_value = event->mVG.mX;

                    // On tire une nouvelle valeur :

                    try_value_log = Generator::gaussByBoxMuller(log10(current_value), event->mVG.mSigmaMH);
                    try_value = pow(10., try_value_log);


                    if (try_value_log >= logMin && try_value_log <= logMax) {
                        // On force la mise à jour de la nouvelle valeur pour calculer h_new
                        // A chaque fois qu'on modifie VG, W change !
                        event->mVG.mX = try_value;
                        event->updateW(); // used by prepareCalculSpline

                        // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG
                        SplineMatrices try_matrices = prepareCalculSpline(mModel->mEvents, vecH);

                        h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mAlphaLissage.mX, vecH);

                        if (mChronocurveSettings.mCoeffLissageType == ChronocurveSettings::eModeBayesian)
                            h_al = h_alpha(try_matrices, mModel->mEvents.size(), mModel->mAlphaLissage.mX);
                        else
                            h_al = 1.;

                        h_VGij = h_VG (mModel->mEvents);

                        if (h_YWI == HUGE_VAL || h_al== HUGE_VAL || h_VGij==HUGE_VAL)
                            try_h = 1.;
                        else
                            try_h = h_YWI * h_al * h_VGij;

                        rapport = (current_h == 0) ? 1 : ((try_h * try_value) / (current_h * current_value));

                    } else {
                        rapport = 0.;
                    }

                    // Mise à jour Metropolis Hastings
                    // A chaque fois qu'on modifie VG, W change !
                    event->mVG.mX = current_value;
                    event->mVG.tryUpdate(try_value, rapport);


                    if ( event->mVG.mLastAccepts.last() == true) {
                        event->updateW();
                        // Pour l'itération suivante : Car mVG a changé
                        current_h = try_h;
                    }

                    if (doMemo) {

                        // On stocke la racine de VG, qui est une variance pour afficher l'écart-type
                        double memoVG = sqrt(event->mVG.mX);
                        event->mVG.memo(&memoVG);
                        event->mVG.saveCurrentAcceptRate();

                    }

            }

    }


        } catch (std::exception& e) {
            qWarning()<< "MCMCLoopChronocurve::update VG : exception caught: " << e.what() << '\n';

        } catch(...) {
            qWarning() << "MCMCLoopChronocurve::update VG Event Caught Exception!\n";

        }

    // Pas bayésien : on sauvegarde la valeur constante dans la trace
    } else {
        for (Event*& event : mModel->mEvents) {
            event->mVG.tryUpdate(event->mVG.mX, 1);
            if (doMemo) {
                event->mVG.memo();
                event->mVG.saveCurrentAcceptRate();
            }
        }
    }
    
    // --------------------------------------------------------------
    //  Update Alpha
    // --------------------------------------------------------------
    if (mChronocurveSettings.mCoeffLissageType == ChronocurveSettings::eModeBayesian) {
        const double logMin = -100.;//log10(1e-20);
        const double logMax = +30;//log10(1e+30);

        // On stocke l'ancienne valeur :
        double alpha_current = mModel->mAlphaLissage.mX;
        // On tire une nouvelle valeur :
        const double alpha_new_log = Generator::gaussByBoxMuller(log10(alpha_current), mModel->mAlphaLissage.mSigmaMH);
        const double alpha_new = pow(10., alpha_new_log);
        
        double rapport = 0.;
        try {
            if (alpha_new_log >= logMin && alpha_new_log <= logMax) {
                SplineMatrices matrices = prepareCalculSpline(mModel->mEvents, vecH);
                long double h_current = h_YWI_AY(matrices, mModel->mEvents, alpha_current, vecH) * h_alpha(matrices, mModel->mEvents.size(), alpha_current);

                // On force la mise à jour de la nouvelle valeur pour calculer h_new
                //mModel->mAlphaLissage.mX = alpha_new;
// pHd ::pb ici matriceR vide
                // Calcul du rapport :
 // pHd : matrices n'a pas changé


               // mModel->mAlphaLissage.mX = alpha_new;
              //  matrices = prepareCalculSpline(mModel->mEvents);
                long double h_new = h_YWI_AY(matrices, mModel->mEvents, alpha_new, vecH) * h_alpha(matrices, mModel->mEvents.size(), alpha_new);
                rapport = (h_current == 0) ? 1 : ((h_new * alpha_new) / (h_current * alpha_current));

                // On remet l'ancienne valeur, qui sera éventuellement mise à jour dans ce qui suit (Metropolis Hastings)
                //mModel->mAlphaLissage.mX = alpha_current;
            }

            mModel->mAlphaLissage.mX = alpha_current;
            mModel->mAlphaLissage.tryUpdate(alpha_new, rapport);

            if (doMemo) {
                // On stocke le log10 de alpha pour afficher les résultats a posteriori
                double memoAlpha = log10(mModel->mAlphaLissage.mX);
               // mModel->mAlphaLissage.mX = log10(memoAlpha);
                mModel->mAlphaLissage.memo(&memoAlpha);
               // mModel->mAlphaLissage.mX = std::move(memoAlpha);

                mModel->mAlphaLissage.saveCurrentAcceptRate();
                // qDebug()<<"mChronocurveSettings.mVarianceType == ChronocurveSettings::eModeBayesian memo mAlpha";
            }
        } catch(...) {
            qDebug() << "MCMCLoopChronocurve::update Alpha Caught Exception!\n";
        }


    }
    // Pas bayésien : on sauvegarde la valeur constante dans la trace
    else {
        mModel->mAlphaLissage.tryUpdate(mModel->mAlphaLissage.mX, 1);
        if (doMemo) {
            // On stocke le log10 de alpha pour afficher les résultats a posteriori
            double memoAlpha = log10(mModel->mAlphaLissage.mX);
            mModel->mAlphaLissage.memo(&memoAlpha);
            mModel->mAlphaLissage.saveCurrentAcceptRate();
        }
    }
    
    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------
    if (doMemo) {
        MCMCSpline spline;
        
        // prepareCalculSpline : ne fait pas intervenir les valeurs Y(x,y,z) des events :mais utilise les theta réduits
        // => On le fait une seule fois pour les 3 composantes

        SplineMatrices matrices = prepareCalculSpline(mModel->mEvents, vecH);
        
        // calculSpline utilise les Y des events
        // => On le calcule ici pour la première composante (x)

        std::vector<std::vector<long double>> matB; //matR;
        const double alpha = mModel->mAlphaLissage.mX;
        if (alpha != 0) {
            std::vector<std::vector<long double>> tmp = multiConstParMat(matrices.matQTW_1Q, alpha, 5);
            matB = addMatEtMat(matrices.matR, tmp, 5);

        } else {
            matB = matrices.matR;
        }

        // Decomposition_Cholesky de matB en matL et matD
        // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
        std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matB, 5, 1);
        //std::vector<long double> vecH = calculVecH(mModel->mEvents);
        std::vector<long double> vecTheta = getThetaEventVector(mModel->mEvents);

        // le calcul de l'erreur est influencé par VG qui induit mWinv, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
        std::vector<long double> vecErrG = calculSplineError(matrices, decomp, alpha); // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

        // qDebug()<< " update vecErrG = "; for (auto&& e : vecErrG) qDebug() << (double) e;
        // Tout le calcul précédent ne change pas


        SplineResults s = calculSplineX(matrices, vecH, decomp, matB, alpha);
        

        MCMCSplineComposante splineX;
        splineX.vecThetaEvents = vecTheta;
        splineX.vecG = std::move(s.vecG);
        splineX.vecGamma = std::move(s.vecGamma);

        splineX.vecErrG = vecErrG; //calculSplineError(matrices, s);
        
        spline.splineX = std::move(splineX);
        
        if ( mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel ||
             mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique ||
             mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D) {

                    // calculSpline utilise les Y des events
            // => On le calcule ici pour la seconde composante (y)

            s = calculSplineY(matrices, vecH, decomp, matB, alpha); //matL et matB ne sont pas changé
            
            MCMCSplineComposante splineY;

            splineY.vecG = std::move(s.vecG);
            splineY.vecGamma = std::move(s.vecGamma);

            splineY.vecThetaEvents = vecTheta; //getThetaEventVector(mModel->mEvents);
            splineY.vecErrG = vecErrG; //calculSplineError(matrices, s); // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =
            
            spline.splineY = std::move(splineY);
        }

        if ( mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel ||
             mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique ||
             mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D) {
            // dans le future ne sera pas utile pour le mode sphérique
            // calculSpline utilise les Z des events
            // => On le calcule ici pour la troisième composante (z)
            
            s = calculSplineZ(matrices, vecH, decomp, matB, alpha);

            MCMCSplineComposante splineZ;

            splineZ.vecG = std::move(s.vecG);
            splineZ.vecGamma = std::move(s.vecGamma);

            splineZ.vecThetaEvents = vecTheta; //getThetaEventVector(mModel->mEvents);
            splineZ.vecErrG = vecErrG; //calculSplineError(matrices, s);
            
            spline.splineZ = std::move(splineZ);
        }
        
        mModel->mMCMCSplines.push_back(spline);
    }
    
    // --------------------------------------------------------------
    //  Restauration des valeurs des theta en années (non espacés pour le calcul)
    // --------------------------------------------------------------
 //   } catch(...) {
 //            qDebug() << "MCMCLoopChronocurve::update () Caught Exception!\n";
 //            return;
 //         }
    try {
     //   restoreEventsTheta(mModel->mEvents);
    } catch(...) {
        qDebug() << "MCMCLoopChronocurve::update () restoreEventsTheta : Caught Exception!\n";
        return;
    }
    if ((mState == eRunning) && (chain.mRunIterIndex == chain.mNumRunIter-1)) {
        mModel->mLogResults += "<hr>";
        mModel->mLogResults += line(textBold(tr("Event adaptation for chain %1").arg(QString::number(mChainIndex+1))) );
        for (auto&& event : mModel->mEvents) {
            mModel->mLogResults += "<hr>";
            mModel->mLogResults += line(textBold(tr("Event : %1 ").arg(event->mName)) );
            mModel->mLogResults += line( textBold(tr("- Acceptance rate : %1 percent").arg( QString::number(100. * event->mTheta.getCurrentAcceptRate()))) );
            // mAdaptLog += line(textBold(tr("- Theta : %1").arg( QString::number(event->mTheta.mX))) );
            mModel->mLogResults += line(textBold(tr("- Sigma_MH on Theta : %1 at %2 ").arg( QString::number(event->mTheta.mSigmaMH), QString::number(100. * event->mTheta.getCurrentAcceptRate()))) );

            for (auto&& date : event->mDates )   {
                mModel->mLogResults += line( textBold(tr("Data : %1").arg(date.mName)) );
                mModel->mLogResults += line( textBold(tr("- Acceptance rate : %1 percent").arg( QString::number(100. * date.mTheta.getCurrentAcceptRate()))) );
                mModel->mLogResults += line( textBold(tr("- Sigma_MH on ti : %1").arg(QString::number(date.mTheta.mSigmaMH) )));
                mModel->mLogResults += line( textBold(tr("- Sigma_i : %1 ").arg(QString::number(date.mSigma.mX) )) );
                mModel->mLogResults += line( textBold(tr("- Sigma_i acceptance rate : %1 percent").arg( QString::number(100. * date.mSigma.getCurrentAcceptRate()))) );
                mModel->mLogResults += line( textBold(tr("- Sigma_MH on Sigma_i : %1").arg(QString::number(date.mSigma.mSigmaMH, 'f'))) );
            }

        }

    }
 } catch (const char* e) {
           qWarning() << "MCMCLoopChronocurve::update () char "<< e;


 } catch (const std::length_error& e) {
        qWarning() << "MCMCLoopChronocurve::update () length_error"<< e.what();
 } catch (const std::out_of_range& e) {
        qWarning() << "MCMCLoopChronocurve::update () out_of_range" <<e.what();
 } catch (const std::exception& e) {
        qWarning() << "MCMCLoopChronocurve::update () "<< e.what();

 } catch(...) {
        qWarning() << "MCMCLoopChronocurve::update () Caught Exception!\n";
        return;
    }

}

bool MCMCLoopChronocurve::adapt()
{
    ChainSpecs& chain = mChains[mChainIndex];

    const double taux_min = 41.;           // taux_min minimal rate of acceptation=42
    const double taux_max = 47.;           // taux_max maximal rate of acceptation=46

    bool allOK = true;

    //--------------------- Adapt -----------------------------------------

    double delta = (chain.mBatchIndex < 10000) ? 0.01 : (1. / sqrt(chain.mBatchIndex));

    for (auto& event : mModel->mEvents) {

       for (auto& date : event->mDates) {

            //--------------------- Adapt Sigma MH de Theta i -----------------------------------------

            if (date.mMethod == Date::eMHSymGaussAdapt) {
                const double taux = 100. * date.mTheta.getCurrentAcceptRate();
                if (taux <= taux_min || taux >= taux_max) {
                    allOK = false;
                    const double sign = (taux <= taux_min) ? -1. : 1.;
                    date.mTheta.mSigmaMH *= pow(10., sign * delta);
                }
            }

            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------

            const double taux = 100. * date.mSigma.getCurrentAcceptRate();
            if (taux <= taux_min || taux >= taux_max) {
                allOK = false;
                const double sign = (taux <= taux_min) ? -1. : 1.;
                date.mSigma.mSigmaMH *= pow(10., sign * delta);
            }
        }

        //--------------------- Adapt Sigma MH de Theta f -----------------------------------------

        double taux = 100. * event->mTheta.getCurrentAcceptRate();
        if (taux <= taux_min || taux >= taux_max) {
            allOK = false;
            const double sign = (taux <= taux_min) ? -1. : 1.;
            event->mTheta.mSigmaMH *= pow(10., sign * delta);
        }
        
        //--------------------- Adapt Sigma MH de VG -----------------------------------------

        taux = 100. * event->mVG.getCurrentAcceptRate();
        if (taux <= taux_min || taux >= taux_max) {
            allOK = false;
            const double sign = (taux <= taux_min) ? -1. : 1.;
            event->mVG.mSigmaMH *= pow(10., sign * delta);
        }
    }
    
    //--------------------- Adapt Sigma MH de Alpha Lissage -----------------------------------------

    const double taux = 100. * mModel->mAlphaLissage.getCurrentAcceptRate();
    if (taux <= taux_min || taux >= taux_max) {
        allOK = false;
        const double sign = (taux <= taux_min) ? -1. : 1.;
        mModel->mAlphaLissage.mSigmaMH *= pow(10., sign * delta);
    }
    
    return allOK;
}

void MCMCLoopChronocurve::finalize()
{
    // if we are in depth mode, we have to extract iteration with only GPrime positive
    if ( mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeUnivarie &&
         mChronocurveSettings.mVariableType == ChronocurveSettings::eVariableTypeProfondeur) {

        int shift  = 0;


        std::vector<MCMCSpline> newMCMCSplines;
        for (int i = 0; i < mChains.size(); ++i) {
            ChainSpecs& chain = mChains[i];

            // we add 1 for the init
            const int initBurnAdaptSize = 1 + chain.mNumBurnIter + int (chain.mBatchIndex * chain.mNumBatchIter);
            const int runSize = floor(chain.mNumRunIter / chain.mThinningInterval);
            const int firstRunPosition = shift + initBurnAdaptSize ;

            std::vector<MCMCSpline>::const_iterator first = mModel->mMCMCSplines.begin() + firstRunPosition;
            std::vector<MCMCSpline>::const_iterator last = first + runSize -1;

            std::vector<MCMCSpline> chainTrace(first, last+1);

            std::vector<MCMCSplineComposante> chainTraceX;

            for (auto&& chTr : chainTrace) {
                chainTraceX.push_back(chTr.splineX);
            }
            std::vector<unsigned> positvIter ;//= listOfIterationsWithPositiveGPrime(chainTraceX); //{5};

            emit stepChanged(tr("Select Positive curves for chain %1").arg(i), 0, chainTraceX.size());
            for (unsigned iter = 0; iter < chainTraceX.size(); ++iter) {
                if (asPositiveGPrime(chainTraceX.at(iter)))
                    positvIter.push_back(iter);
                emit stepProgressed(iter);
            }

            qDebug() << "pourcentage profondeur"<< positvIter.size() << "sur "<< runSize;

            if ( positvIter.size() == 0) {
#ifdef DEBUG
                 qCritical("MCMCLoopChronocurve::finalize : NO POSITIVE curve available\n");
#endif
                 mAbortedReason = QString(tr("Warning : NO POSITIVE curve available with chain n° %1").arg (i));
                 throw mAbortedReason;

            } else {
                emit stepChanged(tr("Erase invalide curves for chain %1").arg(i), 0, chainTrace.size());
                int backshift = 0;
                for (unsigned j = 0; j < chainTrace.size(); ++j) {

                    if (std::binary_search (positvIter.begin(), positvIter.end(), j) == false) {

                        // remove from MCMCSplines, Event, data and Phase
                        mModel->mMCMCSplines.erase(mModel->mMCMCSplines.begin() + firstRunPosition + j - backshift);
                        mModel->mAlphaLissage.mRawTrace->erase(mModel->mAlphaLissage.mRawTrace->begin() + firstRunPosition + j - backshift);
                        mModel->mAlphaLissage.mHistoryAcceptRateMH->erase(mModel->mAlphaLissage.mHistoryAcceptRateMH->begin() + firstRunPosition + j - backshift);

                        for (auto& ev : mModel->mEvents) {
                            ev->mTheta.mRawTrace->erase(ev->mTheta.mRawTrace->begin() + firstRunPosition + j - backshift);
                            ev->mTheta.mHistoryAcceptRateMH->erase(ev->mTheta.mHistoryAcceptRateMH->begin() + firstRunPosition + j - backshift);

                            ev->mVG.mRawTrace->erase(ev->mVG.mRawTrace->begin() + firstRunPosition + j - backshift);
                            ev->mVG.mHistoryAcceptRateMH->erase(ev->mVG.mHistoryAcceptRateMH->begin() + firstRunPosition + j - backshift);
                            for (auto& dat : ev->mDates) {
                                dat.mTheta.mRawTrace->erase(dat.mTheta.mRawTrace->begin() + firstRunPosition + j - backshift);
                                dat.mTheta.mHistoryAcceptRateMH->erase(dat.mTheta.mHistoryAcceptRateMH->begin() + firstRunPosition + j - backshift);

                                dat.mSigma.mRawTrace->erase(dat.mSigma.mRawTrace->begin() + firstRunPosition + j - backshift);
                                dat.mSigma.mHistoryAcceptRateMH->erase(dat.mSigma.mHistoryAcceptRateMH->begin() + firstRunPosition + j - backshift);

                                dat.mWiggle.mRawTrace->erase(dat.mWiggle.mRawTrace->begin() + firstRunPosition + j - backshift);
                                // dat.mWiggle.mHistoryAcceptRateMH->erase(dat.mWiggle.mHistoryAcceptRateMH->begin() + firstRunPosition + j - backshift);
                            }

                        }
                        for (auto& ph : mModel->mPhases) {
                            ph->mAlpha.mRawTrace->erase(ph->mAlpha.mRawTrace->begin() + firstRunPosition + j - backshift);
                            ph->mBeta.mRawTrace->erase(ph->mBeta.mRawTrace->begin() + firstRunPosition + j - backshift);
                            ph->mDuration.mRawTrace->erase(ph->mDuration.mRawTrace->begin() + firstRunPosition + j - backshift);
                        }
                        backshift++;
                    }
                    emit stepProgressed(j);
                }
            }
            // Correction des paramètres de la chaine
           // chain.mThinningInterval = 1;
            chain.mNumRunIter = positvIter.size() * chain.mThinningInterval;

            shift = firstRunPosition + positvIter.size() ;

        }

    }

    // This is not a copy of all data!
    // Chains only contain description of what happened in the chain (numIter, numBatch adapt, ...)
    // Real data are inside mModel members (mEvents, mPhases, ...)
    mModel->mChains = mChains;

    // This is called here because it is calculated only once and will never change afterwards
    // This is very slow : it is for this reason that the results display may be long to appear at the end of MCMC calculation.
    /** @todo Find a way to make it faster !
     */
    mModel->generateCorrelations(mChains);
    
    // This should not be done here because it uses resultsView parameters
    // ResultView will trigger it again when loading the model
    //mModel->generatePosteriorDensities(mChains, 1024, 1);
    
    // Generate numerical results of :
    // - MHVariables (global acceptation)
    // - MetropolisVariable : analysis of Posterior densities and quartiles from traces.
    // This also should be done in results view...
    // mModel->generateNumericalResults(mChains);
    
    // ----------------------------------------
    // Chronocurve specific :
    // ----------------------------------------

    const bool hasY = (mChronocurveSettings.mProcessType != ChronocurveSettings::eProcessTypeUnivarie);
    const bool hasZ = (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel ||
                       mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D);

    std::vector<MCMCSplineComposante> allChainsTraceX, chainTraceX, allChainsTraceY, chainTraceY, allChainsTraceZ, chainTraceZ;

    
#ifdef DEBUG
    qDebug()<<QString("MCMCLoopChronocurve::finalize");
    QElapsedTimer startTime;
    startTime.start();
#endif
    int shift  = 0;
    int shiftTrace = 0;


    for (int i = 0; i < mChains.size(); ++i) {

        const int initBurnAdaptSize = 1 + mChains.at(i).mNumBurnIter + int (mChains.at(i).mBatchIndex * mChains.at(i).mNumBatchIter);
        const int runSize = floor(mChains.at(i).mNumRunIter / mChains.at(i).mThinningInterval);
        const int firstRunPosition = shift + initBurnAdaptSize;

        std::vector<MCMCSpline>::iterator first = mModel->mMCMCSplines.begin() + firstRunPosition ;
        std::vector<MCMCSpline>::iterator last = first + runSize -1 ;

        /*
         * Mettre ici le retour en coordonnée sphérique ou vectoriel
         *
         *   F:=sqrt(sqr(Gx)+sqr(Gy)+sqr(Gz));

        Ic:=arcsinus(Gz/F);
        Dc:=angleD(Gx,Gy);

        Tab_chemin_IDF[iJ].IiJ:=Ic*Deg;
        Tab_chemin_IDF[iJ].DiJ:=Dc*Deg;
        Tab_chemin_IDF[iJ].FiJ:=F;

        // Calcul de la boule d'erreur moyenne par moyenne quadratique
        ErrGx:=Tab_parametrique[iJ].ErrGx;
        ErrGy:=Tab_parametrique[iJ].ErrGy;
        ErrGz:=Tab_parametrique[iJ].ErrGz;
        ErrIDF:=sqrt((sqr(ErrGx)+sqr(ErrGy)+sqr(ErrGz))/3);

    // sauvegarde des erreurs sur chaque param�tre  - on convertit en degr�s pour I et D
        Tab_chemin_IDF[iJ].ErrI:=(ErrIDF/Tab_chemin_IDF[iJ].Fij)*deg;
        Tab_chemin_IDF[iJ].ErrD:=(ErrIDF/(Tab_chemin_IDF[iJ].Fij*cos(Tab_chemin_IDF[iJ].Iij*rad)))*deg;
        Tab_chemin_IDF[iJ].ErrF:=ErrIDF;
         *
         *
         */

        chainTraceX.clear();
        chainTraceY.clear();
        chainTraceZ.clear();
        unsigned progressPosition = 0;
        emit stepChanged(tr("Compute Posterior Mean Composantes for chain %1").arg(i), 0, runSize);
        for (auto& cTrace = first; cTrace != last+1; ++cTrace) {

            chainTraceX.push_back(cTrace->splineX);
            allChainsTraceX.push_back(cTrace->splineX);

            if (hasY) {
                chainTraceY.push_back(cTrace->splineY);
                allChainsTraceY.push_back(cTrace->splineY);
            }

            if (hasZ) {
                chainTraceZ.push_back(cTrace->splineZ);
                allChainsTraceZ.push_back(cTrace->splineZ);
            }
            emit stepProgressed(progressPosition++);
        }



      PosteriorMeanG chainPosteriorMeanG;
      chainPosteriorMeanG.gx = computePosteriorMeanGComposante(chainTraceX);

      if (hasY) {
          chainPosteriorMeanG.gy = computePosteriorMeanGComposante(chainTraceY);
      }
      if (hasZ) {
          chainPosteriorMeanG.gz = computePosteriorMeanGComposante(chainTraceZ);
      }


      mModel->mPosteriorMeanGByChain.push_back(chainPosteriorMeanG);

      shiftTrace += runSize;
      shift = firstRunPosition + runSize;

    }
    
    emit stepChanged(tr("Compute Posterior Mean Composantes X for all chains..."), 0, 0);
    PosteriorMeanG allChainsPosteriorMeanG;
    allChainsPosteriorMeanG.gx = computePosteriorMeanGComposante(allChainsTraceX);
    if (hasY) {
        emit stepChanged(tr("Compute Posterior Mean Composantes Y for all chains..."), 0, 0);
        allChainsPosteriorMeanG.gy = computePosteriorMeanGComposante(allChainsTraceY);
    }
    if (hasZ) {
        emit stepChanged(tr("Compute Posterior Mean Composantes Z for all chains..."), 0, 0);
        allChainsPosteriorMeanG.gz = computePosteriorMeanGComposante(allChainsTraceZ);
    }

    // Conversion apres moyenne
    if ( mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel ||
         mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique) {
        emit stepChanged(tr("Compute system Conversion..."), 0, 0);

        conversionIDF(allChainsPosteriorMeanG);

        for (auto&& chain: mModel->mPosteriorMeanGByChain)
            conversionIDF(chain);

    }


    mModel->mPosteriorMeanG = std::move(allChainsPosteriorMeanG);

#ifdef DEBUG

    QTime timeDiff(0,0,0, int(startTime.elapsed()));
    qDebug() <<  QString("=> MCMCLoopChronocurve::finalize done in %1 h %2 m %3 s %4 ms").arg(QString::number(timeDiff.hour()),
                                                                                      QString::number(timeDiff.minute()),
                                                                                      QString::number(timeDiff.second()),
                                                                                      QString::number(timeDiff.msec()) );
#endif


}

PosteriorMeanGComposante MCMCLoopChronocurve::computePosteriorMeanGComposante(const std::vector<MCMCSplineComposante>& trace)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    const double step = mModel->mSettings.mStep;

    const unsigned nbPoint = floor ((tmax - tmin +1) /step);

    std::vector<long double> vecCumulG2 (nbPoint);
    std::vector<long double> vecCumulErrG2 (nbPoint);
    
    std::vector<long double> vecG (nbPoint);
    std::vector<long double> vecGP (nbPoint);
    std::vector<long double> vecGS (nbPoint);
    std::vector<long double> vecErrG (nbPoint);
    
    unsigned long nbIter = trace.size();

    long double t, g, gp, gs, errG;
    g = 0.;
    gp = 0;
    errG = 0;
    gs = 0;
    for (unsigned i = 0; i<nbIter; ++i) {
        const MCMCSplineComposante& splineComposante = trace.at(i);
        
        unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
        for (unsigned tIdx = 0; tIdx < nbPoint ; ++tIdx) {
            t = (long double)tIdx * step + tmin ;
            valeurs_G_ErrG_GP_GS(t, splineComposante, g, errG, gp, gs, i0);

            vecG[tIdx] += g;
            vecGP[tIdx] += std::move(gp) ;
            vecGS[tIdx] += std::move(gs);
            vecCumulG2[tIdx] += pow(g, 2.);
            vecCumulErrG2[tIdx] += pow(errG, 2.);
            //vecCumulErrG2[tIdx] += errG;//pow(errG, 2.);

        }

    }

    const double nbIter_tmax = nbIter * (tmax - tmin);
    const double nbIter_tmax2 = nbIter * pow(tmax - tmin, 2.);

    for (unsigned tIdx = 0; tIdx < nbPoint ; ++tIdx) {
        vecG[tIdx] /= nbIter;
        vecGP[tIdx] /=  nbIter_tmax;
        vecGS[tIdx] /=  nbIter_tmax2;
        vecErrG[tIdx] = sqrt( (vecCumulG2.at(tIdx) / nbIter) - pow(vecG.at(tIdx), 2.) + (vecCumulErrG2.at(tIdx) / nbIter));
        //vecErrG[tIdx] = vecCumulErrG2.at(tIdx) / nbIter;//sqrt((vecCumulG2.at(tIdx) / nbIter) - pow(vecG.at(tIdx), 2.) + (vecCumulErrG2.at(tIdx) / nbIter));
    }
    
    PosteriorMeanGComposante result;
    result.vecG = std::move(vecG);
    result.vecGP = std::move(vecGP);
    result.vecGS = std::move(vecGS);
    result.vecGErr = std::move(vecErrG);
    
    return result;
}

double MCMCLoopChronocurve::valeurG(const double t, const MCMCSplineComposante& spline, unsigned &i0)
{
    const unsigned n = spline.vecThetaEvents.size();
    double tReduce =reduceTime(t);
    const double t1 = reduceTime(spline.vecThetaEvents.at(0));
    const double tn = reduceTime(spline.vecThetaEvents.at(n-1));
    double g = 0;
    
    if (tReduce < t1) {
        double t2 = reduceTime(spline.vecThetaEvents.at(1));
        double gp1 = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
        gp1 -= (t2 - t1) * spline.vecGamma.at(1) / 6.;
        g = spline.vecG.at(0) - (t1 - tReduce) * gp1;

    } else if(tReduce >= tn) {
        double tn1 = reduceTime(spline.vecThetaEvents[n-2]);
        double gpn = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tn1); // code d'origine
       // double gpn = (spline.vecG.at(n-2) - spline.vecG.at(n-1)) / (tn - tn1);
        gpn += (tn - tn1) * spline.vecGamma.at(n-2) / 6.;
        g = spline.vecG.at(n-1) + (tReduce - tn) * gpn;

    } else {
        for (; i0 < n-1; ++i0) {
            double ti1 = reduceTime(spline.vecThetaEvents.at(i0));
            double ti2 = reduceTime(spline.vecThetaEvents.at(i0+1));
            if ((tReduce >= ti1) && (tReduce < ti2)) {
                double h = ti2 - ti1;
                double gi1 = spline.vecG.at(i0);
                double gi2 = spline.vecG.at(i0+1);

                // Linear part :
              //  g = ( (tReduce-ti1) * gi2 + (ti2-tReduce) * gi1 ) / h; // modif pHd
               // g = gi1 - (gi2-gi1)*(ti1-tReduce)/h; // code d'origine
                g = gi1 + (gi2-gi1)*(tReduce-ti1)/h;

                // Smoothing part :
                double gamma1 = spline.vecGamma.at(i0);
                double gamma2 = spline.vecGamma.at(i0+1);
                double p = (1./6.) * ((tReduce-ti1) * (ti2-tReduce)) * ((1.+(tReduce-ti1)/h) * gamma2 + (1.+(ti2-tReduce)/h) * gamma1); // code d'origine
                g -= p;

                break;
            }
        }
    }

    return g;
}

// ------------------------------------------------------------------
//  Valeur de Err_G en t à partir de Vec_Err_G
//  par interpolation linéaire des erreurs entre les noeuds
// ------------------------------------------------------------------
double MCMCLoopChronocurve::valeurErrG(const double t, const MCMCSplineComposante& spline, unsigned& i0)
{
    const unsigned n = spline.vecThetaEvents.size();
    
    double t1 = spline.vecThetaEvents[0];
    double tn = spline.vecThetaEvents[n-1];
    double errG = 0;
    
    if (t < t1) {
        errG = spline.vecErrG.at(0);

    } else if(t >= tn) {
        errG = spline.vecErrG.at(n-1);

    } else {
        for (; i0 <n-1; ++i0) {
            double ti1 = spline.vecThetaEvents[i0];
            double ti2 = spline.vecThetaEvents[i0+1];
            if ((t >= ti1) && (t < ti2)) {
                /* code rencurve
                 *   Err1:=Vec_splineP.Err_G[i];
                 * Err2:=Vec_splineP.Err_G[i+1];
                 * Err_G:= Err1+((t-ti1)/(ti2-ti1))*(Err2-Err1);
                 */
                double err1 = spline.vecErrG[i0];
                double err2 = spline.vecErrG[i0+1];
                errG = err1 + ((t-ti1) / (ti2-ti1)) * (err2 - err1);
                break;
            }
        }
    }

    return errG;
}

// cans RenCurve U-CMT-Routine_Spline Valeur_Gp
double MCMCLoopChronocurve::valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0)
{
   unsigned n = spline.vecThetaEvents.size();
   const double tReduce =  reduceTime(t);
   const double t1 = reduceTime(spline.vecThetaEvents.at(0));
   const double tn = reduceTime(spline.vecThetaEvents.at(n-1));
   double gPrime = 0.;
    
    // la dérivée première est toujours constante en dehors de l'intervalle [t1,tn]
    if (tReduce < t1) {
        const double t2 = reduceTime(spline.vecThetaEvents.at(1));
        gPrime = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
        gPrime -= (t2 - t1) * spline.vecGamma.at(1) / 6.;
        /* Code Rencurve
         * ti2:=Vec_splineP.t[2];
         * GP1:=(Vec_spline.G[2]-Vec_spline.g[1])/(ti2-ti1);
         * GP:=GP1-(ti2-ti1)*Vec_spline.gamma[2]/6;
         */



    } else if (tReduce >= tn) {
        const double tin_1 = reduceTime(spline.vecThetaEvents.at(n-2));
        gPrime = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tin_1); // code d'origine
       //  gPrime = (spline.vecG.at(n-2) - spline.vecG.at(n-1)) / (tn - tn1);
        gPrime += (tn - tin_1) * spline.vecGamma.at(n-2) / 6.;

        /* Code Rencurve
         * tin_1:=Vec_splineP.t[nb_noeuds-1];
         * GPn:=(Vec_spline.G[nb_noeuds]-Vec_spline.G[nb_noeuds-1])/(tin-tin_1);
         * GP:=GPn+(tin-tin_1)*Vec_spline.gamma[nb_noeuds-1]/6;
         */

    } else {
        for ( ;i0< n-1; ++i0) { // pHd  !!!recherche et interpolation de GPrime
            const double ti1 = reduceTime(spline.vecThetaEvents.at(i0));
            const double ti2 = reduceTime(spline.vecThetaEvents.at(i0+1));
            if ((tReduce >= ti1) && (tReduce < ti2)) {
                double h = ti2 - ti1;
                double gi1 = spline.vecG.at(i0);
                double gi2 = spline.vecG.at(i0+1);
                double gamma1 = spline.vecGamma.at(i0);
                double gamma2 = spline.vecGamma.at(i0+1);

                gPrime = ((gi2-gi1)/h) - (1./6.) * (tReduce-ti1) * (ti2-tReduce) * ((gamma2-gamma1)/h);
                gPrime += (1./6.) * ((tReduce-ti1) - (ti2-tReduce)) * ( (1.+(tReduce-ti1)/h) * gamma2 + (1+(ti2-tReduce)/h) * gamma1 );

                /* Code Rencurve
                 * h:=ti2-ti1;
                 * Gi1    := Vec_spline.G[i];
                 * Gi2    := Vec_spline.G[i+1];
                 * gamma1 := Vec_spline.gamma[i];
                 * gamma2 := Vec_spline.gamma[i+1];
                 * GP := ((Gi2-Gi1)/h) - (1/6)*(t-ti1)*(ti2-t)*((gamma2-gamma1)/h);
                 * GP := GP + (1/6)*((t-ti1)-(ti2-t)) * ( (1+(t-ti1)/h)*gamma2 + (1+(ti2-t)/h)*gamma1 );
                */
                break;
            }
        }




    }

    return gPrime;
}

double MCMCLoopChronocurve::valeurGSeconde(const double t, const MCMCSplineComposante& spline)
{
   const int n = spline.vecThetaEvents.size();
    const double tReduce = reduceTime(t);
    // la dérivée seconde est toujours nulle en dehors de l'intervalle [t1,tn]
    double gSeconde = 0.;
    
    for (int i = 0; i < n-1; ++i) {
        const double ti1 = reduceTime(spline.vecThetaEvents.at(i));
        const double ti2 = reduceTime(spline.vecThetaEvents.at(i+1));
        if ((tReduce >= ti1) && (tReduce < ti2)) {
            const double h = ti2 - ti1;
            const double gamma1 = spline.vecGamma.at(i);
            const double gamma2 = spline.vecGamma.at(i+1);
            gSeconde = ((tReduce-ti1) * gamma2 + (ti2-tReduce) * gamma1) / h;
            break;
        }
    }
    
    return gSeconde;
}

void MCMCLoopChronocurve::valeurs_G_ErrG_GP_GS(const double t, const MCMCSplineComposante& spline, long double& G, long double& errG, long double& GP, long double& GS, unsigned& i0)
{

    unsigned n = spline.vecThetaEvents.size();
    const double tReduce =  reduceTime(t);
    const double t1 = reduceTime(spline.vecThetaEvents.at(0));
    const double tn = reduceTime(spline.vecThetaEvents.at(n-1));
    GP = 0.;
    GS = 0.;

     // la dérivée première est toujours constante en dehors de l'intervalle [t1,tn]
     if (tReduce < t1) {
         const double t2 = reduceTime(spline.vecThetaEvents.at(1));

         // ValeurGPrime
         GP = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
         GP -= (t2 - t1) * spline.vecGamma.at(1) / 6.;

         // ValeurG
         G = spline.vecG.at(0) - (t1 - tReduce) * GP;

         // valeurErrG
         errG = spline.vecErrG.at(0);

         // valeurGSeconde
         //GS = 0.;

     } else if (tReduce >= tn) {

         const double tn1 = reduceTime(spline.vecThetaEvents.at(n-2));
         // ValeurG

         // valeurErrG
         errG = spline.vecErrG.at(n-1);

         // ValeurGPrime
         GP = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tn1);
         GP += (tn - tn1) * spline.vecGamma.at(n-2) / 6.;

         // valeurGSeconde
         //GS =0.;

         G = spline.vecG.at(n-1) + (tReduce - tn) * GP;

     } else {
         for (; i0 < n-1; ++i0) {
             const double ti1 = reduceTime(spline.vecThetaEvents.at(i0));
             const double ti2 = reduceTime(spline.vecThetaEvents.at(i0 + 1));
             if ((tReduce >= ti1) && (tReduce < ti2)) {
                 const double h = ti2 - ti1;
                 const double gi1 = spline.vecG.at(i0);
                 const double gi2 = spline.vecG.at(i0 + 1);
                 const double gamma1 = spline.vecGamma.at(i0);
                 const double gamma2 = spline.vecGamma.at(i0 + 1);

                 // ValeurG
                 /* code rencurve
                  *  G := ( (t-ti1)*Gi2+(ti2-t)*Gi1 )/h;
                  *  G := G - (1/6)*(t-ti1)*(ti2-t) * ( (1+(t-ti1)/h)*gamma2 + (1+(ti2-t)/h)*gamma1 );
                  */
                    // Linear part :
                // G = gi1 + (gi2-gi1)*(tReduce-ti1)/h;
                 G = ( (tReduce-ti1)*gi2 + (ti2-tReduce)*gi1 ) /h;
                    // Smoothing part :
                 G -= (1./6.) * ((tReduce-ti1) * (ti2-tReduce)) * ((1.+(tReduce-ti1)/h) * gamma2 + (1.+(ti2-tReduce)/h) * gamma1);

                 // valeurErrG
                 /* Code Rencurve
                  *   Err1:=Vec_splineP.Err_G[i];
                  *   Err2:=Vec_splineP.Err_G[i+1];
                  *   Err_G:= Err1+((t-ti1)/(ti2-ti1))*(Err2-Err1);
                  */
                 const double err1 = spline.vecErrG.at(i0);
                 const double err2 = spline.vecErrG.at(i0 + 1);
                 errG = err1 + ((tReduce-ti1) / (ti2-ti1)) * (err2 - err1);
                // errG = err2;
                // errG = ( (tReduce-ti1)*err2 + (ti2-tReduce)*err1 ) /h;

                 // ValeurGPrime
                 /* Code RenCurve
                  *   h:=ti2-ti1;
                  *   Gi1    := Vec_spline.G[i];
                  *   Gi2    := Vec_spline.G[i+1];
                  *   gamma1 := Vec_spline.gamma[i];
                  *   gamma2 := Vec_spline.gamma[i+1];
                  *   GP := ((Gi2-Gi1)/h) - (1/6)*(t-ti1)*(ti2-t)*((gamma2-gamma1)/h);
                  *   GP := GP + (1/6)*((t-ti1)-(ti2-t)) * ( (1+(t-ti1)/h)*gamma2 + (1+(ti2-t)/h)*gamma1 );
                  */

                 GP = ((gi2-gi1)/h) - (1./6.) * (tReduce-ti1) * (ti2-tReduce) * ((gamma2-gamma1)/h);
                 GP += (1./6.) * ((tReduce-ti1) - (ti2-tReduce)) * ( (1.+(tReduce-ti1)/h) * gamma2 + (1+(ti2-tReduce)/h) * gamma1 );

                 // valeurGSeconde
                 GS = ((tReduce-ti1) * gamma2 + (ti2-tReduce) * gamma1) / h;

                 break;
             }
         }

     }


}



#pragma mark Related to : calibrate

void MCMCLoopChronocurve::prepareEventsY(const QList<Event *> &lEvents)
{
    for (Event* event : lEvents) {
        prepareEventY(event);
    }
}

/**
 * Preparation des valeurs Yx, Yy, Yz et Sy à partir des valeurs saisies dans l'interface : Yinc, Ydec, Sinc, Yint, Sint
 */
void MCMCLoopChronocurve::prepareEventY(Event* const event  )
{
    const double rad = M_PI / 180.;
    if (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeUnivarie) {
        if (mChronocurveSettings.mVariableType == ChronocurveSettings::eVariableTypeInclinaison) {
            event->mYx = event->mYInc;
            event->mSy = event->mSInc;

        } else if (mChronocurveSettings.mVariableType == ChronocurveSettings::eVariableTypeDeclinaison) {
            event->mYx = event->mYDec;
            event->mSy = event->mSInc / cos(event->mYInc * rad);

        } else {
            event->mYx = event->mYInt;
            event->mSy = event->mSInt;
        }
        
        // Non utilisé en univarié, mais mis à zéro notamment pour les exports CSV :
        event->mYy = 0;
        event->mYz = 0;

    } else if (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique) {
        event->mYInt = 100.; // Pour traiter le cas sphérique , on utilise le cas vectoriel en posant Yint = 100
        event->mSInt = 0.;

        event->mYx = event->mYInt * cos(event->mYInc * rad) * cos(event->mYDec * rad);
        event->mYy = event->mYInt * cos(event->mYInc * rad) * sin(event->mYDec * rad);
        event->mYz = event->mYInt * sin(event->mYInc * rad);

        const double sYInc = event->mSInc / 2.4477;
        event->mSy = sqrt( (pow(event->mSInt, 2.) + 2. * pow(event->mYInt, 2.) * pow(sYInc, 2.)) /3.);

    } else if (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel) {
        event->mYx = event->mYInt * cos(event->mYInc * rad) * cos(event->mYDec * rad);
        event->mYy = event->mYInt * cos(event->mYInc * rad) * sin(event->mYDec * rad);
        event->mYz = event->mYInt * sin(event->mYInc * rad);

        const double sYInc = event->mSInc /2.4477 ;
        event->mSy = sqrt( (pow(event->mSInt, 2.) + 2. * pow(event->mYInt, 2.) * pow(sYInc, 2.)) /3.);

    } else if (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D) {

        event->mYx = event->mYInc;
        event->mYy = event->mYDec;
        event->mYz = event->mYInt;

        event->mSy = sqrt( (pow(event->mSInt, 2.) + pow(event->mSInc, 2.)) /2.);

    }
    
    if (!mChronocurveSettings.mUseErrMesure) {
        event->mSy = 0.;
    }
}

/*void MCMCLoopChronocurve::prepareEventY(EventKnown* const event)
{
    const double rad = M_PI / 180.;
    if (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeUnivarie) {
        if (mChronocurveSettings.mVariableType == ChronocurveSettings::eVariableTypeInclinaison) {
            event->mYx = event->mYInc;
            event->mSy = event->mSInc / 2.4477;

        } else if (mChronocurveSettings.mVariableType == ChronocurveSettings::eVariableTypeDeclinaison) {
            event->mYx = event->mYDec;
            event->mSy = event->mSInc / (2.4477 * cos(event->mYInc * rad));

        } else {
            event->mYx = event->mYInt;
            event->mSy = event->mSInt;
        }

        // Non utilisé en univarié, mais mis à zéro notamment pour les exports CSV :
        event->mYy = 0;
        event->mYz = 0;

    } else if (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique) {
        event->mYInt = 100.; // Pour traiter le cas sphérique , on utilise le cas vectoriel en posant Yint = 100
        event->mSInt = 0.;//
        // fichier cmt_lit_sauve, ligne 637
        event->mYx = event->mYInt * cos(event->mYInc * rad) * cos(event->mYDec * rad);
        event->mYy = event->mYInt * cos(event->mYInc * rad) * sin(event->mYDec * rad);
        event->mYz = event->mYInt * sin(event->mYInc * rad);

        const double sYInc = event->mSInc / 2.4477;
        event->mSy = sqrt( (pow(event->mSInt, 2.) + 2. * pow(event->mYInt, 2.) * pow(sYInc, 2.)) /3.);

        // Non utilisé en univarié, mais mis à zéro notamment pour les exports CSV :
       // event->mYz = 0;

    } else if (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel) {
        event->mYx = event->mYInt * cos(event->mYInc * rad) * cos(event->mYDec * rad);
        event->mYy = event->mYInt * cos(event->mYInc * rad) * sin(event->mYDec * rad);
        event->mYz = event->mYInt * sin(event->mYInc * rad);

        const double sYInc = event->mSInc / 2.4477;
        event->mSy = sqrt( (pow(event->mSInt, 2.) + 2. * pow(event->mYInt, 2.) * pow(sYInc, 2.)) /3.);

    } else if (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D) {

        event->mYx = event->mYInc;
        event->mYy = event->mYDec;
        event->mYz = event->mYInt;

        event->mSy = sqrt( (pow(event->mSInt, 2.) + pow(event->mSInc, 2.)) /2.);


    }

    if (!mChronocurveSettings.mUseErrMesure){
        event->mSy = 0.;
    }
}
*/
#pragma mark Related to : update : calcul de h_new

/**
 * Calcul de h_YWI_AY pour toutes les composantes de Y event (suivant la configuration univarié, spérique ou vectoriel)
 */
long double MCMCLoopChronocurve::h_YWI_AY(const SplineMatrices& matrices, const QList<Event *> &lEvents, const double alphaLissage, const std::vector<long double>& vecH)
{
     long double h = h_YWI_AY_composanteX (matrices, lEvents, alphaLissage, vecH);
    
    if (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique) {

        h *= h_YWI_AY_composanteY (matrices, lEvents, alphaLissage, vecH);
        h *= h_YWI_AY_composanteZ (matrices, lEvents, alphaLissage, vecH);

    } else  if (mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel) {

        h *= h_YWI_AY_composanteY(matrices, lEvents, alphaLissage, vecH);
        h *= h_YWI_AY_composanteZ(matrices, lEvents, alphaLissage, vecH);
    }
    return h;
}


long double MCMCLoopChronocurve::h_YWI_AY_composanteX(const SplineMatrices &matrices, const QList<Event *> lEvents, const double alphaLissage, const std::vector<long double>& vecH)
{
    if (alphaLissage == 0.) { // Attention double == 0
        return 1.;
    }
    errno = 0;
      if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);

    std::vector<long double> vecY (mModel->mEvents.size());
    std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYx;});

    SplineResults spline = calculSpline(matrices, vecY, alphaLissage, vecH);
    std::vector<long double>& vecG = spline.vecG;

    std::vector<long double>& matD = spline.matD;
    const std::vector<std::vector<long double>>& matQTQ = matrices.matQTQ;

    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y
    long double YWY = 0.;
    long double YWAY = 0.;

    const int nb_noeuds = lEvents.size();

    int i = 0;
    for (auto& e : lEvents) {
        YWY  += e->mW * e->mYx * e->mYx;
        YWAY += e->mW * e->mYx * vecG.at(i);
        ++i;
    }

    long double h_exp = -0.5 * (YWY-YWAY);

    /* -------------------------------------------
    * Calcul de la norme
    * -------------------------------------------
    * Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    * (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    * ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    * inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ
    */

    std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matQTQ, 5, 1);

    std::vector<long double>& matDq = decomp.second;

    long double det_1_2 = 1.;
    for (int i = 1; i < nb_noeuds-1; ++i) {
        det_1_2 *= (matDq.at(i)/ matD.at(i));
    }

    // calcul à un facteur (2*PI) puissance -(n-2) près
    long double res = 0.5 * (nb_noeuds-2.) * log(alphaLissage) + h_exp;
    res = exp(res) * sqrt(det_1_2);
#ifdef DEBUG
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"errno set to EDOM";
      }
      if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"MCMCLoopChronocurve::h_YWI_AY_composanteX -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
      }
    //return exp(0.5 * (nb_noeuds-2.) * log(alphaLissage) + h_exp) * sqrt(det_1_2);
    if (res == 0) {
        qWarning("h_YWI_AY_composanteX res == 0");
    }
    if (res == HUGE_VAL) {
        qWarning("h_YWI_AY_composanteX res == HUGE_VAL");
    }
#endif
    return res;
}

long double MCMCLoopChronocurve::h_YWI_AY_composanteY(const SplineMatrices& matrices, const QList<Event *> lEvents, const double alphaLissage, const std::vector<long double>& vecH)
{
    if (alphaLissage == 0.) { // Attention double == 0
        return 1.;
    }
    errno = 0;
      if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);

    std::vector<long double> vecY (mModel->mEvents.size());
    std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYy;});

    SplineResults spline = calculSpline(matrices, vecY, alphaLissage, vecH);
    std::vector<long double>& vecG = spline.vecG;

    std::vector<long double>& matD = spline.matD;
    const std::vector<std::vector<long double>>& matQTQ = matrices.matQTQ;

    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y
    long double YWY = 0.;
    long double YWAY = 0.;

    const int nb_noeuds = lEvents.size();

    int i = 0;
    for (auto& e : lEvents) {
        YWY += e->mW * e->mYy * e->mYy;
        YWAY += e->mYy * e->mW * vecG.at(i);
        ++i;
    }

    long double h_exp = -0.5 * (YWY-YWAY);

    /* -------------------------------------------
    * Calcul de la norme
    * -------------------------------------------
    * Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    * (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    * ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    * inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ */

    std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matQTQ, 5, 1);
   // std::vector<std::vector<double>> matLq = decomp.first;
    std::vector<long double>& matDq = decomp.second;

    long double det_1_2 = 1.;
    for (int i = 1; i < nb_noeuds-1; ++i) {
        det_1_2 *= matDq.at(i)/ matD.at(i);
    }

    // calcul à un facteur (2*PI) puissance -(n-2) près
    long double res = 0.5l * (nb_noeuds-2.) * log(alphaLissage) + h_exp;
    res = exp(res) * sqrt(det_1_2);
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"errno set to EDOM";
      }
      if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"MCMCLoopChronocurve::h_YWI_AY_composanteY -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
      }
    //return exp(0.5 * (nb_noeuds-2.) * log(alphaLissage) + h_exp) * sqrt(det_1_2);
    return res;
}
long double MCMCLoopChronocurve::h_YWI_AY_composanteZ(const SplineMatrices& matrices, const QList<Event *> lEvents, const double alphaLissage, const std::vector<long double>& vecH)
{
    if (alphaLissage == 0.) { // Attention double == 0
        return 1.;
    }
    errno = 0;
      if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);

    std::vector<long double> vecY (mModel->mEvents.size());
    std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYz;});

    SplineResults spline = calculSpline(matrices, vecY, alphaLissage, vecH);
    std::vector<long double>& vecG = spline.vecG;
    //std::vector<double>& vecGamma = spline.vecGamma;
  //  std::vector<std::vector<double>> matL = spline.matL;
    std::vector<long double>& matD = spline.matD;
    const std::vector<std::vector<long double>>& matQTQ = matrices.matQTQ;

    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y
    long double YWY = 0.;
    long double YWAY = 0.;

    const int nb_noeuds = lEvents.size();

    int i = 0;
    for (auto& e : lEvents) {
        YWY += e->mW * e->mYz * e->mYz;
        YWAY += e->mYz * e->mW * vecG.at(i);
        ++i;
    }

    long double h_exp = -0.5 * (YWY-YWAY);

    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matQTQ, 5, 1);
   // std::vector<std::vector<double>> matLq = decomp.first;
    std::vector<long double>& matDq = decomp.second;

    long double det_1_2 = 1.;
    for (int i = 1; i < nb_noeuds-1; ++i) {
        det_1_2 *= matDq.at(i)/ matD.at(i);
    }

    // calcul à un facteur (2*PI) puissance -(n-2) près
    long double res = 0.5l * (nb_noeuds-2.) * log(alphaLissage) + h_exp;
    res = exp(res) * sqrt(det_1_2);
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"errno set to EDOM";
      }
      if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"MCMCLoopChronocurve::h_YWI_AY_composanteZ -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
      }
    //return exp(0.5 * (nb_noeuds-2.) * log(alphaLissage) + h_exp) * sqrt(det_1_2);
    return res;
}


long double MCMCLoopChronocurve::h_alpha(const SplineMatrices& matrices, const int nb_noeuds, const double & alphaLissage)
{
    const std::vector<long double>& diagWInv = matrices.diagWInv;
    const std::vector<std::vector<long double>>& matR = matrices.matR;
    const std::vector<std::vector<long double>>& matQ = matrices.matQ;
    const std::vector<std::vector<long double>>& matQT = matrices.matQT;
    
    // La trace de la matrice produit W_1.K est égal à la somme des valeurs propores si W_1.K est symétrique,
    // ce qui implique que W_1 doit être une constante
    // d'où on remplace W_1 par la matrice W_1m moyenne des (W_1)ii

    long double W_1m = 0.;
    W_1m = std::accumulate(diagWInv.begin(), diagWInv.begin() + nb_noeuds, 0.);
    W_1m /= nb_noeuds;

    // calcul des termes diagonaux de W_1.K
    std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matR, 3, 1);
  /*  if (determinant(matR) == 0) {
        qDebug() << "cholesky impossible matrice non inversible";
        throw "error cholesky noninversibler matrix";
    }


    std::pair<std::vector<std::vector<double>>, std::vector<double>> decomp = choleskyLDL(matR); // ne peut pas être appliqué directemenent car décalage de zéro
*/
    std::vector<std::vector<long double>>& matL = decomp.first;
    std::vector<long double>& matD = decomp.second;
    
    std::vector<std::vector<long double>> matRInv = inverseMatSym(matL, matD, 5, 1);// ne fait pas une inversion de matrice
    
    std::vector<std::vector<long double>> tmp = multiMatParMat(matQ, matRInv, 3, 3);
    std::vector<std::vector<long double>> matK = multiMatParMat(tmp, matQT, 3, 3);

    long double vm = 0.;
    // pHd : Si la bande vaut 1, on prend donc la diagonale de matK, le calcul se simpifie
    /*
     * std::vector<double> diag_W_1m (nb_noeuds, W_1m);//= initVecteur(nb_noeuds);
     * std::vector<std::vector<double>> matWInvK = multiDiagParMat(diag_W_1m, matK, 1);

    double vm = 0.;
    for (int i = 0; i < nb_noeuds; ++i) {
        vm += matWInvK[i][i];
    }
    */
    for (int i = 0; i < nb_noeuds; ++i)
        vm += W_1m * matK.at(i).at(i);

    long double c = (nb_noeuds-2) / vm;

    // initialisation de l'exposant mu du prior "shrinkage" sur alpha : fixe
    // en posant mu=2, on la moyenne a priori sur alpha finie = (nb_noeuds-2)/somme(Mat_W_1K[i,i]) ;
    // et la variance a priori sur alpha est infinie
    // si on veut un shrinkage avec espérance et variance finies, alors mu > 2
    long double mu = 2.;
    
    // prior "shrinkage"
    long double r;
    if (isinf(c))   // pHd à controler !!
        r = mu / (nb_noeuds-2);
    else
        r = (mu/c) * pow(c/(c + alphaLissage), mu+1);

    return r;
}


/* ancienne fonction U_cmt_MCMC:: h_Vgij dans RenCurve
 */
long double MCMCLoopChronocurve::h_VG(const QList<Event *> lEvents)
{
    // Densité a priori sur variance de type "shrinkage" avec paramètre S02
    // bool_shrinkage_uniforme:=true;

    long double shrink_VG;
    
    if (mChronocurveSettings.mVarianceType == ChronocurveSettings::eModeFixed) {
        shrink_VG = mChronocurveSettings.mVarianceFixed;

    } else {
        
        const int nb_noeuds = lEvents.size();
        
        if (mChronocurveSettings.mUseVarianceIndividual) {
            
            shrink_VG = 1.;
            long double S02;
            for (Event* e :lEvents) {
                if (e->type() == Event::eDefault) {
                    S02 = pow(e->mSy, 2.);
                    shrink_VG *= (S02 / pow(S02 + e->mVG.mX, 2.));

                } else {
                    EventKnown* ek = static_cast<EventKnown *>(e);
                    S02 = pow(ek->mSy, 2.);
                    shrink_VG *= (S02 / pow(S02 + ek->mVG.mX, 2.));
                }
            }
            
        } else {
            
            // S02 : moyenne harmonique des erreurs sur Y
            long double som_inv_S02 = 0.;
            
            for (Event* e :lEvents) {
                som_inv_S02 += (1. / (e->mSy * e->mSy));
            }
            long double S02 = nb_noeuds/som_inv_S02;
            // Shrinkage avec a = 1
            
            shrink_VG = S02 / pow(S02 + lEvents.at(0)->mVG.mX, 2.);
        }
    }
    
    return shrink_VG;
}

/**
 * Les calculs sont faits avec les dates (theta event, ti dates, delta, sigma) exprimées en années.
 * Lors de l'appel de cette fonction, theta event a été réduit (voir les appels plus haut).
 * Il faut donc soit réduire les autres variables (plus coûteux en calculs), soit repasser theta event en années.
*/
// voir U-cmt_MCMC ligne 105 calcul_h
long double MCMCLoopChronocurve::h_theta(QList<Event *> lEvents)
{

    long double h = 1.;
    long double p, t_moy, h1;

    for (Event*& e : lEvents) {
        if (e->mType == Event::eDefault) {
            p = 0.;
            t_moy = 0.;
            for (auto&& date : e->mDates) {
                long double pi = 1. / pow(date.mSigma.mX, 2.);
                p += pi;
                t_moy += (date.mTheta.mX + date.mDelta) * pi;
            }
            t_moy /= p;

            //h *= exp(-0.5 * p * pow((tmin + e->mTheta.mX * (tmax - tmin)) - t_moy, 2.));
            // h *= exp(-0.5 * p * pow( e->mTheta.mX  - t_moy, 2.));
            h1 = exp(-0.5 * p * pow( (long double)(e->mTheta.mX  - t_moy), 2.));
 #ifdef DEBUG
            if (h1 == 0.) {
                qWarning( "MCMCLoopChronocurve::h_theta() h1 == 0");
            }
 #endif
            h *= h1;
        }
    }
#ifdef DEBUG
    if (h == 0.) {
        qWarning( "MCMCLoopChronocurve::h_theta() h == 0");
    }
#endif
    return h;
}

#pragma mark Manipulation des theta event pour le calcul spline (equivalent "Do Cravate")

/**
 * Cette fonction est l'ancien do_cravate() qui portait mal son nom car nous ne faisons pas de cravates !
 * Une cravate correspondait à des theta tellement proches qu'on "fusionnait" les events correspondants pour les calculs.
 * Or, reorderEventsByTheta a pour rôle d'introduire un espace minimal entre chaque theta pour justement ne pas avoir de cravate.
 * Cette fonction ne fait donc que retourner le résultat de definitionNoeuds dans un format pratique pour la suite des calculs
*/
void MCMCLoopChronocurve::orderEventsByTheta(QList<Event *> & lEvent)
{
    /* On manipule directement la liste des évènements
       Ici on peut utiliser lEvent en le déclarant comme copy ??
    */
    QList<Event*>& result = lEvent;
    
    std::sort(result.begin(), result.end(), [](const Event* a, const Event* b) { return (a->mTheta.mX < b->mTheta.mX); });
}

void MCMCLoopChronocurve::orderEventsByThetaReduced(QList<Event *>& lEvent)
{
    // On manipule directement la liste des évènements
    // Ici on peut utiliser lEvent en le déclarant comme copy ??
    QList<Event*>& result = lEvent;

    std::sort(result.begin(), result.end(), [](const Event* a, const Event* b) { return (a->mThetaReduced < b->mThetaReduced); });
}

void MCMCLoopChronocurve::saveEventsTheta(QList<Event *>& lEvent)
{
    mThetasMemo.clear();
    for (Event*& e : lEvent) {
        mThetasMemo.insert(std::pair<int, double>(e->mId, e->mTheta.mX));
    }
}

void MCMCLoopChronocurve::restoreEventsTheta(QList<Event *>& lEvent)
{
    for (Event*& e : lEvent) {
        e->mTheta.mX = mThetasMemo.at(e->mId);
    }
}

/**
 * @brief MCMCLoopChronocurve::minimalThetaDifference, if theta are sort, the result is positive
 * @param lEvents
 * @return
 */
long double MCMCLoopChronocurve::minimalThetaDifference(QList<Event *>& lEvents)
{
    std::vector<long double> result (lEvents.size());
    std::transform (lEvents.begin(), lEvents.end()-1, lEvents.begin()+1, result.begin(), [](const Event* e0, const  Event* e1) {return (long double)(e1->mTheta.mX - e0->mTheta.mX); });
   // result.erase(result.begin()); // the firs value is not a difference, it's just the first value of LEvents
    std::sort(result.begin(), result.end());
    return std::move(*std::find_if_not (result.begin(), result.end(), [](long double v){return v==0.;} ));
}

long double MCMCLoopChronocurve::minimalThetaReducedDifference(QList<Event *>& lEvents)
{
    std::vector<long double> result (lEvents.size());
    std::transform (lEvents.begin(), lEvents.end()-1, lEvents.begin()+1, result.begin(), [](const Event* e0, const  Event* e1) {return (long double)(e1->mThetaReduced - e0->mThetaReduced); });
   // result.erase(result.begin()); // the firs value is not a difference, it's just the first value of LEvents
    std::sort(result.begin(), result.end());
    return std::move(*std::find_if_not (result.begin(), result.end(), [](long double v){return v==0.;} ));
}

// not used
void MCMCLoopChronocurve::spreadEventsTheta(QList<Event *>& lEvent, double minStep)
{
    // On manipule directement la liste des évènements
    QList<Event*>& result = lEvent;
    
    // Espacement possible ?
    const int count = result.size();
    double firstValue = result.at(0)->mTheta.mX;
    double lastValue = result.at(count - 1)->mTheta.mX;
    if ((lastValue - firstValue) < (count - 1) * minStep) {
        throw tr("Not enought span between events theta");
    }
    
    // Il faut au moins 3 points
    if (count < 3) {
        throw tr("3 events minimum required");
    }

    // 0 veut dire qu'on n'a pas détecté d'égalité :
    int startIndex = 0;
    int endIndex = 0;
    
    for (int i = 1; i < count; ++i) {
        double value = result.at(i)->mTheta.mX;
        double lastValue = result.at(i - 1)->mTheta.mX;
        
        // Si l'écart n'est pas suffisant entre la valeur courante et la précedente,
        // alors on mémorise l'index précédent comme le début d'une égalité
        // (à condition de ne pas être déjà dans une égalité)
        if ((value - lastValue < minStep) && (startIndex == 0)) {
            // La valeur à l'index 0 ne pourra pas être déplacée vers la gauche !
            // S'il y a égalité dès le départ, on considère qu'elle commence à l'index 1.
            startIndex = (i == 1) ? 1 : (i-1);
        }
        
        //qDebug() << "i = " << i << " | value = " << value << " | lastValue = " << lastValue << " | startIndex = " << startIndex;
        
        // Si on est à la fin du tableau et dans un cas d'égalité,
        // alors on s'assure d'avoir suffisamment d'espace disponible
        // en incluant autant de points précédents que nécessaire dans l'égalité.
        if ((i == count - 1) && (startIndex != 0)) {
            endIndex = i-1;
            double delta;
            double deltaMin;
            for (int j = startIndex; j >= 1; j--) {
                delta = value - result.at(j-1)->mTheta.mX;
                deltaMin = minStep * (i - j + 1);
                if (delta >= deltaMin) {
                    startIndex = j;
                    qDebug() << "=> Egalité finale | startIndex = " << startIndex << " | endIndex = " << endIndex;
                    break;
                }
            }
        }
        
        // Si l'écart entre la valeur courante et la précédente est suffisant
        // ET que l'on était dans un cas d'égalité (pour les valeurs précédentes),
        // alors il se peut qu'on ait la place de les espacer.
        if ((value - lastValue >= minStep) && (startIndex != 0)) {
            double startValue = result.at(startIndex-1)->mTheta.mX;
            double delta = (value - startValue);
            double deltaMin = minStep * (i - startIndex + 1);
            
            //qDebug() << "=> Vérification de l'espace disponible | delta = " << delta << " | deltaMin = " << deltaMin;
            
            if (delta >= deltaMin) {
                endIndex = i-1;
            }
        }
        
        if (endIndex != 0) {
            //qDebug() << "=> On espace les valeurs entre les bornes " << result[startIndex - 1]->mTheta.mX << " et " << result[i]->mTheta.mX;
            
            // On a la place d'espacer les valeurs !
            // - La borne inférieure ne peut pas bouger (en startIndex-1)
            // - La borne supérieure ne peut pas bouger (en endIndex)
            // => On espace les valeurs intermédiaires (de startIndex à endIndex-1) du minimum nécessaire
            double startSpread = result.at(endIndex) - result.at(startIndex);

            for (int j = startIndex; j <= endIndex; j++) {
                if ((result.at(j)->mTheta.mX - result.at(j-1)->mTheta.mX) < minStep) {
                    result.at(j)->mTheta.mX = result.at(j-1)->mTheta.mX + minStep;
                }
            }
            // En espaçant les valeurs vers la droite, on a "décentré" l'égalité.
            // => On redécale tous les points de l'égalité vers la gauche pour les recentrer :
            double endSpread = result.at(endIndex)->mTheta.mX - result.at(startIndex)->mTheta.mX;
            double shiftBack = (endSpread - startSpread) / 2.;
            
            // => On doit prendre garde à ne pas trop se rappocher le la borne de gauche :
            if ((result.at(startIndex)->mTheta.mX - shiftBack) - result.at(startIndex-1)->mTheta.mX < minStep) {
                shiftBack = result.at(startIndex)->mTheta.mX - (result.at(startIndex-1)->mTheta.mX + minStep);
            }
            
            // On doit décaler suffisamment vers la gauche pour ne pas être trop près de la borne de droite :
            if (result.at(endIndex + 1)->mTheta.mX - (result.at(endIndex)->mTheta.mX - shiftBack) < minStep) {
                shiftBack = result.at(endIndex)->mTheta.mX - (result.at(endIndex + 1)->mTheta.mX - minStep);
            }

            //for (int j = startIndex; j <= endIndex; j++) {
              //   result[j]->mTheta.mX -= shiftBack;
            for (auto r = result.begin() + startIndex; r != result.begin() + endIndex; ++r) {
                (*r)->mTheta.mX -= shiftBack;
            }
            
            // On marque la fin de l'égalité
            startIndex = 0;
            endIndex = 0;
        }
    }
}

void MCMCLoopChronocurve::spreadEventsThetaReduced(QList<Event *> &lEvent, double minStep)
{
    // On manipule directement la liste des évènements
    QList<Event*>& result = lEvent;

    // Espacement possible ?
    const int count = result.size();
    double firstValue = result.at(0)->mThetaReduced;
    double lastValue = result.at(count - 1)->mThetaReduced;
    if ((lastValue - firstValue) < (count - 1) * minStep) {
        throw tr("Not enought span between events theta");
    }

    // Il faut au moins 3 points
    if (count < 3) {
        throw tr("3 events minimum required");
    }

    // 0 veut dire qu'on n'a pas détecté d'égalité :
    int startIndex = 0;
    int endIndex = 0;

    for (int i = 1; i < count; ++i) {
        double value = result.at(i)->mThetaReduced;
        double lastValue = result.at(i - 1)->mThetaReduced;

        // Si l'écart n'est pas suffisant entre la valeur courante et la précedente,
        // alors on mémorise l'index précédent comme le début d'une égalité
        // (à condition de ne pas être déjà dans une égalité)
        if ((value - lastValue < minStep) && (startIndex == 0)) {
            // La valeur à l'index 0 ne pourra pas être déplacée vers la gauche !
            // S'il y a égalité dès le départ, on considère qu'elle commence à l'index 1.
            startIndex = (i == 1) ? 1 : (i-1);
        }

        //qDebug() << "i = " << i << " | value = " << value << " | lastValue = " << lastValue << " | startIndex = " << startIndex;

        // Si on est à la fin du tableau et dans un cas d'égalité,
        // alors on s'assure d'avoir suffisamment d'espace disponible
        // en incluant autant de points précédents que nécessaire dans l'égalité.
        if ((i == count - 1) && (startIndex != 0)) {
            endIndex = i-1;
            double delta;
            double deltaMin;
            for (int j = startIndex; j >= 1; j--) {
                delta = value - result.at(j-1)->mThetaReduced;
                deltaMin = minStep * (i - j + 1);
                if (delta >= deltaMin) {
                    startIndex = j;
                    qDebug() << "=> Egalité finale | startIndex = " << startIndex << " | endIndex = " << endIndex;
                    break;
                }
            }
        }

        // Si l'écart entre la valeur courante et la précédente est suffisant
        // ET que l'on était dans un cas d'égalité (pour les valeurs précédentes),
        // alors il se peut qu'on ait la place de les espacer.
        if ((value - lastValue >= minStep) && (startIndex != 0)) {
            double startValue = result.at(startIndex-1)->mThetaReduced;
            double delta = (value - startValue);
            double deltaMin = minStep * (i - startIndex + 1);

            //qDebug() << "=> Vérification de l'espace disponible | delta = " << delta << " | deltaMin = " << deltaMin;

            if (delta >= deltaMin) {
                endIndex = i-1;
            }
        }

        if (endIndex != 0) {
            //qDebug() << "=> On espace les valeurs entre les bornes " << result[startIndex - 1]->mTheta.mX << " et " << result[i]->mTheta.mX;

            // On a la place d'espacer les valeurs !
            // - La borne inférieure ne peut pas bouger (en startIndex-1)
            // - La borne supérieure ne peut pas bouger (en endIndex)
            // => On espace les valeurs intermédiaires (de startIndex à endIndex-1) du minimum nécessaire
            double startSpread = result.at(endIndex) - result.at(startIndex);

            for (int j = startIndex; j <= endIndex; j++) {
                if ((result.at(j)->mThetaReduced - result.at(j-1)->mThetaReduced) < minStep) {
                    result.at(j)->mThetaReduced = result.at(j-1)->mThetaReduced + minStep;
                }
            }
            // En espaçant les valeurs vers la droite, on a "décentré" l'égalité.
            // => On redécale tous les points de l'égalité vers la gauche pour les recentrer :
            double endSpread = result.at(endIndex)->mThetaReduced - result.at(startIndex)->mThetaReduced;
            double shiftBack = (endSpread - startSpread) / 2.;

            // => On doit prendre garde à ne pas trop se rappocher le la borne de gauche :
            if ((result.at(startIndex)->mThetaReduced - shiftBack) - result.at(startIndex-1)->mThetaReduced < minStep) {
                shiftBack = result.at(startIndex)->mThetaReduced - (result.at(startIndex-1)->mThetaReduced + minStep);
            }

            // On doit décaler suffisamment vers la gauche pour ne pas être trop près de la borne de droite :
            if (result.at(endIndex + 1)->mThetaReduced - (result.at(endIndex)->mThetaReduced - shiftBack) < minStep) {
                shiftBack = result.at(endIndex)->mThetaReduced - (result.at(endIndex + 1)->mThetaReduced - minStep);
            }

            //for (int j = startIndex; j <= endIndex; j++) {
              //   result[j]->mTheta.mX -= shiftBack;
            for (auto r = result.begin() + startIndex; r != result.begin() + endIndex; ++r) {
                (*r)->mThetaReduced -= shiftBack;
            }

            // On marque la fin de l'égalité
            startIndex = 0;
            endIndex = 0;
        }
    }
}

// Les Events sont considèrés triés dans l'ordre croissant
/**
 * @brief MCMCLoopChronocurve::spreadEventsThetaReduced0
 * @param sortedEvents
 * @param minStep  é
 */
void MCMCLoopChronocurve::spreadEventsThetaReduced0(QList<Event *> &sortedEvents, double spreadSpan)
{
    QList<Event*>::iterator itEvenFirst = sortedEvents.end();
    QList<Event*>::iterator itEventLast = sortedEvents.end();
    unsigned nbEgal = 0;
    //double precision;

    if (spreadSpan == 0.) {
        spreadSpan = std::numeric_limits<double>::epsilon() * 1.E6;// epsilon = 1E-16

    }

    // repère première egalité
    for (QList<Event*>::iterator itEvent = sortedEvents.begin(); itEvent != sortedEvents.end() -1; itEvent++) {

        if ((*itEvent)->mThetaReduced == (*(itEvent+1))->mThetaReduced) {
            if (itEvenFirst == sortedEvents.end()) {
                itEvenFirst = itEvent;
                itEventLast = itEvent + 1;
                nbEgal = 2;
            } else {
                itEventLast = itEvent + 1;
                ++nbEgal;
            }

        } else {
            if (itEvenFirst != sortedEvents.end()) {
                // on sort d'une égalité, il faut répartir les dates entre les bornes
                // itEvent == itEventLast
                double lowBound = itEvenFirst == sortedEvents.begin() ? sortedEvents.first()->mThetaReduced : (*(itEvenFirst -1))->mThetaReduced ; //valeur à gauche non égale
                double upBound = itEvent == sortedEvents.end()-2 ? sortedEvents.last()->mThetaReduced : (*(itEvent + 1))->mThetaReduced;

                double step = spreadSpan / (nbEgal-1); // écart théorique
                double min;

                // Controle du debordement sur les valeurs encadrantes
                if (itEvenFirst == sortedEvents.begin()) {
                    // Cas de l'égalité avec la première valeur de la liste
                    // Donc ous les Events sont à droite de la première valeur de la liste
                    min = (*itEvent)->mThetaReduced;

                } else {
                    // On essaie de placer une moitier des Events à gauche et l'autre moitier à droite
                    min = (*itEvent)->mThetaReduced - step*floor(nbEgal/2.);
                    // controle du debordement sur les valeurs encadrantes
                    min = std::max(lowBound + step, min );
                }

                double max = std::min(upBound - spreadSpan, (*itEvent)->mThetaReduced + step*ceil(nbEgal/2.) );
                step = (max- min)/ (nbEgal - 1); // écart corrigé

                QList<Event*>::iterator itEventEgal;
                int count;
                for (itEventEgal = itEvenFirst, count = 0; itEventEgal != itEvent+1; itEventEgal++, count++ ) {
                    (*itEventEgal)->mThetaReduced = min + count*step;
                }
                // Fin correction, pres pour nouveau groupe/cravate
                itEvenFirst = sortedEvents.end();

            }
        }


    }

    // sortie de la boucle avec itFirst validé donc itEventLast == sortedEvents.end()-1

    if (itEvenFirst != sortedEvents.end()) {
        // On sort de la boucle et d'une égalité, il faut répartir les dates entre les bornes
        // itEvent == itEventLast
        double lowBound = (*(itEvenFirst -1))->mThetaReduced ; //la première valeur à gauche non égale

        double value = (*(sortedEvents.end()-1))->mThetaReduced;
        double step = spreadSpan / (nbEgal-1.); // ecart théorique

        double min = std::max(lowBound + spreadSpan, value - step*(nbEgal-1) );
        double max = value;
        step = (max- min)/ (nbEgal-1); // écart corrigé

        // Tout est réparti à gauche
        int count;
        QList<Event*>::iterator itEventEgal;
        for (itEventEgal = itEvenFirst, count = 0; itEventEgal != sortedEvents.end(); itEventEgal++, count++ ) {
            (*itEventEgal)->mThetaReduced = min + count *step;
        }

    }

}

void MCMCLoopChronocurve::reduceEventsTheta(QList<Event *> &lEvent)
{
    for (auto&& e : lEvent)
        e->mTheta.mX = reduceTime( e->mTheta.mX );

}

double MCMCLoopChronocurve::reduceTime(double t)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    return (t - tmin) / (tmax - tmin);
}


#pragma mark Pratique pour debug

std::vector<long double> MCMCLoopChronocurve::getThetaEventVector(const QList<Event *> &lEvent)
{
    std::vector<long double> vecT(lEvent.size());
   /* for (int i = 0; i < lEvent.size(); ++i) {
        vecT.push_back(lEvent[i]->mTheta.mX);
    }*/
    std::transform(lEvent.begin(), lEvent.end(), vecT.begin(), [](Event* ev) {return ev->mTheta.mX;});

    return vecT;
}

std::vector<long double> MCMCLoopChronocurve::getYEventVector(const QList<Event*>& lEvent)
{
    std::vector<long double> vecY;
   /* for (int i = 0; i < lEvent.size(); ++i) {
        vecY.push_back(lEvent[i]->mY);
    }*/
    std::transform(lEvent.begin(), lEvent.end(), vecY.begin(), [](Event* ev) {return ev->mY;});

    return vecY;
}

#pragma mark Calcul Spline

// dans RenCurve procedure Calcul_Mat_Q_Qt_R ligne 66; doit donner une matrice symetrique; sinon erreur dans cholesky
std::vector<std::vector<long double>> MCMCLoopChronocurve::calculMatR(std::vector<long double>& vecH)
{
    // Calcul de la matrice R, de dimension (n-2) x (n-2) contenue dans une matrice n x n
    // Par exemple pour n = 5 :
    // 0 0 0 0 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 0 0 0 0
    
    // vecH est de dimension n-1
    const int n = vecH.size()+1;
    
    // matR est de dimension n-2 x n-2, mais contenue dans une matrice nxn
    std::vector<std::vector<long double>> matR = initLongMatrix(n, n);
    // On parcourt n-2 valeurs :
    for (int i = 1; i < n-1; ++i) {
        matR[i][i] = (vecH[i-1] + vecH[i]) / 3.;
        // Si on est en n-2 (dernière itération), on ne calcule pas les valeurs de part et d'autre de la diagonale (termes symétriques)
        if (i < n-2) {
            matR[i][i+1] = vecH[i] / 6.;
            matR[i+1][i] = vecH[i] / 6.;
        }
    }

    // pHd : ici la vrai forme est une matrice de bande k=1
    // Par exemple pour n = 5 :
    // 0 0 0 0 0
    // 0 X a 0 0
    // 0 a X b 0
    // 0 0 b X 0
    // 0 0 0 0 0

    // Si on est en n-2 (dernière itération), on ne calcule pas les valeurs de part et d'autre de la diagonale (termes symétriques)
 /*    for (int i = 1; i < n-2; ++i) {
        matR[i][i] = (vecH[i-1] + vecH[i]) / 3.;
        matR[i][i+1] = vecH[i] / 6.;
        matR[i+1][i] = matR.at(i).at(i+1);
    }
    matR[n-2][n-2] = (vecH[n-2-1] + vecH[n-2]) / 3.;
*/
    return matR;
}

// dans RenCurve procedure Calcul_Mat_Q_Qt_R ligne 55
std::vector<std::vector<long double>> MCMCLoopChronocurve::calculMatQ(std::vector<long double>& vecH)
{
    // Calcul de la matrice Q, de dimension n x (n-2) contenue dans une matrice n x n
    // Les 1ère et dernière colonnes sont nulles
    // Par exemple pour n = 5 :
    // 0 X 0 0 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 0 0 X 0
    
    // vecH est de dimension n-1
    const unsigned n = vecH.size()+1;
    
    // matQ est de dimension n x n-2, mais contenue dans une matrice nxn
    std::vector<std::vector<long double>> matQ = initLongMatrix(n, n);
    // On parcourt n-2 valeurs :
    for (unsigned i = 1; i < n-1; ++i) {
        matQ[i-1][i] = 1. / vecH.at(i-1);
        matQ[i][i] = -((1./vecH.at(i-1)) + (1./vecH.at(i)));
        matQ[i+1][i] = 1. / vecH.at(i);
#ifdef DEBUG
        if (vecH.at(i)<=0)
            throw "calculMatQ vecH <=0 ";
#endif
    }
    // pHd : ici la vrai forme est une matrice de dimension n x (n-2), de bande k=1; les termes diagonaux sont négatifs
     // Les 1ère et dernière colonnes sont nulles
    // Par exemple pour n = 5 :
    // 0 +X  0  0 0
    // 0 -X +a  0 0
    // 0 +a -X +b 0
    // 0  0 +b -X 0
    // 0  0  0 +X 0

    return matQ;
}


double diffX (Event* e0, Event*e1) {return (e1->mThetaReduced - e0->mThetaReduced);}

// An other function exist for vector ChronocurveUtilities::calculVecH(const vector<double>& vec)
std::vector<long double> MCMCLoopChronocurve::calculVecH(const QList<Event *> &lEvent)
{
    std::vector<long double> result (lEvent.size()-1);
    std::transform(lEvent.begin(), lEvent.end()-1, lEvent.begin()+1 , result.begin(), diffX);
#ifdef DEBUG
    int i =0;
    for (auto &&r :result) {
        if (r <= 0.) {
            char th [200];
            //char num [] ;
            sprintf(th, "MCMCLoopChronocurve::calculVecH diff Theta null %.2lf et %.2lf", lEvent.at(i)->mThetaReduced, lEvent.at(i+1)->mThetaReduced);
            throw th ;
        }
        ++i;
    }
#endif
    return result;
}


/**
 * La création de la matrice diagonale des erreurs est nécessaire à chaque mise à jour de :
 * - Theta event : qui peut engendrer un nouvel ordonnancement des events (definitionNoeuds)
 * - VG event : qui intervient directement dans le calcul de W
 */
std::vector<long double> MCMCLoopChronocurve::createDiagWInv(const QList<Event*>& lEvents)
{
    std::vector<long double> diagWInv (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), diagWInv.begin(), [](Event* ev) {return ev->mWInv;});

    return diagWInv;
}

/**
 * @brief MCMCLoopChronocurve::prepareCalculSpline
 * produit
 * SplineMatrices matrices;
 *  matrices.diagWInv
 *  matrices.matR
 *  matrices.matQ
 *  matrices.matQT
 *  matrices.matQTW_1Q  // Seule affectée par changement de VG
 *  matrices.matQTQ
 *
 * @param sortedEvents
 * @return
 */
SplineMatrices MCMCLoopChronocurve::prepareCalculSpline(const QList<Event *>& sortedEvents, std::vector<long double>& vecH)
{
   /* for (int i=0 ; i < sortedEvents.size();  i++) {
        qDebug()<<" prepareCalculSpline sortedEvents" <<i << " "<<sortedEvents[i]->mThetaReduced <<sortedEvents[i]->mName;
    }
    */
    //std::vector<long double> vecH = calculVecH(sortedEvents);
    std::vector<std::vector<long double>> matR = calculMatR(vecH);
    std::vector<std::vector<long double>> matQ = calculMatQ(vecH);
    
    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    std::vector<std::vector<long double>> matQT = transpose(matQ, 3);
  //  std::vector<std::vector<long double>> matQTControle = transpose0(matQ);

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    std::vector<long double> diagWInv = createDiagWInv(sortedEvents);
    std::vector<std::vector<long double>> tmp = multiMatParDiag(matQT, diagWInv, 3);
    std::vector<std::vector<long double>> matQTW_1Q = multiMatParMat(tmp, matQ, 3, 3);

    //Strassen Stra;

  //  std::vector<std::vector<long double>> matQTW_1QControle = multiMatParMat0(tmp, matQ);


    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    std::vector<std::vector<long double>> matQTQ = multiMatParMat(matQT, matQ, 3, 3);
  //  std::vector<std::vector<long double>> matQTQControle = multiMatParMat0(matQT, matQ);
    
    SplineMatrices matrices;
    matrices.diagWInv = std::move(diagWInv);
    matrices.matR = std::move(matR);
    matrices.matQ = std::move(matQ);
    matrices.matQT = std::move(matQT);
    matrices.matQTW_1Q = std::move(matQTW_1Q); // Seule affectée par changement de VG
    matrices.matQTQ = std::move(matQTQ);
    
    return matrices;
}


/**
 * @brief MCMCLoopChronocurve::calculSpline
 * fabrication de spline avec
 *      spline.vecG
 *      spline.vecGamma
 *      spline.matB
 *      spline.matL
 *      spline.matD
 * @param matrices
 * @param vecY
 * @return
 */
SplineResults MCMCLoopChronocurve::calculSpline(const SplineMatrices& matrices, const std::vector<long double>& vecY, const double alpha, const std::vector<long double>& vecH)
{
    SplineResults spline;
    const std::vector<std::vector<long double>>& matR = matrices.matR;
    const std::vector<std::vector<long double>>& matQ = matrices.matQ;
    const std::vector<std::vector<long double>>& matQTW_1Q = matrices.matQTW_1Q;

    try {
        // calcul de: R + alpha * Qt * W-1 * Q = Mat_B
        // Mat_B : matrice carrée (n-2) x (n-2) de bande 5 qui change avec alpha et Diag_W_1
        std::vector<std::vector<long double>> matB;
       // const double alpha = mModel->mAlphaLissage.mX;
        if (alpha != 0) {
            std::vector<std::vector<long double>> tmp = multiConstParMat(matQTW_1Q, alpha, 5);
            matB = addMatEtMat(matR, tmp, 5);

        } else {
            matB = matR;
        }

        // Decomposition_Cholesky de matB en matL et matD
        // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
        std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matB, 5, 1);
        std::vector<std::vector<long double>> matL = decomp.first;
        std::vector<long double> matD = decomp.second;

        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = mModel->mEvents.size();


        // Calcul du vecteur Vec_QtY, de dimension (n-2)
        //std::vector<long double> vecH = calculVecH(mModel->mEvents);



        std::vector<long double> vecG (n);
        std::vector<long double> vecQtY(n);

        for (size_t i = 1; i < n-1; ++i) {
            long double term1 = (vecY.at(i+1) - vecY.at(i)) / vecH.at(i);
            long double term2 = (vecY[i] - vecY[i-1]) / vecH[i-1];
            vecQtY[i] = term1 - term2;
        }

        // Calcul du vecteur Gamma
        std::vector<long double> vecGamma = resolutionSystemeLineaireCholesky(matL, matD, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - alpha * W-1 * Q * gamma
        if (alpha != 0) {
            std::vector<long double> vecTmp2 = multiMatParVec(matQ, vecGamma, 3);
            std::vector<long double> diagWInv = createDiagWInv(mModel->mEvents);
            for (unsigned i = 0; i < n; ++i) {
                vecG[i] = vecY.at(i) - alpha * diagWInv.at(i) * vecTmp2.at(i);
            }

        } else {
            vecG = vecY;
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopChronocurve::calculSpline vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopChronocurve::calculSpline vecGamma NULL";
        }
#endif

        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);
        spline.matB = std::move(matB);
        spline.matL = std::move(matL);
        spline.matD = std::move(matD);

    } catch(...) {
                qCritical() << "MCMCLoopChronocurve::calculSpline : Caught Exception!\n";
    }

    return spline;
}

/*
 * MatB doit rester en copie
 */
SplineResults MCMCLoopChronocurve::calculSplineX(const SplineMatrices& matrices, const std::vector<long double>& vecH, std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const std::vector<std::vector<long double>> matB, const double alpha)
{
   //
   // const std::vector<std::vector<double>>& matR = matrices.matR;
   // const std::vector<std::vector<double>>& matQ = matrices.matQ;
   // const std::vector<std::vector<double>>& matQTW_1Q = matrices.matQTW_1Q;
    SplineResults spline;
    try {
        // calcul de: R + alpha * Qt * W-1 * Q = Mat_B
        // Mat_B : matrice carrée (n-2) x (n-2) de bande 5 qui change avec alpha et Diag_W_1
    /*    std::vector<std::vector<double>> matB = matrices.matR; //matR;
        const double alpha = mModel->mAlphaLissage.mX;
        if (alpha != 0) {
            std::vector<std::vector<double>> tmp = multiConstParMat(matrices.matQTW_1Q, alpha, 5);
            matB = addMatEtMat(matrices.matR, tmp, 5);
        }

        // Decomposition_Cholesky de matB en matL et matD
        // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
        std::pair<std::vector<std::vector<double>>, std::vector<double>> decomp = decompositionCholesky(matB, 5, 1);


      */


        const std::vector<std::vector<long double>> matL = decomp.first; // we must keep a copy
        const std::vector<long double> matD = decomp.second; // we must keep a copy

        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = mModel->mEvents.size();


        // Calcul du vecteur Vec_QtY, de dimension (n-2)
       // std::vector<double> vecH = calculVecH(mModel->mEvents);



        std::vector<long double> vecG (n);
        std::vector<long double> vecQtY(n);

        for (size_t i = 1; i < n-1; ++i) {
            long double term1 = (mModel->mEvents.at(i+1)->mYx - mModel->mEvents.at(i)->mYx) / vecH.at(i);
            long double term2 = (mModel->mEvents.at(i)->mYx - mModel->mEvents.at(i-1)->mYx) / vecH[i-1];
            vecQtY[i] = term1 - term2;
        }

        // Calcul du vecteur Gamma
        std::vector<long double> vecGamma = resolutionSystemeLineaireCholesky(matL, matD, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - alpha * W-1 * Q * gamma
        if (alpha != 0) {
            std::vector<long double> vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            std::vector<long double> diagWInv = createDiagWInv(mModel->mEvents);
            for (unsigned i = 0; i < n; ++i) {
                vecG[i] = mModel->mEvents.at(i)->mYx - alpha * diagWInv.at(i) * vecTmp2.at(i);
            }

        } else {
            //vecG = vecY;
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecG.begin(), [](Event* ev) {return ev->mYx;});
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopChronocurve::calculSpline vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopChronocurve::calculSpline vecGamma NULL";
        }
#endif

        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);
        spline.matB = std::move(matB);
        spline.matL = std::move(matL);
        spline.matD = std::move(matD);

    } catch(...) {
                qCritical() << "MCMCLoopChronocurve::calculSpline : Caught Exception!\n";
    }

    return spline;
}

/*
 * MatB doit rester en copie
 */
SplineResults MCMCLoopChronocurve::calculSplineY(const SplineMatrices& matrices, const  std::vector<long double>& vecH, std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const  std::vector<std::vector<long double>> matB, const double alpha)
{

    SplineResults spline;
    try {

        const std::vector<std::vector<long double>> matL = decomp.first;
        const std::vector<long double> matD = decomp.second;

        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = mModel->mEvents.size();


        // Calcul du vecteur Vec_QtY, de dimension (n-2)
       // std::vector<double> vecH = calculVecH(mModel->mEvents);



        std::vector<long double> vecG (n);
        std::vector<long double> vecQtY(n);

        for (size_t i = 1; i < n-1; ++i) {
            long double term1 = (mModel->mEvents.at(i+1)->mYy - mModel->mEvents.at(i)->mYy) / vecH.at(i);
            long double term2 = (mModel->mEvents.at(i)->mYy - mModel->mEvents.at(i-1)->mYy) / vecH.at(i-1);
            vecQtY[i] = term1 - term2;
        }

        // Calcul du vecteur Gamma
        std::vector<long double> vecGamma = resolutionSystemeLineaireCholesky(matL, matD, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - alpha * W-1 * Q * gamma
        if (alpha != 0) {
            std::vector<long double> vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            std::vector<long double> diagWInv = createDiagWInv(mModel->mEvents);
            for (unsigned i = 0; i < n; ++i) {
                vecG[i] = mModel->mEvents.at(i)->mYy - alpha * diagWInv.at(i) * vecTmp2.at(i);
            }

        } else {
            //vecG = vecY;
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecG.begin(), [](Event* ev) {return ev->mYy;});
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopChronocurve::calculSpline vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopChronocurve::calculSpline vecGamma NULL";
        }
#endif

        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);
        spline.matB = std::move(matB);
        spline.matL = std::move(matL);
        spline.matD = std::move(matD);

    } catch(...) {
                qCritical() << "MCMCLoopChronocurve::calculSpline : Caught Exception!\n";
    }

    return spline;
}

/*
 * MatB doit rester en copie
 */
SplineResults MCMCLoopChronocurve::calculSplineZ(const SplineMatrices& matrices, const std::vector<long double>& vecH, std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const std::vector<std::vector<long double>> matB, const double alpha)
{
    SplineResults spline;
    try {

        const std::vector<std::vector<long double>> matL = decomp.first; // we must keep a copy
        const std::vector<long double> matD = decomp.second; // we must keep a copy

        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = mModel->mEvents.size();


        // Calcul du vecteur Vec_QtY, de dimension (n-2)
       // std::vector<double> vecH = calculVecH(mModel->mEvents);



        std::vector<long double> vecG (n);
        std::vector<long double> vecQtY(n);

        for (size_t i = 1; i < n-1; ++i) {
            long double term1 = (mModel->mEvents.at(i+1)->mYz - mModel->mEvents.at(i)->mYz) / vecH.at(i);
            long double term2 = (mModel->mEvents.at(i)->mYz - mModel->mEvents.at(i-1)->mYz) / vecH.at(i-1);
            vecQtY[i] = term1 - term2;
        }

        // Calcul du vecteur Gamma
        std::vector<long double> vecGamma = resolutionSystemeLineaireCholesky(matL, matD, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - alpha * W-1 * Q * gamma
        if (alpha != 0) {
            std::vector<long double> vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            std::vector<long double> diagWInv = createDiagWInv(mModel->mEvents);
            for (unsigned i = 0; i < n; ++i) {
                vecG[i] = mModel->mEvents.at(i)->mYz - alpha * diagWInv.at(i) * vecTmp2.at(i);
            }

        } else {
            //vecG = vecY;
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecG.begin(), [](Event* ev) {return ev->mYz;});
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopChronocurve::calculSpline vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopChronocurve::calculSpline vecGamma NULL";
        }
#endif

        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);

        spline.matB = std::move(matB);
        spline.matL = std::move(matL);
        spline.matD = std::move(matD);

    } catch(...) {
                qCritical() << "MCMCLoopChronocurve::calculSpline : Caught Exception!\n";
    }

    return spline;
}

/**
 Cette procedure calcule la matrice inverse de B:
 B = R + alpha * Qt * W-1 * Q
 puis calcule la matrice d'influence A(alpha)
 Bande : nombre de diagonales non nulles
 used only with MCMCLoopChronocurve::calculSplineError()
 */
std::vector<long double> MCMCLoopChronocurve::calculMatInfluence(const SplineMatrices& matrices,const std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const int nbBandes, const double alpha)
{
    const int n = mModel->mEvents.size();
    std::vector<long double> matA = initLongVector(n);


    if (alpha != 0) {

        const std::vector<std::vector<long double>>& matL = decomp.first; // we must keep a copy ?
        const std::vector<long double>& matD = decomp.second; // we must keep a copy ?

        std::vector<std::vector<long double>> matB1 = inverseMatSym(matL, matD, nbBandes + 4, 1);
        std::vector<long double> matQB_1QT = initLongVector(n);
        
        auto matQi = matrices.matQ.at(0);
        long double term = pow(matQi.at(0), 2) * matB1.at(0).at(0);
        term += pow(matQi.at(1), 2) * matB1.at(1).at(1);
        term += 2 * matQi.at(0) * matQi.at(1) * matB1.at(0).at(1);
        matQB_1QT[0] = term;
        
        for (int i = 1; i < n-1; ++i) {
            matQi = matrices.matQ.at(i);
            term = pow(matQi.at(i-1), 2.) * matB1.at(i-1).at(i-1);
            term += pow(matQi.at(i), 2.) * matB1.at(i).at(i);
            term += pow(matQi.at(i+1), 2.) * matB1.at(i+1).at(i+1);
            term += 2 * matQi.at(i-1) * matQi.at(i) * matB1.at(i-1).at(i);
            term += 2 * matQi.at(i-1) * matQi.at(i+1) * matB1.at(i-1).at(i+1);
            term += 2 * matQi.at(i) * matQi.at(i+1) * matB1.at(i).at(i+1);
            matQB_1QT[i] = term;
        }
        
        matQi = matrices.matQ.at(n-1);
        term = pow(matQi.at(n-2), 2.) * matB1.at(n-2).at(n-2);
        term += pow(matQi.at(n-1), 2.) * matB1.at(n-1).at(n-1);
        term += 2 * matQi.at(n-2) * matQi.at(n-1) * matB1.at(n-2).at(n-1);
        matQB_1QT[n-1] = term;
        
        // Multi_diag_par_Mat(Diag_W_1c,Mat_QB_1QT,Nb_noeudsc,1,tmp1);
        // Multi_const_par_Mat(-alphac,tmp1,Nb_noeudsc,1,Mat_Ac);
        // Addit_I_et_Mat(Mat_Ac,Nb_noeudsc);
        // remplacé par:
        for (int i = 0; i < n; ++i) {

            matA[i] = 1 - alpha * matrices.diagWInv.at(i) * matQB_1QT.at(i);

            if (matA.at(i) <= 0) {
                qWarning ("MCMCLoopChronocurve::calculMatInfluence -> Oups matA.at(i)=  <= 0  change to 1E-100");
                matA[i] = 1E-100; //pHd : A voir arbitraire
                // throw "MCMCLoopChronocurve::calculMatInfluence -> Oups matA.at(i) <= 0 change to 1E-100";

            }
        }

    } else {
       std::generate(matA.begin(), matA.end(), []{return 1.;});
    }
    return matA;
}

std::vector<long double> MCMCLoopChronocurve::calculSplineError(const SplineMatrices& matrices, const std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const double alpha)
{
    const size_t n = mModel->mEvents.size();
    std::vector<long double> matA = calculMatInfluence(matrices, decomp, 1, alpha);
    std::vector<long double> errG (n); //= initVecteur(n);
    
    int i = 0;
    for (auto& aii : matA) {
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii <= 0) {
            throw "MCMCLoopChronocurve::calculSplineError -> Oups aii<0 ";
        }
        errG[i] = sqrt(aii * (1 / mModel->mEvents.at(i)->mW));
        ++i;
    }
    
    return errG;
}



std::vector<unsigned>  MCMCLoopChronocurve::listOfIterationsWithPositiveGPrime (const std::vector<MCMCSplineComposante>& splineTrace)
{
    size_t nbIter = splineTrace.size();

    std::vector<unsigned> resultList;

    for (unsigned iter = 0; iter<nbIter; ++iter) {
        const MCMCSplineComposante& splineComposante = splineTrace.at(iter);

        if (asPositiveGPrime(splineComposante))
            resultList.push_back(iter);
    }

    return resultList;
}

bool  MCMCLoopChronocurve::asPositiveGPrime (const MCMCSplineComposante& splineComposante)
{

    const double& tmin = mModel->mSettings.mTmin;
    const double& tmax = mModel->mSettings.mTmax;
    const double& step = mModel->mSettings.mStep;

    const unsigned nbPoint = floor ((tmax - tmin +1) /step);
    unsigned i0, i1, i2;
    double t;
    bool accepted = true;
    i0 = 0;
    i1 = 0;
    i2 = 0;
    for (unsigned tIdx=0; tIdx <= nbPoint ; ++tIdx) {
        t = (double)tIdx * step + tmin ;

     //   double deltaG = ( valeurG(t+step/2., splineComposante, i0) - valeurG(t-step/2., splineComposante, i2) )/ step;
        double GPrime = valeurGPrime(t, splineComposante, i1);

        if (GPrime < 0.) {
            accepted = false;
            break;
        }
    }



    return accepted;
}












