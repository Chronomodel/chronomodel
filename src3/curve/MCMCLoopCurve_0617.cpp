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
#include "Matrix.h"

#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QTime>
#include <QProgressDialog>
#include <execution>

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
            date.mTi.reset();
            date.mTi.reserve(initReserve);
            date.mTi.mLastAccepts.reserve(acceptBufferLen);
            date.mTi.mLastAcceptsLength = acceptBufferLen;

            date.mSigmaTi.reset();
            date.mSigmaTi.reserve(initReserve);
            date.mSigmaTi.mLastAccepts.reserve(acceptBufferLen);
            date.mSigmaTi.mLastAcceptsLength = acceptBufferLen;

            date.mWiggle.reset();
            date.mWiggle.reserve(initReserve);
            date.mWiggle.mLastAccepts.reserve(acceptBufferLen);
            date.mWiggle.mLastAcceptsLength = acceptBufferLen;
        }
    }

    for (auto&& phase : mModel->mPhases) {
        phase->mAlpha.reset();
        phase->mBeta.reset();
        phase->mTau.reset();
        phase->mDuration.reset();

        phase->mAlpha.mRawTrace->reserve(initReserve);
        phase->mBeta.mRawTrace->reserve(initReserve);
        phase->mTau.mRawTrace->reserve(initReserve);
        phase->mDuration.mRawTrace->reserve(initReserve);
   }
    mModel->mLambdaSpline.reset();
    mModel->mLambdaSpline.reserve(initReserve);
    mModel->mLambdaSpline.mLastAccepts.reserve(acceptBufferLen);
    mModel->mLambdaSpline.mLastAcceptsLength = acceptBufferLen;
    
    mModel->mS02Vg.reset();
    mModel->mS02Vg.reserve(initReserve);
    mModel->mS02Vg.mLastAccepts.reserve(acceptBufferLen);
    mModel->mS02Vg.mLastAcceptsLength = acceptBufferLen;

    // Ré-initialisation du stockage des splines
    mModel->mSplinesTrace.clear();

    // Ré-initialisation des résultats
    mModel->mPosteriorMeanGByChain.clear();

}

/**
 * Idem Chronomodel + initialisation de VG (events) et Lambda Spline (global)
 */
QString MCMCLoopCurve::initialize()
{
    QList<Event*>& events(mModel->mEvents);
    QList<Phase*>& phases (mModel->mPhases);
    QList<PhaseConstraint*>& phasesConstraints (mModel->mPhaseConstraints);

    const double tminPeriod = mModel->mSettings.mTmin;
    const double tmaxPeriod = mModel->mSettings.mTmax;

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
    /* --------------------------  Init Bounds --------------*/
    try {
        for (int i = 0; i < events.size(); ++i) {
            if (events.at(i)->mType == Event::eBound) {
                EventKnown* bound = dynamic_cast<EventKnown*>(events[i]);

                if (bound) {
                    bound->mTheta.mX = bound->mFixed;
                    bound->mThetaReduced = mModel->reduceTime(bound->mTheta.mX);
                    bound->mTheta.mLastAccepts.clear();
                    bound->mTheta.tryUpdate(bound->mTheta.mX, 2.);

                    bound->mInitialized = true;
                    // prepareEventY(bound);
                }
                bound = nullptr;
            }
        }

    }  catch (...) {
        qWarning() <<"Init Bound ???";
        mAbortedReason = QString("Error in Init Bound ???");
        return mAbortedReason;
    }
    // ----------------------------------------------------------------
    //  Init theta event, ti, ...
    // ----------------------------------------------------------------

    QVector<Event*> unsortedEvents = ModelUtilities::unsortEvents(events);

    emit stepChanged(tr("Initializing Events..."), 0, unsortedEvents.size());
    try {

        // ------
        // Check Strati constraint
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

                const double min = unsortedEvents.at(i)->getThetaMinRecursive (tminPeriod);

                // ?? Comment circularEventName peut-il être pas vide ?
                if (!circularEventName.isEmpty()) {
                    mAbortedReason = QString(tr("Warning : Find Circular constraint with %1  bad path  %2 ")).arg(unsortedEvents.at(i)->mName, circularEventName);
                    return mAbortedReason;
                }

                mModel->initNodeEvents();
                const double max = unsortedEvents.at(i)->getThetaMaxRecursive(tmaxPeriod);
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

                sampleInCumulatedRepartition(unsortedEvents.at(i), mModel->mSettings, min, max);

                unsortedEvents.at(i)->mThetaReduced = mModel->reduceTime(unsortedEvents.at(i)->mTheta.mX);
                unsortedEvents.at(i)->mInitialized = true;

#ifdef DEBUG
                //   qDebug() << QString("initialize theta event : %1 %2 %3 %4").arg(unsortedEvents.at(i)->mName, QString::number(min, 'f', 30), QString::number(unsortedEvents.at(i)->mTheta.mX, 'f', 30), QString::number(max, 'f', 30));
#endif
                // ----------------------------------------------------------------

                double s02_sum = 0.;
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
                    date.initDelta(unsortedEvents.at(i));
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
                    date.mSigmaTi.mX = std::abs(date.mTi.mX - (unsortedEvents.at(i)->mTheta.mX - date.mDelta));

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
                unsortedEvents.at(i)->mS02 = unsortedEvents.at(i)->mDates.size() / s02_sum;

                // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
                unsortedEvents.at(i)->mTheta.mSigmaMH = sqrt(unsortedEvents.at(i)->mS02);
                unsortedEvents.at(i)->mAShrinkage = 1.;

                // 6- Clear mLastAccepts  array
                unsortedEvents.at(i)->mTheta.mLastAccepts.clear();
                //unsortedEvents.at(i)->mTheta.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
                unsortedEvents.at(i)->mTheta.tryUpdate(unsortedEvents.at(i)->mTheta.mX, 2.);

            }

            if (isInterruptionRequested())
                return ABORTED_BY_USER;

            emit stepProgressed(i);
        }

    }  catch (...) {
        qWarning() <<"Init theta event, ti,  ???";
        mAbortedReason = QString("Error in Init theta event, ti,  ???");
        return mAbortedReason;
    }

    // Initialisation pour adaptation au code RenCurve
