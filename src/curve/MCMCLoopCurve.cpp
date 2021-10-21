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


#include "MCMCLoopCurve.h"
#include "ModelCurve.h"

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
#include <QProgressDialog>
//#include <execution>

#include <errno.h>      /* errno, EDOM */
#include <fenv.h>
#include <exception>

MCMCLoopCurve::MCMCLoopCurve(ModelCurve* model, Project* project):MCMCLoop(),
mModel(model)
{
    mProject = project;
    if (mModel){
        setMCMCSettings(mModel->mMCMCSettings);
    }
    
    QJsonObject state = project->mState;
    mCurveSettings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());
}

MCMCLoopCurve::~MCMCLoopCurve()
{
    mModel = nullptr;
    mProject = nullptr;
}

#pragma mark MCMC Loop Overloads

/**
 * Idem Chronomodel + prepareEventsY() qui sert à corriger les données d'entrées de Curve.
 * (Calcul de Yx, Yy, Yz et de Sy)
 */
QString MCMCLoopCurve::calibrate()
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

        
        return QString();
    }
    return tr("Invalid model");
}

/**
 * Idem Chronomodel + initialisation des variables aléatoires VG (events) et Lambda Spline (global)
 * TODO : initialisation des résultats g(t), g'(t), g"(t)
 */
void MCMCLoopCurve::initVariablesForChain()
{
    // today we have the same acceptBufferLen for every chain
    const int acceptBufferLen =  mChains.at(0).mIterPerBatch;
    int initReserve (0);
    
    for (auto& c: mChains) {
       initReserve += ( 1 + (c.mMaxBatchs*c.mIterPerBatch) + c.mIterPerBurn + (c.mIterPerAquisition/c.mThinningInterval) );
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
    
    mModel->mLambdaSpline.reset();
    mModel->mLambdaSpline.reserve(initReserve);
    mModel->mLambdaSpline.mLastAccepts.reserve(acceptBufferLen);
    mModel->mLambdaSpline.mLastAcceptsLength = acceptBufferLen;
    
    // Ré-initialisation du stockage des splines
    mModel->mSplinesTrace.clear();

    // Ré-initialisation des résultats
    mModel->mPosteriorMeanGByChain.clear();
    mModel->mPosteriorMeanG.gx.vecG.clear();
    mModel->mPosteriorMeanG.gy.vecG.clear();
    mModel->mPosteriorMeanG.gz.vecG.clear();

    mModel->mPosteriorMeanG.gx.vecVarG.clear();
    mModel->mPosteriorMeanG.gy.vecVarG.clear();
    mModel->mPosteriorMeanG.gz.vecVarG.clear();

    mModel->mPosteriorMeanG.gx.vecGP.clear();
    mModel->mPosteriorMeanG.gy.vecGP.clear();
    mModel->mPosteriorMeanG.gz.vecGP.clear();

    mModel->mPosteriorMeanG.gx.vecGS.clear();
    mModel->mPosteriorMeanG.gy.vecGS.clear();
    mModel->mPosteriorMeanG.gz.vecGS.clear();
}

/**
 * Idem Chronomodel + initialisation de VG (events) et Lambda Spline (global)
 */
QString MCMCLoopCurve::initialize()
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

             //   bound->mTheta.memo();
                bound->mTheta.mLastAccepts.clear();
                bound->mTheta.mLastAccepts.push_back(1.);
                //bound->mTheta.saveCurrentAcceptRate();
                bound->mInitialized = true;
                prepareEventY(bound);
            }
            bound = nullptr;
        }
    }

    // ----------------------------------------------------------------
    //  Init theta event, ti, ...
    // ----------------------------------------------------------------

    prepareEventsY(mModel->mEvents);

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

        // Controle la cohérence des contraintes strati-temporel et des valeurs de profondeurs
        if (mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth ) {
             for (auto&& eForWard : e->mConstraintsFwd) {
                 const bool notOk (e->mZField > eForWard->mEventTo->mZField);
                 if (notOk) {
                     mAbortedReason = QString(tr("Warning: chronological constraint not in accordance with the stratigraphy: %1 - %2 path, control depth value!")).arg (e->mName, eForWard->mEventTo->mName);
                     return mAbortedReason;
                 }
             }
        }
    }

    for (int i = 0; i<unsortedEvents.size(); ++i) {
        if (unsortedEvents.at(i)->mType == Event::eDefault) {

            mModel->initNodeEvents(); // Doit être réinitialisé pour toute recherche getThetaMinRecursive et getThetaMaxRecursive
            QString circularEventName = "";

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
                qDebug() << tr("-----Error Init for event : %1 : min = %2 : max = %3-------").arg(unsortedEvents.at(i)->mName, QString::number(min, 'f', 30), QString::number(max, 'f', 30));
#endif
            if (min >= max) {
                mAbortedReason = QString(tr("Error Init for event : %1 : min = %2 : max = %3-------").arg(unsortedEvents.at(i)->mName, QString::number(min, 'f', 30), QString::number(max, 'f', 30)));
                return mAbortedReason;
            }
            // ----------------------------------------------------------------
            // Curve init Theta event :
            // On initialise les theta près des dates ti
            // ----------------------------------------------------------------
       //     if (true) {//mCurveSettings.mTimeType == CurveSettings::eModeFixed)
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
                sampleInCumulatedRepartition(unsortedEvents.at(i), mModel->mSettings, min, max);

       /*        } else {
                // Idem Chronomodel, mais jamais utilisé ici :
                unsortedEvents.at(i)->mTheta.mX = Generator::randomUniform(min, max);
            } */
            unsortedEvents.at(i)->mThetaReduced = reduceTime(unsortedEvents.at(i)->mTheta.mX);
            unsortedEvents.at(i)->mInitialized = true;

#ifdef DEBUG
         //   qDebug() << QString("initialize theta event : %1 %2 %3 %4").arg(unsortedEvents.at(i)->mName, QString::number(min, 'f', 30), QString::number(unsortedEvents.at(i)->mTheta.mX, 'f', 30), QString::number(max, 'f', 30));
#endif
            // ----------------------------------------------------------------

            double s02_sum (0.);
            for (int j = 0; j < unsortedEvents.at(i)->mDates.size(); ++j) {
                Date& date = unsortedEvents.at(i)->mDates[j];

                // 1 - Init ti
                double sigma;
                // modif du 2021-06-16 pHd
                FunctionStat data = analyseFunction(vector_to_map(date.mCalibration->mCurve, mModel->mSettings.mTmin, mModel->mSettings.mTmax, date.mCalibration->mStep));
                sigma = double (data.std);
                //
                if (!date.mCalibration->mRepartition.isEmpty()) {
                    const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mCalibration->mRepartition);
                    date.mTheta.mX = date.mCalibration->mTmin + idx * date.mCalibration->mStep;
                    /* // modif du 2021-06-16 pHd
                      FunctionStat data = analyseFunction(vector_to_map(date.mCalibration->mCurve, tmin, tmax, date.mCalibration->mStep));
                      sigma = double (data.std);
                    */

                } else { // in the case of mRepartion curve is null, we must init ti outside the study period
                       // For instance we use a gaussian random sampling
                    sigma = mModel->mSettings.mTmax - mModel->mSettings.mTmin;
                    qDebug()<<"mRepartion curve is null for"<< date.mName;
                    const double u = Generator::gaussByBoxMuller(0., sigma);
                    if (u<0)
                        date.mTheta.mX = mModel->mSettings.mTmin + u;
                    else
                        date.mTheta.mX = mModel->mSettings.mTmax + u;

                    if (date.mTheta.mSamplerProposal == MHVariable::eInversion) {
                        qDebug()<<"Automatic sampling method exchange eInversion to eMHSymetric for"<< date.mName;
                        date.mTheta.mSamplerProposal = MHVariable::eMHSymetric;
                        date.autoSetTiSampler(true);
                    }

                }
                
                // 2 - Init Delta Wiggle matching and Clear mLastAccepts array
                date.initDelta(unsortedEvents.at(i));
                date.mWiggle.mLastAccepts.clear();
                //date.mWiggle.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
                date.mWiggle.tryUpdate(date.mWiggle.mX, 2.);

                // 3 - Init sigma MH adaptatif of each Data ti
                date.mTheta.mSigmaMH = sigma;

                // 4 - Clear mLastAccepts array and set this init at 100%
                date.mTheta.mLastAccepts.clear();
                //date.mTheta.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
                date.mTheta.tryUpdate(date.mTheta.mX, 2.);


                // intermediary calculus for the harmonic average
                s02_sum += 1. / (sigma * sigma);
            }

            // 4 - Init S02 of each Event
            unsortedEvents.at(i)->mS02 = unsortedEvents.at(i)->mDates.size() / s02_sum;

            // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
            unsortedEvents.at(i)->mTheta.mSigmaMH = sqrt((long double)unsortedEvents.at(i)->mS02);
            unsortedEvents.at(i)->mAShrinkage = 1.;
            
            // 6- Clear mLastAccepts  array
            unsortedEvents.at(i)->mTheta.mLastAccepts.clear();
            //unsortedEvents.at(i)->mTheta.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
            unsortedEvents.at(i)->mTheta.tryUpdate(unsortedEvents.at(i)->mTheta.mX, 2.);
            
        }

        // ----------------------------------------------------------------
        // Curve init VG :
        // ----------------------------------------------------------------

        Event* event = unsortedEvents.at(i);
        const double var_RICE = Calcul_Variances_Yij_Rice_GSJ(mModel->mEvents);
        if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
            event->mVG.mX = mCurveSettings.mVarianceFixed;

        } else {
            if (mCurveSettings.mUseVarianceIndividual) { // Variance G individuelle
                if (mCurveSettings.mUseErrMesure)
                    event->mVG.mX = var_RICE;
                else
                    event->mVG.mX = 1.;

            } else { // Variance G Clobale
                if ( i == 0) {
                    if (mCurveSettings.mUseErrMesure) {
                        event->mVG.mX = var_RICE;

                    } else {
                        event->mVG.mX = 1.;
                    }

                }  else {
                    event->mVG.mX = unsortedEvents.at(0)->mVG.mX;
                }

            }
        }
        event->mVG.mLastAccepts.clear();
        // event->mVG.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
        event->mVG.tryUpdate(event->mVG.mX, 2.);

        event->mVG.mSigmaMH = 1.;

        // ----------------------------------------------------------------
        //  Les W des events ne dépendant que de leur VG
        //  Lors de l'update, on a besoin de W pour les calculs de mise à jour de theta, VG et Lambda Spline
        //  On sera donc amenés à remettre le W à jour à chaque modification de VG
        //  On le calcul ici lors de l'initialisation pour avoir sa valeur de départ
        // ----------------------------------------------------------------
        event->updateW();


        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepProgressed(i);
    }







    // ----------------------------------------------------------------
    //  Init Sigma_i and its Sigma_MH
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
            date.mSigma.mSigmaMH = 1.;//1.27;  //1.;

            date.mSigma.mLastAccepts.clear();
            date.mSigma.tryUpdate(date.mSigma.mX, 2.);

        }
        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Init Lambda Spline
    // ----------------------------------------------------------------

    if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed){
        mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;

    } else {
        // il ne faut pas que h_YWI_AY() soit incalculable ou nul
        // long double res0 = 0.5 * (nb_noeuds-2.) * log((long double)alphaLissage) + h_exp;
        // long double res = exp(res0) * sqrt(det_1_2);
        mModel->mLambdaSpline.mX = 1.E-6;
        mModel->mLambdaSpline.mX = initLambdaSpline();
        // autre possibilité d'initialiser Lambda Spline
        //const double alphaInit = 1./ pow( (mModel->mSettings.mTmax - mModel->mSettings.mTmin)/ mModel->mEvents.size(), 3.);
       // mModel->mLambdaSpline.mX = alphaInit;
    }
    
    mModel->mLambdaSpline.mSigmaMH = 1.;
    mModel->mLambdaSpline.mLastAccepts.clear();
    mModel->mLambdaSpline.tryUpdate(mModel->mLambdaSpline.mX, 2.);


    // Initialisation pour adaptation au code RenCurve
/*
    mModel->mEvents[0]->mS02 = 2500;
    mModel->mEvents[0]->mThetaReduced = reduceTime(200);
    mModel->mEvents[0]->mTheta.mX = 200.;
    mModel->mEvents[0]->mDates[0].mTheta.mX = yearTime(0.00202246905664585);
    mModel->mEvents[0]->mDates[0].mSigma.mX = 83.5;

    mModel->mEvents[1]->mS02 = 2500;
    mModel->mEvents[1]->mThetaReduced = reduceTime((500.));
    mModel->mEvents[1]->mTheta.mX = 500.;
    mModel->mEvents[1]->mDates[0].mTheta.mX= yearTime(0.511381982047279);
    mModel->mEvents[1]->mDates[0].mSigma.mX = 1376.1;

    mModel->mEvents[2]->mS02 = 2500;
    mModel->mEvents[2]->mThetaReduced = reduceTime(900.);
    mModel->mEvents[2]->mTheta.mX = 900;
    mModel->mEvents[2]->mDates[0].mTheta.mX= yearTime(0.775249279549509);
    mModel->mEvents[2]->mDates[0].mSigma.mX = 1632.7;

    mModel->mEvents[3]->mS02 = 2500;
    mModel->mEvents[3]->mThetaReduced = reduceTime(1100);
    mModel->mEvents[3]->mTheta.mX = 1100;
    mModel->mEvents[3]->mDates[0].mTheta.mX= yearTime(0.975249813559322);
    mModel->mEvents[3]->mDates[0].mSigma.mX = 1583.3;

    mModel->mEvents[4]->mS02 = 2500;
    mModel->mEvents[4]->mThetaReduced = reduceTime(1500);
    mModel->mEvents[4]->mTheta.mX = 1500;
    mModel->mEvents[4]->mDates[0].mTheta.mX= yearTime(0.929440498773417);
    mModel->mEvents[4]->mDates[0].mSigma.mX = 686.9;

    mModel->mEvents[5]->mS02 = 2500;
    mModel->mEvents[5]->mThetaReduced = reduceTime(1800);
    mModel->mEvents[5]->mTheta.mX = 1800;
    mModel->mEvents[5]->mDates[0].mTheta.mX= yearTime(1.);
    mModel->mEvents[5]->mDates[0].mSigma.mX = 1;
*/

// fin test


    // --------------------------- Current spline ----------------------
    
    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z
    // --------------------------------------------------------------
    mModel->mSpline = currentSpline(mModel->mEvents, true);

    // --------------------------- Init phases ----------------------
    emit stepChanged(tr("Initializing Phases..."), 0, phases.size());

    i = 0;
    for (auto&& phase : phases ) {
        phase->updateAll(tmin, tmax);


        if (isInterruptionRequested())
            return ABORTED_BY_USER;
        ++i;

        emit stepProgressed(i);
    }



    return QString();
}

