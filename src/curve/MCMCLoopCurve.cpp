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

#include "MCMCLoopCurve.h"

#include "CalibrationCurve.h"
#include "ModelCurve.h"
#include "CurveUtilities.h"
#include "Bound.h"
#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "Date.h"
#include "Project.h"
#include "ModelUtilities.h"
#include "Matrix.h"

#ifdef DEBUG
#include "QtUtilities.h"
#endif

#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QTime>
#include <QProgressDialog>

#include <errno.h>      /* errno, EDOM */
#include <fenv.h>
#include <exception>
#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <time.h>
#include <chrono>


#ifdef _WIN32
#include <QtWidgets>
#include "winbase.h"
#endif

MCMCLoopCurve::MCMCLoopCurve(ModelCurve* model, Project* project):
    MCMCLoop(project)
{
    mModel = model;
    if (mModel) {
        setMCMCSettings(mModel->mMCMCSettings);
    }
    
    QJsonObject state = project->mState;
    mCurveSettings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());
}

MCMCLoopCurve::~MCMCLoopCurve()
{
    mModel = nullptr;
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
            size_t num_dates = ev->mDates.size();
            for (size_t j = 0; j<num_dates; ++j) {
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
                if (date->mCalibration->mVector.isEmpty())
                    date->calibrate(mProject);

                if (date->mCalibration->mVector.size() < 6) {
                    const double new_step = date->mCalibration->mStep /5.;
                    date->mCalibration->mVector.clear();
                    date->mCalibration->mMap.clear();
                    date->mCalibration->mRepartition.clear();
                    date->mCalibration = nullptr;

                    QString mes = tr("Definition of the calibration curve insufficient for the Event %1 \r Decrease the study period step to %2").arg(date->mName, QString::number(new_step));
                    return (mes);

                }


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
 * Idem MCMCLoopChrono + initialisation de VG (events) et Lambda Spline (global)
 */

QString MCMCLoopCurve::initialize()
{
    QString initTime = initialize_time(mModel);
    if (initTime != QString())
        return initTime;

    mComputeY = ( mCurveSettings.mProcessType != CurveSettings::eProcessTypeUnivarie);
    mComputeZ = ( mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
                        mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
                        mCurveSettings.mProcessType == CurveSettings::eProcessType3D);

    if (mCurveSettings.mLambdaSplineType == CurveSettings::eInterpolation)
        return initialize_interpolate();
    else
        return initialize_321();
    //return initialize_Komlan();

}

QString MCMCLoopCurve::initialize_time0()
{
    tminPeriod = mModel->mSettings.mTmin;
    tmaxPeriod = mModel->mSettings.mTmax;

    QList<Event*>& allEvents (mModel->mEvents);
    QList<Phase*>& phases (mModel->mPhases);
    QList<PhaseConstraint*>& phasesConstraints (mModel->mPhaseConstraints);

    if (isInterruptionRequested())
        return ABORTED_BY_USER;
    // initialisation des bornes
    // ---------------------- Reset Events ---------------------------
    for (Event* ev : allEvents) {
        ev->mInitialized = false;
        ev->mS02.mSamplerProposal = MHVariable::eFixe;
        // ev->mS02.mSamplerProposal = MHVariable::eMHAdaptGauss; // not yet integrate within update_321
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
                    bound->mThetaReduced = mModel->reduceTime(bound->mTheta.mX);
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

    QVector<Event*> unsortedEvents = ModelUtilities::unsortEvents(allEvents);

    emit stepChanged(tr("Initializing Events..."), 0, unsortedEvents.size());
    try {

        // Check Strati constraint
        for (Event* ev : unsortedEvents) {
            mModel->initNodeEvents();
            QString circularEventName = "";
            QList<Event*> startEvents = QList<Event*>();

            const bool ok (ev->getThetaMaxPossible (ev, circularEventName, startEvents));
            if (!ok) {
                mAbortedReason = QString(tr("Warning : Find Circular Constraint Path %1  %2 ")).arg (ev->mName, circularEventName);
                return mAbortedReason;
            }

            // Controle la cohérence des contraintes strati-temporelle et des valeurs de profondeurs
            if (mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth ) {
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

                    mModel->initNodeEvents(); // Doit être réinitialisé pour toute recherche getThetaMinRecursive et getThetaMaxRecursive
                    QString circularEventName = "";
                    
                    const double min = uEvent->getThetaMinRecursive (tminPeriod);
                    
                    // ?? Comment circularEventName peut-il être pas vide ?
                    if (!circularEventName.isEmpty()) {
                        mAbortedReason = QString(tr("Warning : Find Circular constraint with %1  bad path  %2 ")).arg(uEvent->mName, circularEventName);
                        return mAbortedReason;
                    }
                    
                    mModel->initNodeEvents();
                    const double max = uEvent->getThetaMaxRecursive(tmaxPeriod);
#ifdef DEBUG
                    if (min >= max)
                        qDebug() << tr("-----Error Init for event : %1 : min = %2 : max = %3-------").arg(uEvent->mName, QString::number(min, 'f', 30), QString::number(max, 'f', 30));
#endif
                    if (min >= max) {
                        mAbortedReason = QString(tr("Error Init for event : %1 : min = %2 : max = %3-------").arg(uEvent->mName, QString::number(min, 'f', 30), QString::number(max, 'f', 30)));
                        return mAbortedReason;
                    }
                    // ----------------------------------------------------------------
                    // Curve init Theta event :
                    // On initialise les theta près des dates ti
                    // ----------------------------------------------------------------
                    
                    sampleInCumulatedRepartition(uEvent, mModel->mSettings, min, max);
                    
                    uEvent->mThetaReduced = mModel->reduceTime(uEvent->mTheta.mX);
                    uEvent->mInitialized = true;
                    
                    // ----------------------------------------------------------------
                    
                    double s02_sum = 0.;
                    
                    for (Date& date : uEvent->mDates) {
                        
                        // 1 - Init ti

                        // modif du 2021-06-16 pHd
                        const FunctionStat &data = analyseFunction(date.mCalibration->mMap);
                        double sigma = double (data.std);
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
                    uEvent->mS02.mX = uEvent->mDates.size() / s02_sum;
                    
                    // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
                    uEvent->mTheta.mSigmaMH = sqrt(uEvent->mS02.mX);
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
                    sampleInCumulatedRepartition_thetaFixe(uEvent, mModel->mSettings);
                else
                    uEvent->mTheta.mX = static_cast<Bound*>(uEvent)->mFixed;
                // nous devons sauvegarder la valeur ici car dans loop.memo(), les variables fixes ne sont pas memorisées.
                // Pourtant, il faut récupèrer la valeur pour les affichages et les stats
                uEvent->mTheta.memo();

                uEvent->mThetaReduced = mModel->reduceTime(uEvent->mTheta.mX);
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

                // 4 - Init S02 of each Event
                uEvent->mS02.mX = 0;
                uEvent->mS02.mLastAccepts.clear();
                uEvent->mS02.mSamplerProposal = MHVariable::eFixe;

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
            for (Event* ev : mModel->mEvents) {
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

QString MCMCLoopCurve::initialize_321()
{
    updateLoop = &MCMCLoopCurve::update_321;
    QList<Event*>& allEvents (mModel->mEvents);

    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
        mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (Event* ev : allEvents) {
            if (ev->mPointType == Event::eNode)
                mNodeEvent.push_back(ev);
            else
                mPointEvent.push_back(ev);

            //ev->mS02.mSamplerProposal = MHVariable::eMHAdaptGauss; // not yet integrate within update_321
        }
    } else {
        for (Event* ev : allEvents)
            mPointEvent.push_back(ev);
    }


    // -------------------------------- SPLINE part--------------------------------
    // Init function G

    prepareEventsY(allEvents);

    emit stepChanged(tr("Initializing G ..."), 0, allEvents.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    // ----------------------------------------------------------------
    //  Init Lambda Spline
    // ----------------------------------------------------------------

    std::vector<t_reduceTime> vecH = calculVecH(mModel->mEvents);
    SplineMatrices matricesWI = prepareCalculSpline_WI(mModel->mEvents, vecH);
    try {
        if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed) {
            mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;
            mModel->mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
            double memoLambda = log10(mModel->mLambdaSpline.mX);
            mModel->mLambdaSpline.memo(&memoLambda);

        } else {
            mModel->mLambdaSpline.mX = 1E-6;
            mModel->mLambdaSpline.mLastAccepts.clear();
            mModel->mLambdaSpline.tryUpdate(mModel->mLambdaSpline.mX, 2.);

        }
        mModel->mLambdaSpline.mSigmaMH = 1.;

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
            // S02_Vg_Yx() Utilise la valeur de lambda courant, sert à initialise S02_Vg
            const auto var_residu_X = S02_Vg_Yx(mModel->mEvents, matricesWI, vecH, mModel->mLambdaSpline.mX);
            //std::cout<<" var_residu_X = " << var_residu_X;
            if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeUnivarie) {
                Var_residual_spline = var_residu_X;

            } else {
                    const auto var_residu_Y = S02_Vg_Yy(mModel->mEvents, matricesWI, vecH, mModel->mLambdaSpline.mX);

                    if ( mComputeZ) {
                        const auto var_residu_Z =  S02_Vg_Yz(mModel->mEvents, matricesWI, vecH, mModel->mLambdaSpline.mX);

                        Var_residual_spline = (var_residu_X + var_residu_Y + var_residu_Z)/3.;

                    } else {
                        Var_residual_spline = (var_residu_X + var_residu_Y)/2.;
                    }

            }
            //std::cout<<" Var_residual_spline theorique = " << Var_residual_spline<<" inv= "<< 1/Var_residual_spline <<"\n";
            //---------------------------


        }

        // memo Vg
        /* ----------------------------------------------------------------
         * The W of the events depend only on their VG
         * During the update, we need W for the calculations of theta, VG and Lambda Spline update
         * We will thus have to update the W at each VG modification
         * We calculate it here during the initialization to have its starting value
         * ---------------------------------------------------------------- */

        int i = 0;
        if (mCurveSettings.mUseVarianceIndividual) {
            for (Event* &e : mPointEvent) {
                i++;
                e->mVg.mX = Var_residual_spline;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mAllAccepts->clear(); //don't clean, avalable for cumulate chain


                if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                } else {
                    e->mVg.tryUpdate(e->mVg.mX, 2.);
                }
                e->updateW();
                e->mVg.mSigmaMH = 1.;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }

            for (Event* &e : mNodeEvent) {
                i++;
                e->mVg.mX = 0.;
                e->mVg.mSamplerProposal = MHVariable::eFixe;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mAllAccepts->clear(); //don't clean, avalable for cumulate chain

                // check if Sy == 0
                if (e->mSy == 0) {
                    mAbortedReason = QString("Error: a Node cannot have a null error \n Change error in : ") + e->mName;
                    return mAbortedReason;
                }
                e->updateW();
                double memoVG = sqrt(e->mVg.mX);
                e->mVg.memo(&memoVG);
                e->mVg.mSigmaMH = 1.;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }

       } else {
            // Pas de Noeud dans le cas de Vg Global
            for (Event* &e : allEvents) {
                i++;
                if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
                    e->mVg.mX = mCurveSettings.mVarianceFixed;
                    e->mVg.mSamplerProposal = MHVariable::eFixe;
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                    // Check if Sy + Vg == 0
                    if (e->mVg.mX + e->mSy * e->mSy == 0) {
                        mAbortedReason = QString("Error: a Node cannot have a null error with Variance null \n Change error in : ") + e->mName;
                        return mAbortedReason;
                    }

                } else {
                    e->mPointType = Event::ePoint; // force Node to be a simple Event
                    e->mVg.mX = Var_residual_spline;
                    e->mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;
                }
                e->mVg.mLastAccepts.clear();
                // e->mVg.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
                e->mVg.tryUpdate(e->mVg.mX, 2.);
                e->updateW();

                e->mVg.mSigmaMH = 1.;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }
        }


    }  catch (...) {
        qWarning() << "Curve init Vg  ???";
        mAbortedReason = QString("Curve init Vg  ???");
        return mAbortedReason;
    }

    // ----------------------------------------------------------------
    // Curve init S02 Vg
    // ----------------------------------------------------------------
    mModel->mS02Vg.mX = Var_residual_spline;
    mModel->mS02Vg.mLastAccepts.clear();
    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
        mModel->mS02Vg.mSamplerProposal = MHVariable::eFixe;
        double memoS02 = sqrt(mModel->mS02Vg.mX);
        mModel->mS02Vg.memo(&memoS02);

    } else {
        mModel->mS02Vg.tryUpdate(Var_residual_spline, 2.);
    }


    mModel->mS02Vg.mSigmaMH = 1.;

    if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeUnivarie) {
        std::vector<double> vecY (mModel->mEvents.size());
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
        mAbortedReason = QString(tr("Error : Variance on Y is null, do computation with Variance G fixed  = 0 for this model "));
        return mAbortedReason;
    }
    // --------------------------- Current spline ----------------------
    try {
        /* --------------------------------------------------------------
         *  Calcul de la spline g, g" pour chaque composante x y z
         *-------------------------------------------------------------- */
        mModel->mSpline = currentSpline(mModel->mEvents, true);

        // init Posterior MeanG and map
        const int nbPoint = 300;// Density curve size and curve size

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapG.min_value = +INFINITY;
        clearCompo.mapG.max_value = 0;

        clearCompo.vecG = std::vector<double> (nbPoint); // column
        clearCompo.vecGP = std::vector<double> (nbPoint);
        clearCompo.vecGS = std::vector<double> (nbPoint);
        clearCompo.vecVarG = std::vector<double> (nbPoint);
        clearCompo.vecVarianceG = std::vector<double> (nbPoint);
        clearCompo.vecVarErrG = std::vector<double> (nbPoint);

        PosteriorMeanG clearMeanG;
        clearMeanG.gx = clearCompo;

        double minY = +INFINITY;
        double maxY = -INFINITY;
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, Event* e) {return std::min(e->mYx, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, Event* e) {return std::max(e->mYx, x);});

        int i = 0;
        for (auto g : mModel->mSpline.splineX.vecG) {
            const auto e = 1.96*sqrt(mModel->mSpline.splineX.vecVarG.at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }

        clearMeanG.gx.mapG.setRangeY(minY, maxY);

        if ( mComputeY) {
            clearMeanG.gy = clearCompo;

            minY = +INFINITY;
            maxY = -INFINITY;

            minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double y, Event* e) {return std::min(e->mYy, y);});
            maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double y, Event* e) {return std::max(e->mYy, y);});


            int i = 0;
            for (auto g : mModel->mSpline.splineY.vecG) {
                const auto e = 1.96*sqrt(mModel->mSpline.splineY.vecVarG.at(i));
                minY = std::min(minY, g - e);
                maxY = std::max(maxY, g + e);
                i++;
            }


            clearMeanG.gy.mapG.setRangeY(minY, maxY);

            if (mComputeZ) {
                clearMeanG.gz = clearCompo;

                minY = +INFINITY;
                maxY = -INFINITY;

                minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double z, Event* e) {return std::min(e->mYz, z);});
                maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double z, Event* e) {return std::max(e->mYz, z);});

                int i = 0;
                for (auto g : mModel->mSpline.splineZ.vecG) {
                    const auto e = 1.96*sqrt(mModel->mSpline.splineZ.vecVarG.at(i));
                    minY = std::min(minY, g - e);
                    maxY = std::max(maxY, g + e);
                    i++;
                }

                clearMeanG.gz.mapG.setRangeY(minY, maxY);
            }

        }

        // Convertion IDF
        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||  mModel->mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical) {

            const double deg = 180. / M_PI ;
            // 1 - new extrenum
            const double gzFmax = sqrt(pow(clearMeanG.gx.mapG.maxY(), 2.) + pow(clearMeanG.gy.mapG.maxY(), 2.) + pow(clearMeanG.gz.mapG.maxY(), 2.));
            const double gxIncMax = asin(clearMeanG.gz.mapG.maxY() / gzFmax) * deg;
            const double gyDecMax = atan2(clearMeanG.gy.mapG.maxY(), clearMeanG.gx.mapG.maxY()) * deg;

            const double gzFmin = sqrt(pow(clearMeanG.gx.mapG.minY(), 2.) + pow(clearMeanG.gy.mapG.minY(), 2.) + pow(clearMeanG.gz.mapG.minY(), 2.));
            const double gxIncMin = asin(clearMeanG.gz.mapG.minY() / gzFmin) * deg;
            const double gyDecMin = atan2(clearMeanG.gy.mapG.minY(), clearMeanG.gx.mapG.minY()) * deg;

            clearMeanG.gx.mapG.setRangeY(gxIncMin, gxIncMax);
            clearMeanG.gy.mapG.setRangeY(gyDecMin, gyDecMax);
            clearMeanG.gz.mapG.setRangeY(gzFmin, gzFmax);

        }


        mModel->mPosteriorMeanGByChain.push_back(clearMeanG);
        if (mChainIndex == 0)
            mModel->mPosteriorMeanG = clearMeanG;

    }  catch (...) {
        qWarning() <<"init Posterior MeanG and map  ???";
    }

    /*
     * INIT UPDATE
     */
    // init the current state
    try {
        initListEvents.resize(mModel->mEvents.size());
        std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    } catch (...) {
        qWarning() <<"init Posterior MeanG and map  ???";
        return QString("[MCMCLoopCurve::initialize_321] problem");
    }


    return QString();
}