/*
   // mModel->mEvents[0]->mS02 = 100;
    mModel->mEvents[0]->mThetaReduced = mModel->reduceTime(1800);
    mModel->mEvents[0]->mTheta.mX = +1800.;
    mModel->mEvents[0]->mDates[0].mTi.mX = +1800;//yearTime(0.00202246905664585);
    mModel->mEvents[0]->mDates[0].mSigmaTi.mX = 50.;//83.5;

   // mModel->mEvents[1]->mS02 = 100;
    mModel->mEvents[1]->mThetaReduced = mModel->reduceTime(+1500);
    mModel->mEvents[1]->mTheta.mX = +100.;
    mModel->mEvents[1]->mDates[0].mTi.mX= +100; //yearTime(0.511381982047279);
    mModel->mEvents[1]->mDates[0].mSigmaTi.mX = 50.;//1376.1;

   // mModel->mEvents[2]->mS02 = 100;
    mModel->mEvents[2]->mThetaReduced = mModel->reduceTime(0.);
    mModel->mEvents[2]->mTheta.mX = 0;
    mModel->mEvents[2]->mDates[0].mTi.mX= 0; //yearTime(0.775249279549509);
    mModel->mEvents[2]->mDates[0].mSigmaTi.mX = 50.;//1632.7;

   // mModel->mEvents[3]->mS02 = 100;
    mModel->mEvents[3]->mThetaReduced = mModel->reduceTime(-100);
    mModel->mEvents[3]->mTheta.mX = -100;
    mModel->mEvents[3]->mDates[0].mTi.mX= -100; //yearTime(0.975249813559322);
    mModel->mEvents[3]->mDates[0].mSigmaTi.mX = 50.;//1583.3;

  //  mModel->mEvents[4]->mS02 = 100;
    mModel->mEvents[4]->mThetaReduced = mModel->reduceTime(-200);
    mModel->mEvents[4]->mTheta.mX = -200;
    mModel->mEvents[4]->mDates[0].mTi.mX= -200;//yearTime(0.929440498773417);
    mModel->mEvents[4]->mDates[0].mSigmaTi.mX = 50.;//686.9;

    // Il y a aussi lambda spline plus bas


   // mModel->mEvents[5]->mS02 = 2500;
    mModel->mEvents[5]->mThetaReduced = mModel->reduceTime(1800);
    mModel->mEvents[5]->mTheta.mX = yearTime(0.9995);
    mModel->mEvents[5]->mDates[0].mTi.mX= yearTime(0.9995);
    mModel->mEvents[5]->mDates[0].mSigmaTi.mX = 50;
*/

    // --------------------------- Init alpha and beta phases ----------------------
    emit stepChanged(tr("Initializing Phases..."), 0, phases.size());
    try {
        i = 0;
        for (auto&& phase : phases ) {
            //phase->updateAll(tminPeriod, tmaxPeriod);
            // tau is still initalize

            double tmp = phase->mEvents[0]->mTheta.mX;
            // All Event must be Initialized
            std::for_each(phase->mEvents.begin(), phase->mEvents.end(), [&tmp] (Event* ev){tmp = std::min(ev->mTheta.mX, tmp);});
            phase->mAlpha.mX = tmp;

            tmp = phase->mEvents[0]->mTheta.mX;
            std::for_each(phase->mEvents.begin(), phase->mEvents.end(), [&tmp] (Event* ev){tmp = std::max(ev->mTheta.mX, tmp);});
            phase->mBeta.mX = tmp;

            phase->mDuration.mX = phase->mBeta.mX - phase->mAlpha.mX;

            if (isInterruptionRequested())
                return ABORTED_BY_USER;
            ++i;

            emit stepProgressed(i);
        }

    }  catch (...) {
        qWarning() << "Init alpha and beta phases  ???";
        mAbortedReason = QString("Init alpha and beta phases  ???");
        return mAbortedReason;
    }

    // -------------------------------- SPLINE part--------------------------------
    // Init function G

    prepareEventsY(events);

    emit stepChanged(tr("Initializing G ..."), 0, events.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    // ----------------------------------------------------------------
    //  Init Lambda Spline
    // ----------------------------------------------------------------

    std::vector<double> vecH = calculVecH(mModel->mEvents);
    SplineMatrices matricesWI = prepareCalculSpline_WI(mModel->mEvents, vecH);
    try {
        if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed)
            mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;

        else
            mModel->mLambdaSpline.mX = 1E-6;// Pour RenCurve

            //mModel->mLambdaSpline.mX = S02_lambda_WI(matricesWI, events.size()); // = C
            //mModel->mLambdaSpline.mX = initLambdaSplineBy_h_YWI_AY();

            //mModel->mLambdaSpline.mX = initLambdaSpline();// Utilise S02_lambda_WI()
            //mModel->mLambdaSpline.mX =initLambdaSplineByCV();


        mModel->mLambdaSpline.mSigmaMH = 1;
        mModel->mLambdaSpline.mLastAccepts.clear();
        mModel->mLambdaSpline.tryUpdate(mModel->mLambdaSpline.mX, 2.);

    }  catch (...) {
        qWarning() << "Init Lambda Spline  ???";
        mAbortedReason = QString("Init Lambda Spline  ???");
        return mAbortedReason;
    }


    // ----------------------------------------------------------------
    // Curve init Vg_i
    // ----------------------------------------------------------------
    try {
        if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
            Var_residual_spline = mCurveSettings.mVarianceFixed;

        } else { // si individuel ou global VG = S02
            // S02_Vg_Yx() Utilise la valeur de lambda courant
            Var_residual_spline = S02_Vg_Yx(mModel->mEvents, matricesWI, vecH, mModel->mLambdaSpline.mX);

        }


        for (Event*& e : mModel->mEvents) {
            e->mVG.mX = Var_residual_spline;
        }

        // memo VG
        for (int i= 0; i< events.size(); ++i) {

            events[i]->mVG.mLastAccepts.clear();
            // event->mVG.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
            events[i]->mVG.tryUpdate(events[i]->mVG.mX, 2.);

            events[i]->mVG.mSigmaMH = 1.;

            /* ----------------------------------------------------------------
             * The W of the events depend only on their VG
             * During the update, we need W for the calculations of theta, VG and Lambda Spline update
             * We will thus have to update the W at each VG modification
             * We calculate it here during the initialization to have its starting value
             * ---------------------------------------------------------------- */
            events[i]->updateW();

            if (isInterruptionRequested())
                return ABORTED_BY_USER;

            emit stepProgressed(i);
        }

    }  catch (...) {
        qWarning() << "Curve init Vgi  ???";
        mAbortedReason = QString("Curve init Vgi  ???");
        return mAbortedReason;
    }

    // ----------------------------------------------------------------
    // Curve init S02 Vg
    // ----------------------------------------------------------------
    mModel->mS02Vg.mX = Var_residual_spline;
    mModel->mS02Vg.mLastAccepts.clear();
    mModel->mS02Vg.tryUpdate(events[i]->mVG.mX, 2.);

    mModel->mS02Vg.mSigmaMH = 1.;

    if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeUnivarie) {
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYx;});

        var_Y = pow(std_Knuth( vecY), 2);

    } else {
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYx;});
        var_Y = pow(std_Knuth( vecY), 2);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYy;});
        var_Y += pow(std_Knuth( vecY), 2);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYz;});
        var_Y += pow(std_Knuth( vecY), 2);

        var_Y /= 3.;
    }


    if ( (var_Y <= 0) && (mCurveSettings.mVarianceType != CurveSettings::eModeFixed)) {
        mAbortedReason = QString(tr("Error : Variance on Y is null, do the computation with VG fix  = 0 for this model "));
        return mAbortedReason;
    }
    // --------------------------- Current spline ----------------------
 try {
    /* --------------------------------------------------------------
     *  Calcul de la spline g, g" pour chaque composante x y z
     *--------------------------------------------------------------*/
    mModel->mSpline = currentSpline(mModel->mEvents, true);

    // init Posterior MeanG and map
    const int nbPoint = 300;//501;//500; 1611;// map size and curve size
    std::pair<std::vector<double>::iterator, std::vector<double>::iterator> minMaxY_X;
    std::pair<std::vector<double>::iterator, std::vector<double>::iterator> minMaxVarY_X;

    const bool hasY = (mCurveSettings.mProcessType != CurveSettings::eProcessTypeUnivarie);
    const bool hasZ = (mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
                       mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
                       mCurveSettings.mProcessType == CurveSettings::eProcessType3D);

    PosteriorMeanGComposante clearCompo;
    clearCompo.mapG = CurveMap (nbPoint, nbPoint);
    clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
    clearCompo.mapG.min_value = +INFINITY;
    clearCompo.mapG.max_value = 0;

    clearCompo.vecG = std::vector<double> (nbPoint);
    clearCompo.vecGP = std::vector<double> (nbPoint);
    clearCompo.vecGS = std::vector<double> (nbPoint);
    clearCompo.vecVarG = std::vector<double> (nbPoint);
    clearCompo.vecVarianceG = std::vector<double> (nbPoint);
    clearCompo.vecVarErrG = std::vector<double> (nbPoint);

    PosteriorMeanG clearMeanG;
    clearMeanG.gx = clearCompo;
    minMaxY_X = std::minmax_element(mModel->mSpline.splineX.vecG.begin(), mModel->mSpline.splineX.vecG.end());
    minMaxVarY_X = std::minmax_element(mModel->mSpline.splineX.vecVarG.begin(), mModel->mSpline.splineX.vecVarG.end());

    double minY_X = *minMaxY_X.first;
    double maxY_X = *minMaxY_X.second;

    minY_X = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY_X, [](double x, Event* e) {return std::min(e->mYx, x);});
    maxY_X = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY_X, [](double x, Event* e) {return std::max(e->mYx, x);});

    double maxVarY_X = *minMaxVarY_X.second;
    double spanY_X = (maxY_X - minY_X) / 5.;
    minY_X = minY_X - 1.96*sqrt(maxVarY_X) - spanY_X;
    maxY_X = maxY_X + 1.96*sqrt(maxVarY_X) + spanY_X;

    clearMeanG.gx.mapG.setRangeY(minY_X, maxY_X);

    if ( hasY) {
        clearMeanG.gy = clearCompo;
        minMaxY_X = std::minmax_element(mModel->mSpline.splineY.vecG.begin(), mModel->mSpline.splineY.vecG.end());
        minMaxVarY_X = std::minmax_element(mModel->mSpline.splineY.vecVarG.begin(), mModel->mSpline.splineY.vecVarG.end());
        minY_X = *minMaxY_X.first;
        maxY_X = *minMaxY_X.second;
        maxVarY_X = *minMaxVarY_X.second;
        spanY_X = (maxY_X - minY_X) / 5.;
        minY_X = minY_X - 1.96*sqrt(maxVarY_X) - spanY_X;
        maxY_X = maxY_X + 1.96*sqrt(maxVarY_X) + spanY_X;

        clearMeanG.gy.mapG.setRangeY(minY_X, maxY_X);
        if (hasZ) {
            clearMeanG.gz = clearCompo;
            minMaxY_X = std::minmax_element(mModel->mSpline.splineZ.vecG.begin(), mModel->mSpline.splineZ.vecG.end());
            minMaxVarY_X = std::minmax_element(mModel->mSpline.splineZ.vecVarG.begin(), mModel->mSpline.splineZ.vecVarG.end());
            minY_X = *minMaxY_X.first;
            maxY_X = *minMaxY_X.second;
            maxVarY_X = *minMaxVarY_X.second;
            spanY_X = (maxY_X - minY_X) / 5.;
            minY_X = minY_X - 1.96*sqrt(maxVarY_X) - spanY_X;
            maxY_X = maxY_X + 1.96*sqrt(maxVarY_X) + spanY_X;

            clearMeanG.gz.mapG.setRangeY(minY_X, maxY_X);
        }

    }

    mModel->mPosteriorMeanGByChain.push_back(clearMeanG);
    if (mChainIndex == 0)
        mModel->mPosteriorMeanG = clearMeanG;//std::move(clearMeanG);

}  catch (...) {
    qWarning() <<"init Posterior MeanG and map  ???";
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
        const double tminPeriod = mModel->mSettings.mTmin;
        const double tmaxPeriod = mModel->mSettings.mTmax;

        // --------------------------------------------------------------
        //  A - Update ti Dates (idem chronomodel)
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
                    if (date.mSigmaTi.mX <= 0)
                        qDebug(" date.mSigma.mX <= 0 ");
#endif
                    //date.updateSigmaReParam(event);
                    date.updateWiggle();

                }
            }

        }  catch (...) {
            qWarning() <<"update Date ???";
        }
        // Variable du MH de la spline
        double current_value, current_h, current_h_theta, current_h_YWI, current_h_lambda, current_h_VG;
        SplineMatrices current_matrices, current_matriceWI; //, current_matrices_Vg0, try_matrices_Vg0;
        SplineResults current_spline; //, current_spline_Vg0, try_spline_Vg0;
        std::vector<double> current_vecH;

        double try_value, try_h, try_h_theta, try_h_YWI, try_h_lambda, try_h_VG;
        SplineMatrices try_matrices;
        std::vector<double> try_vecH;

        double rapport;
        // --------------------------------------------------------------
        //  B - Update theta Events
        // --------------------------------------------------------------
        try {

            // copie la liste des pointeurs
            std::vector<Event*> initListEvents (mModel->mEvents.size());
            std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

            // find minimal step;
            // long double minStep = minimalThetaReducedDifference(mModel->mEvents)/10.;


            // init the current state
            orderEventsByThetaReduced(mModel->mEvents);
            spreadEventsThetaReduced0(mModel->mEvents);

            current_vecH = calculVecH(mModel->mEvents);

            current_matrices = prepareCalculSpline(mModel->mEvents, current_vecH);
            current_h_YWI = h_YWI_AY(current_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH); // utile si mCurveSettings.mTimeType == CurveSettings::eModeFixe

            current_h_lambda = h_lambda(current_matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

            // Pour h_theta(), mTheta doit être en année, et h_YWI_AY utilise mThetaReduced

            // RenCurve
            // current_h = current_h_YWI * current_h_lambda * current_h_theta;

            // ChronoCurve
            // current_h = current_h_YWI * current_h_lambda * current_h_theta;

            if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {

                /* ----------------------------------------------------------------------
                 *  Dans Chronomodel, on appelle simplement : event->updateTheta(t_min,t_max); sur tous les events.
                 *  Pour mettre à jour un theta d'event dans Curve, on doit accéder aux thetas des autres events.
                 *  => on effectue donc la mise à jour directement ici, sans passer par une fonction
                 *  de la classe event (qui n'a pas accès aux autres events)
                 * ---------------------------------------------------------------------- */

                  for (Event*& event : initListEvents) {

                //for (unsigned i = 0 ; i<initListEvents.size(); i++) {
               //     Event*& event = initListEvents[i];

                    if (event->mType == Event::eDefault) {

                        // ----
                        const double min = event->getThetaMin(tminPeriod);
                        const double max = event->getThetaMax(tmaxPeriod);

                        if (min >= max) {
                            throw QObject::tr("Error for event theta : %1 : min = %2 : max = %3").arg(event->mName, QString::number(min), QString::number(max));
                        }

                        // On stocke l'ancienne valeur :
                        current_value = event->mTheta.mX;
                        current_h_theta = h_theta_Event(event);
                        current_h = current_h_YWI * current_h_lambda * current_h_theta;

                        // On tire une nouvelle valeur :
                        try_value = Generator::gaussByBoxMuller(current_value, event->mTheta.mSigmaMH);


                        if (try_value >= min && try_value <= max) {
                            // On force la mise à jour de la nouvelle valeur pour calculer h_new

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);

                            orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                            spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date

                            try_vecH = calculVecH(mModel->mEvents);
                            try_matrices = prepareCalculSpline(mModel->mEvents, try_vecH);

                            try_h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, try_vecH);

                            try_h_lambda = h_lambda(try_matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

                            try_h_theta = h_theta_Event(event);

                            // Calcul du rapport :

                            const double try_h = try_h_YWI * try_h_lambda* try_h_theta;

                            rapport = try_h / current_h;

                        } else {
                            rapport = -1.;

                        }

                        // restore Theta to used function tryUpdate
                        event->mTheta.mX = current_value;
                        event->mTheta.tryUpdate(try_value, rapport);

                        if ( event->mTheta.mLastAccepts.last() == true) {
                            // Pour l'itération suivante :                       
                            current_h_YWI = std::move(try_h_YWI);
                            current_vecH = std::move(try_vecH);
                            current_matrices = std::move(try_matrices);
                            current_h_lambda = std::move(try_h_lambda);

                        }


                    } else { // this is a bound, nothing to sample. Always the same value
                        event->updateTheta(tminPeriod, tmaxPeriod);
                    }

                    // update after tryUpdate or updateTheta
                    event->mThetaReduced = mModel->reduceTime(event->mTheta.mX);


                    //--------------------- Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
                    //for (auto&& phInEv : event->mPhases)
                    //    phInEv->updateAll(tminPeriod, tmaxPeriod);
                     std::for_each(event->mPhases.begin(), event->mPhases.end(), [tminPeriod, tmaxPeriod] (Phase* p) {p->updateAll (tminPeriod, tmaxPeriod);});

                } // End of loop initListEvents


                // Rétablissement de l'ordre initial. Est-ce nécessaire ?
                //   std::copy(initListEvents.begin(), initListEvents.end(), mModel->mEvents.begin() );



            } else { // Pas bayésien : on sauvegarde la valeur constante dans la trace
                for (Event*& event : mModel->mEvents) {
                    event->mTheta.tryUpdate(event->mTheta.mX, 1.);

                    //--------------------- Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
                    // maybe not usefull ??
                    //for (auto&& phInEv : event->mPhases)
                    //    phInEv->updateAll(tminPeriod, tmaxPeriod);
                    std::for_each(event->mPhases.begin(), event->mPhases.end(), [tminPeriod, tmaxPeriod] (Phase* p) {p->updateAll (tminPeriod, tmaxPeriod);});
                }

            }

        } catch(...) {
            qDebug() << "MCMCLoopCurve::update Theta : Caught Exception!\n";
        }

        // --------------------------------------------------------------
        //  C - Update Phases constraints
        // --------------------------------------------------------------

        //for (auto&& phConst : mModel->mPhaseConstraints )
        //    phConst->updateGamma();
         std::for_each(mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (PhaseConstraint* pc) {pc->updateGamma();});

        // --------------------------------------------------------------
        //  Remarque : à ce stade, tous les theta events sont à jour et ordonnés.
        //  On va à présent mettre à jour tous les VG, puis Lambda Spline.
        //  Pour cela, nous devons espacer les thetas pour permettre les calculs.
        //  Nous le faisons donc ici, et restaurerons les vrais thetas à la fin.
        // --------------------------------------------------------------

        // --------------------------------------------------------------
        //  D - Update Vg Global or individual (Events)
        // --------------------------------------------------------------
        try {
            if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
                // Events must be order

                /* RenCurve
                * current_h = current_h_YWI * current_h_lambda * current_h_VG;
                *
                * ChronoCurve
                * current_h = current_h_YWI * current_h_VG;
                */

                const double logMin = -10.;
                const double logMax = +20.;
                try {
                    // --------------------------------------------------------------
                    //  D-1 - Update S02 Vg
                    // --------------------------------------------------------------
                    try {
                         // On stocke l'ancienne valeur :
                        //current_value = mModel->mS02Vg.mX;

                        current_h = h_S02_Vg(mModel->mEvents, mModel->mS02Vg.mX, var_Y);

                        // On tire une nouvelle valeur :

                        const double try_value_log = Generator::gaussByBoxMuller(log10(mModel->mS02Vg.mX), mModel->mS02Vg.mSigmaMH);
                        try_value = pow(10, try_value_log);

                        long double rapport = -1.;

                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            try_h = h_S02_Vg(mModel->mEvents, try_value, var_Y);

                            rapport = try_h * try_value / (current_h * mModel->mS02Vg.mX);


                        } else {
                            rapport = -1.;
                        }

                        //mModel->mS02Vg.mX = current_value;
                        mModel->mS02Vg.tryUpdate(try_value, rapport);


                    } catch (std::exception& e) {
                        qWarning()<< "MCMCLoopCurve::update S02 Vg : exception caught: " << e.what() << '\n';

                    }

                    // Fin maj SO2 Vg


                    if (mCurveSettings.mUseVarianceIndividual) {

                        // Variance individuelle

                        for (Event*& event : mModel->mEvents)   {
                            current_value = event->mVG.mX;
                            current_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                            current_h = current_h_YWI * current_h_VG;

                            // On tire une nouvelle valeur :
                            const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), event->mVG.mSigmaMH);
                            try_value = pow(10., try_value_log);

                            if (try_value_log >= logMin && try_value_log <= logMax) {
                                // On force la mise à jour de la nouvelle valeur pour calculer try_h
                                // A chaque fois qu'on modifie VG, W change !
                                event->mVG.mX = try_value;
                                event->updateW(); // used by prepareCalculSpline

                                // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG
                                try_matrices = prepareCalculSpline(mModel->mEvents, current_vecH);

                                try_h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH);

                                try_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                                if (try_h_YWI == HUGE_VAL || try_h_VG == HUGE_VAL)
                                    try_h = 0.;
                                else
                                    try_h = try_h_YWI * try_h_VG;

                                if (current_h == 0) {
                                    rapport = 1.;

                                } else {
                                    rapport = (try_h * try_value) / (current_h * current_value);
                                }


                            } else {
                                rapport = -1.; // force reject // force to keep current state
                                // try_h_YWI = current_h_YWI;
                            }

                            // Mise à jour Metropolis Hastings
                            // A chaque fois qu'on modifie VG, W change !
                            event->mVG.mX = current_value;
                            event->mVG.tryUpdate( try_value, rapport);
                            event->updateW();

                            if ( event->mVG.mLastAccepts.last() == true) {
                                // Pour l'itération suivante : Car mVG a changé
                                current_h_YWI = std::move(try_h_YWI);
                                current_matrices = std::move(try_matrices);
                            }

                        }

                    }
                    else {

                        /* Si nous sommes en variance global,
                             * il faut trouver l'indice du premier Event qui ne soit pas bound
                             * L'ordre et donc l'indice change avec le spread
                             */
                        auto& eventVGglobal = mModel->mEvents.at(0);

                         // On stocke l'ancienne valeur :
                        current_value = eventVGglobal->mVG.mX;

                        current_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                        current_h = current_h_YWI * current_h_VG;

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

                            // Calcul du rapport : try_matrices utilise les temps reduits, elle est affectée par le changement de VG
                            try_matrices = prepareCalculSpline(mModel->mEvents, current_vecH);

                            try_h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH);

                            try_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                            /* RenCurve
                             * try_h = try_h_YWI * try_h_lambda * try_h_VG;
                             */

                            try_h = try_h_YWI  * try_h_VG;

                            rapport = (current_h == 0) ? 1 : ((try_h * try_value) / (current_h * current_value));


                            // ON fait le test avec le premier event

                            eventVGglobal->mVG.mX = current_value;
                            eventVGglobal->mVG.tryUpdate(try_value, rapport);
                            eventVGglobal->updateW();


                            if ( eventVGglobal->mVG.mLastAccepts.last() == true) {
                                current_h_YWI = try_h_YWI;
                                current_matrices = std::move(try_matrices);
                                rapport = 2.;

                            } else {
                                rapport = -1.;
                            }

                        } else {
                            rapport = -1.;
                        }

                        for (Event*& ev : mModel->mEvents) {
                            ev->mVG.mX =  eventVGglobal->mVG.mX;
                            try_value = eventVGglobal->mVG.mX;
                            ev->mVG.tryUpdate(try_value, rapport);
                            // On remet l'ancienne valeur, qui sera éventuellement mise à jour dans ce qui suit (Metropolis Hastings)
                            // A chaque fois qu'on modifie VG, W change !
                            ev->updateW();
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

        } catch(...) {
            qDebug() << "MCMCLoopCurve::update VG : Caught Exception!\n";
        }
        // --------------------------------------------------------------
        //  E - Update Lambda
        // --------------------------------------------------------------
        try {
            if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {

                // cas VG individual, current_h_VG doit être recalculé
                /* On modifie le current_h_VG en deux parties numérateur et denominateur
                 * SANS faire les puissances a et a+1
                 * Le terme "a*" devant est volontairement oublier, il se simplifie dans le rapport final
                 * tel que:
                 *          current_h_VG = pow(current_h_VG_num, a) / pow(current_h_VG_denum, a+1);
                 * avec:
                 *          current_h_VG_num = pow(current_S02_Vg, mModel->mEvents.size()) ;
                 *
                 *  et :
                 *          current_h_VG_denum = PRODUIT(current_S02_Vg + event->mVG.mX);
                 *    Le code est compacté plus bas dans le calcul du rapport des h_VG
                 */

                const double logMin = -20.;
                const double logMax = +10.;

                // On stocke l'ancienne valeur :
                current_value = mModel->mLambdaSpline.mX;
                current_h = current_h_YWI * current_h_lambda;

                // On tire une nouvelle valeur :
                const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                try_value = pow(10., try_value_log);

                if (try_value_log >= logMin && try_value_log <= logMax) {
                    // Calcul du rapport :
                    mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_VG
                    try_h_YWI = h_YWI_AY(current_matrices, mModel->mEvents, try_value, current_vecH);

                    try_h_lambda = h_lambda(current_matrices, mModel->mEvents.size(), try_value) ;

                    try_h = try_h_YWI * try_h_lambda;

                    if (current_h == 0) {
                        rapport = 2.; // force accept to change value

                    } else {
                        rapport =  (try_h * try_value) / (current_h * current_value);
                    }


                } else {
                    rapport = -1.; // force reject
                }

                mModel->mLambdaSpline.mX = current_value;
                mModel->mLambdaSpline.tryUpdate(try_value, rapport);


            }
            // Pas bayésien : on sauvegarde la valeur constante dans la trace
            else {
                mModel->mLambdaSpline.tryUpdate(mModel->mLambdaSpline.mX, 2.);
            }

        } catch(...) {
            qDebug() << "MCMCLoopCurve::update Lambda : Caught Exception!\n";
        }


        // --------------------------------------------------------------
        //  F - update MCMCSpline mModel->mSpline
        // --------------------------------------------------------------
        // F.1- Calcul spline avec mModel->mLambdaSpline.mX en interne
        mModel->mSpline = currentSpline(mModel->mEvents, false, current_vecH, current_matrices);

        // F.2 - test GPrime positive
        if (mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth)
            return hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy >mCurveSettings.mThreshold = pas d'acceptation

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
            if (date.mTi.mSamplerProposal == MHVariable::eMHSymGaussAdapt)
                noAdapt *= date.mTi.adapt(taux_min, taux_max, delta);

            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            noAdapt *= date.mSigmaTi.adapt(taux_min, taux_max, delta);

        }

        //--------------------- Adapt Sigma MH de Theta Event -----------------------------------------      
       if ((event->mType != Event::eBound) && ( event->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss) )
           noAdapt *= event->mTheta.adapt(taux_min, taux_max, delta);


       //--------------------- Adapt Sigma MH de VG  -----------------------------------------
       if ((event->mType != Event::eBound) && ( event->mVG.mSamplerProposal == MHVariable::eMHAdaptGauss) )
           noAdapt *= event->mVG.adapt(taux_min, taux_max, delta);


    }

    //--------------------- Adapt Sigma MH de S02 Vg -----------------------------------------
    if (mModel->mS02Vg.mSamplerProposal == MHVariable::eMHAdaptGauss)
        noAdapt *= mModel->mS02Vg.adapt(taux_min, taux_max, delta);

    //--------------------- Adapt Sigma MH de Lambda Spline -----------------------------------------
    if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eMHAdaptGauss)
        noAdapt *= mModel->mLambdaSpline.adapt(taux_min, taux_max, delta);

    return noAdapt;
}