/**
 * Idem Chronomodel pour les Dates
 * Pour les events, theta a une nouvelle composante et est donc entièrement revu
 * Les events doivent aussi mettre VG à jour (sur le même principe que theta)
 * Idem pour le Lambda Spline global
 * Ces 3 mises à jour font intervenir le calcul matriciel (cravate, spline, etc...)
 */
bool MCMCLoopCurve::update()
{
 try {
    const double t_max (mModel->mSettings.mTmax);
    const double t_min (mModel->mSettings.mTmin);
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
#ifdef DEBUG
            if (date.mSigma.mX <= 0)
                qDebug(" date.mSigma.mX <= 0 ");
#endif
            //date.updateSigmaReParam(event);
            date.updateWiggle();

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

    long double current_value, current_h, current_h_theta, current_h_YWI, current_h_lambda, current_h_VG;
    SplineMatrices current_matrices;
    std::vector<long double> current_vecH;

    long double try_value, try_h, try_h_theta, try_h_YWI, try_h_lambda, try_h_VG;
    SplineMatrices try_matrices;
    std::vector<long double> try_vecH;

    long double rapport;

    // init the current state
    orderEventsByThetaReduced(mModel->mEvents);

    spreadEventsThetaReduced0(mModel->mEvents);
    current_vecH = calculVecH(mModel->mEvents);
    current_matrices = prepareCalculSpline(mModel->mEvents, current_vecH);

    // pHd : pour h_theta mTheta doit être en année, et h_YWI_AY utilise mThetaReduced
     //long double h_current= h_YWI_AY(matrices, mModel->mEvents, mModel->mLambdaSpline.mX) * h_alpha(matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX) * h_theta(mModel->mEvents);

    current_h_YWI = h_YWI_AY(current_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH);

    current_h_lambda = h_alpha(current_matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

    current_h_theta = h_theta(mModel->mEvents);

    current_h = current_h_YWI * current_h_lambda * current_h_theta;


    if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
        try {
                       /* ----------------------------------------------------------------------
             *  Dans Chronomodel, on appelle simplement : event->updateTheta(t_min,t_max); sur tous les events.
             *  Pour mettre à jour un theta d'event dans Curve, on doit accéder aux thetas des autres events.
             *  => on effectue donc la mise à jour directement ici, sans passer par une fonction
             *  de la classe event (qui n'a pas accès aux autres events)
             * ---------------------------------------------------------------------- */

          //  for (Event*& event : initListEvents) {
            for (int i = initListEvents.size()-1 ; i>-1; i--) {
            //for (unsigned i = 0 ; i<initListEvents.size(); i++) {
                Event*& event = initListEvents[i];

                if (event->mType == Event::eDefault) {
                    // -----
                   /* orderEventsByThetaReduced(mModel->mEvents);

                    spreadEventsThetaReduced0(mModel->mEvents);
                    std::vector<long double> vecH_current = calculVecH(mModel->mEvents);
                    SplineMatrices matrices = prepareCalculSpline(mModel->mEvents, vecH_current);

                    // pHd : pour h_theta mTheta doit être en année, et h_YWI_AY utilise mThetaReduced
                     //long double h_current= h_YWI_AY(matrices, mModel->mEvents, mModel->mLambdaSpline.mX) * h_alpha(matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX) * h_theta(mModel->mEvents);

                    long double h_YWI_current = h_YWI_AY(matrices, mModel->mEvents, mModel->mLambdaSpline.mX, vecH_current);
                    long double h_al_current, h_al_new;

                    h_al_current = h_alpha(matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);


                    long double h_thet_current = h_theta(mModel->mEvents); //h_theta(mModel->mEvents); // h_theta_Event(event);

                    h_current = h_YWI_current * h_al_current * h_thet_current;

                 */
                    // ----
                    const double min = event->getThetaMin(t_min);
                    const double max = event->getThetaMax(t_max);

                    if (min >= max) {
                        throw QObject::tr("Error for event theta : %1 : min = %2 : max = %3").arg(event->mName, QString::number(min), QString::number(max));
                    }

                    // On stocke l'ancienne valeur :
                    current_value = event->mTheta.mX;

                    // On tire une nouvelle valeur :
                    try_value = Generator::gaussByBoxMuller(current_value, event->mTheta.mSigmaMH);

                    rapport = 0.;


                    if (try_value >= min && try_value <= max) {
                        // On force la mise à jour de la nouvelle valeur pour calculer h_new

                        event->mTheta.mX = try_value; // pHd : Utile pour h_theta
                        event->mThetaReduced = reduceTime(try_value);

                        orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                        //spreadEventsThetaReduced(mModel->mEvents, minStep); // On espace les temps si il y a égalité de date
                        spreadEventsThetaReduced0(mModel->mEvents);

                        try_vecH = calculVecH(mModel->mEvents);
                        try_matrices = prepareCalculSpline(mModel->mEvents, try_vecH);

                        try_h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, try_vecH);

                        try_h_lambda = h_alpha(try_matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

                        try_h_theta = h_theta(mModel->mEvents);

                        try_h = try_h_YWI * try_h_lambda * try_h_theta;

                        // Calcul du rapport :
                        if ( current_h == 0.)
                            rapport = 1.;
                        else
                            rapport = try_h / current_h;

                    }
                    // restore Theta tu used function tryUpdate
                    event->mTheta.mX = current_value;
                    event->mTheta.tryUpdate(try_value, rapport);

                    if ( event->mTheta.mLastAccepts.last() == true) {
                        // Pour l'itération suivante :
                          current_h = try_h;
                          current_h_YWI = try_h_YWI;
                          current_h_lambda = try_h_lambda;
                      //    current_h_theta = try_h_theta;
                          current_vecH = try_vecH;
                          current_matrices = try_matrices;
                    }


                } else { // this is a bound, nothing to sample. Always the same value
                    event->updateTheta(t_min, t_max);
                    // Pour l'itération suivante :
                     // current_h = try_h; // there is no try_h, we stay on current_h
                }

                // update after tryUpdate or updateTheta
                event->mThetaReduced = reduceTime(event->mTheta.mX);


                //--------------------- Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
                for (auto&& phInEv : event->mPhases)
                    phInEv->updateAll(t_min, t_max);

            } // End of loop initListEvents


            // Rétablissement de l'ordre initial. Est-ce nécessaire ?
         //   std::copy(initListEvents.begin(), initListEvents.end(), mModel->mEvents.begin() );

        } catch(...) {
            qDebug() << "MCMCLoopCurve::update mCurveSettings.mTimeType == CurveSettings::eModeBayesian : Caught Exception!\n";
        }

    } else { // Pas bayésien : on sauvegarde la valeur constante dans la trace
        for (Event*& event : mModel->mEvents) {
            event->mTheta.tryUpdate(event->mTheta.mX, 2.);

            //--------------------- Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
            // maybe not usefull ??
            for (auto&& phInEv : event->mPhases)
                phInEv->updateAll(t_min, t_max);
        }

    }


    //--------------------- Update Phases constraints -----------------------------------------
    for (auto&& phConst : mModel->mPhaseConstraints )
        phConst->updateGamma();


    // --------------------------------------------------------------
    //  Remarque : à ce stade, tous les theta events sont à jour et ordonnés.
    //  On va à présent mettre à jour tous les VG, puis Lambda Spline.
    //  Pour cela, nous devons espacer les thetas pour permettre les calculs.
    //  Nous le faisons donc ici, et restaurerons les vrais thetas à la fin.
    // --------------------------------------------------------------

    try {
      /*  orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents); // don't modify theta->mX
        current_vecH = calculVecH(mModel->mEvents);
*/
       // orderEventsByThetaReduced(mModel->mEvents);
        //  saveEventsTheta(mModel->mEvents);
        //  reduceEventsTheta(mModel->mEvents); // On passe en temps réduit entre 0 et 1
        // find minimal step;
        // long double minStep = minimalThetaReducedDifference(mModel->mEvents);
        //spreadEventsThetaReduced(mModel->mEvents, minStep/10.);
       // spreadEventsThetaReduced0(mModel->mEvents);


    } catch(...) {
        qDebug() << "MCMCLoopCurve::update order Event : Caught Exception!\n";
    }
    // --------------------------------------------------------------
    //  Update VG Events
    // --------------------------------------------------------------

//if (mState == eAquisition)
//qDebug()<< ModelUtilities::modelStateDescriptionText(mModel);

    current_h_VG = h_VG(mModel->mEvents);
    current_h = current_h_YWI * current_h_lambda * current_h_VG;

    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        try {

          /*  SplineMatrices current_matrices = prepareCalculSpline(mModel->mEvents, vecH);
            current_h_YWI = h_YWI_AY(current_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, vecH);

            current_h_al = h_alpha(current_matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);
            */


            const double logMin = -10.; //log10(1e-10);
            const double logMax = +20.; //log10(1e+20);

            if (mCurveSettings.mUseVarianceIndividual) {

                // Variance individuelle

                Event* event;
                //for (int i = 0; i < mModel->mEvents.size(); ++i) {
                for (int i = mModel->mEvents.size()-1; i >= 0; --i) {
                    event = mModel->mEvents.at(i);

                    current_value = event->mVG.mX;

                    // On tire une nouvelle valeur :
                    const double try_value_log = Generator::gaussByBoxMuller((double)log10(current_value), event->mVG.mSigmaMH);
                    try_value = pow(10., try_value_log);


                    if (try_value_log >= logMin && try_value_log <= logMax) {
                        // On force la mise à jour de la nouvelle valeur pour calculer try_h
                        // A chaque fois qu'on modifie VG, W change !
                        event->mVG.mX = try_value;
                        event->updateW(); // used by prepareCalculSpline

                        // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG
                        try_matrices = prepareCalculSpline(mModel->mEvents, current_vecH);

                        try_h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH);

                        try_h_lambda = h_alpha(try_matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

                        try_h_VG = h_VG (mModel->mEvents);

                        if (try_h_YWI == HUGE_VAL || try_h_lambda== HUGE_VAL || try_h_VG==HUGE_VAL)
                            try_h = 0.;
                        else
                            try_h = try_h_YWI * try_h_lambda * try_h_VG;

                        if ( current_h == 0.)
                            rapport = 1.;
                        else
                            rapport = ((try_h * try_value) / (current_h * current_value));

                    } else {
                        rapport = -1.; // On force à garder l'état courant
                    }

                    // Mise à jour Metropolis Hastings
                    // A chaque fois qu'on modifie VG, W change !
                    event->mVG.mX = (double) current_value;
                    event->mVG.tryUpdate((double) try_value, rapport);
                    event->updateW();

                    if ( event->mVG.mLastAccepts.last() == true) {

                     //   if (try_value > 1000*1000)
                     //       qDebug()<<"Accepted (try_value > 1000*1000)"<<event->mName << (double) try_value << (double) try_h << (double) current_value << (double) current_h;

                        // Pour l'itération suivante : Car mVG a changé

                        current_h = try_h;
                        current_h_YWI = try_h_YWI;
                        current_h_lambda = try_h_lambda;
                       // current_h_VG = try_h_VG;

                        current_matrices = try_matrices;
                    }


            }

    }
            else {
                // On stocke l'ancienne valeur :
                current_value = mModel->mEvents.at(0)->mVG.mX;

                // On tire une nouvelle valeur :

                const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mEvents.at(0)->mVG.mSigmaMH);
                try_value = pow(10, try_value_log);

                // affectation temporaire pour evaluer la nouvelle proba
                for (Event*& ev : mModel->mEvents) {
                    ev->mVG.mX = try_value;
                    ev->updateW();
                }

                long double rapport = 0.;
                if (try_value_log >= logMin && try_value_log <= logMax) {

                    // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG
                    try_matrices = prepareCalculSpline(mModel->mEvents, current_vecH);

                    try_h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH);

                    try_h_lambda = h_alpha (try_matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

                    try_h_VG = h_VG (mModel->mEvents);

                    try_h = try_h_YWI * try_h_lambda * try_h_VG;

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
                mModel->mEvents.at(0)->mVG.mX = (double) current_value;
                mModel->mEvents.at(0)->mVG.tryUpdate((double) try_value, rapport);
                mModel->mEvents.at(0)->updateW();

                const double acceptRapport = mModel->mEvents.at(0)->mVG.mLastAccepts.last() == true ? 2. : -1;

                for (int i = 1 ; i<mModel->mEvents.size(); i++) {
                    Event* ev = mModel->mEvents[i];
                    ev->mVG.mX = (double) current_value;
                    ev->mVG.tryUpdate((double) try_value, acceptRapport);
                    ev->updateW();
                }

                if ( acceptRapport == 2. ) {
                    //current_h = try_h;
                    current_h_YWI = try_h_YWI;
                    current_h_lambda = try_h_lambda;

                    current_matrices = try_matrices;
                }
            }

        } catch (std::exception& e) {
            qWarning()<< "MCMCLoopCurve::update VG : exception caught: " << e.what() << '\n';

        } catch(...) {
            qWarning() << "MCMCLoopCurve::update VG Event Caught Exception!\n";

        }



    // Pas bayésien : on sauvegarde la valeur constante dans la trace
    } else {        
        for (Event*& event : mModel->mEvents) {
            event->mVG.tryUpdate(mCurveSettings.mVarianceFixed, 1);
           // event->updateW(); //mVG never change so W never change

        }
    }
    
    // --------------------------------------------------------------
    //  Update Lambda Spline
    // --------------------------------------------------------------
    current_h = current_h_YWI * current_h_lambda;

    if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {
        const double logMin = -100.;
        const double logMax = +5;

        // On stocke l'ancienne valeur :
        current_value = mModel->mLambdaSpline.mX;
        // On tire une nouvelle valeur :
        const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
        try_value = powl(10., try_value_log);
        
        double rapport = 0.;
        try {
            if (try_value_log >= logMin && try_value_log <= logMax) {
                //SplineMatrices matrices = prepareCalculSpline(mModel->mEvents, current_vecH);
                //long double h_current = h_YWI_AY(matrices, mModel->mEvents, current_value, current_vecH) * h_alpha(matrices, mModel->mEvents.size(), current_value);

// pHd ::pb ici matriceR vide
                // Calcul du rapport :
 // pHd : current_matrices n'a pas changé

                try_h = h_YWI_AY(current_matrices, mModel->mEvents, try_value, current_vecH) * h_alpha(current_matrices, mModel->mEvents.size(), try_value);

                if ( current_h == 0.)
                    rapport = 1.;
                else
                    rapport = ((try_h * try_value) / (current_h * current_value));
                // On remet l'ancienne valeur, qui sera éventuellement mise à jour dans ce qui suit (Metropolis Hastings)
                //mModel->mLambdaSpline.mX = alpha_current;
            }

            mModel->mLambdaSpline.mX = current_value;
            mModel->mLambdaSpline.tryUpdate(try_value, rapport);


        } catch(...) {
            qDebug() << "MCMCLoopCurve::update Lambda Spline Caught Exception!\n";
        }


    }
    // Pas bayésien : on sauvegarde la valeur constante dans la trace
    else {
        mModel->mLambdaSpline.tryUpdate(mModel->mLambdaSpline.mX, 1);

    }
    
    // update MCMCSpline mModel->mSpline
    // 1- Calcul spline avec mModel->mLambdaSpline.mX en interne
     mModel->mSpline = currentSpline(mModel->mEvents, false, current_vecH, current_matrices);

    // 2 - test GPrime positive

    // bool test = hasPositiveGPrimeByDet(mModel->mSpline.splineX);
   //  bool test2 = hasPositiveGPrime(mModel->mSpline.splineX);
   //  if (test == true) qDebug()<< "test GPrime"<< (test==true);

    if (mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth)
        return hasPositiveGPrimeByDet(mModel->mSpline.splineX);
        //return hasPositiveGPrime(mModel->mSpline.splineX);

    else
        return true;


 } catch (const char* e) {
           qWarning() << "MCMCLoopCurve::update () char "<< e;


 } catch (const std::length_error& e) {
        qWarning() << "MCMCLoopCurve::update () length_error"<< e.what();
 } catch (const std::out_of_range& e) {
        qWarning() << "MCMCLoopCurve::update () out_of_range" <<e.what();
 } catch (const std::exception& e) {
        qWarning() << "MCMCLoopCurve::update () "<< e.what();

 } catch(...) {
        qWarning() << "MCMCLoopCurve::update () Caught Exception!\n";
        return false;
    }