QString MCMCLoopCurve::initialize_Komlan()
{
    updateLoop = &MCMCLoopCurve::update_Komlan;
    QList<Event*>& allEvents (mModel->mEvents);

    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
        mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (Event* ev : allEvents) {
            if (ev->mPointType == Event::eNode)
                mNodeEvent.push_back(ev);
            else
                mPointEvent.push_back(ev);

            ev->mS02.mSupport = MHVariable::eRpStar;
            ev->mS02.mSamplerProposal = MHVariable::eMHAdaptGauss;
            ev->mS02.mSigmaMH = 1.;
        }
    } else {
        for (Event* ev : allEvents)
            mPointEvent.push_back(ev);
    }


    // -------------------------------- SPLINE part--------------------------------
    // Init function G

    prepareEventsY(allEvents);

    emit stepChanged(tr("Initializing G ..."), 0, allEvents.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    // ----------------------------------------------------------------
    //  Init Lambda Spline
    // ----------------------------------------------------------------

    std::vector<t_reduceTime> vecH = calculVecH(mModel->mEvents);
    SplineMatrices matricesWI = prepareCalculSpline_WI(mModel->mEvents, vecH);
    try {
        if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed) {
            mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;
            mModel->mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
            double memoLambda = log10(mModel->mLambdaSpline.mX);
            mModel->mLambdaSpline.memo(&memoLambda);

        } else {
            mModel->mLambdaSpline.mX = 1E-6;
            mModel->mLambdaSpline.mLastAccepts.clear();
            mModel->mLambdaSpline.tryUpdate(mModel->mLambdaSpline.mX, 2.);

        }
        mModel->mLambdaSpline.mSigmaMH = 1.;

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
            // S02_Vg_Yx() Utilise la valeur de lambda courant, sert à initialise S02_Vg
            const auto var_residu_X = S02_Vg_Yx(mModel->mEvents, matricesWI, vecH, mModel->mLambdaSpline.mX);
            //std::cout<<" var_residu_X = " << var_residu_X;
            if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeUnivarie) {
                Var_residual_spline = var_residu_X;

            } else {
                const auto var_residu_Y = S02_Vg_Yy(mModel->mEvents, matricesWI, vecH, mModel->mLambdaSpline.mX);

                if ( mComputeZ) {
                    const auto var_residu_Z =  S02_Vg_Yz(mModel->mEvents, matricesWI, vecH, mModel->mLambdaSpline.mX);

                    Var_residual_spline = (var_residu_X + var_residu_Y + var_residu_Z)/3.;

                } else {
                    Var_residual_spline = (var_residu_X + var_residu_Y)/2.;
                }

            }
            //std::cout<<" Var_residual_spline theorique = " << Var_residual_spline<<" inv= "<< 1/Var_residual_spline <<"\n";
            //---------------------------


        }

        // memo Vg
        /* ----------------------------------------------------------------
         * The W of the events depend only on their VG
         * During the update, we need W for the calculations of theta, VG and Lambda Spline update
         * We will thus have to update the W at each VG modification
         * We calculate it here during the initialization to have its starting value
         * ---------------------------------------------------------------- */

        int i = 0;
        if (mCurveSettings.mUseVarianceIndividual) {
            for (Event* &e : mPointEvent) {
                i++;
                e->mVg.mX = Var_residual_spline;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mAllAccepts->clear(); //don't clean, avalable for cumulate chain


                if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                } else {
                    e->mVg.tryUpdate(e->mVg.mX, 2.);
                }
                e->updateW();
                e->mVg.mSigmaMH = 1.;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }

            for (Event* &e : mNodeEvent) {
                i++;
                e->mVg.mX = 0.;
                e->mVg.mSamplerProposal = MHVariable::eFixe;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mAllAccepts->clear(); //don't clean, avalable for cumulate chain

                // check if Sy == 0
                if (e->mSy == 0) {
                    mAbortedReason = QString("Error: a Node cannot have a null error \n Change error in : ") + e->mName;
                    return mAbortedReason;
                }
                e->updateW();
                double memoVG = sqrt(e->mVg.mX);
                e->mVg.memo(&memoVG);
                e->mVg.mSigmaMH = 1.;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }

        } else {
            // Pas de Noeud dans le cas de Vg Global
            for (Event* &e : allEvents) {
                i++;
                if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
                    e->mVg.mX = mCurveSettings.mVarianceFixed;
                    e->mVg.mSamplerProposal = MHVariable::eFixe;
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                    // Check if Sy + Vg == 0
                    if (e->mVg.mX + e->mSy * e->mSy == 0) {
                        mAbortedReason = QString("Error: a Node cannot have a null error with Variance null \n Change error in : ") + e->mName;
                        return mAbortedReason;
                    }

                } else {
                    e->mPointType = Event::ePoint; // force Node to be a simple Event
                    e->mVg.mX = Var_residual_spline;
                    e->mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;
                }
                e->mVg.mLastAccepts.clear();
                // e->mVg.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
                e->mVg.tryUpdate(e->mVg.mX, 2.);
                e->updateW();

                e->mVg.mSigmaMH = 1.;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }
        }


    }  catch (...) {
        qWarning() << "Curve init Vg  ???";
        mAbortedReason = QString("Curve init Vg  ???");
        return mAbortedReason;
    }

    // ----------------------------------------------------------------
    // Curve init S02 Vg
    // ----------------------------------------------------------------
    mModel->mS02Vg.mX = Var_residual_spline;
    mModel->mS02Vg.mLastAccepts.clear();
    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
        mModel->mS02Vg.mSamplerProposal = MHVariable::eFixe;
        double memoS02 = sqrt(mModel->mS02Vg.mX);
        mModel->mS02Vg.memo(&memoS02);

    } else {
        mModel->mS02Vg.tryUpdate(Var_residual_spline, 2.);
    }


    mModel->mS02Vg.mSigmaMH = 1.;

    if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeUnivarie) {
        std::vector<double> vecY (mModel->mEvents.size());
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
        mAbortedReason = QString(tr("Error : Variance on Y is null, do computation with Variance G fixed  = 0 for this model "));
        return mAbortedReason;
    }
    // --------------------------- Current spline ----------------------
    try {
        /* --------------------------------------------------------------
         *  Calcul de la spline g, g" pour chaque composante x y z
         *-------------------------------------------------------------- */
        mModel->mSpline = currentSpline(mModel->mEvents, true);

        // init Posterior MeanG and map
        const int nbPoint = 300;// Density curve size and curve size

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapG.min_value = +INFINITY;
        clearCompo.mapG.max_value = 0;

        clearCompo.vecG = std::vector<double> (nbPoint); // column
        clearCompo.vecGP = std::vector<double> (nbPoint);
        clearCompo.vecGS = std::vector<double> (nbPoint);
        clearCompo.vecVarG = std::vector<double> (nbPoint);
        clearCompo.vecVarianceG = std::vector<double> (nbPoint);
        clearCompo.vecVarErrG = std::vector<double> (nbPoint);

        PosteriorMeanG clearMeanG;
        clearMeanG.gx = clearCompo;

        double minY = +INFINITY;
        double maxY = -INFINITY;
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, Event* e) {return std::min(e->mYx, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, Event* e) {return std::max(e->mYx, x);});

        int i = 0;
        for (auto g : mModel->mSpline.splineX.vecG) {
            const auto e = 1.96*sqrt(mModel->mSpline.splineX.vecVarG.at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }

        clearMeanG.gx.mapG.setRangeY(minY, maxY);

        if ( mComputeY) {
            clearMeanG.gy = clearCompo;

            minY = +INFINITY;
            maxY = -INFINITY;

            minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double y, Event* e) {return std::min(e->mYy, y);});
            maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double y, Event* e) {return std::max(e->mYy, y);});


            int i = 0;
            for (auto g : mModel->mSpline.splineY.vecG) {
                const auto e = 1.96*sqrt(mModel->mSpline.splineY.vecVarG.at(i));
                minY = std::min(minY, g - e);
                maxY = std::max(maxY, g + e);
                i++;
            }


            clearMeanG.gy.mapG.setRangeY(minY, maxY);

            if (mComputeZ) {
                clearMeanG.gz = clearCompo;

                minY = +INFINITY;
                maxY = -INFINITY;

                minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double z, Event* e) {return std::min(e->mYz, z);});
                maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double z, Event* e) {return std::max(e->mYz, z);});

                int i = 0;
                for (auto g : mModel->mSpline.splineZ.vecG) {
                    const auto e = 1.96*sqrt(mModel->mSpline.splineZ.vecVarG.at(i));
                    minY = std::min(minY, g - e);
                    maxY = std::max(maxY, g + e);
                    i++;
                }

                clearMeanG.gz.mapG.setRangeY(minY, maxY);
            }

        }

        // Convertion IDF
        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||  mModel->mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical) {

            const double deg = 180. / M_PI ;
            // 1 - new extrenum
            const double gzFmax = sqrt(pow(clearMeanG.gx.mapG.maxY(), 2.) + pow(clearMeanG.gy.mapG.maxY(), 2.) + pow(clearMeanG.gz.mapG.maxY(), 2.));
            const double gxIncMax = asin(clearMeanG.gz.mapG.maxY() / gzFmax) * deg;
            const double gyDecMax = atan2(clearMeanG.gy.mapG.maxY(), clearMeanG.gx.mapG.maxY()) * deg;

            const double gzFmin = sqrt(pow(clearMeanG.gx.mapG.minY(), 2.) + pow(clearMeanG.gy.mapG.minY(), 2.) + pow(clearMeanG.gz.mapG.minY(), 2.));
            const double gxIncMin = asin(clearMeanG.gz.mapG.minY() / gzFmin) * deg;
            const double gyDecMin = atan2(clearMeanG.gy.mapG.minY(), clearMeanG.gx.mapG.minY()) * deg;

            clearMeanG.gx.mapG.setRangeY(gxIncMin, gxIncMax);
            clearMeanG.gy.mapG.setRangeY(gyDecMin, gyDecMax);
            clearMeanG.gz.mapG.setRangeY(gzFmin, gzFmax);

        }


        mModel->mPosteriorMeanGByChain.push_back(clearMeanG);
        if (mChainIndex == 0)
            mModel->mPosteriorMeanG = clearMeanG;

    }  catch (...) {
        qWarning() <<"init Posterior MeanG and map  ???";
    }

    /*
     * INIT UPDATE
     */
    // init the current state
    try {
        initListEvents.resize(mModel->mEvents.size());
        std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    } catch (...) {
        qWarning() <<"init Posterior MeanG and map  ???";
        return QString("[MCMCLoopCurve::initialize_Komlan] problem");
    }


    return QString();
}

/**
 * @brief MCMCLoopCurve::initialize_interpolate pour l'initialisation, il n'y a pas de VG donc équivalent à un modèle avec que des nodes
 * @return
 */

QString MCMCLoopCurve::initialize_interpolate()
{
    updateLoop = &MCMCLoopCurve::update_interpolate;
    QList<Event*>& allEvents (mModel->mEvents);

    initListEvents.resize(mModel->mEvents.size());
    std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent = allEvents;
    mPointEvent.clear();


    // -------------------------------- SPLINE part--------------------------------
    // Init function G

    prepareEventsY(allEvents);

    emit stepChanged(tr("Initializing G ..."), 0, allEvents.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    // ----------------------------------------------------------------
    //  Init Lambda Spline
    // ----------------------------------------------------------------

    //std::vector<t_reduceTime> vecH = calculVecH(mModel->mEvents);
    //SplineMatrices matricesWI = prepareCalculSpline_WI(mModel->mEvents, vecH);
    try {

        mModel->mLambdaSpline.mX = 0.;
        mModel->mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
        mModel->mLambdaSpline.memo();

        mModel->mLambdaSpline.mSigmaMH = 1.;

    }  catch (...) {
        qWarning() << "Init Lambda Spline  ???";
        mAbortedReason = QString("Init Lambda Spline  ???");
        return mAbortedReason;
    }

    // ----------------------------------------------------------------
    // Curve init Vg_i
    // ----------------------------------------------------------------
    try {
        Var_residual_spline = 0.;

        int i = 0;
        // que des Noeuds dans le cas de Lambda == 0
        for (Event* &e : allEvents) {

            e->mVg.mX = 0.;
            e->mVg.mSamplerProposal = MHVariable::eFixe;
            e->mVg.memo();

            e->mVg.mLastAccepts.clear();

            if (mCurveSettings.mUseErrMesure == true) {
                e->updateW();
                // Check if Sy == 0
                if ( e->mSy == 0) {
                    mAbortedReason = QString("Error: in interpolation mode, Event and Node cannot have a zero error \n Change error within : ") + e->mName;
                    return mAbortedReason;
                }
            } else
                e->mW = 1.;

            e->mVg.mSigmaMH = 1.;

            if (isInterruptionRequested())
                return ABORTED_BY_USER;

            emit stepProgressed(++i);
        }



    }  catch (...) {
        mAbortedReason = QString("Curve init Vg  ???");
        return mAbortedReason;
    }

    // ----------------------------------------------------------------
    // Curve init S02 Vg
    // ----------------------------------------------------------------
    mModel->mS02Vg.mX = 0;
    mModel->mS02Vg.mLastAccepts.clear();
    mModel->mS02Vg.mSamplerProposal = MHVariable::eFixe;
    mModel->mS02Vg.memo();

    mModel->mS02Vg.mSigmaMH = 1.;

    if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeUnivarie) {
        std::vector<double> vecY (mModel->mEvents.size());
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
        mAbortedReason = QString(tr("Error : Variance on Y is null, do computation with Variance G fixed  = 0 for this model "));
        return mAbortedReason;
    }
    // --------------------------- Current spline ----------------------
    try {
        /* --------------------------------------------------------------
         *  Calcul de la spline g, g" pour chaque composante x y z
         *-------------------------------------------------------------- */
        mModel->mSpline = currentSpline_WI(mModel->mEvents, true);

        // init Posterior MeanG and map
        const int nbPoint = 300;// map size and curve size

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapG.min_value = +INFINITY;
        clearCompo.mapG.max_value = 0;

        clearCompo.vecG = std::vector<double> (nbPoint); // column
        clearCompo.vecGP = std::vector<double> (nbPoint);
        clearCompo.vecGS = std::vector<double> (nbPoint);
        clearCompo.vecVarG = std::vector<double> (nbPoint);
        clearCompo.vecVarianceG = std::vector<double> (nbPoint);
        clearCompo.vecVarErrG = std::vector<double> (nbPoint);

        PosteriorMeanG clearMeanG;
        clearMeanG.gx = clearCompo;

        double minY = +INFINITY;
        double maxY = -INFINITY;
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, Event* e) {return std::min(e->mYx, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, Event* e) {return std::max(e->mYx, x);});


        double maxVarY = *std::max_element(mModel->mSpline.splineX.vecVarG.begin(), mModel->mSpline.splineX.vecVarG.end());
        double spanY_X =  0;
        minY = minY - 1.96*sqrt(maxVarY) - spanY_X;
        maxY = maxY + 1.96*sqrt(maxVarY) + spanY_X;

        clearMeanG.gx.mapG.setRangeY(minY, maxY);

        if ( mComputeY) {
            clearMeanG.gy = clearCompo;

            minY = +INFINITY;
            maxY = -INFINITY;

            minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, Event* e) {return std::min(e->mYy, x);});
            maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, Event* e) {return std::max(e->mYy, x);});

            maxVarY = *std::max_element(mModel->mSpline.splineY.vecVarG.begin(), mModel->mSpline.splineY.vecVarG.end());
            spanY_X = 0;
            minY = minY - 1.96*sqrt(maxVarY) - spanY_X;
            maxY = maxY + 1.96*sqrt(maxVarY) + spanY_X;

            clearMeanG.gy.mapG.setRangeY(minY, maxY);

            if (mComputeZ) {
                clearMeanG.gz = clearCompo;

                minY = +INFINITY;
                maxY = -INFINITY;

                maxVarY = *std::max_element(mModel->mSpline.splineZ.vecVarG.begin(), mModel->mSpline.splineZ.vecVarG.end());

                minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, Event* e) {return std::min(e->mYz, x);});
                maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, Event* e) {return std::max(e->mYz, x);});

                spanY_X = 0;
                minY = minY - 1.96*sqrt(maxVarY) - spanY_X;
                maxY = maxY + 1.96*sqrt(maxVarY) + spanY_X;

                clearMeanG.gz.mapG.setRangeY(minY, maxY);
            }

        }

        // Convertion IDF
        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||  mModel->mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical) {

            const double deg = 180. / M_PI ;
            // 1 - new extrenum
            const double gzFmax = sqrt(pow(clearMeanG.gx.mapG.maxY(), 2.) + pow(clearMeanG.gy.mapG.maxY(), 2.) + pow(clearMeanG.gz.mapG.maxY(), 2.));
            const double gxIncMax = asin(clearMeanG.gz.mapG.maxY() / gzFmax) * deg;
            const double gyDecMax = atan2(clearMeanG.gy.mapG.maxY(), clearMeanG.gx.mapG.maxY()) * deg;

            const double gzFmin = sqrt(pow(clearMeanG.gx.mapG.minY(), 2.) + pow(clearMeanG.gy.mapG.minY(), 2.) + pow(clearMeanG.gz.mapG.minY(), 2.));
            const double gxIncMin = asin(clearMeanG.gz.mapG.minY() / gzFmin) * deg;
            const double gyDecMin = atan2(clearMeanG.gy.mapG.minY(), clearMeanG.gx.mapG.minY()) * deg;

            clearMeanG.gx.mapG.setRangeY(gxIncMin, gxIncMax);
            clearMeanG.gy.mapG.setRangeY(gyDecMin, gyDecMax);
            clearMeanG.gz.mapG.setRangeY(gzFmin, gzFmax);

        }


        mModel->mPosteriorMeanGByChain.push_back(clearMeanG);
        if (mChainIndex == 0)
            mModel->mPosteriorMeanG = clearMeanG;//std::move(clearMeanG);

    }  catch (...) {
        qWarning() <<"init Posterior MeanG and map  ???";
    }
    return QString();
}


bool MCMCLoopCurve::update()
{
    return   (this->*updateLoop)();

}


bool MCMCLoopCurve::update_321()
{
    try {

        t_prob rate;


        // find minimal step;
        // long double minStep = minimalThetaReducedDifference(mModel->mEvents)/10.;

        // init the current state
        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);

        current_vecH = calculVecH(mModel->mEvents);

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        current_decomp_QTQ = decompositionCholesky(current_splineMatrices.matQTQ, 5, 1); // used only with update Theta
        //auto current_decomp_2 = choleskyLDLT_Dsup0(current_splineMatrices.matQTQ, 5, 1);


        current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);

        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH
        current_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. :
                                                             ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mComputeY, mComputeZ);

        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);

        if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
            current_h_lambda = 1;
        else
            current_h_lambda = h_lambda(current_splineMatrices,  mModel->mEvents.size(), mModel->mLambdaSpline.mX) ;


        /* --------------------------------------------------------------
         *  A - Update ti Dates
         *  B - Update Theta Events
         *  C.1 - Update Alpha, Beta & Duration Phases
         *  C.2 - Update Tau Phase
         *  C.3 - Update Gamma Phases
         *
         *  Dans MCMCLoopChrono, on appelle simplement : event->updateTheta(t_min,t_max); sur tous les events.
         *  Pour mettre à jour un theta d'event dans Curve, on doit accéder aux thetas des autres events.
         *  => On effectue donc la mise à jour directement ici, sans passer par une fonction
         *  de la classe event (qui n'a pas accès aux autres events)
         * ---------------------------------------------------------------------- */
        if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
            /* --------------------------------------------------------------
             *  A - Update ti Dates
             * -------------------------------------------------------------- */
            try {
                if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
                    for (auto&& event : mModel->mEvents) {
                        for (auto&& date : event->mDates )   {
                            date.updateDate(event);
                        }
                    }
                }

            } catch(std::exception& exc) {
                qWarning() << "[MCMCLoopCurve::update] Ti : Caught Exception!\n"<<exc.what();

            }  catch (...) {
                qWarning() << "[MCMCLoopCurve::update] Ti : Caught Exception!";
            }


            /* --------------------------------------------------------------
             *  B - Update Theta Events
             * -------------------------------------------------------------- */
            try {
                for (Event*& event : initListEvents) {
#ifdef _WIN32
    SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
#endif
                    if (event->mType == Event::eDefault) {
                        const double min = event->getThetaMin(tminPeriod);
                        const double max = event->getThetaMax(tmaxPeriod);

                        if (min >= max) {
                            throw QObject::tr("Error for event theta : %1 : min = %2 : max = %3").arg(event->mName, QString::number(min), QString::number(max));
                        }

                        // On stocke l'ancienne valeur
                        const double current_value = event->mTheta.mX;
                        current_h_theta = h_theta_Event(event);

                        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH

                        // On tire une nouvelle valeur :
                        const double try_value = Generator::gaussByBoxMuller(current_value, event->mTheta.mSigmaMH);

                        if (try_value >= min && try_value <= max) {
                            // On force la mise à jour de la nouvelle valeur pour calculer h_new

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);

                            orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                            spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date

                            try_vecH = calculVecH(mModel->mEvents);

                            try_splineMatrices = prepareCalculSpline(mModel->mEvents, try_vecH);

                            try_decomp_QTQ = decompositionCholesky(try_splineMatrices.matQTQ, 5, 1);
                            try_decomp_matB = decomp_matB(try_splineMatrices, mModel->mLambdaSpline.mX);

                            try_ln_h_YWI_1_2 = ln_h_YWI_1_2(try_decomp_QTQ, try_decomp_matB);
                            try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, try_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mComputeY, mComputeZ);

                            if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
                                try_h_lambda = 1;
                            else
                                try_h_lambda = h_lambda( try_splineMatrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

                            try_h_theta = h_theta_Event(event);

                            rate = (try_h_lambda* try_h_theta) / (current_h_lambda* current_h_theta) * exp(0.5 * ( try_ln_h_YWI_1_2 + try_ln_h_YWI_3
                                                                                                                     - current_ln_h_YWI_1_2 - current_ln_h_YWI_3));


                        } else {
                            rate = -1.;

                        }

                        // restore Theta to used function tryUpdate
                        event->mTheta.mX = current_value;
                        event->mTheta.tryUpdate(try_value, rate);


                        if ( event->mTheta.mLastAccepts.last() == true) {
                            // Pour l'itération suivante :
                            std::swap(current_ln_h_YWI_1_2, try_ln_h_YWI_1_2);
                            std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);

                            std::swap(current_vecH, try_vecH);
                            std::swap(current_splineMatrices, try_splineMatrices);
                            std::swap(current_h_lambda, try_h_lambda);
                            std::swap(current_decomp_matB, try_decomp_matB);
                            std::swap(current_decomp_QTQ, try_decomp_QTQ);

                        }

                    } else { // this is a bound, nothing to sample. Always the same value
                       //  event->updateTheta(tminPeriod, tmaxPeriod); //useless if fixed
                    }

                    // update after tryUpdate or updateTheta
                    event->mThetaReduced = mModel->reduceTime(event->mTheta.mX);

                    /* --------------------------------------------------------------
                     * C.1 - Update Alpha, Beta & Duration Phases
                     * -------------------------------------------------------------- */
                    //  Update Phases -set mAlpha and mBeta ; they coud be used by the Event in the other Phase ----------------------------------------
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (Phase* p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});

                } // End of loop initListEvents

                //  Update Phases Tau; they coud be used by the Event in the other Phase ----------------------------------------
                /* --------------------------------------------------------------
                 *  C.2 - Update Tau Phases
                 * -------------------------------------------------------------- */
                std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (Phase* p) {p->update_Tau (tminPeriod, tmaxPeriod);});
                
                /* --------------------------------------------------------------
                 *  C.3 - Update Gamma Phases
                 * -------------------------------------------------------------- */
                std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (PhaseConstraint* pc) {pc->updateGamma();});

            } catch(std::exception& exc) {
                qWarning() << "[MCMCLoopCurve::update] Theta : Caught Exception!\n"<<exc.what();
            }

        } else { // Pas bayésien : Tous les temps sont fixes

        }


        /* --------------------------------------------------------------
         *  Remarque : à ce stade, tous les theta events sont à jour et ordonnés.
         *  On va à présent, mettre à jour tous les Vg, puis Lambda Spline.
         *  Pour cela, nous devons espacer les thetas pour permettre les calculs.
         *  Nous le faisons donc ici, et restaurerons les vrais thetas à la fin.
         * -------------------------------------------------------------- */


        /* --------------------------------------------------------------
         *  D - Update S02 Vg
         *  E.1 - Update Vg for Points only
         *  E.2 - Update Vg Global
         * -------------------------------------------------------------- */
        try {
            if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

                current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB); // Has not been initialized yet

                const double logMin = -10.;
                const double logMax = +20.;
                /* --------------------------------------------------------------
                 *  D - Update S02 Vg, à évaluer dans les deux cas: variance individuelle et globale
                 * -------------------------------------------------------------- */
                try {
                    const double logMin = -10.;
                    const double logMax = +20.;
                    const double try_value_log = Generator::gaussByBoxMuller(log10(mModel->mS02Vg.mX), mModel->mS02Vg.mSigmaMH);
                    const double try_value = pow(10, try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {

                      const t_prob jacobianRate = (t_prob)try_value / mModel->mS02Vg.mX;
                      rate = rate_h_S02_Vg(mPointEvent, mModel->mS02Vg.mX, try_value) * jacobianRate;

                    } else {
                        rate = -1.;
                    }

                    mModel->mS02Vg.tryUpdate(try_value, rate);


                } catch (std::exception& e) {
                    qWarning()<< "[MCMCLoopCurve::update] S02 Vg : exception caught: " << e.what() << '\n';

                }

                if (mCurveSettings.mUseVarianceIndividual) {
                    /* --------------------------------------------------------------
                     *  E.1 - Update Vg for Points only, not the node
                     * -------------------------------------------------------------- */

                    for (Event*& event : mPointEvent)   {

                        const double current_value = event->mVg.mX;
                        current_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                        // On tire une nouvelle valeur :
                        const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), event->mVg.mSigmaMH);
                        const double try_value = pow(10., try_value_log);

                        if (try_value_log >= logMin && try_value_log <= logMax) {
                            // On force la mise à jour de la nouvelle valeur pour calculer try_h
                            // A chaque fois qu'on modifie VG, W change !
                            event->mVg.mX = try_value;
                            event->updateW(); // used by prepareCalculSpline

                            // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG

                            try_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
                            try_decomp_matB = decomp_matB(try_splineMatrices, mModel->mLambdaSpline.mX);

                            //try_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. :
                            try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mComputeY, mComputeZ);
                            try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);
                            try_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                            rate = (try_h_VG * try_value) / (current_h_VG * current_value) * exp(0.5 * ( try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                            - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                        } else {
                            rate = -1.; // force reject // force to keep current state

                        }

                        // Mise à jour Metropolis Hastings
                        // A chaque fois qu'on modifie VG, W change !
                        event->mVg.mX = current_value;
                        event->mVg.tryUpdate( try_value, rate);
                        event->updateW();

                       if ( event->mVg.mLastAccepts.last() == true) {
                            // Pour l'itération suivante : Car mVg a changé
                            std::swap(current_ln_h_YWI_2, try_ln_h_YWI_2);
                            std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);
                            std::swap(current_splineMatrices, try_splineMatrices);
                            std::swap(current_decomp_matB, try_decomp_matB);
                        }
                    }


                } else {
                    /* --------------------------------------------------------------
                     *  E.2 - Update Vg Global
                     * -------------------------------------------------------------- */

                    auto& eventVGglobal = mModel->mEvents.at(0);

                    // On stocke l'ancienne valeur :
                    const double current_value = eventVGglobal->mVg.mX;

                    current_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                    // On tire une nouvelle valeur :

                    const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mEvents.at(0)->mVg.mSigmaMH);
                    const double try_value = pow(10, try_value_log);

                    // Affectation temporaire pour évaluer la nouvelle proba
                    // Dans le cas global pas de différence ente les Points et les Nodes
                    for (Event*& ev : mModel->mEvents) {
                        ev->mVg.mX = try_value;
                        ev->updateW();
                    }

                    //rate = 0.;
                    if (try_value_log >= logMin && try_value_log <= logMax) {

                        // Calcul du rapport : try_matrices utilise les temps reduits, elle est affectée par le changement de Vg
                        try_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);

                        try_decomp_matB = decomp_matB(try_splineMatrices, mModel->mLambdaSpline.mX);

                        try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);
                        try_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. : ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mComputeY, mComputeZ);

                        try_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                        rate = (try_h_VG * try_value) / (current_h_VG * current_value) * exp(0.5 * ( try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                        - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                        // ON fait le test avec le premier event

                        eventVGglobal->mVg.mX = current_value;
                        eventVGglobal->mVg.tryUpdate(try_value, rate);
                        eventVGglobal->updateW();

                        if ( eventVGglobal->mVg.mLastAccepts.last() == true) {
                            current_ln_h_YWI_2 = std::move(try_ln_h_YWI_2);
                            current_ln_h_YWI_3 = std::move(try_ln_h_YWI_3);
                            current_splineMatrices = std::move(try_splineMatrices);
                            current_decomp_matB = std::move(try_decomp_matB);
                            rate = 2.;

                        } else {
                            rate = -1.;
                        }

                    } else {
                        rate = -1.;
                    }

                    for (Event*& ev : mModel->mEvents) {
                        ev->mVg.mX =  eventVGglobal->mVg.mX;
                        ev->mVg.tryUpdate(eventVGglobal->mVg.mX, rate);
                        // On remet l'ancienne valeur, qui sera éventuellement mise à jour dans ce qui suit (Metropolis Hastings)
                        // A chaque fois qu'on modifie VG, W change !
                        ev->updateW();
                    }

                }

                // Not bayesian
            } else { // nothing to do : mCurveSettings.mVarianceType == CurveSettings::eFixed

            }

            } catch (std::exception& e) {
                qWarning()<< "[MCMCLoopCurve::update] VG : exception caught: " << e.what() << '\n';

            } catch(...) {
                qWarning() << "[MCMCLoopCurve::update] VG Event Caught Exception!\n";

            }

        /* --------------------------------------------------------------
         * F - Update Lambda
         * -------------------------------------------------------------- */
        try {
            if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {

                const double logMin = -20.;
                const double logMax = +10.;

                // On stocke l'ancienne valeur :
                const double current_value = mModel->mLambdaSpline.mX;

                // On tire une nouvelle valeur :
                const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                const double try_value = pow(10., try_value_log);

                if (try_value_log >= logMin && try_value_log <= logMax) {
                    //current_h_lambda n'a pas changer ,depuis update theta
                    //   current_h_lambda = h_lambda(current_splineMatrices, mModel->mEvents.size(), current_value) ;

                    // Calcul du rapport :
                    mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                    try_h_lambda = h_lambda(current_splineMatrices, mModel->mEvents.size(), try_value) ;
                    try_decomp_matB = decomp_matB(current_splineMatrices, try_value);
                    try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mComputeY, mComputeZ);
                    try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);

                    const auto n = mModel->mEvents.size();

                    rate = (try_h_lambda * try_value) / (current_h_lambda * current_value)  * exp( 0.5 *  ( (n-2)*log(try_value/current_value)
                                                                                                               + try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                               - current_ln_h_YWI_2 - current_ln_h_YWI_3));
                } else {
                    rate = -1.; // force reject
                }

                mModel->mLambdaSpline.mX = current_value;
                mModel->mLambdaSpline.tryUpdate(try_value, rate);

            // Pas bayésien : on sauvegarde la valeur constante dans la trace
            } else { // Rien à faire

               // mModel->mLambdaSpline.tryUpdate(mModel->mLambdaSpline.mX, 2.);
            }

        } catch(...) {
            qDebug() << "[MCMCLoopCurve::update Lambda] Caught Exception!\n";
        }


        /* --------------------------------------------------------------
         *  G - Update mModel->mSpline
         * -------------------------------------------------------------- */
        // G.1- Calcul spline
        mModel->mSpline = currentSpline(mModel->mEvents, false, current_vecH, current_splineMatrices);

        // G.2 - test GPrime positive
        if (mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth ) {
            return hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

        } else
            return true;


    } catch (const char* e) {
        qWarning() << "[MCMCLoopCurve::update] char "<< e;

    } catch (const std::length_error& e) {
        qWarning() << "[MCMCLoopCurve::update] length_error"<< e.what();

    } catch (const std::out_of_range& e) {
        qWarning() << "[MCMCLoopCurve::update] out of range" <<e.what();

    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve::update]  "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update] Caught Exception!\n";
        return false;
    }

    return false;
}