void MCMCLoopCurve::memo()
{
    for (auto&& event : mModel->mEvents) {
        //--------------------- Memo Events -----------------------------------------
        event->mTheta.memo();
        event->mTheta.saveCurrentAcceptRate();

        // On stocke la racine de VG, qui est une variance pour afficher l'écart-type
        double memoVG = sqrt(event->mVG.mX);


        event->mVG.memo(&memoVG);
        event->mVG.saveCurrentAcceptRate();

        for (auto&& date : event->mDates )   {
            //--------------------- Memo Dates -----------------------------------------
            date.mTi.memo();
            date.mSigmaTi.memo();
            date.mWiggle.memo();

            date.mTi.saveCurrentAcceptRate();
            date.mSigmaTi.saveCurrentAcceptRate();
        }

    }

    //--------------------- Memo Phases -----------------------------------------
   // for (auto&& ph : mModel->mPhases)
   //         ph->memoAll();
    std::for_each(mModel->mPhases.begin(), mModel->mPhases.end(), [](Phase* p) {p->memoAll();} );
    //--------------------- Memo mS02 Vg -----------------------------------------
    double memoS02 = sqrt(mModel->mS02Vg.mX);
    mModel->mS02Vg.memo(&memoS02);
    mModel->mS02Vg.saveCurrentAcceptRate();

    //--------------------- Memo mLambdaSpline Smoothing -----------------------------------------
    // On stocke le log10 de Lambda Spline pour afficher les résultats a posteriori
    double memoLambda = log10(mModel->mLambdaSpline.mX);
    mModel->mLambdaSpline.memo(&memoLambda);
    mModel->mLambdaSpline.saveCurrentAcceptRate();

    //--------------------- Memo Spline -----------------------------------------

    mModel->mSplinesTrace.push_back(mModel->mSpline);
  //  mModel->mSplinesTrace = std::vector<MCMCSpline> {mModel->mSpline};

    if (mState != State::eAquisition)
        return;
    
    
    //--------------------- Create posteriorGMean and map and memo -----------------------------------------

    // 1 - initialisation à faire dans init()

    const bool hasY = (mCurveSettings.mProcessType != CurveSettings::eProcessTypeUnivarie);
    const bool hasZ = (mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
                       mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
                       mCurveSettings.mProcessType == CurveSettings::eProcessType3D);

    int iterAccepted = mChains[mChainIndex].mRealyAccepted + 1;
    memo_PosteriorG( mModel->mPosteriorMeanGByChain[mChainIndex].gx, mModel->mSpline.splineX, iterAccepted );

    int totalIterAccepted = 1;
    for (auto c : mChains)
        totalIterAccepted += c.mRealyAccepted;

    memo_PosteriorG( mModel->mPosteriorMeanG.gx, mModel->mSpline.splineX, totalIterAccepted);

    if (hasY) {
        memo_PosteriorG( mModel->mPosteriorMeanGByChain[mChainIndex].gy, mModel->mSpline.splineY, iterAccepted);
        if (mChains.size() > 1)
            memo_PosteriorG( mModel->mPosteriorMeanG.gy, mModel->mSpline.splineY, totalIterAccepted);

        if (hasZ) {
            memo_PosteriorG( mModel->mPosteriorMeanGByChain[mChainIndex].gz, mModel->mSpline.splineZ, iterAccepted);
            if (mChains.size() > 1)
                memo_PosteriorG( mModel->mPosteriorMeanG.gz, mModel->mSpline.splineZ, totalIterAccepted);
        }
    }

}

/* C'est le même algorithme que ModelCurve::buildCurveAndMap()
 */
void MCMCLoopCurve::memo_PosteriorG(PosteriorMeanGComposante& postGCompo, MCMCSplineComposante& splineComposante, const int realyAccepted)
{
    CurveMap& curveMap = postGCompo.mapG;
    const int nbPtsX = curveMap.column();
    const int nbPtsY = curveMap.row();

    const double ymin = curveMap.minY();
    const double ymax = curveMap.maxY();

    const double stepT = (mModel->mSettings.mTmax - mModel->mSettings.mTmin) / (nbPtsX - 1);
    const double stepY = (ymax - ymin) / (nbPtsY - 1);

    // 2 - Variables temporaires
    // référence sur variables globales
    std::vector<double>& vecVarG = postGCompo.vecVarG;
    // Variables temporaires
    // erreur inter spline
    std::vector<double>& vecVarianceG = postGCompo.vecVarianceG;
    // erreur intra spline
    std::vector<double>& vecVarErrG = postGCompo.vecVarErrG;

    //Pointeur sur tableau
    std::vector<double>::iterator itVecG = postGCompo.vecG.begin();
    std::vector<double>::iterator itVecGP = postGCompo.vecGP.begin();
    std::vector<double>::iterator itVecGS = postGCompo.vecGS.begin();
    //std::vector<long double>::iterator itVecVarG = posteriorMeanCompo.vecVarG.begin();
    // Variables temporaires
    // erreur inter spline
    std::vector<double>::iterator itVecVarianceG = postGCompo.vecVarianceG.begin();
    // erreur intra spline
    std::vector<double>::iterator itVecVarErrG = postGCompo.vecVarErrG.begin();

    // inter derivate variance
    //std::vector<double>::iterator itVecVarianceGP = postGCompo.vecVarGP.begin();

    double t, g, gp, gs, varG, stdG;
    g = 0.;
    gp = 0;
    varG = 0;
    gs = 0;

    double n = realyAccepted;
    double  prevMeanG; //, prevMeanGP;

    const double k = 3.; // Le nombre de fois sigma G, pour le calcul de la densité
    double a, b, surfG;

    int  idxYErrMin, idxYErrMax;

    // 3 - calcul pour la composante
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
        t = (double)idxT * stepT + mModel->mSettings.mTmin ;
        mModel->valeurs_G_VarG_GP_GS(t, splineComposante, g, varG, gp, gs, i0);


        // -- calcul Mean
        prevMeanG = *itVecG;
        *itVecG +=  (g - prevMeanG)/n;

        *itVecGP +=  (gp - *itVecGP)/n;
        *itVecGS +=  (gs - *itVecGS)/n;
        // erreur inter spline
        *itVecVarianceG +=  (g - prevMeanG)*(g - *itVecG);
        // erreur intra spline
        *itVecVarErrG += (varG - *itVecVarErrG) / n  ;

        // inter derivate variance
        //*itVecVarianceGP +=  (gp - prevMeanGP)*(gp - *itVecGP);

        ++itVecG;
        ++itVecGP;
        ++itVecGS;
        ++itVecVarianceG;
        ++itVecVarErrG;


        // -- calcul map
        // g = std::max(ymin, std::min(g, ymax));
        stdG = sqrt(varG);

        // ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
         * https://en.wikipedia.org/wiki/Error_function
         */
        idxYErrMin = inRange( 0, int((g - k*stdG - ymin) / stepY), nbPtsY-1);
        idxYErrMax = inRange( 0, int((g + k*stdG - ymin) / stepY), nbPtsY-1);

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY-1) {
#ifdef DEBUG
                if ((curveMap.row()*idxT + idxYErrMin) < (curveMap.row()*curveMap.column()))
                    curveMap(idxT, idxYErrMin) = curveMap.at(idxYErrMin, idxYErrMin) + 1; // correction à faire dans finalize() + 1./nbIter;
                else
                    qDebug()<<"pb in MCMCLoopCurve::memo_PosteriorG";
#else
                curveMap(idxT, idxYErrMin) = curveMap.at(idxT, idxYErrMin) + 1.; // correction à faire dans finalize/nbIter ;
#endif

                //maxDensity = std::max(maxDensity, curveMap.at(idxT, idxYErrMin));
                curveMap.max_value = std::max(curveMap.max_value, curveMap.at(idxT, idxYErrMin));
                //minDensity = std::min(minDensity, curveMap.at(idxT, idxYErrMin));

        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY) {
            double* ptr_Ymin = curveMap.ptr_at(idxT, idxYErrMin);
            double* ptr_Ymax = curveMap.ptr_at(idxT, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                a = (idErr - 0.5)*stepY + ymin;
                b = (idErr + 0.5)*stepY + ymin;
                surfG = diff_erf(a, b, g, stdG );// correction à faire dans finalyze /nbIter;
#ifdef DEBUG
               // if ((curveMap.row()*idxT + idxY) < (curveMap.row()*curveMap.column()))
                    //curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                    *ptr_idErr = (*ptr_idErr) + surfG;
               // else
               //     qDebug()<<"pb in MCMCLoopCurve::compute_posterior_map_G_composante";
#else
                //curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                *ptr_idErr = (*ptr_idErr) + surfG;
#endif

                //maxDensity = std::max(maxDensity, curveMap.at(idxT, idxY));
                //minDensity = std::min(minDensity, curveMap.at(idxT, idxY));
                curveMap.max_value = std::max(curveMap.max_value, *ptr_idErr);
                // curveMap.min_value = std::min(curveMap.min_value, *ptr_idErr); not possible to find in the loop

                idErr++;
            }
        }


    }
    int tIdx = 0;
    for (auto& vVarG : vecVarG) {
        vVarG = vecVarianceG.at(tIdx)/ n + vecVarErrG.at(tIdx);
        ++tIdx;
    }
}

void MCMCLoopCurve::finalize()
{

#ifdef DEBUG
    qDebug()<<QString("MCMCLoopCurve::finalize");
    QElapsedTimer startTime;
    startTime.start();
#endif

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



    // find the min in the map, can't be done when we do the map
    auto mini = *std::min_element(begin(mModel->mPosteriorMeanG.gx.mapG.data), end(mModel->mPosteriorMeanG.gx.mapG.data));

    mModel->mPosteriorMeanG.gx.mapG.min_value = mini;

    if (hasY) {
        mini = *std::min_element(begin(mModel->mPosteriorMeanG.gy.mapG.data), end(mModel->mPosteriorMeanG.gy.mapG.data));
        mModel->mPosteriorMeanG.gy.mapG.min_value = mini;

        if (hasZ) {
            mini = *std::min_element(begin(mModel->mPosteriorMeanG.gz.mapG.data), end(mModel->mPosteriorMeanG.gz.mapG.data));
            mModel->mPosteriorMeanG.gz.mapG.min_value = mini;

        }
    }


    for (auto& pmc : mModel->mPosteriorMeanGByChain) {
        mini = *std::min_element(begin(pmc.gx.mapG.data), end(pmc.gx.mapG.data));
        pmc.gx.mapG.min_value = mini;

        if (hasY) {
            mini = *std::min_element(begin(pmc.gy.mapG.data), end(pmc.gy.mapG.data));
            pmc.gy.mapG.min_value = mini;

            if (hasZ) {
                mini = *std::min_element(begin(pmc.gz.mapG.data), end(pmc.gz.mapG.data));
                pmc.gz.mapG.min_value = mini;

            }
        }


    }
    // Conversion after the average
    if ( mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
         mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical) {
        emit stepChanged(tr("Compute System Conversion..."), 0, 0);

        if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector) {
            conversionIDF(mModel->mPosteriorMeanG);
            for (auto&& chain: mModel->mPosteriorMeanGByChain)
                conversionIDF(chain);

        } else {
            conversionID(mModel->mPosteriorMeanG);
            for (auto&& chain: mModel->mPosteriorMeanGByChain)
                conversionID(chain);
        }

    }