return false;
}

bool MCMCLoopCurve::adapt(const int batchIndex)
{
    const double taux_min = 0.42;           // taux_min minimal rate of acceptation=42
    const double taux_max = 0.46;           // taux_max maximal rate of acceptation=46

    bool noAdapt = true;

    //--------------------- Adapt -----------------------------------------
    const int sizeAdapt = 10000;
    //  const double delta = (chain.mBatchIndex < 10000) ? 0.01 : (1. / sqrt(chain.mBatchIndex)); // code d'origine
    //  const double delta = (chain.mBatchIndex < 100) ? 0.1 : (0.1 / sqrt(chain.mBatchIndex));

    const double delta = (batchIndex < sizeAdapt) ? pow(sizeAdapt, -1/2.)  : pow(batchIndex, -1/2.);

    for (auto& event : mModel->mEvents) {
       for (auto& date : event->mDates) {

            //--------------------- Adapt Sigma MH de t_i -----------------------------------------
            if (date.mTheta.mSamplerProposal == MHVariable::eMHSymGaussAdapt)
                noAdapt *= date.mTheta.adapt(taux_min, taux_max, delta);

            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            noAdapt *= date.mSigma.adapt(taux_min, taux_max, delta);

        }

        //--------------------- Adapt Sigma MH de Theta Event -----------------------------------------      
       if ((event->mType != Event::eKnown) && ( event->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss) )
           noAdapt *= event->mTheta.adapt(taux_min, taux_max, delta);

       //--------------------- Adapt Sigma MH de VG -----------------------------------------
//qDebug()<<event->mName<< event->mVG.getCurrentAcceptRate() << event->mVG.mX << event->mVG.mSigmaMH;
        noAdapt *= event->mVG.adapt(taux_min, taux_max, delta);

    }
    
    //--------------------- Adapt Sigma MH de Lambda Spline -----------------------------------------
    if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eMHAdaptGauss)
        noAdapt *= mModel->mLambdaSpline.adapt(taux_min, taux_max, delta);
//qDebug()<<mModel->mLambdaSpline.getCurrentAcceptRate() <<"\t"<< mModel->mLambdaSpline.mX <<"\t"<< mModel->mLambdaSpline.mSigmaMH;

    return noAdapt;
}


void MCMCLoopCurve::memo()
{
    for (auto&& event : mModel->mEvents) {
        //--------------------- Memo Events -----------------------------------------
        event->mTheta.memo();
        event->mTheta.saveCurrentAcceptRate();

        // On stocke la racine de VG, qui est une variance pour afficher l'écart-type
        double memoVG = sqrt((double)event->mVG.mX);


        event->mVG.memo(&memoVG);
        //event->mVG.memo();
        event->mVG.saveCurrentAcceptRate();

        for (auto&& date : event->mDates )   {
            //--------------------- Memo Dates -----------------------------------------
            date.mTheta.memo();
            date.mSigma.memo();
            date.mWiggle.memo();

            date.mTheta.saveCurrentAcceptRate();
            date.mSigma.saveCurrentAcceptRate();
        }

    }

    //--------------------- Memo Phases -----------------------------------------
    for (auto&& ph : mModel->mPhases)
            ph->memoAll();

    //--------------------- Memo mLambdaSpline Smoothing -----------------------------------------
    // On stocke le log10 de Lambda Spline pour afficher les résultats a posteriori
    double memoLambda = log10(mModel->mLambdaSpline.mX);
    mModel->mLambdaSpline.memo(&memoLambda);
   // mModel->mLambdaSpline.memo();
    mModel->mLambdaSpline.saveCurrentAcceptRate();

    //--------------------- Memo Spline -----------------------------------------

    mModel->mSplinesTrace.push_back(mModel->mSpline);

}

void MCMCLoopCurve::finalize()
{
   for (int i = 0; i < mChains.size(); ++i) {
        ChainSpecs& chain = mChains[i];
        if (chain.mRealyAccepted == 0) {
            mAbortedReason = QString(tr("Warning : NO POSITIVE curve available with chain n° %1, current seed to change %2").arg (QString::number(i+1), QString::number(chain.mSeed)));
            throw mAbortedReason;
        }

    }
    // This is not a copy of all data!
    // Chains only contains description of what happened in the chain (numIter, numBatch adapt, ...)
    // Real data are inside mModel members (mEvents, mPhases, ...)
    mModel->mChains = mChains;

    // This is called here because it is calculated only once and will never change afterwards
    // This is very slow : it is for this reason that the results display may be long to appear at the end of MCMC calculation.
    /** @todo Find a way to make it faster !
     */
    mModel->generateCorrelations(mChains);

    // This should not be done here because it uses resultsView parameters
    // ResultView will trigger it again when loading the model
    mModel->initDensities();
    // mModel->generatePosteriorDensities(mChains, 1024, 1);
    
    // Generate numerical results of :
    // - MHVariables (global acceptation)
    // - MetropolisVariable : analysis of Posterior densities and quartiles from traces.
    // This also should be done in results view...
    // mModel->generateNumericalResults(mChains);
    
    // ----------------------------------------
    // Curve specific :
    // ----------------------------------------

    const bool hasY = (mCurveSettings.mProcessType != CurveSettings::eProcessTypeUnivarie);
    const bool hasZ = (mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
                       mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
                       mCurveSettings.mProcessType == CurveSettings::eProcessType3D);

    std::vector<MCMCSplineComposante> allChainsTraceX, chainTraceX, allChainsTraceY, chainTraceY, allChainsTraceZ, chainTraceZ;

    
#ifdef DEBUG
    qDebug()<<QString("MCMCLoopCurve::finalize");
    QElapsedTimer startTime;
    startTime.start();
#endif
    int shift  = 0;
    int shiftTrace = 0;

/*    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    const double step = mModel->mSettings.mStep;

    const unsigned nbPoint = floor ((tmax - tmin +1) /step);
*/

    PosteriorMeanG allChainsPosteriorMeanG;
/*
 * allChainsPosteriorMeanG.gx.vecG.resize(nbPoint);
allChainsPosteriorMeanG.gx.vecGP.resize(nbPoint);
allChainsPosteriorMeanG.gx.vecGS.resize(nbPoint);
allChainsPosteriorMeanG.gx.vecVarG.resize(nbPoint);
*/
    for (int i = 0; i < mChains.size(); ++i) {

        const int initBurnAdaptSize = 1 + mChains.at(i).mIterPerBurn + int (mChains.at(i).mBatchIndex * mChains.at(i).mIterPerBatch);
        const int runSize = mChains.at(i).mRealyAccepted;
        const int firstRunPosition = shift + initBurnAdaptSize;

        std::vector<MCMCSpline>::iterator first = mModel->mSplinesTrace.begin() + firstRunPosition ;
        std::vector<MCMCSpline>::iterator last = first + runSize -1 ;

        chainTraceX.clear();
        chainTraceY.clear();
        chainTraceZ.clear();
        unsigned progressPosition = 0;
        emit stepChanged(tr("Build chain %1").arg(i+1), 0, runSize);
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
      chainPosteriorMeanG.gx = compute_posterior_mean_G_composante(chainTraceX, tr("Calcul Mean Composante X for chain %1").arg(i+1));
    //  std::vector<std::vector<int>> mapGx = compute_posterior_map_G_composante(chainTraceX);

      if (hasY) {
          chainPosteriorMeanG.gy = compute_posterior_mean_G_composante(chainTraceY, tr("Calcul Mean Composante Y for chain %1").arg(i+1));
         // chainPosteriorMeanG.gz = compute_posterior_mean_G_composante(chainTraceZ, tr("Calcul Mean Composante Z for chain %1").arg(i+1));
      }
      if (hasZ) {
          chainPosteriorMeanG.gz = compute_posterior_mean_G_composante(chainTraceZ, tr("Calcul Mean Composante Z for chain %1").arg(i+1));
      }

      mModel->mPosteriorMeanGByChain.push_back(chainPosteriorMeanG);

      shiftTrace += runSize;
      shift = firstRunPosition + runSize;

    }
    

    // PosteriorMeanG allChainsPosteriorMeanG;

    if (mChains.size() == 1) {
        allChainsPosteriorMeanG = mModel->mPosteriorMeanGByChain.at(0);

    } else {

        allChainsPosteriorMeanG.gx = compute_posterior_mean_G_composante(allChainsTraceX, tr("Calcul Mean Composante X for All chain"));
        if (hasY) {
            allChainsPosteriorMeanG.gy = compute_posterior_mean_G_composante(allChainsTraceY, tr("Calcul Mean Composante Y for All chain"));
           // allChainsPosteriorMeanG.gz = compute_posterior_mean_G_composante(allChainsTraceZ, tr("Calcul Mean Composante Z for All chain"));
        }
        if (hasZ) {
            allChainsPosteriorMeanG.gz = compute_posterior_mean_G_composante(allChainsTraceZ, tr("Calcul Mean Composante Z for All chain"));
        }
    }

    // Conversion after the average
    if ( mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
         mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical) {
        emit stepChanged(tr("Compute System Conversion..."), 0, 0);

        if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector) {
            conversionIDF(allChainsPosteriorMeanG);
            for (auto&& chain: mModel->mPosteriorMeanGByChain)
                conversionIDF(chain);

        } else {
            conversionID(allChainsPosteriorMeanG);
            for (auto&& chain: mModel->mPosteriorMeanGByChain)
                conversionID(chain);
        }

    }

    mModel->mPosteriorMeanG = std::move(allChainsPosteriorMeanG);

#ifdef DEBUG

    QTime timeDiff(0, 0, 0, int(startTime.elapsed()));
    qDebug() <<  QString("=> MCMCLoopCurve::finalize done in %1 h %2 m %3 s %4 ms").arg(QString::number(timeDiff.hour()),
                                                                                      QString::number(timeDiff.minute()),
                                                                                      QString::number(timeDiff.second()),
                                                                                      QString::number(timeDiff.msec()) );
#endif


}


double MCMCLoopCurve::Calcul_Variances_Yij_Rice_GSJ (const QList<Event *> lEvents)
{
  /*double som_Yij =0;
  double som_Yij2=0;
  for (int k=1; k < lEvents.size(); ++k) {

    double Yij = lEvents.at(k)->mY;// tab_pts[k].Yij;
    som_Yij = som_Yij+Yij;
    som_Yij2  =som_Yij2 + pow(Yij, 2.);
}
  double moy_Yij = som_Yij/lEvents.size();
  double Var_Yij = (som_Yij2/lEvents.size())-pow(moy_Yij, 2.);
*/
  // Calcul de la variance Rice (1984)
  double Var_Rice =0;
  for (int i=1; i < lEvents.size(); ++i) {
        Var_Rice =Var_Rice + pow(lEvents.at(i)->mYx-lEvents.at(i-1)->mYx, 2.);
  }
  Var_Rice = 0.5*Var_Rice/(lEvents.size()-1);
/*
  // Calcul de la variance Gasser, Sroka et Jenner (1986)
  double GSJ =0;
  //for i:=2 to (nb_noeuds-1) do begin
  for (int i=2; i < lEvents.size()-1; ++i) {
    const double ti1 =lEvents.at(i-1)->mTheta.mX;
    const double ti  = lEvents.at(i)->mTheta.mX;
    const double ti2 = lEvents.at(i+1)->mTheta.mX;
    const double Yi1 = lEvents.at(i-1)->mTheta.mX;
    const double Yi  = lEvents.at(i)->mTheta.mX;
    const double Yi2 = lEvents.at(i+1)->mTheta.mX;
//    GSJ:=GSJ+sqr(0.5*Yi1-Yi+0.5*Yi2);
    GSJ = GSJ+pow(Yi-Yi1-((ti-ti1)/(ti2-ti1))*(Yi2-Yi1), 2.);
  }
  const double Var_GSJ =GSJ*((2/3)/(lEvents.size()-2));
  */
  return Var_Rice;
}


/*
 * Obsolete
 */