bool MCMCLoopCurve::update_Komlan()
{
#define CODE_KOMLAN
    try {
        //const double tminPeriod = mModel->mSettings.mTmin;
        //const double tmaxPeriod = mModel->mSettings.mTmax;

        // --------------------------------------------------------------
        //  A - Update ti Dates (idem chronomodel)
        // --------------------------------------------------------------
        try {
            for (Event*& event : mModel->mEvents) {
                for (auto&& date : event->mDates) {
                    date.updateDelta(event);
                    date.updateTi(event);
                    //date.updateSigma(event);
                    //date.updateSigmaJeffreys(event);
                    date.updateSigmaShrinkage(event);
#ifdef DEBUG
                    if (date.mSigmaTi.mX <= 0)
                        qDebug(" date.mSigma.mX <= 0 ");
#endif \
    //date.updateSigmaReParam(event);
                    date.updateWiggle();

                }
            }

        }  catch (...) {
            qWarning() <<"update Date ???";
        }
        // Variable du MH de la spline
#ifdef CODE_KOMLAN
        // Variable du MH de la spline

        double current_value, current_h, current_h_lambda, current_h_YWI, current_h_VG;

        SplineMatrices current_matrices, current_matriceWI;
        SplineResults current_spline;
        std::vector<double> current_vecH;

        double try_value, try_h, try_h_YWI, try_h_lambda, try_h_VG ;
        SplineMatrices try_matrices;
        std::vector<double> try_vecH;

        long double rapport;

#else \
    // Variable du MH de la spline
        double current_value, current_h, current_h_theta, current_h_YWI, current_h_lambda, current_h_VG;

        SplineMatrices current_matrices, current_matriceWI;
        SplineResults current_spline;
        std::vector<double> current_vecH;

        double try_value, try_h, try_h_theta, try_h_YWI, try_h_lambda, try_h_VG ;
        SplineMatrices try_matrices;
        std::vector<double> try_vecH;

        long double rapport;

#endif


        // --------------------------------------------------------------
        //  B - Update theta Events
        // --------------------------------------------------------------
        // copie la liste des pointeurs
        std::vector<Event*> initListEvents (mModel->mEvents.size());
        std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);

        current_vecH = calculVecH(mModel->mEvents);

        current_matrices = prepareCalculSpline(mModel->mEvents, current_vecH) ;

#ifdef CODE_KOMLAN


        const Matrix2D &R = seedMatrix(current_matrices.matR, 1); // dim n-2 * n-2
        const int np = R.size();
        const int mm = 3*np ;

        const Matrix2D &Q = remove_bands_Matrix(current_matrices.matQ, 1); // dim n * n-2
        const Matrix2D &QT = transpose0(Q);


        Matrix2D  matRInv ;
        if (np <= 5) {
            matRInv = inverseMatSym0(R, 0);
        } else {
            const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(R, 3, 0);

            matRInv = inverseMatSym_origin(decomp, mm, 0);

        }

        auto K = multiMatParMat0(multiMatParMat0(Q, matRInv), QT);

        //  auto K =  calcul_QR_1Qt(current_matrices) ; // = K
        auto current_decompK = decompositionCholesky(K, K.size(), 0) ;

#endif

        try {

// find minimal step;
// long double minStep = minimalThetaReducedDifference(mModel->mEvents)/10.;

// init the current state

#ifndef CODE_KOMLAN

            current_h_YWI = h_YWI_AY(current_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH); // utile si mCurveSettings.mTimeType == CurveSettings::eModeFixe
#endif

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
                unsigned e_idx = 0;
                for (Event*& event : initListEvents) {
                    if (event->mType == Event::eDefault) {

                        // ----
                        const double min = event->getThetaMin(tminPeriod);
                        const double max = event->getThetaMax(tmaxPeriod);

                        if (min >= max) {
                            throw QObject::tr("Error for event theta : %1 : min = %2 : max = %3").arg(event->mName, QString::number(min), QString::number(max));
                        }

                        // On stocke l'ancienne valeur :
                        current_value = event->mTheta.mX;

#ifdef CODE_KOMLAN \
    //  auto K =  calcul_QR_1Qt(current_matrices) ; // = K
                        //    auto current_decompK = decompositionCholesky(K, K.size(), 0) ;

                        std::vector<long double> current_detPlus (current_decompK.second);
                        std::sort(current_detPlus.begin(), current_detPlus.end(), [] (double i,double j) { return (i>j);});
                        current_detPlus.resize(current_detPlus.size() - 2);

                        //  current_h_lambda = h_lambda_Komlan(K, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

                        //current_h_YWI = h_exp_fX_theta(event, mModel->mSpline, e_idx) ;

                        // const double current_fTKf = Prior_F(K, mModel->mSpline, mModel->mLambdaSpline.mX);

                        //  current_h = current_h_lambda * current_fTKf;

#else \
    //current_h_YWI = h_exp_fX_theta(event, mModel->mSpline, e_idx) ;

                        current_h_theta = h_theta_Event(event);

                        current_h_lambda = h_lambda(current_matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

                        current_h = current_h_YWI * current_h_lambda * current_h_theta;

#endif

// On tire une nouvelle valeur :
#ifdef CODE_KOMLAN
                        double sum_p = 0.;
                        double sum_t = 0.;

                        for (auto&& date: event->mDates) {
                            const double variance  = pow(date.mSigmaTi.mX, 2.);
                            sum_t += (date.mTi.mX + date.mDelta) / variance;
                            sum_p += 1. / variance;
                        }
                        const double theta_avg = sum_t / sum_p;
                        const double sigma = 1. / sqrt(sum_p);

                        try_value = Generator::gaussByDoubleExp(theta_avg, sigma, min, max);


                        /*     double theta;
                        long long counter = 0.;
                        do {
                            theta = Generator::gaussByBoxMuller(theta_avg, sigma);
                            ++counter;
                           // if (counter == 100000000)
                          //      throw QObject::tr("No MCMC solution could be found using event method %1 for event named %2 ( %3  trials done)").arg(MHVariable::getSamplerProposalText(mTheta.mSamplerProposal), mName, QString::number(counter));

                        } while ((theta < min || theta > max) && counter<10000);

                        if (counter < 10000)
                            try_value = theta;
                        else
                            try_value = current_value; */
#else

                        try_value = Generator::gaussByBoxMuller(current_value, event->mTheta.mSigmaMH); // Problème au niveau de l'échantillonneur
#endif

                        if (try_value >= min && try_value <= max) {
                            // On force la mise à jour de la nouvelle valeur pour calculer h_new

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);

                            orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                            spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date

                            try_vecH = calculVecH(mModel->mEvents);
                            try_matrices = prepareCalculSpline(mModel->mEvents, try_vecH);
#ifdef CODE_KOMLAN

                            const Matrix2D &try_Q = remove_bands_Matrix(try_matrices.matQ, 1); // dim n * n-2

                            const Matrix2D &try_QT = transpose0(try_Q);
                            const Matrix2D &try_R = seedMatrix(try_matrices.matR, 1); // dim n-2 * n-2

                            //  auto try_K =  calcul_QR_1Qt(try_Q, try_R_1) ; //

                            const int try_np = try_R.size();
                            Matrix2D  try_matRInv ;
                            if (try_np <= 5) {

                                try_matRInv = inverseMatSym0(try_R, 0);

                            }else {

                                const std::pair<Matrix2D, MatrixDiag> &try_decomp = decompositionCholesky(try_R, 3, 0);

                                try_matRInv = inverseMatSym_origin(try_decomp, mm, 0);

                            }

                            auto try_K = multiMatParMat0(multiMatParMat0(try_Q, try_matRInv), try_QT);

                            auto try_decompK = decompositionCholesky(try_K, try_K.size() , 0);

                            std::vector<long double> try_detPlus (try_decompK.second);
                            std::sort(try_detPlus.begin(), try_detPlus.end(), [] (double i,double j) { return (i>j);});
                            try_detPlus.resize(try_detPlus.size() - 2);


                            double rapport_detPlus = 1.;
                            for (size_t i = 0 ; i<try_detPlus.size(); i++) {
                                rapport_detPlus *= sqrt(exp(log(try_detPlus[i])  - log(current_detPlus[i])));
                            }

                            try_h_lambda = h_lambda_Komlan(K, try_K, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

#else

                            try_h_theta = h_theta_Event(event);

                            try_h_lambda = h_lambda(try_matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

//try_h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, try_vecH);

// try_h_lambda = h_lambda(try_matrices, mModel->mEvents.size(), mModel->mLambdaSpline.mX);

#endif


// Calcul du rapport :

#ifdef CODE_KOMLAN

                            // try_h_theta = h_theta_Event(event);

                            const double try_fTKf = Prior_F(K, try_K, mModel->mSpline,  mModel->mLambdaSpline.mX);

                            // try_h =  try_h_lambda* try_h_theta*try_fTKf;

                            rapport =   rapport_detPlus * try_fTKf * try_h_lambda;
#else
                            try_h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, try_vecH);


                            try_h = try_h_YWI * try_h_lambda* try_h_theta;

                            rapport = (try_h / current_h);
#endif
                        } else {
                            rapport = -1.;

                        }

                        // restore Theta to used function tryUpdate
                        event->mTheta.mX = current_value;
                        event->mTheta.tryUpdate(try_value, rapport);

                        if ( event->mTheta.mLastAccepts.last() == true) {
                            // Pour l'itération suivante :
#ifndef CODE_KOMLAN
                            current_h_YWI = std::move(try_h_YWI);
                            current_vecH = std::move(try_vecH);
                            current_h_lambda = std::move(try_h_lambda);
#endif
                            current_matrices = std::move(try_matrices);


                        }


                    } else { // this is a bound, nothing to sample. Always the same value
                        event->updateTheta(tminPeriod, tmaxPeriod);
                    }

                    // update after tryUpdate or updateTheta
                    event->mThetaReduced = mModel->reduceTime(event->mTheta.mX);



                    /* --------------------------------------------------------------
                     * C.1 - Update Alpha, Beta & Duration Phases
                     * -------------------------------------------------------------- */
                    //  Update Phases -set mAlpha and mBeta ; they coud be used by the Event in the other Phase ----------------------------------------
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (Phase* p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});


                    e_idx++;
                } // End of loop initListEvents


                // Rétablissement de l'ordre initial. Est-ce nécessaire ?
                //   std::copy(initListEvents.begin(), initListEvents.end(), mModel->mEvents.begin() );



            } else { // Pas bayésien : on sauvegarde la valeur constante dans la trace
                for (Event*& event : initListEvents) {
                    event->mTheta.tryUpdate(event->mTheta.mX, 1.);

                    /* --------------------------------------------------------------
                     * C.1 - Update Alpha, Beta & Duration Phases
                     * -------------------------------------------------------------- */
                    //  Update Phases -set mAlpha and mBeta ; they coud be used by the Event in the other Phase ----------------------------------------
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (Phase* p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});
                }



            }

            //  Update Phases Tau; they coud be used by the Event in the other Phase ----------------------------------------
            /* --------------------------------------------------------------
                 *  C.2 - Update Tau Phases
                 * -------------------------------------------------------------- */
            std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (Phase* p) {p->update_Tau (tminPeriod, tmaxPeriod);});

            /* --------------------------------------------------------------
                 *  C.3 - Update Gamma Phases
                 * -------------------------------------------------------------- */
            std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (PhaseConstraint* pc) {pc->updateGamma();});


        } catch(...) {
            qDebug() << "MCMCLoopCurve::update Theta : Caught Exception!\n";
        }

        // --------------------------------------------------------------
        //  B_2 - Update S02
        // --------------------------------------------------------------

        try {
            for (Event*& event : initListEvents) {
                event->updateS02();

            }
        }  catch (...) {
            qDebug() << "MCMCLoopCurve::update S02 : Caught Exception!\n";

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
                        current_value = mModel->mS02Vg.mX;

                        //const double current_h = h_S02_Vg(mModel->mEvents, mModel->mS02Vg.mX, var_Y);

                        // On tire une nouvelle valeur :

                        const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mS02Vg.mSigmaMH);
                        try_value = pow(10, try_value_log);

                        //long double rapport = -1.;

                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            const double  rapport1 = h_S02_Vg_K(mModel->mEvents, current_value, try_value);

                            rapport = rapport1 *  (try_value / current_value) ;
                        } else {
                            rapport = -1.;
                        }

                        mModel->mS02Vg.mX = current_value;
                        mModel->mS02Vg.tryUpdate(try_value, rapport);

                    } catch (std::exception& e) {
                        qWarning()<< "MCMCLoopCurve::update S02 Vg : exception caught: " << e.what() << '\n';

                    }

                    // Fin maj SO2 Vg


                    if (mCurveSettings.mUseVarianceIndividual) {

                        // Variance individuelle

                        unsigned e_idx = 0;
                        for (Event*& event : initListEvents)   {
                            current_value = event->mVg.mX;
                            current_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);
#ifdef CODE_KOMLAN

                            const double detWi = event->mW;

                            current_h_YWI = sqrt(detWi) * h_exp_fX_theta(event, mModel->mSpline, e_idx);

#else
                            current_h_YWI = h_YWI_AY(current_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH);

#endif

                            current_h = current_h_YWI * current_h_VG;

                            // On tire une nouvelle valeur :
                            const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), event->mVg.mSigmaMH);
                            try_value = pow(10., try_value_log);

                            if (try_value_log >= logMin && try_value_log <= logMax) {
                                // On force la mise à jour de la nouvelle valeur pour calculer try_h
                                // A chaque fois qu'on modifie VG, W change !
                                event->mVg.mX = try_value;
                                event->updateW(); // used by prepareCalculSpline

                                // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG
                                try_matrices = prepareCalculSpline(mModel->mEvents, current_vecH);

#ifdef CODE_KOMLAN
                                const double try_detWi = event->mW;
                                try_h_YWI = sqrt(try_detWi) * h_exp_fX_theta(event, mModel->mSpline, e_idx);
#else \
    //try_matrices = prepareCalculSpline(mModel->mEvents, current_vecH);

                                try_h_YWI = h_YWI_AY(try_matrices, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH);
#endif

                                try_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                                if (try_h_YWI == HUGE_VAL || try_h_VG == HUGE_VAL)
                                    try_h = 0.;
                                else

                                try_h = try_h_YWI * try_h_VG;

                                rapport = (try_h * try_value) / (current_h * current_value);

                            } else {
                                rapport = -1.; // force reject // force to keep current state
                                    // try_h_YWI = current_h_YWI;
                            }

                            // Mise à jour Metropolis Hastings
                            // A chaque fois qu'on modifie VG, W change !
                            event->mVg.mX = current_value;
                            event->mVg.tryUpdate( try_value, rapport);
                            event->updateW();

                            if ( event->mVg.mLastAccepts.last() == true) {
                                // Pour l'itération suivante : Car mVG a changé

                                current_h_YWI = std::move(try_h_YWI);
                                current_matrices = std::move(try_matrices);
                            }
                            e_idx++;
                        }

                    }
                    else {

                        /* Si nous sommes en variance global,
                             * il faut trouver l'indice du premier Event qui ne soit pas bound
                             * L'ordre et donc l'indice change avec le spread
                             */
                        auto& eventVGglobal = mModel->mEvents.at(0);

                        // On stocke l'ancienne valeur :
                        current_value = eventVGglobal->mVg.mX;

                        current_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                        current_h = current_h_YWI * current_h_VG;

                        // On tire une nouvelle valeur :

                        const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mEvents.at(0)->mVg.mSigmaMH);
                        try_value = pow(10, try_value_log);

                        // affectation temporaire pour evaluer la nouvelle proba
                        for (Event*& ev : initListEvents) {
                            ev->mVg.mX = try_value;
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

                            //rapport = (current_h == 0) ? 1 : ((try_h * try_value) / (current_h * current_value));

                            rapport =  ((try_h * try_value) / (current_h * current_value));

                            // ON fait le test avec le premier event

                            eventVGglobal->mVg.mX = current_value;
                            eventVGglobal->mVg.tryUpdate(try_value, rapport);
                            eventVGglobal->updateW();


                            if ( eventVGglobal->mVg.mLastAccepts.last() == true) {
                                current_h_YWI = try_h_YWI;
                                current_matrices = std::move(try_matrices);
                                rapport = 2.;

                            } else {
                                rapport = -1.;
                            }

                        } else {
                            rapport = -1.;
                        }

                        for (Event*& ev : initListEvents) {
                            ev->mVg.mX =  eventVGglobal->mVg.mX;
                            try_value = eventVGglobal->mVg.mX;
                            ev->mVg.tryUpdate(try_value, rapport);
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
                for (Event*& event : initListEvents) {
                    event->mVg.tryUpdate(mCurveSettings.mVarianceFixed, 1);
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

                const double logMin = -10.;
                const double logMax = +10.;

                // On stocke l'ancienne valeur :
                current_value = mModel->mLambdaSpline.mX;
#ifdef CODE_KOMLAN
                // auto K = calcul_QR_1Qt(current_matrices);
                current_h = 1. ;  //h_lambda_K(mModel->mSpline, current_value, K);

#else
                current_h_YWI = h_YWI_AY(current_matrices, mModel->mEvents, current_value, current_vecH);

                current_h_lambda = h_lambda(current_matrices, mModel->mEvents.size(), current_value) ;

                current_h = current_h_YWI * current_h_lambda;
#endif \
    // On tire une nouvelle valeur :
                const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                try_value = pow(10., try_value_log);

                if (try_value_log >= logMin && try_value_log <= logMax) {
                    // Calcul du rapport :
                    mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_VG
#ifdef CODE_KOMLAN

// MCMCSpline try_spline = currentSpline(mModel->mEvents, false, current_vecH, current_matrices);

//  try_h = rate_h_lambda_K(try_spline, current_value, try_value, K);

#else
                    try_h_YWI = h_YWI_AY(current_matrices, mModel->mEvents, try_value, current_vecH);

                    try_h_lambda = h_lambda(current_matrices, mModel->mEvents.size(), try_value) ;

                    try_h = try_h_YWI * try_h_lambda;

#endif

#ifdef CODE_KOMLAN
                    rapport =   rate_h_lambda_K(mModel->mSpline, current_value, try_value, K) ;
#else
                    rapport =  (try_h * try_value ) / (current_h * current_value);
#endif

                } else {
                    rapport = -1.; // force reject
                }

                mModel->mLambdaSpline.mX = current_value;
                mModel->mLambdaSpline.tryUpdate(try_value, rapport);

                /* if (mModel->mLambdaSpline.mX < pow(10,-7.59) )
                    qDebug()<<"ici";*/

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
#ifdef CODE_KOMLAN
        // F.1- Simulation spline avec mModel->mLambdaSpline.mX en interne

        std::vector <double> dd = mModel->mSpline.splineX.vecG ;

        std::vector<double> mat_splines = sampling_spline(mModel->mEvents, current_matrices) ;

        auto rapport_spline =  splines_prior(K, dd, mat_splines);  //try_sprior /  current_sprior ;

        std::vector <double> try_spl ;
        for (int i = 0; i < (np + 2); i++) {
            const double u = Generator::randomUniform();
            if (rapport_spline[i] >= u) {
                try_spl.push_back(mat_splines[i]);
            } else {
                try_spl.push_back(dd[i]);
            }
        }

        mModel->mSpline = samplingSpline_multi(mModel->mEvents, matRInv, Q, try_spl,  false, current_matrices);

// mModel->mSpline = samplingSpline(mModel->mEvents, matRInv, QT, K,false, current_matrices) ;

#else \
    // F.1- Calcul spline avec mModel->mLambdaSpline.mX en interne
        mModel->mSpline = currentSpline(mModel->mEvents, false, current_vecH, current_matrices);
#endif

        // F.2 - test GPrime positive
        if (mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth)
            return hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy >mCurveSettings.mThreshold = pas d'acceptation

        else
            return true;


    } catch (const char* e) {
        qWarning() << "MCMCLoopCurve::update_Komlan () char "<< e;


    } catch (const std::length_error& e) {
        qWarning() << "MCMCLoopCurve::update_Komlan () length_error"<< e.what();
    } catch (const std::out_of_range& e) {
        qWarning() << "MCMCLoopCurve::update_Komlan () out_of_range" <<e.what();
    } catch (const std::exception& e) {
        qWarning() << "MCMCLoopCurve::update_Komlan () "<< e.what();

    } catch(...) {
        qWarning() << "MCMCLoopCurve::update_Komlan () Caught Exception!\n";
        return false;
    }

    return false;


}
/**
 * @brief MCMCLoopCurve::update_interpolate. Ici lambda est fixe et égale à 0. Les variables : ln_h_YWI_2, ln_h_YWI_3, ln_h_YWI_1_2,
 * Il y a seulement les theta à échantillonner et recalculer la spline correspondante
 * @return 
 */