#ifdef DEBUG
    QTime endTime = QTime::currentTime();

    qDebug()<<"ModelCurve computed";
    qDebug()<<tr("MCMCLoopCurve::finalize() finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug()<<tr("Total time elapsed %1").arg(QString(DHMS(startTime.elapsed())));
#endif


}


double MCMCLoopCurve::Calcul_Variance_Rice (const QList<Event *> lEvents)
{
   // Calcul de la variance Rice (1984)
  double Var_Rice = 0;
  for (int i = 1; i < lEvents.size(); ++i) {
        Var_Rice += pow(lEvents.at(i)->mYx-lEvents.at(i-1)->mYx, 2.);
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



//Obsolete
/*
PosteriorMeanGComposante MCMCLoopCurve::computePosteriorMeanGComposante(const std::vector<MCMCSplineComposante>& trace, const QString &ProgressBarText)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    const double step = mModel->mSettings.mStep;

    const unsigned nbPoint = floor ((tmax - tmin +1) /step);

    std::vector<double> vecCumulG2 (nbPoint);
    std::vector<double> vecCumulVarG2 (nbPoint);
    
    std::vector<double> vecG (nbPoint);
    std::vector<double> vecGP (nbPoint);
    std::vector<double> vecGS (nbPoint);
    std::vector<double> vecVarG (nbPoint);
    
    unsigned long nbIter = trace.size();

    double t, g, gp, gs, errG;
    g = 0.;
    gp = 0;
    errG = 0;
    gs = 0;

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
*/

// Obsolete
/*
PosteriorMeanGComposante MCMCLoopCurve::compute_posterior_mean_G_composante(const std::vector<MCMCSplineComposante>& trace, const QString &ProgressBarText)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    const double step = mModel->mSettings.mStep;

    const unsigned nbPoint = floor ((tmax - tmin +1) /step);

    std::vector<double> vecG (nbPoint);
    std::vector<double> vecGP (nbPoint);
    std::vector<double> vecGS (nbPoint);
    // erreur inter spline
    std::vector<double> vecVarianceG (nbPoint);
    std::vector<double> vecVarG (nbPoint);
    // erreur intra spline
    std::vector<long double> vecVarErrG (nbPoint);

    unsigned long nbIter = trace.size();

    double t, g, gp, gs, varG;
    g = 0.;
    gp = 0;
    varG = 0;
    gs = 0;

    emit stepChanged(ProgressBarText, 0, nbIter);

    int n = 0;
    double  prevMeanG;

    for (auto&& splineComposante : trace ) {
        n++;
        unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
        for (unsigned tIdx = 0; tIdx < nbPoint ; ++tIdx) {
            t = (double)tIdx * step + tmin ;
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
*/


// Obsolete
/*
PosteriorMeanGComposante MCMCLoopCurve::compute_posterior_mean_map_G_composante(const std::vector<MCMCSplineComposante>& trace, const double ymin, const double ymax, const unsigned gridLength, const QString& ProgressBarText)
{
#ifdef DEBUG
    qDebug()<<"MCMCLoopCurve::compute_posterior_mean_map_G_composante "<<mModel->mSettings.mTmin<<mModel->mSettings.mTmax;
    QElapsedTimer tClock;
    tClock.start();
#endif
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;

    const int nbPoint = gridLength; //floor ((tmax - tmin +1) /step);

    std::vector<double> vecG (nbPoint);
    std::vector<double> vecGP (nbPoint);
    std::vector<double> vecGS (nbPoint);
    // erreur inter spline
    std::vector<double> vecVarianceG (nbPoint);
    std::vector<double> vecVarG (nbPoint);
    // erreur intra spline
    std::vector<double> vecVarErrG (nbPoint);

    double nbIter = trace.size();

    const int nbPtsX = gridLength;
    const int nbPtsY = gridLength;

    const double stepT = (tmax - tmin) / (nbPtsX - 1);
    const double stepY = (ymax - ymin) / (nbPtsY - 1);

    CurveMap curveMap (nbPtsX, nbPtsY);
    curveMap.setRangeX(tmin, tmax);
    curveMap.setRangeY(ymin, ymax);

    double t, g, gp, gs, varG, stdG;


    g = 0.;
    gp = 0;
    varG = 0;
    gs = 0;

    emit stepChanged(ProgressBarText, 0, trace.size());

    double n = 0;
    double  prevMeanG;

    double maxDensity = 0;
    double minDensity = +INFINITY;
    const double k = 3.; // Le nombre de fois sigma G, pour le calcul de la densité
    double a, b, surfG;
  //  long double  prevMeanG;
    int  idxYErrMin, idxYErrMax;
    for (auto&& splineComposante : trace ) {
        n++;
        unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
        for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
            t = (double)idxT * stepT + tmin ;
            valeurs_G_VarG_GP_GS(t, splineComposante, g, varG, gp, gs, i0);

            // -- calcul Mean
            prevMeanG = vecG.at(idxT);
            vecG[idxT] +=  (g - prevMeanG)/n;
            vecGP[idxT] +=  (gp - vecGP.at(idxT))/n;
            vecGS[idxT] +=  (gs - vecGS.at(idxT))/n;

            vecVarianceG[idxT] +=  (g - prevMeanG)*(g - vecG.at(idxT));

            vecVarErrG[idxT] += (varG - vecVarErrG.at(idxT)) / n  ;

            // -- calcul map
            g = std::max(ymin, std::min(g, ymax));
            stdG = sqrt(varG);

            //idxY = floor((g-ymin) / stepY);


            idxYErrMin = std::max( 0, int((g - k*stdG - ymin) / stepY));
            idxYErrMax = std::min(int( (g + k*stdG - ymin) / stepY), nbPtsY);

            if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY) {
#ifdef DEBUG
                    if ((curveMap.row()*idxT + idxYErrMin) < (curveMap.row()*curveMap.column()))
                        curveMap(idxT, idxYErrMin) = curveMap.at(idxYErrMin, idxYErrMin) + 1./nbIter;
                    else
                        qDebug()<<"pb in MCMCLoopCurve::compute_posterior_mean_map_G_composante";
#else
                    curveMap(idxT, idxYErrMin) = curveMap.at(idxT, idxYErrMin) + 1./nbIter ;
#endif

                    maxDensity = std::max(maxDensity, curveMap.at(idxT, idxYErrMin));
                    minDensity = std::min(minDensity, curveMap.at(idxT, idxYErrMin));

            } else if (0 < idxYErrMin && idxYErrMax < nbPtsY) {
                double* ptr_Ymin = curveMap.ptr_at(idxT, idxYErrMin);
                double* ptr_Ymax = curveMap.ptr_at(idxT, idxYErrMax);

                int idErr = idxYErrMin;
                for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                    a = (idErr - 0.5)*stepY + ymin;
                    b = (idErr + 0.5)*stepY + ymin;
                    surfG = diff_erf(a, b, g, stdG )/nbIter;
  *ptr_idErr = (*ptr_idErr) + surfG;


                    //maxDensity = std::max(maxDensity, curveMap.at(idxT, idxY));
                    //minDensity = std::min(minDensity, curveMap.at(idxT, idxY));
                    maxDensity = std::max(maxDensity, *ptr_idErr);
                    minDensity = std::min(minDensity, *ptr_idErr);

                    idErr++;
                }
            }


        }

         emit stepProgressed(n);

    }

    curveMap.max_value = maxDensity;
    curveMap.min_value = minDensity;

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

    result.mapG = std::move(curveMap);

#ifdef DEBUG
    qDebug() <<  QString("=> MCMCLoopCurve::compute_posterior_mean_map_G_composante done in " + DHMS(tClock.elapsed()));

#endif

    return result;
}
*/

// Obsolete
// Code devant permettre de faire le calcul de la courbe moyenne sur toutes les traces en même temps que la moyenne sur une trace
// CODE A CONTROLER
/*
PosteriorMeanGComposante MCMCLoopCurve::computePosteriorMeanGComposante_chain_allchain(const std::vector<MCMCSplineComposante>& trace, PosteriorMeanGComposante& meanGAllChain, int prevChainSize)
{
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;
    const double step = mModel->mSettings.mStep;

    const unsigned nbPoint = floor ((tmax - tmin +1) /step);

    std::vector<double> vecVarianceG (nbPoint);
    std::vector< double> vecCumulErrG2 (nbPoint);

    std::vector<double> vecG (nbPoint);
    std::vector<double> vecGP (nbPoint);
    std::vector<double> vecGS (nbPoint);
    std::vector<double> vecVarG (nbPoint);
    std::vector<double> vecPrevVarErrG (nbPoint);
    std::vector<double> vecVarErrG (nbPoint);

    std::vector<double>& vecGAllChain = meanGAllChain.vecG;
    std::vector<double>& vecGPAllChain = meanGAllChain.vecGP;
    std::vector<double>& vecGSAllChain = meanGAllChain.vecGS;
    std::vector<double>& vecVarGAllChain = meanGAllChain.vecVarG;

    unsigned long nbIter = trace.size();

     double t, g, gp, gs, varG;
    g = 0.;
    gp = 0;
    varG = 0;
    gs = 0;
    int nPrevAll= prevChainSize;
    int n = 0;

    double  prevMeanG, prevMeanAll;
    for (unsigned i = 0; i<nbIter; ++i) {
        const MCMCSplineComposante& splineComposante = trace.at(i);
        n++;
        nPrevAll++;
        unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
        for (unsigned tIdx = 0; tIdx < nbPoint ; ++tIdx) {
            t = (double)tIdx * step + tmin ;
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
*/

/*
CurveMap MCMCLoopCurve::compute_posterior_map_G_composante(const std::vector<MCMCSplineComposante>& trace, const double ymin , const double ymax, const unsigned gridLength)
{
#ifdef DEBUG
    qDebug()<<"MCMCLoopCurve::compute_posterior_map_G_composante "<<mModel->mSettings.mTmin<<mModel->mSettings.mTmax;
    QElapsedTimer tClock;
    tClock.start();
#endif
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;

    const unsigned nbPtsX = gridLength;
    const unsigned nbPtsY = gridLength;

    const double stepT = (tmax - tmin) / (nbPtsX - 1);
    const double stepY = (ymax - ymin) / (nbPtsY - 1);

    CurveMap curveMap (nbPtsX, nbPtsY);
    curveMap.setRangeX(tmin, tmax);
    curveMap.setRangeY(ymin, ymax);

    double t, g, gp, gs, varG, stdG;


    int n = 0;
    double maxDensity = 0;
    double minDensity = +INFINITY;
    const double k = 3.; // Le nombre de fois sigma G, pour le calcul de la densité
    double a, b, coefG;
  //  long double  prevMeanG;
    int idxY, idxYErrMin, idxYErrMax;
    for (auto&& splineComposante : trace ) {
        n++;
        unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
        for (unsigned idxT = 0; idxT < nbPtsX ; ++idxT) {
            t = (double)idxT * stepT + tmin ;
            valeurs_G_VarG_GP_GS(t, splineComposante, g, varG, gp, gs, i0);

            // pour debug
            g = std::max(ymin, std::min(g, ymax));
            stdG =sqrt(varG);

            idxY = floor((g-ymin) / stepY);

            // ajout densité erreur sur Y

            idxYErrMin = (g - k*stdG - ymin) / stepY;
            idxYErrMax = (g + k*stdG - ymin) / stepY;

            if (idxYErrMin == idxYErrMax) {
#ifdef DEBUG
                    if ((curveMap.row()*idxT + idxY) < (curveMap.row()*curveMap.column()))
                        curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + 1./(double)trace.size();
                    else
                        qDebug()<<"pb in MCMCLoopCurve::compute_posterior_map_G_composante";
#else
                    curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + 1./(double)trace.size() ;
#endif

                    maxDensity = std::max(maxDensity, curveMap.at(idxT, idxY));
                    minDensity = std::min(minDensity, curveMap.at(idxT, idxY));

            } else {
                double* ptr_Ymin = curveMap.ptr_at(idxT, idxYErrMin);
                double* ptr_Ymax = curveMap.ptr_at(idxT, idxYErrMax);

               // for (unsigned idErr = idxYErrMin; idErr <= idxYErrMax; idErr++) {
                unsigned long idErr = idxYErrMin;
                for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                    a = (idErr - 0.5)*stepY + ymin;
                    b = (idErr + 0.5)*stepY + ymin;
                    coefG = diff_erf(a, b, g, stdG )/(double)(trace.size() * 1);
#ifdef DEBUG
                    if ((curveMap.row()*idxT + idxY) < (curveMap.row()*curveMap.column()))
                        *ptr_idErr = (*ptr_idErr) + coefG;
                    else
                        qDebug()<<"pb in MCMCLoopCurve::compute_posterior_map_G_composante";
#else
                    *ptr_idErr = (*ptr_idErr) + coefG;
#endif

                    maxDensity = std::max(maxDensity, *ptr_idErr);
                    minDensity = std::min(minDensity, *ptr_idErr);

                    idErr++;
                }
            }

            for (int i = 0 ; i< 1; i++) {
                g_err = g - sqrt(varG)*(double)i/3.;
                coefG = pow(g_err - g, 2.)*varG;
                coefG = exp( -coefG/2.)/ sqrt(2*M_PI*varG);

                if (coefG > 0) {
                    g_err = std::max(ymin, std::min(g_err, ymax));

                    idxY = unsigned(floor((g_err-ymin)/(ymax-ymin) * (nbPtsY-1)));

#ifdef DEBUG
                    if ((curveMap.row()*idxT + idxY) < (curveMap.row()*curveMap.column()))
                        curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                    else
                        qDebug()<<"pb in MCMCLoopCurve::compute_posterior_map_G_composante";
#else
                    curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + 1./(double)(trace.size() * 1);
#endif

                    maxDensity = std::max(maxDensity, curveMap.at(idxT, idxY));
                    minDensity = std::min(minDensity, curveMap.at(idxT, idxY));
                }

             }
}



    }

    curveMap.max_value = maxDensity;
    curveMap.min_value = minDensity;

#ifdef DEBUG
    qDebug() <<  QString("=> MCMCLoopCurve::compute_posterior_map_G_composante done in " + DHMS(tClock.elapsed()));

#endif

    return curveMap;
}
*/

double MCMCLoopCurve::valeurG(const double t, const MCMCSplineComposante& spline, unsigned &i0)
{
    const unsigned n = spline.vecThetaEvents.size();
    double tReduce = mModel->reduceTime(t);
    const double t1 = mModel->reduceTime(spline.vecThetaEvents.at(0));
    const double tn = mModel->reduceTime(spline.vecThetaEvents.at(n-1));
    double g = 0;
    
    double t2, ti1, ti2, tn1, p, gp1, gpn, gi1, gi2, gamma1, gamma2, h;
    if (tReduce < t1) {
        t2 = mModel->reduceTime(spline.vecThetaEvents.at(1));
        gp1 = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
        gp1 -= (t2 - t1) * spline.vecGamma.at(1) / 6.;
        g = spline.vecG.at(0) - (t1 - tReduce) * gp1;

    } else if (tReduce >= tn) {
        tn1 = mModel->reduceTime(spline.vecThetaEvents.at(n-2));
        gpn = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tn1);
        gpn += (tn - tn1) * spline.vecGamma.at(n-2) / 6.;
        g = spline.vecG.at(n-1) + (tReduce - tn) * gpn;

    } else {
        for (; i0 < n-1; ++i0) {
            ti1 = mModel->reduceTime(spline.vecThetaEvents.at(i0));
            ti2 = mModel->reduceTime(spline.vecThetaEvents.at(i0+1));
            if ((tReduce >= ti1) && (tReduce < ti2)) {
                h = ti2 - ti1;
                gi1 = spline.vecG.at(i0);
                gi2 = spline.vecG.at(i0+1);

                // Linear part :
                g = gi1 + (gi2-gi1)*(tReduce-ti1)/h;

                // Smoothing part :
                gamma1 = spline.vecGamma.at(i0);
                gamma2 = spline.vecGamma.at(i0+1);
                p = (1./6.) * ((tReduce-ti1) * (ti2-tReduce)) * ((1.+(tReduce-ti1)/h) * gamma2 + (1.+(ti2-tReduce)/h) * gamma1);
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
//obsolete
double MCMCLoopCurve::valeurErrG(const double t, const MCMCSplineComposante& spline, unsigned& i0)
{
    const unsigned n = spline.vecThetaEvents.size();
    
    const double t1 = spline.vecThetaEvents[0];
    const double tn = spline.vecThetaEvents[n-1];
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
                const double err1 = sqrt(spline.vecVarG[i0]);
                const double err2 = sqrt(spline.vecVarG[i0+1]);
                errG = err1 + ((t-ti1) / (ti2-ti1)) * (err2 - err1);
                break;
            }
        }
    }

    return errG;
}

// dans RenCurve U-CMT-Routine_Spline Valeur_Gp
double MCMCLoopCurve::valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0)
{
   const unsigned n = spline.vecThetaEvents.size();
   const double tReduce =  mModel->reduceTime(t);
   const double t1 = mModel->reduceTime(spline.vecThetaEvents.at(0));
   const double tn = mModel->reduceTime(spline.vecThetaEvents.at(n-1));
   double gPrime = 0.;
    
    // la dérivée première est toujours constante en dehors de l'intervalle [t1,tn]
    if (tReduce < t1) {
        const double t2 = mModel->reduceTime(spline.vecThetaEvents.at(1));
        gPrime = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
        gPrime -= (t2 - t1) * spline.vecGamma.at(1) / 6.;
        /* Code Rencurve
         * ti2:=Vec_splineP.t[2];
         * GP1:=(Vec_spline.G[2]-Vec_spline.g[1])/(ti2-ti1);
         * GP:=GP1-(ti2-ti1)*Vec_spline.gamma[2]/6;
         */



    } else if (tReduce >= tn) {
        const double tin_1 = mModel->reduceTime(spline.vecThetaEvents.at(n-2));
        gPrime = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tin_1);
        gPrime += (tn - tin_1) * spline.vecGamma.at(n-2) / 6.;

        /* Code Rencurve
         * tin_1:=Vec_splineP.t[nb_noeuds-1];
         * GPn:=(Vec_spline.G[nb_noeuds]-Vec_spline.G[nb_noeuds-1])/(tin-tin_1);
         * GP:=GPn+(tin-tin_1)*Vec_spline.gamma[nb_noeuds-1]/6;
         */

    } else {
        for ( ;i0< n-1; ++i0) {
            const double ti1 = mModel->reduceTime(spline.vecThetaEvents.at(i0));
            const double ti2 = mModel->reduceTime(spline.vecThetaEvents.at(i0+1));
            if ((tReduce >= ti1) && (tReduce < ti2)) {
                const double h = ti2 - ti1;
                const double gi1 = spline.vecG.at(i0);
                const double gi2 = spline.vecG.at(i0+1);
                const double gamma1 = spline.vecGamma.at(i0);
                const double gamma2 = spline.vecGamma.at(i0+1);

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
    const double tReduce = mModel->reduceTime(t);
    // The second derivative is always zero outside the interval [t1,tn].
    double gSeconde = 0.;
    
    for (int i = 0; i < n-1; ++i) {
        const double ti1 = mModel->reduceTime(spline.vecThetaEvents.at(i));
        const double ti2 = mModel->reduceTime(spline.vecThetaEvents.at(i+1));
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
void MCMCLoopCurve::valeurs_G_ErrG_GP_GS(const double t, const MCMCSplineComposante& spline,  double& G,  double& errG, double& GP, double& GS, unsigned& i0)
{

    unsigned n = spline.vecThetaEvents.size();
    const double tReduce =  mModel->reduceTime(t);
    const double t1 = mModel->reduceTime(spline.vecThetaEvents.at(0));
    const double tn = mModel->reduceTime(spline.vecThetaEvents.at(n-1));
    GP = 0.;
    GS = 0.;

     // The first derivative is always constant outside the interval [t1,tn].
     if (tReduce < t1) {
         const double t2 = mModel->reduceTime(spline.vecThetaEvents.at(1));

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

         const double tn1 = mModel->reduceTime(spline.vecThetaEvents.at(n-2));
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
             const double ti1 = mModel->reduceTime(spline.vecThetaEvents.at(i0));
             const double ti2 = mModel->reduceTime(spline.vecThetaEvents.at(i0 + 1));
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
                 const double err1 = sqrt(spline.vecVarG.at(i0));
                 const double err2 = sqrt(spline.vecVarG.at(i0 + 1));
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


/*********************************************************************************
**** Cette procedure calcule la Cross Validation
**** cad la prédiction de (g_(ti)-Yij) sans le point Yij
**********************************************************************************/

// Obsolete
double MCMCLoopCurve::general_cross_validation (const SplineMatrices& matrices, const std::vector<double>& vecH, const double lambdaSpline)
{
    const double N = matrices.diagWInv.size();

    Matrix2D matB = addMatEtMat(matrices.matR, multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matB, 5, 1);
    SplineResults s = calculSplineX (matrices, vecH, decomp, matB, lambdaSpline);

    auto matA = calculMatInfluence_origin(matrices, s , 1, lambdaSpline);
    // Nombre de degré de liberté
    auto DLEc = N - std::accumulate(matA.begin(), matA.end(), 0.);

    double GCV = 0.;
    for (int i = 0 ; i < N; i++) {
       // utiliser mYx pour splineX
        GCV +=  pow(s.vecG.at(i) - mModel->mEvents.at(i)->mYx, 2.)/ matrices.diagWInv.at(i) ;
    }
    GCV /= pow(DLEc, 2.);


    return GCV;
}

 double MCMCLoopCurve::cross_validation (const SplineMatrices& matrices, const std::vector< double>& vecH, const double lambdaSpline)
{

    const double N = matrices.diagWInv.size();
    //Matrix2D tmp = multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5);
    //Matrix2D matB = addMatEtMat(matrices.matR, tmp, 5);

    Matrix2D matB = addMatEtMat(matrices.matR, multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    std::pair<Matrix2D, std::vector< double>> decomp = decompositionCholesky(matB, 5, 1);
    SplineResults s = calculSplineX (matrices, vecH, decomp, matB, lambdaSpline);

    auto matA = calculMatInfluence_origin(matrices, s , 1, lambdaSpline);

   double CV = 0.;
   for (int i = 0 ; i < N; i++) {
       CV +=  pow((s.vecG.at(i) - mModel->mEvents.at(i)->mYx) / (1-matA.at(i)), 2.) ;  // / matrices.diagWInv.at(i)=1 ;
   }

    return CV;
}

/**
 * @brief MCMCLoopCurve::initLambdaSpline
 * On initialise alpha avec la valeur de S02_lambda()
 * @return S02_lambda
 */
 double MCMCLoopCurve::initLambdaSpline()
{

   // orderEventsByThetaReduced(mModel->mEvents); // already done
   // spreadEventsThetaReduced0(mModel->mEvents);

    const auto vecH = calculVecH(mModel->mEvents);
    auto matrices_WI = prepareCalculSpline_WI(mModel->mEvents, vecH);

    return S02_lambda_WI(matrices_WI, mModel->mEvents.size());


}

// initialisation,par cross-validation
 double MCMCLoopCurve::initLambdaSplineByCV()
{
    std::vector< double> CV;
    std::vector< double> lambda;
    std::vector< double> memoVG;

    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    for (auto& ev : mModel->mEvents) {
        memoVG.push_back(ev->mVG.mX);
        ev->mVG.mX = 0.;
    }
    auto vecH = calculVecH(mModel->mEvents);
    auto matrices = prepareCalculSpline(mModel->mEvents, vecH);

    long double lambdaTest;

    for (int idxLambda = -200; idxLambda < 101; ++idxLambda ) {
        lambdaTest = pow(10., ( double)idxLambda/10);

        CV.push_back(cross_validation(matrices, vecH, lambdaTest));
        lambda.push_back(lambdaTest);
    }

    // on recherche la plus petite valeur de CV
    unsigned long idxDifMin = std::distance(CV.begin(), std::min_element(CV.begin(), CV.end()) );

    // si le mini est à une des bornes, il n'y a pas de solution
    // Donc on recherche la plus grande variation, le "coude"
    if (idxDifMin == 0 || idxDifMin == (CV.size()-1)) {
        // On recherche la plus grande variation de CV
        std::vector< double> difResult (CV.size()-1);
        std::transform(CV.begin(), CV.end()-1, CV.begin()+1 , difResult.begin(), []( double a,  double b) {return pow(a-b, 2.l);});
        idxDifMin = std::distance(difResult.begin(), std::max_element(difResult.begin(), difResult.end()) );

    }
    // restore VG
    int i = 0;
    for (auto& ev : mModel->mEvents)
        ev->mVG.mX = memoVG.at(i++);

    return lambda.at(idxDifMin);
}

double MCMCLoopCurve::initLambdaSplineBy_h_YWI_AY()
{
    std::vector<double> list_h_YWI;
    std::vector<double> lambda;
    // theta must already be sorted.
    /* orderEventsByThetaReduced(mModel->mEvents);
       spreadEventsThetaReduced0(mModel->mEvents);
    */

    auto vecH = calculVecH(mModel->mEvents);
    auto matrices = prepareCalculSpline(mModel->mEvents, vecH);

    double lambdaTest;
/*
   // On prend le premier non nul et on stop
    for (int idxLambda = 100; idxLambda > -201; --idxLambda ) {
        lambdaTest = pow(10., (double)idxLambda/10);

        if (h_YWI_AY(matrices, mModel->mEvents,  lambdaTest, vecH) > 0)
            return lambdaTest;

    }
#ifdef DEBUG
    qWarning("initLambdaSplineBy_h_YWI_AY() Lambda init with 1E-20");
#endif
    return 1E-20;
*/

/*
      // On stock toutes les valeurs et on prend l'indice du premier

    for (int idxLambda = 100; idxLambda > -201; --idxLambda ) {
        lambdaTest = pow(10., (double)idxLambda/10);

        list_h_YWI.push_back(h_YWI_AY(matrices, mModel->mEvents,  lambdaTest, vecH));
        lambda.push_back(lambdaTest);
    }


    int i = 0;

    for (double h : list_h_YWI) {
        if (h > 0) {
            return lambda.at(i);
        }
        i++;
    }
    return lambda.at(i);
*/

    // On stock que les lambda qui donne h > 0

    for (int idxLambda = 100; idxLambda > -201; --idxLambda ) {
        lambdaTest = pow(10., (double)idxLambda/10);

        if (h_YWI_AY(matrices, mModel->mEvents,  lambdaTest, vecH) > 0)
            lambda.push_back(lambdaTest);
    }

    // on retourne la valeur médiane
    const int i = lambda.size()>1 ? floor(lambda.size()/2.) : 0;

    return lambda.at(i);

}


#pragma mark Related to : calibrate

void MCMCLoopCurve::prepareEventsY(const QList<Event *> &lEvents)
{
  /*  for (Event* event : lEvents) {
        prepareEventY(event);
    }
*/
    std::for_each( lEvents.begin(), lEvents.end(), [this](Event *e) { prepareEventY(e); });

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

        } if (mCurveSettings.mVariableType == CurveSettings::eVariableTypeField) {
            event->mYx = event->mZField;
            event->mSy = event->mS_ZField;

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
  //  if (event->mSy == 0)
    //    event->mSy = 1.E-20;
}


#pragma mark Related to : update : calcul de h_new

/**
 * Calcul de h_YWI_AY pour toutes les composantes de Y event (suivant la configuration univarié, spérique ou vectoriel)
 */
 double MCMCLoopCurve::h_YWI_AY(const SplineMatrices& matrices, const QList<Event *> &lEvents, const double lambdaSpline, const std::vector< double>& vecH)
{
    double h = h_YWI_AY_composanteX( matrices, lEvents, lambdaSpline, vecH);

    const bool hasY = (mCurveSettings.mProcessType != CurveSettings::eProcessTypeUnivarie);

    if (hasY) {
        h *= h_YWI_AY_composanteY (matrices, lEvents, lambdaSpline, vecH);
        // To treat the 2D case, we use the 3D case by setting Yint = 100
      /*  const bool hasZ = (mCurveSettings.mProcessType == CurveSettings::eProcessType2D ||
                           mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
                           mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
                           mCurveSettings.mProcessType == CurveSettings::eProcessType3D);

        if (hasZ) {*/ //Always true
            h *= h_YWI_AY_composanteZ(matrices, lEvents, lambdaSpline, vecH);
       // }
    }

    return h;
}


 double MCMCLoopCurve::h_YWI_AY_composanteX(const SplineMatrices &matrices, const QList<Event *> lEvents, const double lambdaSpline, const std::vector< double>& vecH)
{
    if (lambdaSpline == 0) {
            return 1.;
    }

    std::vector< double> vecY (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYx;});

    SplineResults spline = calculSpline(matrices, vecY, lambdaSpline, vecH);
    std::vector< double>& vecG = spline.vecG;
    std::vector< double>& matD = spline.matD;
    Matrix2D matQTQ = matrices.matQTQ;

    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    const int nb_noeuds = lEvents.size();

    double h_exp = 0.;
    int i = 0;
    for (auto& e : lEvents) {
        h_exp  +=  e->mW * e->mYx * (e->mYx - vecG.at(i++));
    }

    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matQTQ, 5, 1);
   // Matrix2D matLq = decomp.first;
    std::vector<double>& matDq = decomp.second;

    double log_det_1_2 = 0.;
    for (int i = 1; i < nb_noeuds-1; ++i) { // correspond à i=shift jusqu'à nb_noeuds-shift
        log_det_1_2 += log(matDq.at(i)/ matD.at(i));
    }

    // calcul à un facteur (2*PI) puissance -(n-2) près
    double res = 0.5 * ( (nb_noeuds-2.) * log(lambdaSpline) + log_det_1_2 - h_exp) ;
    res = exp(res) ;
    return res;
}

double MCMCLoopCurve::h_YWI_AY_composanteY(const SplineMatrices& matrices, const QList<Event *> lEvents, const double lambdaSpline, const std::vector<double>& vecH)
{
    if (lambdaSpline == 0.) { // Attention double == 0
        return 1.;
    }
    errno = 0;
#ifdef Q_OS_MAC
      if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);
#endif
    std::vector<double> vecY (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYy;});

    SplineResults spline = calculSpline(matrices, vecY, lambdaSpline, vecH);
    std::vector<double>& vecG = spline.vecG;

    std::vector<double>& matD = spline.matD;
    const Matrix2D& matQTQ = matrices.matQTQ;

    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    const int nb_noeuds = lEvents.size();

    double h_exp = 0.;
    int i = 0;
    for (auto& e : lEvents) {
        h_exp  +=  e->mW * e->mYy * (e->mYy - vecG.at(i++));
    }
    h_exp *= -0.5 ;


    /* -------------------------------------------
    * Calcul de la norme
    * -------------------------------------------
    * Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    * (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    * ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    * inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ */

    std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matQTQ, 5, 1);
    std::vector<double>& matDq = decomp.second;

    double det_1_2 = 1.;
    for (int i = 1; i < nb_noeuds-1; ++i) {
        det_1_2 *= matDq.at(i)/ matD.at(i);
    }

    // calcul à un facteur (2*PI) puissance -(n-2) près
    double res = 0.5l * (nb_noeuds-2.) * log(lambdaSpline) + h_exp;
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

double MCMCLoopCurve::h_YWI_AY_composanteZ(const SplineMatrices& matrices, const QList<Event *> lEvents, const  double lambdaSpline, const std::vector< double>& vecH)
{
    if (lambdaSpline == 0.) { // Attention double == 0
        return 1.;
    }
    errno = 0;
#ifdef Q_OS_MAC
      if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);
#endif
    std::vector<double> vecY (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), vecY.begin(), [](Event* ev) {return ev->mYz;});

    SplineResults spline = calculSpline(matrices, vecY, lambdaSpline, vecH);
    std::vector<double>& vecG = spline.vecG;
    std::vector<double>& matD = spline.matD;
    const Matrix2D& matQTQ = matrices.matQTQ;

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
        h_exp  += (double) e->mW * (double)e->mYz * (( double)e->mYz - vecG.at(i++));
    }
    h_exp *= -0.5 ;

    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matQTQ, 5, 1);
   // std::vector<std::vector<double>> matLq = decomp.first;
    std::vector< double>& matDq = decomp.second;

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


double MCMCLoopCurve::S02_lambda_WI (const SplineMatrices &matrices, const int nb_noeuds)
{
    const Matrix2D& matR = matrices.matR;
    const Matrix2D& matQ = matrices.matQ;
    const Matrix2D& matQT = matrices.matQT;

    // On pose W égale la matrice unité

    // calcul des termes diagonaux de W_1.K
    const std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matR, 3, 1);
    const Matrix2D& matL = decomp.first;
    const std::vector<double>& matD = decomp.second;
    const Matrix2D matRInv = inverseMatSym_origin(matL, matD, 5, 1);

    const Matrix2D matK = multiMatParMat(multiMatParMat(matQ, matRInv, 3, 3), matQT, 3, 3);
    // matK est une matrice pleine
    double vm = 0.;
    for (int i = 0; i < nb_noeuds; ++i) {
        vm += matK[i][i];
    }

    const double S02_lambda = (nb_noeuds-2) / vm;

    return S02_lambda;
}