PosteriorMeanGComposante MCMCLoopCurve::computePosteriorMeanGComposante(const std::vector<MCMCSplineComposante>& trace, const QString &ProgressBarText)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    const double step = mModel->mSettings.mStep;

    const unsigned nbPoint = floor ((tmax - tmin +1) /step);

    std::vector<long double> vecCumulG2 (nbPoint);
    std::vector<long double> vecCumulVarG2 (nbPoint);
    
    std::vector<long double> vecG (nbPoint);
    std::vector<long double> vecGP (nbPoint);
    std::vector<long double> vecGS (nbPoint);
    std::vector<long double> vecVarG (nbPoint);
    
    unsigned long nbIter = trace.size();

    long double t, g, gp, gs, errG;
    g = 0.;
    gp = 0;
    errG = 0;
    gs = 0;
 /* TODO used Welford's online algorithm
  * https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
  */
    emit stepChanged(ProgressBarText, 0, nbIter*nbPoint);

    int progressPosition = 0;

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
            vecCumulVarG2[tIdx] += pow(errG, 2.);
            emit stepProgressed(progressPosition++);
        }

    }

    const double nbIter_tmax = nbIter * (tmax - tmin);
    const double nbIter_tmax2 = nbIter * pow(tmax - tmin, 2.);

    for (unsigned tIdx = 0; tIdx < nbPoint ; ++tIdx) {
        vecG[tIdx] /= nbIter;
        vecGP[tIdx] /=  nbIter_tmax;
        vecGS[tIdx] /=  nbIter_tmax2;
        vecVarG[tIdx] =  (vecCumulG2.at(tIdx) / nbIter) - pow(vecG.at(tIdx), 2.) + (vecCumulVarG2.at(tIdx) / nbIter);

    }
    
    PosteriorMeanGComposante result;
    result.vecG = std::move(vecG);
    result.vecGP = std::move(vecGP);
    result.vecGS = std::move(vecGS);
    result.vecVarG = std::move(vecVarG);


    return result;
}

PosteriorMeanGComposante MCMCLoopCurve::compute_posterior_mean_G_composante(const std::vector<MCMCSplineComposante>& trace, const QString &ProgressBarText)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    const double step = mModel->mSettings.mStep;

    const unsigned nbPoint = floor ((tmax - tmin +1) /step);

    std::vector<long double> vecG (nbPoint);
    std::vector<long double> vecGP (nbPoint);
    std::vector<long double> vecGS (nbPoint);
    // erreur inter spline
    std::vector<long double> vecVarianceG (nbPoint);
    std::vector<long double> vecVarG (nbPoint);
    // erreur intra spline
    std::vector<long double> vecVarErrG (nbPoint);

    unsigned long nbIter = trace.size();

    long double t, g, gp, gs, varG;
    g = 0.;
    gp = 0;
    varG = 0;
    gs = 0;
 /* TODO used Welford's online algorithm
  * https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
  */
    emit stepChanged(ProgressBarText, 0, nbIter);

    int n = 0;
    long double  prevMeanG;

    for (auto&& splineComposante : trace ) {
        n++;
        unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
        for (unsigned tIdx = 0; tIdx < nbPoint ; ++tIdx) {
            t = (long double)tIdx * step + tmin ;
            valeurs_G_VarG_GP_GS(t, splineComposante, g, varG, gp, gs, i0);

            prevMeanG = vecG.at(tIdx);
            vecG[tIdx] +=  (g - prevMeanG)/n;
            vecGP[tIdx] +=  (gp - vecGP.at(tIdx))/n;
            vecGS[tIdx] +=  (gs - vecGS.at(tIdx))/n;

            vecVarianceG[tIdx] +=  (g - prevMeanG)*(g - vecG.at(tIdx));

            vecVarErrG[tIdx] += (varG - vecVarErrG.at(tIdx)) / n  ;

        }

        emit stepProgressed(n);

    }

     int tIdx = 0;
     for (auto& vVarG : vecVarG) {
         vVarG = vecVarianceG.at(tIdx) / n + vecVarErrG.at(tIdx);
         ++tIdx;
     }

     PosteriorMeanGComposante result;
     result.vecG = std::move(vecG);
     result.vecGP = std::move(vecGP);
     result.vecGS = std::move(vecGS);
     result.vecVarG = std::move(vecVarG);

    return result;
}




// Obsolete
// Code devant permettre de faire le calcul de la courbe moyenne sur toutes les trace en même temps que la moyenne sur une trace
// CODE A CONTROLER
PosteriorMeanGComposante MCMCLoopCurve::computePosteriorMeanGComposante_chain_allchain(const std::vector<MCMCSplineComposante>& trace, PosteriorMeanGComposante& meanGAllChain, int prevChainSize)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    const double step = mModel->mSettings.mStep;

    const unsigned nbPoint = floor ((tmax - tmin +1) /step);

    std::vector<long double> vecVarianceG (nbPoint);
    std::vector<long double> vecCumulErrG2 (nbPoint);

    std::vector<long double> vecG (nbPoint);
    std::vector<long double> vecGP (nbPoint);
    std::vector<long double> vecGS (nbPoint);
    std::vector<long double> vecVarG (nbPoint);
    std::vector<long double> vecPrevVarErrG (nbPoint);
    std::vector<long double> vecVarErrG (nbPoint);

    std::vector<long double>& vecGAllChain = meanGAllChain.vecG;
    std::vector<long double>& vecGPAllChain = meanGAllChain.vecGP;
    std::vector<long double>& vecGSAllChain = meanGAllChain.vecGS;
    std::vector<long double>& vecVarGAllChain = meanGAllChain.vecVarG;

    unsigned long nbIter = trace.size();

    long double t, g, gp, gs, varG;
    g = 0.;
    gp = 0;
    varG = 0;
    gs = 0;
    int nPrevAll= prevChainSize;
    int n = 0;
 /* TODO used Welford's online algorithm
  * https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
  */
    long double  prevMeanG, prevMeanAll;
    for (unsigned i = 0; i<nbIter; ++i) {
        const MCMCSplineComposante& splineComposante = trace.at(i);
        n++;
        nPrevAll++;
        unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
        for (unsigned tIdx = 0; tIdx < nbPoint ; ++tIdx) {
            t = (long double)tIdx * step + tmin ;
            valeurs_G_VarG_GP_GS(t, splineComposante, g, varG, gp, gs, i0);

            //Calcul pour la chaine seule
            prevMeanG = vecG.at(tIdx);
            vecG[tIdx] +=  (g - prevMeanG)/n;
            vecGP[tIdx] +=  (gp - vecGP.at(tIdx))/n;
            vecGS[tIdx] +=  (gs - vecGS.at(tIdx))/n;

            vecVarianceG[tIdx] +=  (g - prevMeanG)*(g - vecG.at(tIdx));

            vecVarErrG[tIdx] += (varG - vecVarErrG.at(tIdx)) / n  ;

            // pour toutes les chaines

            prevMeanAll = vecGAllChain.at(tIdx);
            // Calcul des moyennes suivant Knuth
            vecGAllChain[tIdx] = prevMeanAll + (g - prevMeanAll)/nPrevAll;
            vecGPAllChain[tIdx] = vecGPAllChain.at(tIdx) + (gp - vecGPAllChain.at(tIdx))/nPrevAll;
            vecGSAllChain[tIdx] = vecGSAllChain.at(tIdx) + (gs - vecGSAllChain.at(tIdx))/nPrevAll;

            // Calcul de la Variance inter spline
            vecVarGAllChain[tIdx] += (g - prevMeanAll)*(g - vecGAllChain.at(tIdx));

            // Calcul de la moyenne des Variances intra spline avec errG
            vecVarErrG[tIdx] +=  (varG- vecVarErrG.at(tIdx))/n  ;



        }

    }

    int tIdx = 0;
    for (auto& vVarG : vecVarG) {
        vVarG = vecVarianceG.at(tIdx) / n + vecVarErrG.at(tIdx);

        vecVarGAllChain[tIdx] = vecVarGAllChain.at(tIdx)/nPrevAll + vecVarErrG.at(tIdx) ;
        ++tIdx;
    }


    PosteriorMeanGComposante result;
    result.vecG = std::move(vecG);
    result.vecGP = std::move(vecGP);
    result.vecGS = std::move(vecGS);
    result.vecVarG = std::move(vecVarG);

    return result;
}


std::vector<std::vector<int>> MCMCLoopCurve::compute_posterior_map_G_composante(const std::vector<MCMCSplineComposante>& trace)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    const long double ymin = -10000;
    const long double ymax = +10000;

    const unsigned nbPtsX = 100;
    const unsigned nbPtsY = 100;
    //const double step = mModel->mSettings.mStep;
    const double step = (tmin - tmax) / nbPtsX;

   // const unsigned nbPoint = 100;//floor ((tmax - tmin +1) /step);

  /*  std::vector<long double> vecG (nbPoint);
    std::vector<long double> vecGP (nbPoint);
    std::vector<long double> vecGS (nbPoint);
    // erreur inter spline
    std::vector<long double> vecVarianceG (nbPoint);
    std::vector<long double> vecVarG (nbPoint);
    // erreur intra spline
    std::vector<long double> vecVarErrG (nbPoint);
*/
    std::vector<std::vector<int>> curveMap;
    curveMap = initIntMatrix(nbPtsX+1, nbPtsY+1);
    //const unsigned long nbIter = trace.size();

    long double t, g, gp, gs, varG;
    g = 0.;
    gp = 0;
    varG = 0;
    gs = 0;
 /* TODO used Welford's online algorithm
  * https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
  */
   // emit stepChanged(ProgressBarText, 0, nbIter);

    int n = 0;
  //  long double  prevMeanG;
    unsigned idxY;
    for (auto&& splineComposante : trace ) {
        n++;
        unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
        for (unsigned tIdx = 0; tIdx <= nbPtsX ; ++tIdx) {
            t = (long double)tIdx * step + tmin ;
            valeurs_G_VarG_GP_GS(t, splineComposante, g, varG, gp, gs, i0);

            // pour debug
            g = std::max(ymin, std::min(g, ymax));


            idxY = floor((g-ymin)/(ymax-ymin) * nbPtsY);
            curveMap[tIdx][idxY] = curveMap[tIdx][idxY] + 1;
         /*   prevMeanG = vecG.at(tIdx);
            vecG[tIdx] +=  (g - prevMeanG)/n;
            vecGP[tIdx] +=  (gp - vecGP.at(tIdx))/n;
            vecGS[tIdx] +=  (gs - vecGS.at(tIdx))/n;

            vecVarianceG[tIdx] +=  (g - prevMeanG)*(g - vecG.at(tIdx));

            vecVarErrG[tIdx] += (varG - vecVarErrG.at(tIdx)) / n  ;
            */
        }

       // emit stepProgressed(n);

    }

   /*  int tIdx = 0;
     for (auto& vVarG : vecVarG) {
         vVarG = vecVarianceG.at(tIdx) / n + vecVarErrG.at(tIdx);
         ++tIdx;
     }
*/
   /*  PosteriorMeanGComposante result;
     result.vecG = std::move(vecG);
     result.vecGP = std::move(vecGP);
     result.vecGS = std::move(vecGS);
     result.vecVarG = std::move(vecVarG);
*/
    return curveMap;
}


double MCMCLoopCurve::valeurG(const double t, const MCMCSplineComposante& spline, unsigned &i0)
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
                g = gi1 + (gi2-gi1)*(tReduce-ti1)/h;

                // Smoothing part :
                double gamma1 = spline.vecGamma.at(i0);
                double gamma2 = spline.vecGamma.at(i0+1);
                double p = (1./6.) * ((tReduce-ti1) * (ti2-tReduce)) * ((1.+(tReduce-ti1)/h) * gamma2 + (1.+(ti2-tReduce)/h) * gamma1);
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
double MCMCLoopCurve::valeurErrG(const double t, const MCMCSplineComposante& spline, unsigned& i0)
{
    const unsigned n = spline.vecThetaEvents.size();
    
    double t1 = spline.vecThetaEvents[0];
    double tn = spline.vecThetaEvents[n-1];
    double errG = 0;
    
    if (t < t1) {
        errG = sqrt(spline.vecVarG.at(0));

    } else if(t >= tn) {
        errG = sqrt(spline.vecVarG.at(n-1));

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
                double err1 = sqrt(spline.vecVarG[i0]);
                double err2 = sqrt(spline.vecVarG[i0+1]);
                errG = err1 + ((t-ti1) / (ti2-ti1)) * (err2 - err1);
                break;
            }
        }
    }

    return errG;
}