bool MCMCLoopCurve::update_interpolate()
{
    try {
        // copie la liste des pointeurs
       /* std::vector<Event*> initListEvents (mModel->mEvents.size());
        std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );
*/
        // init the current state
        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);

        current_vecH = calculVecH(mModel->mEvents);

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        current_decomp_QTQ = decompositionCholesky(current_splineMatrices.matQTQ, 5, 1); // used only with update Theta
    
        current_decomp_matB = decompositionCholesky(current_splineMatrices.matR, 5, 1); //decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);

        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH
        //current_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. : ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, hasY, hasZ);
        current_ln_h_YWI_3 =  0.;
        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);

        current_h_lambda = 1;


        /* --------------------------------------------------------------
         *  A - Update ti Dates
         *  B - Update Theta Events
         *  C.1 - Update Alpha, Beta & Duration Phases
         *  C.2 - Update Tau Phases
         *  C.3 - Update Gamma Phases
         *
         *  Dans MCMCLoopChrono, on appelle simplement : event->updateTheta(t_min,t_max); sur tous les events.
         *  Pour mettre à jour un theta d'event dans Curve, on doit accéder aux thetas des autres events.
         *  => On effectue donc la mise à jour directement ici, sans passer par une fonction
         *  de la classe event (qui n'a pas accès aux autres events)
         * ---------------------------------------------------------------------- */
        if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
            /* --------------------------------------------------------------
             *  A - Update ti Dates
             * -------------------------------------------------------------- */
            try {
                if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
                    for (auto&& event : mModel->mEvents) {
                        for (auto&& date : event->mDates )   {
                            date.updateDate(event);
                        }
                    }
                }

            } catch(std::exception& exc) {
                qWarning() << "[MCMCLoopCurve::update] Ti : Caught Exception!\n"<<exc.what();

            }  catch (...) {
                qWarning() << "[MCMCLoopCurve::update] Ti : Caught Exception!";
            }


            /* --------------------------------------------------------------
             *  B - Update Theta Events
             * -------------------------------------------------------------- */
            try {
                for (Event*& event : initListEvents) {
                    // Variable du MH de la spline
                    
                    if (event->mType == Event::eDefault) {
                        const double min = event->getThetaMin(tminPeriod);
                        const double max = event->getThetaMax(tmaxPeriod);

                        if (min >= max) {
                            mAbortedReason = QObject::tr("Error for event theta : %1 : min = %2 : max = %3").arg(event->mName, QString::number(min), QString::number(max));
                            return false;
                        }

                        const double current_value = event->mTheta.mX;
                        current_h_theta = h_theta_Event(event);

                        const double try_value = Generator::gaussByBoxMuller(current_value, event->mTheta.mSigmaMH);

                        if (try_value >= min && try_value <= max) {
                            // On force la mise à jour de la nouvelle valeur pour calculer h_new

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);
                            try_h_theta = h_theta_Event(event);
                            const t_prob rate = try_h_theta /current_h_theta;
                            event->mTheta.mX = current_value;
                            event->mTheta.tryUpdate(try_value, rate);
                            
                            if ( event->mTheta.mLastAccepts.last() == true) {

                                // update after tryUpdate or updateTheta
                                event->mThetaReduced = mModel->reduceTime(event->mTheta.mX);
                                
                                /* --------------------------------------------------------------
                                 * C.1 - Update Alpha, Beta & Duration Phases
                                 * -------------------------------------------------------------- */
                                //  Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
                                std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (Phase* p) {p->update_AlphaBeta(tminPeriod, tmaxPeriod);});
                                
                            }
                            
                            
                        } else { // force rejection
                            event->mTheta.mX = current_value;
                            event->mThetaReduced = mModel->reduceTime(event->mTheta.mX);
                            event->mTheta.tryUpdate(try_value, -1);
                            
                        }

                    } 
                    
                    // If  Bound, nothing to sample. Always the same value
                     
                    
                } // End of loop initListEvents
                /* --------------------------------------------------------------
                 *  C.2 - Update Tau Phases
                 * -------------------------------------------------------------- */
                std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (Phase* p) {p->update_Tau (tminPeriod, tmaxPeriod);});
            
                // --------------------------------------------------------------
                //  C.3 - Update Gamma Phases constraints
                // --------------------------------------------------------------
                std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (PhaseConstraint* pc) {pc->updateGamma();});

            } catch (std::exception& exc) {
                qWarning() << "[MCMCLoopCurve::update] Theta : Caught Exception!\n"<<exc.what();
            }

        } 
        
        // if not Bayesian, times are fixed - Nothing to do

        /* --------------------------------------------------------------
         *  Nothing to do
         *  D - Update S02 Vg
         *  E.1 - Update Vg for Points only
         *  E.2 - Update Vg Global
         * -------------------------------------------------------------- */
     
        /* --------------------------------------------------------------
         * Nothing to do
         * F - Update Lambda 
         * -------------------------------------------------------------- */
   
        /* --------------------------------------------------------------
         *  G - Update mModel->mSpline
         * -------------------------------------------------------------- */
        // G.1- Update the current spline
        /*
         *  Preparation of the spline calculation / Tous ces calculs sont effectués dans currentSpline() avec doSortSpreadTheta=true
         orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
         spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date
         try_vecH = calculVecH(mModel->mEvents);
         try_splineMatrices = prepareCalculSpline(mModel->mEvents, try_vecH);

         */
        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);

        current_vecH = calculVecH(mModel->mEvents);

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);

        if (mCurveSettings.mUseErrMesure)
            mModel->mSpline = currentSpline(mModel->mEvents, true, current_vecH, current_splineMatrices);
        else
            mModel->mSpline = currentSpline_WI(mModel->mEvents, true, current_vecH, current_splineMatrices);

        // G.2 - test GPrime positive
        // si dy > mCurveSettings.mThreshold = pas d'acceptation
        if (mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth)
            return hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold);

        else
            return true;


    } catch (const char* e) {
        qWarning() << "[MCMCLoopCurve::update_interpolate] char "<< e;

    } catch (const std::length_error& e) {
        qWarning() << "[MCMCLoopCurve::update_interpolate] length_error"<< e.what();

    } catch (const std::out_of_range& e) {
        qWarning() << "[MCMCLoopCurve::update_interpolate] out of range" <<e.what();

    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve::update_interpolate]  "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update_interpolate] Caught Exception!\n";
        return false;
    }

    return false;
}

bool MCMCLoopCurve::adapt(const int batchIndex)
{
    const double taux_min = 0.42;           // taux_min minimal rate of acceptation=42
    const double taux_max = 0.46;           // taux_max maximal rate of acceptation=46

    bool noAdapt = true;

    // --------------------- Adapt -----------------------------------------
    const int sizeAdapt = 10000;

    const double delta = (batchIndex < sizeAdapt) ? pow(sizeAdapt, -1/2.)  : pow(batchIndex, -1/2.);

    for (auto& event : mModel->mEvents) {
        for (auto& date : event->mDates) {
            //--------------------- Adapt Sigma MH de t_i -----------------------------------------
            if (date.mTi.mSamplerProposal == MHVariable::eMHSymGaussAdapt)
                noAdapt &= date.mTi.adapt(taux_min, taux_max, delta);

            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            noAdapt &= date.mSigmaTi.adapt(taux_min, taux_max, delta);

        }

        //--------------------- Adapt Sigma MH de Theta Event -----------------------------------------
        if ((event->mType != Event::eBound) && ( event->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss) )
            noAdapt &= event->mTheta.adapt(taux_min, taux_max, delta);


        //--------------------- Adapt Sigma MH de VG  -----------------------------------------
        if ((event->mType != Event::eBound) && ( event->mVg.mSamplerProposal == MHVariable::eMHAdaptGauss) )
            noAdapt &= event->mVg.adapt(taux_min, taux_max, delta);


    }

    //--------------------- Adapt Sigma MH de S02 Vg -----------------------------------------
    if (mModel->mS02Vg.mSamplerProposal == MHVariable::eMHAdaptGauss)
        noAdapt &= mModel->mS02Vg.adapt(taux_min, taux_max, delta);

    //--------------------- Adapt Sigma MH de Lambda Spline -----------------------------------------
    if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eMHAdaptGauss)
        noAdapt &= mModel->mLambdaSpline.adapt(taux_min, taux_max, delta);

    return noAdapt;
}


void MCMCLoopCurve::memo()
{
    /* --------------------------------------------------------------
     *  A + B - Memo ti Dates & Theta
     *  -------------------------------------------------------------- */
    for (auto&& event : mModel->mEvents) {
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
            event->mTheta.memo();
            event->mTheta.saveCurrentAcceptRate();


            if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
                event->mS02.memo();
                event->mS02.saveCurrentAcceptRate();
            }

            for (auto&& date : event->mDates )   {
                date.mTi.memo();
                date.mSigmaTi.memo();
                date.mWiggle.memo();

                date.mTi.saveCurrentAcceptRate();
                date.mSigmaTi.saveCurrentAcceptRate();
            }
        }
    }

    /* --------------------------------------------------------------
     * C - Memo Alpha, Beta & Duration Phases
     * -------------------------------------------------------------- */
    std::for_each(mModel->mPhases.begin(), mModel->mPhases.end(), [](Phase* p) {p->memoAll();} );

    /* --------------------------------------------------------------
     *  D -  Memo S02 Vg
     * -------------------------------------------------------------- */
    if (mModel->mS02Vg.mSamplerProposal != MHVariable::eFixe) {
        double memoS02 = sqrt(mModel->mS02Vg.mX);
        mModel->mS02Vg.memo(&memoS02);
        mModel->mS02Vg.saveCurrentAcceptRate();
    }
    /* --------------------------------------------------------------
     *  E -  Memo Vg
     * -------------------------------------------------------------- */
    for (auto&& event : mModel->mEvents) {
         if (event->mVg.mSamplerProposal != MHVariable::eFixe) {
             // On stocke la racine de VG, qui est une variance pour afficher l'écart-type
            double memoVG = sqrt(event->mVg.mX);
            event->mVg.memo(&memoVG);
            event->mVg.saveCurrentAcceptRate();
        }
    }

    /* --------------------------------------------------------------
     * F - Memo Lambda
     * -------------------------------------------------------------- */
    // On stocke le log10 de Lambda Spline pour afficher les résultats a posteriori
    if (mModel->mLambdaSpline.mSamplerProposal != MHVariable::eFixe) {
        double memoLambda = log10(mModel->mLambdaSpline.mX);
        mModel->mLambdaSpline.memo(&memoLambda);
        mModel->mLambdaSpline.saveCurrentAcceptRate();
    }



    // Search map size
    if ( mState == State::eBurning ||
            ((mState == State::eAdapting && mModel->mChains[mChainIndex].mBatchIndex > mModel->mMCMCSettings.mMaxBatches/3)) ) {
        PosteriorMeanG* meanG = &mModel->mPosteriorMeanG;
        PosteriorMeanG* chainG = &mModel->mPosteriorMeanGByChain[mChainIndex];

        double minY_X, minY_Y, minY_Z;
        double maxY_X, maxY_Y, maxY_Z;

        minY_X = meanG->gx.mapG.minY();
        maxY_X = meanG->gx.mapG.maxY();

        if (mComputeY) {
            minY_Y = meanG->gy.mapG.minY();
            maxY_Y = meanG->gy.mapG.maxY();
            if (mComputeZ) {
                minY_Z = meanG->gz.mapG.minY();
                maxY_Z = meanG->gz.mapG.maxY();
            }
        }


        // New extrenum of the maps

        const int nbPtsX = 100;
        const double stepT = (mModel->mSettings.mTmax - mModel->mSettings.mTmin) / (nbPtsX - 1);

        double t;
        double gx_the, gy_the, gz_the, varGx, varGy, varGz, gp, gs;
        unsigned i0 = 0;

        // Convertion IDF
        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||  mModel->mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical) {
            const double deg = 180. / M_PI ;

            for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
                t = (double)idxT * stepT + mModel->mSettings.mTmin ;
                // Le premier calcul avec splineX évalue i0, qui est retoiurné, à la bonne position pour les deux autres splines
                ModelCurve::valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineX, gx_the, varGx, gp, gs, i0, *mModel);
                ModelCurve::valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineY, gy_the, varGy, gp, gs, i0, *mModel);
                ModelCurve::valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineZ, gz_the, varGz, gp, gs, i0, *mModel);

                const double zF = sqrt(pow(gx_the, 2.) + pow(gy_the, 2.) + pow(gz_the, 2.));
                const double xInc = asin(gz_the/ zF) * deg ;
                const double yDec = atan2(gy_the, gx_the) * deg;

                const double errF = sqrt((varGx + varGy + varGz)/3.);

                const double errI = (errF / zF) * deg ;
                const double errD = (errF / (zF * cos(xInc/deg))) * deg;

                minY_X = std::min(xInc - 1.96 * errI, minY_X);
                minY_Y = std::min(yDec - 1.96 * errD, minY_Y);
                minY_Z = std::min(zF - 1.96 * errF, minY_Z);

                maxY_X = std::max(xInc + 1.96 * errI, maxY_X);
                maxY_Y = std::max(yDec + 1.96 * errD, maxY_Y);
                maxY_Z = std::max(zF + 1.96 * errF, maxY_Z);

            }

        }  else {
            for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
                t = (double)idxT * stepT + mModel->mSettings.mTmin ;
                ModelCurve::valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineX, gx_the, varGx, gp, gs, i0, *mModel);
                minY_X = std::min(gx_the - 1.96 * sqrt(varGx), minY_X);
                maxY_X = std::max(gx_the + 1.96 * sqrt(varGx), maxY_X);

                if (mComputeY) {
                    ModelCurve::valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineY, gy_the, varGy, gp, gs, i0, *mModel);
                    minY_Y = std::min(gy_the - 1.96 * sqrt(varGy), minY_Y);
                    maxY_Y = std::max(gy_the + 1.96 * sqrt(varGy), maxY_Y);

                    if (mComputeZ) {
                        ModelCurve::valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineZ, gz_the, varGz, gp, gs, i0, *mModel);
                        minY_Z = std::min(gz_the - 1.96 * sqrt(varGy), minY_Z);
                        maxY_Z = std::max(gz_the + 1.96 * sqrt(varGz), maxY_Z);
                    }
                }


            }
        }


        if (mChainIndex == 0 ) {// do not change the Y range between several chain
            minY_X = std::min(minY_X, meanG->gx.mapG.minY());
            maxY_X = std::max(maxY_X, meanG->gx.mapG.maxY());
            meanG->gx.mapG.setRangeY(minY_X, maxY_X);

        } else {
            minY_X = std::min(minY_X, chainG->gx.mapG.minY());
            maxY_X = std::max(maxY_X, chainG->gx.mapG.maxY());
        }

        chainG->gx.mapG.setRangeY(minY_X, maxY_X);

        if (mComputeY) {
            if (mChainIndex == 0 ) {// do not change the Y range between several chain
                minY_Y = std::min(minY_Y, meanG->gy.mapG.minY());
                maxY_Y = std::max(maxY_Y, meanG->gy.mapG.maxY());
                meanG->gy.mapG.setRangeY(minY_Y, maxY_Y);

            } else {
                minY_Y = std::min(minY_Y, chainG->gy.mapG.minY());
                maxY_Y = std::max(maxY_Y, chainG->gy.mapG.maxY());
            }
            chainG->gy.mapG.setRangeY(minY_Y, maxY_Y);

            if (mComputeZ) {
                if (mChainIndex == 0 ) {// do not change the Y range between several chain
                    minY_Z = std::min(minY_Z, meanG->gz.mapG.minY());
                    maxY_Z = std::max(maxY_Z, meanG->gz.mapG.maxY());
                    meanG->gz.mapG.setRangeY(minY_Z, maxY_Z);

                } else {
                    minY_Z = std::min(minY_Z, chainG->gz.mapG.minY());
                    maxY_Z = std::max(maxY_Z, chainG->gz.mapG.maxY());
                }
                mModel->mPosteriorMeanGByChain[mChainIndex].gz.mapG.setRangeY(minY_Z, maxY_Z);
            }
        }

    }

    if (mState != State::eAquisition)
        return;

    /* --------------------------------------------------------------
     *  G - Memo  mModel->mSpline
     * -------------------------------------------------------------- */
    mModel->mSplinesTrace.push_back(mModel->mSpline);

    //--------------------- Create posteriorGMean and map and memo -----------------------------------------

    // 1 - initialisation à faire dans init()

    int iterAccepted = mChains[mChainIndex].mRealyAccepted + 1;
    int totalIterAccepted = 1;
    for (auto& c : mChains)
        totalIterAccepted += c.mRealyAccepted;

    if (!mComputeY) {
        memo_PosteriorG( mModel->mPosteriorMeanGByChain[mChainIndex].gx, mModel->mSpline.splineX, iterAccepted );
        if (mChains.size() > 1)
            memo_PosteriorG( mModel->mPosteriorMeanG.gx, mModel->mSpline.splineX, totalIterAccepted);

    } else {
      /*  if (mTh_memoCurve.joinable())
            mTh_memoCurve.join();

        mTh_memoCurve = std::thread ([this](int iter){memo_PosteriorG_3D( mModel->mPosteriorMeanGByChain[mChainIndex], mModel->mSpline, mCurveSettings.mProcessType, iter, *mModel );}, iterAccepted);
*/
        memo_PosteriorG_3D( mModel->mPosteriorMeanGByChain[mChainIndex], mModel->mSpline, mCurveSettings.mProcessType, iterAccepted, *mModel );

        if (mChains.size() > 1)
            memo_PosteriorG_3D( mModel->mPosteriorMeanG, mModel->mSpline, mCurveSettings.mProcessType, totalIterAccepted, *mModel);
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


    double n = realyAccepted;

    // 3 - calcul pour la composante
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
        double gp, gs;
        double gx, varGx;
        const double t = (double)idxT * stepT + mModel->mSettings.mTmin ;
        ModelCurve::valeurs_G_VarG_GP_GS(t, splineComposante, gx, varGx, gp, gs, i0, *mModel);

        // -- Calcul Mean
        const double prevMeanG = *itVecG;
        *itVecG +=  (gx - prevMeanG)/n;

        *itVecGP +=  (gp - *itVecGP)/n;
        *itVecGS +=  (gs - *itVecGS)/n;
        // erreur inter spline
        *itVecVarianceG +=  (gx - prevMeanG)*(gx - *itVecG);
        // erreur intra spline
        *itVecVarErrG += (varGx - *itVecVarErrG) / n  ;

        ++itVecG;
        ++itVecGP;
        ++itVecGS;
        ++itVecVarianceG;
        ++itVecVarErrG;


        // -- calcul map

        const double stdG = sqrt(varGx);

        // Ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
         * https://en.wikipedia.org/wiki/Error_function
         */
        //const double k = 3.; // Le nombre de fois sigma G, pour le calcul de la densité (gx - k*stdG - ymin)

        const int idxYErrMin = std::clamp( 0, int((gx - 3.*stdG - ymin) / stepY), nbPtsY-1);
        const int idxYErrMax = std::clamp( 0, int((gx + 3.*stdG - ymin) / stepY), nbPtsY-1);

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY-1) {
#ifdef DEBUG
            if ((curveMap.row()*idxT + idxYErrMin) < (curveMap.row()*curveMap.column()))
                curveMap(idxT, idxYErrMin) = curveMap.at(idxYErrMin, idxYErrMin) + 1; // correction à faire dans finalize() + 1./nbIter;
            else
                qDebug()<<"pb in MCMCLoopCurve::memo_PosteriorG";
#else
            curveMap(idxT, idxYErrMin) = curveMap.at(idxT, idxYErrMin) + 1.; // correction à faire dans finalize/nbIter ;
#endif

            curveMap.max_value = std::max(curveMap.max_value, curveMap.at(idxT, idxYErrMin));

        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY) {
            double* ptr_Ymin = curveMap.ptr_at(idxT, idxYErrMin);
            double* ptr_Ymax = curveMap.ptr_at(idxT, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {

                const double a = (idErr - 0.5)*stepY + ymin;
                const double b = (idErr + 0.5)*stepY + ymin;
                const double surfG = diff_erf(a, b, gx, stdG );// correction à faire dans finalyze /nbIter;
#ifdef DEBUG

                *ptr_idErr = (*ptr_idErr) + surfG;

#else
                //curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                *ptr_idErr = (*ptr_idErr) + surfG;
#endif

                curveMap.max_value = std::max(curveMap.max_value, *ptr_idErr);

                idErr++;
            }
        }


    }
    int tIdx = 0;
    if (updateLoop == &MCMCLoopCurve::update_Komlan) {
        for (auto& vVarG : vecVarG) {

#ifdef CODE_KOMLAN
        vVarG = vecVarianceG.at(tIdx)/ n;
#else
        vVarG = vecVarianceG.at(tIdx)/ n + vecVarErrG.at(tIdx);
#endif
        ++tIdx;
        }

    } else {
        for (auto& vVarG : vecVarG) {
            vVarG = vecVarianceG.at(tIdx)/ n + vecVarErrG.at(tIdx);
            ++tIdx;
        }
    }


}