double MCMCLoopCurve::S02_lambda_old (const SplineMatrices &matrices, const int nb_noeuds)
{
    const std::vector<double>& diagWInv = matrices.diagWInv;
    const Matrix2D& matR = matrices.matR;
    const Matrix2D& matQ = matrices.matQ;
    const Matrix2D& matQT = matrices.matQT;

    // La trace de la matrice produit W_1.K est égale à la somme des valeurs propores si W_1.K est symétrique,
    // ce qui implique que W_1 doit être une constante
    // d'où on remplace W_1 par la matrice W_1m moyenne des (W_1)ii

    const double W_1m = std::accumulate(diagWInv.begin(), diagWInv.begin() + nb_noeuds, 0.) / nb_noeuds;

    // calcul des termes diagonaux de W_1.K
    std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matR, 3, 1);
    Matrix2D& matL = decomp.first;
    std::vector<double>& matD = decomp.second;
    const Matrix2D matRInv = inverseMatSym_origin(matL, matD, 5, 1);

    const Matrix2D matK = multiMatParMat(multiMatParMat(matQ, matRInv, 3, 3), matQT, 3, 3);

    const Matrix2D matW_1K = multiDiagParMat(std::vector<double> (nb_noeuds, W_1m), matK, 1);

    double vm = 0.;
    for (int i = 0; i < nb_noeuds; ++i) {
        vm += matW_1K[i][i];
    }

    const double S02_lambda = (nb_noeuds-2) / vm;

    return S02_lambda;
}