// cans RenCurve U-CMT-Routine_Spline Valeur_Gp
double MCMCLoopCurve::valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0)
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
        gPrime = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tin_1);
        gPrime += (tn - tin_1) * spline.vecGamma.at(n-2) / 6.;

        /* Code Rencurve
         * tin_1:=Vec_splineP.t[nb_noeuds-1];
         * GPn:=(Vec_spline.G[nb_noeuds]-Vec_spline.G[nb_noeuds-1])/(tin-tin_1);
         * GP:=GPn+(tin-tin_1)*Vec_spline.gamma[nb_noeuds-1]/6;
         */

    } else {
        for ( ;i0< n-1; ++i0) {
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

double MCMCLoopCurve::valeurGSeconde(const double t, const MCMCSplineComposante& spline)
{
   const int n = spline.vecThetaEvents.size();
    const double tReduce = reduceTime(t);
    // The second derivative is always zero outside the interval [t1,tn].
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


// Obsolete
void MCMCLoopCurve::valeurs_G_ErrG_GP_GS(const double t, const MCMCSplineComposante& spline, long double& G, long double& errG, long double& GP, long double& GS, unsigned& i0)
{

    unsigned n = spline.vecThetaEvents.size();
    const long double tReduce =  reduceTime(t);
    const long double t1 = reduceTime(spline.vecThetaEvents.at(0));
    const long double tn = reduceTime(spline.vecThetaEvents.at(n-1));
    GP = 0.;
    GS = 0.;

     // The first derivative is always constant outside the interval [t1,tn].
     if (tReduce < t1) {
         const long double t2 = reduceTime(spline.vecThetaEvents.at(1));

         // ValeurGPrime
         GP = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
         GP -= (t2 - t1) * spline.vecGamma.at(1) / 6.;

         // ValeurG
         G = spline.vecG.at(0) - (t1 - tReduce) * GP;

         // valeurErrG
         errG = sqrt(spline.vecVarG.at(0));

         // valeurGSeconde
         //GS = 0.;

     } else if (tReduce >= tn) {

         const long double tn1 = reduceTime(spline.vecThetaEvents.at(n-2));
         // ValeurG

         // valeurErrG
         errG = sqrt(spline.vecVarG.at(n-1));

         // ValeurGPrime
         GP = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tn1);
         GP += (tn - tn1) * spline.vecGamma.at(n-2) / 6.;

         // valeurGSeconde
         //GS =0.;

         G = spline.vecG.at(n-1) + (tReduce - tn) * GP;

     } else {
         for (; i0 < n-1; ++i0) {
             const long double ti1 = reduceTime(spline.vecThetaEvents.at(i0));
             const long double ti2 = reduceTime(spline.vecThetaEvents.at(i0 + 1));
             if ((tReduce >= ti1) && (tReduce < ti2)) {
                 const long double h = ti2 - ti1;
                 const long double gi1 = spline.vecG.at(i0);
                 const long double gi2 = spline.vecG.at(i0 + 1);
                 const long double gamma1 = spline.vecGamma.at(i0);
                 const long double gamma2 = spline.vecGamma.at(i0 + 1);

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
                 const long double err1 = sqrt(spline.vecVarG.at(i0));
                 const long double err2 = sqrt(spline.vecVarG.at(i0 + 1));
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

void MCMCLoopCurve::valeurs_G_VarG_GP_GS(const double t, const MCMCSplineComposante& spline, long double& G, long double& varG, long double& GP, long double& GS, unsigned& i0)
{

    unsigned n = spline.vecThetaEvents.size();
    const long double tReduce =  reduceTime(t);
    const long double t1 = reduceTime(spline.vecThetaEvents.at(0));
    const long double tn = reduceTime(spline.vecThetaEvents.at(n-1));
    GP = 0.;
    GS = 0.;
    long double h;

     // The first derivative is always constant outside the interval [t1,tn].
     if (tReduce < t1) {
         const long double t2 = reduceTime(spline.vecThetaEvents.at(1));

         // ValeurGPrime
         GP = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
         GP -= (t2 - t1) * spline.vecGamma.at(1) / 6.;

         // ValeurG
         G = spline.vecG.at(0) - (t1 - tReduce) * GP;

         // valeurErrG
         varG = spline.vecVarG.at(0);

         // valeurGSeconde
         //GS = 0.;

     } else if (tReduce >= tn) {

         const long double tn1 = reduceTime(spline.vecThetaEvents.at(n-2));
        // deltaTheta = spline.vecThetaEvents.at(n-2) - t;

         // valeurErrG
         varG = spline.vecVarG.at(n-1);

         // ValeurGPrime
         GP = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tn1);
         GP += (tn - tn1) * spline.vecGamma.at(n-2) / 6.;

         // valeurGSeconde
         //GS =0.;

         // ValeurG
         G = spline.vecG.at(n-1) + (tReduce - tn) * GP;


     } else {
         for (; i0 < n-1; ++i0) {
             const long double ti1 = reduceTime(spline.vecThetaEvents.at(i0));
             const long double ti2 = reduceTime(spline.vecThetaEvents.at(i0 + 1));
             h = ti2 - ti1;

             if ((tReduce >= ti1) && (tReduce < ti2)) {

                 const long double gi1 = spline.vecG.at(i0);
                 const long double gi2 = spline.vecG.at(i0 + 1);
                 const long double gamma1 = spline.vecGamma.at(i0);
                 const long double gamma2 = spline.vecGamma.at(i0 + 1);

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
                 const long double err1 = sqrt(spline.vecVarG.at(i0));
                 const long double err2 = sqrt(spline.vecVarG.at(i0 + 1));
                 varG = pow(err1 + ((tReduce-ti1) / (ti2-ti1)) * (err2 - err1) , 2.l);
//if (err1 < 1 || err2 < 1)
  //  qDebug()<<"ici"<<double(err1)<<double(err2);

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

     // Value slope correction
     GP /=(mModel->mSettings.mTmax- mModel->mSettings.mTmin);
     GS /= pow(mModel->mSettings.mTmax- mModel->mSettings.mTmin, 2.);
}


/*********************************************************************************
**** Cette procedure calcule la Cross Validation
**** cad la prédiction de (g_(ti)-Yij) sans le point Yij
**********************************************************************************/
/*Procedure Calcul_Cross_Validation(var CVc,GCVc,DLEc:extended);
var i,k:integer;
    Wij,Wi,Aii,CVC1,CVC2:extended;
begin
  // Nombre de degré de liberté
  DLEc:= Nb_ref-trace(Mat_A,nb_noeuds);

  if (DLEc<>0) then begin

    {Calcul de la validation croisée CV }
    CVc:=0;
    for k:=1 to Nb_ref do begin
      i:=Tab_pts[k].index_crav;
      Wij:=Tab_pts[k].Wij;
      CVC1:= Vec_spline.g[i]-Tab_pts[k].Yij;
      CVC2:= 1 - ((Wij*Mat_A[i,i])/Tab_crav[i].Wi);
      CVc:=CVc + Wij * sqr(CVC1/CVC2);
    end;
    CVc:=CVc/W;

    {Calcul de la validation croisée généralisée GCV }
    GCVc:=0;
    for i:=1 to nb_noeuds do begin
      Wi:=Tab_crav[i].Wi;
      GCVc:=GCVc + Wi * ( sqr(Vec_spline.g[i]-Tab_crav[i].Yi) + Tab_crav[i].Si2 );
    end;
    GCVc:= (GCVc/sqr(DLEc/Nb_ref))/W;

  end;

end;
*/
long double MCMCLoopCurve::general_cross_validation (const SplineMatrices& matrices, const std::vector<long double>& vecH, const double lambdaSpline)
{

    const long double N = matrices.diagWInv.size();
    std::vector<std::vector<long double>> tmp = multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5);
    std::vector<std::vector<long double>> matB = addMatEtMat(matrices.matR, tmp, 5);
    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matB, 5, 1);
    SplineResults s = calculSplineX (matrices, vecH, decomp, matB, lambdaSpline);

    auto matA = calculMatInfluence_origin(matrices, s , 1, lambdaSpline);
    // Nombre de degré de liberté
    auto DLEc = N - std::accumulate(matA.begin(), matA.end(), 0.);

   long double GCV = 0.;
   for (int i = 0 ; i < N; i++) {
       // utiliser mYx pour splineX
        GCV +=  pow(s.vecG.at(i) - mModel->mEvents.at(i)->mYx, 2.)/ matrices.diagWInv.at(i) ;
   }
   GCV /= pow(DLEc, 2.);


    return GCV;
}

long double MCMCLoopCurve::cross_validation (const SplineMatrices& matrices, const std::vector<long double>& vecH, const double lambdaSpline)
{

    const long double N = matrices.diagWInv.size();
    std::vector<std::vector<long double>> tmp = multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5);
    std::vector<std::vector<long double>> matB = addMatEtMat(matrices.matR, tmp, 5);
    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matB, 5, 1);
    SplineResults s = calculSplineX (matrices, vecH, decomp, matB, lambdaSpline);

    auto matA = calculMatInfluence_origin(matrices, s , 1, lambdaSpline);

   long double CV = 0.;
   for (int i = 0 ; i < N; i++) {
       CV +=  pow((s.vecG.at(i) - mModel->mEvents.at(i)->mYx) / (1-matA.at(i)), 2.) / matrices.diagWInv.at(i) ;
   }

    return CV;
}

long double MCMCLoopCurve::initLambdaSpline()
{
    std::vector<long double> CV;
    std::vector<long double> lambda;
    std::vector<long double> memoVG;

    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    auto vecH = calculVecH(mModel->mEvents);
    auto matrices = prepareCalculSpline(mModel->mEvents, vecH);

    for (auto& ev : mModel->mEvents) {
        memoVG.push_back(ev->mVG.mX);
        ev->mVG.mX = 0.;
    }

    long double lambdaTest;

    for (int idxLambda = -20; idxLambda < 10; ++idxLambda ) {
        lambdaTest = pow(10., (long double)idxLambda);

        CV.push_back(cross_validation(matrices, vecH, lambdaTest));
        lambda.push_back(lambdaTest);
    }

    // on recherche la plus petite valeur de CV
    unsigned long idxDifMin = std::distance(CV.begin(), std::min_element(CV.begin(), CV.end()) );

    // si le mini est à une des bornes, il n'y a pas de solution
    // Donc on recherche la plus grande variation, le "coude"
    if (idxDifMin== 0 || idxDifMin == (CV.size()-1)) {
        // On recherche la plus grande variation de GCV
        std::vector<long double> difResult (CV.size()-1);
        std::transform(CV.begin(), CV.end()-1, CV.begin()+1 , difResult.begin(), [](long double a, long double b) {return pow(a-b, 2.l);});
        idxDifMin = 1+ std::distance(difResult.begin(), std::max_element(difResult.begin(), difResult.end()) );

    }

    // restore VG
    int i = 0;
    for (auto& ev : mModel->mEvents)
        ev->mVG.mX = memoVG.at(i++);

    return lambda.at(idxDifMin);
}


#pragma mark Related to : calibrate

void MCMCLoopCurve::prepareEventsY(const QList<Event *> &lEvents)
{
    for (Event* event : lEvents) {
        prepareEventY(event);
    }
}

/**
 * Preparation des valeurs Yx, Yy, Yz et Sy à partir des valeurs saisies dans l'interface : Yinc, Ydec, Sinc, Yint, Sint
 */
void MCMCLoopCurve::prepareEventY(Event* const event  )
{
    const double rad = M_PI / 180.;
    if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeUnivarie) {
        // Dans RenCurve, fichier U_cmt_lit_sauve
        if (mCurveSettings.mVariableType == CurveSettings::eVariableTypeInclination) {

            event->mYx = event->mXIncDepth;
            event->mSy = event->mS_XA95Depth; //ligne 348 : EctYij:= (1/sqrt(Kij))*Deg;

        } else if (mCurveSettings.mVariableType == CurveSettings::eVariableTypeDeclination) {
            event->mYx = event->mYDec;
            event->mSy = event->mS_XA95Depth / cos(event->mXIncDepth * rad); //ligne 364 : EctYij:=(1/(sqrt(Kij)*cos(Iij*rad)))*Deg;

        } else {
            event->mYx = event->mXIncDepth;
            event->mSy = event->mS_XA95Depth;
        }
        
        // Not used in univariate, but set to zero especially for CSV exports:
        event->mYy = 0;
        event->mYz = 0;

    } else if (mCurveSettings.mProcessType == CurveSettings::eProcessType2D) {
        event->mZField = 100.; // To treat the 2D case, we use the 3D case by setting Yint = 100
        event->mS_ZField = 0.;

        event->mYx = event->mXIncDepth;
        event->mYy = event->mYDec;
        event->mYz = event->mZField;

        event->mSy = sqrt( (pow(event->mS_Y, 2.) + pow(event->mS_XA95Depth, 2.)) /2.);

    } else if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical) {
        event->mZField = 100.; // To treat the spherical case, we use the vector case by posing Yint = 100
        event->mS_ZField = 0.;

        event->mYx = event->mZField * cos(event->mXIncDepth * rad) * cos(event->mYDec * rad);
        event->mYy = event->mZField * cos(event->mXIncDepth * rad) * sin(event->mYDec * rad);
        event->mYz = event->mZField * sin(event->mXIncDepth * rad);

        const double sYInc = event->mS_XA95Depth *rad;// 2.4477;
        event->mSy = event->mZField*sYInc; // ligne 537 : EctYij:= Fij/sqrt(Kij);

    } else if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector) {

        event->mYx = event->mZField * cos(event->mXIncDepth * rad) * cos(event->mYDec * rad);
        event->mYy = event->mZField * cos(event->mXIncDepth * rad) * sin(event->mYDec * rad);
        event->mYz = event->mZField * sin(event->mXIncDepth * rad);

        const double sYInc = event->mS_XA95Depth *rad;//2.4477 ;
        event->mSy = sqrt( (pow(event->mS_ZField, 2.) + 2. * pow(event->mZField*sYInc, 2.) ) /3.); // ligne 520 : EctYij:=sqrt( ( sqr(EctFij) + (2*sqr(Fij)/Kij) )/3 );

    } else if (mCurveSettings.mProcessType == CurveSettings::eProcessType3D) {

        event->mYx = event->mXIncDepth;
        event->mYy = event->mYDec;
        event->mYz = event->mZField;

        event->mSy = sqrt( (pow(event->mS_ZField, 2.) + pow(event->mS_Y, 2.) + pow(event->mS_XA95Depth, 2.)) /3.);

    }
    
    if (!mCurveSettings.mUseErrMesure) {
        event->mSy = 0.;
    }

    // Because of the problems of change of formula in case of null value. Zero is forbidden and replaced by an arbitary value
    if (event->mSy == 0)
        event->mSy = std::numeric_limits<double>::epsilon() * 1.E6;
}


#pragma mark Related to : update : calcul de h_new

/**
 * Calcul de h_YWI_AY pour toutes les composantes de Y event (suivant la configuration univarié, spérique ou vectoriel)
 */
long double MCMCLoopCurve::h_YWI_AY(const SplineMatrices& matrices, const QList<Event *> &lEvents, const long double lambdaSpline, const std::vector<long double>& vecH)
{
    long double h = h_YWI_AY_composanteX_origin( matrices, lEvents, lambdaSpline, vecH);

    if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical) {

        h *= h_YWI_AY_composanteY (matrices, lEvents, lambdaSpline, vecH);
        h *= h_YWI_AY_composanteZ (matrices, lEvents, lambdaSpline, vecH);

    } else  if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector) {

        h *= h_YWI_AY_composanteY(matrices, lEvents, lambdaSpline, vecH);
        h *= h_YWI_AY_composanteZ(matrices, lEvents, lambdaSpline, vecH);
    }
    return h;
}


long double MCMCLoopCurve::h_YWI_AY_composanteX(const SplineMatrices &matrices, const QList<Event *> lEvents, const double lambdaSpline, const std::vector<long double>& vecH)
{
    if (lambdaSpline == 0.) { // Attention double == 0
        return 1.;
    }
    errno = 0;
#ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);
#endif
    std::vector<long double> vecY (mModel->mEvents.size());
    std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYx;});

    SplineResults spline = calculSpline(matrices, vecY, lambdaSpline, vecH);
    std::vector<long double>& vecG = spline.vecG;

    std::vector<long double>& matD = spline.matD;
    const std::vector<std::vector<long double>>& matQTQ = matrices.matQTQ;

    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y


    const int nb_noeuds = lEvents.size();

   /* int i = 0;
    long double YWY = 0.;
    long double YWAY = 0.;
    for (auto& e : lEvents) {
        YWY  += (long double)e->mW * (long double)e->mYx * (long double)e->mYx;
        YWAY += (long double)e->mW * (long double)e->mYx * vecG.at(i);
        ++i;
    }
    long double h_exp = -0.5 * (YWY-YWAY);
*/
    long double h_exp = 0.;
    int i = 0;
    for (auto& e : lEvents) {
        h_exp  += (long double) e->mW * (long double)e->mYx * ((long double)e->mYx - vecG.at(i++));
    }
    h_exp *= -0.5 ;


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
    long double res = 0.5 * (nb_noeuds-2.) * log((long double)lambdaSpline) + h_exp;
    res = exp(res) * sqrt(det_1_2);

 //   long double res1= expl(res0 +log(det_1_2)/2);