void MCMCLoopCurve::memo_PosteriorG_3D(PosteriorMeanG &postG, MCMCSpline spline, CurveSettings::ProcessType &curveType, const int realyAccepted, ModelCurve &model)
{
    const double deg = 180. / M_PI ;
    const bool computeZ = (model.mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
                       model.mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
                       model.mCurveSettings.mProcessType == CurveSettings::eProcessType3D);

    auto* curveMap_XInc = &postG.gx.mapG;
    auto* curveMap_YDec = &postG.gy.mapG;
    auto* curveMap_ZF = &postG.gz.mapG;

    const int nbPtsX = curveMap_ZF->column(); // identique à toutes les maps

    const int nbPtsY_XInc = curveMap_XInc->row();
    const int nbPtsY_YDec = curveMap_YDec->row();
    const int nbPtsY_ZF = curveMap_ZF->row();

    const double ymin_XInc = curveMap_XInc->minY();
    const double ymax_XInc = curveMap_XInc->maxY();

    const double ymin_YDec = curveMap_YDec->minY();
    const double ymax_YDec = curveMap_YDec->maxY();

    const double ymin_ZF = curveMap_ZF->minY();
    const double ymax_ZF = curveMap_ZF->maxY();

    const double stepT = (model.mSettings.mTmax - model.mSettings.mTmin) / (nbPtsX - 1);
    const double stepY_XInc = (ymax_XInc - ymin_XInc) / (nbPtsY_XInc - 1);
    const double stepY_YDec = (ymax_YDec - ymin_YDec) / (nbPtsY_YDec - 1);
    const double stepY_ZF = (ymax_ZF - ymin_ZF) / (nbPtsY_ZF - 1);

    // 2 - Variables temporaires
    // référence sur variables globales
    std::vector<double> &vecVarG_XInc = postG.gx.vecVarG;
    std::vector<double> &vecVarG_YDec = postG.gy.vecVarG;
    std::vector<double> &vecVarG_ZF = postG.gz.vecVarG;
    // Variables temporaires
    // erreur inter spline
    std::vector<double> &vecVarianceG_XInc = postG.gx.vecVarianceG;
    std::vector<double> &vecVarianceG_YDec = postG.gy.vecVarianceG;
    std::vector<double> &vecVarianceG_ZF = postG.gz.vecVarianceG;
    // erreur intra spline
    std::vector<double> &vecVarErrG_XInc = postG.gx.vecVarErrG;
    std::vector<double> &vecVarErrG_YDec = postG.gy.vecVarErrG;
    std::vector<double> &vecVarErrG_ZF = postG.gz.vecVarErrG;

    //Pointeur sur tableau
    std::vector<double>::iterator itVecG_XInc = postG.gx.vecG.begin();
    std::vector<double>::iterator itVecGP_XInc = postG.gx.vecGP.begin();
    std::vector<double>::iterator itVecGS_XInc = postG.gx.vecGS.begin();

    std::vector<double>::iterator itVecG_YDec = postG.gy.vecG.begin();
    std::vector<double>::iterator itVecGP_YDec = postG.gy.vecGP.begin();
    std::vector<double>::iterator itVecGS_YDec = postG.gy.vecGS.begin();

    std::vector<double>::iterator itVecG_ZF = postG.gz.vecG.begin();
    std::vector<double>::iterator itVecGP_ZF = postG.gz.vecGP.begin();
    std::vector<double>::iterator itVecGS_ZF = postG.gz.vecGS.begin();

    // Variables temporaires
    // erreur inter spline
    std::vector<double>::iterator itVecVarianceG_XInc = postG.gx.vecVarianceG.begin();
    std::vector<double>::iterator itVecVarianceG_YDec = postG.gy.vecVarianceG.begin();
    std::vector<double>::iterator itVecVarianceG_ZF = postG.gz.vecVarianceG.begin();
    // erreur intra spline
    std::vector<double>::iterator itVecVarErrG_XInc = postG.gx.vecVarErrG.begin();
    std::vector<double>::iterator itVecVarErrG_YDec = postG.gy.vecVarErrG.begin();
    std::vector<double>::iterator itVecVarErrG_ZF = postG.gz.vecVarErrG.begin();

    double n = realyAccepted;
    double  prevMeanG_XInc, prevMeanG_YDec, prevMeanG_ZF;

    const double k = 3.; // Le nombre de fois sigma G, pour le calcul de la densité

    int  idxYErrMin, idxYErrMax;

    // 3 - Calcul pour la composante
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idxT = 0; idxT < nbPtsX ; ++idxT) {

        double gx, gpx, gsx, varGx = 0;
        double gy, gpy, gsy, varGy = 0;
        double gz, gpz, gsz, varGz = 0;

        const double t = (double)idxT * stepT + model.mSettings.mTmin ;
        ModelCurve::valeurs_G_VarG_GP_GS(t, spline.splineX, gx, varGx, gpx, gsx, i0, model);
        ModelCurve::valeurs_G_VarG_GP_GS(t, spline.splineY, gy, varGy, gpy, gsy, i0, model);

        if (computeZ)
            ModelCurve::valeurs_G_VarG_GP_GS(t, spline.splineZ, gz, varGz, gpz, gsz, i0, model);

        // Conversion IDF
        if (curveType == CurveSettings::eProcessTypeVector ||  curveType == CurveSettings::eProcessTypeSpherical) {
            const double F = sqrt(pow(gx, 2.) + pow(gy, 2.) + pow(gz, 2.));
            const double Inc = asin(gz / F);
            const double Dec = atan2(gy, gx);

            const double ErrF = sqrt((varGx + varGy + varGz)/3.);

            const double ErrI = ErrF / F ;
            const double ErrD = ErrF / (F * cos(Inc)) ;

            gx = Inc * deg;
            gy = Dec * deg;
            gz = F;

            varGx = ErrI * deg;
            varGy = ErrD * deg;
            varGz = ErrF;
        }


        // -- Calcul Mean on XInc
        prevMeanG_XInc = *itVecG_XInc;
        *itVecG_XInc +=  (gx - prevMeanG_XInc)/n;

        *itVecGP_XInc +=  (gpx - *itVecGP_XInc)/n;
        *itVecGS_XInc +=  (gsx - *itVecGS_XInc)/n;
        // erreur inter spline
        *itVecVarianceG_XInc +=  (gx - prevMeanG_XInc)*(gx - *itVecG_XInc);
        // erreur intra spline
        *itVecVarErrG_XInc += (varGx - *itVecVarErrG_XInc) / n  ;

        ++itVecG_XInc;
        ++itVecGP_XInc;
        ++itVecGS_XInc;
        ++itVecVarianceG_XInc;
        ++itVecVarErrG_XInc;


        // -- Calcul map on XInc

        const double stdGx = sqrt(varGx);

        // Ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
         * https://en.wikipedia.org/wiki/Error_function
         */
        idxYErrMin = std::clamp( 0, int((gx - k*stdGx - ymin_XInc) / stepY_XInc), nbPtsY_XInc-1);
        idxYErrMax = std::clamp( 0, int((gx + k*stdGx - ymin_XInc) / stepY_XInc), nbPtsY_XInc-1);

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_XInc-1) {
#ifdef DEBUG
            if ((curveMap_XInc->row()*idxT + idxYErrMin) < (curveMap_XInc->row()*curveMap_XInc->column()))
                (*curveMap_XInc)(idxT, idxYErrMin) = curveMap_XInc->at(idxYErrMin, idxYErrMin) + 1; // correction à faire dans finalize() + 1./nbIter;
            else
                qDebug()<<"pb in MCMCLoopCurve::memo_PosteriorG";
#else
            (*curveMap_XInc)(idxT, idxYErrMin) = curveMap_XInc->at(idxT, idxYErrMin) + 1.; // correction à faire dans finalize/nbIter ;
#endif

            curveMap_XInc->max_value = std::max(curveMap_XInc->max_value, curveMap_XInc->at(idxT, idxYErrMin));


        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_XInc) {
            double* ptr_Ymin = curveMap_XInc->ptr_at(idxT, idxYErrMin);
            double* ptr_Ymax = curveMap_XInc->ptr_at(idxT, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_XInc + ymin_XInc;
                double b = (idErr + 0.5) * stepY_XInc + ymin_XInc;
                double surfG = diff_erf(a, b, gx, stdGx );// correction à faire dans finalyze /nbIter;
#ifdef DEBUG
                *ptr_idErr = (*ptr_idErr) + surfG;
#else
                //curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                *ptr_idErr = (*ptr_idErr) + surfG;
#endif

                curveMap_XInc->max_value = std::max(curveMap_XInc->max_value, *ptr_idErr);

                idErr++;
            }
        }



        // -- Calcul Mean on YDec
        prevMeanG_YDec = *itVecG_YDec;
        *itVecG_YDec +=  (gy - prevMeanG_YDec)/n;

        *itVecGP_YDec +=  (gpy - *itVecGP_YDec)/n;
        *itVecGS_YDec +=  (gsy - *itVecGS_YDec)/n;
        // erreur inter spline
        *itVecVarianceG_YDec +=  (gy - prevMeanG_YDec)*(gy - *itVecG_YDec);
        // erreur intra spline
        *itVecVarErrG_YDec += (varGy - *itVecVarErrG_YDec) / n  ;

        ++itVecG_YDec;
        ++itVecGP_YDec;
        ++itVecGS_YDec;
        ++itVecVarianceG_YDec;
        ++itVecVarErrG_YDec;

        // -- Calcul map on YDec

        const auto stdGy = sqrt(varGy);

        // Ajout densité erreur sur Y
        /* Il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
        * https://en.wikipedia.org/wiki/Error_function
        */
        idxYErrMin = std::clamp( 0, int((gy - k*stdGy - ymin_YDec) / stepY_YDec), nbPtsY_YDec -1);
        idxYErrMax = std::clamp( 0, int((gy + k*stdGy - ymin_YDec) / stepY_YDec), nbPtsY_YDec -1);

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_YDec-1) {
#ifdef DEBUG
            if ((curveMap_YDec->row()*idxT + idxYErrMin) < (curveMap_YDec->row()*curveMap_YDec->column()))
                (*curveMap_YDec)(idxT, idxYErrMin) = curveMap_YDec->at(idxYErrMin, idxYErrMin) + 1;
            else
                qDebug()<<"[MCMCLoopCurve::memo_PosteriorG] Problem";
#else
            (*curveMap_YDec)(idxT, idxYErrMin) = curveMap_YDec->at(idxT, idxYErrMin) + 1.; // correction à faire dans finalize/nbIter ;
#endif

            curveMap_YDec->max_value = std::max(curveMap_YDec->max_value, curveMap_YDec->at(idxT, idxYErrMin));


        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_YDec) {
            double* ptr_Ymin = curveMap_YDec->ptr_at(idxT, idxYErrMin);
            double* ptr_Ymax = curveMap_YDec->ptr_at(idxT, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_YDec + ymin_YDec;
                double b = (idErr + 0.5) * stepY_YDec + ymin_YDec;
                double surfG = diff_erf(a, b, gy, stdGy );
#ifdef DEBUG
                *ptr_idErr = (*ptr_idErr) + surfG;
#else
                //curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                *ptr_idErr = (*ptr_idErr) + surfG;
#endif

                curveMap_YDec->max_value = std::max(curveMap_YDec->max_value, *ptr_idErr);

                idErr++;
            }
        }


        if (computeZ) {

            // -- Calcul Mean on ZF
            prevMeanG_ZF = *itVecG_ZF;
            *itVecG_ZF +=  (gz - prevMeanG_ZF)/n;

            *itVecGP_ZF +=  (gpz - *itVecGP_ZF)/n;
            *itVecGS_ZF +=  (gsz - *itVecGS_ZF)/n;
            // erreur inter spline
            *itVecVarianceG_ZF +=  (gz - prevMeanG_ZF)*(gz - *itVecG_ZF);
            // erreur intra spline
            *itVecVarErrG_ZF += (varGz - *itVecVarErrG_ZF) / n  ;

            ++itVecG_ZF;
            ++itVecGP_ZF;
            ++itVecGS_ZF;
            ++itVecVarianceG_ZF;
            ++itVecVarErrG_ZF;


            // -- Calcul map on ZF

            // curveMap = curveMap_ZF;//postG.gz.mapG;
            const auto stdGz = sqrt(varGz);

            // ajout densité erreur sur Y
            /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
             * https://en.wikipedia.org/wiki/Error_function
             */
            idxYErrMin = std::clamp( 0, int((gz - k*stdGz - ymin_ZF) / stepY_ZF), nbPtsY_ZF-1);
            idxYErrMax = std::clamp( 0, int((gz + k*stdGz - ymin_ZF) / stepY_ZF), nbPtsY_ZF-1);

            if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_ZF-1) {
#ifdef DEBUG
                if ((curveMap_ZF->row()*idxT + idxYErrMin) < (curveMap_ZF->row()*curveMap_ZF->column()))
                    (*curveMap_ZF)(idxT, idxYErrMin) = curveMap_ZF->at(idxYErrMin, idxYErrMin) + 1;
                else
                    qDebug()<<"[MCMCLoopCurve::memo_PosteriorG] Problem";
#else
                (*curveMap_ZF)(idxT, idxYErrMin) = curveMap_ZF->at(idxT, idxYErrMin) + 1.;
#endif

                curveMap_ZF->max_value = std::max(curveMap_ZF->max_value, curveMap_ZF->at(idxT, idxYErrMin));


            } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_ZF) {
                double* ptr_Ymin = curveMap_ZF->ptr_at(idxT, idxYErrMin);
                double* ptr_Ymax = curveMap_ZF->ptr_at(idxT, idxYErrMax);

                int idErr = idxYErrMin;
                for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                    double a = (idErr - 0.5) * stepY_ZF + ymin_ZF;
                    double b = (idErr + 0.5) * stepY_ZF + ymin_ZF;
                    double surfG = diff_erf(a, b, gz, stdGz );
#ifdef DEBUG
                    *ptr_idErr = (*ptr_idErr) + surfG;
#else
                    //curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                    *ptr_idErr = (*ptr_idErr) + surfG;
#endif

                    curveMap_ZF->max_value = std::max(curveMap_ZF->max_value, *ptr_idErr);

                    idErr++;
                }
            }


        }

    }


    int tIdx = 0;
    for (auto& vVarG : vecVarG_XInc) {
        vVarG = vecVarianceG_XInc.at(tIdx)/ n + vecVarErrG_XInc.at(tIdx);
        ++tIdx;
    }
    tIdx = 0;
    for (auto& vVarG : vecVarG_YDec) {
        vVarG = vecVarianceG_YDec.at(tIdx)/ n + vecVarErrG_YDec.at(tIdx);
        ++tIdx;
    }
    tIdx = 0;
    for (auto& vVarG : vecVarG_ZF) {
        vVarG = vecVarianceG_ZF.at(tIdx)/ n + vecVarErrG_ZF.at(tIdx);
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
    // il faut la derniere iter
 /*   if (mTh_memoCurve.joinable())
        mTh_memoCurve.join();
*/
#pragma omp parallel for

 /*   for (int i = 0; i < mChains.size(); ++i) {
        ChainSpecs &chain = mChains[i];
        if (chain.mRealyAccepted == 0) {
            mAbortedReason = QString(tr("Warning : NO POSITIVE curve available with chain n° %1, current seed to change %2").arg (QString::number(i+1), QString::number(chain.mSeed)));
            throw mAbortedReason;
        }
    }
*/

    // Suppression des traces des chaines sans courbes acceptées

    int back_position = mModel->mLambdaSpline.mRawTrace->size();
    for (auto i = mChains.size()-1; i>-1; --i) {
        ChainSpecs &chain = mChains[i];
        // we add 1 for the init
        const int initBurnAdaptAcceptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch) + chain.mRealyAccepted;

        const int front_position = back_position - initBurnAdaptAcceptSize;
        if (chain.mRealyAccepted == 0) {
            emit setMessage((tr("Warning : NO POSITIVE curve available with chain n° %1, current seed to change %2").arg (QString::number(i+1), QString::number(chain.mSeed))));

            mChains.remove(i);

            mModel->mPosteriorMeanGByChain.erase(mModel->mPosteriorMeanGByChain.begin() + i);

           //mModel->mSplinesTrace.erase(mModel->mSplinesTrace.begin() + position, mModel->mSplinesTrace.end()); //il y a seulement les courbes acceptées

            mModel->mLambdaSpline.mRawTrace->erase(mModel->mLambdaSpline.mRawTrace->cbegin() + front_position, mModel->mLambdaSpline.mRawTrace->cbegin() + back_position);
            mModel->mLambdaSpline.mHistoryAcceptRateMH->erase(mModel->mLambdaSpline.mHistoryAcceptRateMH->cbegin() + front_position, mModel->mLambdaSpline.mHistoryAcceptRateMH->cbegin() + back_position);
            mModel->mLambdaSpline.mAllAccepts.remove(i);

            mModel->mS02Vg.mRawTrace->erase(mModel->mS02Vg.mRawTrace->cbegin() + front_position, mModel->mS02Vg.mRawTrace->cbegin() + back_position);
            mModel->mS02Vg.mHistoryAcceptRateMH->erase(mModel->mS02Vg.mHistoryAcceptRateMH->cbegin() + front_position, mModel->mS02Vg.mHistoryAcceptRateMH->cbegin() + back_position);
            mModel->mS02Vg.mAllAccepts.remove(i);

            for (auto ev : mModel->mEvents) {
                ev->mTheta.mRawTrace->erase(ev->mTheta.mRawTrace->cbegin() + front_position, ev->mTheta.mRawTrace->cbegin() + back_position);
                ev->mTheta.mHistoryAcceptRateMH->erase(ev->mTheta.mHistoryAcceptRateMH->cbegin() + front_position, ev->mTheta.mHistoryAcceptRateMH->cbegin() + back_position);
                ev->mTheta.mAllAccepts.remove(i);

                ev->mVg.mRawTrace->erase(ev->mVg.mRawTrace->cbegin() + front_position, ev->mVg.mRawTrace->cbegin() + back_position);
                ev->mVg.mHistoryAcceptRateMH->erase(ev->mVg.mHistoryAcceptRateMH->cbegin() + front_position, ev->mVg.mHistoryAcceptRateMH->cbegin() + back_position);
                ev->mVg.mAllAccepts.remove(i);

                if (!ev->mS02.mRawTrace->empty()) {
                    ev->mS02.mRawTrace->erase(ev->mS02.mRawTrace->cbegin() + front_position, ev->mS02.mRawTrace->cbegin() + back_position);
                    ev->mS02.mHistoryAcceptRateMH->erase(ev->mS02.mHistoryAcceptRateMH->cbegin() + front_position, ev->mS02.mHistoryAcceptRateMH->cbegin() + back_position);
                    ev->mS02.mAllAccepts.remove(i);
                }

                for (auto &d : ev->mDates) {
                    d.mTi.mRawTrace->erase(d.mTi.mRawTrace->cbegin() + front_position, d.mTi.mRawTrace->cbegin() + back_position);
                    d.mTi.mHistoryAcceptRateMH->erase(d.mTi.mHistoryAcceptRateMH->cbegin() + front_position, d.mTi.mHistoryAcceptRateMH->cbegin() + back_position);
                    d.mTi.mAllAccepts.remove(i);

                    d.mSigmaTi.mRawTrace->erase(d.mSigmaTi.mRawTrace->cbegin() + front_position, d.mSigmaTi.mRawTrace->cbegin() + back_position);
                    d.mSigmaTi.mHistoryAcceptRateMH->erase(d.mSigmaTi.mHistoryAcceptRateMH->cbegin() + front_position, d.mSigmaTi.mHistoryAcceptRateMH->cbegin() + back_position);
                    d.mSigmaTi.mAllAccepts.remove(i);

                    d.mWiggle.mRawTrace->erase(d.mWiggle.mRawTrace->cbegin() + front_position, d.mWiggle.mRawTrace->cbegin() + back_position);
                    d.mWiggle.mAllAccepts.remove(i);
                }
            }
        }
        back_position = front_position ;
    }
    if (mChains.isEmpty()) {
        mAbortedReason = QString(tr("Warning : NO POSITIVE curve available "));
        throw mAbortedReason;
    }

    // This is not a copy of all data!
    // Chains only contains description of what happened in the chain (numIter, numBatch adapt, ...)
    // Real data are inside mModel members (mEvents, mPhases, ...)
    mModel->mChains = mChains;

    // This is called here because it is calculated only once and will never change afterwards
    // This is very slow : it is for this reason that the results display may be long to appear at the end of MCMC calculation.
    /** @todo Find a way to make it faster !
                     */

    emit setMessage(tr("Computing posterior distributions and numerical results - Correlations"));
    mModel->generateCorrelations(mChains);

    // This should not be done here because it uses resultsView parameters
    // ResultView will trigger it again when loading the model

    emit setMessage(tr("Computing posterior distributions and numerical results - Densities and curves"));
    mModel->initDensities();

    // ----------------------------------------
    // Curve specific :
    // ----------------------------------------

    std::vector<MCMCSplineComposante> allChainsTraceX, chainTraceX, allChainsTraceY, chainTraceY, allChainsTraceZ, chainTraceZ;

    // find the min in the map, can't be done when we do the map

    for (auto& pmc : mModel->mPosteriorMeanGByChain) {
        auto mini = *std::min_element(begin(pmc.gx.mapG.data), end(pmc.gx.mapG.data));
        pmc.gx.mapG.min_value = mini;

        if (mComputeY) {
            mini = *std::min_element(begin(pmc.gy.mapG.data), end(pmc.gy.mapG.data));
            pmc.gy.mapG.min_value = mini;

            if (mComputeZ) {
                mini = *std::min_element(begin(pmc.gz.mapG.data), end(pmc.gz.mapG.data));
                pmc.gz.mapG.min_value = mini;

            }
        }


    }


    // if there is one chain, the mPosteriorMeanG is the mPosteriorMeanGByChain[0]
    if (mChains.size() == 1) {
        mModel->mPosteriorMeanG = mModel->mPosteriorMeanGByChain[0];

    } else {
        const auto minix = *std::min_element(begin(mModel->mPosteriorMeanG.gx.mapG.data), end(mModel->mPosteriorMeanG.gx.mapG.data));

        mModel->mPosteriorMeanG.gx.mapG.min_value = minix;

        if (mComputeY) {
            const auto miniy = *std::min_element(begin(mModel->mPosteriorMeanG.gy.mapG.data), end(mModel->mPosteriorMeanG.gy.mapG.data));
            mModel->mPosteriorMeanG.gy.mapG.min_value = miniy;

            if (mComputeZ) {
                const auto miniz = *std::min_element(begin(mModel->mPosteriorMeanG.gz.mapG.data), end(mModel->mPosteriorMeanG.gz.mapG.data));
                mModel->mPosteriorMeanG.gz.mapG.min_value = miniz;

            }
        }
    }