double MCMCLoopCurve::h_lambda(const SplineMatrices& matrices, const int nb_noeuds, const double &lambdaSpline)
{
    /* initialisation de l'exposant mu du prior "shrinkage" sur lambda : fixe
     en posant mu=2, la moyenne a priori sur alpha est finie = (nb_noeuds-2)/somme(Mat_W_1K[i,i]) ;
     et la variance a priori sur lambda est infinie
     NB : si on veut un shrinkage avec espérance et variance finies, il faut mu >= 3
    */

    const int mu = 3;
    const double c = S02_lambda_WI(matrices, nb_noeuds);

    // prior "shrinkage"
    return pow(c, mu) / pow(c + lambdaSpline, mu+1); //((mu/c) * pow(c/(c + lambdaSpline), mu+1));

}


/* ancienne fonction U_cmt_MCMC:: h_Vgij dans RenCurve
 * lEvents.size() must be geater than 2
 */
double MCMCLoopCurve::h_VG_old(const QList<Event *> _events)
{
    // Densité a priori sur variance de type "shrinkage" avec paramètre S02
    // bool_shrinkage_uniforme:=true;
    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        double h_VG;
        double S02;
 /*
        if (mCurveSettings.mUseVarianceIndividual) {
            // variance RICE locale sur 3 points
*/
/*            int nb_noeuds = lEvents.size();
            S02 = (pow( (long double)lEvents[0]->mYx - (long double)lEvents[1]->mYx , 2.l)) /2.;
            h_VG = (S02 / pow(S02 + (long double)lEvents[0]->mVG.mX, 2.l));

            for ( auto i = 1 ; i< nb_noeuds-1; i++) {
                S02 = (pow( (long double)lEvents[i]->mYx - (long double)lEvents[i-1]->mYx , 2.l) + pow( (long double)lEvents[i]->mYx - (long double)lEvents[i+1]->mYx , 2.l)) /4.;
                // Shrinkage uniforme : a = 1
                h_VG *= (S02 / pow(S02 + (long double)lEvents[i]->mVG.mX, 2.l));
            }

            S02 = (pow( (long double)lEvents[nb_noeuds-2]->mYx - (long double)lEvents[nb_noeuds-1]->mYx , 2.l)) /2.;

            h_VG *= (S02 / pow(S02 + (long double)lEvents[nb_noeuds-1]->mVG.mX, 2.l));
*/
/*
            h_VG = 1;

            for (unsigned long i = 0; i< vecG.size(); i++) {
                S02 = pow(_events[i]->mYx - vecG[i], 2.);
                h_VG *= (S02 / pow(S02 + _events[i]->mVG.mX, 2.));
            }


        } else {
*/

               // S02 : moyenne harmonique des erreurs sur Y
            double som_inv_S02 = 0.;
            for (Event* e :_events) {
                som_inv_S02 += (1. / pow(e->mSy, 2.));
            }
            S02 = (double)_events.size()/som_inv_S02;

            h_VG = S02 / pow(S02 + _events[0]->mVG.mX, 2.);
            // Shrinkage uniforme : a = 1
           // S02 = Calcul_Variance_Rice(lEvents);
/*
            S02 = 0;
            for (unsigned long i = 0; i< vecG.size(); i++) {
                S02 += pow(_events[i]->mYx - vecG[i], 2.);
            }
            S02 /= (double)vecG.size();

            h_VG = S02 / pow(S02 + _events[0]->mVG.mX, 2.);
        }
*/
  /*      S02 = 0;
        for (unsigned long i = 0; i< vecG.size(); i++) {
            S02 += pow(_events[i]->mYx - vecG[i], 2.);
        }
        S02 /= (double)vecG.size();
*/

        // Shrinkage uniforme : a = 1
        //S02 = Calcul_Variance_Rice(_events);

        /*h_VG = 1;
        for (auto&& e : _events)
            h_VG *= S02 / pow(S02 + e->mVG.mX, 2.);
        */

        // Comme RenCurve
     /*   for (auto&& e : _events)
            h_VG *= pow(e->mSy, 2.) / pow(pow(e->mSy, 2.) + e->mVG.mX, 2.);
     */
        return h_VG;

    } else
        return 1.;

}

/*
double MCMCLoopCurve::h_VG_global(const QList<Event *> _events, double VG)
{
    double som_inv_S02 = 0.;
    for (Event* e :_events) {
        som_inv_S02 += (1. / pow(e->mSy, 2.));
    }
    double acc_inv = _events.size();
    acc_inv /= std::accumulate (_events.begin(), _events.end(), 0., [](double x, Event* e) {return x + 1/(e->mSy*e->mSy);});

    const double S02 = (double)_events.size()/som_inv_S02;


    return S02 / pow(S02 + VG, 2.);
}
*/

double MCMCLoopCurve::h_VG_Event(const Event* e, double S02_Vg)
{
    const int a = 3;
    return pow(S02_Vg/(S02_Vg + e->mVG.mX), a+1) / S02_Vg;
}

double MCMCLoopCurve::h_S02_Vg(const QList<Event *> events, double S02_Vg, const double var_Y)
{
    const double alp = var_Y/(var_Y-1.);
    const double prior = pow(1./S02_Vg, alp+1.) * exp(-alp/S02_Vg);
    const int a = 3;

    if (mCurveSettings.mUseVarianceIndividual) {
        double prod_h_Vg = 1.;
        for (auto& e : events) {
            prod_h_Vg *= pow(S02_Vg/(S02_Vg + e->mVG.mX), a+1) / S02_Vg;
        }

       return prior* prod_h_Vg;

    } else {
        return prior* pow(S02_Vg/(S02_Vg + events[0]->mVG.mX), a+1) / S02_Vg;;
    }


}
double MCMCLoopCurve::h_VG_event_old(const Event * e)
{
   return pow(e->mSy, 2.) / pow(pow(e->mSy, 2.) + e->mVG.mX, 2.);
}

/** @brief Calcul la variance individuelle spline
 */
double MCMCLoopCurve::S02_Vg_Yx( QList<Event *> _events, SplineMatrices matricesWI, std::vector<double> vecH, const double lambdaSpline)
{
    const MCMCSpline splineWI = currentSpline(_events, false, vecH, matricesWI);
    const auto& vecG = splineWI.splineX.vecG;

    double S02 = 0;
    for (unsigned long i = 0; i< vecG.size(); i++) {
        S02 += pow(_events[i]->mYx - vecG[i], 2.);
    }

    //  Mat_B = R + alpha * Qt * W-1 * Q
    const Matrix2D matB = addMatEtMat(matricesWI.matR, multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    std::pair<Matrix2D, std::vector< double>> decomp = decompositionCholesky(matB, 5, 1);

    const SplineResults s = calculSplineX (matricesWI, vecH, decomp, matB, lambdaSpline);

    const auto matA = calculMatInfluence_origin(matricesWI, s , 1, lambdaSpline);

    const auto traceA = std::accumulate(matA.begin(), matA.end(), 0.);

    S02 /= (double)(vecG.size()) - traceA;
    return S02;
    //return 100.;// ??? pour test
}

/**
 * @brief Les calculs sont faits avec les dates (theta event, ti dates, delta, sigma) exprimées en années.
*/
// voir U-cmt_MCMC ligne 105 calcul_h
double MCMCLoopCurve::h_theta_Event (const Event * e)
{
    if (e->mType == Event::eDefault) {
        double p = 0.;
        double t_moy = 0.;
        double pi;
        for (auto&& date : e->mDates) {
            pi = 1. / pow(date.mSigmaTi.mX, 2.);
            p += pi;
            t_moy += (date.mTi.mX + date.mDelta) * pi;
        }
        t_moy /= p;

       return exp(-0.5 * p * pow( e->mTheta.mX  - t_moy, 2.));


    } else {
        return 1.;
    }

}

double MCMCLoopCurve::h_theta(QList<Event *> lEvents)
{
    if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
        double h = 1.;
        for (Event*& e : lEvents) {
            h *= h_theta_Event(e);
        }

#ifdef DEBUG
        if (h == 0.) {
            qWarning( "MCMCLoopCurve::h_theta() h == 0");
        }
#endif
        return h;

    } else
        return 1.;
}

#pragma mark Manipulation des theta event pour le calcul spline (equivalent "Do Cravate")

/*
 * Obsolete
 * Cette fonction est l'ancien do_cravate() qui portait mal son nom car nous ne faisons pas de cravates !
 * Une cravate correspondait à des theta tellement proches qu'on "fusionnait" les events correspondants pour les calculs.
 * Or, reorderEventsByTheta a pour rôle d'introduire un espace minimal entre chaque theta pour justement ne pas avoir de cravate.
 * Cette fonction ne fait donc que retourner le résultat de definitionNoeuds dans un format pratique pour la suite des calculs
 */
/*
void MCMCLoopCurve::orderEventsByTheta(QList<Event *> & lEvent)
{
    // On manipule directement la liste des évènements
     //  Ici on peut utiliser lEvent en le déclarant comme copy ??

    QList<Event*>& result = lEvent;
    
    std::sort(result.begin(), result.end(), [](const Event* a, const Event* b) { return (a->mTheta.mX < b->mTheta.mX); });
}
*/

void MCMCLoopCurve::orderEventsByThetaReduced(QList<Event *>& lEvent)
{
    // On manipule directement la liste des évènements
    // Ici on peut utiliser lEvent en le déclarant comme copy ??
    QList<Event*>& result = lEvent;

    std::sort(result.begin(), result.end(), [](const Event* a, const Event* b) { return (a->mThetaReduced < b->mThetaReduced); });
}

// Obsolete
void MCMCLoopCurve::saveEventsTheta(QList<Event *>& lEvent)
{
    mThetasMemo.clear();
    for (Event*& e : lEvent) {
        mThetasMemo.insert(std::pair<int, double>(e->mId, e->mTheta.mX));
    }
}