#ifdef DEBUG
 #ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"errno set to EDOM";
      }
      if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"MCMCLoopCurve::h_YWI_AY_composanteX -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";

        else if (fetestexcept(FE_UNDERFLOW)) {
            qWarning("h_YWI_AY_composanteX res == 0 FE_UNDERFLOW : Underflow range error: The result is too small in magnitude to be represented as a value of the return type.");

        } else if (fetestexcept(FE_OVERFLOW)) {
            qWarning("h_YWI_AY_composanteX res == 0 FE_UNDERFLOW : Overflow range error: The result is too large in magnitude to be represented as a value of the return type.");
        }

      }
#endif
    if (res == HUGE_VAL) {
        qWarning("h_YWI_AY_composanteX res == HUGE_VAL");
    }
    if (res < 1E-250) {
        qWarning("h_YWI_AY_composanteX res < 1E-250");
    }


#endif
    return res;
}

long double MCMCLoopCurve::h_YWI_AY_composanteX_origin(const SplineMatrices &matrices, const QList<Event *> lEvents, const double lambdaSpline, const std::vector<long double>& vecH)
{
    if (lambdaSpline == 0){
            return 1.;
        }
   // ajout pour adaptation au code modifié
    std::vector<long double> vecY (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), vecY.begin(), [](Event* ev) {return (long double)ev->mYx;});



    SplineResults spline = calculSpline(matrices, vecY, lambdaSpline, vecH);
    std::vector<long double>& vecG = spline.vecG;
    std::vector<long double> vecGamma = spline.vecGamma;
    std::vector<std::vector<long double>> matL = spline.matL;
    std::vector<long double> matD = spline.matD;
    std::vector<std::vector<long double>> matQTQ = matrices.matQTQ;

    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y
    long double YWY = 0.;
    long double YWAY = 0.;

    int nb_noeuds = lEvents.size();

    for (int i=0; i<nb_noeuds; ++i)
    {
        Event* e = lEvents[i];

        YWY += (long double)e->mW * (long double)e->mYx * (long double)e->mYx;
        YWAY += (long double)e->mYx * (long double)e->mW * vecG[i];
    }

    long double h_exp = -0.5l * (YWY-YWAY);

/*         long double h_exp = 0.;
        int i = 0;
        for (auto& e : lEvents) {
            h_exp  += (long double) e->mW * (long double)e->mYx * ((long double)e->mYx - vecG.at(i++));
        }
        h_exp *= -0.5l;
  */
    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matQTQ, 5, 1);
    std::vector<std::vector<long double>> matLq = decomp.first;
    std::vector<long double> matDq = decomp.second;

    long double det_1_2 = 1.;
    for(int i=1; i<nb_noeuds-1; ++i){
        det_1_2 *= matDq[i] / matD[i];
    }

    // calcul à un facteur (2*PI) puissance -(n-2) près

    return exp(0.5l * (long double)(nb_noeuds-2) * log(lambdaSpline) + h_exp) * sqrt(det_1_2);
}

long double MCMCLoopCurve::h_YWI_AY_composanteY(const SplineMatrices& matrices, const QList<Event *> lEvents, const long double lambdaSpline, const std::vector<long double>& vecH)
{
    if (lambdaSpline == 0.) { // Attention double == 0
        return 1.;
    }
    errno = 0;
#ifdef Q_OS_MAC
      if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);
#endif
    std::vector<long double> vecY (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYy;});

    SplineResults spline = calculSpline(matrices, vecY, lambdaSpline, vecH);
    std::vector<long double>& vecG = spline.vecG;

    std::vector<long double>& matD = spline.matD;
    const std::vector<std::vector<long double>>& matQTQ = matrices.matQTQ;

    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y


    const int nb_noeuds = lEvents.size();

   /*
    long double YWY = 0.;
    long double YWAY = 0.;
    int i = 0;
    for (auto& e : lEvents) {
        YWY += e->mW * e->mYy * e->mYy;
        YWAY += e->mYy * e->mW * vecG.at(i);
        ++i;
    }

    long double h_exp = -0.5 * (YWY-YWAY);
  */
    long double h_exp = 0.;
    int i = 0;
    for (auto& e : lEvents) {
        h_exp  += (long double) e->mW * (long double)e->mYy * ((long double)e->mYy - vecG.at(i++));
    }
    h_exp *= -0.5 ;


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
    long double res = 0.5l * (nb_noeuds-2.) * log(lambdaSpline) + h_exp;
    res = exp(res) * sqrt(det_1_2);
#ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"errno set to EDOM";
      }
      if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"MCMCLoopCurve::h_YWI_AY_composanteY -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
      }
#endif
    //return exp(0.5 * (nb_noeuds-2.) * log(alphaLissage) + h_exp) * sqrt(det_1_2);
    return res;
}
long double MCMCLoopCurve::h_YWI_AY_composanteZ(const SplineMatrices& matrices, const QList<Event *> lEvents, const long double lambdaSpline, const std::vector<long double>& vecH)
{
    if (lambdaSpline == 0.) { // Attention double == 0
        return 1.;
    }
    errno = 0;
#ifdef Q_OS_MAC
      if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);
#endif
    std::vector<long double> vecY (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYz;});

    SplineResults spline = calculSpline(matrices, vecY, lambdaSpline, vecH);
    std::vector<long double>& vecG = spline.vecG;
    //std::vector<double>& vecGamma = spline.vecGamma;
  //  std::vector<std::vector<double>> matL = spline.matL;
    std::vector<long double>& matD = spline.matD;
    const std::vector<std::vector<long double>>& matQTQ = matrices.matQTQ;

    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------
    const int nb_noeuds = lEvents.size();

    // Calcul de la forme quadratique YT W Y  et  YT WA Y
 /*   long double YWY = 0.;
    long double YWAY = 0.;



    int i = 0;
    for (auto& e : lEvents) {
        YWY += e->mW * e->mYz * e->mYz;
        YWAY += e->mYz * e->mW * vecG.at(i);
        ++i;
    }

    long double h_exp = -0.5 * (YWY-YWAY);
*/
    long double h_exp = 0.;
    int i = 0;
    for (auto& e : lEvents) {
        h_exp  += (long double) e->mW * (long double)e->mYz * ((long double)e->mYz - vecG.at(i++));
    }
    h_exp *= -0.5 ;

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
    long double res = 0.5l * (nb_noeuds-2.) * log(lambdaSpline) + h_exp;
    res = exp(res) * sqrt(det_1_2);
#ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"errno set to EDOM";
      }
      if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"MCMCLoopCurve::h_YWI_AY_composanteZ -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
      }
 #endif
    //return exp(0.5 * (nb_noeuds-2.) * log(alphaLissage) + h_exp) * sqrt(det_1_2);
    return res;
}


long double MCMCLoopCurve::h_alpha(const SplineMatrices& matrices, const int nb_noeuds, const long double &lambdaSpline)
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
    
   // std::vector<std::vector<long double>> matRInv = inverseMatSym_origin(matL, matD, 5, 1);// l'inverson de matR à partir de la décomposition en matL et matD

    std::vector<std::vector<long double>> matRInv;
    if (matD.size() > 3) {
         matRInv = inverseMatSym_origin(matL, matD, 5, 1);

    } else {
        matRInv = initLongMatrix(3, 3);
        matRInv[1][1]= 1/ matD.at(1);  // pHd à faire confirmer
    }



    std::vector<std::vector<long double>> tmp = multiMatParMat(matQ, matRInv, 3, 3);
    std::vector<std::vector<long double>> matK = multiMatParMat(tmp, matQT, 3, 3);

    //long double vm = 0.;
    // pHd : Si la bande vaut 1, on prend donc la diagonale de matK, le calcul se simpifie
    //
      std::vector<long double> diag_W_1m (nb_noeuds, W_1m);//= initVecteur(nb_noeuds);
      std::vector<std::vector<long double>> matW_1K = multiDiagParMat(diag_W_1m, matK, 1);

    long double vm = 0.;
    for (int i = 0; i < nb_noeuds; ++i) {
        vm += matW_1K.at(i).at(i);
    }

   // for (int i = 0; i < nb_noeuds; ++i)
   //     vm += W_1m * matK.at(i).at(i);

    long double c = (nb_noeuds-2) / vm;

    // initialisation de l'exposant mu du prior "shrinkage" sur alpha : fixe
    // en posant mu=2, on la moyenne a priori sur alpha finie = (nb_noeuds-2)/somme(Mat_W_1K[i,i]) ;
    // et la variance a priori sur alpha est infinie
    // si on veut un shrinkage avec espérance et variance finies, alors mu > 2
    const long double mu = 2.;
    
    // prior "shrinkage"
    long double r;
 /*   if (isinf(c))   // pHd à controler !!
        r = mu / (nb_noeuds-2);
    else */

    r = (mu/c) * pow(c/(c + lambdaSpline), mu+1);

    return r;
}


/* ancienne fonction U_cmt_MCMC:: h_Vgij dans RenCurve
 */
long double MCMCLoopCurve::h_VG(const QList<Event *> lEvents)
{
    // Densité a priori sur variance de type "shrinkage" avec paramètre S02
    // bool_shrinkage_uniforme:=true;

    long double shrink_VG;

    const int nb_noeuds = lEvents.size();

    if (mCurveSettings.mUseVarianceIndividual) {

        shrink_VG = 1.;
        long double S02;
        for (Event* e :lEvents) {
            S02 = pow((long double)e->mSy, 2.l);
            shrink_VG *= (S02 / pow(S02 + (long double)e->mVG.mX, 2.l));
        }

    } else {

        // S02 : moyenne harmonique des erreurs sur Y
        long double som_inv_S02 = 0.;

        for (Event* e :lEvents) {
            som_inv_S02 += (1. / pow((long double)e->mSy, 2.l));
        }
        long double S02 = nb_noeuds/som_inv_S02;
        // Shrinkage avec a = 1

        shrink_VG = S02 / pow(S02 + (long double)lEvents.at(0)->mVG.mX, 2.l);
    }

    
    return shrink_VG;
}

/**
 * Les calculs sont faits avec les dates (theta event, ti dates, delta, sigma) exprimées en années.
 * Lors de l'appel de cette fonction, theta event a été réduit (voir les appels plus haut).
 * Il faut donc soit réduire les autres variables (plus coûteux en calculs), soit repasser theta event en années.
*/
// voir U-cmt_MCMC ligne 105 calcul_h
long double MCMCLoopCurve::h_theta_Event (const Event * e)
{
    long double p, t_moy;
    long double h1 = 1.l;
    if (e->mType == Event::eDefault) {
        p = 0.;
        t_moy = 0.;
        for (auto&& date : e->mDates) {
            const long double pi = 1. / pow((long double)date.mSigma.mX, 2.l);
            p += pi;
            t_moy += (long double)(date.mTheta.mX + date.mDelta) * pi;
        }
        t_moy /= p;

        h1 = exp(-0.5l * p * pow( (long double)(e->mTheta.mX  - t_moy), 2.l));
#ifdef DEBUG
        if (h1 == 0.) {
            qWarning( "MCMCLoopCurve::h_theta_Event() h1 == 0");
        }
#endif
    }
    return h1;
}

long double MCMCLoopCurve::h_theta(QList<Event *> lEvents)
{

    long double h = 1.;
/*
    for (Event*& e : lEvents) {
       h *= h_theta_Event(e);
    }
    */
    for (int i= lEvents.size()-1; i>=0; i-- ) {
        Event* e = lEvents[i];
       h *= h_theta_Event(e);
    }
#ifdef DEBUG
    if (h == 0.) {
        qWarning( "MCMCLoopCurve::h_theta() h == 0");
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
void MCMCLoopCurve::orderEventsByTheta(QList<Event *> & lEvent)
{
    /* On manipule directement la liste des évènements
       Ici on peut utiliser lEvent en le déclarant comme copy ??
    */
    QList<Event*>& result = lEvent;
    
    std::sort(result.begin(), result.end(), [](const Event* a, const Event* b) { return (a->mTheta.mX < b->mTheta.mX); });
}

void MCMCLoopCurve::orderEventsByThetaReduced(QList<Event *>& lEvent)
{
    // On manipule directement la liste des évènements
    // Ici on peut utiliser lEvent en le déclarant comme copy ??
    QList<Event*>& result = lEvent;

    std::sort(result.begin(), result.end(), [](const Event* a, const Event* b) { return (a->mThetaReduced < b->mThetaReduced); });
}

void MCMCLoopCurve::saveEventsTheta(QList<Event *>& lEvent)
{
    mThetasMemo.clear();
    for (Event*& e : lEvent) {
        mThetasMemo.insert(std::pair<int, double>(e->mId, e->mTheta.mX));
    }
}

void MCMCLoopCurve::restoreEventsTheta(QList<Event *>& lEvent)
{
    for (Event*& e : lEvent) {
        e->mTheta.mX = mThetasMemo.at(e->mId);
    }
}

/**
 * @brief MCMCLoopCurve::minimalThetaDifference, if theta are sort, the result is positive
 * @param lEvents
 * @return
 */
long double MCMCLoopCurve::minimalThetaDifference(QList<Event *>& lEvents)
{
    std::vector<long double> result (lEvents.size());
    std::transform (lEvents.begin(), lEvents.end()-1, lEvents.begin()+1, result.begin(), [](const Event* e0, const  Event* e1) {return (long double)(e1->mTheta.mX - e0->mTheta.mX); });
   // result.erase(result.begin()); // the firs value is not a difference, it's just the first value of LEvents
    std::sort(result.begin(), result.end());
    return std::move(*std::find_if_not (result.begin(), result.end(), [](long double v){return v==0.;} ));
}

long double MCMCLoopCurve::minimalThetaReducedDifference(QList<Event *>& lEvents)
{
    std::vector<long double> result (lEvents.size());
    std::transform (lEvents.begin(), lEvents.end()-1, lEvents.begin()+1, result.begin(), [](const Event* e0, const  Event* e1) {return (long double)(e1->mThetaReduced - e0->mThetaReduced); });
   // result.erase(result.begin()); // the firs value is not a difference, it's just the first value of LEvents
    std::sort(result.begin(), result.end());
    return std::move(*std::find_if_not (result.begin(), result.end(), [](long double v){return v==0.;} ));
}

// not used
void MCMCLoopCurve::spreadEventsTheta(QList<Event *>& lEvent, double minStep)
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

void MCMCLoopCurve::spreadEventsThetaReduced(QList<Event *> &lEvent, double minStep)
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
 * @brief MCMCLoopCurve::spreadEventsThetaReduced0
 * @param sortedEvents
 * @param spreadSpan
 */
void MCMCLoopCurve::spreadEventsThetaReduced0(QList<Event *> &sortedEvents, long double spreadSpan)
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
                long double lowBound = itEvenFirst == sortedEvents.begin() ? sortedEvents.first()->mThetaReduced : (*(itEvenFirst -1))->mThetaReduced ; //valeur à gauche non égale
                long double upBound = itEvent == sortedEvents.end()-2 ? sortedEvents.last()->mThetaReduced : (*(itEvent + 1))->mThetaReduced;

                long double step = spreadSpan / (nbEgal-1); // écart théorique
                long double min;

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

                long double max = std::min(upBound - spreadSpan, (*itEvent)->mThetaReduced + step*ceil(nbEgal/2.) );
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
        long double lowBound = (*(itEvenFirst -1))->mThetaReduced ; //la première valeur à gauche non égale

        long double value = (*(sortedEvents.end()-1))->mThetaReduced;
        long double step = spreadSpan / (nbEgal-1.); // ecart théorique

        long double min = std::max(lowBound + spreadSpan, value - step*(nbEgal-1) );
        long double max = value;
        step = (max- min)/ (nbEgal-1); // écart corrigé

        // Tout est réparti à gauche
        int count;
        QList<Event*>::iterator itEventEgal;
        for (itEventEgal = itEvenFirst, count = 0; itEventEgal != sortedEvents.end(); itEventEgal++, count++ ) {
            (*itEventEgal)->mThetaReduced = min + count *step;
        }

    }

}