#ifdef DEBUG
    QTime endTime = QTime::currentTime();

    qDebug()<<"ModelCurve computed";
    qDebug()<<tr("[MCMCLoopCurve::finalize] finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug()<<tr("Total time elapsed %1").arg(QString(DHMS(startTime.elapsed())));
#endif


}


double MCMCLoopCurve::Calcul_Variance_Rice (const QList<Event *> &events) const
{
    // Calcul de la variance Rice (1984)
    double Var_Rice = 0;
    for (int i = 1; i < events.size(); ++i) {
        Var_Rice += pow(events.at(i)->mYx - events.at(i-1)->mYx, 2.);
    }
    Var_Rice = 0.5*Var_Rice/(events.size()-1);

    return Var_Rice;
}


double MCMCLoopCurve::valeurG(const double t, const MCMCSplineComposante& spline, unsigned long &i0) const
{
    const auto n = spline.vecThetaEvents.size();
    const auto tReduce = mModel->reduceTime(t);
    const auto t1 = mModel->reduceTime(spline.vecThetaEvents.at(0));
    const auto tn = mModel->reduceTime(spline.vecThetaEvents.at(n-1));
    double g = 0;

    if (tReduce < t1) {
        const auto t2 = mModel->reduceTime(spline.vecThetaEvents.at(1));
        double gp1 = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
        gp1 -= (t2 - t1) * spline.vecGamma.at(1) / 6.;
        return (double) (spline.vecG.at(0) - (t1 - tReduce) * gp1);

    } else if (tReduce >= tn) {
        const auto tn1 = mModel->reduceTime(spline.vecThetaEvents.at(n-2));
        auto gpn = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tn1);
        gpn += (tn - tn1) * spline.vecGamma.at(n-2) / 6.;
        return (double) (spline.vecG.at(n-1) + (tReduce - tn) * gpn);

    } else {
        for (; i0 < n-1; ++i0) {
            const auto ti1 = mModel->reduceTime(spline.vecThetaEvents.at(i0));
            const auto ti2 = mModel->reduceTime(spline.vecThetaEvents.at(i0+1));
            if ((tReduce >= ti1) && (tReduce < ti2)) {
               const auto h = ti2 - ti1;
                const double gi1 = spline.vecG.at(i0);
                const double gi2 = spline.vecG.at(i0+1);

                // Linear part :
                g = gi1 + (gi2-gi1)*(tReduce-ti1)/h;

                // Smoothing part :
                const auto gamma1 = spline.vecGamma.at(i0);
                const auto gamma2 = spline.vecGamma.at(i0+1);
                const auto p = (1./6.) * ((tReduce-ti1) * (ti2-tReduce)) * ((1.+(tReduce-ti1)/h) * gamma2 + (1.+(ti2-tReduce)/h) * gamma1);
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

    } else if (t >= tn) {
        errG = sqrt(spline.vecVarG.at(n-1));

    } else {
        for (; i0 <n-1; ++i0) {
            const auto ti1 = spline.vecThetaEvents[i0];
            const auto ti2 = spline.vecThetaEvents[i0+1];
            if ((t >= ti1) && (t < ti2)) {

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


    } else if (tReduce >= tn) {
        const double tin_1 = mModel->reduceTime(spline.vecThetaEvents.at(n-2));
        gPrime = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tin_1);
        gPrime += (tn - tin_1) * spline.vecGamma.at(n-2) / 6.;


    } else {
        for ( ;i0< n-1; ++i0) {
            const auto ti1 = mModel->reduceTime(spline.vecThetaEvents.at(i0));
            const auto ti2 = mModel->reduceTime(spline.vecThetaEvents.at(i0+1));
            if ((tReduce >= ti1) && (tReduce < ti2)) {
                const auto h = ti2 - ti1;
                const double gi1 = spline.vecG.at(i0);
                const double gi2 = spline.vecG.at(i0+1);
                const double gamma1 = spline.vecGamma.at(i0);
                const double gamma2 = spline.vecGamma.at(i0+1);

                gPrime = ((gi2-gi1)/h) - (1./6.) * (tReduce-ti1) * (ti2-tReduce) * ((gamma2-gamma1)/h);
                gPrime += (1./6.) * ((tReduce-ti1) - (ti2-tReduce)) * ( (1.+(tReduce-ti1)/h) * gamma2 + (1+(ti2-tReduce)/h) * gamma1 );

                break;
            }
        }

    }

    return gPrime;
}

double MCMCLoopCurve::valeurGSeconde(const double t, const MCMCSplineComposante& spline)
{
    const int n = spline.vecThetaEvents.size();
    const auto tReduce = mModel->reduceTime(t);
    // The second derivative is always zero outside the interval [t1,tn].
    double gSeconde = 0.;

    for (int i = 0; i < n-1; ++i) {
        const t_reduceTime ti1 = mModel->reduceTime(spline.vecThetaEvents.at(i));
        const t_reduceTime ti2 = mModel->reduceTime(spline.vecThetaEvents.at(i+1));
        if ((tReduce >= ti1) && (tReduce < ti2)) {
            const t_reduceTime h = ti2 - ti1;
            const double gamma1 = spline.vecGamma.at(i);
            const double gamma2 = spline.vecGamma.at(i+1);
            gSeconde = ((tReduce-ti1) * gamma2 + (ti2-tReduce) * gamma1) / h;
            break;
        }
    }

    return gSeconde;
}



#pragma mark Related to : calibrate

void MCMCLoopCurve::prepareEventsY(const QList<Event *> &events)
{
    std::for_each( events.begin(), events.end(), [this](Event *e) { prepareEventY(e); });
}

/**
 * Préparation des valeurs Yx, Yy, Yz et Sy à partir des valeurs saisies dans l'interface : Yinc, Ydec, Sinc, Yint, Sint
 */
void MCMCLoopCurve::prepareEventY(Event* const event  )
{
    const double rad = M_PI / 180.;
    if (mCurveSettings.mProcessType == CurveSettings::eProcessTypeUnivarie) {
        // Dans RenCurve, fichier U_cmt_lit_sauve

        switch (mCurveSettings.mVariableType) {
        case CurveSettings::eVariableTypeInclination:
            event->mYx = event->mXIncDepth;
            event->mSy = event->mS_XA95Depth; //ligne 348 : EctYij:= (1/sqrt(Kij))*Deg;
            break;
        case CurveSettings::eVariableTypeDeclination:
            event->mYx = event->mYDec;
            event->mSy = event->mS_XA95Depth / cos(event->mXIncDepth * rad); //ligne 364 : EctYij:=(1/(sqrt(Kij)*cos(Iij*rad)))*Deg;
            break;
        case CurveSettings::eVariableTypeField:
            event->mYx = event->mZField;
            event->mSy = event->mS_ZField;
            break;
        default:
            event->mYx = event->mXIncDepth;
            event->mSy = event->mS_XA95Depth;
            break;
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

}


#pragma mark Related to : update : calcul de h_new

/**
 * Calcul de h_YWI_AY pour toutes les composantes de Y event (suivant la configuration univarié, spérique ou vectoriel)
 */
t_prob MCMCLoopCurve::h_YWI_AY(const SplineMatrices& matrices, const QList<Event *> &events, const double lambdaSpline, const std::vector< double>& vecH, const bool hasY, const bool hasZ)
{
    (void) hasZ;
    const Matrix2D& matR = matrices.matR;

    Matrix2D matB;

    if (lambdaSpline != 0) {
        // Decomposition_Cholesky de matB en matL et matD
        // Si lambda global: calcul de Mat_B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D

        const Matrix2D &tmp = multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5);
        matB = addMatEtMat(matR, tmp, 5);

    } else {
        matB = matR;
    }

    const std::pair<Matrix2D, MatrixDiag> &decomp_matB = decompositionCholesky(matB, 5, 1);
    const std::pair<Matrix2D, MatrixDiag> &decomp_QTQ = decompositionCholesky(matrices.matQTQ, 5, 1);

    if (hasY) {
        // decomp_matB, decompQTQ sont indépendantes de la composante
        const t_prob hX = h_YWI_AY_composanteX(matrices, events, vecH, decomp_matB, decomp_QTQ, lambdaSpline);
        const t_prob hY = h_YWI_AY_composanteY(matrices, events, vecH, decomp_matB, decomp_QTQ, lambdaSpline);

        // To treat the 2D case, we use the 3D case by setting Yint = 100
        /*  const bool hasZ = (mCurveSettings.mProcessType == CurveSettings::eProcessType2D ||
                                           mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
                                           mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
                                           mCurveSettings.mProcessType == CurveSettings::eProcessType3D);

                        if (hasZ) {*/ //Always true
        const t_prob hZ = h_YWI_AY_composanteZ(matrices, events, vecH, decomp_matB, decomp_QTQ, lambdaSpline);
        return hX * hY *hZ;

    } else {
        return h_YWI_AY_composanteX(matrices, events, vecH, decomp_matB, decomp_QTQ, lambdaSpline);

    }

}


t_prob MCMCLoopCurve::h_YWI_AY_composanteX(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp_matB, const std::pair<Matrix2D, MatrixDiag> &decomp_QTQ, const double lambdaSpline)
{
    if (lambdaSpline == 0) {
        return 1.;
    }

    const SplineResults &spline = doSplineX(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector< double> &vecG = spline.vecG;
    const MatrixDiag &matD = decomp_matB.second;
   // const std::vector< double> &matD = spline.matD;
   // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    const int n = events.size();
    // Schoolbook algo
    /*
    t_prob h_exp = 0.;
    int i = 0;
    for (const auto& e : events) {
        h_exp  +=  e->mW * e->mYx * (e->mYx - vecG.at(i++));
    }
    */
    // C++ algo
    const t_prob h_exp = std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](Event* e,  double g) { return e->mW * e->mYx * (e->mYx - g); });

    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    const MatrixDiag &matDq = decomp_QTQ.second;

    // Schoolbook algo
    /*
    t_prob log_det_1_2 = 0.; // ne dépend pas de la composante X, Y ou Z
    for (int i = 1; i < n-1; ++i) { // correspond à i=shift jusqu'à nb_noeuds-shift
        log_det_1_2 += logl(matDq.at(i)/ matD.at(i));
    }
    */
    // C++ algo
    const t_prob log_det_1_2 = std::transform_reduce(PAR matDq.cbegin()+1, matDq.cend()-1, matD.begin()+1, 0., std::plus{}, [](double val1,  double val2) { return logl(val1/val2); });

#ifdef DEBUG
#ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteX] errno set to EDOM";
    }
    if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteX] -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
    }
#endif
#endif
    // calcul à un facteur (2*PI) puissance -(n-2) près
    const t_prob res = 0.5 * ( (n -2.) * logl(lambdaSpline) + log_det_1_2 - h_exp) ;
    return exp(res) ;
}

t_prob MCMCLoopCurve::h_YWI_AY_composanteY(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const std::pair<Matrix2D, MatrixDiag > &decomp_QTQ, const double lambdaSpline)
{
    if (lambdaSpline == 0) {
        return 1.;
    }

    const SplineResults &spline = doSplineY(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector< double> &vecG = spline.vecG;
    const MatrixDiag &matD = decomp_matB.second;//.matD;
   // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    const int n = events.size();

    // Schoolbook algo
    /*
    t_prob h_exp = 0.;
    int i = 0;
    for (const auto& e : events) {
        h_exp  +=  e->mW * e->mYy * (e->mYy - vecG.at(i++));
    }
    */
    // C++ algo
    const t_prob h_exp = std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](Event* e,  double g) { return e->mW * e->mYy * (e->mYy - g); });

    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    const MatrixDiag& matDq = decomp_QTQ.second;
    /*
    t_prob log_det_1_2 = 0.;
    for (int i = 1; i < n-1; ++i) { // correspond à i=shift jusqu'à nb_noeuds-shift
        log_det_1_2 += logl(matDq.at(i)/ matD.at(i));
    }
    */
    // C++ algo
    const t_prob log_det_1_2 = std::transform_reduce(PAR matDq.cbegin()+1, matDq.cend()-1, matD.begin()+1, 0., std::plus{}, [](double val1,  double val2) { return log(val1/val2); });

#ifdef DEBUG

#ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteY] errno set to EDOM";
    }
    if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteY] -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
    }
#endif
#endif
    // calcul à un facteur (2*PI) puissance -(n-2) près
    const t_prob res = 0.5 * ( (n - 2.) * logl(lambdaSpline) + log_det_1_2 - h_exp) ;
    return exp(res);
}

t_prob MCMCLoopCurve::h_YWI_AY_composanteZ(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime>& vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const std::pair<Matrix2D, MatrixDiag> &decomp_QTQ, const double lambdaSpline)
{
    if (lambdaSpline == 0) {
        return 1.;
    }

    const SplineResults &spline = doSplineZ(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector< double> &vecG = spline.vecG;
    const MatrixDiag &matD = decomp_matB.second;//.matD;
   // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    const int n = events.size();

    /*
    t_prob h_exp = 0.;
    int i = 0;
    for (const auto& e : events) {
        h_exp  +=  e->mW * e->mYz * (e->mYz - vecG.at(i++));
    }
    */
    // C++ algo
    const t_prob h_exp = std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](Event* e,  double g) { return e->mW * e->mYz * (e->mYz - g); });

    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    const MatrixDiag& matDq = decomp_QTQ.second;

    /*
    t_prob log_det_1_2 = 0.;
    for (int i = 1; i < n-1; ++i) { // correspond à i=shift jusqu'à nb_noeuds-shift
        log_det_1_2 += logl(matDq.at(i)/ matD.at(i));
    }
    */
    // C++ algo
    const t_prob log_det_1_2 = std::transform_reduce(PAR matDq.cbegin()+1, matDq.cend()-1, matD.begin()+1, 0., std::plus{}, [](double val1,  double val2) { return logl(val1/val2); });

#ifdef DEBUG
#ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteZ] errno set to EDOM";
    }
    if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteZ] -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
    }
#endif
#endif
    // calcul à un facteur (2*PI) puissance -(n-2) près
    const t_prob res = 0.5 * ( (n -2.) * logl(lambdaSpline) + log_det_1_2 - h_exp) ;
    return exp(res);
}

# pragma mark optimization

// use ASYNC thread
t_prob MCMCLoopCurve::ln_h_YWI_3_update_ASYNC(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ)
{
    Q_ASSERT_X(lambdaSpline!=0, "[MCMCLoopCurve::ln_h_YWI_3_update]", "lambdaSpline=0");
    std::future<t_prob> handle_try_ln_h_YWI_3_X = std::async(std::launch::async,  MCMCLoopCurve::ln_h_YWI_3_X, matrices, events, vecH, decomp_matB, lambdaSpline);

    // On prepare les variables pour multi-dimension
    std::promise<t_prob> tmpPromiseY ;

    std::future<t_prob> handle_try_ln_h_YWI_3_Y = hasY ? std::async(std::launch::async,  MCMCLoopCurve::ln_h_YWI_3_Y, matrices, events, vecH, decomp_matB, lambdaSpline) :
                                                          tmpPromiseY.get_future() ;

    std::promise<t_prob> tmpPromiseZ ;

    std::future<t_prob> handle_try_ln_h_YWI_3_Z = hasZ ? std::async(std::launch::async,  MCMCLoopCurve::ln_h_YWI_3_Z, matrices, events, vecH, decomp_matB, lambdaSpline) :
                                                           tmpPromiseZ.get_future() ;
    tmpPromiseY.set_value(1.);
    tmpPromiseZ.set_value(1.);

    return handle_try_ln_h_YWI_3_X.get() + handle_try_ln_h_YWI_3_Y.get() + handle_try_ln_h_YWI_3_Z.get();

}

t_prob MCMCLoopCurve::ln_h_YWI_3_update(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ)
{
    Q_ASSERT_X(lambdaSpline!=0, "[MCMCLoopCurve::ln_h_YWI_3_update]", "lambdaSpline=0");
    const t_prob try_ln_h_YWI_3_X = ln_h_YWI_3_X(matrices, events, vecH, decomp_matB, lambdaSpline);

    const t_prob try_ln_h_YWI_3_Y = hasY ? ln_h_YWI_3_Y(matrices, events, vecH, decomp_matB, lambdaSpline) : 0.;

    const t_prob try_ln_h_YWI_3_Z = hasZ ? ln_h_YWI_3_Z( matrices, events, vecH, decomp_matB, lambdaSpline) : 0.;

    return try_ln_h_YWI_3_X + try_ln_h_YWI_3_Y + try_ln_h_YWI_3_Z;

}

t_prob MCMCLoopCurve::ln_h_YWI_3_X(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline)
{
    const SplineResults &spline = doSplineX(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector<double> &vecG = spline.vecG;

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    return -std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](Event* e,  double g) { return e->mW * e->mYx * (e->mYx - g); });

}

t_prob MCMCLoopCurve::ln_h_YWI_3_Y(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline)
{
    const SplineResults &spline = doSplineY(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector<double> &vecG = spline.vecG;
    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    return -std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](Event* e,  double g) { return e->mW * e->mYy * (e->mYy - g); });
}

t_prob MCMCLoopCurve::ln_h_YWI_3_Z(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp_matB, const double lambdaSpline)
{
    const SplineResults &spline = doSplineZ(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector<double> &vecG = spline.vecG;

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    return -std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](Event* e,  double g) { return e->mW * e->mYz * (e->mYz - g); });
}


double MCMCLoopCurve::S02_lambda_WI(const SplineMatrices &matrices, const int nb_noeuds)
{
    const Matrix2D &matR = matrices.matR;
    const Matrix2D &matQ = matrices.matQ;
    const Matrix2D &matQT = matrices.matQT;

    // On pose W = matrice unité

    // calcul des termes diagonaux de W_1.K
    const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(matR, 3, 1);
    const Matrix2D &matRInv = inverseMatSym_origin(decomp, 5, 1);

   // const Matrix2D matK = multiMatParMat(multiMatParMat(matQ, matRInv, 5, 5), matQT, 3, 3);
    const Matrix2D &matK = multiplyMatrixBanded_Winograd(multiplyMatrixBanded_Winograd(matQ, matRInv, 2), matQT, 0); // bandwith->k1=k2=0, car on peut utiliser que les diagonales pour calculer la digonale de matK
    // matK est une matrice pleine
    /*
    double vm = 0.;
    for (int i = 0; i < nb_noeuds; ++i) {
        vm += matK[i][i];
    }
    const double S02_lambda = (nb_noeuds-2) / vm;
    return S02_lambda;
    */
    long unsigned i = 0;
    const double vm = std::accumulate(begin(matK), end(matK), 0., [&i] (double sum, auto k_i) { return sum + k_i[i++] ;});

    return (nb_noeuds-2) / vm;
}

t_prob MCMCLoopCurve::h_lambda(const SplineMatrices &matrices, const int nb_noeuds, const double lambdaSpline)
{
    /* initialisation de l'exposant mu du prior "shrinkage" sur lambda : fixe
       en posant mu=2, la moyenne a priori sur alpha est finie = (nb_noeuds-2)/somme(Mat_W_1K[i,i]) ;
       et la variance a priori sur lambda est infinie
       NB : si on veut un shrinkage avec espérance et variance finies, il faut mu >= 3
    */

    const int mu = 3;
    const t_prob c = S02_lambda_WI(matrices, nb_noeuds);

    // prior "shrinkage"
    return pow(c, mu) / pow(c + lambdaSpline, mu+1); //((mu/c) * pow(c/(c + lambdaSpline), mu+1));

}


/* ancienne fonction U_cmt_MCMC:: h_Vgij dans RenCurve
* lEvents.size() must be geater than 2
*/

t_prob MCMCLoopCurve::h_VG_Event(const Event* e, double S02_Vg) const
{
    const int a = 1; // pHd
    //const int a = 3; // version du 2022-06-17
    return pow(S02_Vg/(S02_Vg + e->mVg.mX), a+1) / S02_Vg;
}


/*
 * This calculation does not differentiate between points and nodes
 */