// Obsolete
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
double MCMCLoopCurve::minimalThetaDifference(QList<Event *>& lEvents)
{
    std::vector<double> result (lEvents.size());
    std::transform (lEvents.begin(), lEvents.end()-1, lEvents.begin()+1, result.begin(), [](const Event* e0, const  Event* e1) {return (e1->mTheta.mX - e0->mTheta.mX); });
   // result.erase(result.begin()); // the firs value is not a difference, it's just the first value of LEvents
    std::sort(result.begin(), result.end());
    return std::move(*std::find_if_not (result.begin(), result.end(), [](double v){return v==0.;} ));
}

double MCMCLoopCurve::minimalThetaReducedDifference(QList<Event *>& lEvents)
{
    std::vector<double> result (lEvents.size());
    std::transform (lEvents.begin(), lEvents.end()-1, lEvents.begin()+1, result.begin(), [](const Event* e0, const  Event* e1) {return (e1->mThetaReduced - e0->mThetaReduced); });
   // result.erase(result.begin()); // the firs value is not a difference, it's just the first value of LEvents
    std::sort(result.begin(), result.end());
    return std::move(*std::find_if_not (result.begin(), result.end(), []( double v){return v==0.;} ));
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

// Obsolete
/*
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
*/

// Les Events sont considèrés triés dans l'ordre croissant
/**
 * @brief MCMCLoopCurve::spreadEventsThetaReduced0
 * @param sortedEvents
 * @param spreadSpan
 */
void MCMCLoopCurve::spreadEventsThetaReduced0(QList<Event *> &sortedEvents, double spreadSpan)
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
                const double lowBound = itEvenFirst == sortedEvents.begin() ? sortedEvents.first()->mThetaReduced : (*(itEvenFirst -1))->mThetaReduced ; //valeur à gauche non égale
                const double upBound = itEvent == sortedEvents.end()-2 ? sortedEvents.last()->mThetaReduced : (*(itEvent + 1))->mThetaReduced;

                double step = spreadSpan / (nbEgal-1); // écart théorique
                double min;

                // Controle du debordement sur les valeurs encadrantes
                if (itEvenFirst == sortedEvents.begin()) {
                    // Cas de l'égalité avec la première valeur de la liste
                    // Donc tous les Events sont à droite de la première valeur de la liste
                    min = (*itEvent)->mThetaReduced;

                } else {
                    // On essaie de placer une moitier des Events à gauche et l'autre moitier à droite
                    min = (*itEvent)->mThetaReduced - step*floor(nbEgal/2.);
                    // controle du debordement sur les valeurs encadrantes
                    min = std::max(lowBound + step, min );
                }

                const double max = std::min(upBound - spreadSpan, (*itEvent)->mThetaReduced + (double)step*ceil(nbEgal/2.) );
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
        const double lowBound = (*(itEvenFirst -1))->mThetaReduced ; //la première valeur à gauche non égale

        const double max = (*(sortedEvents.end()-1))->mThetaReduced;
        double step = spreadSpan / (nbEgal-1.); // ecart théorique

        const double min = std::max(lowBound + spreadSpan, max - step*(nbEgal-1) );
        //const double max = value;
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
    return mModel->reduceEventsTheta(lEvent);
}

double MCMCLoopCurve::yearTime(double reduceTime)
{
    return mModel->yearTime(reduceTime) ;
}

#pragma mark Pratique pour debug
std::vector<double> MCMCLoopCurve::getThetaEventVector(const QList<Event *> &lEvent)
{
    std::vector<double> vecT(lEvent.size());
    //std::transform(std::execution::par, lEvent.begin(), lEvent.end(), vecT.begin(), [](Event* ev) {return ev->mTheta.mX;}); // not yet implemented
    std::transform( lEvent.begin(), lEvent.end(), vecT.begin(), [](Event* ev) {return ev->mTheta.mX;});

    return vecT;
}

std::vector<double> MCMCLoopCurve::getYEventVector(const QList<Event*>& lEvent)
{
    std::vector<double> vecY;
    std::transform(lEvent.begin(), lEvent.end(), vecY.begin(), [](Event* ev) {return ev->mY;});

    return vecY;
}

#pragma mark Calcul Spline

// dans RenCurve procedure Calcul_Mat_Q_Qt_R ligne 66; doit donner une matrice symetrique; sinon erreur dans cholesky
Matrix2D MCMCLoopCurve::calculMatR(const std::vector< double>& vecH)
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
    Matrix2D matR = initMatrix2D(n, n);
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
Matrix2D MCMCLoopCurve::calculMatQ(const std::vector<double>& vecH)
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
   Matrix2D matQ = initMatrix2D(n, n);
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


inline double diffX (Event* e0, Event*e1) {return (e1->mThetaReduced - e0->mThetaReduced);}

// An other function exist for vector CurveUtilities::calculVecH(const vector<double>& vec)
std::vector<double> MCMCLoopCurve::calculVecH(const QList<Event *> &lEvent)
{
    std::vector<double> result (lEvent.size()-1);
    std::transform(lEvent.begin(), lEvent.end()-1, lEvent.begin()+1 , result.begin(), diffX);

#ifdef DEBUG
    int i =0;
    for (auto &&r :result) {
        if (r <= 0.) {
            char th [200];
            //char num [] ;
            sprintf(th, "MCMCLoopCurve::calculVecH diff Theta null %.2f et %.2f", lEvent.at(i)->mThetaReduced, lEvent.at(i+1)->mThetaReduced);
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
std::vector<double> MCMCLoopCurve::createDiagWInv(const QList<Event*>& lEvents)
{
    std::vector<double> diagWInv (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), diagWInv.begin(), [](Event* ev) {return 1/ev->mW;});

    return diagWInv;
}

std::vector<double> MCMCLoopCurve::createDiagWInv_Vg0(const QList<Event*>& lEvents)
{
    std::vector<double> diagWInv (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), diagWInv.begin(), [](Event* ev) {return pow(ev->mSy, 2.);});

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
SplineMatrices MCMCLoopCurve::prepareCalculSpline(const QList<Event *>& sortedEvents, const std::vector<double> &vecH)
{
    Matrix2D matR = calculMatR(vecH);
    Matrix2D matQ = calculMatQ(vecH);
    
    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    Matrix2D matQT = transpose(matQ, 3);

    std::vector<double> diagWInv (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.begin(), [](Event* ev) {return 1/ev->mW;});

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    Matrix2D tmp = multiMatParDiag(matQT, diagWInv, 3);
    Matrix2D matQTW_1Q = multiMatParMat(tmp, matQ, 3, 3);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    Matrix2D matQTQ = multiMatParMat(matQT, matQ, 3, 3);

    SplineMatrices matrices;
    matrices.diagWInv = std::move(diagWInv);
    matrices.matR = std::move(matR);
    matrices.matQ = std::move(matQ);
    matrices.matQT = std::move(matQT);
    matrices.matQTW_1Q = std::move(matQTW_1Q); // Seule affectée par changement de VG
    matrices.matQTQ = std::move(matQTQ);
    
    return matrices;
}

SplineMatrices MCMCLoopCurve::prepareCalculSpline_W_Vg0(const QList<Event *>& sortedEvents, std::vector<double>& vecH)
{
    Matrix2D matR = calculMatR(vecH);
    Matrix2D matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    Matrix2D matQT = transpose(matQ, 3);

   // Diag Winv est égale à la diagonale des variances
    std::vector<double> diagWInv (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.begin(), [](Event* ev) {return pow(ev->mSy, 2.);});

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    Matrix2D tmp = multiMatParDiag(matQT, diagWInv, 3);
    Matrix2D matQTW_1Q = multiMatParMat(tmp, matQ, 3, 3);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    Matrix2D matQTQ = multiMatParMat(matQT, matQ, 3, 3);

    SplineMatrices matrices;
    matrices.diagWInv = std::move(diagWInv);
    matrices.matR = std::move(matR);
    matrices.matQ = std::move(matQ);
    matrices.matQT = std::move(matQT);
    matrices.matQTW_1Q = std::move(matQTW_1Q); // Seule affectée par changement de VG, ici VG=0
    matrices.matQTQ = std::move(matQTQ);

    return matrices;
}

/**
 * @brief MCMCLoopCurve::prepareCalculSpline_WI
 * With W = identity
 * @param sortedEvents
 * @param vecH
 * @return
 */
SplineMatrices MCMCLoopCurve::prepareCalculSpline_WI(const QList<Event *>& sortedEvents, const std::vector<double>& vecH)
{
    Matrix2D matR = calculMatR(vecH);
    Matrix2D matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    Matrix2D matQT = transpose(matQ, 3);

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    std::vector<double> diagWInv (sortedEvents.size(), 1);

    Matrix2D matQTW_1Q = multiMatParMat(matQT, matQ, 3, 3);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    Matrix2D matQTQ = multiMatParMat(matQT, matQ, 3, 3);

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
SplineResults MCMCLoopCurve::calculSpline(const SplineMatrices& matrices, const std::vector<double>& vecY, const double lambdaSpline, const std::vector<double>& vecH)
{
    SplineResults spline;
    const Matrix2D& matR = matrices.matR;
    const Matrix2D& matQ = matrices.matQ;
    const Matrix2D& matQTW_1Q = matrices.matQTW_1Q;

    try {
        // calcul de: R + lambda * Qt * W-1 * Q = Mat_B
        // Mat_B : matrice carrée (n-2) x (n-2) de bande 5 qui change avec alpha et Diag_W_1
        Matrix2D matB;

        if (lambdaSpline != 0) {
            const Matrix2D tmp = multiConstParMat(matQTW_1Q, lambdaSpline, 5);
            matB = addMatEtMat(matR, tmp, 5);

        } else {
            matB = matR;
        }

        // Decomposition_Cholesky de matB en matL et matD
        // Si lambda global: calcul de Mat_B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
        const std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matB, 5, 1);
        const Matrix2D matL = decomp.first;
        const std::vector<double> matD = decomp.second;

        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = mModel->mEvents.size();


        // Calcul du vecteur Vec_QtY, de dimension (n-2)

        std::vector<double> vecG (n);
        std::vector<double> vecQtY(n);

        for (size_t i = 1; i < n-1; ++i) {
            const double term1 = (vecY.at(i+1) - vecY.at(i)) / vecH.at(i);
            const double term2 = (vecY.at(i) - vecY.at(i-1)) / vecH.at(i-1);
            vecQtY[i] = term1 - term2;
        }

        // Calcul du vecteur Gamma
        std::vector<double> vecGamma = resolutionSystemeLineaireCholesky(matL, matD, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - lambda * W_1 * Q * gamma
        if (lambdaSpline != 0) {
            const std::vector<double> vecTmp2 = multiMatParVec(matQ, vecGamma, 3);
            const std::vector<double>& diagWInv = matrices.diagWInv;//createDiagWInv(mModel->mEvents);
            for (unsigned i = 0; i < n; ++i) {
                vecG[i] = vecY.at(i) - lambdaSpline * diagWInv.at(i) * vecTmp2.at(i);
            }

        } else {
            vecG = vecY;
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0., [](double sum, auto m) {return sum + (m == 0? 0: 1);}) == 0.) { // Pas forcement un problème
            qDebug() <<"MCMCLoopCurve::calculSpline vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0., [](double sum, auto m) {return sum + (m == 0? 0: 1);}) == 0.) {
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
SplineResults MCMCLoopCurve::calculSplineX(const SplineMatrices& matrices, const std::vector<double>& vecH, std::pair<Matrix2D, std::vector<double> > &decomp, const Matrix2D matB, const double lambdaSpline)
{

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


        const Matrix2D matL = decomp.first; // we must keep a copy
        std::vector<double> matD = decomp.second; // we must keep a copy

        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = mModel->mEvents.size();

        std::vector<double> vecG;
        std::vector<double> vecQtY;

        double term1, term2;

        // VecQtY doit être de taille n, donc il faut mettre un zéro au début et à la fin
        vecQtY.push_back(0.);
        for (size_t i = 1; i < n-1; ++i) {
            term1 = (mModel->mEvents[i+1]->mYx - mModel->mEvents[i]->mYx) / vecH[i];
            term2 = (mModel->mEvents[i]->mYx - mModel->mEvents[i-1]->mYx) / vecH[i-1];
            vecQtY.push_back(term1 - term2);
        }
        vecQtY.push_back(0.);

        // Calcul du vecteur Gamma
        std::vector<double> vecGamma = resolutionSystemeLineaireCholesky(matL, matD, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - lamnbda * W-1 * Q * gamma
        if (lambdaSpline != 0) {
            std::vector<double> vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            const std::vector<double>& diagWInv = matrices.diagWInv;//createDiagWInv(mModel->mEvents);

            for (unsigned i = 0; i < n; ++i) {
                vecG.push_back( mModel->mEvents[i]->mYx - lambdaSpline * diagWInv[i] * vecTmp2[i]) ;
            }

        } else {
            //vecG = vecY;
            vecG.resize(n);
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecG.begin(), [](Event* ev) {return ev->mYx;});
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0., [](double sum, auto m) {return sum + (m == 0? 0: 1);}) == 0.) {
            qDebug() <<"MCMCLoopCurve::calculSplineX vecG NULL";
        }
        // Check all term are null
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0., [](double sum, auto m) {return sum + (m == 0? 0: 1);}) == 0.) {
            qDebug() <<"MCMCLoopCurve::calculSplineX vecGamma NULL";
        }
#endif

        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);
        spline.matB = std::move(matB);
        spline.matL = std::move(matL);
        spline.matD = matD;

    } catch(...) {
                qCritical() << "MCMCLoopCurve::calculSplineX : Caught Exception!\n";
    }

    return spline;
}

/*
 * MatB doit rester en copie
 */