void MCMCLoopCurve::reduceEventsTheta(QList<Event *> &lEvent)
{
    for (auto&& e : lEvent)
        e->mTheta.mX = reduceTime( e->mTheta.mX );

}

long double MCMCLoopCurve::reduceTime(double t)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    return (long double) (t - tmin) / (tmax - tmin);
}

long double MCMCLoopCurve::yearTime(double reduceTime)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    return (long double)  reduceTime * (tmax - tmin) + tmin ;
}
#pragma mark Pratique pour debug

std::vector<long double> MCMCLoopCurve::getThetaEventVector(const QList<Event *> &lEvent)
{
    std::vector<long double> vecT(lEvent.size());
    //std::transform(std::execution::par, lEvent.begin(), lEvent.end(), vecT.begin(), [](Event* ev) {return ev->mTheta.mX;}); // not yet implemented
    std::transform( lEvent.begin(), lEvent.end(), vecT.begin(), [](Event* ev) {return ev->mTheta.mX;});

    return vecT;
}

std::vector<long double> MCMCLoopCurve::getYEventVector(const QList<Event*>& lEvent)
{
    std::vector<long double> vecY;
    std::transform(lEvent.begin(), lEvent.end(), vecY.begin(), [](Event* ev) {return ev->mY;});

    return vecY;
}

#pragma mark Calcul Spline

// dans RenCurve procedure Calcul_Mat_Q_Qt_R ligne 66; doit donner une matrice symetrique; sinon erreur dans cholesky
std::vector<std::vector<long double>> MCMCLoopCurve::calculMatR(std::vector<long double>& vecH)
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
        matR[i][i] = (vecH.at(i-1) + vecH.at(i)) / 3.;
        // Si on est en n-2 (dernière itération), on ne calcule pas les valeurs de part et d'autre de la diagonale (termes symétriques)
        if (i < n-2) {
            matR[i][i+1] = vecH.at(i) / 6.;
            matR[i+1][i] = vecH.at(i) / 6.;
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
std::vector<std::vector<long double>> MCMCLoopCurve::calculMatQ(std::vector<long double>& vecH)
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


long double diffX (Event* e0, Event*e1) {return (e1->mThetaReduced - e0->mThetaReduced);}

// An other function exist for vector CurveUtilities::calculVecH(const vector<double>& vec)
std::vector<long double> MCMCLoopCurve::calculVecH(const QList<Event *> &lEvent)
{
    std::vector<long double> result (lEvent.size()-1);
    std::transform(lEvent.begin(), lEvent.end()-1, lEvent.begin()+1 , result.begin(), diffX);
#ifdef DEBUG
    int i =0;
    for (auto &&r :result) {
        if (r <= 0.) {
            char th [200];
            //char num [] ;
            sprintf(th, "MCMCLoopCurve::calculVecH diff Theta null %.2Lf et %.2Lf", lEvent.at(i)->mThetaReduced, lEvent.at(i+1)->mThetaReduced);
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
std::vector<long double> MCMCLoopCurve::createDiagWInv(const QList<Event*>& lEvents)
{
    std::vector<long double> diagWInv (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), diagWInv.begin(), [](Event* ev) {return 1/ev->mW;});

    return diagWInv;
}

/**
 * @brief MCMCLoopCurve::prepareCalculSpline
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
SplineMatrices MCMCLoopCurve::prepareCalculSpline(const QList<Event *>& sortedEvents, std::vector<long double>& vecH)
{
    std::vector<std::vector<long double>> matR = calculMatR(vecH);
    std::vector<std::vector<long double>> matQ = calculMatQ(vecH);
    
    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    std::vector<std::vector<long double>> matQT = transpose(matQ, 3);

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    std::vector<long double> diagWInv = createDiagWInv(sortedEvents);
    std::vector<std::vector<long double>> tmp = multiMatParDiag(matQT, diagWInv, 3);
    std::vector<std::vector<long double>> matQTW_1Q = multiMatParMat(tmp, matQ, 3, 3);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    std::vector<std::vector<long double>> matQTQ = multiMatParMat(matQT, matQ, 3, 3);

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
 * @brief MCMCLoopCurve::calculSpline
 * fabrication de spline avec
 *      spline.vecG
 *      spline.vecGamma
 *      spline.matB
 *      spline.matL
 *      spline.matD
 * @param matrices
 * @param vecY
 * @param lambdaSpline
 * @param vecH
 * @return
 */
SplineResults MCMCLoopCurve::calculSpline(const SplineMatrices& matrices, const std::vector<long double>& vecY, const double lambdaSpline, const std::vector<long double>& vecH)
{
    SplineResults spline;
    const std::vector<std::vector<long double>>& matR = matrices.matR;
    const std::vector<std::vector<long double>>& matQ = matrices.matQ;
    const std::vector<std::vector<long double>>& matQTW_1Q = matrices.matQTW_1Q;

    try {
        // calcul de: R + lambda * Qt * W-1 * Q = Mat_B
        // Mat_B : matrice carrée (n-2) x (n-2) de bande 5 qui change avec alpha et Diag_W_1
        std::vector<std::vector<long double>> matB;

        if (lambdaSpline != 0) {
            std::vector<std::vector<long double>> tmp = multiConstParMat(matQTW_1Q, lambdaSpline, 5);
            matB = addMatEtMat(matR, tmp, 5);

        } else {
            matB = matR;
        }

        // Decomposition_Cholesky de matB en matL et matD
        // Si lambda global: calcul de Mat_B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
        std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matB, 5, 1);
        std::vector<std::vector<long double>> matL = decomp.first;
        std::vector<long double> matD = decomp.second;

        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = mModel->mEvents.size();


        // Calcul du vecteur Vec_QtY, de dimension (n-2)

        std::vector<long double> vecG (n);
        std::vector<long double> vecQtY(n);

        for (size_t i = 1; i < n-1; ++i) {
            const long double term1 = (vecY.at(i+1) - vecY.at(i)) / vecH.at(i);
            const long double term2 = (vecY.at(i) - vecY.at(i-1)) / vecH.at(i-1);
            vecQtY[i] = term1 - term2;
        }

        // Calcul du vecteur Gamma
        std::vector<long double> vecGamma = resolutionSystemeLineaireCholesky(matL, matD, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - lambda * W_1 * Q * gamma
        if (lambdaSpline != 0) {
            std::vector<long double> vecTmp2 = multiMatParVec(matQ, vecGamma, 3);
            std::vector<long double> diagWInv = createDiagWInv(mModel->mEvents);
            for (unsigned i = 0; i < n; ++i) {
                vecG[i] = vecY.at(i) - lambdaSpline * diagWInv.at(i) * vecTmp2.at(i);
            }

        } else {
            vecG = vecY;
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopCurve::calculSpline vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopCurve::calculSpline vecGamma NULL";
        }
#endif

        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);
        spline.matB = std::move(matB);
        spline.matL = std::move(matL);
        spline.matD = std::move(matD);

    } catch(...) {
                qCritical() << "MCMCLoopCurve::calculSpline : Caught Exception!\n";
    }

    return spline;
}

/*
 * MatB doit rester en copie
 */
SplineResults MCMCLoopCurve::calculSplineX(const SplineMatrices& matrices, const std::vector<long double>& vecH, std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const std::vector<std::vector<long double>> matB, const double lambdaSpline)
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
        const double alpha = mModel->mLambdaSpline.mX;
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
            long double term2 = (mModel->mEvents.at(i)->mYx - mModel->mEvents.at(i-1)->mYx) / vecH.at(i-1);
            vecQtY[i] = term1 - term2;
        }

        // Calcul du vecteur Gamma
        std::vector<long double> vecGamma = resolutionSystemeLineaireCholesky(matL, matD, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - lamnbda * W-1 * Q * gamma
        if (lambdaSpline != 0) {
            std::vector<long double> vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            std::vector<long double> diagWInv = createDiagWInv(mModel->mEvents);
            for (unsigned i = 0; i < n; ++i) {
                vecG[i] = mModel->mEvents.at(i)->mYx - lambdaSpline * diagWInv.at(i) * vecTmp2.at(i);
            }

        } else {
            //vecG = vecY;
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecG.begin(), [](Event* ev) {return ev->mYx;});
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopCurve::calculSpline vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopCurve::calculSpline vecGamma NULL";
        }
#endif

        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);
        spline.matB = std::move(matB);
        spline.matL = std::move(matL);
        spline.matD = std::move(matD);

    } catch(...) {
                qCritical() << "MCMCLoopCurve::calculSplineX : Caught Exception!\n";
    }

    return spline;
}

/*
 * MatB doit rester en copie
 */
SplineResults MCMCLoopCurve::calculSplineY(const SplineMatrices& matrices, const  std::vector<long double>& vecH, std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const  std::vector<std::vector<long double>> matB, const double lambdaSpline)
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
        if (lambdaSpline != 0) {
            std::vector<long double> vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            std::vector<long double> diagWInv = createDiagWInv(mModel->mEvents);
            for (unsigned i = 0; i < n; ++i) {
                vecG[i] = mModel->mEvents.at(i)->mYy - lambdaSpline * diagWInv.at(i) * vecTmp2.at(i);
            }

        } else {
            //vecG = vecY;
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecG.begin(), [](Event* ev) {return ev->mYy;});
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopCurve::calculSpline vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopCurve::calculSpline vecGamma NULL";
        }
#endif

        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);
        spline.matB = std::move(matB);
        spline.matL = std::move(matL);
        spline.matD = std::move(matD);

    } catch(...) {
                qCritical() << "MCMCLoopCurve::calculSpline : Caught Exception!\n";
    }

    return spline;
}

/*
 * MatB doit rester en copie
 */
SplineResults MCMCLoopCurve::calculSplineZ(const SplineMatrices& matrices, const std::vector<long double>& vecH, std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const std::vector<std::vector<long double>> matB, const double lambdaSpline)
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

        // Calcul du vecteur g = Y - lambda * W-1 * Q * gamma
        if (lambdaSpline != 0) {
            std::vector<long double> vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            std::vector<long double> diagWInv = createDiagWInv(mModel->mEvents);
            for (unsigned i = 0; i < n; ++i) {
                vecG[i] = mModel->mEvents.at(i)->mYz - lambdaSpline * diagWInv.at(i) * vecTmp2.at(i);
            }

        } else {
            //vecG = vecY;
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecG.begin(), [](Event* ev) {return ev->mYz;});
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopCurve::calculSpline vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0.) == 0.) {
            qDebug() <<"MCMCLoopCurve::calculSpline vecGamma NULL";
        }
#endif

        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);

        spline.matB = std::move(matB);
        spline.matL = std::move(matL);
        spline.matD = std::move(matD);

    } catch(...) {
                qCritical() << "MCMCLoopCurve::calculSpline : Caught Exception!\n";
    }

    return spline;
}

/**
 Cette procedure calcule la matrice inverse de B:
 B = R + lambda * Qt * W-1 * Q
 puis calcule la matrice d'influence A(lambda)
 Bande : nombre de diagonales non nulles
 used only with MCMCLoopCurve::calculSplineError()
 */
/*
std::vector<long double> MCMCLoopCurve::calculMatInfluence(const SplineMatrices& matrices, const std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const int nbBandes, const double lambdaSpline)
{
    const int n = mModel->mEvents.size();
    std::vector<long double> matA = initLongVector(n);


    if (lambdaSpline != 0) {

        const std::vector<std::vector<long double>>& matL = decomp.first; // we must keep a copy ?
        const std::vector<long double>& matD = decomp.second; // we must keep a copy ?

        std::vector<std::vector<long double>> matB1 = inverseMatSym_origin(matL, matD, nbBandes + 4, 1);
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

            matA[i] = 1 - lambdaSpline * matrices.diagWInv.at(i) * matQB_1QT.at(i);



            if (matA.at(i) <= 0) {
                qWarning ("MCMCLoopCurve::calculMatInfluence -> Oups matA.at(i)=  <= 0  change to 1E-100");
                matA[i] = 0; //1e-100; //pHd : A voir arbitraire
                // throw "MCMCLoopCurve::calculMatInfluence -> Oups matA.at(i) <= 0 change to 1E-100";

            }
        }

    } else {
       std::generate(matA.begin(), matA.end(), []{return 1.;});
    }

    // pour controle
    const long double sooomm = std::accumulate(matA.begin(), matA.end(), 0.);
    qDebug() << "MCMCLoopCurve::calculMatInfluence sooomm,matAii" << (double) sooomm;

    return matA;
}
*/