t_prob MCMCLoopCurve::h_S02_Vg(const QList<Event *> &events, double S02_Vg) const
{
    const double alp = 1.;
    const t_prob prior = pow(1./S02_Vg, alp+1.) * exp(-alp/S02_Vg);
    const int a = 1;

    if (mCurveSettings.mUseVarianceIndividual) {

         // Schoolbook algo
        /*
        t_prob prod_h_Vg;
        t_prob prod_h_Vg0 = 1.;
        for (auto& e : events) {
            prod_h_Vg0 *= pow(S02_Vg/(S02_Vg + e->mVg.mX), a+1) / S02_Vg;
        }

        */
        // math optimize algo
        /*
        t_prob prod_h_Vg1 = pow(S02_Vg, events.size()*a);
        t_prob prod_h_Vg2 = 1.;
        for (auto& e : events) {
            prod_h_Vg2 *= S02_Vg + e->mVg.mX;
        }
        prod_h_Vg  = prod_h_Vg1 / pow(prod_h_Vg2, a +1);
        */

        // C++ optimization, Can be parallelized
        const t_prob prod_h_Vg_denum = std::accumulate(events.begin(), events.end(), 1., [S02_Vg] (t_prob prod, auto e){return prod * (S02_Vg + e->mVg.mX);});
        const t_prob prod_h_Vg  = pow(S02_Vg, events.size()*a) / pow(prod_h_Vg_denum, a + 1);

        return prior * prod_h_Vg;

    } else {
        return prior * pow(S02_Vg/(S02_Vg + events[0]->mVg.mX), a+1) / S02_Vg;;
    }

}

/**
 * @brief MCMCLoopCurve::rate_h_S02_Vg
 * @abstract Optimization of h_S02_Vg(), tested with over 30000 Events
 * @param events
 * @param S02_Vg
 * @param try_S02
 * @return
 */
t_prob MCMCLoopCurve::rate_h_S02_Vg(const QList<Event *> &pointEvents, double S02_Vg, double try_S02) const
{
   // const double alp = 1.;
    const t_prob r_prior = pow((t_prob)S02_Vg/try_S02, 2.) * exp(1./(t_prob)S02_Vg - 1./try_S02);
    const t_prob n_pointEvents = pointEvents.size();

    if (mCurveSettings.mUseVarianceIndividual) {
        /* Schoolbook algo
         * const int a = 1;
         * t_prob prod_h_Vg;
         * t_prob prod_h_Vg0 = 1.;
         * for (auto& e : events) {
         *   prod_h_Vg0 *= pow(S02_Vg/(S02_Vg + e->mVg.mX), a+1) / S02_Vg;
         * }

        */

        // C++ optimization, Can be parallelized
        const t_prob prod_h_Vg_accumulate = std::accumulate(pointEvents.begin(), pointEvents.end(), 1., [S02_Vg, try_S02] (t_prob prod, auto e) {
            return   prod * (t_prob)((S02_Vg + e->mVg.mX)/(try_S02 + e->mVg.mX));
        });
        const t_prob ln_prod_h_Vg  = n_pointEvents * log((t_prob)try_S02/S02_Vg) + 2*log(prod_h_Vg_accumulate) ;
        return r_prior * exp(ln_prod_h_Vg);

    } else {
        const int a = 1;
        return r_prior * pow( try_S02/ (try_S02 + pointEvents.at(0)->mVg.mX)  * (S02_Vg + pointEvents.at(0)->mVg.mX)/S02_Vg , a+1) * S02_Vg/ try_S02; // à controler
    }
}

/* Identique à la fonction rate_h_S02_Vg, mais test si Event est un point ou un Noeud
 * Et fait le calcul avec les points
 *
 */
t_prob MCMCLoopCurve::rate_h_S02_Vg_test(const QList<Event *> &events, double S02_Vg, double try_S02) const
{
   // const double alp = 1.;
    const t_prob r_prior = pow((t_prob)S02_Vg/try_S02, 2.) * exp(1./(t_prob)S02_Vg - 1./try_S02);
    const int n_DefaultEvent = std::accumulate(events.begin(), events.end(), 1., [] (t_prob sum, auto e) {
        return  (e->mPointType == Event::eNode ? sum + 1. : sum);
    });

    if (mCurveSettings.mUseVarianceIndividual) {

         // Schoolbook algo
        /*
         * const int a = 1;
         * t_prob prod_h_Vg;
         * t_prob prod_h_Vg0 = 1.;
         * for (auto& e : events) {
         *   prod_h_Vg0 *= pow(S02_Vg/(S02_Vg + e->mVg.mX), a+1) / S02_Vg;
         * }

        */

        // C++ optimization, Can be parallelized
        const t_prob prod_h_Vg_accumulate = std::accumulate(events.begin(), events.end(), 1., [S02_Vg, try_S02] (t_prob prod, auto e) {
            return  (e->mPointType == Event::eNode ? prod * (t_prob)((S02_Vg + e->mVg.mX)/(try_S02 + e->mVg.mX)) : prod);
        });
        const t_prob ln_prod_h_Vg  = (t_prob)n_DefaultEvent * log((t_prob)try_S02/S02_Vg) + 2*log(prod_h_Vg_accumulate) ;
        return r_prior * exp(ln_prod_h_Vg);

    } else {
        const int a = 1;
        //return prior * pow(S02_Vg/(S02_Vg + events[0]->mVg.mX), a+1) / S02_Vg;
        return r_prior * pow( try_S02/ (try_S02 + events[0]->mVg.mX)  * (S02_Vg + events[0]->mVg.mX)/S02_Vg , a+1) * S02_Vg/ try_S02; // à controler
    }
}

/**
 * @brief MCMCLoopCurve::h_VG_Event_318 fonction utilisée dans la version 3.1.8
 * @param e
 * @param S02_Vg
 * @return
 */

/** @brief Calcul la variance individuelle spline, utilisé seulement pour var_residu_X à l'initialisation
 */