SplineResults MCMCLoopCurve::calculSplineY(const SplineMatrices& matrices, const  std::vector<double>& vecH, std::pair<Matrix2D, std::vector<double> > &decomp, const Matrix2D matB, const double lambdaSpline)
{

    SplineResults spline;
    try {

        const Matrix2D matL = decomp.first;
        const std::vector<double> matD = decomp.second;

        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = mModel->mEvents.size();

        std::vector< double> vecG;
        std::vector< double> vecQtY(n);

        for (size_t i = 1; i < n-1; ++i) {
             double term1 = (mModel->mEvents[i+1]->mYy - mModel->mEvents[i]->mYy) / vecH[i];
             double term2 = (mModel->mEvents[i]->mYy - mModel->mEvents[i-1]->mYy) / vecH.at(i-1);
            vecQtY[i] = term1 - term2;
        }

        // Calcul du vecteur Gamma
        std::vector< double> vecGamma = resolutionSystemeLineaireCholesky(matL, matD, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - alpha * W-1 * Q * gamma
        if (lambdaSpline != 0) {
            std::vector< double> vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            const std::vector< double>& diagWInv = matrices.diagWInv;//createDiagWInv(mModel->mEvents);
            for (unsigned i = 0; i < n; ++i) {
                vecG.push_back(mModel->mEvents[i]->mYy - lambdaSpline * diagWInv[i] * vecTmp2[i]);
            }

        } else {
            //vecG = vecY;
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecG.begin(), [](Event* ev) {return ev->mYy;});
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0., [](double sum, auto m) {return sum + (m == 0? 0: 1);}) == 0.)  {
            qDebug() <<"MCMCLoopCurve::calculSplineY vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0., [](double sum, auto m) {return sum + (m == 0? 0: 1);}) == 0.)  {
            qDebug() <<"MCMCLoopCurve::calculSplineY vecGamma NULL";
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
SplineResults MCMCLoopCurve::calculSplineZ(const SplineMatrices& matrices, const std::vector<double>& vecH, std::pair<Matrix2D, std::vector<double> > &decomp, const Matrix2D matB, const double lambdaSpline)
{
    SplineResults spline;
    try {

        const Matrix2D matL = decomp.first; // we must keep a copy
        const std::vector< double> matD = decomp.second; // we must keep a copy

        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = mModel->mEvents.size();


        // Calcul du vecteur Vec_QtY, de dimension (n-2)

        std::vector<double> vecG;
        std::vector<double> vecQtY(n);

        double term1, term2;
        for (size_t i = 1; i < n-1; ++i) {
             term1 = (mModel->mEvents[i+1]->mYz - mModel->mEvents[i]->mYz) / vecH[i];
             term2 = (mModel->mEvents[i]->mYz - mModel->mEvents[i-1]->mYz) / vecH[i-1];
            vecQtY[i] = term1 - term2;
        }

        // Calcul du vecteur Gamma
        std::vector<double> vecGamma = resolutionSystemeLineaireCholesky(matL, matD, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - lambda * W-1 * Q * gamma
        if (lambdaSpline != 0) {
            std::vector<double> vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            const std::vector<double>&  diagWInv = matrices.diagWInv;
            for (unsigned i = 0; i < n; ++i) {
                vecG.push_back( mModel->mEvents[i]->mYz - lambdaSpline * diagWInv[i] * vecTmp2[i]);
            }

        } else {
            //vecG = vecY;
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecG.begin(), [](Event* ev) {return ev->mYz;});
        }

#ifdef DEBUG
        if (std::accumulate(vecG.begin(), vecG.end(), 0., [](double sum, auto m) {return sum + (m == 0? 0: 1);}) == 0.)  {
            qDebug() <<"MCMCLoopCurve::calculSplineZ vecG NULL";
        }
        if (std::accumulate(vecGamma.begin(), vecGamma.end(), 0., [](double sum, auto m) {return sum + (m == 0? 0: 1);}) == 0.)  {
            qDebug() <<"MCMCLoopCurve::calculSplineZ vecGamma NULL";
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
std::vector<double> MCMCLoopCurve::calculMatInfluence_origin(const SplineMatrices& matrices, const SplineResults& splines , const int nbBandes, const double lambdaSpline)
{
    const size_t n = mModel->mEvents.size();
    std::vector<double> matA;// = initVector(n);

    if (lambdaSpline != 0) {

       // const std::vector<std::vector<long double>>& matL = decomp.first; // we must keep a copy ?
       // const std::vector<long double>& matD = decomp.second; // we must keep a copy ?
        Matrix2D matB_1 = inverseMatSym_origin(splines.matL, splines.matD, nbBandes + 4, 1);

        std::vector<double> matQB_1QT = initVector(n);

        auto matQi = matrices.matQ[0];
        double term = pow(matQi[0], 2) * matB_1[0][0];
        term += pow(matQi[1], 2) * matB_1[1][1];
        term += 2 * matQi[0] * matQi[1] * matB_1[0][1];
        matQB_1QT[0] = term;

        for (size_t i = 1; i < n-1; ++i) {
            matQi = matrices.matQ[i];
            term = pow(matQi[i-1], 2.) * matB_1[i-1][i-1];
            term += pow(matQi[i], 2.) * matB_1[i][i];
            term += pow(matQi[i+1], 2.) * matB_1[i+1][i+1];
            term += 2 * matQi[i-1] * matQi[i] * matB_1[i-1][i];
            term += 2 * matQi[i-1] * matQi[i+1] * matB_1[i-1][i+1];
            term += 2 * matQi[i] * matQi[i+1] * matB_1[i][i+1];
            matQB_1QT[i] = term;
        }

        matQi = matrices.matQ[n-1];
        term = pow(matQi[n-2], 2.) * matB_1[n-2][n-2];
        term += pow(matQi[n-1], 2.) * matB_1[n-1][n-1];
        term += 2 * matQi[n-2] * matQi[n-1] * matB_1[n-2][n-1];
        matQB_1QT[n-1] = term;

        // Multi_diag_par_Mat(Diag_W_1c,Mat_QB_1QT,Nb_noeudsc,1,tmp1);
        // Multi_const_par_Mat(-alphac,tmp1,Nb_noeudsc,1,Mat_Ac);
        // Addit_I_et_Mat(Mat_Ac,Nb_noeudsc);
        // remplacé par:
        for (size_t i = 0; i < n; ++i) {

            matA.push_back(1 - lambdaSpline * matrices.diagWInv[i] * matQB_1QT[i]);
#if DEBUG
            if (matA[i] < 0.) {
                qWarning ("MCMCLoopCurve::calculMatInfluence -> Oups matA.at(i) < 0  change to 0");
                matA[i] = 0.;
            }

            if (matA[i] == 0.) {
                qWarning ("MCMCLoopCurve::calculMatInfluence -> Oups matA.at(i) == 0  change to 0");
                matA[i] = 0;
            }

            if (matA[i] > 1) {
                qWarning ("MCMCLoopCurve::calculMatInfluence -> Oups matA.at(i) > 1  change to 1");
                matA[i] = 1.;
            }
#endif
        }

    } else {
        matA.resize(n, 1.);
    }

    return matA;
}




std::vector<double> MCMCLoopCurve::calculSplineError_origin(const SplineMatrices& matrices, const SplineResults& splines, const double lambdaSpline)
{
    unsigned int n = mModel->mEvents.size();
    std::vector<double> matA = calculMatInfluence_origin(matrices, splines, 1, lambdaSpline);
    std::vector<double> errG;

    for (unsigned int i = 0; i < n; ++i) {
#ifdef DEBUG
        const double& aii = matA.at(i);
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii <= 0) {
            throw "Oups";
        }
#endif
        errG.push_back(sqrt(matA[i]  / mModel->mEvents[i]->mW));
    }

    return errG;
}
std::vector<double> MCMCLoopCurve::calcul_spline_variance(const SplineMatrices& matrices, const SplineResults& splines, const double lambdaSpline)
{
    unsigned int n = mModel->mEvents.size();
    std::vector<double> matA = calculMatInfluence_origin(matrices, splines, 1, lambdaSpline);
    std::vector<double> varG;

    for (unsigned int i = 0; i < n; ++i) {
#ifdef DEBUG
        const double& aii = matA[i];
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii <= 0.) {
            qDebug()<<" calcul_spline_variance : Oups aii <= 0";
            varG.push_back(0.);

        } else {
            varG.push_back(matA[i]  / mModel->mEvents[i]->mW);
        }
#else
         varG.push_back(matA[i]  / mModel->mEvents[i]->mW);
#endif

    }

    return varG;
}

MCMCSpline MCMCLoopCurve::currentSpline ( QList<Event *> &lEvents, bool doSortAndSpreadTheta, std::vector<double> vecH, SplineMatrices matrices)
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

    std::vector<double> vecTheta = getThetaEventVector(lEvents);

    // calculSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    Matrix2D matB; //matR;
    const double lambda = mModel->mLambdaSpline.mX;
    if (lambda != 0) {
        Matrix2D tmp = multiConstParMat(matrices.matQTW_1Q, lambda, 5);
        matB = addMatEtMat(matrices.matR, tmp, 5);

    } else {
        matB = matrices.matR;
    }

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matB, 5, 1);


    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédent ne change pas

    SplineResults s = calculSplineX(matrices, vecH, decomp, matB, lambda); // Voir si matB est utile ???
    std::vector<double> vecVarG = calcul_spline_variance(matrices, s, lambda); // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

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

// obsolete
/*
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
*/

// obsolete
/*
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
*/

bool  MCMCLoopCurve::hasPositiveGPrimeByDet (const MCMCSplineComposante& splineComposante)
{

    for (unsigned long i= 0; i< splineComposante.vecThetaEvents.size()-1; i++) {

        const double t_i = mModel->reduceTime(splineComposante.vecThetaEvents.at(i));
        const double t_i1 = mModel->reduceTime(splineComposante.vecThetaEvents.at(i+1));
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
        if (delta < 0) {
             //qDebug()<<" hasPositiveGPrimeByDet delta < 0";
             return false;
        }

        double t1_res = (-bDelta - sqrt(delta)) / (2*aDelta);
        double t2_res = (-bDelta + sqrt(delta)) / (2*aDelta);

        if (a < 0) {
            return false;

        } else if ( t_i < t1_res && t1_res< t_i1) {
                return false;

        } else if ( t_i < t2_res && t2_res< t_i1) {
            return false;
        }

    }

    return true;
}

bool MCMCLoopCurve::hasPositiveGPrimeByDerivate (const MCMCSplineComposante& splineComposante, const double k)
{

    for (unsigned long i= 0; i< splineComposante.vecThetaEvents.size()-1; i++) {

        const double t_i = mModel->reduceTime(splineComposante.vecThetaEvents[i]);
        const double t_i1 = mModel->reduceTime(splineComposante.vecThetaEvents[i+1]);
        const double hi = t_i1 - t_i;

        const double gamma_i = splineComposante.vecGamma[i];
        const double gamma_i1 = splineComposante.vecGamma[i+1];

        const double g_i = splineComposante.vecG[i];
        const double g_i1 = splineComposante.vecG[i+1];

        const double a = (g_i1 - g_i) /hi;
        const double b = (gamma_i1 - gamma_i) /(6*hi);
        const double s = t_i + t_i1;
        const double p = t_i * t_i1;
        const double d = ( (t_i1 - 2*t_i)*gamma_i1 + (2*t_i1 - t_i)*gamma_i )/(6*hi);

        const double aDelta = 3* b ;
        const double bDelta = 2*d - 2*s*b;
        double cDelta = p*b - s*d + a;

        const double delta_0 = (pow(bDelta, 2.) - 4*aDelta*cDelta);
        //const double yVertex =  - pow(bDelta, 2.)/ (4.*aDelta) + cDelta ;

        if (aDelta < 0) {
            double delta_prim ;
            try {

                if (delta_0 < 0)
                    delta_prim  = - delta_0 * k ;
                else
                    delta_prim  = delta_0 * (1 + k) ;
            }
            catch(...) { // happen when 1/ k = INF, on accepte tout
                continue;
            }

            double t2_res = (-bDelta - sqrt(delta_prim )) / (2*aDelta);
            double t1_res = (-bDelta + sqrt(delta_prim )) / (2*aDelta);

            //C'est un maximum entre les solutions
            if ( !( t1_res < t_i && t_i1 < t2_res) )
                return false;


        } else {

        //const double xmin = -bDelta/(2*aDelta);
        //const double ymin = aDelta*xmin*xmin + bDelta*xmin + cDelta ;


    //    if (yVertex < 0) {
            //cDelta -= ymin * dy; // remontage du niveau en fonction de dy%
            //const double delta = pow(bDelta, 2.) - 4*aDelta*cDelta;

             const double delta_prim = (1-k) * delta_0;

             if (delta_0 < 0) // On accepte tout
                 continue;

             double t1_res = (-bDelta - sqrt(delta_0 * (1 - k))) / (2*aDelta);
             double t2_res = (-bDelta + sqrt(delta_0 * (1 - k))) / (2*aDelta);

            if (t1_res>t2_res)
                std::swap(t1_res, t2_res);

            if (delta_prim > 0) {
                //C'est un minimum entre les solutions
                if ( t1_res < t_i && t_i < t2_res)
                    return false;
                else if ( t1_res < t_i1 && t_i1 < t2_res)
                    return false;

            }
      //  }

        } // aDelta>0
    }
    return true;
}

/**
 * @brief MCMCLoopCurve::hasPositiveGPrimePlusConst
 * @param splineComposante
 * @param dy in Y unit per time unit ex meter/Year
 * @return
 */
bool MCMCLoopCurve::hasPositiveGPrimePlusConst(const MCMCSplineComposante& splineComposante, const double dy_threshold)
{

    double t_i, t_i1, hi, gamma_i, gamma_i1, g_i, g_i1, a, b, s, p, d;
    double aDelta, bDelta, cDelta, delta, t1_res, t2_res;

   // double yVertex, yVertex_new, Gpi, Gpi1; // pour debug

    const double dY = - dy_threshold * (mModel->mSettings.mTmax - mModel->mSettings.mTmin); // On doit inverser le signe et passer en temps réduit

    for (unsigned long i= 0; i< splineComposante.vecThetaEvents.size()-1; i++) {

         t_i = mModel->reduceTime(splineComposante.vecThetaEvents[i]);
         t_i1 = mModel->reduceTime(splineComposante.vecThetaEvents[i+1]);
         hi = t_i1 - t_i;

        gamma_i = splineComposante.vecGamma[i];
        gamma_i1 = splineComposante.vecGamma[i+1];

        g_i = splineComposante.vecG[i];
        g_i1 = splineComposante.vecG[i+1];

        a = (g_i1 - g_i) /hi;
        b = (gamma_i1 - gamma_i) /(6*hi);
        s = t_i + t_i1;
        p = t_i * t_i1;
        d = ( (t_i1 - 2*t_i)*gamma_i1 + (2*t_i1 - t_i)*gamma_i ) / (6*hi);

        // résolution équation

        aDelta = 3* b;
        bDelta = 2*d - 2*s*b;
        cDelta = p*b - s*d + a;

        /* for DEBUG
        yVertex =  - pow(bDelta, 2.)/ (4.*aDelta) + cDelta ;
        Gpi = aDelta*pow(t_i, 2.) + bDelta*t_i + cDelta;
        Gpi1 = aDelta*pow(t_i1, 2.) + bDelta*t_i1 + cDelta;
        yVertex_new =  - pow(bDelta, 2.)/ (4.*aDelta) + cDelta+dY ;
        */

        delta = pow(bDelta, 2.) - 4*aDelta*(cDelta + dY);


        if (delta < 0) { // No solution
            if (aDelta < 0)
                return false;
            else
                continue;
        }


        t1_res = (-bDelta - sqrt(delta)) / (2*aDelta);
        t2_res = (-bDelta + sqrt(delta)) / (2*aDelta);

        if (t1_res > t2_res)
            std::swap(t1_res, t2_res);

        if (aDelta > 0) { //C'est un maximum entre les solutions
            if (!( t_i1 < t1_res || t2_res< t_i)) {
                return false;
            }

        } else { //C'est un minimum entre les solutions
            if ( !( t1_res < t_i && t_i1 < t2_res) )
                return false;
        }

    }

    return true;
}