/*
std::vector<long double> MCMCLoopCurve::calculMatInfluence0(const SplineMatrices& matrices, const std::vector<std::vector<long double>> &matB , const int nbBandes, const double lambdaSpline)
{
    const int n = mModel->mEvents.size();
    std::vector<long double> matA = initLongVector(n);


    if (lambdaSpline != 0) {

      //  const std::vector<std::vector<long double>>& matL = decomp.first; // we must keep a copy ?
       // const std::vector<long double>& matD = decomp.second; // we must keep a copy ?

        std::vector<std::vector<long double>> matB_1 = inverseMatSym0(matB, 1);//inverseMatSym(matL, matD, nbBandes + 4, 1);
        std::vector<long double> matQB_1QT = initLongVector(n);

        auto matQi = matrices.matQ.at(0);
        long double term = pow(matQi.at(0), 2) * matB_1.at(0).at(0);
        term += pow(matQi.at(1), 2) * matB_1.at(1).at(1);
        term += 2 * matQi.at(0) * matQi.at(1) * matB_1.at(0).at(1);
        matQB_1QT[0] = term;

        for (int i = 1; i < n-1; ++i) {
            matQi = matrices.matQ.at(i);
            term = pow(matQi.at(i-1), 2.) * matB_1.at(i-1).at(i-1);
            term += pow(matQi.at(i), 2.) * matB_1.at(i).at(i);
            term += pow(matQi.at(i+1), 2.) * matB_1.at(i+1).at(i+1);
            term += 2 * matQi.at(i-1) * matQi.at(i) * matB_1.at(i-1).at(i);
            term += 2 * matQi.at(i-1) * matQi.at(i+1) * matB_1.at(i-1).at(i+1);
            term += 2 * matQi.at(i) * matQi.at(i+1) * matB_1.at(i).at(i+1);
            matQB_1QT[i] = term;
        }

        matQi = matrices.matQ.at(n-1);
        term = pow(matQi.at(n-2), 2.) * matB_1.at(n-2).at(n-2);
        term += pow(matQi.at(n-1), 2.) * matB_1.at(n-1).at(n-1);
        term += 2 * matQi.at(n-2) * matQi.at(n-1) * matB_1.at(n-2).at(n-1);
        matQB_1QT[n-1] = term;

        // Multi_diag_par_Mat(Diag_W_1c,Mat_QB_1QT,Nb_noeudsc,1,tmp1);
        // Multi_const_par_Mat(-alphac,tmp1,Nb_noeudsc,1,Mat_Ac);
        // Addit_I_et_Mat(Mat_Ac,Nb_noeudsc);
        // remplacé par:
        for (int i = 0; i < n; ++i) {

            matA[i] = 1 - lambdaSpline * matrices.diagWInv.at(i) * matQB_1QT.at(i);

            if (matA.at(i) <= 0) {
                qWarning ("MCMCLoopCurve::calculMatInfluence -> Oups matA.at(i)=  <= 0  change to 1E-100");
                matA[i] = 1E-100; //pHd : A voir arbitraire
                // throw "MCMCLoopCurve::calculMatInfluence -> Oups matA.at(i) <= 0 change to 1E-100";

            }
        }

    } else {
       std::generate(matA.begin(), matA.end(), []{return 1.;});
    }
    return matA;
}
*/

std::vector<long double> MCMCLoopCurve::calculMatInfluence_origin(const SplineMatrices& matrices, const SplineResults& splines , const int nbBandes, const double lambdaSpline)
{
    const size_t n = mModel->mEvents.size();
    std::vector<long double> matA = initLongVector(n);


    if (lambdaSpline != 0) {

      //  const std::vector<std::vector<long double>>& matL = decomp.first; // we must keep a copy ?
       // const std::vector<long double>& matD = decomp.second; // we must keep a copy ?
        std::vector<std::vector<long double>> matB_1;
        if (splines.matD.size() > 3) {
             matB_1 = inverseMatSym_origin(splines.matL, splines.matD, nbBandes + 4, 1);

        } else {
            matB_1 = initLongMatrix(3, 3);
            matB_1[1][1] = 1./ splines.matD.at(1);  // pHd à faire confirmer, cas 3 points
        }
        std::vector<long double> matQB_1QT = initLongVector(n);

        auto matQi = matrices.matQ.at(0);
        long double term = pow(matQi.at(0), 2) * matB_1.at(0).at(0);
        term += pow(matQi.at(1), 2) * matB_1.at(1).at(1);
        term += 2 * matQi.at(0) * matQi.at(1) * matB_1.at(0).at(1);
        matQB_1QT[0] = term;

        for (size_t i = 1; i < n-1; ++i) {
            matQi = matrices.matQ.at(i);
            term = pow(matQi.at(i-1), 2.) * matB_1.at(i-1).at(i-1);
            term += pow(matQi.at(i), 2.) * matB_1.at(i).at(i);
            term += pow(matQi.at(i+1), 2.) * matB_1.at(i+1).at(i+1);
            term += 2 * matQi.at(i-1) * matQi.at(i) * matB_1.at(i-1).at(i);
            term += 2 * matQi.at(i-1) * matQi.at(i+1) * matB_1.at(i-1).at(i+1);
            term += 2 * matQi.at(i) * matQi.at(i+1) * matB_1.at(i).at(i+1);
            matQB_1QT[i] = term;
        }

        matQi = matrices.matQ.at(n-1);
        term = pow(matQi.at(n-2), 2.) * matB_1.at(n-2).at(n-2);
        term += pow(matQi.at(n-1), 2.) * matB_1.at(n-1).at(n-1);
        term += 2 * matQi.at(n-2) * matQi.at(n-1) * matB_1.at(n-2).at(n-1);
        matQB_1QT[n-1] = term;

        // Multi_diag_par_Mat(Diag_W_1c,Mat_QB_1QT,Nb_noeudsc,1,tmp1);
        // Multi_const_par_Mat(-alphac,tmp1,Nb_noeudsc,1,Mat_Ac);
        // Addit_I_et_Mat(Mat_Ac,Nb_noeudsc);
        // remplacé par:
        for (size_t i = 0; i < n; ++i) {

            matA[i] = 1 - lambdaSpline * matrices.diagWInv.at(i) * matQB_1QT.at(i);

            if (matA.at(i) < 0.) {
                qWarning ("MCMCLoopCurve::calculMatInfluence -> Oups matA.at(i) < 0  change to 0");
                matA[i] = 0.; //1E-100; //pHd : A voir arbitraire

            }
            if (matA.at(i) == 0.) {
                qWarning ("MCMCLoopCurve::calculMatInfluence -> Oups matA.at(i) = 0  change to 0");
                matA[i] = 0;//1E-100; //pHd : A voir arbitraire
            }

            if (matA.at(i) > 1E+4) {
                qWarning ("MCMCLoopCurve::calculMatInfluence -> Oups matA.at(i) > 1E+6  change to 0");
                matA[i] = 0.; //1E-100; //pHd : A voir arbitraire

            }

        }

    } else {
       std::generate(matA.begin(), matA.end(), []{return 1.;});
    }

    return matA;
}


/*
std::vector<long double> MCMCLoopCurve::calculSplineError(const SplineMatrices& matrices, const std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const double lambdaSpline)
{
    const size_t n = mModel->mEvents.size();
    std::vector<long double> matA = calculMatInfluence(matrices, decomp, 1, lambdaSpline);
    std::vector<long double> errG (n); //= initVecteur(n);
    
    int i = 0;
    for (auto& aii : matA) {
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii < 0) {
            throw "MCMCLoopCurve::calculSplineError -> Oups aii<0 ";
        }
        errG[i] = sqrt(aii * (1 / mModel->mEvents.at(i)->mW));
        ++i;
    }
    
    return errG;
}
*/
/*
std::vector<long double> MCMCLoopCurve::calculSplineError0(const SplineMatrices& matrices, const std::vector<std::vector<long double>> &matB, const double lambdaSpline)
{
    const size_t n = mModel->mEvents.size();
    std::vector<long double> matA = calculMatInfluence0(matrices, matB, 1, lambdaSpline);
    std::vector<long double> errG (n); //= initVecteur(n);

    int i = 0;
    for (auto& aii : matA) {
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii <= 0) {
            throw "MCMCLoopCurve::calculSplineError -> Oups aii<0 ";
        }
        errG[i] = sqrt(aii  / mModel->mEvents.at(i)->mW);
        ++i;
    }

    return errG;
}
*/

std::vector<long double> MCMCLoopCurve::calculSplineError_origin(const SplineMatrices& matrices, const SplineResults& splines, const double lambdaSpline)
{
    unsigned int n = mModel->mEvents.size();
    std::vector<long double> matA = calculMatInfluence_origin(matrices, splines, 1, lambdaSpline);
    std::vector<long double> errG (n);// = initVecteur(n);

    for (unsigned int i=0; i<n; ++i) {
        const double& aii = matA.at(i);
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii <= 0) {
            throw "Oups";
        }
        errG[i] = sqrt(aii  / mModel->mEvents.at(i)->mW);
    }

    return errG;
}
std::vector<long double> MCMCLoopCurve::calcul_spline_variance(const SplineMatrices& matrices, const SplineResults& splines, const double lambdaSpline)
{
    unsigned int n = mModel->mEvents.size();
    std::vector<long double> matA = calculMatInfluence_origin(matrices, splines, 1, lambdaSpline);
    std::vector<long double> varG (n);

    for (unsigned int i=0; i<n; ++i) {
        const double& aii = matA.at(i);
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii <= 0.) {
            qDebug()<<" calcul_spline_variance : Oups aii <= 0";
            varG[i] = 0.;

        } else {
            varG[i] = aii  / mModel->mEvents.at(i)->mW;
        }

    }

    return varG;
}
MCMCSpline MCMCLoopCurve::currentSpline ( QList<Event *> &lEvents, bool doSortAndSpreadTheta, std::vector<long double> vecH, SplineMatrices matrices)
{
    MCMCSpline spline;
    if (doSortAndSpreadTheta) {
        orderEventsByThetaReduced(lEvents);
        spreadEventsThetaReduced0(lEvents);

        vecH = calculVecH(lEvents);

        // prepareCalculSpline : ne fait pas intervenir les valeurs Y(x,y,z) des events :mais utilise les theta réduits
        // => On le fait une seule fois pour les 3 composantes

        matrices = prepareCalculSpline(lEvents, vecH);

    }

    std::vector<long double> vecTheta = getThetaEventVector(lEvents);

    // calculSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    std::vector<std::vector<long double>> matB; //matR;
    const double lambda = mModel->mLambdaSpline.mX;
    if (lambda != 0) {
        std::vector<std::vector<long double>> tmp = multiConstParMat(matrices.matQTW_1Q, lambda, 5);
        matB = addMatEtMat(matrices.matR, tmp, 5);

    } else {
        matB = matrices.matR;
    }

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decomp = decompositionCholesky(matB, 5, 1);


    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédent ne change pas

    SplineResults s = calculSplineX(matrices, vecH, decomp, matB, lambda);
    std::vector<long double> vecVarG = calcul_spline_variance(matrices, s, lambda); // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------

    MCMCSplineComposante splineX;
    splineX.vecThetaEvents = vecTheta;
    splineX.vecG = std::move(s.vecG);
    splineX.vecGamma = std::move(s.vecGamma);

    splineX.vecVarG = vecVarG;

    spline.splineX = std::move(splineX);


    if ( mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
         mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
         mCurveSettings.mProcessType == CurveSettings::eProcessType2D ||
         mCurveSettings.mProcessType == CurveSettings::eProcessType3D) {

        // calculSpline utilise les Y des events
        // => On le calcule ici pour la seconde composante (y)

        s = calculSplineY(matrices, vecH, decomp, matB, lambda); //matL et matB ne sont pas changés

        MCMCSplineComposante splineY;

        splineY.vecG = std::move(s.vecG);
        splineY.vecGamma = std::move(s.vecGamma);

        splineY.vecThetaEvents = vecTheta;
        splineY.vecVarG = vecVarG;  // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

        spline.splineY = std::move(splineY);
    }

    if ( mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
         mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
         mCurveSettings.mProcessType == CurveSettings::eProcessType3D) {
        // dans le future, ne sera pas utile pour le mode sphérique
        // calculSpline utilise les Z des events
        // => On le calcule ici pour la troisième composante (z)

        s = calculSplineZ(matrices, vecH, decomp, matB, lambda);

        MCMCSplineComposante splineZ;

        splineZ.vecG = std::move(s.vecG);
        splineZ.vecGamma = std::move(s.vecGamma);

        splineZ.vecThetaEvents = vecTheta;
        splineZ.vecVarG = vecVarG;

        spline.splineZ = std::move(splineZ);
    }

    return spline;
}

std::vector<unsigned>  MCMCLoopCurve::listOfIterationsWithPositiveGPrime (const std::vector<MCMCSplineComposante>& splineTrace)
{
    size_t nbIter = splineTrace.size();

    std::vector<unsigned> resultList;

    for (unsigned iter = 0; iter<nbIter; ++iter) {
        const MCMCSplineComposante& splineComposante = splineTrace.at(iter);

        if (hasPositiveGPrime(splineComposante))
            resultList.push_back(iter);
    }

    return resultList;
}

bool  MCMCLoopCurve::hasPositiveGPrime (const MCMCSplineComposante& splineComposante)
{

    const double& tmin = mModel->mSettings.mTmin;
    const double& tmax = mModel->mSettings.mTmax;
    const double& step = mModel->mSettings.mStep;

    const unsigned nbPoint = floor ((tmax - tmin +1) /step);
    unsigned i1;
    double t;
    bool accepted = true;
  //  i0 = 0;
    i1 = 0;
  //  i2 = 0;
    for (unsigned tIdx=0; tIdx <= nbPoint ; ++tIdx) {
        t = (double)tIdx * step + tmin ;

        //double deltaG = ( valeurG(t+step/2., splineComposante, i0) - valeurG(t-step/2., splineComposante, i2) )/ step;
        double GPrime = valeurGPrime(t, splineComposante, i1);

        if (GPrime < 0.) {
            accepted = false;
            break;
        }
    }

    return accepted;
}


bool  MCMCLoopCurve::hasPositiveGPrimeByDet (const MCMCSplineComposante& splineComposante)
{

    for (unsigned long i= 0; i< splineComposante.vecThetaEvents.size()-1; i++) {

        const double t_i = reduceTime(splineComposante.vecThetaEvents.at(i));
        const double t_i1 = reduceTime(splineComposante.vecThetaEvents.at(i+1));
        const double hi = t_i1 - t_i;

        const double gamma_i = splineComposante.vecGamma.at(i);
        const double gamma_i1 = splineComposante.vecGamma.at(i+1);

        const double g_i = splineComposante.vecG.at(i);
        const double g_i1 = splineComposante.vecG.at(i+1);

        const double a = (g_i1 - g_i) /hi;
        const double b = (gamma_i1 - gamma_i) /(6*hi);
        const double s = t_i + t_i1;
        const double p = t_i * t_i1;
        const double d = ( (t_i1 - 2*t_i)*gamma_i1 + (2*t_i1 - t_i)*gamma_i )/(6*hi);
        // résolution équation

        const double aDelta = 3* b;
        const double bDelta = 2*d - 2*s*b;
        const double cDelta = p*b - s*d + a;

        const double delta = pow(bDelta, 2.) - 4*aDelta*cDelta;
        const double t1_res = (-bDelta - sqrt(delta)) / (2*aDelta);
        const double t2_res = (-bDelta + sqrt(delta)) / (2*aDelta);

        if (g_i1-g_i<0) {
            return false;

        } else if ( t_i < t1_res && t1_res< t_i1) {
                return false;

        } else if ( t_i < t2_res && t2_res< t_i1) {
            return false;
        }

    }

    return true;
}