double MCMCLoopCurve::S02_Vg_Yx(QList<Event *> &events, SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline)
{
    const MCMCSpline &splineWI = currentSpline(events, false, vecH, matricesWI); // peut-être utiliser doSplineX()
    const std::vector< double> &vecG = splineWI.splineX.vecG;

    double S02 = 0.;
    // schoolbook
    /*
    for (unsigned long i = 0; i< vecG.size(); i++) {
        S02 += pow(_events[i]->mYx - vecG[i], 2.);
    }
    */
    // C++ optimization
    auto g = vecG.begin();
    for (auto& ev : events) {
        S02 += pow(ev->mYx - *g++, 2.);
    }
    //  Mat_B = R + alpha * Qt * W-1 * Q
    const Matrix2D matB = addMatEtMat(matricesWI.matR, multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    std::pair<Matrix2D, std::vector< Matrix2D::value_type::value_type>> decomp = decompositionCholesky(matB, 5, 1);

    const SplineResults s = doSplineX (matricesWI, events, vecH, decomp, lambdaSpline);

    const std::vector< double> &matA = calculMatInfluence_origin(matricesWI, 1, decomp, lambdaSpline);

    const double traceA = std::accumulate(matA.begin(), matA.end(), 0.);

    S02 /= (double)(vecG.size()) - traceA;
    return std::move(S02);

}


double MCMCLoopCurve::S02_Vg_Yy(QList<Event *> &events, SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline)
{
    const MCMCSpline &splineWI = currentSpline(events, false, vecH, matricesWI);
    const std::vector< double> &vecG = splineWI.splineY.vecG;

    double S02 = 0;
    // schoolbook
    /*
    for (unsigned long i = 0; i< vecG.size(); i++) {
        S02 += pow(_events[i]->mYy - vecG[i], 2.);
    }
    */
    // C++ optimization
    auto g = vecG.begin();
    for (auto& ev : events) {
        S02 += pow(ev->mYy - *g++, 2.);
    }
    //  Mat_B = R + alpha * Qt * W-1 * Q
    const Matrix2D &matB = addMatEtMat(matricesWI.matR, multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    const std::pair<Matrix2D, std::vector< Matrix2D::value_type::value_type>> &decomp = decompositionCholesky(matB, 5, 1);

   // const SplineResults &s = doSplineY (matricesWI, events, vecH, decomp, lambdaSpline);

    const std::vector< double> &matA = calculMatInfluence_origin(matricesWI, 1, decomp, lambdaSpline);

    const double traceA = std::accumulate(matA.begin(), matA.end(), 0.);

    S02 /= (double)(vecG.size()) - traceA;
    return std::move(S02);

}

double MCMCLoopCurve::S02_Vg_Yz(QList<Event *> &events, SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline)
{
    const MCMCSpline &splineWI = currentSpline(events, false, vecH, matricesWI);
    const std::vector< double> &vecG = splineWI.splineZ.vecG;

    double S02 = 0;
    // schoolbook
    /*
    for (unsigned long i = 0; i< vecG.size(); i++) {
        S02 += pow(_events[i]->mYz - vecG[i], 2.);
    }
    */
    // C++ optimization
    auto g = vecG.begin();
    for (auto& ev : events) {
        S02 += pow(ev->mYz - *g++, 2.);
    }
    //  Mat_B = R + alpha * Qt * W-1 * Q
    const Matrix2D &matB = addMatEtMat(matricesWI.matR, multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    const std::pair<Matrix2D, MatrixDiag>& decomp = decompositionCholesky(matB, 5, 1);

    //const SplineResults &s = doSplineZ (matricesWI, events, vecH, decomp, lambdaSpline);

    const std::vector<double> &matA = calculMatInfluence_origin(matricesWI, 1, decomp, lambdaSpline);

    const double traceA = std::accumulate(matA.begin(), matA.end(), 0.);

    S02 /= (double)(vecG.size()) - traceA;
    return std::move(S02);

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
        for (auto& date : e->mDates) {
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

t_prob MCMCLoopCurve::h_theta(const QList<Event *> &events) const
{
    if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
        t_prob h = 1.;
        for (Event* e : events) {
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


void MCMCLoopCurve::orderEventsByThetaReduced(QList<Event *> &event)
{
    // On manipule directement la liste des évènements
    // Ici on peut utiliser event en le déclarant comme copy ??
    QList<Event*> &result = event;

    std::sort(result.begin(), result.end(), [](const Event* a, const Event* b) { return (a->mThetaReduced < b->mThetaReduced); });
}


/**
 * @brief MCMCLoopCurve::minimalThetaDifference, if theta are sort, the result is positive
 * @param lEvents
 * @return
 */
double MCMCLoopCurve::minimalThetaDifference(QList<Event *>& events)
{
    std::vector<double> result (events.size());
    std::transform (events.begin(), events.end()-1, events.begin()+1, result.begin(), [](const Event* e0, const  Event* e1) {return (e1->mTheta.mX - e0->mTheta.mX); });
    // result.erase(result.begin()); // the firs value is not a difference, it's just the first value of LEvents
    std::sort(result.begin(), result.end());
    return std::move(*std::find_if_not (result.begin(), result.end(), [](double v){return v==0.;} ));
}

double MCMCLoopCurve::minimalThetaReducedDifference(QList<Event *> &events)
{
    std::vector<t_reduceTime> result (events.size());
    std::transform (events.begin(), events.end()-1, events.begin()+1, result.begin(), [](const Event* e0, const  Event* e1) {return (e1->mThetaReduced - e0->mThetaReduced); });
    // result.erase(result.begin()); // the firs value is not a difference, it's just the first value of LEvents
    std::sort(result.begin(), result.end());
    return std::move(*std::find_if_not (result.begin(), result.end(), []( t_reduceTime v){return v==0.;} ));
}

// not used
void MCMCLoopCurve::spreadEventsTheta(QList<Event *> &events, double minStep)
{
    // On manipule directement la liste des évènements
    QList<Event*> &result = events;

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
            const double startSpread = result.at(endIndex) - result.at(startIndex);

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

            for (auto r = result.begin() + startIndex; r != result.begin() + endIndex; ++r) {
                (*r)->mTheta.mX -= shiftBack;
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
 * @details On parcourt les events triées par date réduite croissante, si la date suivante ne vérifie pas le spreadSpand, on repère une cravate:
 * on mémorise la date comme début temps et on continue en vérifiant le spread suivant. Si le spreadSpan est vérifié, on note la fin de temps et on répartie les dates entres le début et la fin.
 * Sinon, on compte nbEgal+1 et on continue avec la date suivante.
 * @param sortedEvents
 * @param spreadSpan
 */
void MCMCLoopCurve::spreadEventsThetaReduced0(QList<Event *> &sortedEvents, t_reduceTime spreadSpan)
{
    QList<Event*>::iterator itEvenFirst = sortedEvents.end();
    QList<Event*>::iterator itEventLast = sortedEvents.end();
    unsigned nbEgal = 0;

    if (spreadSpan == 0.) {
        spreadSpan = 1.E-8; //std::numeric_limits<double>::epsilon() * 1.E12;//1.E6;// epsilon = 1E-16
    }

    // repère première egalité
    for (QList<Event*>::iterator itEvent = sortedEvents.begin(); itEvent != sortedEvents.end() -1; itEvent++) {

       // if ((*itEvent)->mThetaReduced == (*(itEvent+1))->mThetaReduced) {
        if ((*(itEvent+1))->mThetaReduced - (*(itEvent))->mThetaReduced <= spreadSpan) {

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
                const t_reduceTime lowBound = itEvenFirst == sortedEvents.begin() ? sortedEvents.first()->mThetaReduced : (*(itEvenFirst -1))->mThetaReduced ; //valeur à gauche non égale
                const t_reduceTime upBound = itEvent == sortedEvents.end()-2 ? sortedEvents.last()->mThetaReduced : (*(itEvent + 1))->mThetaReduced;

                t_reduceTime step = spreadSpan / (nbEgal-1); // écart théorique
                t_reduceTime min;

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

                const t_reduceTime max = std::min(upBound - spreadSpan, (*itEvent)->mThetaReduced + (t_reduceTime)(step*ceil(nbEgal/2.)) );
                step = (max- min)/ (nbEgal - 1); // écart corrigé

                QList<Event*>::iterator itEventEgal;
                int count;
                for (itEventEgal = itEvenFirst, count = 0; itEventEgal != itEvent+1; itEventEgal++, count++ ) {
#ifdef DEBUG
                //    const auto t_init = (*itEventEgal)->mThetaReduced;
#endif
                    (*itEventEgal)->mThetaReduced = min + count*step;
#ifdef DEBUG
                //    qDebug()<<"[MCMCLoopCurve] spreadEventsThetaReduced0() "<<(*itEventEgal)->mName <<" int time ="<<t_init <<" move to "<<(*itEventEgal)->mThetaReduced;
#endif
                }
                // Fin correction, prêt pour nouveau groupe/cravate
                itEvenFirst = sortedEvents.end();

            }
        }


    }

    // sortie de la boucle avec itFirst validé donc itEventLast == sortedEvents.end()-1

    if (itEvenFirst != sortedEvents.end()) {
        // On sort de la boucle et d'une égalité, il faut répartir les dates entre les bornes
        // itEvent == itEventLast
        const t_matrix lowBound = (*(itEvenFirst -1))->mThetaReduced ; //la première valeur à gauche non égale

        const t_matrix max = (*(sortedEvents.end()-1))->mThetaReduced;
        t_matrix step = spreadSpan / (nbEgal-1.); // ecart théorique

        const t_matrix min = std::max(lowBound + spreadSpan, max - step*(nbEgal-1) );

        step = (max- min)/ (nbEgal-1); // écart corrigé

        // Tout est réparti à gauche
        int count;
        QList<Event*>::iterator itEventEgal;
        for (itEventEgal = itEvenFirst, count = 0; itEventEgal != sortedEvents.end(); itEventEgal++, count++ ) {
#ifdef DEBUG
         //   const auto t_init = (*itEventEgal)->mThetaReduced;
#endif
            (*itEventEgal)->mThetaReduced = min + count *step;

#ifdef DEBUG
         //   qDebug()<<"[MCMCLoopCurve] spreadEventsThetaReduced0() "<<(*itEventEgal)->mName <<" int time ="<<t_init <<" move to "<<(*itEventEgal)->mThetaReduced;
#endif
        }

    }

}

double MCMCLoopCurve::yearTime(double reduceTime)
{
    return mModel->yearTime(reduceTime) ;
}

#pragma mark Pratique pour debug
std::vector<double> MCMCLoopCurve::getThetaEventVector(const QList<Event *> &events)
{
    std::vector<double> vecT(events.size());
    std::transform(PAR events.begin(), events.end(), vecT.begin(), [](Event* ev) {return ev->mTheta.mX;});

    return vecT;
}

#pragma mark Calcul Spline

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
SplineMatrices MCMCLoopCurve::prepareCalculSpline(const QList<Event *> &sortedEvents, const std::vector<t_reduceTime> &vecH)
{
    const Matrix2D &rMatR = calculMatR(vecH);
    const Matrix2D &rMatQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    const Matrix2D &rMatQT = transpose(rMatQ, 3);

    MatrixDiag diagWInv (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.begin(), [](Event* ev) {return 1/ev->mW;});

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    const Matrix2D &tmp = multiMatParDiag(rMatQT, diagWInv, 3);

    //Matrix2D matQTW_1Qb = multiMatParMat(tmp, matQ, 3, 3);
    const Matrix2D &rMatQTW_1Q = multiplyMatrixBanded_Winograd(tmp, rMatQ, 1);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    //Matrix2D matQTQb = multiMatParMat(matQT, matQ, 3, 3);
    const Matrix2D &rMatQTQ = multiplyMatrixBanded_Winograd(rMatQT, rMatQ, 1);

    SplineMatrices matrices;
    matrices.diagWInv = std::move(diagWInv);
    matrices.matR = std::move(rMatR);
    matrices.matQ = std::move(rMatQ);
    matrices.matQT = std::move(rMatQT);
    matrices.matQTW_1Q = std::move(rMatQTW_1Q); // Seule affectée par changement de VG
    matrices.matQTQ = std::move(rMatQTQ);

    return matrices;
}



/**
 * @brief MCMCLoopCurve::prepareCalculSpline_WI
 * With W = identity
 * @param sortedEvents
 * @param vecH
 * @return
 */
SplineMatrices MCMCLoopCurve::prepareCalculSpline_WI(const QList<Event *> &sortedEvents, const std::vector<t_reduceTime> &vecH)
{
    const Matrix2D& matR = calculMatR(vecH);
    const Matrix2D& matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n

    const Matrix2D& matQT = transpose(matQ, 3);

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    MatrixDiag diagWInv (sortedEvents.size(), 1);

   // Matrix2D matQTW_1Qb = multiMatParMat(matQT, matQ, 3, 3);
    const Matrix2D& matQTW_1Q = multiplyMatrixBanded_Winograd(matQT, matQ, 1);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
   // Matrix2D matQTQb = multiMatParMat(matQT, matQ, 3, 3);
    Matrix2D matQTQ = matQTW_1Q;// multiplyMatrixBanded_Winograd(matQT, matQ, 1); // pHd :: ?? on fait deux fois le même calcul !!

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
 * @brief MCMCLoopCurve::doSpline
 * fabrication de spline avec
 *      spline.vecG
 *      spline.vecGamma
 *
 * @param matrices
 * @param events
 * @param vecH
 * @param decomp
 * @param lambdaSpline
 *
 * @return SplineResults
 */

/**
 Cette procedure calcule la matrice inverse de B:
 B = R + lambda * Qt * W-1 * Q
 puis calcule la matrice d'influence A(lambda)
 Bande : nombre de diagonales non nulles
 used only with MCMCLoopCurve::calcul_spline_variance()
*/
std::vector<double> MCMCLoopCurve::calculMatInfluence_origin(const SplineMatrices& matrices, const int nbBandes, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline)
{
   // Q_ASSERT_X(lambdaSpline!=0, "[MCMCLoopCurve::ln_h_YWI_3_update]", "lambdaSpline=0");
    /*
     * if lambda spline = 0
     * return matA.resize(n, 1.);
     */
    const size_t n = mModel->mEvents.size();

    const Matrix2D &matB_1 = inverseMatSym_origin(decomp, nbBandes + 4, 1);

    std::vector<double> matQB_1QT;// = initVector(n);

    /*Calcul des termes diagonaux de Q (matB_1) Qt, de i=1 à nb_noeuds
             ??? vérifier le contrôle des indices i+1: débordement de matrices */


    const auto* matQi = begin(matrices.matQ[0]); // matQ est une matrice encadrée de 0
    t_matrix term = pow(matQi[0], 2) * matB_1[0][0]; // matQ[0][0] ici vaut toujours 0
    term += pow(matQi[1], 2) * matB_1[1][1];
    term += 2 * matQi[0] * matQi[1] * matB_1[0][1]; // ici vaut toujours 0
    //matQB_1QT[0] = term;
    matQB_1QT.push_back(term);

    for (size_t i = 1; i < n-1; ++i) {
        matQi = begin(matrices.matQ[i]);
        t_matrix term_i = pow(matQi[i-1], 2.) * matB_1[i-1][i-1];
        term_i += pow(matQi[i], 2.) * matB_1[i][i];
        term_i += pow(matQi[i+1], 2.) * matB_1[i+1][i+1];

        term_i += 2 * matQi[i-1] * matQi[i] * matB_1[i-1][i];
        term_i += 2 * matQi[i-1] * matQi[i+1] * matB_1[i-1][i+1];
        term_i += 2 * matQi[i] * matQi[i+1] * matB_1[i][i+1];
        //matQB_1QT[i] = term;
        matQB_1QT.push_back(term_i);
    }


    matQi = begin(matrices.matQ[n-1]);
    term = pow(matQi[n-2], 2.) * matB_1[n-2][n-2];
    term += pow(matQi[n-1], 2.) * matB_1[n-1][n-1]; // matQ[n-1][n-1] ici vaut toujours 0
    term += 2 * matQi[n-2] * matQi[n-1] * matB_1[n-2][n-1]; // matQ[n-1][n-1] ici vaut toujours 0
    //matQB_1QT[n-1] = term;
    matQB_1QT.push_back(term);


    // Multi_diag_par_Mat(Diag_W_1c, Mat_QB_1QT, Nb_noeudsc, 1, tmp1); // donne une matrice, donc la diagonale est le produit des diagonales
    // Multi_const_par_Mat(-alphac, tmp1, Nb_noeudsc,1, Mat_Ac);
    // Addit_I_et_Mat(Mat_Ac,Nb_noeudsc);
    // remplacé par:

    std::vector<double> matA;
    for (size_t i = 0; i < n; ++i) {
        matA.push_back(1 - lambdaSpline * matrices.diagWInv[i] * matQB_1QT[i]);
#if DEBUG
        if (matA[i] == 0.) {
            qWarning ("[MCMCLoopCurve::calculMatInfluence_origin] -> Oups matA.at(i) == 0 ");
        }

        if (matA[i] < 0.) {
            qDebug ()<<"[MCMCLoopCurve::calculMatInfluence_origin] -> Oups matA.at(i) ="<< matA[i] << " < 0  change to 1E-10 LambdaSpline="<<lambdaSpline;
            matA[i] = 1.E-10;
        }

        if (matA[i] > 1) {
            qWarning ("[MCMCLoopCurve::calculMatInfluence_origin] -> Oups matA.at(i) > 1  change to 1");
            matA[i] = 1.;
        }
#endif
    }

    return matA;
}


std::vector<double> MCMCLoopCurve::calcul_spline_variance(const SplineMatrices& matrices, const QList<Event*> &events, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline)
{
    unsigned int n = events.size();
    std::vector<double> matA = calculMatInfluence_origin(matrices, 1, decomp, lambdaSpline);
    std::vector<double> varG;

    for (unsigned int i = 0; i < n; ++i) {
#ifdef DEBUG
        const double& aii = matA[i];
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii <= 0.) {
            qDebug()<<"[MCMCLoopCurve] calcul_spline_variance() : Oups aii="<< aii <<"<= 0 change to 0" << "mW="<<mModel->mEvents[i]->mW;
            varG.push_back(0.);

        } else {
            varG.push_back(matA[i]  / events[i]->mW);
        }
#else
        varG.push_back(matA[i]  / events[i]->mW);
#endif

    }

    return varG;
}

MCMCSpline MCMCLoopCurve::currentSpline (QList<Event *> &events, bool doSortAndSpreadTheta, const std::vector<t_reduceTime> &vecH, const SplineMatrices &matrices)
{
    const std::vector<t_reduceTime>* pVecH;
    const SplineMatrices* pMatrices;

    SplineMatrices matricesSpread;
    std::vector<t_reduceTime> vecHSpread;

    if (doSortAndSpreadTheta) {
        orderEventsByThetaReduced(events);
        spreadEventsThetaReduced0(events);

        vecHSpread = calculVecH(events);
        pVecH = &vecHSpread;
        // prepareCalculSpline : ne fait pas intervenir les valeurs Y(x,y,z) des events :mais utilise les theta réduits
        // => On le fait une seule fois pour les 3 composantes

        matricesSpread = prepareCalculSpline(events, vecHSpread);
        pMatrices = &matricesSpread;

    } else {
        pVecH = &vecH;
        pMatrices = &matrices;
    }

    const std::vector<double> &vecTheta = getThetaEventVector(events);

    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    Matrix2D matB; //matR;
    const double lambda = mModel->mLambdaSpline.mX;
    if (lambda != 0) {
        const Matrix2D &tmp = multiConstParMat(pMatrices->matQTW_1Q, lambda, 5);
        matB = addMatEtMat(pMatrices->matR, tmp, 5);

    } else {
        matB = pMatrices->matR;
    }

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(matB, 5, 1);


    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédent ne change pas

    const SplineResults &sx = doSplineX(*pMatrices, events, *pVecH, decomp, lambda); // Voir si matB est utile ???
    const std::vector<double> &vecVarG = calcul_spline_variance(*pMatrices, events, decomp, lambda); // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------

    MCMCSplineComposante splineX;
    splineX.vecThetaEvents = vecTheta;
    splineX.vecG = std::move(sx.vecG);
    splineX.vecGamma = std::move(sx.vecGamma);

    splineX.vecVarG = vecVarG;

    MCMCSpline spline;
    spline.splineX = std::move(splineX);


    if ( mComputeY) {

        // doSpline utilise les Y des events
        // => On le calcule ici pour la seconde composante (y)

        const SplineResults &sy = doSplineY(*pMatrices, events, *pVecH, decomp, lambda); //matL et matB ne sont pas changés

        MCMCSplineComposante splineY;

        splineY.vecG = std::move(sy.vecG);
        splineY.vecGamma = std::move(sy.vecGamma);

        splineY.vecThetaEvents = vecTheta;
        splineY.vecVarG = vecVarG;  // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

        spline.splineY = std::move(splineY);
    }

    if ( mComputeZ) {
        // dans le future, ne sera pas utile pour le mode sphérique
        // doSpline utilise les Z des events
        // => On le calcule ici pour la troisième composante (z)

        const SplineResults &sz = doSplineZ(*pMatrices, events, *pVecH, decomp, lambda);

        MCMCSplineComposante splineZ;

        splineZ.vecG = std::move(sz.vecG);
        splineZ.vecGamma = std::move(sz.vecGamma);

        splineZ.vecThetaEvents = vecTheta;
        splineZ.vecVarG = vecVarG;

        spline.splineZ = std::move(splineZ);
    }

    return spline;
}

/**
 * @brief MCMCLoopCurve::currentSpline_WI. Lambda = 0
 * @param events
 * @param doSortAndSpreadTheta
 * @param vecH
 * @param matrices
 * @return
 */
MCMCSpline MCMCLoopCurve::currentSpline_WI (QList<Event *> &events, bool doSortAndSpreadTheta, const std::vector<t_reduceTime> &vecH, const SplineMatrices &matrices)
{
    //Q_ASSERT_X(mModel->mLambdaSpline.mX!=0, "[MCMCLoopCurve::ln_h_YWI_3_update]", "lambdaSpline=0");

    std::vector<t_reduceTime> _VecH;
    const SplineMatrices* pMatrices;

    SplineMatrices rMatrices;
    SplineMatrices lmatrices;
    if (doSortAndSpreadTheta) {
        orderEventsByThetaReduced(events);
        spreadEventsThetaReduced0(events);

        const std::vector<t_reduceTime> &rVecH = calculVecH(events);
        _VecH = rVecH;

        rMatrices = prepareCalculSpline_WI(events, rVecH);
        pMatrices = std::move(&rMatrices);

    } else {
        _VecH = vecH;
        pMatrices = &matrices;
    }

    const std::vector<double> &rVecTheta = getThetaEventVector(events);

    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    const Matrix2D &matB  = pMatrices->matR;

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(matB, 5, 1);


    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédent ne change pas

    std::vector<double> vecVarG;
    if (mCurveSettings.mUseErrMesure)
        vecVarG = calcul_spline_variance(*pMatrices, events, decomp, 0.); // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =
    else
        vecVarG = std::vector(events.size(), 0.);
    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------

    const SplineResults &sx = doSplineX(*pMatrices, events, _VecH, decomp, 0.);

    MCMCSplineComposante splineX;
    splineX.vecThetaEvents = rVecTheta;
    splineX.vecG = std::move(sx.vecG);
    splineX.vecGamma = std::move(sx.vecGamma);

    splineX.vecVarG = vecVarG;

    MCMCSpline spline;
    spline.splineX = std::move(splineX);


    if ( mComputeY) {

        const SplineResults &sy = doSplineY(*pMatrices, events, _VecH, decomp, 0.); //matL et matB ne sont pas changés

        MCMCSplineComposante splineY;

        splineY.vecG = std::move(sy.vecG);
        splineY.vecGamma = std::move(sy.vecGamma);

        splineY.vecThetaEvents = rVecTheta;
        splineY.vecVarG = vecVarG;  // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

        spline.splineY = std::move(splineY);
    }

    if ( mComputeZ) {
        // dans le future, ne sera pas utile pour le mode sphérique
        // doSpline utilise les Z des events
        // => On le calcule ici pour la troisième composante (z)

        const SplineResults &sz = doSplineZ(*pMatrices, events, _VecH, decomp, 0.);

        MCMCSplineComposante splineZ;

        splineZ.vecG = std::move(sz.vecG);
        splineZ.vecGamma = std::move(sz.vecGamma);

        splineZ.vecThetaEvents = rVecTheta;
        splineZ.vecVarG = vecVarG;

        spline.splineZ = std::move(splineZ);
    }

    return spline;
}



bool  MCMCLoopCurve::hasPositiveGPrimeByDet (const MCMCSplineComposante &splineComposante)
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

        if (t1_res > t2_res)
            std::swap(t1_res, t2_res);

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
    const double dY = - dy_threshold * (mModel->mSettings.mTmax - mModel->mSettings.mTmin); // On doit inverser le signe et passer en temps réduit

    decltype(splineComposante.vecThetaEvents)::const_iterator iVecThetaEvents = splineComposante.vecThetaEvents.cbegin();
    decltype(splineComposante.vecGamma)::const_iterator iVecGamma = splineComposante.vecGamma.cbegin();
    decltype(splineComposante.vecG)::const_iterator iVecG = splineComposante.vecG.cbegin();

   // Calcul dérivé avant les thetas
    const t_reduceTime t0 = mModel->reduceTime(splineComposante.vecThetaEvents.at(0));
    const t_reduceTime t1 = mModel->reduceTime(splineComposante.vecThetaEvents.at(1));
    double gPrime = (splineComposante.vecG.at(1) - splineComposante.vecG.at(0)) / (t1 - t0);
    gPrime -= (t1 - t0) * splineComposante.vecGamma.at(1) / 6.;
    if (gPrime < dy_threshold)
        return false;
     // Calcul dérivé avant les thetas

    const size_t n = splineComposante.vecThetaEvents.size();
    const t_reduceTime tn = mModel->reduceTime(splineComposante.vecThetaEvents.at(n-1));
    const t_reduceTime tn_1 = mModel->reduceTime(splineComposante.vecThetaEvents.at(n-2));
    gPrime = (splineComposante.vecG.at(n-1) - splineComposante.vecG.at(n-2)) / (tn - tn_1);
    gPrime += (tn - tn_1) * splineComposante.vecGamma.at(n-2) / 6.;
    if (gPrime < dy_threshold)
        return false;

//
    for (unsigned long i= 0; i< splineComposante.vecThetaEvents.size()-1; i++) {

        const t_reduceTime t_i = mModel->reduceTime(*iVecThetaEvents);
        ++iVecThetaEvents;
        const t_reduceTime t_i1 = mModel->reduceTime(*iVecThetaEvents);

        const t_reduceTime hi = t_i1 - t_i;

        const double gamma_i = *iVecGamma;
        ++iVecGamma;
        const double gamma_i1 = *iVecGamma;

        const double g_i = *iVecG;
        ++iVecG;
        const double g_i1 = *iVecG;

        const double a = (g_i1 - g_i) /hi;
        const double b = (gamma_i1 - gamma_i) /(6*hi);
        const double s = t_i + t_i1;
        const double p = t_i * t_i1;
        const double d = ( (t_i1 - 2*t_i)*gamma_i1 + (2*t_i1 - t_i)*gamma_i ) / (6*hi);

        // résolution équation

        const double aDelta = 3* b;
        const double bDelta = 2*d - 2*s*b;
        const double cDelta = p*b - s*d + a;

        /* to DEBUG
           double yVertex, yVertex_new, Gpi, Gpi1;
           yVertex =  - pow(bDelta, 2.)/ (4.*aDelta) + cDelta ;
           Gpi = aDelta*pow(t_i, 2.) + bDelta*t_i + cDelta;
           Gpi1 = aDelta*pow(t_i1, 2.) + bDelta*t_i1 + cDelta;
           yVertex_new =  - pow(bDelta, 2.)/ (4.*aDelta) + cDelta+dY ;
        */
       // const double t0_res = -bDelta / (2.*aDelta);

        double delta = pow(bDelta, 2.) - 4*aDelta*(cDelta + dY);


        if (delta <= 0) { // No solution; la courbe est toujours positive ou négative
            if (aDelta < 0) // convexe
                return false;
            else           // concave
                continue;
        }


        double t1_res = (-bDelta - sqrt(delta)) / (2.*aDelta);
        double t2_res = (-bDelta + sqrt(delta)) / (2.*aDelta);


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

#pragma mark import_Komlan



double MCMCLoopCurve::rate_h_lambda_K(const MCMCSpline &s, const double current_lambda, const double try_lambda, const Matrix2D &K)
{
    const int mu = 3;
    const int n = K.size();
    double som = 0;
    double vm = 0.;

    auto fK = multiMatByVectCol0_KK(K, s.splineX.vecG);
    // produit fK*ft
    std::vector<double> fKft;
    for (int i = 0; i < n; ++i) {
        const int u = Signe_Number(fK[i]) * Signe_Number(s.splineX.vecG[i]) ;
        fKft.push_back(u * pow(n, log_p(abs(fK[i]), n) + log_p(abs(s.splineX.vecG[i]), n)));
    }

    som = std::accumulate(fKft.begin(), fKft.end(), 0.);

    for (int i = 0; i < n; ++i) {
        vm += K[i][i];
    }

    const double c = (n - 2) / vm;

    const double prod = exp(-0.5 * (try_lambda - current_lambda )*som);
    const double prod2 =  exp(0.5*(n - 2)*log(try_lambda / current_lambda))   ;//pow(try_lambda / current_lambda, 0.5*(n - 2)) ;
    const double prod3 = exp((mu + 1)*log((c + current_lambda) / (c + try_lambda)))  ; // pow((c + current_lambda) / (c + try_lambda), mu+1);

    const double produit = prod * prod2 * prod3 * (try_lambda / current_lambda) ;

    return produit;
}


double MCMCLoopCurve::S02_lambda_WIK (const Matrix2D &K, const int nb_noeuds)
{

    double vm = 0.;
    for (int i = 0; i < nb_noeuds; ++i) {
        vm += K[i][i];
    }

    const double S02_lambda = (nb_noeuds - 2) / vm;

    return S02_lambda;
}

double MCMCLoopCurve::h_lambda_Komlan(const Matrix2D &K, const Matrix2D &K_new, const int nb_noeuds, const double &lambdaSpline)
{
    /* initialisation de l'exposant mu du prior "shrinkage" sur lambda : fixe
     en posant mu=2, la moyenne a priori sur alpha est finie = (nb_noeuds-2)/somme(Mat_W_1K[i,i]) ;
     et la variance a priori sur lambda est infinie
     NB : si on veut un shrinkage avec espérance et variance finies, il faut mu >= 3
    */

    const int mu = 3;
    const double c = S02_lambda_WIK(K, nb_noeuds);

    const double c_new = S02_lambda_WIK(K_new, nb_noeuds);

    // prior "shrinkage"

    const double prior = exp(mu*log(c_new / c) + (mu + 1)*log((c + lambdaSpline) / (c_new + lambdaSpline))) ;

    return prior;
}

MatrixDiag MCMCLoopCurve::createDiagWInv_Vg0(const QList<Event*>& lEvents)
{
    MatrixDiag diagWInv (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), diagWInv.begin(), [](Event* ev) {return pow(ev->mSy, 2.);});

    return diagWInv;
}


SplineMatrices MCMCLoopCurve::prepareCalculSpline_W_Vg0(const QList<Event *>& sortedEvents, std::vector<double>& vecH)
{
    Matrix2D matR = calculMatR(vecH);
    Matrix2D matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    Matrix2D matQT = transpose(matQ, 3);

    // Diag Winv est égale à la diagonale des variances
    MatrixDiag diagWInv (sortedEvents.size());
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


MCMCSpline MCMCLoopCurve::samplingSpline_multi(QList<Event *> &lEvents, const Matrix2D &RR_1, const Matrix2D &Q, std::vector<double> &vecfx,  bool doSortAndSpreadTheta, SplineMatrices matrices)
{
    MCMCSpline spline;

    // ---------------

    // Calcul de gamma
    std::vector<double> vecTheta = getThetaEventVector(lEvents);

    // auto R_1 = inverseMatSym0(seedMatrix(matrices.matR, 1));

    /* Matrix2D QT (matrices.matQT.size() - 2);
    auto itQT = begin(QT);
    for ( auto it = begin(matrices.matQT) + 1; it != end(matrices.matQT) - 1; ++it) {
        *itQT =  (*it);
        ++itQT;
    }*/

    const Matrix2D &QT = transpose0(Q);

    auto QTfx = multiMatByVectCol0_KK(QT, vecfx) ;


    // const Matrix2D &RR = seedMatrix(R, 1); // dim n-2 * n-2

    //const Matrix2D  &R_1 = inverseMatSym0(RR, 0);

    std::vector<double> gamma =  multiMatByVectCol0_KK(RR_1, QTfx);

    gamma.push_back(0.);
    gamma.insert(gamma.begin(), 0.);

    MCMCSplineComposante splineX;
    splineX.vecThetaEvents = vecTheta;
    splineX.vecG = std::move(vecfx);
    splineX.vecGamma = std::move(gamma);

    const int n = matrices.matQT.size() ;

    splineX.vecVarG = std::vector<double>(n, 0.0000000001) ;

    spline.splineX = std::move(splineX);

    return spline;
}




std::vector<double> MCMCLoopCurve::splines_prior(const Matrix2D &KK, std::vector<double> &g, std::vector<double> &g_new)
{

    const double lambda = mModel->mLambdaSpline.mX ;

    auto KKlambda = multiConstParMat0(KK, lambda);

    // auto kg = multiMatByVectCol0(KKlambda, g);

    // auto  kg_new = multiMatByVectCol0(KKlambda, g_new);

    int n = KK.size();

    // auto gi = g;
    std::vector<double> prod ;

    for (int i = 0; i < n; i++) {
        auto gi = g;
        const double a = g_new[i];
        gi[i] = a;

        std::vector<double> g_new_g ;

        std::vector<double> gg ;

        for (int j = 0; j < n; j++) {
            g_new_g.push_back(gi[j] - g[j]);

            gg.push_back(gi[j] + g[j]);
        }

        // auto  difK = multiMatByVectCol0(KKlambda, gg);

        std::vector<double> PP ;

        for(int k = 0; k < n; k++){

            // PP.push_back(g_new_g[k]*difK[k]) ;

            int u = Signe_Number(KKlambda[i][k]) * Signe_Number(g_new_g[i]) * Signe_Number(gg[k]);

            PP.push_back(u * exp(log(abs(KKlambda[i][k])) + log(abs(g_new_g[i])) + log(abs(gg[k])))) ;
        }

        const double som = std::accumulate(PP.begin(), PP.end(), 0.);

        prod.push_back(exp(-0.5*som));
    }

    return prod;
}

double MCMCLoopCurve::Signe_Number(const double &a)
{
    int x = 0;

    if (a < 0) {
        x = -1 ;
    } else {
        x = 1 ;
    }

    return x;
}

double MCMCLoopCurve::Prior_F (const Matrix2D& K, const Matrix2D& K_new, const MCMCSpline &s,  const double lambdaSpline)
{

    const int n = K.size();

    auto K1 = multiConstParMat0(K, -1.);

    auto KK = addMatEtMat0(K_new, K1) ;

    auto fK = multiMatByVectCol0_KK(multiConstParMat0(KK, lambdaSpline), s.splineX.vecG);

    // produit fK*ft
    std::vector<double> fKft;
    for (int i = 0; i < n; ++i) {
        int u = Signe_Number(fK[i]) * Signe_Number(s.splineX.vecG[i]) ;

        fKft.push_back(u * pow(n, log_p(abs(fK[i]), n) + log_p(abs(s.splineX.vecG[i]), n)));
    }

    const double som = std::accumulate(fKft.begin(), fKft.end(), 0.);

    const double d = exp(-0.5 * som);

    return d;
}


std::vector<double> MCMCLoopCurve::multiMatByVectCol0_KK(const  Matrix2D &KKK, const std::vector<double> &gg)
{
    const int nl1 = KKK.size();
    // const int nc1 = matrix[0].size();
    const int nc2 = gg.size();

    // nc1 doit etre égal à nc2
    std::vector<double> result = initVector(nl1);
    // const double* itMat1;
    double sum;

    for (int i = 0; i < nl1; ++i) {
        // itMat1 = begin(KKK[i]);

        sum = 0;
        for (int k = 0; k < nc2; ++k) {
            const int u = Signe_Number(KKK[i][k]) * Signe_Number(gg[k]) ;

            sum += u * pow(nl1, log_p(abs(KKK[i][k]), nl1) + log_p(abs(gg[k]), nl1)) ; //(*(itMat1 + k)) * gg[k];
        }

        result[i] = sum;
    }
    return result;
}


double MCMCLoopCurve::log_p(const double &x, const int &n)
{

    const double d = log(x) / log(n) ;

    return d;
}

double MCMCLoopCurve::h_exp_fX_theta (Event* e, const MCMCSpline &s, unsigned idx)
{
    const double h = -0.5 * (e->mW * pow( e->mYx - s.splineX.vecG[idx] ,2.));
    return exp(h);
}

std::vector<double> MCMCLoopCurve::sampling_spline (QList<Event *> &lEvents, SplineMatrices matrices)
{
    int n = matrices.diagWInv.size();
    /*  std::vector<double> W; // matrice diagonale
    for (auto &w_1 : matrices.diagWInv ) {
        W.push_back(w_1);
    }*/

    std::vector< double> vecYx (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), vecYx.begin(), [](Event* ev) {return ev->mYx;});

    std::vector<double> X;
    for(int i = 0; i < n; i++){
        X.push_back(Generator::gaussByBoxMuller(vecYx[i], sqrt(matrices.diagWInv[i])));
    }

    return X;
}

double MCMCLoopCurve::h_S02_Vg_K(const QList<Event *> events, double S02_Vg, double try_Vg)
{
    const int alp = 1; //var_Y/(var_Y-1.);

    const double prior = exp((alp + 1)*log(S02_Vg / try_Vg)) * exp(-((S02_Vg - try_Vg) / (try_Vg*S02_Vg)))   ;// pow((S02_Vg / try_Vg), alp + 1) * exp(-((S02_Vg - try_Vg) / (try_Vg*S02_Vg)));

    const int a = 1;
    double pp ;
    if (mCurveSettings.mUseVarianceIndividual) {
        double prod_h_Vg = 1.;
        for (auto& e : events) {

            //  prod_h_Vg *= pow(S02_Vg/(S02_Vg + e->mVG.mX), a+1) / S02_Vg;

            prod_h_Vg *= (exp((a + 1)*log((S02_Vg + e->mVg.mX) / (try_Vg + e->mVg.mX))) * (try_Vg / S02_Vg)) ;// (pow(((S02_Vg + e->mVG.mX) / (try_Vg + e->mVG.mX)), a + 1) * (try_Vg / S02_Vg));
        }

        const double pp = (prior * prod_h_Vg);

        return pp;

    } else {

        const double   prod_h_Vg = (exp((a + 1)*log((S02_Vg + events[0]->mVg.mX) / (try_Vg + events[0]->mVg.mX))) * (try_Vg / S02_Vg));

        pp = prior * prod_h_Vg;

        return pp;
    }

}
