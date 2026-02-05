/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2026

Authors :
    Philippe LANOS
    Helori LANOS
    Philippe DUFRESNE
    Komlan NOUKPOAPE

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

#include "AxisTool.h"
#include "CalibrationCurve.h"
#include "ModelCurve.h"
#include "CurveUtilities.h"
#include "Functions.h"
#include "Generator.h"
#include "StateKeys.h"
#include "Date.h"
#include "Project.h"
#include "Matrix.h"
#include "MainWindow.h"
#include "QtUtilities.h"

#include <iostream>
#include <memory>


#ifdef DEBUG
#include "QtUtilities.h"
#endif

#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QTime>
#include <QProgressDialog>
#include <QElapsedTimer>

#include <exception>
#include <vector>
#include <cmath>
#include <time.h>


#ifdef _WIN32
#include <QtWidgets>
//#include <winbase.h>
#include <windows.h> //for Qt 6.7
#endif

MCMCLoopCurve::MCMCLoopCurve(std::shared_ptr<ModelCurve> model):
    MCMCLoop(model)
{
    if (mModel) {
        setMCMCSettings(mModel->mMCMCSettings);
    }
    
    mCurveSettings = CurveSettings::fromJson(getState_ptr()->value(STATE_CURVE).toObject());
}

MCMCLoopCurve::~MCMCLoopCurve()
{
   qDebug()<<"[MCMCLoopCurve::~MCMCLoopCurve()]";
}

#pragma mark MCMC Loop Overloads

/**
 * Idem Chronomodel + prepareEventsY() qui sert à corriger les données d'entrées de Curve.
 * (Calcul de Yx, Yy, Yz et de Sy)
 */
QString MCMCLoopCurve::calibrate()
{
    if (mModel) {
        std::vector<std::shared_ptr<Event>>& events = mModel->mEvents;
        //events.reserve(mModel->mEvents.size());
        
        //----------------- Calibrate measurements --------------------------------------

        QList<Date*> dates;
        // find number of dates, to optimize memory space
        /*int nbDates = 0;
          for (auto&& ev : events)
            nbDates += ev->mDates.size();

          dates.reserve(nbDates);
        */
        for (auto&& ev : events) {
            size_t num_dates = ev->mDates.size();
            for (size_t j = 0; j<num_dates; ++j) {
                Date* date = &ev->mDates[j];
                dates.push_back(date);
            }
        }
        

        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepChanged(tr("Calibrating..."), 0, (int)dates.size());

        int i = 0;
        for (auto&& date : dates) {
            if (date->mCalibration) {
                if (date->mCalibration->mVector.empty())
                    date->calibrate(getProject_ptr());

                if (date->mCalibration->mVector.size() < 6) {
                    const double new_step = date->mCalibration->mStep /5.;
                    date->mCalibration->mVector.clear();
                    date->mCalibration->mMap.clear();
                    date->mCalibration->mRepartition.clear();
                    date->mCalibration = nullptr;

                    const QString mes = tr("Insufficient resolution for the Event %1 \r Decrease the step in the study period box to %2").arg(date->getQStringName(), QString::number(new_step));
                    return (mes);

                }


            } else
                return (tr("Invalid Model -> No Calibration on Data %1").arg(date->getQStringName()));


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
 * Idem MCMCLoopChrono + initialisation de VG (events) et mLambdaSpline (global)
 */

QString MCMCLoopCurve::initialize()
{
    const QString initTime = initialize_time();
    if (initTime != QString())
        return initTime;

    if (mCurveSettings.mLambdaSplineType == CurveSettings::eInterpolation)
        return initialize_interpolate();

    else // changer S02_BAYESIAN dans Event.h

#if VERSION_MAJOR == 3 && VERSION_MINOR == 2 && VERSION_PATCH == 1
    return initialize_321();

#elif VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH < 5
    return initialize_330();

#elif VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH >= 5
        return initialize_335();

#endif

#ifdef old_S02_BAYESIAN
        //return initialize_400();
        return initialize_401(); // experimental
        #define notCODE_KOMLAN
        //return initialize_Komlan();
#endif


}

bool MCMCLoopCurve::update()
{
    return   (this->*updateLoop)();

}

#if VERSION_MAJOR == 3 && VERSION_MINOR == 2 && VERSION_PATCH == 1
#pragma mark Version 3.2.1
QString MCMCLoopCurve::initialize_321()
{
#ifdef DEBUG
    std::cout << "[MCMCLoopCurve::initialize_321]" << std::endl;
#endif
    updateLoop = &MCMCLoopCurve::update_321;

    std::vector<std::shared_ptr<Event>> &allEvents (mModel->mEvents);

    //if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
    //    mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    //if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (std::shared_ptr<Event> &ev : allEvents) {
            if (mModel->is_curve && ev->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                ev->mTheta.mSamplerProposal = MHVariable::eMHAdaptGauss;
            }
            if (ev->mPointType == Event::eNode)
                mNodeEvent.push_back(ev);
            else
                mPointEvent.push_back(ev);

            ev->mS02Theta.mSamplerProposal = MHVariable::eFixe; // not yet integrate within update_321
        }
    } else {
        for (const std::shared_ptr<Event> &ev : allEvents) {
            if (mModel->is_curve && ev->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                ev->mTheta.mSamplerProposal = MHVariable::eMHAdaptGauss;
            }
            mPointEvent.push_back(ev);

        }
    }


    // -------------------------------- SPLINE part--------------------------------
    // Init function G

    prepareEventsY(allEvents);

    emit stepChanged(tr("Initializing G ..."), 0, (int)allEvents.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    current_vecH = calculVecH(mModel->mEvents);
    // ----------------------------------------------------------------
    //  Init Lambda Spline
    // ----------------------------------------------------------------

    SplineMatricesLD matricesWI = prepare_calcul_spline_WI(current_vecH);
    try {
        if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed) {
            mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;
            mModel->mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
            double memoLambda = log10(mModel->mLambdaSpline.mX);
            mModel->mLambdaSpline.memo(&memoLambda);

        } else {
            if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth)
                mModel->mLambdaSpline.mX = 1.;
            else
                mModel->mLambdaSpline.mX = 1.E-6; // default = 1E-6.
            mModel->mLambdaSpline.mLastAccepts.clear();
            mModel->mLambdaSpline.accept_update(mModel->mLambdaSpline.mX);

        }
        mModel->mLambdaSpline.mSigmaMH = 1.; // default = 1.

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
            std::for_each( mModel->mEvents.begin(), mModel->mEvents.end(), [](std::shared_ptr<Event> e) { e->mW = 1.; });
            const auto var_residu_X = S02_Vg_Yx(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);
            //std::cout<<" var_residu_X = " << var_residu_X;
            if (mModel->compute_X_only) {
                Var_residual_spline = var_residu_X;

            } else {
                const auto var_residu_Y = S02_Vg_Yy(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);

                if (mModel->compute_XYZ) {
                    const auto var_residu_Z =  S02_Vg_Yz(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);
                    Var_residual_spline = (var_residu_X + var_residu_Y + var_residu_Z)/3.;

                } else {
                    Var_residual_spline = (var_residu_X + var_residu_Y)/2.;
                }

            }


        }

        // memo Vg
        /* ----------------------------------------------------------------
         * The W of the events depend only on their VG
         * During the update, we need W for the calculations of theta, VG and Lambda Spline update
         * We will thus have to update the W at each VG modification
         * We calculate it here during the initialization to have its starting value
         * ---------------------------------------------------------------- */

        int i = 0;
        if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
            for (std::shared_ptr<Event> &e : mPointEvent) {
                i++;
                e->mVg.mX = Var_residual_spline;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain


                if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                } else {
                    e->mVg.accept_update(e->mVg.mX);
                }
                e->updateW();
                e->mVg.mSigmaMH = 1.;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }

            for (std::shared_ptr<Event> &e : mNodeEvent) {
                i++;
                e->mVg.mX = 0.;
                e->mVg.mSamplerProposal = MHVariable::eFixe;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                // check if Sy == 0
                if (e->mSy == 0) {
                    mAbortedReason = QString("Error: a Node cannot have a null error \n Change error in : %1").arg(e->getQStringName());
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
            // No global zero variance if no measurement error
            if (! mCurveSettings.mUseErrMesure && mCurveSettings.mVarianceType == CurveSettings::eModeFixed && mCurveSettings.mVarianceFixed == 0.) {
                mAbortedReason = QString("Warning: If no measurement error, global error std gi cannot be zero");
                return mAbortedReason;
            }
            // Pas de Noeud dans le cas de Vg Global
            for (std::shared_ptr<Event> &e : allEvents) {
                i++;
                if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
                    e->mVg.mX = mCurveSettings.mVarianceFixed;
                    e->mVg.mSamplerProposal = MHVariable::eFixe;
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                    // Check if Sy + Vg == 0
                    if (e->mVg.mX + e->mSy * e->mSy == 0) {
                        mAbortedReason = QString("Error: a Node cannot have a null error with Variance null \n Change error in : %1").arg(e->getQStringName());
                        return mAbortedReason;
                    }

                } else {
                    e->mPointType = Event::ePoint; // force Node to be a simple Event
                    e->mVg.mX = Var_residual_spline;
                    e->mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;
                }
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                e->mVg.accept_update(e->mVg.mX);
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
    /*mModel->mS02Vg.mX = Var_residual_spline;
    mModel->mS02Vg.mLastAccepts.clear();
    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
        mModel->mS02Vg.mSamplerProposal = MHVariable::eFixe;
        double memoS02 = sqrt(mModel->mS02Vg.mX);
        mModel->mS02Vg.memo(&memoS02);

    } else {
        mModel->mS02Vg.tryUpdate(Var_residual_spline, 2.);
    }

    mModel->mS02Vg.mSigmaMH = 1.;
    */
    // ----------------------------------------------------------------
    // Curve init S02 Vg -- loi gamma
    // ----------------------------------------------------------------
    //mModel->mS02Vg.mX = Var_residual_spline;
    mModel->mS02Vg.mLastAccepts.clear();
    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
        mModel->mS02Vg.mSamplerProposal = MHVariable::eFixe;
        double memoS02 = sqrt(mModel->mS02Vg.mX);
        mModel->mS02Vg.memo(&memoS02);

    } else {
        const double s_harmonique = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0., [](double s0, auto e) { return s0 + 1./pow(e->mSy, 2.);});
        const double mean_harmonique = sqrt((double)mModel->mEvents.size()/ s_harmonique);
        const double beta = 1.004680139*(1 - exp(- 0.0000847244 * pow(mean_harmonique, 2.373548593)));

        mModel->mSO2_beta = std::max(1E-10, beta);
        // il y a une erreur de formule, il faut mettre :
        // const double S02Vg = 1. / Generator::gammaDistribution(1., 1/mModel->mSO2_beta); Vu le 2025/02/11

        const double S02Vg = 1.0 / Generator::gammaDistribution(1., mModel->mSO2_beta);
#ifdef DEBUG
        if (S02Vg == INFINITY)
            qDebug()<<"[MCMCLoopCurve::initialize_321]  S02Vg == INFINITY";
#endif
        mModel->mS02Vg.accept_update(S02Vg);
    }

    mModel->mS02Vg.mSigmaMH = 1.;



    // ___________________________
    if (mModel->compute_X_only) {
        std::vector<double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

        var_Y = variance_Knuth( vecY);

    } else if (mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec) { // à controler
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecY);

        var_Y /= 2.;

    } else {
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYz;});
        var_Y += variance_Knuth( vecY);

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

        current_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);
        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

        // init Posterior MeanG and map
        const int nbPoint = 300;// Density curve size and curve size

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapG.min_value = +INFINITY;
        clearCompo.mapG.max_value = 0;

        clearCompo.mapGP = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapGP.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapGP.min_value = +INFINITY;
        clearCompo.mapGP.max_value = 0;

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
        /*minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYx, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYx, x);});

        int i = 0;
        for (auto g : mModel->mSpline.splineX.vecG) { // use lambda init
            const auto e = 1.96*sqrt(mModel->mSpline.splineX.vecVarG.at(i));

            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }
        */
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYx - e->mSy, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYx + e->mSy, x);});

        const auto e = 0.1 *(maxY -minY);

        minY = minY - e;
        maxY = maxY + e;

        Scale sc ;
        sc.findOptimal(minY, maxY);

        clearMeanG.gx.mapG.setRangeY(sc.min, sc.max);

        if (mModel->compute_Y) {
            clearMeanG.gy = clearCompo;

            minY = +INFINITY;
            maxY = -INFINITY;

            minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double y, std::shared_ptr<Event> e) {return std::min(e->mYy, y);});
            maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double y, std::shared_ptr<Event> e) {return std::max(e->mYy, y);});

            /*int i = 0;
            for (auto g : mModel->mSpline.splineY.vecG) {
                const auto e = 1.96*sqrt(mModel->mSpline.splineY.vecVarG.at(i));
                minY = std::min(minY, g - e);
                maxY = std::max(maxY, g + e);
                i++;
            }
            */
            const auto e = 0.1 *(maxY -minY);

            minY = minY - e;
            maxY = maxY + e;

            Scale sc ;
            sc.findOptimal(minY, maxY);

            clearMeanG.gy.mapG.setRangeY(sc.min, sc.max);

            if (mModel->compute_XYZ) {
                clearMeanG.gz = clearCompo;

                minY = +INFINITY;
                maxY = -INFINITY;

                minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double z, std::shared_ptr<Event> e) {return std::min(e->mYz, z);});
                maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double z, std::shared_ptr<Event> e) {return std::max(e->mYz, z);});

                /*int i = 0;
                for (auto g : mModel->mSpline.splineZ.vecG) {
                    const auto e = 1.96*sqrt(mModel->mSpline.splineZ.vecVarG.at(i));
                    minY = std::min(minY, g - e);
                    maxY = std::max(maxY, g + e);
                    i++;
                }*/

                const auto e = 0.1 *(maxY -minY);

                minY = minY - e;
                maxY = maxY + e;

                Scale sc ;
                sc.findOptimal(minY, maxY);

                clearMeanG.gz.mapG.setRangeY(sc.min, sc.max);
            }

        }

        // Convertion IDF
        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Vector ||  mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Spherical) {

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

// MCMC Reinsch
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

        current_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);
        current_decomp_QTQ = decompositionCholesky(current_splineMatricesLD.matQTQ, 5, 1); // used only with update Theta
        //auto current_decomp_2 = choleskyLDLT_Dsup0(current_splineMatricesLD.matQTQ, 5, 1);


        current_decomp_matB = decomp_matB(current_splineMatricesLD, mModel->mLambdaSpline.mX);

        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH
        current_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. :
                                 ln_h_YWI_3_update(current_splineMatricesLD, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);

        if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
            current_h_lambda = 1;
        else
            current_h_lambda = h_lambda_321(current_splineMatricesLD,  (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX) ;


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
                            date.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);
                        }
                    }
                }

            } catch(std::exception& exc) {
                qWarning() << "[MCMCLoopCurve::update_321] Ti : Caught Exception!\n"<<exc.what();

            }  catch (...) {
                qWarning() << "[MCMCLoopCurve::update_321] Ti : Caught Exception!";
            }


            /* --------------------------------------------------------------
             *  B - Update Theta Events
             * -------------------------------------------------------------- */
            try {
                for (std::shared_ptr<Event> event : initListEvents) {
#ifdef _WIN32
    SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
#endif
                    if (event->mType == Event::eDefault) {
                        const double min = event->getThetaMin(tminPeriod);
                        const double max = event->getThetaMax(tmaxPeriod);

                        if (min >= max) {
                            const auto seed = mLoopChains.at(mChainIndex).mSeed;
                            throw QObject::tr("[MCMCLoopCurve::update_321] Error for event theta : %1 :\n min = %2 : max = %3 \n seed = %4").arg(event->getQStringName(), QString::number(min), QString::number(max), QString::number(seed));
                        }

                        // On stocke l'ancienne valeur
                        const double current_value = event->mTheta.mX;
                        current_h_theta = h_theta_Event(event);

                        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH

                        // On tire une nouvelle valeur :
                        const double try_value = Generator::normalDistribution(current_value, event->mTheta.mSigmaMH);

                        if (try_value >= min && try_value <= max) {
                            // On force la mise à jour de la nouvelle valeur pour calculer h_new

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);

                            orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                            spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date

                            try_vecH = calculVecH(mModel->mEvents);

                            try_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, try_vecH);

                            try_decomp_QTQ = decompositionCholesky(try_splineMatricesLD.matQTQ, 5, 1);
                            try_decomp_matB = decomp_matB(try_splineMatricesLD, mModel->mLambdaSpline.mX);

                            try_ln_h_YWI_1_2 = ln_h_YWI_1_2(try_decomp_QTQ, try_decomp_matB);
                            try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatricesLD, mModel->mEvents, try_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                            if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
                                try_h_lambda = 1;
                            else
                                try_h_lambda = h_lambda_321( try_splineMatricesLD, (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX);

                            try_h_theta = h_theta_Event(event);

                            rate = (try_h_lambda* try_h_theta) / (current_h_lambda* current_h_theta) * exp(0.5 * ( try_ln_h_YWI_1_2 + try_ln_h_YWI_3
                                                                                                                     - current_ln_h_YWI_1_2 - current_ln_h_YWI_3));


                        } else {
                            rate = -1.;

                        }

                        // restore Theta to used function tryUpdate
                        event->mTheta.mX = current_value;
                        event->mTheta.try_update(try_value, rate);


                        if ( event->mTheta.accept_buffer_full()) {
                            // Pour l'itération suivante :
                            std::swap(current_ln_h_YWI_1_2, try_ln_h_YWI_1_2);
                            std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);

                            std::swap(current_vecH, try_vecH);
                            std::swap(current_splineMatricesLD, try_splineMatricesLD);
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
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});

                } // End of loop initListEvents

                //  Update Phases Tau; they coud be used by the Event in the other Phase ----------------------------------------
                /* --------------------------------------------------------------
                 *  C.2 - Update Tau Phases
                 * -------------------------------------------------------------- */
                std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_Tau (tminPeriod, tmaxPeriod);});

                /* --------------------------------------------------------------
                 *  C.3 - Update Gamma Phases
                 * -------------------------------------------------------------- */
                std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (std::shared_ptr<PhaseConstraint> pc) {pc->updateGamma();});

            } catch(std::exception& exc) {
                qWarning() << "[MCMCLoopCurve::update_321] Theta : Caught Exception!\n"<<exc.what();
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
         *  D - Update S02 Vg / Variance Param.
         *  E.1 - Update Vg for Points only
         *  E.2 - Update Vg Global
         * -------------------------------------------------------------- */
        try {
            const double logMin = -10.;
            const double logMax = +20.;

            /* --------------------------------------------------------------
            *  D - Update S02 Vg, à évaluer dans les deux cas: variance individuelle et globale
            * -------------------------------------------------------------- */
            /*try {
                if (mCurveSettings.mVarianceType != CurveSettings::eModeFixed ) {
                    const double try_value_log = Generator::normalDistribution(log10(mModel->mS02Vg.mX), mModel->mS02Vg.mSigmaMH);
                    const double try_value = pow(10, try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {

                        const t_prob jacobianRate = (t_prob)try_value / mModel->mS02Vg.mX;
                        rate = rate_h_S02_Vg(mPointEvent, mModel->mS02Vg.mX, try_value) * jacobianRate;

                    } else {
                        rate = -1.;
                    }

                    mModel->mS02Vg.tryUpdate(try_value, rate);
                }

            } catch (std::exception& e) {
                qWarning()<< "[MCMCLoopCurve::update_321] S02 Vg : exception caught: " << e.what() << '\n';

            }
            */
            // --------------------------------------------------------------
            //  D-1 - Update S02 Vg, loi gamma
            // --------------------------------------------------------------
            try {
                // On stocke l'ancienne valeur :
                const double current_value = mModel->mS02Vg.mX;

                // On tire une nouvelle valeur :

                const double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mS02Vg.mSigmaMH);

                const double try_value = pow(10., try_value_log);

                //long double rapport = -1.;

                if (try_value_log >= logMin && try_value_log <= logMax) {

                    const double rapport1 = h_S02_Vg_K(mModel->mEvents, current_value, try_value);

                    rate = rapport1 * (try_value / current_value) ;
                } else {
                    rate = -1.;
                }

                mModel->mS02Vg.mX = current_value;
                mModel->mS02Vg.try_update(try_value, rate);

            } catch (std::exception& e) {
                qWarning()<< "[MCMCLoopCurve::update_321] S02 Vg : exception caught: " << e.what() << '\n';

            }

            // Fin maj SO2 Vg

            // ____________________

            if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

                current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB); // Has not been initialized yet

                /* --------------------------------------------------------------
                *  E.1 - Update Vg for Points only, not the node
                * -------------------------------------------------------------- */

                for (std::shared_ptr<Event>& event : mPointEvent)   {

                    const double current_value = event->mVg.mX;
                    current_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                    // On tire une nouvelle valeur :
                    const double try_value_log = Generator::normalDistribution(log10(current_value), event->mVg.mSigmaMH);
                    const double try_value = pow(10., try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {
                        // On force la mise à jour de la nouvelle valeur pour calculer try_h
                        // A chaque fois qu'on modifie VG, W change !
                        event->mVg.mX = try_value;
                        event->updateW(); // used by prepareCalculSpline

                        // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG

                        try_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);
                        try_decomp_matB = decomp_matB(try_splineMatricesLD, mModel->mLambdaSpline.mX);

                        //try_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. :
                        try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatricesLD, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);
                        try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);
                        try_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                        rate = (try_h_VG * try_value) / (current_h_VG * current_value) * exp(0.5 * ( try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                    - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                    } else {
                        rate = -1.; // force reject // force to keep current state

                    }

                    // Metropolis Hastings update
                    // A chaque fois qu'on modifie VG, W change !
                    event->mVg.mX = current_value;
                    event->mVg.try_update( try_value, rate);
                    event->updateW();

                    if ( event->mVg.accept_buffer_full()) {
                        // Pour l'itération suivante : Car mVg a changé
                        std::swap(current_ln_h_YWI_2, try_ln_h_YWI_2);
                        std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);
                        std::swap(current_splineMatricesLD, try_splineMatricesLD);
                        std::swap(current_decomp_matB, try_decomp_matB);
                    }
                }


            } else if (mCurveSettings.mVarianceType == CurveSettings::eModeGlobal) {

                /* --------------------------------------------------------------
                *  E.2 - Update Vg Global
                * -------------------------------------------------------------- */

                auto& eventVGglobal = mModel->mEvents.at(0);

                // On stocke l'ancienne valeur :
                const double current_value = eventVGglobal->mVg.mX;

                current_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                // On tire une nouvelle valeur :

                const double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mEvents.at(0)->mVg.mSigmaMH);
                const double try_value = pow(10, try_value_log);

                // Affectation temporaire pour évaluer la nouvelle proba
                // Dans le cas global pas de différence ente les Points et les Nodes
                for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                        ev->mVg.mX = try_value;
                        ev->updateW();
                }

                if (try_value_log >= logMin && try_value_log <= logMax) {

                    // Calcul du rapport : try_matrices utilise les temps reduits, elle est affectée par le changement de Vg
                    try_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);

                    try_decomp_matB = decomp_matB(try_splineMatricesLD, mModel->mLambdaSpline.mX);

                    try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);
                    try_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. : ln_h_YWI_3_update(try_splineMatricesLD, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                    try_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                    rate = (try_h_VG * try_value) / (current_h_VG * current_value) * exp(0.5 * ( try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                    - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                    // ON fait le test avec le premier event

                    eventVGglobal->mVg.mX = current_value;
                    eventVGglobal->mVg.try_update(try_value, rate);
                    eventVGglobal->updateW();

                    if ( eventVGglobal->mVg.accept_buffer_full()) {
                        current_ln_h_YWI_2 = std::move(try_ln_h_YWI_2);
                        current_ln_h_YWI_3 = std::move(try_ln_h_YWI_3);
                        current_splineMatricesLD = std::move(try_splineMatricesLD);
                        current_decomp_matB = std::move(try_decomp_matB);
                        rate = 2.;

                    } else {
                        rate = -1.;
                    }

                } else {
                        rate = -1.;
                }

                for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                    ev->mVg.mX =  eventVGglobal->mVg.mX;
                    ev->mVg.try_update(eventVGglobal->mVg.mX, rate);
                    // On remet l'ancienne valeur, qui sera éventuellement mise à jour dans ce qui suit (Metropolis Hastings)
                    // A chaque fois qu'on modifie VG, W change !
                    ev->updateW();
                }



                // Not bayesian
            } else { // nothing to do : mCurveSettings.mVarianceType == CurveSettings::eFixed
            }
        } catch (std::exception& e) {
            qWarning()<< "[MCMCLoopCurve::update_321] VG : exception caught: " << e.what() << '\n';

        } catch(...) {
            qWarning() << "[MCMCLoopCurve::update_321] VG Event Caught Exception!\n";

        }

        /* --------------------------------------------------------------
         * F - Update Lambda
         * -------------------------------------------------------------- */
        bool ok = true;

        try {

            if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {
                if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth  ) {
                   /* const double current_value = mModel->mLambdaSpline.mX;
                    const double logMax = +10.;

                    int counter = 10; // pour test standard counter=10
                    double logMin = -20.;
                    double try_value_log ;
                    double try_value;
                    // On stocke l'ancienne valeur :

                    do {

                        // On tire une nouvelle valeur :
                        try_value_log = Generator::normalDistribution(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                        //try_value_log = Generator::normalDistribution(log10(current_value), mModel->mLambdaSpline.mSigmaMH, logMin, logMax); //nouveau code
                        try_value = pow(10., try_value_log);
                        counter++;
                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            const auto try_spline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, try_value, mModel->compute_Y, mModel->compute_XYZ);
                            ok = hasPositiveGPrimePlusConst(try_spline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

                            if (!ok)
                                logMin = try_value_log;

                        }


                    } while (!ok && counter<10);
                    logMin = -20.;
                    */

                    //-- Code origine
                    const double logMin = -20.;
                    const double logMax = +10.;

                    // On stocke l'ancienne valeur :
                    const double current_value = mModel->mLambdaSpline.mX;

                    // On tire une nouvelle valeur :
                    const double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mLambdaSpline.mSigmaMH);

                    const double try_value = pow(10., try_value_log);

                    // --

                    if (try_value_log >= logMin && try_value_log <= logMax) {

                        // Calcul du rapport :
                        mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                        try_h_lambda = h_lambda_321(current_splineMatricesLD, (int)mModel->mEvents.size(), try_value) ;
                        try_decomp_matB = decomp_matB(current_splineMatricesLD, try_value);
                        try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatricesLD, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_XYZ);
                        try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);

                        const auto n = mModel->mEvents.size();

                        rate = (try_h_lambda * try_value) / (current_h_lambda * current_value)  * exp( 0.5 *  ( (n-2)*log(try_value/current_value)
                                                                                                            + try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                            - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                    } else {
                        rate = -1.; // force reject
                    }

                    mModel->mLambdaSpline.mX = current_value;
                    mModel->mLambdaSpline.try_update(try_value, rate);
                    // il faut refaire le test car on ne sait pas si l'ancien lambda donnait positif
                    // G.1- Calcul spline
                    mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                   // if ( mModel->mLambdaSpline.mLastAccepts.last() == false) {
                        ok = hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation
                   // }
                    //qDebug()<<"[update_321] Depth counter="<<counter;



                } else {  // Not Depth
                    const double logMin = -20.;
                    const double logMax = +10.;

                    // On stocke l'ancienne valeur :
                    const double current_value = mModel->mLambdaSpline.mX;

                    // On tire une nouvelle valeur :
                    const double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mLambdaSpline.mSigmaMH);

                    const double try_value = pow(10., try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {
                        //current_h_lambda n'a pas changer ,depuis update theta

                        // Calcul du rapport :
                        mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                        try_h_lambda = h_lambda_321(current_splineMatricesLD, (int)mModel->mEvents.size(), try_value) ;
                        try_decomp_matB = decomp_matB(current_splineMatricesLD, try_value);
                        try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatricesLD, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_XYZ);
                        try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);

                        const auto n = mModel->mEvents.size();

                        rate = (try_h_lambda * try_value) / (current_h_lambda * current_value)  * exp( 0.5 *  ( (n-2)*log(try_value/current_value)
                                                                                                            + try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                            - current_ln_h_YWI_2 - current_ln_h_YWI_3));
                    } else {
                        rate = -1.; // force reject
                    }

                    mModel->mLambdaSpline.mX = current_value;
                    mModel->mLambdaSpline.try_update(try_value, rate);

                    ok = true;
                    /* --------------------------------------------------------------
                     *  G - Update mModel->mSpline
                     * -------------------------------------------------------------- */
                    // G.1- Calcul spline
                    mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                }
            // Pas bayésien : on sauvegarde la valeur constante dans la trace
            } else { // Rien à faire
                /* --------------------------------------------------------------
                 *  G - Update mModel->mSpline
                 * -------------------------------------------------------------- */
                // G.1- Calcul spline
                mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                // G.2 - test GPrime positive
                if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth  ) {
                    ok = hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

                } else
                    ok = true;

            }


        } catch(...) {
            qDebug() << "[MCMCLoopCurve::update_321] Update Lambda  Caught Exception!\n";
        }

        return ok;


    } catch (const char* e) {
        qWarning() << "[MCMCLoopCurve::update] char "<< e;

    } catch (const std::length_error& e) {
        qWarning() << "[MCMCLoopCurve::update] Length_error"<< e.what();

    } catch (const std::out_of_range& e) {
        qWarning() << "[MCMCLoopCurve::update] Out of range" << e.what();

    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve_321::update]  "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update_321] Caught Exception!\n";
        return false;
    }

    return false;
}
#endif

#pragma mark Version 3.3.0
#if VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH < 5

QString MCMCLoopCurve::initialize_330()
{
    updateLoop = &MCMCLoopCurve::update_330;

    std::vector<std::shared_ptr<Event>> &allEvents (mModel->mEvents);

    mNodeEvent.clear();
    mPointEvent.clear();

    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (std::shared_ptr<Event> &ev : allEvents) {
            if (mModel->is_curve && ev->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                ev->mTheta.mSamplerProposal = MHVariable::eMHAdaptGauss;
            }
            if (ev->mPointType == Event::eNode)
                mNodeEvent.push_back(ev);
            else
                mPointEvent.push_back(ev);

            ev->mS02Theta.mSamplerProposal = MHVariable::eFixe; // not yet integrated within update_330
        }
    } else {
        for (const std::shared_ptr<Event> &ev : allEvents) {
            if (mModel->is_curve && ev->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                ev->mTheta.mSamplerProposal = MHVariable::eMHAdaptGauss;
            }
            mPointEvent.push_back(ev);

        }
    }


    // -------------------------------- SPLINE part--------------------------------
    // Init function G

    prepareEventsY(allEvents);

    emit stepChanged(tr("Initializing G ..."), 0, (int)allEvents.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    current_vecH = calculVecH(mModel->mEvents);


    // ___________________________ Controle que les points ne sont pas sur une droite horizontal
    // et calcul de var_Y pour la suite
    if (mModel->compute_X_only) {
        std::vector<double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

        var_Y = variance_Knuth( vecY);

    } else if (mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec) { // à controler
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecY);

        var_Y /= 2.0;

    } else {
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYz;});
        var_Y += variance_Knuth( vecY);

        var_Y /= 3.0;
    }


    if ( (var_Y <= 0.0) && (mCurveSettings.mVarianceType != CurveSettings::eModeFixed)) {
        mAbortedReason = QString(tr("Error : Variance on Y is null, do computation with Variance G = 0 for this model "));
        return mAbortedReason;
    }

    // ----------------------------------------------------------------
    //  Init Lambda Spline
    // ----------------------------------------------------------------

    SplineMatricesLD matricesWI = prepare_calcul_spline_WI_LD(current_vecH);
    try {
        if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed) {
            mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;
            mModel->mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
            double memoLambda = log10(mModel->mLambdaSpline.mX);
            mModel->mLambdaSpline.memo(&memoLambda);

        } else {
           /* if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth)
                mModel->mLambdaSpline.mX = 1.;
            else*/
               // mModel->mLambdaSpline.mX = 1.0E2; // default = 1E-6.
            mModel->mLambdaSpline.mLastAccepts.clear();
            mModel->mLambdaSpline.accept_update(1E-6); // default = 1E+5.

        }
        mModel->mLambdaSpline.mSigmaMH = 1.0; // default = 1.0

        mModel->mC_lambda = (mModel->mEvents.size()-2.0)/(11.24 * std::pow(mModel->mEvents.size(), 4.0659)); // hypothese theta réparti uniforme

        mModel->mC_lambda /= var_Y;



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


            // ---
            const std::vector<t_matrix>& current_vect_Theta = get_vector<t_matrix>(get_Theta, mModel->mEvents);

            std::vector<t_matrix> current_vect_Yx, current_vect_Yy, current_vect_Yz;

            // Les vecteurs positions X, Y et Z doivent suivre l'ordre des thétas
            current_vect_Yx = get_vector<t_matrix>(get_Yx, mModel->mEvents);

            if (mModel->compute_Y) {
                current_vect_Yy = get_vector<t_matrix>(get_Yy, mModel->mEvents);
            }

            if (mModel->compute_XYZ) {
                current_vect_Yz = get_vector<t_matrix>(get_Yz, mModel->mEvents);
            }

            if (mModel->compute_XYZ) {
                Var_residual_spline = var_Gasser_3D(current_vect_Theta, current_vect_Yx, current_vect_Yy, current_vect_Yz);

            } else if (mModel->compute_Y) {
                Var_residual_spline = var_Gasser_2D(current_vect_Theta, current_vect_Yx, current_vect_Yy);

            } else {
                Var_residual_spline = var_Gasser(current_vect_Theta, current_vect_Yx);
            }



        }

        // memo Vg
        /* ----------------------------------------------------------------
         * The W of the events depend only on their VG
         * During the update, we need W for the calculations of theta, VG and Lambda Spline update
         * We will thus have to update the W at each VG modification
         * We calculate it here during the initialization to have its starting value
         * ---------------------------------------------------------------- */

        int i = 0;

        if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
            for (std::shared_ptr<Event>& e : mPointEvent) {
                i++;
                e->mVg.mX = Var_residual_spline;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain


                if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                } else {
                    e->mVg.accept_update(e->mVg.mX);
                }
                e->updateW();
                e->mVg.mSigmaMH = 1.0;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }

            for (std::shared_ptr<Event>& e : mNodeEvent) {
                i++;
                e->mVg.mX = 0.0;
                e->mVg.mSamplerProposal = MHVariable::eFixe;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                // check if Sy == 0
                if (e->mSy == 0.0) {
                    mAbortedReason = QString("Error: a Node cannot have a null error \n Change error in : %1").arg(e->getQStringName());
                    return mAbortedReason;
                }
                e->updateW();
                double memoVG = sqrt(e->mVg.mX);
                e->mVg.memo(&memoVG);
                e->mVg.mSigmaMH = 1.0;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }

        } else {
            // No global zero variance if no measurement error
            if (! mCurveSettings.mUseErrMesure && mCurveSettings.mVarianceType == CurveSettings::eModeFixed && mCurveSettings.mVarianceFixed == 0.0) {
                mAbortedReason = QString("Warning: If no measurement error, global error std gi cannot be zero");
                return mAbortedReason;
            }
            // Pas de Noeud dans le cas de Vg Global
            for (std::shared_ptr<Event> &e : allEvents) {
                i++;
                if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
                    e->mVg.mX = mCurveSettings.mVarianceFixed;
                    e->mVg.mSamplerProposal = MHVariable::eFixe;
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                    // Check if Sy + Vg == 0
                    if (e->mVg.mX + e->mSy * e->mSy == 0.0) {
                        mAbortedReason = QString("Error: a Node cannot have a null error with Variance null \n Change error in : %1").arg(e->getQStringName());
                        return mAbortedReason;
                    }

                } else {
                    e->mPointType = Event::ePoint; // force Node to be a simple Event
                    e->mVg.mX = Var_residual_spline;

                    e->mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;
                }
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                e->mVg.accept_update(e->mVg.mX);
                e->updateW();

                e->mVg.mSigmaMH = 1.0;

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
    // Curve init S02 Vg = Var_residual_spline // mS02Vg n'est plus Bayesien
    // ----------------------------------------------------------------
    mModel->mS02Vg = Var_residual_spline;

    // --------------------------- Current spline ----------------------
    try {
        /* --------------------------------------------------------------
         *  Calcul de la spline g, g" pour chaque composante x y z
         *-------------------------------------------------------------- */

        current_splineMatrices_LD = prepare_calcul_spline_LD(mModel->mEvents, current_vecH);
        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices_LD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

        // init Posterior MeanG and map
        const int nbPoint = 300;// Density curve size and curve size

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapG.setMinValue(+std::numeric_limits<double>::max());
        clearCompo.mapG.setMaxValue(0);

        clearCompo.mapGP = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapGP.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapGP.setMinValue(+std::numeric_limits<double>::max());
        clearCompo.mapGP.setMaxValue(0);

        clearCompo.vecG = std::vector<double> (nbPoint); // column
        clearCompo.vecGP = std::vector<double> (nbPoint);
        clearCompo.vecGS = std::vector<double> (nbPoint);
        clearCompo.vecVarG = std::vector<double> (nbPoint);
        clearCompo.vecVarianceG = std::vector<double> (nbPoint);
        clearCompo.vecVarErrG = std::vector<double> (nbPoint);

        PosteriorMeanG clearMeanG;
        clearMeanG.gx = clearCompo;

        //______
        // find X for t_min and t_max
        double g, gp, gs, varG;
        unsigned i0 = 0;
        valeurs_G_VarG_GP_GS(mModel->mSettings.mTmin, mModel->mSpline.splineX, g, varG, gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        double gx_tmin_sup = g + 1.96*sqrt(varG);
        double gx_tmin_inf = g - 1.96*sqrt(varG);

        valeurs_G_VarG_GP_GS(mModel->mSettings.mTmax, mModel->mSpline.splineX, g, varG, gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        double gx_tmax_sup = g + 1.96*sqrt(varG);;
        double gx_tmax_inf = g - 1.96*sqrt(varG);

        // La map est dans l'unité des données
        std::vector< double> vect_XInc (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vect_XInc.begin(), [](std::shared_ptr<Event> ev) {return ev->mXIncDepth;});
        auto minMax_XInc = std::minmax_element(vect_XInc.begin(), vect_XInc.end());
        const auto e = 0.1 *std::abs(*minMax_XInc.first - *minMax_XInc.second);

        Scale scx ;
        scx.findOptimal(std::min({*minMax_XInc.first - e, gx_tmin_inf, gx_tmax_inf  }) , std::max({ *minMax_XInc.second + e, gx_tmin_sup, gx_tmax_sup} ));

        clearMeanG.gx.mapG.setRangeY(scx.min, scx.max);

        if (mModel->compute_Y) {
            double gy_tmin_sup , gy_tmin_inf, gy_tmax_sup, gy_tmax_inf;
            i0 = 0;
            valeurs_G_VarG_GP_GS(mModel->mSettings.mTmin, mModel->mSpline.splineY, g, varG, gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
            gy_tmin_sup = g + 1.96 * sqrt(varG);
            gy_tmin_inf = g - 1.96 * sqrt(varG);

            valeurs_G_VarG_GP_GS(mModel->mSettings.mTmax, mModel->mSpline.splineY, g, varG, gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
            gy_tmax_sup = g + 1.96 * sqrt(varG);;
            gy_tmax_inf = g - 1.96 * sqrt(varG);


            clearMeanG.gy = clearCompo;


            std::vector< double> vect_YDec (mModel->mEvents.size());
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vect_YDec.begin(), [](std::shared_ptr<Event> ev) {return ev->mYDec;});
            auto minMax_YDec = std::minmax_element(vect_YDec.begin(), vect_YDec.end());

            std::vector< double> vect_sYDec (mModel->mEvents.size());
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vect_sYDec.begin(), [](std::shared_ptr<Event> ev) {return ev->mS_Y;});
            auto minMax_sYDec = std::minmax_element(vect_sYDec.begin(), vect_sYDec.end());

            const auto e = std::max(0.1 *std::abs(*minMax_YDec.first - *minMax_YDec.second), std::abs(*minMax_sYDec.second));


            Scale scy ;
            //sc.findOptimal(*minMax_YDec.first-e, *minMax_YDec.second+e);
            scy.findOptimal(std::min({*minMax_YDec.first - e, gy_tmin_inf, gy_tmax_inf  }) , std::max({ *minMax_YDec.second + e, gy_tmin_sup, gy_tmax_sup} ));


            clearMeanG.gy.mapG.setRangeY(scy.min, scy.max);

            if (mModel->compute_XYZ) {
                double gz_tmin_sup , gz_tmin_inf, gz_tmax_sup, gz_tmax_inf;
                i0 = 0;
                valeurs_G_VarG_GP_GS(mModel->mSettings.mTmin, mModel->mSpline.splineZ, g, varG, gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                gz_tmin_sup = g + 1.96 * sqrt(varG);
                gz_tmin_inf = g - 1.96 * sqrt(varG);

                valeurs_G_VarG_GP_GS(mModel->mSettings.mTmax, mModel->mSpline.splineZ, g, varG, gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                gz_tmax_sup = g + 1.96 * sqrt(varG);;
                gz_tmax_inf = g - 1.96 * sqrt(varG);

                clearMeanG.gz = clearCompo;

                std::vector< double> vect_ZF (mModel->mEvents.size());
                std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vect_ZF.begin(), [](std::shared_ptr<Event> ev) {return ev->mZField;});
                auto minMax_ZF = std::minmax_element(vect_ZF.begin(), vect_ZF.end());

                std::vector< double> vect_sZF (mModel->mEvents.size());
                std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vect_sZF.begin(), [](std::shared_ptr<Event> ev) {return ev->mS_ZField;});
                auto minMax_sZF = std::minmax_element(vect_sZF.begin(), vect_sZF.end());

                const auto e = std::max(0.1 *std::abs(*minMax_ZF.first - *minMax_ZF.second), std::abs(*minMax_sZF.second));

                Scale scz ;
                scz.findOptimal(std::min({*minMax_ZF.first - e, gz_tmin_inf, gz_tmax_inf  }) , std::max({ *minMax_ZF.second + e, gz_tmin_sup, gz_tmax_sup} ));

                clearMeanG.gz.mapG.setRangeY(scz.min, scz.max);
            }

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
        return QString("[MCMCLoopCurve::initialize_330] problem");
    }


    return QString();
}


#pragma mark update_330
bool MCMCLoopCurve::update_330()
{
    int n_points = mModel->mEvents.size();
    try {

        t_prob rate;
        // init the current state

        // les temps sont déjà initialisés et mis à jour dans B - Update Theta Events, si les temps sont bayésiens

        current_splineMatrices_LD = prepare_calcul_spline_LD(mModel->mEvents, current_vecH);
        current_decomp_QTQ_LD = decompositionCholesky(current_splineMatrices_LD.matQTQ, 5, 1); // used only with update Theta

        current_decomp_matB_LD = decomp_matB(current_splineMatrices_LD, mModel->mLambdaSpline.mX);

        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH
        current_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0.0 ? 0.0 :
                                 ln_h_YWI_3_update(current_splineMatrices_LD, mModel->mEvents, current_vecH, current_decomp_matB_LD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

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
                            date.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);
                        }
                    }
                }

            } catch(std::exception& exc) {
                qWarning() << "[MCMCLoopCurve::update_330] Ti : Caught Exception!\n" << exc.what();

            }  catch (...) {
                qWarning() << "[MCMCLoopCurve::update_330] Ti : Caught Exception!";
            }


            std::vector<t_matrix> current_vect_Yx, current_vect_Yy, current_vect_Yz;

            // Les vecteurs positions X, Y et Z doivent suivre l'ordre des thétas
            current_vect_Yx = get_vector<t_matrix>(get_Yx, mModel->mEvents);

            if (mModel->compute_Y) {
                current_vect_Yy = get_vector<t_matrix>(get_Yy, mModel->mEvents);
            }

            if (mModel->compute_XYZ) {
                current_vect_Yz = get_vector<t_matrix>(get_Yz, mModel->mEvents);
            }

            const std::vector<t_matrix>& current_vect_Theta = get_vector<t_matrix>(get_Theta, mModel->mEvents);

            t_matrix current_var_Gasser;
            t_matrix try_var_Gasser;
            // Les vecteurs positions X, Y et Z doivent suivre l'ordre des thétas

            if (mModel->compute_XYZ) {
                current_var_Gasser = var_Gasser_3D(current_vect_Theta, current_vect_Yx, current_vect_Yy, current_vect_Yz);

            } else if (mModel->compute_Y) {
                current_var_Gasser = var_Gasser_2D(current_vect_Theta, current_vect_Yx, current_vect_Yy);

            } else {
                current_var_Gasser = var_Gasser(current_vect_Theta, current_vect_Yx);
            }
            /* --------------------------------------------------------------
             *  B - Update Theta Events
             * -------------------------------------------------------------- */
            try {
                for (std::shared_ptr<Event> event : initListEvents) {
#ifdef _WIN32
    SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
#endif
                    if (event->mType == Event::eDefault) {
                        const double min = event->getThetaMin(tminPeriod);
                        const double max = event->getThetaMax(tmaxPeriod);

                        if (min >= max) {
                            const auto seed = mLoopChains.at(mChainIndex).mSeed;
                            throw QObject::tr("[MCMCLoopCurve::update_330] Error for event theta : %1 :\n min = %2 : max = %3 \n seed = %4").arg(event->getQStringName(), QString::number(min), QString::number(max), QString::number(seed));
                        }

                        // On stocke l'ancienne valeur
                        const double current_value = event->mTheta.mX;
                        current_h_theta = h_theta_Event(event); // utilise la valeur courante de mTheta.mX

                        // On tire une nouvelle valeur :
                        const double try_value = Generator::normalDistribution(current_value, event->mTheta.mSigmaMH);

                        if (try_value >= min && try_value <= max) {
                            // On force la mise à jour de la nouvelle valeur pour calculer h_new

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);

                            orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                            spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date

                            try_vecH = calculVecH(mModel->mEvents);

                            try_splineMatrices_LD = prepare_calcul_spline_LD(mModel->mEvents, try_vecH); // les Y suivent l'ordre des Events

                            try_decomp_QTQ_LD = decompositionCholesky(try_splineMatrices_LD.matQTQ, 5, 1);
                            try_decomp_matB_LD = decomp_matB(try_splineMatrices_LD, mModel->mLambdaSpline.mX);

                           // try_ln_h_YWI_1_2 = ln_h_YWI_1_2(try_decomp_QTQ, try_decomp_matB);

                            auto ln_rate_det = ln_rate_det_QtQ_det_B(try_decomp_QTQ_LD, try_decomp_matB_LD, current_decomp_QTQ_LD, current_decomp_matB_LD);

                            try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices_LD, mModel->mEvents, try_vecH, try_decomp_matB_LD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                           // t_prob rate_h_YWI2 = exp( 0.5 * ( try_ln_h_YWI_1_2 + try_ln_h_YWI_3 - current_ln_h_YWI_1_2 - current_ln_h_YWI_3));

                            t_prob rate_h_YWI = exp( 0.5 * ( ln_rate_det + try_ln_h_YWI_3 - current_ln_h_YWI_3));

                            try_h_theta = h_theta_Event(event);

                            // Conditionnel du au shrinkage,
                            const std::vector<t_matrix>& try_vect_Theta = get_vector<t_matrix>(get_Theta, mModel->mEvents);


                            // Les vecteurs positions X, Y et Z doivent suivre l'ordre des thétas
                            const std::vector<t_matrix>& try_vect_Yx = get_vector<t_matrix>(get_Yx, mModel->mEvents);

                            if (mModel->compute_XYZ) {
                                const std::vector<t_matrix>& try_vect_Yy = get_vector<t_matrix>(get_Yy, mModel->mEvents);
                                const std::vector<t_matrix>& try_vect_Yz = get_vector<t_matrix>(get_Yz, mModel->mEvents);
                                try_var_Gasser = var_Gasser_3D(try_vect_Theta, try_vect_Yx, try_vect_Yy, try_vect_Yz);

                            } else if (mModel->compute_Y) {
                                const std::vector<t_matrix>& try_vect_Yy = get_vector<t_matrix>(get_Yy, mModel->mEvents);
                                try_var_Gasser = var_Gasser_2D(try_vect_Theta, try_vect_Yx, try_vect_Yy);

                            } else {
                                try_var_Gasser = var_Gasser(try_vect_Theta, try_vect_Yx);
                            }

                            //  Le rapport du shrinkage VG dépendant de Gasser qui lui aussi dépend de theta
                            // faire une boucle sur les events dans le cas individuelle sinon une seule fois, pour VG fixe ou global

                            auto rate_VG = try_var_Gasser / current_var_Gasser;
                            if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
                                rate_VG = pow(rate_VG, mPointEvent.size()); // faire puissance n pour individuelle, le nombre d'Event non-Bound;

                                for (const auto &e : mPointEvent) {
                                    //rate_VG *= pow((e->mVg.mX + current_var_Gasser) / (e->mVg.mX + try_var_Gasser), 2.0);
                                    // rate_VG *= (e->mVg.mX + current_var_Gasser) / (e->mVg.mX + try_var_Gasser); false
                                    auto r = (e->mVg.mX + current_var_Gasser) / (e->mVg.mX + try_var_Gasser);
                                    rate_VG *= r * r;
                                }
                                rate_VG *= rate_VG;

                            } else {
                                auto r = (event->mVg.mX + current_var_Gasser) / (event->mVg.mX + try_var_Gasser);
                                rate_VG *= r * r;

                            }

                            // fin boucle VG
                            rate = rate_h_YWI;
                            rate *= try_h_theta / current_h_theta ;
                            rate *= rate_VG;


                        } else {
                            rate = -1.0;
#ifdef DEBUG
                            qDebug() << " update_330 try_value >= min && try_value <= max " << min << try_value << max << current_value << event->mTheta.mSigmaMH << event->name();
#endif
                        }



                        // test_update() return true if accepted
                        // set current_value or try_value to mX according to the test of rate
                        if ( event->mTheta.test_update(current_value, try_value, rate)) {

                            // Pour l'itération suivante :

                            std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);

                            std::swap(current_vecH, try_vecH);
                            std::swap(current_splineMatrices_LD, try_splineMatrices_LD);
                            //std::swap(current_h_lambda, try_h_lambda);
                            std::swap(current_decomp_matB_LD, try_decomp_matB_LD);
                            std::swap(current_decomp_QTQ_LD, try_decomp_QTQ_LD);
                            current_var_Gasser = try_var_Gasser;


                        }
                        event->mThetaReduced = mModel->reduceTime(event->mTheta.mX);

                    } else { // this is a bound, nothing to sample. Always the same value
                       //  event->updateTheta(tminPeriod, tmaxPeriod); //useless if fixed
                    }

                    // update after tryUpdate or updateTheta
                    event->mThetaReduced = mModel->reduceTime(event->mTheta.mX);

                    /* --------------------------------------------------------------
                     * C.1 - Update Alpha, Beta & Duration Phases
                     * -------------------------------------------------------------- */
                    //  Update Phases -set mAlpha and mBeta ; they coud be used by the Event in the other Phase ----------------------------------------
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});

                } // End of loop initListEvents

                //  Update Phases Tau; they coud be used by the Event in the other Phase ----------------------------------------
                /* --------------------------------------------------------------
                 *  C.2 - Update Tau Phases
                 * -------------------------------------------------------------- */
                std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_Tau (tminPeriod, tmaxPeriod);});

                /* --------------------------------------------------------------
                 *  C.3 - Update Gamma Phases
                 * -------------------------------------------------------------- */
                std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (std::shared_ptr<PhaseConstraint> pc) {pc->updateGamma();});

            } catch(std::exception& exc) {
                qWarning() << "[MCMCLoopCurve::update_330] Theta : Caught Exception!\n"<<exc.what();
            }

        }
        /* Pas bayésien : Tous les temps sont fixes
         *  current_vecH ne change pas jusqu'à la prochaine itération
        */


        /* --------------------------------------------------------------
         *  E - Update Vg
         *  D.1 - Update Vg for Points only
         *  D.2 - Update Vg Global
         * -------------------------------------------------------------- */
        try {
            constexpr double logMin = -10.0;
            constexpr double logMax = +20.0;
            /* --------------------------------------------------------------
            * F - Update mS02Vg, tau in Komlan
            * -------------------------------------------------------------- */


            if (mCurveSettings.mVarianceType != CurveSettings::eModeFixed) {
                if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {

                    // Mise à jour de Var_residual_spline avec le Y dans l'ordre des theta
                    const std::vector<t_matrix>& vec_t = get_vector<t_matrix>(get_Theta, mModel->mEvents);
                    const std::vector<t_matrix>& vect_Yx = get_vector<t_matrix>(get_Yx, mModel->mEvents);

                    if (mModel->compute_XYZ) {
                        const std::vector<t_matrix>& vect_Yy = get_vector<t_matrix>(get_Yy, mModel->mEvents);
                        const std::vector<t_matrix>& vect_Yz = get_vector<t_matrix>(get_Yz, mModel->mEvents);
                        Var_residual_spline = var_Gasser_3D(vec_t, vect_Yx, vect_Yy, vect_Yz);

                    } else if (mModel->compute_Y) {
                        const std::vector<t_matrix>& vect_Yy = get_vector<t_matrix>(get_Yy, mModel->mEvents);
                        Var_residual_spline = var_Gasser_2D(vec_t, vect_Yx, vect_Yy);

                    } else {
                        Var_residual_spline = var_Gasser(vec_t, vect_Yx);
                    }


                }

                mModel->mS02Vg = Var_residual_spline;
            }
            /* --------------------------------------------------------------
            *  D - Update Vg
            * -------------------------------------------------------------- */
#pragma mark Update Vg Bayesian Individual
            if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

                //current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB); // Has not been initialized yet
                // Normalement ici tout est déjà calculé
                current_ln_h_YWI_3 = ln_h_YWI_3_update(current_splineMatrices_LD, mModel->mEvents, current_vecH, current_decomp_matB_LD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);
                //current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);

                /* --------------------------------------------------------------
                *  D.1 - Update Vg for Points only, not the node
                * -------------------------------------------------------------- */

                for (std::shared_ptr<Event>& event : mPointEvent) {

                    const double current_value = event->mVg.mX;
                    //const t_prob W_current = event->mW; // pour test V335

                    double try_value = current_value;

                    if (current_value != 0.0) {
                        current_h_VG = h_VG_Event(current_value, mModel->mS02Vg);

                        // On tire une nouvelle valeur :
                        const double try_value_log = Generator::normalDistribution(log10(current_value), event->mVg.mSigmaMH);

                        if (try_value_log >= logMin && try_value_log <= logMax) {
                            try_value = pow(10., try_value_log);
                            // On force la mise à jour de la nouvelle valeur pour calculer try_h
                            // A chaque fois qu'on modifie VG, W change !
                            event->mVg.mX = try_value;
                            event->updateW(); // used by prepareCalculSpline

                            // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG

                            try_splineMatrices_LD = prepare_calcul_spline_LD(mModel->mEvents, current_vecH); // utilise vecH et e->mW
                            try_decomp_matB_LD = decomp_matB(try_splineMatrices_LD, mModel->mLambdaSpline.mX);

                            try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices_LD, mModel->mEvents, current_vecH, try_decomp_matB_LD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                            auto ln_rate_B = ln_rate_det_B(try_decomp_matB_LD, current_decomp_matB_LD);

                            auto rate_h_YWI = exp( 0.5 * ( ln_rate_B + try_ln_h_YWI_3 - current_ln_h_YWI_3));

                            // conditionnel du au shrinkage
                            try_h_VG = h_VG_Event(try_value, mModel->mS02Vg);

                            // Calcul du rapport de probabilité d'acceptation.
                            rate = rate_h_YWI;
                            rate *= try_h_VG / current_h_VG;

                            // multiplier par le jacobien
                            rate *= try_value / current_value;


                        } else {
                            rate = -1.; // force reject // force to keep current state

                        }
                    } else {
                        rate = -1.; // force reject // force to keep current state VG = 0
                    }


                    // Metropolis Hastings update
                    // A chaque fois qu'on modifie VG, W change !

                    if ( event->mVg.test_update(current_value, try_value, rate)) {

                        // Pour l'itération suivante : Car mVg a changé
                       // std::swap(current_ln_h_YWI_1_2, try_ln_h_YWI_1_2);
                       // std::swap(current_ln_h_YWI_2, try_ln_h_YWI_2);
                        std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);
                        std::swap(current_splineMatrices_LD, try_splineMatrices_LD);
                        std::swap(current_decomp_matB_LD, try_decomp_matB_LD);
                    }
                    event->updateW();
#ifdef DEBUG
                    if (event->mVg.mX > 1E4*1E4) {
                        qDebug() << "[MCMCLoopCurve::update_330] grand VG ??" << event->mVg.mX << event->name();
                    }
#endif
                }


            } else if (mCurveSettings.mVarianceType == CurveSettings::eModeGlobal) {
#pragma mark update Vg Global
               // current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB); // Has not been initialized yet

                current_ln_h_YWI_3 = ln_h_YWI_3_update(current_splineMatrices_LD, mModel->mEvents, current_vecH, current_decomp_matB_LD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);



                /* --------------------------------------------------------------
                *  D.2 - Update Vg Global
                * -------------------------------------------------------------- */

                // On stocke l'ancienne valeur :
                const double current_value = mModel->mEvents[0]->mVg.mX;

                // On tire une nouvelle valeur :

                const double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mEvents[0]->mVg.mSigmaMH);


                if (try_value_log >= logMin && try_value_log <= logMax) {
                    const double try_value = pow(10, try_value_log);
                    current_h_VG = h_VG_Event(current_value, mModel->mS02Vg);

                    // Affectation temporaire pour évaluer la nouvelle proba
                    // Dans le cas global pas de différence entre les Points et les Nodes
                    for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                        ev->mVg.mX = try_value;
                        ev->updateW();
                    }

                    // Calcul du rapport : try_splineMatricesLD utilise les temps reduits, elle est affectée par le changement de Vg
                    //try_splineMatricesLD = prepareCalculSpline(mModel->mEvents, current_vecH); // utilise vecH et e->mW
                    try_splineMatrices_LD = update_splineMatrice_with_mW(current_splineMatrices_LD, mModel->mEvents );

                    try_decomp_matB_LD = decomp_matB(try_splineMatrices_LD, mModel->mLambdaSpline.mX);
                    try_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0.0 ? 0.0 : ln_h_YWI_3_update(try_splineMatrices_LD, mModel->mEvents, current_vecH, try_decomp_matB_LD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                    auto ln_rate_B = ln_rate_det_B(try_decomp_matB_LD, current_decomp_matB_LD);
                    auto rate_h_YWI = exp( 0.5 * ( ln_rate_B + try_ln_h_YWI_3 - current_ln_h_YWI_3));

                    try_h_VG = h_VG_Event(try_value, mModel->mS02Vg);

                    // Calcul du rapport de probabilité d'acceptation.
                    rate = rate_h_YWI;
                    rate *= try_h_VG / current_h_VG;

                    // multiplier par le jacobien
                    rate *= try_value / current_value;

                    // On fait le test du rapport directement, car les fonctions test_update() et try_update(), ajoutent une valeur à mLastAccept
                    bool accepted;
                    if (rate >= 1.0) {
                        // Accept unconditionally
                        accepted = true;

                    } else if (rate < 0.0) {
                        // Reject outright
                        accepted = false;

                    } else {

                        // For rate between 0.0 and 1.0, perform a Metropolis-Hastings accept/reject step
                        const double random_number = Generator::randomUniform();

                        if (random_number <= rate) {
                            accepted = true;

                        } else {
                            accepted = false;
                        }
                    }

                    if ( accepted) {

                        //std::swap(try_ln_h_YWI_2, current_ln_h_YWI_2);
                        std::swap(try_ln_h_YWI_3, current_ln_h_YWI_3);
                        std::swap(try_splineMatrices_LD, current_splineMatrices_LD);
                        std::swap(try_decomp_matB_LD, current_decomp_matB_LD);

                        // accepted
                        for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                            ev->mVg.accept_update(try_value);
                            ev->updateW();
                        }

                    } else {// rejected
                        for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                            ev->mVg.mX = current_value; // forced update, to restore the previous current_value
                            ev->updateW();
                            ev->mVg.reject_update();
                        }
                    }

                } else { // rejected
                    for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                        ev->mVg.mX = current_value; // forced update, to restore the previous current_value
                        ev->updateW();
                        ev->mVg.reject_update();
                    }

                }


                // Not bayesian
            } else { // nothing to do : mCurveSettings.mVarianceType == CurveSettings::eFixed
            }
        } catch (std::exception& e) {
            qWarning()<< "[MCMCLoopCurve::update_330] VG : exception caught: " << e.what() << '\n';

        } catch(...) {
            qWarning() << "[MCMCLoopCurve::update_330] VG Event Caught Exception!\n";

        }

        /* --------------------------------------------------------------
         * E - Update Lambda
         * -------------------------------------------------------------- */
        // Si mW est modifié, current_splineMatricesLD est mis à jour lors de update VG, ainsi que current_decomp_matB
        //current_vecH = calculVecH(mModel->mEvents);
        //current_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH); // utilise vecH et e->mW // peut-être inutile??
        //current_decomp_matB = decomp_matB(current_splineMatricesLD, mModel->mLambdaSpline.mX);

        bool ok = true;

        try {

            if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {

                constexpr double lambda_logMin = -20.0;
                constexpr double lambda_logMax = +10.0;

                current_ln_h_YWI_3 = ln_h_YWI_3_update(current_splineMatrices_LD, mModel->mEvents, current_vecH, current_decomp_matB_LD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);
                //current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB);

                // On stocke l'ancienne valeur :
                const double current_value = mModel->mLambdaSpline.mX;

                // On tire une nouvelle valeur :
                const double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                const double try_value = pow(10., try_value_log);


                if (try_value_log >= lambda_logMin && try_value_log <= lambda_logMax) {


                    // Calcul du rapport :
                    mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                    //try_h_lambda = h_lambda_330(try_value) ; // calcul différent, inutile calcul directe des rapports
                    constexpr int mu = 3;
                    const t_prob c = mModel->mC_lambda;
                    /**
                     * \f$ P(\lambda) = \frac{ 1} { \left( {c + \lambda} \right)^{\mu + 1}} \f$
                     */
                    const t_prob rate_h_lambda = pow((c + current_value) / (c + try_value), mu + 1);

                    try_decomp_matB_LD = decomp_matB(current_splineMatrices_LD, try_value);
                    //try_ln_h_YWI_3 = try_value == 0.0 ? 0.0 : ln_h_YWI_3_update(current_splineMatricesLD, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_XYZ);
                    //try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);

                    auto ln_rate_B = ln_rate_det_B(try_decomp_matB_LD, current_decomp_matB_LD);

                    try_ln_h_YWI_3 = try_value == 0.0 ? 0.0 : ln_h_YWI_3_update(current_splineMatrices_LD, mModel->mEvents, current_vecH, try_decomp_matB_LD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                    // La partie exp( 0.5 *  ( (n-2)*log(try_value/current_value) , n'est pas présente dans les formules ln_h_YWI, il faut l'ajouter ici
                    //t_prob rate_h_YWI2 = exp( 0.5 *   ( (n_points - 2)*log(try_value/current_value)
                    //                                   + try_ln_h_YWI_2 + try_ln_h_YWI_3 - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                    t_prob rate_h_YWI = exp( 0.5 *   ( (n_points - 2)*log(try_value/current_value)
                                                       + ln_rate_B + try_ln_h_YWI_3 - current_ln_h_YWI_3));

                    // Calcul du rapport de probabilité d'acceptation.
                    rate = rate_h_YWI * rate_h_lambda;

                    // multiplier par le jacobien
                    rate *= try_value / current_value;
                    mModel->mLambdaSpline.test_update(current_value, try_value, rate);

                } else {
                    mModel->mLambdaSpline.reject_update();
                }


            }


            /* --------------------------------------------------------------
             *  G - Update mModel->mSpline
             * -------------------------------------------------------------- */
            // G.1- Calcul spline
            mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices_LD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

            // G.2 - Test GPrime positive
            if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth  ) {
                ok = hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mModel->mSettings.mTmin, mModel->mSettings.mTmin, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation
                //if (!ok)
                   // qDebug()<<"[MCMCLoopCurve::update_330] rejetéé GPrime positive  "<< (double) rate;

            } else
                ok = true;

        } catch(...) {
            ok = false;
            qDebug() << "[MCMCLoopCurve::update_330] Update Lambda  Caught Exception!\n";
        }

        return ok;


    } catch (const char* e) {
        qWarning() << "[MCMCLoopCurve::update_330] char "<< e;

    } catch (const std::length_error& e) {
        qWarning() << "[MCMCLoopCurve::update_330] Length_error"<< e.what();

    } catch (const std::out_of_range& e) {
        qWarning() << "[MCMCLoopCurve::update_330] Out of range" <<e.what();

    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve::update_330]  "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update_330] Caught Exception!\n";
        return false;
    }

    return false;
}
#endif

#pragma mark Version 3.3.5
#if VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH >= 5

QString MCMCLoopCurve::initialize_335()
{
#ifdef DEBUG
    std::cout << "[MCMCLoopCurve::initialize_335]" << std::endl;
#endif

    updateLoop = &MCMCLoopCurve::update_335;

    std::vector<std::shared_ptr<Event>> &allEvents (mModel->mEvents);

    mNodeEvent.clear();
    mPointEvent.clear();

    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (std::shared_ptr<Event> &ev : allEvents) {
            if (mModel->is_curve && ev->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                ev->mTheta.mSamplerProposal = MHVariable::eMHAdaptGauss;
            }
            if (ev->mPointType == Event::eNode)
                mNodeEvent.push_back(ev);
            else
                mPointEvent.push_back(ev);

#ifdef S02_BAYESIAN
            ev->mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss; // not yet integrated within update_330
#else
            ev->mS02Theta.mSamplerProposal = MHVariable::eFixe; // not yet integrated within update_330
#endif

        }
    } else {
        for (const std::shared_ptr<Event> &ev : allEvents) {
            if (mModel->is_curve && ev->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                ev->mTheta.mSamplerProposal = MHVariable::eMHAdaptGauss;
            }
            mPointEvent.push_back(ev);

        }
    }


    // -------------------------------- SPLINE part--------------------------------
    // Init function G


    auto vect_XIncDepth = get_vector<double>(get_XIncDepth, mModel->mEvents);
    auto vect_YDec = get_vector<double>(get_YDec, mModel->mEvents);
    auto vect_ZField = get_vector<double>(get_ZField, mModel->mEvents);
    prepareEventsY(allEvents);

    //emit stepChanged(tr("Initializing G ..."), 0, (int)allEvents.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    allEvents = mModel->mEvents; //les Events ont été réordonnées

    current_vecH = calculVecH(mModel->mEvents);

    std::vector<double> vec_theta = get_vector<double>(get_Theta, mModel->mEvents);
    auto vec_X = get_vector<double>(get_Yx, mModel->mEvents);
    auto vec_Y = get_vector<double>(get_Yy, mModel->mEvents);
    auto vec_Z = get_vector<double>(get_Yz, mModel->mEvents);


    // ___________________________ Controle que les points ne sont pas sur une droite horizontal
    // et calcul de var_Y pour la suite
    if (mModel->compute_X_only) {
        var_Y = variance_Knuth( vec_X);

    } else if (mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec) { // à controler
        var_Y = variance_Knuth( vec_X);
        var_Y += variance_Knuth( vec_Y);

        var_Y /= 2.0;

    } else {
        var_Y = variance_Knuth( vec_X);
        var_Y += variance_Knuth( vec_Y);
        var_Y += variance_Knuth( vec_Z);

        var_Y /= 3.0;
    }


    if ( (var_Y <= 0.0) && (mCurveSettings.mVarianceType != CurveSettings::eModeFixed)) {
        mAbortedReason = QString(tr("Error : Variance on Y is null, do computation with Variance G = 0 for this model "));
        return mAbortedReason;
    }


    // ----------------------------------------------------------------
    // Curve init Vg_i
    // ----------------------------------------------------------------
    try {
        if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
            Var_residual_spline = mCurveSettings.mVarianceFixed;

        } else { // si individuel ou global VG = S02

            if (mModel->compute_X_only) {
                Var_residual_spline = var_Gasser(vec_theta, vec_X);


            } else {
                if (mModel->compute_XYZ) {

                    Var_residual_spline = var_Gasser_3D(vec_theta, vec_X, vec_Y, vec_Z);


                } else {
                    Var_residual_spline = var_Gasser_2D(vec_theta, vec_X, vec_Y);

                }

            }


        }

        // ----------------------------------------------------------------
        // Curve init S02 Vg = Var_residual_spline // inutile mS02Vg si plus Bayesien; sauf cas KOMLAN
        // ----------------------------------------------------------------

#ifdef KOMLAN
        const double s_harmonique = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0., [](double s0, auto e) { return s0 + 1./pow(e->mSy, 2.);});
        const double mean_harmonique = sqrt((double)mModel->mEvents.size()/ s_harmonique);

        double mean = sqrt( (Var_residual_spline + pow(mean_harmonique, 2)) /2 );
        std::cout << "init mean " << mean << " mean_harmonique=" << mean_harmonique << " Var_residual_spline=" << Var_residual_spline << std::endl;
        const double beta = 1.004680139 * (1 - exp(- 0.0000847244 * pow(mean, 2.373548593)));
        mModel->mSO2Vg_beta = beta;//std::max(1E-10, beta);
        std::cout << "init beta " << beta << std::endl;


        const double S02Vg = 1.0 / Generator::gammaDistribution(1.0, beta);

        mModel->mS02Vg.mLastAccepts.clear();
        mModel->mS02Vg.accept_update(S02Vg);
        mModel->mS02Vg.mSamplerProposal = MHVariable::eMHAdaptGauss;
        mModel->mS02Vg.mSigmaMH = 1.0;

#else
        mModel->mS02Vg = Var_residual_spline;// <- code d'origine
#endif
        /* ----------------------------------------------------------------
         * The W of the events depend only on their VG
         * During the update, we need W for the calculations of theta, VG and Lambda Spline update
         * We will thus have to update the W at each VG modification
         * We calculate it here during the initialization to have its starting value
         * ---------------------------------------------------------------- */


        //int i = 0;

        if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

            for (std::shared_ptr<Event>& e : mModel->mEvents) {
                //i++;

                e->mVg.mX = Var_residual_spline;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain


                if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                } else {
                    e->mVg.accept_update(e->mVg.mX);
                }
                e->updateW();
                e->mVg.mSigmaMH = 1.0;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                //emit stepProgressed(i);
            }

            for (std::shared_ptr<Event>& e : mNodeEvent) {
                //i++;
                e->mVg.mX = 0.0;
                e->mVg.mSamplerProposal = MHVariable::eFixe;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                // check if Sy == 0
                if (e->mSy == 0.0) {
                    mAbortedReason = QString("Error: a Node cannot have a null error \n Change error in : %1").arg(e->getQStringName());
                    return mAbortedReason;
                }
                e->updateW();
                double memoVG = sqrt(e->mVg.mX);
                e->mVg.memo(&memoVG);
                e->mVg.mSigmaMH = 1.0;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

               // emit stepProgressed(i);
            }


        } else {
            // No global zero variance if no measurement error
            if (! mCurveSettings.mUseErrMesure && mCurveSettings.mVarianceType == CurveSettings::eModeFixed && mCurveSettings.mVarianceFixed == 0.0) {
                mAbortedReason = QString("Warning: If no measurement error, global error std gi cannot be zero");
                return mAbortedReason;
            }

            // Pas de Noeud dans le cas de Vg Global
            for (std::shared_ptr<Event> &e : allEvents) {
                //i++;
                if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
                    e->mVg.mX = mCurveSettings.mVarianceFixed;
                    e->mVg.mSamplerProposal = MHVariable::eFixe;
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                    // Check if Sy + Vg == 0
                    if (e->mVg.mX + e->mSy * e->mSy == 0.0) {
                        mAbortedReason = QString("Error: a Node cannot have a null error with Variance null \n Change error in : %1").arg(e->getQStringName());
                        return mAbortedReason;
                    }

                } else { // Mode Global
                    e->mPointType = Event::ePoint; // force Node to be a simple Event

                    e->mVg.mX = Var_residual_spline;

                    e->mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;
                }
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                e->mVg.accept_update(e->mVg.mX);
                e->updateW();



                e->mVg.mSigmaMH = 1.0;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                //emit stepProgressed(i);
            }
        }


    }  catch (...) {
        qWarning() << "Curve init Vg  ???";
        mAbortedReason = QString("Curve init Vg  ???");
        return mAbortedReason;
    }



#pragma mark initialize Lambda

    // ----------------------------------------------------------------
    //  F. Init Lambda Spline
    // ----------------------------------------------------------------

    SplineMatricesD matricesWI = prepare_calcul_spline_WI_D(current_vecH);
    MCMCSpline try_spline;
    SplineMatricesD try_spline_matrices;
    MatrixD K;
    try {
        if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed) {
            mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;
            mModel->mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
            double memoLambda = log10(mModel->mLambdaSpline.mX);
            mModel->mLambdaSpline.memo(&memoLambda);

        } else {
            if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth) {
                // F.2 - test GPrime positive
                /*
                mModel->mLambdaSpline.mX = 1.0;

                double try_value = 10;

                int max_iterations = 100; // Limite maximale de tentatives
                int iteration_count = 0;
                */

                SparseMatrixD R = calculMatR_D(current_vecH);// dim n-2 * n-2 //R est une matrice creuse symetrique padded

                SparseMatrixD Q = calculMatQ_D(current_vecH); // matrice creuse
                DiagonalMatrixD W_1 (mModel->mEvents.size()) ; // correspond à 1.0/mW

                std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), W_1.diagonal().begin(), [](std::shared_ptr<Event> ev){return 1.0/ ev->mW;});// {return (ev->mSy*ev->mSy + ev->mVg.mX;});

                SparseMatrixD Qt = Q.transpose();

                // Memo des matrices
                current_splineMatrices_D.diagWInv = W_1;
                current_splineMatrices_D.matR = R;
                current_splineMatrices_D.matQ = Q;
                current_splineMatrices_D.matQTW_1Q = Q.transpose()* W_1* Q;
                current_splineMatrices_D.matQTQ = Q.transpose() * Q;


                SparseQuadraticFormSolver solver(1); // shift=1 notre padding
                solver.factorize(R); // Factorisation une seule fois, crée le solver ldlt

                MatrixD R_1QT = solver.compute_Rinv_QT(Q);

                initialize_spline_for_depth(mModel->mEvents, R, R_1QT, Q);


                // std::cout << " lambda for depth = " << try_value*10 << " depth_OK = " << depth_OK  << std::endl;


            } else {
                mModel->mLambdaSpline.mX = 1.0E-6; // default = 1E-6.
            }

            mModel->mLambdaSpline.mLastAccepts.clear();
            mModel->mLambdaSpline.accept_update(mModel->mLambdaSpline.mX);

        }
        mModel->mLambdaSpline.mSigmaMH = 1.0; // default = 1.0

#ifdef KOMLAN
        auto trK = K.trace();
        mModel->mC_lambda = (mModel->mEvents.size()-2.0) / trK;
#else
        /**
        * C = frac{n - 2.0}{11.24 * n^{4.0659}}
        */
        mModel->mC_lambda = (mModel->mEvents.size()-2.0) / (11.24 * std::pow(mModel->mEvents.size(), 4.0659)); // hypothese: les thetas sont répartis uniformement
        mModel->mC_lambda /= var_Y;
#endif


    }  catch (...) {
        qWarning() << "Init Lambda Spline  ???";
        mAbortedReason = QString("Init Lambda Spline  ???");
        return mAbortedReason;
    }

    // --------------------------- Current spline ----------------------
    try {

        /* --------------------------------------------------------------
         *  Calcul de la spline g, g" pour chaque composante x y z
         *-------------------------------------------------------------- */
        if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth && mCurveSettings.mLambdaSplineType != CurveSettings::eModeFixed) {
            // nothing to do all is done in initialize_spline_for_depth(); lamdba, matrice, Gx, gamma
            //current_splineMatrices_D = try_spline_matrices;
            //mModel->mSpline = try_spline;

        } else {
            current_splineMatrices_D = prepare_calcul_spline_D(mModel->mEvents, current_vecH);
            mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices_D, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);
        }

        // find X for t_min and t_max
        double g, gp, gs;
        unsigned i0 = 0;
        valeurs_G_GP_GS(mModel->mSettings.mTmin, mModel->mSpline.splineX, g, gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        double gx_tmin = g;

        valeurs_G_GP_GS(mModel->mSettings.mTmax, mModel->mSpline.splineX, g,gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        double gx_tmax = g ;


        double gy_tmin, gy_tmax;
        if (mModel->compute_Y) {
            valeurs_G_GP_GS(mModel->mSettings.mTmin, mModel->mSpline.splineY, g, gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
            gy_tmin = g;;

            valeurs_G_GP_GS(mModel->mSettings.mTmax, mModel->mSpline.splineY, g, gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
            gy_tmax = g ;
        }

        // init Posterior MeanG and map
        const int nbPoint = 300;// Density curve size and curve size

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapG.setMinValue(+std::numeric_limits<double>::max());
        clearCompo.mapG.setMaxValue(0);

        clearCompo.mapGP = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapGP.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapGP.setMinValue(+std::numeric_limits<double>::max());
        clearCompo.mapGP.setMaxValue(0);

        clearCompo.vecG = std::vector<double> (nbPoint); // column
        clearCompo.vecVarG = std::vector<double> (nbPoint);
        clearCompo.vecGP = std::vector<double> (nbPoint);
        clearCompo.vecVarGP = std::vector<double> (nbPoint);
        clearCompo.vecGS = std::vector<double> (nbPoint);

        clearCompo.vecVarianceG = std::vector<double> (nbPoint);

       clearCompo.vecVarErrG = std::vector<double> (0);

        PosteriorMeanG clearMeanG;
        clearMeanG.gx = clearCompo;

        // La map est dans l'unité des données
        std::vector< double> vect_XInc (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vect_XInc.begin(), [](std::shared_ptr<Event> ev) {return ev->mXIncDepth;});
        auto minMax_XInc = std::minmax_element(vect_XInc.begin(), vect_XInc.end());
        const auto e = 0.1 *std::abs(*minMax_XInc.first - *minMax_XInc.second);

        Scale sc ;
        sc.findOptimal(std::min({*minMax_XInc.first - e, gx_tmin, gx_tmax  }) , std::max({ *minMax_XInc.second + e, gx_tmin, gx_tmax} ));

        clearMeanG.gx.mapG.setRangeY(sc.min, sc.max);

        if (mModel->compute_Y) {
            clearMeanG.gy = clearCompo;

            std::vector< double> vect_XDec (mModel->mEvents.size());
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vect_XDec.begin(), [](std::shared_ptr<Event> ev) {return ev->mYDec;});
            auto minMax_YDec = std::minmax_element(vect_XDec.begin(), vect_XDec.end());

            std::vector< double> vect_sYDec (mModel->mEvents.size());
            std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vect_sYDec.begin(), [](std::shared_ptr<Event> ev) {return ev->mS_Y;});
            auto minMax_sYDec = std::minmax_element(vect_sYDec.begin(), vect_sYDec.end());

            const auto e = std::max({0.1 *std::abs(*minMax_YDec.first - *minMax_YDec.second), std::abs(*minMax_sYDec.second), gy_tmin, gy_tmax});


            Scale sc ;
            sc.findOptimal(*minMax_YDec.first - e, *minMax_YDec.second + e);

            clearMeanG.gy.mapG.setRangeY(sc.min, sc.max);

            if (mModel->compute_XYZ) {
                clearMeanG.gz = clearCompo;

                std::vector< double> vect_ZF (mModel->mEvents.size());
                std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vect_ZF.begin(), [](std::shared_ptr<Event> ev) {return ev->mZField;});
                auto minMax_ZF = std::minmax_element(vect_ZF.begin(), vect_ZF.end());

                std::vector< double> vect_sZF (mModel->mEvents.size());
                std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vect_sZF.begin(), [](std::shared_ptr<Event> ev) {return ev->mS_ZField;});
                auto minMax_sZF = std::minmax_element(vect_sZF.begin(), vect_sZF.end());

                const auto e = std::max(0.1 *std::abs(*minMax_ZF.first - *minMax_ZF.second), std::abs(*minMax_sZF.second));

                Scale sc ;
                sc.findOptimal(*minMax_ZF.first-e, *minMax_ZF.second+e);

                clearMeanG.gz.mapG.setRangeY(sc.min, sc.max);
            }

        }

        mModel->mPosteriorMeanGByChain.push_back(clearMeanG);
        if (mChainIndex == 0)
            mModel->mPosteriorMeanG = clearMeanG;

    }  catch (...) {
        qWarning() <<"[MCMCLoopCurve::initialize_335] init Posterior MeanG and map  ???";
    }

    /*
     * INIT UPDATE
     */
    // init the current state
    try {
        initListEvents.resize(mModel->mEvents.size());
        std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    } catch (...) {
        qWarning() <<"[MCMCLoopCurve::initialize_335] init Posterior MeanG and map  ???";
        return QString("[MCMCLoopCurve::initialize_335] Problem");
    }


    return QString();
}

/**
 * @brief MCMCLoopCurve::update_335 : Echantillonnage MCMC de G par gaussienne multivariée
 * @return
 */

bool MCMCLoopCurve::update_335()
{
    long long n_points = mModel->mEvents.size();
    long long n_components = 1;

    if (mModel->compute_XYZ) {
        n_components = 3;

    } else if (mModel->compute_Y) {
        n_components = 2;
    }

    try {

        // --------------------------------------------------------------
        //  A - Update ti Dates (idem MCMCLoopChrono)
        // --------------------------------------------------------------
        try {
            if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
                for (std::shared_ptr<Event>& event : mModel->mEvents) {
                    for (auto&& date : event->mDates) {
                        date.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);
                    }
                }
            }

        }  catch (...) {
            qWarning() <<"[MCMCLoopCurve::update_335] update Date ???";
        }
        // Variable du MH de la spline

        double current_value;
        double try_value;

        // SparseMatrixLD current_R, current_Q;
        MatrixD current_R_1QT;
        MatrixD current_K;
        MatrixD current_Y(n_points, n_components), current_G(n_points, n_components);

        //SparseMatrixLD try_R, try_Q;
        //MatrixLD  try_K, try_R_1QT;
        DiagonalMatrixD W_1;

        t_prob rate;


        // --------------------------------------------------------------
        //  B - Update theta Events
        // --------------------------------------------------------------
        // copie la liste des pointeurs
        std::vector<std::shared_ptr<Event>> initListEvents (mModel->mEvents.size());
        std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

        std::vector<double> sy = get_vector<double> (get_Sy, initListEvents);

        std::vector<double> current_vect_Yx, current_vect_Yy, current_vect_Yz;

        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);

        // Les vecteurs positions X, Y et Z doivent suivre l'ordre des thétas

        // Construction de la matrice Y_mat des points, les positions (x, y, z) ne sont mis à jour
        // qu'à la fin

        if (mModel->compute_XYZ) {
            const ColumnVectorD& x_vec = get_ColumnVector<double>(get_Yx, mModel->mEvents);
            const ColumnVectorD& y_vec = get_ColumnVector<double>(get_Yy, mModel->mEvents);
            const ColumnVectorD& z_vec = get_ColumnVector<double>(get_Yz, mModel->mEvents);
            current_Y << x_vec, y_vec, z_vec;

            const ColumnVectorD& gx_vec = get_ColumnVector<double>(get_Gx, mModel->mEvents);
            const ColumnVectorD& gy_vec = get_ColumnVector<double>(get_Gy, mModel->mEvents);
            const ColumnVectorD& gz_vec = get_ColumnVector<double>(get_Gz, mModel->mEvents);
            current_G << gx_vec, gy_vec, gz_vec;

        } else if (mModel->compute_Y) {
            const ColumnVectorD& x_vec = get_ColumnVector<double>(get_Yx, mModel->mEvents);
            const ColumnVectorD& y_vec = get_ColumnVector<double>(get_Yy, mModel->mEvents);
            current_Y << x_vec, y_vec;

            const ColumnVectorD& gx_vec = get_ColumnVector<double>(get_Gx, mModel->mEvents);
            const ColumnVectorD& gy_vec = get_ColumnVector<double>(get_Gy, mModel->mEvents);
            current_G << gx_vec, gy_vec;

        } else {
            const ColumnVectorD& x_vec = get_ColumnVector<double>(get_Yx, mModel->mEvents);
            current_Y << x_vec;

            const ColumnVectorD& gx_vec = get_ColumnVector<double>(get_Gx, mModel->mEvents);
            current_G << gx_vec;
        }

        current_vecH = calculVecH(mModel->mEvents);
        auto [current_Q, current_R] = calculMatQR_D(current_vecH);

        SparseQuadraticFormSolver solver(1); // shift selon votre padding
        solver.factorize(current_R); // Factorisation une seule fois, crée le solver ldlt

        current_R_1QT = solver.compute_Rinv_QT(current_Q); // On passe Q, et la fonction calcul Qt pour résoudre  R*X=Qt -> X = R^{-1} * Q^T
        current_K = current_Q * current_R_1QT; // Matrice pleine

        try {

              // init the current state
#pragma mark update Theta
            if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {

                /* ----------------------------------------------------------------------
                 *  Dans Chronomodel, on appelle simplement : event->updateTheta(t_min,t_max); sur tous les events.
                 *  Pour mettre à jour un theta d'event dans Curve, on doit accéder aux thetas des autres events.
                 *  => on effectue donc la mise à jour directement ici, sans passer par une fonction
                 *  de la classe event (qui n'a pas accès aux autres events)
                 * ---------------------------------------------------------------------- */
                std::vector<double> current_vect_Theta = get_vector<double>(get_Theta, mModel->mEvents);

                double current_var_Gasser = var_Gasser(current_vect_Theta, current_Y);
                // Les vecteurs positions X, Y et Z doivent suivre l'ordre des thétas

                for (std::shared_ptr<Event>& event : initListEvents) {
                    if (event->mType == Event::eDefault) {
                        // ----
                        const double min = event->getThetaMin(tminPeriod);
                        const double max = event->getThetaMax(tmaxPeriod);

                        if (min >= max) {
                            const auto seed = mLoopChains.at(mChainIndex).mSeed;
                            throw QObject::tr("Error for event theta : %1 :\n min = %2 : max = %3 \n seed = %4").arg(event->getQStringName(), QString::number(min), QString::number(max), QString::number(seed));
                        }

                        // On stocke l'ancienne valeur :
                        current_value = event->mTheta.mX;


#define THETA_MH
#ifdef THETA_MH
                        // On tire une nouvelle valeur MH adaptatif :
                        current_h_theta = h_theta_Event(event);
                        const double try_value = Generator::normalDistribution(current_value, event->mTheta.mSigmaMH);
#else
                        // tirage double exp
                        double sum_p = 0.;
                        double sum_t = 0.;

                        for (auto&& date: event->mDates) {
                            const double variance  = pow(date.mSigmaTi.mX, 2.);
                            sum_t += (date.mTi.mX + date.mDelta) / variance;
                            sum_p += 1. / variance;
                        }
                        const double theta_avg = sum_t / sum_p;
                        const double sigma = 1. / sqrt(sum_p);
                        const double try_value = Generator::gaussByDoubleExp(theta_avg, sigma, min, max);
                        current_h_theta = 1.;
#endif

                        // try_value ne peut pas être égale à min ou max,
                        // car dans ce cas, il y a un zéro dans vec_H

                        if (try_value > min && try_value < max) {

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);

                            //auto try_Event = mModel->mEvents;
                            // On force la mise à jour de la nouvelle valeur pour calculer h_new
                            // on teste si l'ordre à changer:
                            const bool ordered = std::is_sorted(mModel->mEvents.begin(), mModel->mEvents.end(),
                                                                   [](const std::shared_ptr<Event> a, const std::shared_ptr<Event> b) {
                                                                       return a->mThetaReduced < b->mThetaReduced;
                                                                   });
                            MatrixD try_Y(n_points, n_components);
                            MatrixD try_G(n_points, n_components);
                            if (!ordered) {
                                orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants

                                // Les vecteurs positions Y, Gx, Gy et Gz doivent suivre l'ordre des thétas
                                if (mModel->compute_XYZ) {
                                    const ColumnVectorD& x_vec = get_ColumnVector<double>(get_Yx, mModel->mEvents);
                                    const ColumnVectorD& y_vec = get_ColumnVector<double>(get_Yy, mModel->mEvents);
                                    const ColumnVectorD& z_vec = get_ColumnVector<double>(get_Yz, mModel->mEvents);
                                    try_Y << x_vec, y_vec, z_vec;

                                    const ColumnVectorD& gx_vec = get_ColumnVector<double>(get_Gx, mModel->mEvents);
                                    const ColumnVectorD& gy_vec = get_ColumnVector<double>(get_Gy, mModel->mEvents);
                                    const ColumnVectorD& gz_vec = get_ColumnVector<double>(get_Gz, mModel->mEvents);

                                    try_G << gx_vec, gy_vec, gz_vec;

                                } else if (mModel->compute_Y) {
                                    const ColumnVectorD& x_vec = get_ColumnVector<double>(get_Yx, mModel->mEvents);
                                    const ColumnVectorD& y_vec = get_ColumnVector<double>(get_Yy, mModel->mEvents);
                                    try_Y << x_vec, y_vec;

                                    const ColumnVectorD& gx_vec = get_ColumnVector<double>(get_Gx, mModel->mEvents);
                                    const ColumnVectorD& gy_vec = get_ColumnVector<double>(get_Gy, mModel->mEvents);

                                    try_G << gx_vec, gy_vec;

                                } else {
                                    const ColumnVectorD& x_vec = get_ColumnVector<double>(get_Yx, mModel->mEvents);
                                    try_Y << x_vec;

                                    const ColumnVectorD& gx_vec = get_ColumnVector<double>(get_Gx, mModel->mEvents);
                                    try_G << gx_vec;
                                }


                            } else {
                                try_Y = current_Y;
                                try_G = current_G;
                            }

                            spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date

                            const std::vector<double>& try_vect_Theta = get_vector<double>(get_Theta, mModel->mEvents);

                            double try_var_Gasser = var_Gasser(try_vect_Theta, try_Y);
#ifdef THETA_MH
                            try_h_theta = h_theta_Event(event);
#else
                            try_h_theta = 1.;
#endif
                            try_vecH = calculVecH(mModel->mEvents);

                            auto [try_Q, try_R] = calculMatQR_D(try_vecH);

                            SparseQuadraticFormSolver try_solver(1); // shift selon notre padding
                            try_solver.factorize(try_R); // Factorisation une seule fois

                            MatrixD try_R_1QT = try_solver.compute_Rinv_QT(try_Q);

                            MatrixD try_K = try_Q * try_R_1QT;

                            // Calcul des rapports

                            // pseudo-déterminant (produit des valeurs propres non nulles) : le déterminant de K n'est pas définie
                            // pdet⁡(K)  =  det⁡ ⁣(R−1/2(Q⊤Q)R−1/2)  =  det⁡(Q⊤Q)det⁡(R).

                            /* t_matrix det_try_R =  determinant_padded_matrix(try_R);
                            t_matrix det_try_QtQ = determinant_padded_matrix(try_Q.transpose() * try_Q); // Qt*Q est une padded Matrix

                            t_matrix det_R =  determinant_padded_matrix(current_R);
                            t_matrix det_QtQ = determinant_padded_matrix(current_Q.transpose() * current_Q);

                            t_prob rate_detPlusK = sqrt(det_try_QtQ /det_QtQ * det_R /det_try_R);
                            */

                            MatrixD try_QtQ = try_Q.transpose() * try_Q;
                            MatrixD current_QtQ = current_Q.transpose() * current_Q;
                            t_prob rate_detPlusK = exp(0.5*(ln_rate_determinant_padded_matrix_A_B(try_QtQ, try_R)
                                                          - ln_rate_determinant_padded_matrix_A_B(current_QtQ, current_R)));

                            if (mModel->compute_XYZ) {
                                 rate_detPlusK = pow(rate_detPlusK, 3.0) ;

                            } else if (mModel->compute_Y) {
                                rate_detPlusK = pow(rate_detPlusK, 2.0) ;
                            }



                            t_prob rate_try_ftKf = rate_ftKf(current_G, current_K, try_G, try_K, mModel->mLambdaSpline.mX) ; // rate quadratic form


                            //  Le rapport du shrinkage VG dépendant de Gasser qui lui aussi dépend de theta
                            // faire une boucle sur les events dans le cas individuelle sinon une seule fois, pour VG fixe ou global


                            // Conditionnel du au shrinkage,
                            auto rate_VG = try_var_Gasser/ current_var_Gasser;
                            if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
                                rate_VG = pow(rate_VG, mPointEvent.size()); // faire puissance n pour individuelle, le nombre d'Event non-Bound;
                                for (const auto &e : mPointEvent) {
                                    rate_VG *= pow( (e->mVg.mX + current_var_Gasser) / (e->mVg.mX + try_var_Gasser), 2.0);
                                }

                            } else {
                                rate_VG *= pow((event->mVg.mX + current_var_Gasser) / (event->mVg.mX + try_var_Gasser), 2.0);

                            }


                            // fin boucle VG

                            t_prob rate_detK_ftKf = rate_detPlusK * rate_try_ftKf;
                            rate =  rate_detK_ftKf;
                            rate *= rate_VG;
                            rate *= try_h_theta / current_h_theta; // conditionnelle sur les theta

                            if (event->mTheta.test_update(current_value, try_value, rate)) {
                                // Pour l'itération suivante : toutes les matrices doivent suivre l'ordre des thetas

                                current_vecH = std::move(try_vecH);
                                current_K = std::move(try_K);                             

                                current_R_1QT = std::move(try_R_1QT);
                                current_R = std::move(try_R);
                                current_Q = std::move(try_Q);
                                current_Y = std::move(try_Y);
                                current_G = std::move(try_G);

                                current_var_Gasser = try_var_Gasser;
                                Var_residual_spline = current_var_Gasser;

                            } else {

                                event->mThetaReduced = mModel->reduceTime(current_value);
                                // Nous ne modifions pas l'ordre donc il n'est pas nécessaire de
                                // réordonner les Events
                                orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                                spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date
                            }

                        } else {
                            event->mTheta.reject_update();

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
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});

                } // End of loop initListEvents


            } else { // Pas bayésien : on sauvegarde la valeur constante dans la trace
                for (std::shared_ptr<Event>& event : initListEvents) {
                    event->mTheta.accept_update(event->mTheta.mX);

                    /* --------------------------------------------------------------
                     * C.1 - Update Alpha, Beta & Duration Phases
                     * -------------------------------------------------------------- */
                    //  Update Phases -set mAlpha and mBeta ; they coud be used by the Event in the other Phase ----------------------------------------
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});
                }

            }

            //  Update Phases Tau; they could be used by the Event in the other Phase ----------------------------------------
            /* --------------------------------------------------------------
            *  C.2 - Update Tau Phases
            * -------------------------------------------------------------- */
            std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_Tau (tminPeriod, tmaxPeriod);});

            /* --------------------------------------------------------------
            *  C.3 - Update Gamma Phases
            * -------------------------------------------------------------- */
            std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (std::shared_ptr<PhaseConstraint> pc) {pc->updateGamma();});


        } catch(...) {
            qDebug() << "[MCMCLoopCurve::update_335] Theta : Caught Exception!\n";
        }

        // --------------------------------------------------------------
        //  D - Update S02 - à faire dans la version 4 ou EDM2
        // --------------------------------------------------------------
#ifdef S02_BAYESIAN
        if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
            try {
                for (std::shared_ptr<Event> &event : initListEvents) {
                    event->updateS02();

                }
            }  catch (...) {
                qDebug() << "MCMCLoopCurve::update S02 : Caught Exception!\n";

            }
        }
#endif

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
            if (mCurveSettings.mVarianceType != CurveSettings::eModeFixed ) {
                // Events must be ordered

                /* --------------------------------------------------------------
                * F - Update mS02Vg, tau in Komlan thesis
                * -------------------------------------------------------------- */
#pragma mark update mModel->mS02Vg
                try {

                    if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
                            const std::vector<double>& vec_t = get_vector<double>(get_Theta, mModel->mEvents);

                            Var_residual_spline = var_Gasser(vec_t, current_Y);

                    }

                    mModel->mS02Vg = Var_residual_spline;// <- code d'origine


                } catch (std::exception& e) {
                    std::cout<< "[MCMCLoopCurve::update_335] S02 Vg : exception caught: " << e.what() << std::endl;

                }


                try {

                    // --------------------------------------------------------------
                    //  D-1 - Update Vg
                    // --------------------------------------------------------------

#pragma mark update Vg
                    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

                        for (std::shared_ptr<Event>& event : initListEvents)   {

                            if (event->mVg.mSamplerProposal != MHVariable::eFixe) { // rejete les Bounds


                                // Valeur actuelle
                                current_value = event->mVg.mX;
                                const t_prob W_current = event->mW;

                                // On tire une nouvelle valeur :

                                //double try_value = Generator::shrinkageUniforme(mModel->mS02Vg.mX);

                                const double try_value_log = Generator::normalDistribution(log10(current_value), event->mVg.mSigmaMH);


                                t_prob rate = -1.0;

                                // ------------------------------------------------------------------
                                // Problème de conditionnement pour factorisation LLt, il faut que la matrice A soit SPD
                                // donc lambda * W_1 * Q * B_1 * Qt < I
                                //  W_1 = Sy^2 + Vg  -> hypothèse lambda * W_1 < 1  -> W_1 < 1/ lambda
                                 // double max_sampling = (1.0 / mModel->mLambdaSpline.mX) - event->mSy * event->mSy;


                                DiagonalMatrixD W_1_current (initListEvents.size()) ; // correspond à 1.0/mW
                                std::transform(initListEvents.cbegin(), initListEvents.cend(), W_1_current.diagonal().begin(), [](std::shared_ptr<Event> ev){return 1.0/ ev->mW;});// {return (ev->mSy*ev->mSy + ev->mVg.mX;});

                                //auto B = R + mModel->mLambdaSpline.mX * Q.transpose() * W_1_current * Q;
                                // MatrixLD B_1 = inverse_padded_matrix(B);

                                // Pour tirage avec shrinkageUniforme
                                //constexpr double Min = 0.;//1.0E-20;
                                //constexpr double Max = 1.0E+10;//1E+20; // Produit des gammas trés grands
                                //if (try_value >= Min && try_value <= Max) {


                                if (try_value_log >= -20 && try_value_log <= 10) {
                                    try_value = pow(10., try_value_log);

                                    event->mVg.mX = try_value;
                                    event->updateW();

                                    // calcul direct
                                    auto r_vg = (mModel->mS02Vg + current_value) / (mModel->mS02Vg + try_value);
                                    //auto r_vg = 1.0 + (current_value - try_value) / (mModel->mS02Vg + try_value); // peut aussi s'écrire comme ca
                                    auto rate_h_vg = r_vg * r_vg;

                                    // Inverse des Poids

                                    const t_prob W_1_try  = try_value + event->mSy * event->mSy;

                                    // Pré-calcul de la différence carrée
                                    const t_prob dx = event->mYx - event->mGx;
                                    t_prob sum_sq = dx * dx;

                                    // Si on modélise Y
                                    if (mModel->compute_Y) {
                                        const t_prob dy = event->mYy - event->mGy;
                                        sum_sq += dy * dy;
                                    }

                                    // Si on modélise Z
                                    if (mModel->compute_XYZ) {
                                        const t_prob dz = event->mYz - event->mGz;
                                        sum_sq += dz * dz;
                                    }

                                    // Delta
                                    const t_prob delta_h = 0.5 * (sum_sq / W_1_try - sum_sq * W_current);

                                    // Calcul de la racine du ratio
                                    const t_prob sqrt_W_ratio = 1.0 / sqrt(W_current * W_1_try);

                                    // Exposant dépendant des dimensions modélisées
                                    const t_prob rate_sqrt_Wi = std::pow(sqrt_W_ratio, n_components);

                                    // Taux final
                                    rate = rate_sqrt_Wi * std::exp(-delta_h) * rate_h_vg;
                                    // multiplier par la jacobien
                                    rate *= try_value / current_value;

                                    event->mVg.test_update(current_value, try_value, rate);
                                    event->updateW();


                                } else {
                                    // rate = -1.; // force reject // force to keep current state
                                    event->mVg.mX = current_value;
                                    event->updateW();
                                    event->mVg.reject_update();

                                }

                            }
                        }

                    }
                    else {  // mode VG Global
#pragma mark update Vg Global
                        // QUE DEVIENNENT LES BORNES ?? -> il n'y a plus de Node, les Nodes sont transformés en Event

                        /* Si nous sommes en variance global,
                             * il faut trouver l'indice du premier Event qui ne soit pas bound
                             * L'ordre et donc l'indice change avec le spread
                             */

                        // Ici nous devons retrouver le premier Event, car plus de Node
                        auto it = std::find_if (initListEvents.begin(), initListEvents.end(),[](auto event) {return event->mPointType == Event::ePoint; });

                        // On tire une nouvelle valeur :
                        double current_value = it->get()->mVg.mX;


                        // Echantillonnage boxmuller
                        // On tire une nouvelle valeur :
                        const double try_value_log = Generator::normalDistribution(log10(current_value), it->get()->mVg.mSigmaMH);

                        if (try_value_log >= -20 && try_value_log <= 10) {
                            // Echantillonnage shrinkage
                            //try_value = Generator::shrinkageUniforme(mModel->mS02Vg.mX);
                            //auto rate_h_vg = 1 ;

                            // rapport des a priori du proposal=shrinkage, si echantillonnage avec le shrinkageUniforme() rate_h_vg = 1
                            try_value = pow(10., try_value_log);

                            try_h_VG = h_VG_Event(try_value, mModel->mS02Vg) ;
                            current_h_VG = h_VG_Event(current_value, mModel->mS02Vg);

                            // multiplier par le Jacobien, du à l'échantillonnage en log10
                            auto rate_h_vg = (try_h_VG * try_value) / (current_h_VG * current_value);

                            // current
                            double W_current = 1.0;
                            t_prob h_current = 0.0;

                            for (std::shared_ptr<Event>& ev : mModel->mEvents) {

                                W_current *= ev->mW;

                                h_current += -0.5 * pow( ev->mYx - ev->mGx, 2.0) * ev->mW;

                                if (mModel->compute_Y) {
                                    h_current += -0.5 * pow( ev->mYy - ev->mGy, 2.0) * ev->mW;
                                    W_current *= ev->mW;
                                }

                                if (mModel->compute_XYZ) {
                                    h_current += -0.5 * pow( ev->mYz - ev->mGz, 2.0) * ev->mW;
                                    W_current *= ev->mW;
                                }
                            }

                            // try
                            double W_1_try = 1.0;
                            t_prob h_try = 0.0;

                            for (std::shared_ptr<Event>& ev : initListEvents) {

                                double mW_try = 1.0 / (try_value + ev->mSy * ev->mSy);
                                W_1_try *= try_value + ev->mSy * ev->mSy;

                                h_try += -0.5 *  pow( ev->mYx - ev->mGx, 2.0) * mW_try;

                                if (mModel->compute_Y) {
                                    h_try += -0.5 *  pow( ev->mYy - ev->mGy, 2.0) * mW_try;
                                    W_1_try *= try_value + ev->mSy * ev->mSy;
                                }
                                if (mModel->compute_XYZ) {
                                    h_try += -0.5 *  pow( ev->mYz - ev->mGz, 2.0) * mW_try;
                                    W_1_try *= try_value + ev->mSy * ev->mSy;
                                }
                            }
                            auto rate_sqrt_Wi = 1.0 / sqrt(W_current * W_1_try); // Nous sommes avec W_1 donc le rapport est inversé

                            rate = rate_sqrt_Wi * exp(h_try - h_current) * rate_h_vg;


                        } else {
                            rate = -1.0;
                        }

                        // On fait le test du rapport directement, car les fonctions test_update() et try_update(), ajoutent une valeur à mLastAccept
                        bool accepted;
                        if (rate >= 1.0) {
                            // Accept unconditionally
                            accepted = true;

                        } else if (rate <= 0.0) {
                            // Reject outright
                            accepted = false;

                        } else {

                            // For rate between 0.0 and 1.0, perform a Metropolis-Hastings accept/reject step
                            const double random_number = Generator::randomUniform();

                            if (random_number <= rate) {
                                accepted = true;

                            } else {
                                accepted = false;
                            }
                        }

                        if ( accepted) {

                            // accepted
                            for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                                ev->mVg.accept_update(try_value);
                                ev->updateW();
                            }

                        } else {// rejected
                            for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                                ev->mVg.mX = current_value; // forced update, to restore the previous current_value
                                ev->updateW();
                                ev->mVg.reject_update();
                            }
                        }

                    }

                } catch (std::exception& e) {
                    std::cout << "[MCMCLoopCurve::update_335] VG : exception caught: " << e.what() << std::endl;

                } catch(...) {
                    std::cout << "[MCMCLoopCurve::update_335] update VG Event Caught Exception!" << std::endl;
                }


                // Pas bayésien : on sauvegarde la valeur constante dans la trace
            } else {
                for (std::shared_ptr<Event>& event : initListEvents) {
                    event->mVg.accept_update(mCurveSettings.mVarianceFixed);
                    // event->updateW(); //mVG never change so W never change

                }
            }

        } catch(...) {
            std::cout << "[MCMCLoopCurve::update_335] update VG Event Caught Exception!" << std::endl;
        }


        // --------------------------------------------------------------
        //  E - Update Lambda
        // --------------------------------------------------------------
#pragma mark update Lambda

        try {
            // On stocke l'ancienne valeur :
            current_value = mModel->mLambdaSpline.mX;


            if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {

                // les Events peuvent avoir changé d'ordre depuis la mise à jour des Thétas
                // et il faut vérifier que current_G soit à jour depuis update theta
                // produit f^t*K*f et la trace de la matrice K
                double sum_quadratic = quadratic_form(current_K, current_G);

                // On tire une nouvelle valeur :

                constexpr double lambda_logMin = -20.0;
                constexpr double lambda_logMax = +10.0;


                double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mLambdaSpline.mSigmaMH);

                if (try_value_log >= lambda_logMin && try_value_log <= lambda_logMax ) {

                    try_value = pow(10.0, try_value_log);

                    const double rate_try_ftKf = exp(-(try_value - current_value) * 0.5 * sum_quadratic); // rate_ftKF()

                    // Calcul du rapport de probabilité d'acceptation.
                    if (mModel->compute_XYZ) {
                        rate = rate_h_lambda_XYZ_335(current_value, try_value, n_points) ;

                    } else if (mModel->compute_Y) {
                        rate = rate_h_lambda_XY_335(current_value, try_value, n_points) ;

                    } else {
                        /**
                         * \f$ P(\lambda) = \frac{ \lambda^{\tfrac{1}{2}(n_{\text{points}} - 2)}} { \left( {c + \lambda} \right)^{\mu + 1}} \f$
                         */
                        rate = rate_h_lambda_X_335(current_value, try_value, n_points) ;
                        // multiplier par le jacobien
                        rate *= rate_try_ftKf * try_value / current_value;


                    }

                    mModel->mLambdaSpline.test_update(current_value, try_value, rate);


                } else {

                    mModel->mLambdaSpline.reject_update();
                }



            }
            // Pas bayésien : on sauvegarde la valeur constante dans la trace
            else {

                mModel->mLambdaSpline.accept_update(current_value);

            }

        } catch(...) {
            qDebug() << "[MCMCLoopCurve::update_335] Lambda : Caught Exception!\n";
        }


        // --------------------------------------------------------------
        //  F - update G(x) in MCMCSpline mModel->mSpline
        // --------------------------------------------------------------

        //-------- Simulation gaussienne multivariées des splines f
#define POSITIVECURVE
        // F.1- Calcul spline

        // Toutes les matrices doivent être à jours, aprés le passage dans update theta, et update VG met à jour event->mW
        // mModel->mSpline = samplingSpline_multi2(mModel->mEvents, current_R, current_R_1QT, current_Q); // utilise mModel->mLambdaSpline.mX et ev->mW et mets à jour lEvents[i]-> mGx = fx[i];

        // F.2 - test GPrime positive
        if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth) {
#ifdef POSITIVECURVE
            mModel->mSpline = samplingSpline_multi_depth(mModel->mEvents, current_R, current_R_1QT, current_Q);
#else
            mModel->mSpline = samplingSpline_multi2(mModel->mEvents, current_R, current_R_1QT, current_Q);
            //return hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mModel->mSettings.mTmin, mModel->mSettings.mTmax, mCurveSettings.mThreshold); // si dy >mCurveSettings.mThreshold => pas de memo de la courbe
#endif
            return hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mModel->mSettings.mTmin, mModel->mSettings.mTmax, mCurveSettings.mThreshold); // si dy >mCurveSettings.mThreshold => pas de memo de la courbe

        } else {
            mModel->mSpline = samplingSpline_multi2(mModel->mEvents, current_R, current_R_1QT, current_Q); // utilise mModel->mLambdaSpline.mX et ev->mW et mets à jour lEvents[i]-> mGx = fx[i];
            return true;
        }


    } catch (const char* e) {
        qWarning() << "[MCMCLoopCurve::update_335] char "<< e;

    } catch (const std::length_error& e) {
        qWarning() << "[MCMCLoopCurve::update_335] length_error"<< e.what();

    } catch (const std::out_of_range& e) {
        qWarning() << "[MCMCLoopCurve::update_335] out_of_range" << e.what();

    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve::update_335] "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update_335] Caught Exception!\n";
        return false;
    }

    return false;


}

#endif


#pragma mark Version 4
#if VERSION_MAJOR == 4 && VERSION_MINOR >= 0 && VERSION_PATCH >= 0

// MCMC Reinsch
QString MCMCLoopCurve::initialize_400()
{
    updateLoop = &MCMCLoopCurve::update_400;
    std::vector<std::shared_ptr<Event>>& allEvents (mModel->mEvents);

   // if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
    //    mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    //if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (std::shared_ptr<Event> &ev : allEvents) {
            if (mModel->is_curve && ev->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                ev->mTheta.mSamplerProposal = MHVariable::eDoubleExp;
            }
            if (ev->mPointType == Event::eNode)
                mNodeEvent.push_back(ev);

            else {
                mPointEvent.push_back(ev);
            }
        }
    } else {
        for (std::shared_ptr<Event> &ev : allEvents)
            mPointEvent.push_back(ev);
    }


    // -------------------------------- SPLINE Part--------------------------------
    // Init function G

    prepareEventsY(allEvents);

    emit stepChanged(tr("Initializing G ..."), 0, (int)allEvents.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    // ----------------------------------------------------------------
    //  Init Smoothing = mLambdaSpline
    // ----------------------------------------------------------------

    current_vecH = calculVecH(mModel->mEvents);
    SplineMatricesLD matricesWI = prepare_calcul_spline_WI(current_vecH);
    try {
        if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed) {
            mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;
            mModel->mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
            double memoLambda = log10(mModel->mLambdaSpline.mX);
            mModel->mLambdaSpline.memo(&memoLambda);

        } else {
            if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth ) {
                SilvermanParam sv;
                sv.force_positive_curve = true;
                sv.lambda_fixed = false;
                sv.use_error_measure = true;

                auto vec_theta_red = get_vector<double>(get_ThetaReduced, mModel->mEvents);
                const auto &vec_tmp_x = get_vector<t_matrix>(get_Yx, mModel->mEvents);
                const auto &vec_tmp_x_err = get_vector<t_matrix>(get_Sy, mModel->mEvents);

                auto init_lambda_Vg = initLambdaSplineBySilverman(sv, vec_tmp_x, vec_tmp_x_err, current_vecH);
                //auto Vg = lambda_Vg.second;
                mModel->mLambdaSpline.mX = init_lambda_Vg.first;

            } else
                mModel->mLambdaSpline.mX = 1.0E-6;

            mModel->mLambdaSpline.mLastAccepts.clear();
            mModel->mLambdaSpline.accept_update(mModel->mLambdaSpline.mX);

        }
        mModel->mLambdaSpline.mSigmaMH = 1.0;

    }  catch (...) {
        qWarning() << "Init Lambda Spline  ???";
        mAbortedReason = QString("Init Smoothing Spline  ???");
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
           // std::for_each( mModel->mEvents.begin(), mModel->mEvents.end(), [](std::shared_ptr<Event>e) { e->mW = 1.; });
            const auto var_residu_X = S02_Vg_Yx(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);
            //std::cout<<" var_residu_X = " << var_residu_X;
            if (mModel->compute_X_only) {
                Var_residual_spline = var_residu_X;

            } else {
                const auto var_residu_Y = S02_Vg_Yy(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);

                if (mModel->compute_XYZ) {
                    const auto var_residu_Z =  S02_Vg_Yz(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);
                    Var_residual_spline = (var_residu_X + var_residu_Y + var_residu_Z)/3.;

                } else {
                    Var_residual_spline = (var_residu_X + var_residu_Y)/2.;
                }

            }


        }

        // memo Vg
        /* ----------------------------------------------------------------
         * The W of the events depend only on their VG
         * During the update, we need W for the calculations of theta, VG and Lambda Spline update
         * We will thus have to update the W at each VG modification
         * We calculate it here during the initialization to have its starting value
         * ---------------------------------------------------------------- */

        int i = 0;
        //if (mCurveSettings.mUseVarianceIndividual) {
        if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
            for (std::shared_ptr<Event> &e : mPointEvent) {
                i++;
                e->mVg.mX = Var_residual_spline;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain


                if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                } else {
                    e->mVg.try_update(e->mVg.mX, 2.);
                }
                e->updateW();
                e->mVg.mSigmaMH = 1.;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }

            for (std::shared_ptr<Event> &e : mNodeEvent) {
                i++;
                e->mVg.mX = 0.;
                e->mVg.mSamplerProposal = MHVariable::eFixe;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                // check if Sy == 0
                if (e->mSy == 0) {
                    mAbortedReason = QString("Error: a Node cannot have a null error \n Change error in :%1 ").arg(e->getQStringName());
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
            // No global zero variance if no measurement error
            if (! mCurveSettings.mUseErrMesure && mCurveSettings.mVarianceType == CurveSettings::eModeFixed && mCurveSettings.mVarianceFixed == 0.) {
                mAbortedReason = QString("Warning: If no measurement error, global error std gi cannot be zero ");
                return mAbortedReason;
            }
            // No node in the case of Vg Global
            for (std::shared_ptr<Event> &e : allEvents) {
                i++;
                if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
                    e->mVg.mX = mCurveSettings.mVarianceFixed;
                    e->mVg.mSamplerProposal = MHVariable::eFixe;
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                    // Check if Sy + Vg == 0
                    if (e->mVg.mX + e->mSy * e->mSy == 0) {
                        mAbortedReason = QString("Error: a Node cannot have a null error with Variance null \n Change error in : %1").arg(e->getQStringName());
                        return mAbortedReason;
                    }

                } else {
                    e->mPointType = Event::ePoint; // force Node to be a simple Event
                    e->mVg.mX = Var_residual_spline;
                    e->mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;
                }
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                e->mVg.try_update(e->mVg.mX, 2.0);
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
   /* mModel->mS02Vg.mX = Var_residual_spline;
    mModel->mS02Vg.mLastAccepts.clear();
    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
        mModel->mS02Vg.mSamplerProposal = MHVariable::eFixe;
        double memoS02 = sqrt(mModel->mS02Vg.mX);
        mModel->mS02Vg.memo(&memoS02);

    } else {
        mModel->mS02Vg.tryUpdate(Var_residual_spline, 2.);
    }*/
    // -----------------------
    // ----------------------------------------------------------------
    // Curve init S02 Vg -- loi gamma
    // ----------------------------------------------------------------
    mModel->mS02Vg.mX = Var_residual_spline;
    mModel->mS02Vg.mLastAccepts.clear();
    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
        mModel->mS02Vg.mSamplerProposal = MHVariable::eFixe;
        double memoS02 = sqrt(mModel->mS02Vg.mX);
        mModel->mS02Vg.memo(&memoS02);

    } else {
        const double s_harmonique = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0., [](double s0, auto e) { return s0 + 1./pow(e->mSy, 2.);});
        const double mean_harmonique = sqrt((double)mModel->mEvents.size()/ s_harmonique);
        const double beta = 1.004680139*(1 - exp(- 0.0000847244 * pow(mean_harmonique, 2.373548593)));
        mModel->mSO2_beta = beta;
        // il y a une erreur de formule, il faut mettre :
        // const double S02Vg = 1. / Generator::gammaDistribution(1., 1/mModel->mSO2_beta); Vu le 2025/02/11

        const double S02Vg = 1. / Generator::gammaDistribution(1., beta);
#ifdef DEBUG
        if (S02Vg == INFINITY)
            qDebug()<<"in memo initialize_400  S02Vg == INFINITY";
#endif
        mModel->mS02Vg.try_update(S02Vg, 2.0);
    }
    mModel->mS02Vg.mSigmaMH = 1.;

    if (mModel->compute_X_only) {
        std::vector<double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

        var_Y = variance_Knuth( vecY);

    } else if (mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec) { // à controler
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecY);

        var_Y /= 2.;

    } else {
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYz;});
        var_Y += variance_Knuth( vecY);

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
        //orderEventsByThetaReduced(mModel->mEvents);
        //spreadEventsThetaReduced0(mModel->mEvents);
        current_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);
        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);
#ifdef DEBUG
        if ( mCurveSettings.mProcessType == CurveSettings::eProcess_Depth ) {
            const bool ok = hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation
            qDebug()<<"[MCMCLoopCurve::initialize_400] positive curve = "<<ok;
        }
#endif
        // init Posterior MeanG and map
        const int nbPoint = 300;// Density curve size and curve size default = 300

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapG.min_value = +INFINITY;
        clearCompo.mapG.max_value = 0;

        clearCompo.mapGP = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapGP.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapGP.min_value = +INFINITY;
        clearCompo.mapGP.max_value = 0;

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
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYx, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYx, x);});

        int i = 0;
        for (auto g : mModel->mSpline.splineX.vecG) {
            const auto e = 1.96*sqrt(mModel->mSpline.splineX.vecVarG.at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }
        clearMeanG.gx.mapG.setRangeY(minY, maxY);
        clearMeanG.gx.mapGP.setRangeY(+INFINITY, -INFINITY);

        if (mModel->compute_Y) {
            clearMeanG.gy = clearCompo;

            minY = +INFINITY;
            maxY = -INFINITY;

            minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double y, std::shared_ptr<Event> e) {return std::min(e->mYy, y);});
            maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double y, std::shared_ptr<Event> e) {return std::max(e->mYy, y);});

            int i = 0;
            for (auto g : mModel->mSpline.splineY.vecG) {
                const auto e = 1.96*sqrt(mModel->mSpline.splineY.vecVarG.at(i));
                minY = std::min(minY, g - e);
                maxY = std::max(maxY, g + e);
                i++;
            }


            clearMeanG.gy.mapG.setRangeY(minY, maxY);
            clearMeanG.gy.mapGP.setRangeY(+INFINITY, -INFINITY);

            if (mModel->compute_XYZ) {
                clearMeanG.gz = clearCompo;

                minY = +INFINITY;
                maxY = -INFINITY;

                minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double z, std::shared_ptr<Event> e) {return std::min(e->mYz, z);});
                maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double z, std::shared_ptr<Event> e) {return std::max(e->mYz, z);});

                int i = 0;
                for (auto g : mModel->mSpline.splineZ.vecG) {
                    const auto e = 1.96*sqrt(mModel->mSpline.splineZ.vecVarG.at(i));
                    minY = std::min(minY, g - e);
                    maxY = std::max(maxY, g + e);
                    i++;
                }

                clearMeanG.gz.mapG.setRangeY(minY, maxY);
                clearMeanG.gz.mapGP.setRangeY(+INFINITY, -INFINITY);
            }

        }

        // Convertion IDF
        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Vector ||  mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Spherical) {

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

            clearMeanG.gx.mapGP.setRangeY(+INFINITY, -INFINITY);
            clearMeanG.gy.mapGP.setRangeY(+INFINITY, -INFINITY);
            clearMeanG.gz.mapGP.setRangeY(+INFINITY, -INFINITY);

        }


        mModel->mPosteriorMeanGByChain.push_back(clearMeanG);
        if (mChainIndex == 0)
            mModel->mPosteriorMeanG = clearMeanG;

    }  catch (...) {
        qWarning() <<"[MCMCLoopCurve::initialize_400]init Posterior MeanG and map  ???";
    }

    /*
     * INIT UPDATE
     */
    // init the current state
    try {
        initListEvents.resize(mModel->mEvents.size());
        std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    } catch (...) {
        qWarning() <<"[MCMCLoopCurve::initialize_400] init Posterior MeanG and map  ???";
        return QString("[MCMCLoopCurve::initialize_400] problem");
    }


    return QString();
}


QString MCMCLoopCurve::initialize_401()
{
    updateLoop = &MCMCLoopCurve::update_400;
    std::vector<std::shared_ptr<Event>>& allEvents (mModel->mEvents);

    //if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
    //    mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    //if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (std::shared_ptr<Event> ev : allEvents) {
            if (mModel->is_curve && ev->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                ev->mTheta.mSamplerProposal = MHVariable::eDoubleExp;
            }
            if (ev->mPointType == Event::eNode)
                mNodeEvent.push_back(ev);

            else {
                mPointEvent.push_back(ev);
            }
        }
    } else {
        for (std::shared_ptr<Event> ev : allEvents)
            mPointEvent.push_back(ev);
    }


    // -------------------------------- SPLINE Part--------------------------------
    // Init function G

    prepareEventsY(allEvents);

    emit stepChanged(tr("Initializing G ..."), 0, (int)allEvents.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    // ----------------------------------------------------------------
    //  Init Smoothing = mLambdaSpline
    // ----------------------------------------------------------------

    current_vecH = calculVecH(mModel->mEvents);
    SplineMatricesLD matricesWI = prepare_calcul_spline_WI(current_vecH);
    try {
        if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed) {
            mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;
            mModel->mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
            double memoLambda = log10(mModel->mLambdaSpline.mX);
            mModel->mLambdaSpline.memo(&memoLambda);

        } else {
            if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth ) {
                SilvermanParam sv;
                sv.force_positive_curve = true;
                sv.lambda_fixed = false;
                sv.use_error_measure = true;

                const auto &vec_theta_red = get_vector<t_reduceTime>(get_ThetaReduced, mModel->mEvents);
                const auto &vec_tmp_x = get_vector<t_matrix>(get_Yx, mModel->mEvents);
                const auto &vec_tmp_x_err = get_vector<t_matrix>(get_Sy, mModel->mEvents);

                auto init_lambda_Vg = initLambdaSplineBySilverman(sv, vec_tmp_x, vec_tmp_x_err, current_vecH);
                //auto Vg = lambda_Vg.second;
                mModel->mLambdaSpline.mX = init_lambda_Vg.first;
            } else
                mModel->mLambdaSpline.mX = 1E-6;

            mModel->mLambdaSpline.mLastAccepts.clear();
            mModel->mLambdaSpline.try_update(mModel->mLambdaSpline.mX, 2.0);

        }
        mModel->mLambdaSpline.mSigmaMH = 1.;

    }  catch (...) {
        qWarning() << "Init Lambda Spline  ???";
        mAbortedReason = QString("Init Smoothing Spline  ???");
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
            std::for_each( mModel->mEvents.begin(), mModel->mEvents.end(), [](std::shared_ptr<Event>e) { e->mW = 1.; });
            const auto var_residu_X = S02_Vg_Yx(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);
            //std::cout<<" var_residu_X = " << var_residu_X;
            if (mModel->compute_X_only) {
                Var_residual_spline = var_residu_X;

            } else {
                const auto var_residu_Y = S02_Vg_Yy(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);

                if (mModel->compute_XYZ) {
                    const auto var_residu_Z =  S02_Vg_Yz(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);
                    Var_residual_spline = (var_residu_X + var_residu_Y + var_residu_Z)/3.;

                } else {
                    Var_residual_spline = (var_residu_X + var_residu_Y)/2.;
                }

            }


        }

        // memo Vg


        int i = 0;
        //if (mCurveSettings.mUseVarianceIndividual) {
        if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
            for (std::shared_ptr<Event> &e : mPointEvent) {
                i++;
                e->mVg.mX = Var_residual_spline;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain


                if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                } else {
                    e->mVg.try_update(e->mVg.mX, 2.0);
                }
                e->updateW();
                e->mVg.mSigmaMH = 1.;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }

            for (std::shared_ptr<Event> &e : mNodeEvent) {
                i++;
                e->mVg.mX = 0.;
                e->mVg.mSamplerProposal = MHVariable::eFixe;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                // check if Sy == 0
                if (e->mSy == 0) {
                    mAbortedReason = QString("Error: a Node cannot have a null error \n Change error in : %1").arg(e->getQStringName());
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
            // No global zero variance if no measurement error
            if (! mCurveSettings.mUseErrMesure && mCurveSettings.mVarianceType == CurveSettings::eModeFixed && mCurveSettings.mVarianceFixed == 0.) {
                mAbortedReason = QString("Warning: If no measurement error, global error std gi cannot be zero");
                return mAbortedReason;
            }
            // No node in the case of Vg Global
            for (std::shared_ptr<Event> &e : allEvents) {
                i++;
                if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
                    e->mVg.mX = mCurveSettings.mVarianceFixed;
                    e->mVg.mSamplerProposal = MHVariable::eFixe;
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                    // Check if Sy + Vg == 0
                    if (e->mVg.mX + e->mSy * e->mSy == 0) {
                        mAbortedReason = QString("Error: a Node cannot have a null error with Variance null \n Change error in : %1").arg(e->getQStringName());
                        return mAbortedReason;
                    }

                } else {
                    e->mPointType = Event::ePoint; // force Node to be a simple Event
                    e->mVg.mX = Var_residual_spline;
                    e->mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;
                }
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                e->mVg.try_update(e->mVg.mX, 2.0);
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
        mModel->mS02Vg.try_update(Var_residual_spline, 2.0);
    }

    mModel->mS02Vg.mSigmaMH = 1.;

    if (mModel->compute_X_only) {
        const auto &vecY = get_vector<t_matrix>(get_Yx, mModel->mEvents);
        //std::vector<double> vecY (mModel->mEvents.size());
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

        var_Y = variance_Knuth( vecY);

    } else if (mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec) { // à controler
        const auto &vecY = get_vector<t_matrix>(get_Yx, mModel->mEvents);
        //std::vector< double> vecY (mModel->mEvents.size());
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        const auto &vecYy = get_vector<t_matrix>(get_Yy, mModel->mEvents);
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecYy);

        var_Y /= 2.;

    } else {
        const auto &vecY = get_vector<t_matrix>(get_Yx, mModel->mEvents);
        //std::vector< double> vecY (mModel->mEvents.size());
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        const auto &vecYy = get_vector<t_matrix>(get_Yy, mModel->mEvents);
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecYy);

        const auto &vecYz = get_vector<t_matrix>(get_Yz, mModel->mEvents);
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYz;});
        var_Y += variance_Knuth( vecYz);

        var_Y /= 3.;
    }


    if ( (var_Y <= 0) && (mCurveSettings.mVarianceType != CurveSettings::eModeFixed)) {
        mAbortedReason = QString(tr("Error : Variance on Y is null, do computation with Variance G fixed  = 0 for this model "));
        return mAbortedReason;
    }
    // --------------------------- Current spline ----------------------
    try {

        //orderEventsByThetaReduced(mModel->mEvents);
        //spreadEventsThetaReduced0(mModel->mEvents);
        current_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);
        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);
#ifdef DEBUG
        if ( mCurveSettings.mProcessType == CurveSettings::eProcess_Depth ) {
            const bool ok = hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation
            qDebug()<<"[MCMCLoopCurve::initialize_400] positive curve = "<<ok;
        }
#endif
        // init Posterior MeanG and map
        const int nbPoint = 300;// Density curve size and curve size default = 300

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapG.min_value = +INFINITY;
        clearCompo.mapG.max_value = 0;

        clearCompo.mapGP = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapGP.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapGP.min_value = +INFINITY;
        clearCompo.mapGP.max_value = 0;

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
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYx, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYx, x);});

        int i = 0;
        for (auto g : mModel->mSpline.splineX.vecG) {
            const auto e = 1.96*sqrt(mModel->mSpline.splineX.vecVarG.at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }
        clearMeanG.gx.mapG.setRangeY(minY, maxY);

        clearMeanG.gx.mapGP.setRangeY(+INFINITY, -INFINITY);

        if (mModel->compute_Y) {
            clearMeanG.gy = clearCompo;

            minY = +INFINITY;
            maxY = -INFINITY;

            minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double y, std::shared_ptr<Event> e) {return std::min(e->mYy, y);});
            maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double y, std::shared_ptr<Event> e) {return std::max(e->mYy, y);});

            int i = 0;
            for (auto g : mModel->mSpline.splineY.vecG) {
                const auto e = 1.96*sqrt(mModel->mSpline.splineY.vecVarG.at(i));
                minY = std::min(minY, g - e);
                maxY = std::max(maxY, g + e);
                i++;
            }


            clearMeanG.gy.mapG.setRangeY(minY, maxY);
            clearMeanG.gy.mapGP.setRangeY(+INFINITY, -INFINITY);

            if (mModel->compute_XYZ) {
                clearMeanG.gz = clearCompo;

                minY = +INFINITY;
                maxY = -INFINITY;

                minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double z, std::shared_ptr<Event> e) {return std::min(e->mYz, z);});
                maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double z, std::shared_ptr<Event> e) {return std::max(e->mYz, z);});

                int i = 0;
                for (auto g : mModel->mSpline.splineZ.vecG) {
                    const auto e = 1.96*sqrt(mModel->mSpline.splineZ.vecVarG.at(i));
                    minY = std::min(minY, g - e);
                    maxY = std::max(maxY, g + e);
                    i++;
                }

                clearMeanG.gz.mapG.setRangeY(minY, maxY);
                clearMeanG.gz.mapGP.setRangeY(+INFINITY, -INFINITY);
            }

        }

        // Convertion IDF
        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Vector ||  mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Spherical) {

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

            clearMeanG.gx.mapGP.setRangeY(+INFINITY, -INFINITY);
            clearMeanG.gy.mapGP.setRangeY(+INFINITY, -INFINITY);
            clearMeanG.gz.mapGP.setRangeY(+INFINITY, -INFINITY);

        }


        mModel->mPosteriorMeanGByChain.push_back(clearMeanG);
        if (mChainIndex == 0)
            mModel->mPosteriorMeanG = clearMeanG;

    }  catch (...) {
        qWarning() <<"init Posterior MeanG and map  ???";
    }


    // init the current state
    try {
        initListEvents.resize(mModel->mEvents.size());
        std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    } catch (...) {
        qWarning() <<"init Posterior MeanG and map  ???";
        return QString("[MCMCLoopCurve::initialize_400] problem");
    }


    return QString();
}
#endif

// MCMC Multi-sampling gaussian
#pragma mark Version KOMLAN
#if VERSION_MAJOR == KOMLAN_old
QString MCMCLoopCurve::initialize_Komlan()
{
#ifdef DEBUG
    std::cout << "[MCMCLoopCurve::initialize_Komlan]" << std::endl;
#endif
    updateLoop = &MCMCLoopCurve::update_Komlan;
    std::vector<std::shared_ptr<Event>>& allEvents (mModel->mEvents);

    //if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
    //    mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    //if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (std::shared_ptr<Event> &ev : allEvents) {
            if (ev->mPointType == Event::eNode)
                mNodeEvent.push_back(ev);
            else
                mPointEvent.push_back(ev);

            ev->mS02Theta.mSupport = MHVariable::eRpStar;
            ev->mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;
            ev->mS02Theta.mSigmaMH = 1.;
        }
    } else {
        for (std::shared_ptr<Event> &ev : allEvents)
            mPointEvent.push_back(ev);
    }


    // -------------------------------- SPLINE part--------------------------------
    // Init function G

    prepareEventsY(allEvents);

    emit stepChanged(tr("Initializing G ..."), 0, (int)allEvents.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    // ----------------------------------------------------------------
    //  Init Lambda Spline
    // ----------------------------------------------------------------

    current_vecH = calculVecH(mModel->mEvents);
    const SplineMatricesLD &matricesWI = prepare_calcul_spline_WI(current_vecH);
    try {
        if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed) {
            mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;
            mModel->mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
            double memoLambda = log10(mModel->mLambdaSpline.mX);
            mModel->mLambdaSpline.memo(&memoLambda);

        } else {

            current_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH) ;

            auto current_math = current_splineMatricesLD ;

            const auto R = seedMatrix(current_splineMatricesLD.matR, 1); // dim n-2 * n-2
            const int np = R.rows();
            //const int mm = 3*np ;

            const auto Q = remove_bands_Matrix(current_splineMatricesLD.matQ, 1); // dim n * n-2
            const auto QT = transpose0(Q);

            MatrixLD  matRInv;
            if (np <= 3) {
                matRInv = inverseMatSym0(R, 0) ;
            } else {
                const auto& decomp = banded_Cholesky_LDLt_MoreSorensen(R, 3);// decompositionCholeskyKK(R, 3, 0);
                matRInv = choleskyInvert(decomp);
                //matRInv = inverseMatSym_originKK(decomp.first, decomp.second, mm, 0);
            }

            const auto R_1QT = multiMatParMat0(matRInv, QT);

            const auto K = multiMatParMat0(Q, R_1QT) ;

            const auto vecG = get_vector<double>(get_Yx, mModel->mEvents) ;

            auto fK = multiMatByVectCol0(K, vecG);

            // produit fK*ft et la trace de la matrice K
            double som = 0.;
            for (long i = 0; i < K.rows(); ++i) {
                som += fK[i] * vecG[i];

            }

            double meanEexp = 2. / som ;
            //const double meanEexp = som / 2.;


            if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth ) {
                bool has_positif = false;
                do {
                    std::for_each( mModel->mEvents.begin(), mModel->mEvents.end(), [](std::shared_ptr<Event>e) { e->mW = 1./e->mSy; });
                    const auto &spline_matrice = prepare_calcul_spline(mModel->mEvents, current_vecH);

                    auto spline = currentSpline(mModel->mEvents, current_vecH, spline_matrice, meanEexp, false, false);

                    has_positif =  hasPositiveGPrimePlusConst(spline.splineX);
                    if (!has_positif) {
                        meanEexp *= 10.;
                    }
                } while (has_positif || meanEexp>1.E10);
            }

            mModel->mLambdaSpline.mX =  meanEexp ; //1E-6  ;


            mModel->mLambdaSpline.mLastAccepts.clear();
            mModel->mLambdaSpline.try_update(mModel->mLambdaSpline.mX, 2.0);
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
            std::for_each( mModel->mEvents.begin(), mModel->mEvents.end(), [](std::shared_ptr<Event>e) { e->mW = 1.; });
            const auto var_residu_X = S02_Vg_Yx(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);
            //std::cout<<" var_residu_X = " << var_residu_X;
            if (mModel->compute_X_only) {
                Var_residual_spline = var_residu_X;

            } else {
                const auto var_residu_Y = S02_Vg_Yy(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);

                if ( mModel->compute_XYZ) {
                    const auto var_residu_Z =  S02_Vg_Yz(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);

                    Var_residual_spline = (var_residu_X + var_residu_Y + var_residu_Z)/3.;

                } else {
                    Var_residual_spline = (var_residu_X + var_residu_Y)/2.;
                }

            }

        }

        // memo Vg
        /* ----------------------------------------------------------------
         * The W of the events depend only on their VG
         * During the update, we need W for the calculations of theta, VG and Lambda Spline update
         * We will thus have to update the W at each VG modification
         * We calculate it here during the initialization to have its starting value
         * ---------------------------------------------------------------- */

        int i = 0;
        //if (mCurveSettings.mUseVarianceIndividual) {
        if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
            for (std::shared_ptr<Event> &e : mPointEvent) {
                i++;
                e->mVg.mX = Var_residual_spline;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain


                if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                } else {
                    e->mVg.try_update(e->mVg.mX, 2.0);
                }
                e->updateW();
                e->mVg.mSigmaMH = 1.;

                if (isInterruptionRequested())
                    return ABORTED_BY_USER;

                emit stepProgressed(i);
            }

            for (std::shared_ptr<Event> &e : mNodeEvent) {
                i++;
                e->mVg.mX = 0.;
                e->mVg.mSamplerProposal = MHVariable::eFixe;
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain

                // check if Sy == 0
                if (e->mSy == 0) {
                    mAbortedReason = QString("Error: a Node cannot have a null error \n Change error in : %1").arg(e->getQStringName());
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
            // No global zero variance if no measurement error
            if (! mCurveSettings.mUseErrMesure && mCurveSettings.mVarianceType == CurveSettings::eModeFixed && mCurveSettings.mVarianceFixed == 0.) {
                mAbortedReason = QString("Warning: If no measurement error, global error std gi cannot be zero");
                return mAbortedReason;
            }
            // Pas de Noeud dans le cas de Vg Global
            for (std::shared_ptr<Event> &e : allEvents) {
                i++;
                if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
                    e->mVg.mX = mCurveSettings.mVarianceFixed;
                    e->mVg.mSamplerProposal = MHVariable::eFixe;
                    double memoVG = sqrt(e->mVg.mX);
                    e->mVg.memo(&memoVG);

                    // Check if Sy + Vg == 0
                    if (e->mVg.mX + e->mSy * e->mSy == 0) {
                        mAbortedReason = QString("Error: a Node cannot have a null error with Variance null \n Change error in : %1").arg(e->getQStringName());
                        return mAbortedReason;
                    }

                } else {
                    e->mPointType = Event::ePoint; // force Node to be a simple Event
                    e->mVg.mX = Var_residual_spline;
                    e->mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;
                }
                e->mVg.mLastAccepts.clear();
                // e->mVg.mNbValuesAccepted->clear(); //don't clean, avalable for cumulate chain
                e->mVg.try_update(e->mVg.mX, 2.0);
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
        const double s_harmonique = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0.0, [](double s0, auto e) { return s0 + 1.0/pow(e->mSy, 2);});
        const double mean_harmonique = sqrt((double)mModel->mEvents.size()/ s_harmonique);
        const double beta = 1.004680139*(1 - exp(- 0.0000847244 * pow(mean_harmonique, 2.373548593)));
        mModel->mSO2_beta = beta;
        // il y a une erreur de formule, il faut mettre :
        // const double S02Vg = 1. / Generator::gammaDistribution(1., 1/mModel->mSO2_beta); Vu le 2025/02/11
        const double S02Vg = 1.0 / Generator::gammaDistribution(1.0, beta);

        mModel->mS02Vg.accept_update(S02Vg);
    }

    mModel->mS02Vg.mSigmaMH = 1.;

    if (mModel->compute_X_only) {
        std::vector<double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

    } else if (!mModel->compute_XYZ) {
        std::vector<double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecY);

        var_Y /= 2.;

    } else {
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYz;});
        var_Y += variance_Knuth( vecY);

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
       // orderEventsByThetaReduced(mModel->mEvents);
       // spreadEventsThetaReduced0(mModel->mEvents);
       // mModel->mSpline = currentSpline(mModel->mEvents, true);

        current_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);
        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);
        // init Posterior MeanG and map
        const int nbPoint = 300;// Density curve size and curve size

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapG.min_value = +INFINITY;
        clearCompo.mapG.max_value = 0;

        clearCompo.mapGP = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapGP.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapGP.min_value = +INFINITY;
        clearCompo.mapGP.max_value = 0;

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
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYx, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYx, x);});

        int i = 0;
        for (auto g : mModel->mSpline.splineX.vecG) {
            const auto e = 1.96*sqrt(mModel->mSpline.splineX.vecVarG.at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }

        clearMeanG.gx.mapG.setRangeY(minY, maxY);
        clearMeanG.gx.mapGP.setRangeY(+INFINITY, -INFINITY);

        if (mModel->compute_Y) {
            clearMeanG.gy = clearCompo;

            minY = +INFINITY;
            maxY = -INFINITY;

            minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double y, std::shared_ptr<Event> e) {return std::min(e->mYy, y);});
            maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double y, std::shared_ptr<Event> e) {return std::max(e->mYy, y);});


            int i = 0;
            for (auto g : mModel->mSpline.splineY.vecG) {
                const auto e = 1.96*sqrt(mModel->mSpline.splineY.vecVarG.at(i));
                minY = std::min(minY, g - e);
                maxY = std::max(maxY, g + e);
                i++;
            }


            clearMeanG.gy.mapG.setRangeY(minY, maxY);           
            clearMeanG.gy.mapGP.setRangeY(+INFINITY, -INFINITY);

            if (mModel->compute_XYZ) {
                clearMeanG.gz = clearCompo;

                minY = +INFINITY;
                maxY = -INFINITY;

                minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double z, std::shared_ptr<Event> e) {return std::min(e->mYz, z);});
                maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double z, std::shared_ptr<Event> e) {return std::max(e->mYz, z);});

                int i = 0;
                for (auto g : mModel->mSpline.splineZ.vecG) {
                    const auto e = 1.96*sqrt(mModel->mSpline.splineZ.vecVarG.at(i));
                    minY = std::min(minY, g - e);
                    maxY = std::max(maxY, g + e);
                    i++;
                }

                clearMeanG.gz.mapG.setRangeY(minY, maxY);
                clearMeanG.gz.mapGP.setRangeY(+INFINITY, -INFINITY);
            }

        }

        // Convertion IDF
        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Vector ||  mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Spherical) {

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

            clearMeanG.gx.mapGP.setRangeY(+INFINITY, -INFINITY);
            clearMeanG.gy.mapGP.setRangeY(+INFINITY, -INFINITY);
            clearMeanG.gz.mapGP.setRangeY(+INFINITY, -INFINITY);

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
#endif

#pragma mark Interpolate
/**
 * @brief MCMCLoopCurve::initialize_interpolate pour l'initialisation, il n'y a pas de VG donc équivalent à un modèle avec que des nodes
 * @return
 */

QString MCMCLoopCurve::initialize_interpolate()
{
#ifdef DEBUG
    std::cout << "[MCMCLoopCurve::initialize_interpolate]" << std::endl;
#endif

    updateLoop = &MCMCLoopCurve::update_interpolate;
    const std::vector<std::shared_ptr<Event>>& allEvents (mModel->mEvents);

    initListEvents.resize(mModel->mEvents.size());
    std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    //mCurveSettings.mUseVarianceIndividual = false;
    //mCurveSettings.mVarianceType = CurveSettings::eModeFixed;

    mNodeEvent = allEvents;
    mPointEvent.clear();


    // -------------------------------- SPLINE part--------------------------------
    // Init function G

    prepareEventsY(allEvents);

    emit stepChanged(tr("Initializing G ..."), 0, (int)allEvents.size());
    orderEventsByThetaReduced(mModel->mEvents);
    spreadEventsThetaReduced0(mModel->mEvents);

    // ----------------------------------------------------------------
    //  Init Lambda Spline
    // ----------------------------------------------------------------

    //std::vector<t_reduceTime> vecH = calculVecH(mModel->mEvents);
    //SplineMatricesLD matricesWI = prepareCalculSpline_WI(mModel->mEvents, vecH);
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
        for (std::shared_ptr<Event> e : allEvents) {

            e->mVg.mX = 0.;
            e->mVg.mSamplerProposal = MHVariable::eFixe;
            e->mVg.memo();

            e->mVg.mLastAccepts.clear();

            if (mCurveSettings.mUseErrMesure == true) {
                e->updateW();
                // Check if Sy == 0
                if ( e->mSy == 0) {
                    mAbortedReason = QString("Error: in interpolation mode, Event and Node cannot have a zero error \n Change error within : %1").arg(e->getQStringName());
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
#ifdef KOMLAN
    mModel->mS02Vg.mX = 0;
#else
    mModel->mS02Vg = 0;
#endif


    if (mModel->compute_X_only) {
        std::vector<double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

        var_Y = variance_Knuth( vecY);

    } else if (!mModel->compute_XYZ) {
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecY);

        var_Y /= 2.;

    } else {
        std::vector< double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecY);

        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYz;});
        var_Y += variance_Knuth( vecY);

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
        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);
        mModel->mSpline = currentSpline_WI(mModel->mEvents, mModel->compute_Y, mModel->compute_XYZ, mCurveSettings.mUseErrMesure);

        // init Posterior MeanG and map
        const int nbPoint = 300;// map size and curve size

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapG.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapG.setMinValue(+std::numeric_limits<double>::max());
        clearCompo.mapG.setMaxValue(0);

        clearCompo.mapGP = CurveMap (nbPoint, nbPoint);// (row, column)
        clearCompo.mapGP.setRangeX(mModel->mSettings.mTmin, mModel->mSettings.mTmax);
        clearCompo.mapGP.setMinValue(+std::numeric_limits<double>::max());
        clearCompo.mapGP.setMaxValue(0);

        clearCompo.vecG = std::vector<double> (nbPoint); // column
        clearCompo.vecVarG = std::vector<double> (nbPoint);
        clearCompo.vecGP = std::vector<double> (nbPoint);
        clearCompo.vecVarGP = std::vector<double> (nbPoint);
        clearCompo.vecGS = std::vector<double> (nbPoint);

        clearCompo.vecVarianceG = std::vector<double> (nbPoint);
        clearCompo.vecVarErrG = std::vector<double> (nbPoint);

        PosteriorMeanG clearMeanG;
        clearMeanG.gx = clearCompo;

        double minY = +std::numeric_limits<double>::max();
        double maxY = -std::numeric_limits<double>::max();
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYx, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYx, x);});


        double maxVarY = *std::max_element(mModel->mSpline.splineX.vecVarG.begin(), mModel->mSpline.splineX.vecVarG.end());
        double spanY_X =  0;
        minY = minY - 1.96*sqrt(maxVarY) - spanY_X;
        maxY = maxY + 1.96*sqrt(maxVarY) + spanY_X;

        clearMeanG.gx.mapG.setRangeY(minY, maxY);
        clearMeanG.gx.mapGP.setRangeY(+std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());

        if ( mModel->compute_Y) {
            clearMeanG.gy = clearCompo;

            minY = +std::numeric_limits<double>::max();
            maxY = -std::numeric_limits<double>::max();

            minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYy, x);});
            maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYy, x);});

            maxVarY = *std::max_element(mModel->mSpline.splineY.vecVarG.begin(), mModel->mSpline.splineY.vecVarG.end());
            spanY_X = 0;
            minY = minY - 1.96*sqrt(maxVarY) - spanY_X;
            maxY = maxY + 1.96*sqrt(maxVarY) + spanY_X;

            clearMeanG.gy.mapG.setRangeY(minY, maxY);
            clearMeanG.gy.mapGP.setRangeY(+std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());

            if (mModel->compute_XYZ) {
                clearMeanG.gz = clearCompo;

                minY = +std::numeric_limits<double>::max();
                maxY = -std::numeric_limits<double>::max();

                maxVarY = *std::max_element(mModel->mSpline.splineZ.vecVarG.begin(), mModel->mSpline.splineZ.vecVarG.end());

                minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYz, x);});
                maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYz, x);});

                spanY_X = 0;
                minY = minY - 1.96 * sqrt(maxVarY) - spanY_X;
                maxY = maxY + 1.96 * sqrt(maxVarY) + spanY_X;

                clearMeanG.gz.mapG.setRangeY(minY, maxY);
                clearMeanG.gz.mapGP.setRangeY(+std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
            }

        }

        // Convertion IDF
        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Vector ||  mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Spherical) {

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

            clearMeanG.gx.mapGP.setRangeY(+std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
            clearMeanG.gy.mapGP.setRangeY(+std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
            clearMeanG.gz.mapGP.setRangeY(+std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
        }


        mModel->mPosteriorMeanGByChain.push_back(clearMeanG);
        if (mChainIndex == 0)
            mModel->mPosteriorMeanG = clearMeanG;

    }  catch (...) {
        qWarning() <<"[MCMCLoopCurve::initialize_interpolate] init Posterior MeanG and map  ???";
    }

    /*
     * INIT UPDATE
     */
    // init the current state
    try {
        initListEvents.resize(mModel->mEvents.size());
        std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    } catch (...) {
        qWarning() <<"[MCMCLoopCurve::initialize_interpolate]init Posterior MeanG and map  ???";
        return QString("[MCMCLoopCurve::initialize_interpolate] problem");
    }
    return QString();
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

        current_splineMatrices_D = prepare_calcul_spline_D(mModel->mEvents, current_vecH);
        current_decomp_QTQ_D = decompositionCholesky(current_splineMatrices_D.matQTQ, 5, 1); // used only with update Theta

        current_decomp_matB_D = decompositionCholesky(current_splineMatrices_D.matR.toDense(), 5, 1);

        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH
        //current_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. : ln_h_YWI_3_update(current_splineMatricesLD, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, hasY, hasZ);
        //current_ln_h_YWI_3 =  0.;
        //current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);

        //current_h_lambda = 1;


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
                            date.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);
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
                for (std::shared_ptr<Event>& event : initListEvents) {
                    // Variable du MH de la spline

                    if (event->mType == Event::eDefault) {
                        const double min = event->getThetaMin(tminPeriod);
                        const double max = event->getThetaMax(tmaxPeriod);

                        if (min >= max) {
                            const auto seed = mLoopChains.at(mChainIndex).mSeed;
                            mAbortedReason = QObject::tr("Error for event theta : %1 :\n min = %2 : max = %3 \n seed = %4").arg(event->getQStringName(), QString::number(min), QString::number(max), QString::number(seed));
                            return false;
                        }

                        const double current_value = event->mTheta.mX;
                        current_h_theta = h_theta_Event(event);

                        const double try_value = Generator::normalDistribution(current_value, event->mTheta.mSigmaMH);

                        if (try_value >= min && try_value <= max) {
                            // On force la mise à jour de la nouvelle valeur pour calculer h_new

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);
                            try_h_theta = h_theta_Event(event);
                            const t_prob rate = try_h_theta /current_h_theta;
                            event->mTheta.mX = current_value;
                            event->mTheta.try_update(try_value, rate);

                            if ( event->mTheta.accept_buffer_full()) {

                                // update after tryUpdate or updateTheta
                                event->mThetaReduced = mModel->reduceTime(event->mTheta.mX);

                                /* --------------------------------------------------------------
                                 * C.1 - Update Alpha, Beta & Duration Phases
                                 * -------------------------------------------------------------- */
                                //  Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
                                std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_AlphaBeta(tminPeriod, tmaxPeriod);});

                            }


                        } else { // force rejection
                            event->mTheta.mX = current_value;
                            event->mThetaReduced = mModel->reduceTime(event->mTheta.mX);
                            event->mTheta.reject_update();

                        }

                    }

                    // If  Bound, nothing to sample. Always the same value


                } // End of loop initListEvents
                /* --------------------------------------------------------------
                 *  C.2 - Update Tau Phases
                 * -------------------------------------------------------------- */
                std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_Tau (tminPeriod, tmaxPeriod);});

                // --------------------------------------------------------------
                //  C.3 - Update Gamma Phases constraints
                // --------------------------------------------------------------
                std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (std::shared_ptr<PhaseConstraint> pc) {pc->updateGamma();});

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

        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);

        current_vecH = calculVecH(mModel->mEvents);

        current_splineMatrices_D = prepare_calcul_spline_D(mModel->mEvents, current_vecH);

        if (mCurveSettings.mUseErrMesure)
            mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices_D, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);
        else {

            mModel->mSpline = currentSpline_WI(mModel->mEvents, mModel->compute_Y, mModel->compute_XYZ, mCurveSettings.mUseErrMesure);
        }

        // G.2 - test GPrime positive
        // si dy > mCurveSettings.mThreshold = pas d'acceptation
        if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth)
            return hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mModel->mSettings.mTmin, mModel->mSettings.mTmin, mCurveSettings.mThreshold);

        else
            return true;


    } catch (const char* e) {
        qWarning() << "[MCMCLoopCurve::update_interpolate] char "<< e;

    } catch (const std::length_error& e) {
        qWarning() << "[MCMCLoopCurve::update_interpolate] length_error"<< e.what();

    } catch (const std::out_of_range& e) {
        qWarning() << "[MCMCLoopCurve::update_interpolate] out of range" << e.what();

    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve::update_interpolate]  "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update_interpolate] Caught Exception!\n";
        return false;
    }

    return false;
}



constexpr double mixing_depth = 1.0;
void MCMCLoopCurve::test_depth(std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const SplineMatricesLD &matrices, const double lambda, double &rate, bool &ok)
{

    if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth  ) {

        const auto& test_spline = currentSpline(events, vecH, matrices, lambda, false, false);

        ok = hasPositiveGPrimePlusConst(test_spline.splineX, mModel->mSettings.mTmin, mModel->mSettings.mTmin, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

        if (!ok) { // mixing depth
            const double uniform = Generator::randomUniform();
            rate = (uniform <= mixing_depth) ? rate : -1.;

        }



    } else {
        ok = true;
    }
}



#if VERSION_MAJOR == 4 && VERSION_MINOR >= 0 && VERSION_PATCH >= 0
/**
 * @brief MCMCLoopCurve::update_400 with updateS02() on Event
 * @return
 */
bool MCMCLoopCurve::update_400()
{
    try {

        t_prob rate;

        // init the current state
        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);

        current_vecH = calculVecH(mModel->mEvents);

        current_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);
        current_decomp_QTQ = decompositionCholesky(current_splineMatricesLD.matQTQ, 5, 1); // used only with update Theta

        current_decomp_matB = decomp_matB(current_splineMatricesLD, mModel->mLambdaSpline.mX);

        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH
        current_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. :
                                 ln_h_YWI_3_update(current_splineMatricesLD, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);

        if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
            current_h_lambda = 1;
        else // prevoir de mettre h_lambda_330
            current_h_lambda = h_lambda_321(current_splineMatricesLD,  (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX) ;


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
                            date.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);
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

                const MatrixLD &R = seedMatrix(current_splineMatricesLD.matR, 1); // dim n-2 * n-2
                const int np = (int)R.size();
                const int mm = 3*np ;

                const MatrixLD &Q = remove_bands_Matrix(current_splineMatricesLD.matQ, 1); // dim n * n-2
                const MatrixLD &QT = transpose0(Q);

                MatrixLD  matRInv ;
                if (np <= 3) {
                    matRInv = inverseMatSym0(R, 0) ;

                } else {
                    const auto& decomp = decompositionCholeskyKK(R, 3, 0);
                    matRInv = inverseMatSym_originKK(decomp.first, decomp.second, mm, 0);
                }

                auto K = multiMatParMat0(multiMatParMat0(Q, matRInv), QT) ;

                for (std::shared_ptr<Event>& event : initListEvents) {
#ifdef _WIN32
                    SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
#endif
                    if (event->mType == Event::eDefault) {
                        const double min = event->getThetaMin(tminPeriod);
                        const double max = event->getThetaMax(tmaxPeriod);

                        if (min >= max) {
                            const auto seed = mLoopChains.at(mChainIndex).mSeed;
                            throw QObject::tr("Error for event theta : %1 :\n min = %2 : max = %3 \n seed = %4").arg(event->getQStringName(), QString::number(min), QString::number(max), QString::number(seed));
                        }

                        // On stocke l'ancienne valeur
                        const double current_value = event->mTheta.mX;
                        //current_h_theta = h_theta_Event(event);

                        //----- Nouveau code
                        double sum_p = 0.;
                        double sum_t = 0.;

                        for (auto&& date : event->mDates) {
                            const double variance  = pow(date.mSigmaTi.mX, 2.);
                            sum_t += (date.mTi.mX + date.mDelta) / variance;
                            sum_p += 1. / variance;
                        }
                        const double theta_avg = sum_t / sum_p;
                        const double sigma = 1. / sqrt(sum_p);

                        const double try_value = Generator::gaussByDoubleExp(theta_avg, sigma, min, max);

                        if (try_value >= min && try_value <= max) {
                            // On force la mise à jour de la nouvelle valeur pour calculer h_new

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);

                            orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                            spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date

                            try_vecH = calculVecH(mModel->mEvents);
                            try_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, try_vecH);

                            const MatrixLD &try_R = seedMatrix(try_splineMatricesLD.matR, 1); // dim n-2 * n-2
                            const MatrixLD &try_Q = remove_bands_Matrix(try_splineMatricesLD.matQ, 1); // dim n * n-2

                            const MatrixLD &try_QT = transpose0(try_Q);

                            const int try_np = (int) try_R.size();
                            MatrixLD  try_matRInv ;
                            if (try_np <= 3) {
                                try_matRInv = inverseMatSym0(try_R, 0);

                            } else {
                                const std::pair<MatrixLD, DiagonalMatrixLD> &try_decomp = decompositionCholesky(try_R, 3, 0);
                                try_matRInv = inverseMatSym_origin(try_decomp, mm, 0);

                            }

                            const auto &try_K = multiMatParMat0(multiMatParMat0(try_Q, try_matRInv), try_QT);

                            const double try_h_lambda_komlan = h_lambda_Komlan(K, try_K, (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX);
                            double try_fTKf = rapport_Theta(get_Gx, mModel->mEvents, K, try_K, mModel->mLambdaSpline.mX) ;

                            if (mModel->compute_Y)
                                try_fTKf *= rapport_Theta(get_Gy, mModel->mEvents, K, try_K, mModel->mLambdaSpline.mX) ;

                            if (mModel->compute_XYZ)
                                try_fTKf *= rapport_Theta(get_Gz, mModel->mEvents, K, try_K, mModel->mLambdaSpline.mX) ;



                            const double rate_detPlus = rapport_detK_plus(K,  try_K) ;

                            rate =  rate_detPlus * try_fTKf * try_h_lambda_komlan ;
                        } else {
                            rate = -1.;

                        }


                        // restore Theta to used function tryUpdate

                        event->mTheta.mX = current_value;
                        event->mTheta.try_update(try_value, rate);


                        if ( event->mTheta.accept_buffer_full()) {
                            // Pour l'itération suivante :
                            //std::swap(current_ln_h_YWI_1_2, try_ln_h_YWI_1_2);
                            //std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);
                            //current_ln_h_YWI_1_2 = ln_h_YWI_1_2(try_decomp_QTQ, try_decomp_matB);
                            //current_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatricesLD, mModel->mEvents, try_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                            std::swap(current_vecH, try_vecH);
                            std::swap(current_splineMatricesLD, try_splineMatricesLD);
                            //current_splineMatricesLD = prepareCalculSpline(mModel->mEvents, try_vecH);

                           // std::swap(current_h_lambda, try_h_lambda);
                            //std::swap(current_decomp_matB, try_decomp_matB);
                            //current_decomp_matB = decomp_matB(current_splineMatricesLD, mModel->mLambdaSpline.mX);
                            //std::swap(current_decomp_QTQ, try_decomp_QTQ);

                            //current_decomp_QTQ = decompositionCholesky(current_splineMatricesLD.matQTQ, 5, 1);
                        }

                        // --------------------------------------------------------------
                        //  B_2 - Update S02 -- like update_Komlan()
                        // --------------------------------------------------------------
                        try {
                            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                                event->updateS02();
                        }  catch (...) {
                            qDebug() << "[MCMCLoopCurve::update_400] S02 : Caught Exception!\n";
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
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});

                } // End of loop initListEvents

                //  Update Phases Tau; they coud be used by the Event in the other Phase ----------------------------------------
                /* --------------------------------------------------------------
                 *  C.2 - Update Tau Phases
                 * -------------------------------------------------------------- */
                std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_Tau (tminPeriod, tmaxPeriod);});

                /* --------------------------------------------------------------
                 *  C.3 - Update Gamma Phases
                 * -------------------------------------------------------------- */
                std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (std::shared_ptr<PhaseConstraint> pc) {pc->updateGamma();});

            } catch(std::exception& exc) {
                qWarning() << "[MCMCLoopCurve::update] Theta : Caught Exception!\n"<<exc.what();
            }

        } else { // Pas bayésien : Tous les temps sont fixes

        }

        current_decomp_QTQ = decompositionCholesky(current_splineMatricesLD.matQTQ, 5, 1);
        current_decomp_matB = decomp_matB(current_splineMatricesLD, mModel->mLambdaSpline.mX);
        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);
        current_ln_h_YWI_3 = ln_h_YWI_3_update(current_splineMatricesLD, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

        /* --------------------------------------------------------------
         *  Remarque : à ce stade, tous les theta events sont à jour et ordonnés.
         *  On va à présent, mettre à jour tous les Vg, puis LambdaSpline.
         *  Pour cela, nous devons espacer les thetas pour permettre les calculs.
         *  Nous le faisons donc ici, et restaurerons les vrais thetas à la fin.
         * -------------------------------------------------------------- */


        /* --------------------------------------------------------------
         *  D - Update S02 Vg
         *  E.1 - Update Vg for Points only
         *  E.2 - Update Vg Global
         * -------------------------------------------------------------- */
        const double logMin = -20.;
        const double logMax = +20.;
        try {
            /* --------------------------------------------------------------
                 *  D - Update S02 Vg, à évaluer dans les deux cas: variance individuelle et globale
                 * -------------------------------------------------------------- */
            try {
                if (mCurveSettings.mVarianceType != CurveSettings::eModeFixed) {
                    // On stocke l'ancienne valeur :
                    const double  current_value = mModel->mS02Vg.mX;

                    const double try_value_log = Generator::normalDistribution(log10(mModel->mS02Vg.mX), mModel->mS02Vg.mSigmaMH);
                    const double try_value = pow(10., try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {
                        rate = h_S02_Vg_K(mModel->mEvents, current_value, try_value);//, sy);

                        rate *=  (try_value / current_value) ;

                    } else {
                        rate = -1.0;
                    }

                    mModel->mS02Vg.try_update(try_value, rate);
                }

            } catch (std::exception& e) {
                qWarning()<< "[MCMCLoopCurve::update_400] S02 Vg : exception caught: " << e.what() << '\n';

            }

            if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

                current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB); // Has not been initialized yet

               /* --------------------------------------------------------------
               *  E.1 - Update Vg for Points only, not the node
               * -------------------------------------------------------------- */

               for (std::shared_ptr<Event>& event : mPointEvent)   {

                    const double current_value = event->mVg.mX;
                    current_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                    // On tire une nouvelle valeur :
                    const double try_value_log = Generator::normalDistribution(log10(current_value), event->mVg.mSigmaMH);
                    const double try_value = pow(10., try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {
                            // On force la mise à jour de la nouvelle valeur pour calculer try_h
                            // A chaque fois qu'on modifie VG, W change !
                            event->mVg.mX = try_value;
                            event->updateW(); // used by prepareCalculSpline

                            // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG

                            try_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);
                            try_decomp_matB = decomp_matB(try_splineMatricesLD, mModel->mLambdaSpline.mX);

                            try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatricesLD, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);
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
                    event->mVg.try_update(try_value, rate);
                    event->updateW();

                    if ( event->mVg.accept_buffer_full()) {
                            // Pour l'itération suivante : Car mVg a changé
                            std::swap(current_ln_h_YWI_2, try_ln_h_YWI_2);
                            std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);
                            std::swap(current_splineMatricesLD, try_splineMatricesLD);
                            std::swap(current_decomp_matB, try_decomp_matB);
                    }
               }


            } else if (mCurveSettings.mVarianceType == CurveSettings::eModeGlobal)  {
                    /* --------------------------------------------------------------
                     *  E.2 - Update Vg Global
                     * -------------------------------------------------------------- */

                    auto& eventVGglobal = mModel->mEvents.at(0);

                    // On stocke l'ancienne valeur :
                    const double current_value = eventVGglobal->mVg.mX;

                    current_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                    // On tire une nouvelle valeur :

                    const double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mEvents.at(0)->mVg.mSigmaMH);
                    const double try_value = pow(10, try_value_log);

                    // Affectation temporaire pour évaluer la nouvelle proba
                    // Dans le cas global pas de différence ente les Points et les Nodes
                    for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                        ev->mVg.mX = try_value;
                        ev->updateW();
                    }

                    //rate = 0.;
                    if (try_value_log >= logMin && try_value_log <= logMax) {

                        // Calcul du rapport : try_matrices utilise les temps reduits, elle est affectée par le changement de Vg
                        try_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);

                        try_decomp_matB = decomp_matB(try_splineMatricesLD, mModel->mLambdaSpline.mX);

                        try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);
                        try_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. : ln_h_YWI_3_update(try_splineMatricesLD, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                        try_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                        rate = (try_h_VG * try_value) / (current_h_VG * current_value) * exp(0.5 * ( try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                    - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                        // ON fait le test avec le premier event

                        eventVGglobal->mVg.mX = current_value;
                        eventVGglobal->mVg.try_update(try_value, rate);
                        eventVGglobal->updateW();

                        if ( eventVGglobal->mVg.mLastAccepts.back()) {
                            current_ln_h_YWI_2 = std::move(try_ln_h_YWI_2);
                            current_ln_h_YWI_3 = std::move(try_ln_h_YWI_3);
                            current_splineMatricesLD = std::move(try_splineMatricesLD);
                            current_decomp_matB = std::move(try_decomp_matB);
                            rate = 2.0;

                        } else {
                            rate = -1.0;
                        }

                    } else {
                        rate = -1.0;
                    }

                    for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                        ev->mVg.mX =  eventVGglobal->mVg.mX;
                        ev->mVg.try_update(eventVGglobal->mVg.mX, rate);
                        // On remet l'ancienne valeur, qui sera éventuellement mise à jour dans ce qui suit (Metropolis Hastings)
                        // A chaque fois qu'on modifie VG, W change !
                        ev->updateW();
                    }



                // Not bayesian
            } else { // nothing to do : mCurveSettings.mVarianceType == CurveSettings::eFixed

            }

        } catch (std::exception& e) {
            qWarning()<< "[MCMCLoopCurve::update_400] VG : exception caught: " << e.what() << '\n';

        } catch(...) {
            qWarning() << "[MCMCLoopCurve::update_400] VG Event Caught Exception!\n";

        }

        /* --------------------------------------------------------------
         * F - Update Lambda
         * G - Update mModel->mSpline
         * -------------------------------------------------------------- */

        bool ok = true;
        try {
            if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {
                if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth  ) {
                        const double current_value = mModel->mLambdaSpline.mX;
                         double logMax = +10.;
                        double logMin = -20.;

                        double try_value_log ;
                        double try_value;
                        // On stocke l'ancienne valeur : prevoir h_lambda_330
                        current_h_lambda = h_lambda_321(current_splineMatricesLD, (int)mModel->mEvents.size(), current_value) ;

                        // -- Nouveau code
/*                         int counter = 10; // pour test standard counter=10


                        do {

                            // On tire une nouvelle valeur :
                            //try_value_log = Generator::normalDistribution(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                            try_value_log = Generator::gaussByDoubleExp(log10(current_value), mModel->mLambdaSpline.mSigmaMH, logMin, logMax); //nouveau code
                            try_value = pow(10., try_value_log);

                            counter++;
                            if (try_value_log >= logMin && try_value_log <= logMax) {
                                mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                                const auto try_spline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, try_value, mModel->compute_Y, mModel->compute_XYZ);
                                ok = hasPositiveGPrimePlusConst(try_spline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

                               // if (!ok)
                               //     logMin = try_value_log;

                            } else {
                                ok = false;
                            }


                        } while (!ok && counter<10);
                        //qDebug()<<"[MCMCLoopCurve::update_400] Depth counter="<<counter;
*/

                        //  -- Ancien code
                        try_value_log = Generator::normalDistribution(log10(current_value), mModel->mLambdaSpline.mSigmaMH);

                        //try_value_log = Generator::gaussByDoubleExp(log10(current_value), mModel->mLambdaSpline.mSigmaMH, logMin, logMax);
                        try_value = pow(10., try_value_log);

                        mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                        const auto try_spline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, try_value, mModel->compute_Y, mModel->compute_XYZ);
                        ok = hasPositiveGPrimePlusConst(try_spline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

                        // -- Fin ancien code


                        logMin = -20.;
                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            // Calcul du rapport :
                            mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg
                            // prévoir h_lambda_330
                            try_h_lambda = h_lambda_321(current_splineMatricesLD, (int)mModel->mEvents.size(), try_value) ;
                            try_decomp_matB = decomp_matB(current_splineMatricesLD, try_value);
                            try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatricesLD, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_XYZ);
                            try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);

                            const auto n = mModel->mEvents.size();

                            rate = (try_h_lambda * try_value) / (current_h_lambda * current_value)  * exp( 0.5 *  ( (n-2)*log(try_value/current_value)
                                                                                                                + try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                                - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                        } else {
                            rate = -1.0; // force reject
                        }

                        mModel->mLambdaSpline.mX = current_value;
                        mModel->mLambdaSpline.try_update(try_value, rate);

                        // G.1- Calcul spline
                        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                        // Il faut refaire le test car on ne sait pas si l'ancien lambda donne positif, avec les nouveaux theta.
                        if ( mModel->mLambdaSpline.accept_buffer_full()) {
                            ok = hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation
                        }
                        //qDebug()<<"[MCMCLoopCurve::update_400] Depth counter="<<counter <<" positif="<<ok<<" accept="<< ( mModel->mLambdaSpline.mLastAccepts.last() == true);

                        // Not Depth
                } else { // Not Depth
                    const double logMin = -20.;
                    const double logMax = +10.;

                    // On stocke l'ancienne valeur :
                    const double current_value = mModel->mLambdaSpline.mX;
                    // prévoir h_lambda_330
                    current_h_lambda = h_lambda_321(current_splineMatricesLD, (int)mModel->mEvents.size(), current_value) ;

                    // On tire une nouvelle valeur :
                     const double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                     const double try_value = pow(10., try_value_log);

                    //const double try_value_log = Generator::normalDistribution(log2(current_value), mModel->mLambdaSpline.mSigmaMH); // pour test komlan
                    //const double try_value = exp2(try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {

                            // Calcul du rapport :
                            mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg
                            // prévoir h_lambda_330
                            try_h_lambda = h_lambda_321(current_splineMatricesLD, (int)mModel->mEvents.size(), try_value) ;
                            try_decomp_matB = decomp_matB(current_splineMatricesLD, try_value);
                            try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatricesLD, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_XYZ);
                            try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);

                            const auto n = mModel->mEvents.size();

                            rate = (try_h_lambda * try_value) / (current_h_lambda * current_value)  * exp( 0.5 *  ( (n-2)*log(try_value/current_value)
                                                                                                                + try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                                - current_ln_h_YWI_2 - current_ln_h_YWI_3));
                    } else {
                            rate = -1.; // force reject
                    }

                    mModel->mLambdaSpline.mX = current_value;
                    mModel->mLambdaSpline.try_update(try_value, rate);

                    // G.1- Calcul spline
                    mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                    ok = true;

                }


            } else { // Nothing to do
                // Pas bayésien : on sauvegarde la valeur constante dans la trace
                // G.1- Calcul spline
                mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatricesLD, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_XYZ);

                // G.2 - test GPrime positive
                if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth  ) {
                    ok = hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

                } else
                    ok = true;

            }



        } catch(...) {
            qDebug() << "[MCMCLoopCurve::update_400] update Lambda Caught Exception!\n";
        }
        return ok;


    } catch (const char* e) {
        qWarning() << "[MCMCLoopCurve::update] char "<< e;

    } catch (const std::length_error& e) {
        qWarning() << "[MCMCLoopCurve::update] length_error"<< e.what();

    } catch (const std::out_of_range& e) {
        qWarning() << "[MCMCLoopCurve::update] out of range" << e.what();

    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve::update]  "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update] Caught Exception!\n";
        return false;
    }

    return false;
}
#endif

/**
 * @brief MCMCLoopCurve::update_Komlan, valable uniquement en univariate
 * @return
 */
#if VERSION_MAJOR == KOMLAN_old
bool MCMCLoopCurve::update_Komlan()
{
    try {

        // --------------------------------------------------------------
        //  A - Update ti Dates (idem chronomodel)
        // --------------------------------------------------------------
        try {
            if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
                for (std::shared_ptr<Event>& event : mModel->mEvents) {
                    for (auto&& date : event->mDates) {
                        date.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);
                    }
                }
            }

        }  catch (...) {
            qWarning() <<"[update_Komlan] update Date ???";
        }
        // Variable du MH de la spline

        double current_value, current_h, current_h_YWI = 0.0, current_h_VG;


        double try_value, try_h, try_h_YWI , try_h_lambda, try_h_VG ;
        SparseMatrixLD R,Q, QT, try_R, try_Q, try_QT;
        MatrixLD K, try_K, R_1QT, try_R_1QT;
        long double rapport;


        // --------------------------------------------------------------
        //  B - Update theta Events
        // --------------------------------------------------------------
        // copie la liste des pointeurs
        std::vector<std::shared_ptr<Event>> initListEvents (mModel->mEvents.size());
        std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

        std::vector<double> sy;
        sy.resize(initListEvents.size());
        std::transform(initListEvents.begin(), initListEvents.end(), sy.begin(), [](std::shared_ptr<Event> ev) {return ev->mSy;});

        std::vector<t_matrix> vectY;
        vectY.resize(initListEvents.size());
        std::transform(initListEvents.begin(), initListEvents.end(), vectY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

        std::vector<double> vectstd;
        vectstd.resize(initListEvents.size());
        std::transform(initListEvents.begin(), initListEvents.end(), vectstd.begin(), [](std::shared_ptr<Event> ev) {return sqrt(pow(ev->mSy, 2) + ev->mVg.mX);});

        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);

        current_vecH = calculVecH(mModel->mEvents);

        current_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH) ;


        auto current_math = current_splineMatricesLD ;

        R = seedMatrix(current_splineMatricesLD.matR, 1); // dim n-2 * n-2
        const int np = R.rows();
        //const int mm = 3*np ;

        Q = remove_bands_Matrix(current_splineMatricesLD.matQ, 1); // dim n * n-2
        QT = transpose0(Q);

        MatrixLD  matRInv ;
        if (np <= 3) {
            matRInv = inverseMatSym0(R, 0) ;

        } else {
            // code d'origine qui bug
            // const auto& decomp = decompositionCholeskyKK(R, 3, 0);

            // matRInv = inverseMatSym_originKK(decomp.first, decomp.second, mm, 0);


            const auto& decomp = banded_Cholesky_LDLt_MoreSorensen(R, 3);
            matRInv = choleskyInvert(decomp);
        }

        R_1QT = multiMatParMat0(matRInv, QT);

        K = multiMatParMat0(Q, R_1QT) ;


        try {

              // init the current state

            if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {

                /* ----------------------------------------------------------------------
                 *  Dans Chronomodel, on appelle simplement : event->updateTheta(t_min,t_max); sur tous les events.
                 *  Pour mettre à jour un theta d'event dans Curve, on doit accéder aux thetas des autres events.
                 *  => on effectue donc la mise à jour directement ici, sans passer par une fonction
                 *  de la classe event (qui n'a pas accès aux autres events)
                 * ---------------------------------------------------------------------- */
                //unsigned e_idx = 0;
                for (std::shared_ptr<Event>& event : initListEvents) {
                    if (event->mType == Event::eDefault) {
                        // ----
                        const double min = event->getThetaMin(tminPeriod);
                        const double max = event->getThetaMax(tmaxPeriod);

                        if (min >= max) {
                            const auto seed = mLoopChains.at(mChainIndex).mSeed;
                            throw QObject::tr("Error for event theta : %1 :\n min = %2 : max = %3 \n seed = %4").arg(event->getQStringName(), QString::number(min), QString::number(max), QString::number(seed));
                        }

                        // On stocke l'ancienne valeur :
                        current_value = event->mTheta.mX;


                        // On tire une nouvelle valeur :
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


                        if (try_value >= min && try_value <= max) {
                            // On force la mise à jour de la nouvelle valeur pour calculer h_new

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);

                            orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                            spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date

                            try_vecH = calculVecH(mModel->mEvents);
                            try_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, try_vecH);

                            try_R = seedMatrix(try_splineMatricesLD.matR, 1); // dim n-2 * n-2
                            try_Q = remove_bands_Matrix(try_splineMatricesLD.matQ, 1); // dim n * n-2

                            try_QT = transpose0(try_Q);

                            const int try_np = (int)try_R.rows();
                            MatrixLD  try_matRInv ;
                            if (try_np <= 3) {

                                try_matRInv = inverseMatSym0(try_R, 0);

                            } else {
                                // code d'origine
                                //const std::pair<MatrixLD, MatrixDiag> &try_decomp = decompositionCholesky(try_R, 3, 0);

                                //try_matRInv = inverseMatSym_origin(try_decomp, mm, 0);

                                const auto& try_decomp = banded_Cholesky_LDLt_MoreSorensen(try_R, 3);// decompositionCholeskyKK(R, 3, 0);
                                try_matRInv = choleskyInvert(try_decomp);
                            }

                            try_R_1QT = multiMatParMat0(try_matRInv, try_QT) ;

                            try_K = multiMatParMat0(try_Q, try_R_1QT);

                            try_h_lambda = h_lambda_Komlan(K, try_K, (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX);

                            // Calcul du rapport :


                            // try_h_theta = h_theta_Event(event);

                            const double try_fTKf = rapport_Theta(get_Gx, mModel->mEvents, K, try_K, mModel->mLambdaSpline.mX) ;

                            const double rapport_detPlus = rapport_detK_plus(K,  try_K) ;


                            rapport =  rapport_detPlus * try_fTKf * try_h_lambda ;

                        } else {
                            rapport = -1.;

                        }

                        // restore Theta to used function tryUpdate
                        event->mTheta.mX = current_value;
                        event->mTheta.try_update(try_value, rapport);

                        if ( event->mTheta.accept_buffer_full()) {
                            // Pour l'itération suivante :
                            //current_h_YWI = std::move(try_h_YWI);
                            current_vecH = std::move(try_vecH);
                            current_splineMatricesLD = std::move(try_splineMatricesLD);
                            //current_h_lambda = std::move(try_h_lambda);
                            K = std::move(try_K);
                            R_1QT = std::move(try_R_1QT);
                            R = std::move(try_R);
                            Q = std::move(try_Q);
                            QT = std::move(try_QT);

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
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});


                    //e_idx++;
                } // End of loop initListEvents


                // Rétablissement de l'ordre initial. Est-ce nécessaire ?
                //   std::copy(initListEvents.begin(), initListEvents.end(), mModel->mEvents.begin() );



            } else { // Pas bayésien : on sauvegarde la valeur constante dans la trace
                for (std::shared_ptr<Event>& event : initListEvents) {
                    event->mTheta.try_update(event->mTheta.mX, 1.);

                    /* --------------------------------------------------------------
                     * C.1 - Update Alpha, Beta & Duration Phases
                     * -------------------------------------------------------------- */
                    //  Update Phases -set mAlpha and mBeta ; they coud be used by the Event in the other Phase ----------------------------------------
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});
                }

            }

            //  Update Phases Tau; they coud be used by the Event in the other Phase ----------------------------------------
            /* --------------------------------------------------------------
                 *  C.2 - Update Tau Phases
                 * -------------------------------------------------------------- */
            std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_Tau (tminPeriod, tmaxPeriod);});

            /* --------------------------------------------------------------
                 *  C.3 - Update Gamma Phases
                 * -------------------------------------------------------------- */
            std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (std::shared_ptr<PhaseConstraint> pc) {pc->updateGamma();});


        } catch(...) {
            qDebug() << "MCMCLoopCurve::update Theta : Caught Exception!\n";
        }

        // --------------------------------------------------------------
        //  B_2 - Update S02
        // --------------------------------------------------------------

        try {
            for (std::shared_ptr<Event> &event : initListEvents) {
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
        //std::for_each(mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (std::shared_ptr<PhaseConstraint> pc) {pc->updateGamma();});

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
            if (mCurveSettings.mVarianceType != CurveSettings::eModeFixed) {
                // Events must be order

                const double logMin = -20.;
                const double logMax = +20.;
                try {
                    // --------------------------------------------------------------
                    //  D-1 - Update S02 Vg
                    // --------------------------------------------------------------
                    try {
                        // On stocke l'ancienne valeur :
                        current_value = mModel->mS02Vg.mX;

                        // On tire une nouvelle valeur :

                        const double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mS02Vg.mSigmaMH);

                        try_value =pow(10., try_value_log);

                        //long double rapport = -1.;

                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            const double  rapport1 = h_S02_Vg_K(mModel->mEvents, current_value, try_value);

                            rapport = rapport1 * (try_value / current_value) ;
                        } else {
                            rapport = -1.;
                        }

                        mModel->mS02Vg.mX = current_value;
                        mModel->mS02Vg.try_update(try_value, rapport);

                    } catch (std::exception& e) {
                        qWarning()<< "[MCMCLoopCurve::update_Komlan] S02 Vg : exception caught: " << e.what() << '\n';

                    }

                    // --------------------------------------------------------------
                    //  D-2 - Update Vg
                    // --------------------------------------------------------------


                    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

                        // Variance individuelle

                        unsigned e_idx = 0;
                        for (std::shared_ptr<Event>& event : initListEvents)   {

                            if(event->mVg.mSamplerProposal != MHVariable::eFixe) {

                                current_value = event->mVg.mX;
                               // current_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                                const double detWi = event->mW;

                                current_h_YWI = sqrt(detWi) * h_exp_fX_theta(event, mModel->mSpline, e_idx);

                                //current_h = current_h_YWI * current_h_VG;

                                current_h = current_h_YWI ;

                                // On tire une nouvelle valeur :

                                const double try_value = Generator::shrinkageUniforme(mModel->mS02Vg.mX);


                                constexpr double Min = 0.;
                                constexpr double Max = 1E+10;
                                if (try_value >= Min && try_value <= Max) {
                                    // On force la mise à jour de la nouvelle valeur pour calculer try_h
                                    // A chaque fois qu'on modifie VG, W change !
                                    event->mVg.mX = try_value;
                                    event->updateW(); // used by prepareCalculSpline

                                // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG

                                    const double try_detWi = event->mW;
                                    try_h_YWI = sqrt(try_detWi) * h_exp_fX_theta(event, mModel->mSpline, e_idx);

                                    try_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                                    if (try_h_YWI == HUGE_VAL || try_h_VG == HUGE_VAL) {
                                        try_h = 0.;

                                    } else {
                                        //try_h = try_h_YWI * try_h_VG;
                                        try_h = try_h_YWI ;
                                    }

                                    //rapport = (try_h * try_value) / (current_h * current_value);
                                    rapport = try_h  / current_h ;

                                } else {
                                    rapport = -1.; // force reject // force to keep current state
                                        // try_h_YWI = current_h_YWI;
                                }

                                // Mise à jour Metropolis Hastings
                                // A chaque fois qu'on modifie VG, W change !
                                event->mVg.mX = current_value;
                                event->mVg.try_update(try_value, rapport);
                                event->updateW();

                                if ( event->mVg.accept_buffer_full()) {
                                    // Pour l'itération suivante : Car mVG a changé

                                  //  current_h_YWI = std::move(try_h_YWI);
                                    current_splineMatricesLD = std::move(try_splineMatricesLD);
                                }
                                e_idx++;
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
                        current_value = eventVGglobal->mVg.mX;

                        current_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                        current_h = current_h_YWI * current_h_VG;

                        // On tire une nouvelle valeur :

                        const double try_value_log = Generator::normalDistribution(log10(current_value), mModel->mEvents.at(0)->mVg.mSigmaMH);
                        try_value = pow(10., try_value_log);

                        // affectation temporaire pour evaluer la nouvelle proba
                        for (std::shared_ptr<Event>& ev : initListEvents) {
                            ev->mVg.mX = try_value;
                            ev->updateW();
                        }

                        long double rapport = 0.;
                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            // Calcul du rapport : try_matrices utilise les temps reduits, elle est affectée par le changement de VG
                            try_splineMatricesLD = prepare_calcul_spline(mModel->mEvents, current_vecH);

                            try_h_YWI = h_YWI_AY(try_splineMatricesLD, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH);

                            try_h_VG = h_VG_Event(eventVGglobal, mModel->mS02Vg.mX);

                            /* RenCurve
                             * try_h = try_h_YWI * try_h_lambda * try_h_VG;
                             */

                            try_h = try_h_YWI  * try_h_VG;

                            //rapport = (current_h == 0) ? 1 : ((try_h * try_value) / (current_h * current_value));

                            rapport =  ((try_h * try_value) / (current_h * current_value));

                            // ON fait le test avec le premier event

                            eventVGglobal->mVg.mX = current_value;
                            eventVGglobal->mVg.try_update(try_value, rapport);
                            eventVGglobal->updateW();


                            if ( eventVGglobal->mVg.accept_buffer_full()) {
                               // current_h_YWI = try_h_YWI;
                                current_splineMatricesLD = std::move(try_splineMatricesLD);
                                rapport = 2.;

                            } else {
                                rapport = -1.;
                            }

                        } else {
                            rapport = -1.;
                        }

                        for (std::shared_ptr<Event>& ev : initListEvents) {
                            ev->mVg.mX =  eventVGglobal->mVg.mX;
                            try_value = eventVGglobal->mVg.mX;
                            ev->mVg.try_update(try_value, rapport);
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
                for (std::shared_ptr<Event>& event : initListEvents) {
                    event->mVg.accept_update(mCurveSettings.mVarianceFixed);
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

                const double Min = 0.;
                const double Max = +1000000;
                const int n = (int)K.rows();
                double som = 0;
                double vm = 0.;

                // On stocke l'ancienne valeur :
                current_value = mModel->mLambdaSpline.mX;


                auto fK = multiMatByVectCol0(K, mModel->mSpline.splineX.vecG);

                // produit fK*ft et la trace de la matrice K

                for (int i = 0; i < n; ++i) {

                    som += fK[i] * mModel->mSpline.splineX.vecG[i];

                    vm += K(i, i);
                }

                const double meanEexp = som / 2.;

                try_value = Generator::exponentialeDistribution(meanEexp);

                if (try_value  >= Min && try_value <= Max) {

                    // Calcul du rapport : probabilité d'acceptation.

                    mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_VG

                    rapport =  rate_h_lambda_K(current_value, try_value, vm, n) ;

                } else {
                    rapport = -1.; // force reject
                }

                mModel->mLambdaSpline.mX = current_value;

                mModel->mLambdaSpline.try_update(try_value, rapport);

            }
            // Pas bayésien : on sauvegarde la valeur constante dans la trace
            else {

                mModel->mLambdaSpline.accept_update(mModel->mLambdaSpline.mX);

            }

        } catch(...) {
            qDebug() << "[MCMCLoopCurve::update_Komlan] Lambda : Caught Exception!\n";
        }


        // --------------------------------------------------------------
        //  F - update MCMCSpline mModel->mSpline
        // --------------------------------------------------------------

        //-------- Simulation gaussienne multivariées des splines f

        mModel->mSpline = samplingSpline_multi(mModel->mEvents, initListEvents, vectY, vectstd, R, R_1QT, Q) ;

        // F.2 - test GPrime positive
        if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth)
            return hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy >mCurveSettings.mThreshold = pas d'acceptation

        else
            return true;

    } catch (const char* e) {
        qWarning() << "[MCMCLoopCurve::update_Komlan] char "<< e;

    } catch (const std::length_error& e) {
        qWarning() << "[MCMCLoopCurve::update_Komlan] length_error"<< e.what();
    } catch (const std::out_of_range& e) {
        qWarning() << "[MCMCLoopCurve::update_Komlan] out_of_range" << e.what();
    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve::update_Komlan] "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update_Komlan] Caught Exception!\n";
        return false;
    }

    return false;


}
#endif


/*
bool MCMCLoopCurve::adapt(const int batchIndex)
{
    // Pour un Random‑Walk Metropolis en une dimension, la théorie (Roberts et al., 1997) montre que le taux optimal est ≈ 0.44
    const double taux_min = 0.42;
    const double taux_max = 0.46;

    //
    // En haute dimension (> 5), le taux optimal se rapproche de 0.23.
    //    Si vous avez des vecteurs de grande dimension, il serait judicieux de réduire la fenêtre (ex. 0.20‑0.30).

    //const double taux_min = 0.20;
    //const double taux_max = 0.30;


    bool noAdapt = true;

    // --------------------- Adapt -----------------------------------------
    //const int sizeAdapt = 10000;

    //const double delta = (batchIndex < sizeAdapt) ? pow(sizeAdapt, -1/2.)  : pow(batchIndex, -1/2.);
    const double delta = 0.5 * std::pow(static_cast<double>(batchIndex+1), -0.5);

    for (auto& event : mModel->mEvents) {
        for (auto& date : event->mDates) {
            //--------------------- Adapt Sigma MH de t_i -----------------------------------------
            if (date.mTi.mSamplerProposal == MHVariable::eMHAdaptGauss)
                noAdapt &= date.mTi.adapt(taux_min, taux_max, delta);

            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            if (date.mSigmaTi.mSamplerProposal == MHVariable::eMHAdaptGauss)
                noAdapt &= date.mSigmaTi.adapt(taux_min, taux_max, delta);

        }

        //--------------------- Adapt Sigma MH de Theta Event -----------------------------------------
        if ((event->mType != Event::eBound) && ( event->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss) )
            noAdapt &= event->mTheta.adapt(taux_min, taux_max, delta);


        //--------------------- Adapt Sigma MH de VG  -----------------------------------------
        if ((event->mPointType != Event::eNode) && ( event->mVg.mSamplerProposal == MHVariable::eMHAdaptGauss) )
            noAdapt &= event->mVg.adapt(taux_min, taux_max, delta);

        if ( event->mS02Theta.mSamplerProposal == MHVariable::eMHAdaptGauss)
             noAdapt = event->mS02Theta.adapt(taux_min, taux_max, delta) && noAdapt;

    }

    //--------------------- Adapt Sigma MH de Lambda Spline -----------------------------------------
    if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eMHAdaptGauss)
        noAdapt &= mModel->mLambdaSpline.adapt(taux_min, taux_max, delta);

    return noAdapt;
}
*/

// fonction qui suit le principe de Robbins et Monro en utilisant batchIndex
bool MCMCLoopCurve::adapt(const int batchIndex)
{
    // Pour un Random‑Walk Metropolis en une dimension, la théorie (Roberts et al., 1997) montre que le taux optimal est ≈ 0.44
    const double taux_min = 0.42;
    const double taux_max = 0.46;

    //
    /* En haute dimension (> 5), le taux optimal se rapproche de 0.23.
        Si vous avez des vecteurs de grande dimension, il serait judicieux de réduire la fenêtre (ex. 0.20‑0.30).
    */
    //const double taux_min = 0.20;
    //const double taux_max = 0.30;


    bool noAdapt = true;

    // --------------------- Adapt -----------------------------------------

    for (auto& event : mModel->mEvents) {
        for (auto& date : event->mDates) {
            //--------------------- Adapt Sigma MH de t_i -----------------------------------------
            if (date.mTi.mSamplerProposal == MHVariable::eMHAdaptGauss)
                noAdapt &= date.mTi.adapt(taux_min, taux_max, batchIndex);

            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            if (date.mSigmaTi.mSamplerProposal == MHVariable::eMHAdaptGauss)
                noAdapt &= date.mSigmaTi.adapt(taux_min, taux_max, batchIndex);

        }

        //--------------------- Adapt Sigma MH de Theta Event -----------------------------------------
        if ((event->mType != Event::eBound) && ( event->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss) )
            noAdapt &= event->mTheta.adapt(taux_min, taux_max, batchIndex);


        //--------------------- Adapt Sigma MH de VG  -----------------------------------------
        if ((event->mPointType != Event::eNode) && ( event->mVg.mSamplerProposal == MHVariable::eMHAdaptGauss) )
            noAdapt &= event->mVg.adapt(taux_min, taux_max, batchIndex);

        if ( event->mS02Theta.mSamplerProposal == MHVariable::eMHAdaptGauss)
             noAdapt = event->mS02Theta.adapt(taux_min, taux_max, batchIndex) && noAdapt;

    }

    //--------------------- Adapt Sigma MH de Lambda Spline -----------------------------------------
    if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eMHAdaptGauss)
        noAdapt &= mModel->mLambdaSpline.adapt(taux_min, taux_max, batchIndex);

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


            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
                double memoS02 = sqrt(event->mS02Theta.mX);
                event->mS02Theta.memo(&memoS02);
                event->mS02Theta.saveCurrentAcceptRate();
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
    std::for_each(mModel->mPhases.begin(), mModel->mPhases.end(), [](std::shared_ptr<Phase> p) {p->memoAll();} );

    /* --------------------------------------------------------------
     *  D -  Memo S02 Vg - not yet Bayesian, yes if KOMLAN
     * -------------------------------------------------------------- */
#ifdef KOMLAN

    if (mModel->mS02Vg.mSamplerProposal != MHVariable::eFixe) {
        // On stocke la racine de VG, qui est une variance pour afficher l'écart-type
        //double memoS02Vg = sqrt(mModel->mS02Vg.mX);
        mModel->mS02Vg.memo();//&memoS02Vg);
        mModel->mS02Vg.saveCurrentAcceptRate();
    }

#endif
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

        double minY_GP_X = 0., minY_GP_Y = 0., minY_GP_Z = 0.;
        double maxY_GP_X = 0., maxY_GP_Y = 0., maxY_GP_Z = 0.;

        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Depth) {
            minY_GP_X = mModel->mCurveSettings.mThreshold;

        } else {
            minY_GP_X = meanG->gx.mapGP.minY();
        }
        maxY_GP_X = meanG->gx.mapGP.maxY();

        if (mModel->compute_Y) {
            minY_GP_Y = meanG->gy.mapGP.minY();
            maxY_GP_Y = meanG->gy.mapGP.maxY();

            if (mModel->compute_XYZ) {
                minY_GP_Z = meanG->gz.mapGP.minY();
                maxY_GP_Z = meanG->gz.mapGP.maxY();
            }
        }


        // New extrenum of the maps

        const int nbPtsX = 100;
        const double stepT = (mModel->mSettings.mTmax - mModel->mSettings.mTmin) / (nbPtsX - 1);

        double t;
        double gx_the, gy_the, gz_the, varGx, varGy, varGz, gpx, gpy, gpz, gs;
        unsigned i0 = 0;

        // Convertion IDF
        if (mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Vector ||  mModel->mCurveSettings.mProcessType == CurveSettings::eProcess_Spherical) {

            for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
                t = (double)idxT * stepT + mModel->mSettings.mTmin ;
                // Le premier calcul avec splineX évalue i0, qui est retourné, à la bonne position pour les deux autres splines
                valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineX, gx_the, varGx, gpx, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineY, gy_the, varGy, gpy, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineZ, gz_the, varGz, gpz, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);

                //double Inc, Dec, F;
                //convertToIDF(gx_the, gy_the, gz_the, Inc, Dec, F);

                double dIncdt, dDecdt, dFdt;
                computeDerivatives(gx_the, gy_the, gz_the, gpx, gpy, gpz, dIncdt, dDecdt, dFdt);

                minY_GP_X = std::min(dIncdt, minY_GP_X);
                maxY_GP_X = std::max(dIncdt, maxY_GP_X);

                minY_GP_Y = std::min(dDecdt, minY_GP_Y);
                maxY_GP_Y = std::max(dDecdt, maxY_GP_Y);

                minY_GP_Z = std::min(dFdt, minY_GP_Z);
                maxY_GP_Z = std::max(dFdt, maxY_GP_Z);
            }

        }  else {
            for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
                t = (double)idxT * stepT + mModel->mSettings.mTmin ;
                valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineX, gx_the, varGx, gpx, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                if (mModel->mCurveSettings.mProcessType != CurveSettings::eProcess_Depth) { // Pour la profondeur on garde: minY_GP_X = mModel->mCurveSettings.mThreshold;
                    minY_GP_X = std::min(gpx, minY_GP_X);
                }
                maxY_GP_X = std::max(gpx, maxY_GP_X);

                if (mModel->compute_Y) {
                    valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineY, gy_the, varGy, gpy, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                    minY_GP_Y = std::min(gpy, minY_GP_Y);
                    maxY_GP_Y = std::max(gpy, maxY_GP_Y);

                    if (mModel->compute_XYZ) {
                        valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineZ, gz_the, varGz, gpz, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                        minY_GP_Z = std::min(gpz, minY_GP_Z);
                        maxY_GP_Z = std::max(gpz, maxY_GP_Z);
                    }
                }

            }
        }


        if (mChainIndex == 0 ) {// do not change the Y range between several chain
            if (mModel->mCurveSettings.mProcessType != CurveSettings::eProcess_Depth) { // Pour la profondeur on garde: minY_GP_X = mModel->mCurveSettings.mThreshold;
                minY_GP_X = std::min(gpx, minY_GP_X);
            }
            //minY_GP_X = std::min(minY_GP_X, meanG->gx.mapGP.minY());
            maxY_GP_X = std::max(maxY_GP_X, meanG->gx.mapGP.maxY());
            meanG->gx.mapGP.setRangeY(minY_GP_X, maxY_GP_X);

        } else {
            minY_GP_X = std::min(minY_GP_X, chainG->gx.mapGP.minY());
            maxY_GP_X = std::max(maxY_GP_X, chainG->gx.mapGP.maxY());
         }

        chainG->gx.mapGP.setRangeY(minY_GP_X, maxY_GP_X);

        if (mModel->compute_Y) {
            if (mChainIndex == 0 ) {// do not change the Y range between several chain
                minY_GP_Y = std::min(minY_GP_Y, meanG->gy.mapGP.minY());
                maxY_GP_Y = std::max(maxY_GP_Y, meanG->gy.mapGP.maxY());
                meanG->gy.mapGP.setRangeY(minY_GP_Y, maxY_GP_Y);

            } else {
                minY_GP_Y = std::min(minY_GP_Y, meanG->gy.mapGP.minY());
                maxY_GP_Y = std::max(maxY_GP_Y, meanG->gy.mapGP.maxY());
            }
            chainG->gy.mapGP.setRangeY(minY_GP_Y, maxY_GP_Y);

            if (mModel->compute_XYZ) {
                if (mChainIndex == 0 ) {// do not change the Y range between several chain

                    minY_GP_Z = std::min(minY_GP_Z, meanG->gz.mapGP.minY());
                    maxY_GP_Z = std::max(maxY_GP_Z, meanG->gz.mapGP.maxY());
                    meanG->gz.mapGP.setRangeY(minY_GP_Z, maxY_GP_Z);

                } else {
                    minY_GP_Z = std::min(minY_GP_Z, meanG->gz.mapGP.minY());
                    maxY_GP_Z = std::max(maxY_GP_Z, meanG->gz.mapGP.maxY());
                }
                chainG->gz.mapGP.setRangeY(minY_GP_Z, maxY_GP_Z);
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

    int iterAccepted = mLoopChains[mChainIndex].mRealyAccepted + 1;
    int totalIterAccepted = 1;
    for (auto& c : mLoopChains)
        totalIterAccepted += c.mRealyAccepted;

    if (mModel->compute_X_only) {
        mModel->memo_PosteriorG(mModel->mPosteriorMeanGByChain[mChainIndex].gx, mModel->mSpline.splineX, iterAccepted );
        if (mLoopChains.size() > 1)
            mModel->memo_PosteriorG( mModel->mPosteriorMeanG.gx, mModel->mSpline.splineX, totalIterAccepted);

    } else if (!mModel->compute_XYZ) {
        mModel->memo_PosteriorG( mModel->mPosteriorMeanGByChain[mChainIndex].gx, mModel->mSpline.splineX, iterAccepted );
        mModel->memo_PosteriorG( mModel->mPosteriorMeanGByChain[mChainIndex].gy, mModel->mSpline.splineY, iterAccepted );

        if (mLoopChains.size() > 1) {
            mModel->memo_PosteriorG( mModel->mPosteriorMeanG.gx, mModel->mSpline.splineX, totalIterAccepted);
            mModel->memo_PosteriorG( mModel->mPosteriorMeanG.gy, mModel->mSpline.splineY, totalIterAccepted);
        }

    }
    else {


#if VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH >= 5
        mModel->memo_PosteriorG_3D_335( mModel->mPosteriorMeanGByChain[mChainIndex], mModel->mSpline, mCurveSettings.mProcessType, iterAccepted);

        if (mLoopChains.size() > 1)
            mModel->memo_PosteriorG_3D_335( mModel->mPosteriorMeanG, mModel->mSpline, mCurveSettings.mProcessType, totalIterAccepted);



#else
        mModel->memo_PosteriorG_3D( mModel->mPosteriorMeanGByChain[mChainIndex], mModel->mSpline, mCurveSettings.mProcessType, iterAccepted);

        if (mLoopChains.size() > 1)
            mModel->memo_PosteriorG_3D( mModel->mPosteriorMeanG, mModel->mSpline, mCurveSettings.mProcessType, totalIterAccepted);
#endif
    }

}


/*
void MCMCLoopCurve::finalize()
{

#ifdef DEBUG
    qDebug()<<QString("[MCMCLoopCurve::finalize]");
    QElapsedTimer startTime;
    startTime.start();
#endif

    // Removing traces of chains without accepted curves

    int back_position = (int)mModel->mLambdaSpline.mRawTrace->size();
    size_t i = 0;

    for (auto chain = mLoopChains.rbegin(); chain != mLoopChains.rend(); chain++) {
        // we add 1 for the init
        const int initBurnAdaptAcceptSize = 1 + chain->mIterPerBurn + int (chain->mBatchIndex * chain->mIterPerBatch) + chain->mRealyAccepted;

        const int front_position = back_position - initBurnAdaptAcceptSize;
        if (chain->mRealyAccepted == 0) {
            emit setMessage((tr("Warning : NO POSITIVE curve available with chain n° %1, current seed to change %2").arg (QString::number(i+1), QString::number(chain->mSeed))));

            mLoopChains.erase(mLoopChains.cbegin()+i);

            mModel->mPosteriorMeanGByChain.erase(mModel->mPosteriorMeanGByChain.begin() + i);

            if (mModel->mLambdaSpline.mSamplerProposal != MHVariable::eFixe ) {
                mModel->mLambdaSpline.mRawTrace->erase(mModel->mLambdaSpline.mRawTrace->cbegin() + front_position, mModel->mLambdaSpline.mRawTrace->cbegin() + back_position);
                mModel->mLambdaSpline.mHistoryAcceptRateMH->erase(mModel->mLambdaSpline.mHistoryAcceptRateMH->cbegin() + front_position, mModel->mLambdaSpline.mHistoryAcceptRateMH->cbegin() + back_position);
                mModel->mLambdaSpline.mNbValuesAccepted.erase(mModel->mLambdaSpline.mNbValuesAccepted.begin() + i);
            }

            if (mModel->mS02Vg.mSamplerProposal != MHVariable::eFixe ) {
                mModel->mS02Vg.mRawTrace->erase(mModel->mS02Vg.mRawTrace->cbegin() + front_position, mModel->mS02Vg.mRawTrace->cbegin() + back_position);
                mModel->mS02Vg.mHistoryAcceptRateMH->erase(mModel->mS02Vg.mHistoryAcceptRateMH->cbegin() + front_position, mModel->mS02Vg.mHistoryAcceptRateMH->cbegin() + back_position);
                mModel->mS02Vg.mNbValuesAccepted.erase(mModel->mS02Vg.mNbValuesAccepted.begin() + i);
            }

            for (const auto &ev : mModel->mEvents) {
                if (ev->mTheta.mSamplerProposal != MHVariable::eFixe) {
                    ev->mTheta.mRawTrace->erase(ev->mTheta.mRawTrace->cbegin() + front_position, ev->mTheta.mRawTrace->cbegin() + back_position);
                    ev->mTheta.mHistoryAcceptRateMH->erase(ev->mTheta.mHistoryAcceptRateMH->cbegin() + front_position, ev->mTheta.mHistoryAcceptRateMH->cbegin() + back_position);
                    ev->mTheta.mNbValuesAccepted.erase(ev->mTheta.mNbValuesAccepted.begin() + i);

                    if (!ev->mVg.mRawTrace->empty() && ev->mVg.mSamplerProposal != MHVariable::eFixe ) {
                        ev->mVg.mRawTrace->erase(ev->mVg.mRawTrace->cbegin() + front_position, ev->mVg.mRawTrace->cbegin() + back_position);
                        ev->mVg.mHistoryAcceptRateMH->erase(ev->mVg.mHistoryAcceptRateMH->cbegin() + front_position, ev->mVg.mHistoryAcceptRateMH->cbegin() + back_position);
                        ev->mVg.mNbValuesAccepted.erase(ev->mVg.mNbValuesAccepted.begin() + i);
                    }

                    if (!ev->mS02Theta.mRawTrace->empty() && ev->mVg.mSamplerProposal != MHVariable::eFixe) {
                        ev->mS02Theta.mRawTrace->erase(ev->mS02Theta.mRawTrace->cbegin() + front_position, ev->mS02Theta.mRawTrace->cbegin() + back_position);
                        ev->mS02Theta.mHistoryAcceptRateMH->erase(ev->mS02Theta.mHistoryAcceptRateMH->cbegin() + front_position, ev->mS02Theta.mHistoryAcceptRateMH->cbegin() + back_position);
                        ev->mS02Theta.mNbValuesAccepted.erase(ev->mS02Theta.mNbValuesAccepted.begin() + i);
                    }

                    for (auto &d : ev->mDates) {
                        d.mTi.mRawTrace->erase(d.mTi.mRawTrace->cbegin() + front_position, d.mTi.mRawTrace->cbegin() + back_position);
                        d.mTi.mHistoryAcceptRateMH->erase(d.mTi.mHistoryAcceptRateMH->cbegin() + front_position, d.mTi.mHistoryAcceptRateMH->cbegin() + back_position);
                        d.mTi.mNbValuesAccepted.erase(d.mTi.mNbValuesAccepted.begin() + i);

                        d.mSigmaTi.mRawTrace->erase(d.mSigmaTi.mRawTrace->cbegin() + front_position, d.mSigmaTi.mRawTrace->cbegin() + back_position);
                        d.mSigmaTi.mHistoryAcceptRateMH->erase(d.mSigmaTi.mHistoryAcceptRateMH->cbegin() + front_position, d.mSigmaTi.mHistoryAcceptRateMH->cbegin() + back_position);
#ifdef DEBUG
                        if (d.mSigmaTi.mHistoryAcceptRateMH->empty()) {
                            qDebug()<<"MCMCLoopCurve::finalize";
                        }
#endif
                        d.mSigmaTi.mNbValuesAccepted.erase(d.mSigmaTi.mNbValuesAccepted.begin() + i);

                        d.mWiggle.mRawTrace->erase(d.mWiggle.mRawTrace->cbegin() + front_position, d.mWiggle.mRawTrace->cbegin() + back_position);
                        d.mWiggle.mNbValuesAccepted.erase(d.mWiggle.mNbValuesAccepted.begin() + i);
                    }
                }
            }
        }
        back_position = front_position ;
        i++;
    }
    if (mLoopChains.empty()) {
        mAbortedReason = QString(tr("Warning : NO POSITIVE curve available "));
        throw mAbortedReason;
    }

    // This is not a copy of all data!
    // Chains only contains description of what happened in the chain (numIter, numBatch adapt, ...)
    // Real data are inside mModel members (mEvents, mPhases, ...)
    mModel->mChains = mLoopChains;

    // This is called here because it is calculated only once and will never change afterwards
    // This is very slow : it is for this reason that the results display may be long to appear at the end of MCMC calculation.

    emit setMessage(tr("Computing posterior distributions and numerical results"));
    mModel->generateCorrelations(mLoopChains);

    // This should not be done here because it uses resultsView parameters
    // ResultView will trigger it again when loading the model
    mModel->initDensities();

    // ----------------------------------------
    // Curve specific :
    // ----------------------------------------

    std::vector<MCMCSplineComposante> allChainsTraceX, chainTraceX, allChainsTraceY, chainTraceY, allChainsTraceZ, chainTraceZ;

    // find the min in the map, can't be done when we do the map

    for (auto& pmc : mModel->mPosteriorMeanGByChain) {
        pmc.gx.mapG.min_value = *std::min_element(begin(pmc.gx.mapG.data), end(pmc.gx.mapG.data));
        pmc.gx.mapGP.min_value = *std::min_element(begin(pmc.gx.mapGP.data), end(pmc.gx.mapGP.data));

        if (mModel->compute_Y) {
            pmc.gy.mapG.min_value = *std::min_element(begin(pmc.gy.mapG.data), end(pmc.gy.mapG.data));
            pmc.gy.mapGP.min_value = *std::min_element(begin(pmc.gy.mapGP.data), end(pmc.gy.mapGP.data));

            if (mModel->compute_XYZ) {
                pmc.gz.mapG.min_value = *std::min_element(begin(pmc.gz.mapG.data), end(pmc.gz.mapG.data));
                pmc.gz.mapGP.min_value = *std::min_element(begin(pmc.gz.mapGP.data), end(pmc.gz.mapGP.data));
            }
        }


    }


    // if there is one chain, the mPosteriorMeanG is the mPosteriorMeanGByChain[0]
    if (mLoopChains.size() == 1) {
        mModel->mPosteriorMeanG = mModel->mPosteriorMeanGByChain[0];

    } else {
        mModel->mPosteriorMeanG.gx.mapG.min_value = *std::min_element(begin(mModel->mPosteriorMeanG.gx.mapG.data), end(mModel->mPosteriorMeanG.gx.mapG.data));
        mModel->mPosteriorMeanG.gx.mapGP.min_value = *std::min_element(begin(mModel->mPosteriorMeanG.gx.mapGP.data), end(mModel->mPosteriorMeanG.gx.mapGP.data));

        if (mModel->compute_Y) {
            mModel->mPosteriorMeanG.gy.mapG.min_value = *std::min_element(begin(mModel->mPosteriorMeanG.gy.mapG.data), end(mModel->mPosteriorMeanG.gy.mapG.data));
            mModel->mPosteriorMeanG.gy.mapGP.min_value = *std::min_element(begin(mModel->mPosteriorMeanG.gy.mapGP.data), end(mModel->mPosteriorMeanG.gy.mapGP.data));

            if (mModel->compute_XYZ) {
                mModel->mPosteriorMeanG.gz.mapG.min_value = *std::min_element(begin(mModel->mPosteriorMeanG.gz.mapG.data), end(mModel->mPosteriorMeanG.gz.mapG.data));
                mModel->mPosteriorMeanG.gz.mapGP.min_value = *std::min_element(begin(mModel->mPosteriorMeanG.gz.mapGP.data), end(mModel->mPosteriorMeanG.gz.mapGP.data));
            }
        }
    }



#ifdef DEBUG
    QTime endTime = QTime::currentTime();
    qDebug()<<tr("[MCMCLoopCurve::finalize] finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug()<<tr("Total time elapsed %1").arg(QString(DHMS(startTime.elapsed())));
#endif


}
*/

void MCMCLoopCurve::finalize()
{
#ifdef DEBUG
    qDebug()<<QString("[MCMCLoopCurve::finalize]");
    QElapsedTimer startTime;
    startTime.start();
#endif

    // Helper utilitaires
    auto safe_erase_range = [](auto &vec, int front, int back) {
        // vec must support size(), begin(), erase()
        if (front < 0) front = 0;
        if (back < 0) return;
        const auto sz = static_cast<int>(vec.size());
        if (front >= sz || front >= back) return;
        if (back > sz) back = sz;
        vec.erase(vec.begin() + front, vec.begin() + back);
    };

    auto safe_erase_index = [](auto &vec, size_t idx) {
        if (idx < vec.size()) vec.erase(vec.begin() + static_cast<std::ptrdiff_t>(idx));
    };

    struct RemovalInfo {
        size_t idx_from_begin; // index dans mLoopChains (depuis begin)
        int front;             // front_position
        int back;              // back_position
        bool remove;
        QString warning;
    };

    // ----------------------------------------
    // A - Suppression des traces correspondant au courbe non positives
    // ----------------------------------------

    // 1) Phase d'analyse (lecture seule) : parcourir en reverse pour reproduire la logique
    std::vector<RemovalInfo> removals;
    removals.reserve(mLoopChains.size());

    int back_position = static_cast<int>(mModel->mLambdaSpline.mRawTrace->size());
    size_t reverse_i = 0;

    for (auto it = mLoopChains.rbegin(); it != mLoopChains.rend(); ++it, ++reverse_i) {
        const auto &chain = *it;
        const int initBurnAdaptAcceptSize =
            1 + chain.mIterPerBurn + static_cast<int>(chain.mBatchIndex * chain.mIterPerBatch) + chain.mRealyAccepted;

        const int front_position = back_position - initBurnAdaptAcceptSize;

        RemovalInfo info;
        // convert reverse index to forward index
        info.idx_from_begin = static_cast<size_t>(mLoopChains.size() - 1 - reverse_i);
        info.front = front_position;
        info.back = back_position;
        info.remove = (chain.mRealyAccepted == 0);

        if (info.remove) {
            info.warning = tr("Warning : NO POSITIVE curve available with chain n° %1, current seed to change %2")
                               .arg(QString::number(reverse_i + 1), QString::number(chain.mSeed));
        }

        removals.push_back(std::move(info));

        back_position = front_position;
    }

    // 2) Phase de suppression : rassembler indices à supprimer et supprimer par ordre décroissant
    std::vector<size_t> indices_to_erase;
    indices_to_erase.reserve(removals.size());
    for (const auto &r : removals) if (r.remove) indices_to_erase.push_back(r.idx_from_begin);

    /* 👉 Sur macOS, toute création de fenêtre (NSWindow, QMessageBox, QDialog, …) doit se faire dans le thread principal GUI.
     * Si tu appelles QMessageBox::warning(...) depuis ton MCMCLoopCurve::finalize() alors que ça tourne dans un thread de calcul MCMC,
     * Qt va tenter de créer une NSWindow depuis un thread secondaire → crash immédiat.
     */
/*
    if (!indices_to_erase.empty()) {
        // Construire le texte de warning
        QString msg = tr("The following chains have no positive curves and will be removed:\n\n");

        for (size_t idx : indices_to_erase) {
            const auto &r = removals.at(idx);  // récupération du RemovalInfo
            const auto &chain = mLoopChains.at(idx);
            msg += tr(" - Chain #%1 (seed %2)\n")
                       .arg(r.idx_from_begin + 1)
                       .arg(chain.mSeed);

        }

       // QMessageBox::warning(nullptr,
         //                    tr("Invalid chains"),
           //                  msg);



    }
*/


    if (!indices_to_erase.empty()) {

        std::sort(indices_to_erase.begin(), indices_to_erase.end(), std::greater<size_t>());

        // Build map idx -> RemovalInfo for quick lookup
        std::unordered_map<size_t, RemovalInfo> remap;
        remap.reserve(removals.size());
        for (const auto &r : removals) if (r.remove) remap.emplace(r.idx_from_begin, r);

        // For convenience, take references/pointers to commonly used containers
        auto &lambda = mModel->mLambdaSpline;
#ifdef KOMLAN
        auto &S02Vg = mModel->mS02Vg;
#endif
        auto &posteriorByChain = mModel->mPosteriorMeanGByChain;
        auto &events = mModel->mEvents;

        for (size_t idx : indices_to_erase) {
            const auto &r = remap.at(idx);
            const int front = r.front;
            const int back = r.back;

            // Emit warning (sequential, GUI-safe)
            //emit setMessage(r.warning);
#ifdef DEBUG
            qDebug()<<tr("[MCMCLoopCurve::finalize] %1").arg(r.warning);
#endif
            // 1) erase chain structure
            safe_erase_index(mLoopChains, idx);

            // 2) erase posterior per-chain
            safe_erase_index(posteriorByChain, idx);

            // 3) erase global traces (lambda)
            if (lambda.mSamplerProposal != MHVariable::eFixe) {
                if (lambda.mRawTrace) safe_erase_range(*lambda.mRawTrace, front, back);
                if (lambda.mHistoryAcceptRateMH) safe_erase_range(*lambda.mHistoryAcceptRateMH, front, back);
                safe_erase_index(lambda.mNbValuesAccepted, idx);
            }
#ifdef KOMLAN
            // 3) erase global traces (lambda)
            if (S02Vg.mSamplerProposal != MHVariable::eFixe) {
                if (S02Vg.mRawTrace) safe_erase_range(*S02Vg.mRawTrace, front, back);
                if (S02Vg.mHistoryAcceptRateMH) safe_erase_range(*S02Vg.mHistoryAcceptRateMH, front, back);
                safe_erase_index(S02Vg.mNbValuesAccepted, idx);
            }
#endif
            // 4) events
            for (const auto &ev : events) {
                if (ev->mTheta.mSamplerProposal != MHVariable::eFixe) {
                    if (ev->mTheta.mRawTrace) safe_erase_range(*ev->mTheta.mRawTrace, front, back);
                    if (ev->mTheta.mHistoryAcceptRateMH) safe_erase_range(*ev->mTheta.mHistoryAcceptRateMH, front, back);
                    safe_erase_index(ev->mTheta.mNbValuesAccepted, idx);

                    if (!ev->mVg.mRawTrace->empty() && ev->mVg.mSamplerProposal != MHVariable::eFixe) {
                        safe_erase_range(*ev->mVg.mRawTrace, front, back);
                        safe_erase_range(*ev->mVg.mHistoryAcceptRateMH, front, back);
                        safe_erase_index(ev->mVg.mNbValuesAccepted, idx);
                    }

                    if (!ev->mS02Theta.mRawTrace->empty() && ev->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
                        safe_erase_range(*ev->mS02Theta.mRawTrace, front, back);
                        safe_erase_range(*ev->mS02Theta.mHistoryAcceptRateMH, front, back);
                        safe_erase_index(ev->mS02Theta.mNbValuesAccepted, idx);
                    }

                    for (auto &d : ev->mDates) {
                        if (d.mTi.mRawTrace) safe_erase_range(*d.mTi.mRawTrace, front, back);
                        if (d.mTi.mHistoryAcceptRateMH) safe_erase_range(*d.mTi.mHistoryAcceptRateMH, front, back);
                        safe_erase_index(d.mTi.mNbValuesAccepted, idx);

                        if (d.mSigmaTi.mRawTrace) safe_erase_range(*d.mSigmaTi.mRawTrace, front, back);
                        if (d.mSigmaTi.mHistoryAcceptRateMH) safe_erase_range(*d.mSigmaTi.mHistoryAcceptRateMH, front, back);
                        safe_erase_index(d.mSigmaTi.mNbValuesAccepted, idx);

                        if (d.mWiggle.mRawTrace) safe_erase_range(*d.mWiggle.mRawTrace, front, back);
                        safe_erase_index(d.mWiggle.mNbValuesAccepted, idx);
                    }
                }
            }
        } // for each idx to erase
    } // if any to erase

    // After deletions check
    if (mLoopChains.empty()) {
        mAbortedReason = QString(tr("Warning : NO POSITIVE curve available "));
        throw mAbortedReason;
    }

    // ----------------------------------------
    // B - Copy remaining chains into model
    // ----------------------------------------
    mModel->mChains = mLoopChains;

    // Compute correlations & densities (unchanged)
    emit setMessage(tr("Computing posterior distributions and numerical results"));
    mModel->generateCorrelations(mLoopChains);
    mModel->initDensities();

    // ----------------------------------------
    // C - Curve specific : compute min_values
    // ----------------------------------------
    auto compute_min_in_map = [](auto &mapContainer) {
        // assume mapContainer.data is a container
        if (!mapContainer.isEmpty()) {
            mapContainer.setMinValue(*std::min_element(mapContainer.begin(), mapContainer.end()));
        }
    };

    for (auto &pmc : mModel->mPosteriorMeanGByChain) {
        compute_min_in_map(pmc.gx.mapG);
        compute_min_in_map(pmc.gx.mapGP);

        if (mModel->compute_Y) {
            compute_min_in_map(pmc.gy.mapG);
            compute_min_in_map(pmc.gy.mapGP);

            if (mModel->compute_XYZ) {
                compute_min_in_map(pmc.gz.mapG);
                compute_min_in_map(pmc.gz.mapGP);
            }
        }
    }

    if (mLoopChains.size() == 1) {
        mModel->mPosteriorMeanG = mModel->mPosteriorMeanGByChain[0];
    } else {
        compute_min_in_map(mModel->mPosteriorMeanG.gx.mapG);
        compute_min_in_map(mModel->mPosteriorMeanG.gx.mapGP);

        if (mModel->compute_Y) {
            compute_min_in_map(mModel->mPosteriorMeanG.gy.mapG);
            compute_min_in_map(mModel->mPosteriorMeanG.gy.mapGP);

            if (mModel->compute_XYZ) {
                compute_min_in_map(mModel->mPosteriorMeanG.gz.mapG);
                compute_min_in_map(mModel->mPosteriorMeanG.gz.mapGP);
            }
        }
    }

#ifdef DEBUG
    qDebug()<<tr("[MCMCLoopCurve::finalize] elapsed ms: %1").arg(startTime.elapsed());
#endif
}


double MCMCLoopCurve::Calcul_Variance_Rice (const std::vector<std::shared_ptr<Event> > &events) const
{
    // Calcul de la variance Rice (1984) 
    double Var_Rice = 0.0;
    const size_t size = events.size();

    if (size <= 1) {
        return 0.0; // Si le tableau a 1 élément ou est vide, la variance est 0.
    }

    for (size_t i = 1; i < size; ++i) {
        double diff = events[i]->mYx - events[i-1]->mYx;
        Var_Rice += diff * diff;
    }

    Var_Rice = 0.5 * Var_Rice / (size - 1);

    return Var_Rice;
}



#pragma mark Related to : calibrate

void MCMCLoopCurve::prepareEventsY(const std::vector<std::shared_ptr<Event>> &events)
{
    std::for_each( events.begin(), events.end(), [this](std::shared_ptr<Event> e) { prepareEventY(e); });
}

/**
 * Préparation des valeurs Yx, Yy, Yz et Sy à partir des valeurs saisies dans l'interface : Yinc, Ydec, Sinc, Yint, Sint
 */
void MCMCLoopCurve::prepareEventY(const std::shared_ptr<Event> event  )
{
    const double rad = M_PI / 180.;

    switch (mCurveSettings.mProcessType) {
        case CurveSettings:: eProcess_Inclination:
             event->mYx = event->mXIncDepth;
             event->mSy = event->mS_XA95Depth/2.448;

             event->mYy = 0;
             event->mYz = 0;
        break;
        case CurveSettings::eProcess_Univariate:
        case CurveSettings::eProcess_Depth:
            event->mYx = event->mXIncDepth;
            event->mSy = event->mS_XA95Depth;

            event->mYy = 0;
            event->mYz = 0;
            break;

        case CurveSettings::eProcess_2D:
            event->mZField = 100.; // To treat the 2D case, we use the 3D case by setting Yint = 100
            event->mS_ZField = 0.;

            event->mYx = event->mXIncDepth;
            event->mYy = event->mYDec;
            event->mYz = event->mZField;

            event->mSy = sqrt( (pow(event->mS_Y, 2.) + pow(event->mS_XA95Depth, 2.)) /2.);
            break;

        case CurveSettings::eProcess_3D:
            event->mYx = event->mXIncDepth;
            event->mYy = event->mYDec;
            event->mYz = event->mZField;

            event->mSy = sqrt( (pow(event->mS_ZField, 2.) + pow(event->mS_Y, 2.) + pow(event->mS_XA95Depth, 2.)) /3.);
            break;

        case CurveSettings::eProcess_Declination:
            event->mYx = event->mYDec;
            event->mSy = (event->mS_XA95Depth / cos(event->mXIncDepth * rad))/2.448; //ligne 364 : EctYij:=(1/(sqrt(Kij)*cos(Iij*rad)))*Deg;
            event->mYy = 0;
            event->mYz = 0;
            break;

        case CurveSettings::eProcess_Field:
            event->mYx = event->mZField;
            event->mSy = event->mS_ZField;
            event->mYy = 0;
            event->mYz = 0;
            break;

        case CurveSettings::eProcess_Spherical:
            event->mZField = 100.; // To treat the spherical case, we use the vector case by posing Yint = 100
            event->mS_ZField = 0.;

            event->mYx = event->mZField * cos(event->mXIncDepth * rad) * cos(event->mYDec * rad);
            event->mYy = event->mZField * cos(event->mXIncDepth * rad) * sin(event->mYDec * rad);
            event->mYz = event->mZField * sin(event->mXIncDepth * rad);

            event->mSy = event->mZField * event->mS_XA95Depth *rad /2.448; // ligne 537 : EctYij:= Fij/sqrt(Kij);
            break;

        case CurveSettings::eProcess_Unknwon_Dec:
            event->mYx = event->mXIncDepth;
            event->mYy = event->mZField;

            event->mSy = sqrt( (pow(event->mS_ZField, 2.) +  pow(event->mZField * event->mS_XA95Depth *rad /2.448, 2.) ) / 2.);

            event->mYz = 0;
            break;

        case CurveSettings::eProcess_Vector:
            event->mYx = event->mZField * cos(event->mXIncDepth * rad) * cos(event->mYDec * rad);
            event->mYy = event->mZField * cos(event->mXIncDepth * rad) * sin(event->mYDec * rad);
            event->mYz = event->mZField * sin(event->mXIncDepth * rad);

            event->mSy = sqrt( (pow(event->mS_ZField, 2.) + 2. * pow(event->mZField * event->mS_XA95Depth *rad /2.448, 2.) ) /3.); // ligne 520 : EctYij:=sqrt( ( sqr(EctFij) + (2*sqr(Fij)/Kij) )/3 );
            break;

        case CurveSettings::eProcess_None:
        default:
            event->mYx = 0;
            event->mYy = 0;
            event->mYz = 0;

            event->mSy = 0;
            break;
    }

    if (!mCurveSettings.mUseErrMesure) {
        event->mSy = 0.;
    }

}


#pragma mark Related to : update : calcul de h_new




t_prob MCMCLoopCurve::h_lambda_321(const SplineMatricesLD &matrices, const int nb_noeuds, const double lambdaSpline)
{
    /* initialisation de l'exposant mu du prior "shrinkage" sur lambda : fixe
       en posant mu=2, la moyenne a priori sur alpha est finie = (nb_noeuds-2)/somme(Mat_W_1K[i,i]) ;
       et la variance a priori sur lambda est infinie
       NB : si on veut un shrinkage avec espérance et variance finies, il faut mu >= 3
    */

    constexpr int mu = 3;
    const t_prob c = S02_lambda_WI(matrices, nb_noeuds);

    // prior "shrinkage"
    return pow(c, mu) / pow(c + lambdaSpline, mu+1); //((mu/c) * pow(c/(c + lambdaSpline), mu+1));

}

t_prob MCMCLoopCurve::h_lambda_330(const double lambdaSpline)
{
    /* initialisation de l'exposant mu du prior "shrinkage" sur lambda : fixe
       en posant mu=2, la moyenne a priori sur alpha est finie = (nb_noeuds-2)/somme(Mat_W_1K[i,i]) ;
       et la variance a priori sur lambda est infinie
       NB : si on veut un shrinkage avec espérance et variance finies, il faut mu >= 3
    */

    constexpr int mu = 3;
    auto c = mModel->mC_lambda ;//(nb_noeuds-2.0)/(11.24 * std::pow(nb_noeuds, 4.0659))/ var_Y; // hypothese theta réparti uniforme

    // prior "shrinkage"
    return pow(c, mu) / pow(c + lambdaSpline, mu+1); //((mu/c) * pow(c/(c + lambdaSpline), mu+1));

}


/* ancienne fonction U_cmt_MCMC:: h_Vgij dans RenCurve
* lEvents.size() must be geater than 2
*/
/**
 * @brief Computes the probability density function for a given event.
 *
 * This function calculates the probability density based on the inverse gamma
 * distribution parameters. The formula used is:
 *
 * \f[
 *     h(Vg_{e})= \frac{S02\_Vg^{a+1}}{(S02\_Vg + Vg_{e})^{a+1}} \times \frac{1}{S02\_Vg} = \frac{S02\_Vg^{a}}{(S02\_Vg + Vg_{e})^{a+1}}
 * \f]
 *
 * where:
 * - \( S02\_Vg \) is the scale parameter.
 * - \( x \) is the event'sVG value (e->mVg.mX).
 * - \( a \) is the shape parameter, currently set to 1.
 * \f[
 *    h(Vg_{e}) = \frac{S02\_Vg}{(S02\_Vg + Vg_{e}) \cdot (S02\_Vg + Vg_{e})}
 * ]
 * This implementation optimizes the calculation by precomputing intermediate
 * values and simplifying the expression to reduce computational overhead.
 *
 * @param e A reference to the event for which the probability is calculated.
 * @param S02_Vg The scale parameter of the inverse gamma distribution.
 * @return The probability density for the given event.
 */
t_prob MCMCLoopCurve::h_VG_Event(const std::shared_ptr<Event>& e, const double S02_Vg) const
{
    //const int a = 1;
    //const int a = 3; // version du 2022-06-17 - avant l'introduction de l'inverse gamma sur S02_Vg
    //int a = 2;
    //return pow(S02_Vg/(S02_Vg + e->mVg.mX), a+1) / S02_Vg;

    /* const int a = 1;
     const double S02_power = S02_Vg; // a=1 donc pow(S02_Vg, a) = S02_Vg;
     return S02_power / S02_x_power;
    */
    const double S02_plus_x = S02_Vg + e->mVg.mX;
    const double S02_x_power = S02_plus_x * S02_plus_x ; // a=1 donc pow(S02_plus_x, a + 1);


    return S02_Vg / S02_x_power;
}

t_prob MCMCLoopCurve::h_VG_Event(const double Vg, const double S02_Vg) const
{
    //const int a = 1;
    //const int a = 3; // version du 2022-06-17 - avant l'introduction de l'inverse gamma sur S02_Vg
    //int a = 2;
    //return pow(S02_Vg/(S02_Vg + e->mVg.mX), a+1) / S02_Vg;

    /* const int a = 1;
     const double S02_power = S02_Vg; // a=1 donc pow(S02_Vg, a) = S02_Vg;
     return S02_power / S02_x_power;
    */
    const double S02_plus_x = S02_Vg + Vg;
    const double S02_x_power = S02_plus_x * S02_plus_x ; // a=1 donc pow(S02_plus_x, a + 1);


    return S02_Vg / S02_x_power;
}

/*
 * This calculation does not differentiate between points and nodes
 */
t_prob MCMCLoopCurve::h_S02_Vg(const std::vector<std::shared_ptr<Event>> &events, double S02_Vg) const
{
    //const double alp = 1.;
    //const t_prob prior = pow(1./S02_Vg, alp+1.) * exp(-alp/S02_Vg);
    const t_prob prior =  exp(-1/S02_Vg) / pow(S02_Vg, 2.);
    const int a = 1;

    //if (mCurveSettings.mUseVarianceIndividual) {
    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

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
 * @brief Calculates the h function for S02_Vg_K in MCMC sampling
 *
 * @details Mathematical formulation:
 * @f[
 * h = prior \cdot prod\_h\_Vg
 * @f]
 * where:
 * @f[
 * prior = \left( \frac{S02\_Vg }{ try\_S02}\right)^{\alpha +1} \cdot \exp(-\beta \cdot \frac{(S02\_Vg - try\_S02)}{(try\_S02 \times S02\_Vg)})
 * @f]
 *
 * @param events Vector of shared pointers to Event objects
 * @param S02_Vg Current value of the S02 variance parameter
 * @param try_S02 Proposed new value for the S02 variance parameter
 * @return t_prob The calculated h value
 */
 /*
t_prob MCMCLoopCurve::h_S02_Vg_K(const std::vector<std::shared_ptr<Event>>& events, const double S02_Vg, const double try_S02) const
{
    // Constantes et pré-calculs
    constexpr int alpha = 1;
    constexpr int a = 1;
    const t_prob beta = static_cast<t_prob>(mModel->mSO2_beta);
    const size_t n_DefaultEvent = mPointEvent.size();

    // Pré-calcul des ratios fréquemment utilisés
    const t_prob s02_ratio = static_cast<t_prob>(S02_Vg) / try_S02;
    const t_prob ln_s02_ratio = std::log(s02_ratio);

    if (events.empty()) return 0.0;

    t_prob prod_h_Vg;

    // if (mCurveSettings.mUseVarianceIndividual) {
    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        // Calcul parallélisé de la somme des logarithmes
        t_prob log_sum = 0.0;

        #pragma omp parallel reduction(+:log_sum)
        {
            #pragma omp for nowait
            for (size_t i = 0; i < events.size(); ++i) {
                const auto& event = events[i];
                if (event->mPointType == Event::ePoint) {
                    const t_prob vg_i = static_cast<t_prob>(event->mVg.mX);
                    log_sum += std::log((S02_Vg + vg_i) / (try_S02 + vg_i));
                }
            }
        }

        // Calcul final en utilisant les logarithmes
        const t_prob ln_prod_h_Vg = n_DefaultEvent * (-ln_s02_ratio) + (a + 1) * log_sum;
        prod_h_Vg = std::exp(ln_prod_h_Vg);

    } else {
        // Optimisation pour le cas variance global
        const t_prob vg_0 = static_cast<t_prob>(events[0]->mVg.mX);
        const t_prob ratio = (S02_Vg + vg_0) / (try_S02 + vg_0);
        prod_h_Vg = std::exp((a + 1) * std::log(ratio)) / s02_ratio;
    }

    // Calcul optimisé du prior
    const t_prob prior_exp = (alpha + 1) * ln_s02_ratio -  beta * (S02_Vg - try_S02) / (try_S02 * S02_Vg);
    const t_prob prior = std::exp(prior_exp);

    return prior * prod_h_Vg;
}
*/
#ifdef KOMLAN
t_prob MCMCLoopCurve::h_S02_Vg_K(const std::vector<std::shared_ptr<Event>>& events, const double S02_Vg, const double try_Vg)
{

    const int a = 1;

    double prod_h_Vg = 1.;
    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (auto& e : events) {
            prod_h_Vg *= (exp((a + 1)*log((S02_Vg + e->mVg.mX) / (try_Vg + e->mVg.mX))) * (try_Vg / S02_Vg)) ;
        }

    } else {
        prod_h_Vg = (exp((a + 1)*log((S02_Vg + events[0]->mVg.mX) / (try_Vg + events[0]->mVg.mX))) * (try_Vg / S02_Vg));
    }

    /*const double s_harmonique = std::accumulate(initListEvents.begin(), initListEvents.end(), 0., [] (double s0, auto e){return s0 + 1. / pow(e->mSy, 2.);});

    const double ecart = sqrt(events.size() / s_harmonique) ;
*/
    const int alpha = 1;

    //const double beta =  1.004680139*(1 - exp(- 0.0000847244 * pow(ecart, 2.373548593)));

    const t_prob beta = static_cast<t_prob>(mModel->mSO2Vg_beta);

    const double prior1 = exp((alpha + 1)*log(S02_Vg / try_Vg)) * exp(-beta * ((S02_Vg - try_Vg) / (try_Vg * S02_Vg)))   ;

    const double prior = prior1 * prod_h_Vg;

    return prior ;

}
#endif


/**
 * @brief Calculates the acceptance rate for S02_Vg parameter in MCMC sampling
 *
 * This function computes the acceptance rate for the variance parameter S02_Vg.
 * It uses an inverse gamma prior distribution
 * with alpha=1 and beta=1, and handles both individual and global variance cases.
 *
 * @details Mathematical formulation:
 *
 * Prior ratio (inverse gamma distribution):
 *  * @f[
 * r_{prior} = \left(\frac{S02\_Vg}{try\_S02}\right)^2 \exp\left(\frac{1}{S02\_Vg} - \frac{1}{try\_S02}\right)
 * @f]
 *
 * For individual variance (mUseVarianceIndividual = true):
 * @f[
 * r = r_{prior} \cdot \exp\left(n \cdot\ln\left(\frac{try\_S02}{S02\_Vg}\right) + 2\cdot\ln\left(\prod_{i=1}^n \frac{S02\_Vg + Vg_{i}}{try\_S02 + Vg_{i}}\right)\right)
 * @f]
 * where @f$n@f$ is the number of events of type ePoint and @f$Vg_{i}@f$ is the variance of event i
 *
 * For global variance (mUseVarianceIndividual = false):
 * @f[
 * r = r_{prior} \cdot \left(\frac{try\_S02}{try\_S02 + Vg_{0}} \cdot \frac{S02\_Vg + V_g{0}}{S02\_Vg}\right)^2 \cdot \frac{S02\_Vg}{try\_S02}
 * @f]
 * where @f$Vg_{0}@f$ is the variance of the first event
 *
 * @param pointEvents Vector of shared pointers to Event objects containing of type ePoint
 * @param S02_Vg Current value of the S02 variance parameter
 * @param try_S02 Proposed new value for the S02 variance parameter
 *
 * @return t_prob The acceptance rate for the proposed parameter change
 *
 * @note When mUseVarianceIndividual is true, the function uses an optimized accumulation
 *       algorithm that can be parallelized. Otherwise, it uses a simpler calculation
 *       for global variance.
 *
 * Implementation details:
 * 1. For individual variance (mUseVarianceIndividual = true):
 *    - Uses std::accumulate for optimized computation
 *    - Calculates product of variance ratios for all events of type ePoint
 * 2. For global variance (mUseVarianceIndividual = false):
 *    - Uses a simpler calculation based on the first event only
 *    - Applies a power law with parameter a = 1
 */
t_prob MCMCLoopCurve::rate_h_S02_Vg(const std::vector<std::shared_ptr<Event>> &pointEvents, double S02_Vg, double try_S02) const
{
   // const double alp = 1.;
    // loi inverse gamma avec alp=1 et beta = 1
/*    const t_prob r_prior = pow((t_prob)S02_Vg/try_S02, 2.) * exp(1./(t_prob)S02_Vg - 1./try_S02);
    const t_prob n_pointEvents = pointEvents.size();

    if (mCurveSettings.mUseVarianceIndividual) {

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
 */

    // Pré-calcul des valeurs fréquemment utilisées
        const t_prob s02_ratio = static_cast<t_prob>(S02_Vg) / try_S02;
        const t_prob inv_diff = 1.0 / static_cast<t_prob>(S02_Vg) - 1.0 / try_S02;
        const t_prob r_prior = s02_ratio * s02_ratio * std::exp(inv_diff);

        const size_t n_pointEvents = pointEvents.size();
        if (n_pointEvents == 0) return 0.0;

        // if (mCurveSettings.mUseVarianceIndividual) {
        if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
            // Utilisation de log-sum au lieu de produit pour éviter les dépassements numériques
            t_prob log_sum = 0.0;

            #pragma omp parallel reduction(+:log_sum)
            {
                #pragma omp for nowait
                for (size_t i = 0; i < n_pointEvents; ++i) {
                    const t_prob vg_i = static_cast<t_prob>(pointEvents[i]->mVg.mX);
                    const t_prob ratio = (S02_Vg + vg_i) / (try_S02 + vg_i);
                    log_sum += std::log(ratio);
                }
            }

            // Calcul final en utilisant les logarithmes
            const t_prob ln_prod_h_Vg = n_pointEvents * std::log(try_S02/S02_Vg) + 2.0 * log_sum;
            return r_prior * std::exp(ln_prod_h_Vg);

        } else {
            // Optimisation pour le cas de variance collective
            const t_prob vg_0 = static_cast<t_prob>(pointEvents[0]->mVg.mX);
            const t_prob ratio = (try_S02 / (try_S02 + vg_0)) *
                               ((S02_Vg + vg_0) / S02_Vg);
            return r_prior * ratio * ratio * s02_ratio;
        }

}

/* Identique à la fonction rate_h_S02_Vg, mais test si Event est un point ou un Noeud
 * Et fait le calcul avec les points
 *
 */
t_prob MCMCLoopCurve::rate_h_S02_Vg_test(const std::vector<std::shared_ptr<Event> > &events, double S02_Vg, double try_S02) const
{
   // const double alp = 1.;
    const t_prob r_prior = pow(S02_Vg/try_S02, 2.0) * exp(1./S02_Vg - 1.0/try_S02);
    const int n_DefaultEvent = std::accumulate(events.begin(), events.end(), 1., [] (t_prob sum, auto e) {
        return  (e->mPointType == Event::eNode ? sum + 1. : sum);
    });

    //if (mCurveSettings.mUseVarianceIndividual) {
    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

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
        const t_prob ln_prod_h_Vg  = n_DefaultEvent * log(try_S02/S02_Vg) + 2*log(prod_h_Vg_accumulate) ;
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
double MCMCLoopCurve::S02_Vg_Yx(std::vector<std::shared_ptr<Event>> &events, const SplineMatricesLD &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline)
{

    const auto& vecY = get_vector<t_matrix>(get_Yx, events);
    return var_residual(vecY, matricesWI, vecH, lambdaSpline);

    //vecY.resize(events.size());
    //std::transform(events.begin(), events.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});


    // const MCMCSpline &splineWI = currentSpline(events, vecH, matricesWI, lambdaSpline, mModel->compute_Y, mModel->compute_XYZ); // peut-être utiliser doSplineX()
    // const std::vector< double> &vecG = splineWI.splineX.vecG;
    //  Mat_B = R + alpha * Qt * W-1 * Q
    //const MatrixLD matB = addMatEtMat(matricesWI.matR, multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    //std::pair<MatrixLD, std::vector< MatrixLD::value_type::value_type>> decomp = decompositionCholesky(matB, 5, 1);

    // ---
    //const std::vector<double> &vec_theta_red = get_vector(get_ThetaReduced, events);

    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    /*MatrixLD matB; //matR;

    if (lambdaSpline != 0.0) {
        const MatrixLD &tmp = multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5);
        matB = addMatEtMat(matricesWI.matR, tmp, 5);

    } else {
        matB = matricesWI.matR;
    }

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<MatrixLD, MatrixDiag> &decomp = decompositionCholesky(matB, 5, 1);


    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédant ne change pas

    const SplineResults &sx = doSplineX(matricesWI, events, vecH, decomp, lambdaSpline);
    const std::vector< double> &vecG = sx.vecG;
    //--
      */



    // schoolbook
    /*
    for (unsigned long i = 0; i< vecG.size(); i++) {
        S02 += pow(_events[i]->mYx - vecG[i], 2.);
    }
    */
    // C++ optimization and Compensation for rounding error
     /*
            long double res = 0.0L;
    auto g = vecG.begin();
    long double compensation = 0.0;
    for (auto& ev : events) {
        // Calcul de la différence et son carré
        long double diff = ev->mYx - *g++;
        long double diffSquared = diff * diff;

        // Algorithme de Kahan
        long double y = diffSquared - compensation;
        long double t = res + y;
        compensation = (t - res) - y;
        res = t;
    }

    double N = static_cast<double>(vecG.size());
    const std::vector<t_matrix>& matA = diagonal_influence_matrix(matricesWI, 1, decomp, lambdaSpline);
    // Calcul DLE
    //const double traceA = std::accumulate(matA.begin(), matA.end(), 0.0);
    const long double traceA = compensated_sum(matA);

    const long double EDF = N - traceA;
    res /= EDF; // update 2025-03-13


    return static_cast<double>(res);
   */
}


double MCMCLoopCurve::S02_Vg_Yy(std::vector<std::shared_ptr<Event> >& events, const SplineMatricesLD& matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline)
{
    const MCMCSpline& splineWI = currentSpline(events,  vecH, matricesWI, lambdaSpline, mModel->compute_Y, mModel->compute_XYZ);
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
    const MatrixLD matB = addMatEtMat(matricesWI.matR, multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    const std::pair<MatrixLD, DiagonalMatrixLD> decomp = decompositionCholesky(matB, 5, 1);

   // const SplineResults &s = doSplineY (matricesWI, events, vecH, decomp, lambdaSpline);

    const DiagonalMatrixLD matA = diagonal_influence_matrix(matricesWI, 1, decomp, lambdaSpline);

    const double traceA = matA.diagonal().trace();// std::accumulate(matA.begin(), matA.end(), 0.0L);

    S02 /= (double)(vecG.size()) - traceA;
    return std::move(S02);

}

double MCMCLoopCurve::S02_Vg_Yz(std::vector<std::shared_ptr<Event>> &events, const SplineMatricesLD &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline)
{
    const MCMCSpline& splineWI = currentSpline(events, vecH, matricesWI, lambdaSpline, mModel->compute_Y, mModel->compute_XYZ);
    const std::vector<double>& vecG = splineWI.splineZ.vecG;

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
    const MatrixLD& matB = addMatEtMat(matricesWI.matR, multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    const std::pair<MatrixLD, DiagonalMatrixLD>& decomp = decompositionCholesky(matB, 5, 1);

    //const SplineResults &s = doSplineZ (matricesWI, events, vecH, decomp, lambdaSpline);

    const DiagonalMatrixLD matA = diagonal_influence_matrix(matricesWI, 1, decomp, lambdaSpline);

    const double trace_A = matA.diagonal().trace(); //std::accumulate(matA.begin(), matA.end(), 0.0L);

    S02 /= (double)(vecG.size()) - trace_A;
    return std::move(S02);

}
/**
 * @brief Les calculs sont faits avec les dates (theta event, ti dates, delta, sigma) exprimées en années.
 * \f$  h(\theta) = \prod_{i,j}^{} \frac{1}{\sigma _{t_{ij}}^2} \exp\left(-\frac{1}{2} \left(\frac{\theta - \bar{t}}{\sigma _{t_{ij}}}\right) ^2 \right) \f$
 */
double MCMCLoopCurve::h_theta_Event (const std::shared_ptr<Event> e)
{
    if (e->mType == Event::eDefault) {
        double p = 0.0;
        double t_moy = 0.0;

        for (auto& date : e->mDates) {
            double sigmaTiSquared = date.mSigmaTi.mX * date.mSigmaTi.mX;
            double pi = 1.0 / sigmaTiSquared;
            p += pi;
            t_moy += (date.mTi.mX + date.mDelta) * pi;
        }
        t_moy /= p;

        double thetaMinusTmoy = e->mTheta.mX - t_moy;
        return exp(-0.5 * p * thetaMinusTmoy * thetaMinusTmoy);
        //return exp(-0.5 * p * pow( e->mTheta.mX  - t_moy, 2.0));


    } else {
        return 1.0;
    }

}

t_prob MCMCLoopCurve::h_theta(const QList<std::shared_ptr<Event> > &events) const
{
    if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
        t_prob h = 1.0;
        for (const std::shared_ptr<Event>& e : events) {
            h *= h_theta_Event(e);
        }

#ifdef DEBUG
        if (h == 0.0) {
            qWarning( "MCMCLoopCurve::h_theta() h == 0");
        }
#endif
        return h;

    } else
        return 1.0;
}

#pragma mark Manipulation des theta event pour le calcul spline (equivalent "Do Cravate")


void MCMCLoopCurve::orderEventsByThetaReduced(std::vector<std::shared_ptr<Event>> &event)
{
    // On manipule directement la liste des évènements
    // Ici on peut utiliser event en le déclarant comme copy ??
    std::vector<std::shared_ptr<Event>> &result = event;

    std::sort(result.begin(), result.end(), [](const std::shared_ptr<Event> a, const std::shared_ptr<Event> b) { return (a->mThetaReduced < b->mThetaReduced); });
}


/**
 * @brief MCMCLoopCurve::minimalThetaDifference, if theta are sort, the result is positive
 * @param lEvents
 * @return
 */
double MCMCLoopCurve::minimalThetaDifference(std::vector<std::shared_ptr<Event> > &events)
{
    std::vector<double> result (events.size());
    std::transform (events.begin(), events.end()-1, events.begin()+1, result.begin(), [](const std::shared_ptr<Event> e0, const std::shared_ptr<Event> e1) {return (e1->mTheta.mX - e0->mTheta.mX); });
    // result.erase(result.begin()); // the firs value is not a difference, it's just the first value of LEvents
    std::sort(result.begin(), result.end());
    return std::move(*std::find_if_not (result.begin(), result.end(), [](double v){return v==0.;} ));
}

double MCMCLoopCurve::minimalThetaReducedDifference(std::vector<std::shared_ptr<Event>> &events)
{
    std::vector<t_reduceTime> result (events.size());
    std::transform (events.begin(), events.end()-1, events.begin()+1, result.begin(), [](const std::shared_ptr<Event> e0, const  std::shared_ptr<Event> e1) {return (e1->mThetaReduced - e0->mThetaReduced); });
    // result.erase(result.begin()); // the firs value is not a difference, it's just the first value of LEvents
    std::sort(result.begin(), result.end());
    return std::move(*std::find_if_not (result.begin(), result.end(), []( t_reduceTime v){return v==0.;} ));
}

// not used
void MCMCLoopCurve::spreadEventsTheta(std::vector<std::shared_ptr<Event> > &events, double minStep)
{
    // On manipule directement la liste des évènements
    std::vector<std::shared_ptr<Event>> &result = events;

    // Espacement possible ?
    const int count = (int)result.size();
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
            const double startSpread = result.at(endIndex)->mTheta.mX - result.at(startIndex)->mTheta.mX;

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
void MCMCLoopCurve::spreadEventsThetaReduced0(std::vector<std::shared_ptr<Event> > &sortedEvents, t_reduceTime spreadSpan)
{
    std::vector<std::shared_ptr<Event>>::iterator itEvenFirst = sortedEvents.end();
    std::vector<std::shared_ptr<Event>>::iterator itEventLast = sortedEvents.end();

    if (spreadSpan == 0.0) {
        spreadSpan = 1.0E-8; //std::numeric_limits<double>::epsilon() * 1.E12;//1.E6;// epsilon = 1E-16
       // spreadSpan = 1.0E-8; // ici test
    }

    Eigen::VectorXd diffs(sortedEvents.size() - 1);

    for (std::size_t i = 0; i < sortedEvents.size() - 1; ++i) {
        double delta = std::abs(sortedEvents[i+1]->mThetaReduced - sortedEvents[i]->mThetaReduced);
        diffs[i] = delta;
    }

    // Eigen fournit directement le minimum
    double min_diff = diffs.minCoeff();


    if (min_diff > spreadSpan)
        return;

    unsigned nbEgal = 0;

    // repère première egalité
    for (std::vector<std::shared_ptr<Event>>::iterator itEvent = sortedEvents.begin(); itEvent != sortedEvents.end() -1; itEvent++) {

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
                const t_reduceTime lowBound = itEvenFirst == sortedEvents.begin() ? (*sortedEvents.begin())->mThetaReduced : (*(itEvenFirst -1))->mThetaReduced ; //valeur à gauche non égale
                const t_reduceTime upBound = itEvent == sortedEvents.end()-2 ? (*sortedEvents.crbegin())->mThetaReduced : (*(itEvent + 1))->mThetaReduced;

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

                std::vector<std::shared_ptr<Event>>::iterator itEventEgal;
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
        std::vector<std::shared_ptr<Event>>::iterator itEventEgal;
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

// identique à MCMCLoopCurve::spreadEventsThetaReduced0 , mais directement avec les thetas
// à utiliser avec prudence car ne tient pas compte des contraintes strati

std::vector<double> MCMCLoopCurve::spreadEventsTheta0(std::vector<std::shared_ptr<Event>>& Events, t_reduceTime spreadSpan)
{
    std::vector<double> sortedTheta ;
    auto toto = get_vector<double>(get_Theta, Events);
    std::vector<double>::iterator itEvenFirst = sortedTheta.end();
    std::vector<double>::iterator itEventLast = sortedTheta.end();
    unsigned nbEgal = 0;

    if (spreadSpan == 0.0) {
        spreadSpan = 1.E-8; //std::numeric_limits<double>::epsilon() * 1.E12;//1.E6;// epsilon = 1E-16
    }

    // repère première egalité
    for (std::vector<double>::iterator theta_ptr = sortedTheta.begin(); theta_ptr != sortedTheta.end() -1; theta_ptr++) {

        if ((*(theta_ptr+1)) - (*(theta_ptr)) <= spreadSpan) {

            if (itEvenFirst == sortedTheta.end()) {
                itEvenFirst = theta_ptr;
                itEventLast = theta_ptr + 1;
                nbEgal = 2;

            } else {
                itEventLast = theta_ptr + 1;
                ++nbEgal;
            }

        } else {
            if (itEvenFirst != sortedTheta.end()) {
                // on sort d'une égalité, il faut répartir les dates entre les bornes
                // itEvent == itEventLast
                const t_reduceTime lowBound = itEvenFirst == sortedTheta.begin() ? (*sortedTheta.begin()) : (*(itEvenFirst -1)); //valeur à gauche non égale
                const t_reduceTime upBound = theta_ptr == sortedTheta.end()-2 ? (*sortedTheta.crbegin()) : (*(theta_ptr + 1));

                t_reduceTime step = spreadSpan / (nbEgal-1); // écart théorique
                t_reduceTime min;

                // Controle du debordement sur les valeurs encadrantes
                if (itEvenFirst == sortedTheta.begin()) {
                    // Cas de l'égalité avec la première valeur de la liste
                    // Donc tous les Events sont à droite de la première valeur de la liste
                    min = (*theta_ptr);

                } else {
                    // On essaie de placer une moitier des Events à gauche et l'autre moitier à droite
                    min = (*theta_ptr) - step*floor(nbEgal/2.);
                    // controle du debordement sur les valeurs encadrantes
                    min = std::max(lowBound + step, min );
                }

                const double max = std::min(upBound - spreadSpan, (*theta_ptr) + (double)(step*ceil(nbEgal/2.)) );
                step = (max- min)/ (nbEgal - 1); // écart corrigé

                std::vector<double>::iterator itEventEgal;
                int count;
                for (itEventEgal = itEvenFirst, count = 0; itEventEgal != theta_ptr+1; itEventEgal++, count++ ) {
#ifdef DEBUG
                //    const auto t_init = (*itEventEgal)->mThetaReduced;
#endif
                    (*itEventEgal) = min + count*step;
#ifdef DEBUG
                //    qDebug()<<"[MCMCLoopCurve] spreadEventsThetaReduced0() "<<(*itEventEgal)->mName <<" int time ="<<t_init <<" move to "<<(*itEventEgal)->mThetaReduced;
#endif
                }
                // Fin correction, prêt pour nouveau groupe/cravate
                itEvenFirst = sortedTheta.end();

            }
        }


    }

    // sortie de la boucle avec itFirst validé donc itEventLast == sortedEvents.end()-1

    if (itEvenFirst != sortedTheta.end()) {
        // On sort de la boucle et d'une égalité, il faut répartir les dates entre les bornes
        // itEvent == itEventLast
        const t_matrix lowBound = (*(itEvenFirst -1)) ; //la première valeur à gauche non égale

        const t_matrix max = (*(sortedTheta.end()-1));
        t_matrix step = spreadSpan / (nbEgal-1.); // ecart théorique

        const t_matrix min = std::max(lowBound + spreadSpan, max - step*(nbEgal-1) );

        step = (max- min)/ (nbEgal-1); // écart corrigé

        // Tout est réparti à gauche
        int count;
        std::vector<double>::iterator itEventEgal;
        for (itEventEgal = itEvenFirst, count = 0; itEventEgal != sortedTheta.end(); itEventEgal++, count++ ) {
#ifdef DEBUG
         //   const auto t_init = (*itEventEgal)->mThetaReduced;
#endif
            (*itEventEgal) = min + count *step;

#ifdef DEBUG
         //   qDebug()<<"[MCMCLoopCurve] spreadEventsTheta0() "<<(*itEventEgal)->mName <<" int time ="<<t_init <<" move to "<<(*itEventEgal)->mThetaReduced;
#endif
        }

    }
    return sortedTheta;

}

/**
 * @brief Evenly distributes theta values of events that are too close to each other.
 *
 * This function identifies groups of events whose theta values are separated by a distance
 * less than or equal to spreadSpan and redistributes the intermediate values uniformly
 * while preserving the values of elements at the extremities of each group.
 *
 * @param events Vector containing the events to process
 * @param spreadSpan Maximum distance between two consecutive theta values to consider them
 *                  as part of the same group
 * @return std::vector<double> New vector containing the redistributed theta values
 *                           (empty if fewer than 2 events)
 *
 * @note Theta values are accessed via ev->mTheta.mX for each event.
 * @note Values at the extremities of each group remain unchanged.
 * @note If a group contains only 2 elements, no modification is applied.
 */
std::vector<double> MCMCLoopCurve::unclumpTheta(const std::vector<std::shared_ptr<Event>>& events, double spreadSpan)
{
    if (events.size() < 2)
        return {};

    // On récupère les theta dans un vecteur
    std::vector<double> thetas;
    thetas.reserve(events.size());
    for (const auto& ev : events) {
        thetas.push_back(ev->mTheta.mX);
    }

    // Indices pour garder le lien avec l'ordre initial
    std::vector<size_t> indices(thetas.size());
    std::iota(indices.begin(), indices.end(), 0);

    // On trie les indices par valeurs croissantes de thetas
    std::sort(indices.begin(), indices.end(), [&thetas](size_t a, size_t b) {
        return thetas[a] < thetas[b];
    });

    // Création du vecteur de sortie (copie des valeurs originales)
    std::vector<double> unclumped = thetas;

    size_t i = 0;
    const size_t n = thetas.size();
    while (i < n - 1) {
        if (std::abs(thetas[indices[i + 1]] - thetas[indices[i]]) <= spreadSpan) {
            // Début d'un groupe à égaliser
            size_t first = i;
            size_t last = i + 1;
            while (last + 1 < n &&
                   std::abs(thetas[indices[last + 1]] - thetas[indices[last]]) <= spreadSpan)
                ++last;

            const size_t groupSize = last - first + 1;

            // Si le groupe a seulement 2 éléments, ils sont déjà à leurs places finales
            if (groupSize > 2) {
                // Préserver les valeurs aux positions first et last
                double firstVal = thetas[indices[first]];
                double lastVal = thetas[indices[last]];

                // Répartir uniquement les valeurs entre first et last
                for (size_t k = 1; k < groupSize - 1; ++k) {
                    // Interpolation linéaire entre firstVal et lastVal
                    double t = static_cast<double>(k) / (groupSize - 1);
                    unclumped[indices[first + k]] = firstVal + t * (lastVal - firstVal);
                }
            }

            i = last + 1;
        } else {
            ++i;
        }
    }

    return unclumped;
}


double MCMCLoopCurve::yearTime(double reduceTime)
{
    return mModel->yearTime(reduceTime) ;
}




#pragma mark Calcul Spline




bool asPositiveGPrimeByDerivate (const MCMCSplineComposante& splineComposante, const double k)
{

    for (unsigned long i= 0; i< splineComposante.vecThetaReduced.size()-1; i++) {

        const double t_i = splineComposante.vecThetaReduced[i];
        const double t_i1 = splineComposante.vecThetaReduced[i+1];
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
* @brief hasPositiveGPrimePlusConst
* @param splineComposante
* @param dy in Y unit per time unit ex meter/Year
* @return
 */
bool hasPositiveGPrimePlusConst(const MCMCSplineComposante& splineComposante, const double tmin, const double tmax, const double dy_threshold)
{
    //const double dY = - dy_threshold * (mModel->mSettings.mTmax - mModel->mSettings.mTmin); // On doit inverser le signe et passer en temps réduit
    const double dY = - dy_threshold * (tmax - tmin); // On doit inverser le signe et passer en temps réduit

    decltype(splineComposante.vecThetaReduced)::const_iterator iVecThetaRed = splineComposante.vecThetaReduced.cbegin();
    decltype(splineComposante.vecGamma)::const_iterator iVecGamma = splineComposante.vecGamma.cbegin();
    decltype(splineComposante.vecG)::const_iterator iVecG = splineComposante.vecG.cbegin();

   // Calcul dérivé avant les thetas
    const t_reduceTime t0 = splineComposante.vecThetaReduced.at(0);
    const t_reduceTime t1 = splineComposante.vecThetaReduced.at(1);
    double gPrime = (splineComposante.vecG.at(1) - splineComposante.vecG.at(0)) / (t1 - t0);
    gPrime -= (t1 - t0) * splineComposante.vecGamma.at(1) / 6.;
    if (gPrime < dy_threshold)
        return false;
     // Calcul dérivé avant les thetas

    const size_t n = splineComposante.vecThetaReduced.size();
    const t_reduceTime tn = splineComposante.vecThetaReduced.at(n-1);
    const t_reduceTime tn_1 = splineComposante.vecThetaReduced.at(n-2);
    gPrime = (splineComposante.vecG.at(n-1) - splineComposante.vecG.at(n-2)) / (tn - tn_1);
    gPrime += (tn - tn_1) * splineComposante.vecGamma.at(n-2) / 6.;
    if (gPrime < dy_threshold)
        return false;

//
    for (unsigned long i= 0; i< splineComposante.vecThetaReduced.size()-1; i++) {

        const t_reduceTime t_i = *iVecThetaRed;
        ++iVecThetaRed;
        const t_reduceTime t_i1 = *iVecThetaRed;

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


        if (delta <= 0) {
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



double MCMCLoopCurve::rate_h_lambda_K(const double current_lambda, const double try_lambda, const double tr_K, const unsigned int n)
{
    constexpr int mu = 3;

    const double c = (n - 2) / tr_K;

    const double p = (0.5*(n - 2)*log(try_lambda / current_lambda)) + (mu + 1)*log((c + current_lambda) / (c + try_lambda));

    const double produit = exp(p) ;

    return produit;
}


/**
 * @brief Computes the Hastings ratio for the parameter \f$\lambda\f$ in the MCMC.
 *
 * This function evaluates the transition probability ratio between the current
 * value of \f$\lambda\f$ and a proposed candidate \f$\lambda\f$ according to:
 *
 * \f[
 *    h(\lambda_{\text{try}} \mid \lambda_{\text{current}})
 *    = \left( \frac{\lambda_{\text{try}}}{\lambda_{\text{current}}} \right)^{\tfrac{1}{2}(n_{\text{points}} - 2)}
 *      \cdot \left( \frac{c + \lambda_{\text{current}}}{c + \lambda_{\text{try}}} \right)^{\mu + 1}
 * \f]
 *
 * where:
 * - \f$\lambda_{\text{current}}\f$ is the current value of the parameter,
 * - \f$\lambda_{\text{try}}\f$ is the proposed candidate value,
 * - \f$n_{\text{points}}\f$ is the number of data points,
 * - \f$c\f$ is the constant given by \f$mModel \rightarrow mC\_lambda\f$,
 * - \f$\mu = 3\f$ (fixed in this implementation).
 *
 * @param current_lambda Current value of \f$\lambda\f$.
 * @param try_lambda Proposed candidate value of \f$\lambda\f$.
 * @param n_points Number of data points \f$n_{\text{points}}\f$.
 * @return The Hastings ratio \f$h(\lambda_{\text{try}} \mid \lambda_{\text{current}})\f$.
 */
t_prob MCMCLoopCurve::rate_h_lambda_X_335(const double current_lambda, const double try_lambda, const unsigned int n_points)
{
    constexpr int mu = 3;//3;

    const double c = mModel->mC_lambda;

    const t_prob produit = pow(try_lambda / current_lambda, 0.5 * (n_points - 2)) *
                           pow((c + current_lambda) / (c + try_lambda), mu + 1);

    return produit;
}

t_prob MCMCLoopCurve::rate_h_lambda_XY_335(const double current_lambda, const double try_lambda, const unsigned int n_points)
{
    constexpr int mu = 3;

    const t_prob c = mModel->mC_lambda;

    const t_prob produit = pow(try_lambda / current_lambda, 1 * (n_points - 2)) *
                           pow((c + current_lambda) / (c + try_lambda), mu + 1);
    return produit;
}

t_prob MCMCLoopCurve::rate_h_lambda_XYZ_335(const double current_lambda, const double try_lambda, const unsigned n_points)
{
    constexpr int mu = 3;

    const t_prob c = mModel->mC_lambda;

    const t_prob produit = pow(try_lambda / current_lambda, 1.5 * (n_points - 2)) *
                           pow((c + current_lambda) / (c + try_lambda), mu + 1);


    return produit;
}

double MCMCLoopCurve::S02_lambda_WIK (const MatrixLD &K, const int nb_noeuds)
{

    double vm = 0.;
    for (int i = 0; i < nb_noeuds; ++i) {
        vm += K(i, i);
    }

    const double S02_lambda = (nb_noeuds - 2) / vm;

    return S02_lambda;
}

double MCMCLoopCurve::h_lambda_Komlan(const MatrixLD &K, const MatrixLD &K_new, const int nb_noeuds, const double &lambdaSpline)
{
    /* initialisation de l'exposant mu du prior "shrinkage" sur lambda : fixe
     en posant mu=2, la moyenne a priori sur alpha est finie = (nb_noeuds-2)/somme(Mat_W_1K[i,i]) ;
     et la variance a priori sur lambda est infinie
     NB : si on veut un shrinkage avec espérance et variance finies, il faut mu >= 3
    */

    constexpr int mu = 3;
    const double c = S02_lambda_WIK(K, nb_noeuds);// = (n-2)/tr(K)

    const double cnew = S02_lambda_WIK(K_new, nb_noeuds);

    const double prior1 = exp((mu + 1)*log(c / (c + lambdaSpline)));
    const double prior2 = exp((mu + 1)*log(cnew / (cnew + lambdaSpline)));

    const double prior = (prior2 / prior1) * (c / cnew) ;

    return prior;
}


SplineMatricesLD MCMCLoopCurve::prepareCalculSpline_W_Vg0(const std::vector<std::shared_ptr<Event>> &sortedEvents, std::vector<double>& vecH)
{
    SparseMatrixLD matR = calculMatR_LD(vecH);
    SparseMatrixLD matQ = calculMatQ_LD(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    auto Qt = matQ.transpose();// transpose(matQ, 3);

    // Diag Winv est égale à la diagonale des variances
    DiagonalMatrixLD diagWInv (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.diagonal().begin(), [](std::shared_ptr<Event>ev) {return pow(ev->mSy, 2.);});

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    MatrixLD tmp = Qt * diagWInv; //multiMatParDiag(matQT, diagWInv, 3);
    MatrixLD matQTW_1Q = tmp * matQ;//multiMatParMat(tmp, matQ, 3, 3);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    MatrixLD matQTQ = Qt * matQ;//multiMatParMat(matQT, matQ, 3, 3);

    SplineMatricesLD matrices;
    matrices.diagWInv = std::move(diagWInv);
    matrices.matR = std::move(matR);
    matrices.matQ = std::move(matQ);
    //matrices.matQT = std::move(matQT);
    matrices.matQTW_1Q = std::move(matQTW_1Q); // Seule affectée par changement de VG, ici VG=0
    matrices.matQTQ = std::move(matQTQ);

    return matrices;
}


MCMCSpline MCMCLoopCurve::samplingSpline_multi(std::vector<std::shared_ptr<Event> > &lEvents, std::vector<std::shared_ptr<Event> > &lEventsinit, std::vector<t_matrix> vecYx, std::vector<double> vecYstd, const SparseMatrixLD &R, const MatrixLD &R_1QT, const SparseMatrixLD &Q)
{
    MCMCSpline spline;

    const int n = lEvents.size() ;
    try {

        DiagonalMatrixLD W_1 (n);
        for (int i = 0; i <n; i++) {
            W_1.diagonal()[i] = pow(vecYstd[i], 2);
        }

        MatrixLD B = R + mModel->mLambdaSpline.mX * Q.transpose() * W_1 * Q;
        MatrixLD B_1 = inverse_padded_matrix(B);

        const MatrixLD& A = computeMatA_optimized_kahan(Q, B_1, W_1, mModel->mLambdaSpline.mX );
        //showMatrix(A, " samplinSplin A=");

        const MatrixLD& WlambdaK_1 = multiMatParDiag0(A, W_1); // doit être SPD
//showMatrix(WlambdaK_1,"WlambdaK_1 samplingSpline_multi");

        // Calcul du vecteur moyen AY de la conditionnelle complète
        const std::vector<t_matrix>  &mu = multiMatByVectCol0(A, vecYx);
//showVector(mu, "mu_Yx samplingSpline_multi");

        // simulation de la fonction f
        const std::vector<double> &fx = multinormal_sampling(mu, WlambdaK_1);
//const std::vector<double>& fx {124.97296, 7.95364, 0.056, 0.056, 7.95364, 124.97296}; //test

        for (int i = 0; i < n; i++) {
            lEventsinit[i]-> mGx = fx[i];
        }
//showVector(fx, "fx");

        // Calcul de la dérivée seconde de la fonction f

        std::vector<t_matrix> vecGamma_L =  multiMatByVectCol0(R_1QT, fx);

        // transtypage
        std::vector<double> vecGamma (vecGamma_L.begin(), vecGamma_L.end());
        vecGamma.push_back(0.);
        vecGamma.insert(vecGamma.begin(), 0.);
//showVector(vecGamma, "vecGamma Komlan");

        // La sauvegarde de theta, f et f'
        MCMCSplineComposante splineX;
        splineX.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents);;
        splineX.vecG = fx;
        splineX.vecGamma = vecGamma;

        splineX.vecVarG = std::vector<double>(n, 0.0);

        spline.splineX = std::move(splineX);

    } catch (...) {
        std::cout << "[MCMCLoopCurve::samplingSpline_multi] error" << std::endl;
    }
    return spline;
}



// les Events doivent être ordonnées
// Q est une matrice de bande 3
/**
 * @brief MCMCLoopCurve::samplingSpline_multi2
 *
 * @param lEvents
 * @param R
 * @param R_1QT
 * @param Q
 * @return
 */
MCMCSpline MCMCLoopCurve::samplingSpline_multi2(std::vector<std::shared_ptr<Event> > &lEvents, const SparseMatrixLD &R, const MatrixLD &R_1Qt, const SparseMatrixLD& Q)
{
    MCMCSpline spline;

    const size_t n_points = lEvents.size() ;
    try {

        const auto lambda = mModel->mLambdaSpline.mX;
        DiagonalMatrixLD W_1 (lEvents.size()) ; // correspond à 1.0/mW

        std::transform(lEvents.begin(), lEvents.end(), W_1.diagonal().begin(), [](std::shared_ptr<Event> ev){return 1.0/ ev->mW;});// {return (ev->mSy*ev->mSy + ev->mVg.mX;});

        SparseMatrixLD Qt = Q.transpose();
        SparseMatrixLD B = R + lambda * Qt * W_1 * Q; // B est une padded matrice

        // Remplacer : MatrixLD B_1 = inverse_padded_matrix(B);
        // est plus stable que de calculer
        // Oui, résoudre le système Bx = y est plus stable que d’inverser B pour obtenir x = B⁻¹y.
        // C’est ce que recommandent toutes les bonnes pratiques numériques (y compris LAPACK, Eigen, NumPy, etc.).
        //Eigen::LDLT<MatrixLD> solver(B);

        // indices du sous-bloc utile (ici 1..n-2 si matrice n×n)
        int first = 1;
        int nsub  = B.rows() - 2;

        // Extraire le sous-bloc utile
        SparseMatrixLD Bsub = B.block(first, first, nsub, nsub);
        SparseMatrixLD Qtsub = Qt.middleRows(first, nsub);

        // Factorisation creuse LDLT
        Eigen::SimplicialLDLT<SparseMatrixLD> solver; //  -> x = B_1 * y = B_1 * Qt
        solver.compute(Bsub);

        // Résolution
        MatrixLD B_1Qtsub = solver.solve(MatrixLD(Qtsub));
#ifdef DEBUG
        if (solver.info() != Eigen::Success) {
            std::cerr << "LDLT failed!" << std::endl;
        }
#endif
        // Recomposer le résultat complet
        MatrixLD B_1Qt = MatrixLD::Zero(B.rows(), Qt.cols());
        B_1Qt.middleRows(first, nsub) = B_1Qtsub;

        MatrixLD QB_1Qt = Q * B_1Qt;

        DiagonalMatrixLD I (W_1.rows());
        I.setIdentity();

        /** Simulation de la fonction f
         *   Σ = A * W_1
         * Ancien Calcule L = Cholesky(Σ) (via LLᵀ)
         * Nouveau Calcul Vroot via une SVD, plus stable
         * Tire f = μ + L·z, avec z ~ 𝒩(0, I)
         */

        // Calcul du vecteur moyen AY de la conditionnelle complète avec le valeur de Yx


        /**
        * Calcule A = I - λ * W_1 * Q * B_1 * Qᵀ * W_1
        *
        */

        // matrice de projection (ou opérateur de lissage).
        MatrixLD A = I.toDenseMatrix() - lambda * W_1 * QB_1Qt;// forme classique

        // Matrice de Covaiance
        // Σ = A * W_1
        const MatrixLD V =  A * W_1 ;

        ColumnVectorLD Yx = stdVectorToColumnVector(get_vector<t_matrix>(get_Yx, lEvents));

        ColumnVectorLD mu_Yx = A * Yx;


        const ColumnVectorLD fx = multinormal_sampling(mu_Yx, V);


        for (size_t i = 0; i < n_points; i++) {
            lEvents[i]-> mGx = fx[i];
        }
        ColumnVectorLD vecGamma_x = R_1Qt * fx; // Condition spline directe

        // La sauvegarde de theta, f et f'
        MCMCSplineComposante splineX;
        splineX.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents);
        splineX.vecG = std::vector<double>(fx.data(), fx.data() + fx.size());
        splineX.vecGamma = std::vector<double>(vecGamma_x.data(), vecGamma_x.data() + vecGamma_x.size());

        splineX.vecVarG = std::vector<double>(n_points, 0.0);

        spline.splineX = std::move(splineX);


        if (mModel->compute_Y) {

            ColumnVectorLD Yy = stdVectorToColumnVector(get_vector<t_matrix>(get_Yy, lEvents));
            ColumnVectorLD mu_Yy = A * Yy;

            // simulation de la fonction f
            const ColumnVectorLD fy = multinormal_sampling(mu_Yy, V);

            for (size_t i = 0; i < n_points; i++) {
                lEvents[i]-> mGy = fy[i];
            }

            // Calcul de la dérivée seconde de la fonction f
            ColumnVectorLD vecGamma_y =  R_1Qt * fy;

            // La sauvegarde de theta, f et f'
            MCMCSplineComposante splineY;
            splineY.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents); ;
            splineY.vecG = std::vector<double>(fy.data(), fy.data() + fy.size());
            splineY.vecGamma = std::vector<double>(vecGamma_y.data(), vecGamma_y.data() + vecGamma_y.size());

            splineY.vecVarG = std::vector<double>(n_points, 0.0);

            spline.splineY  = std::move(splineY);
        }
        if (mModel->compute_XYZ) {

            ColumnVectorLD Yz = stdVectorToColumnVector(get_vector<t_matrix>(get_Yz, lEvents));
            ColumnVectorLD mu_Yz = A * Yz;

            // simulation de la fonction f
            const ColumnVectorLD fz = multinormal_sampling(mu_Yz, V);

            for (size_t i = 0; i < n_points; i++) {
                lEvents[i]-> mGz = fz[i];
            }

            // Calcul de la dérivée seconde de la fonction f
            ColumnVectorLD vecGamma_z =  R_1Qt * fz;

            // La sauvegarde de theta, f et f'
            MCMCSplineComposante splineZ;
            splineZ.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents); ;
            splineZ.vecG = std::vector<double>(fz.data(), fz.data() + fz.size()); //fz;
            splineZ.vecGamma = std::vector<double>(vecGamma_z.data(), vecGamma_z.data() + vecGamma_z.size()); //vecGamma;

            splineZ.vecVarG = std::vector<double>(n_points, 0.0);

            spline.splineZ  = std::move(splineZ);
        }


    } catch (...) {
        std::cout << "[MCMCLoopCurve::samplingSpline_multi2] error" << std::endl;
    }
    return spline;
}

MCMCSpline MCMCLoopCurve::samplingSpline_multi2(std::vector<std::shared_ptr<Event> > &lEvents, const SparseMatrixD &R, const MatrixD &R_1Qt, const SparseMatrixD& Q)
{
    MCMCSpline spline;

    const size_t n_points = lEvents.size() ;
    try {

        const auto lambda = mModel->mLambdaSpline.mX;
        DiagonalMatrixD W_1 (lEvents.size()) ; // correspond à 1.0/mW

        std::transform(lEvents.begin(), lEvents.end(), W_1.diagonal().begin(), [](std::shared_ptr<Event> ev){return 1.0/ ev->mW;});// {return (ev->mSy*ev->mSy + ev->mVg.mX;});

        SparseMatrixD Qt = Q.transpose();
        SparseMatrixD B = R + lambda * Qt * W_1 * Q; // B est une padded matrice

        // Remplacer : MatrixLD B_1 = inverse_padded_matrix(B);
        // est plus stable que de calculer
        // Oui, résoudre le système Bx = y est plus stable que d’inverser B pour obtenir x = B⁻¹y.
        // C’est ce que recommandent toutes les bonnes pratiques numériques (y compris LAPACK, Eigen, NumPy, etc.).
        //Eigen::LDLT<MatrixLD> solver(B);

        // indices du sous-bloc utile (ici 1..n-2 si matrice n×n)
        int first = 1;
        int nsub  = B.rows() - 2;

        // Extraire le sous-bloc utile
        SparseMatrixD Bsub = B.block(first, first, nsub, nsub);
        SparseMatrixD Qtsub = Qt.middleRows(first, nsub);

        // Factorisation creuse LDLT
        Eigen::SimplicialLDLT<SparseMatrixD> solver; //  -> x = B_1 * y = B_1 * Qt
        solver.compute(Bsub);

        // Résolution
        MatrixD B_1Qtsub = solver.solve(MatrixD(Qtsub));
#ifdef DEBUG
        if (solver.info() != Eigen::Success) {
            std::cerr << "LDLT failed!" << std::endl;
        }
#endif
        // Recomposer le résultat complet
        MatrixD B_1Qt = MatrixD::Zero(B.rows(), Qt.cols());
        B_1Qt.middleRows(first, nsub) = B_1Qtsub;

        MatrixD QB_1Qt = Q * B_1Qt;

        DiagonalMatrixD I (W_1.rows());
        I.setIdentity();

        /** Simulation de la fonction f
         *   Σ = A * W_1
         * Ancien Calcule L = Cholesky(Σ) (via LLᵀ)
         * Nouveau Calcul Vroot via une SVD, plus stable
         * Tire f = μ + L·z, avec z ~ 𝒩(0, I)
         */

        // Calcul du vecteur moyen AY de la conditionnelle complète avec le valeur de Yx


        /**
        * Calcule A = I - λ * W_1 * Q * B_1 * Qᵀ * W_1
        *
        */

        // matrice de projection (ou opérateur de lissage).
        MatrixD A = I.toDenseMatrix() - lambda * W_1 * QB_1Qt;// forme classique

        // Matrice de Covaiance
        // Σ = A * W_1
        const MatrixD V =  A * W_1 ;

        ColumnVectorD Yx = stdVectorToColumnVector(get_vector<double>(get_Yx, lEvents));

        ColumnVectorD mu_Yx = A * Yx;


        const ColumnVectorD fx = multinormal_sampling(mu_Yx, V);


        for (size_t i = 0; i < n_points; i++) {
            lEvents[i]-> mGx = fx[i];
        }
        ColumnVectorD vecGamma_x = R_1Qt * fx; // Condition spline directe

        // La sauvegarde de theta, f et f'
        MCMCSplineComposante splineX;
        splineX.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents);
        splineX.vecG = std::vector<double>(fx.data(), fx.data() + fx.size());
        splineX.vecGamma = std::vector<double>(vecGamma_x.data(), vecGamma_x.data() + vecGamma_x.size());

        splineX.vecVarG = std::vector<double>(n_points, 0.0);

        spline.splineX = std::move(splineX);


        if (mModel->compute_Y) {

            ColumnVectorD Yy = stdVectorToColumnVector(get_vector<double>(get_Yy, lEvents));
            ColumnVectorD mu_Yy = A * Yy;

            // simulation de la fonction f
            const ColumnVectorD fy = multinormal_sampling(mu_Yy, V);

            for (size_t i = 0; i < n_points; i++) {
                lEvents[i]-> mGy = fy[i];
            }

            // Calcul de la dérivée seconde de la fonction f
            ColumnVectorD vecGamma_y =  R_1Qt * fy;

            // La sauvegarde de theta, f et f'
            MCMCSplineComposante splineY;
            splineY.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents); ;
            splineY.vecG = std::vector<double>(fy.data(), fy.data() + fy.size());
            splineY.vecGamma = std::vector<double>(vecGamma_y.data(), vecGamma_y.data() + vecGamma_y.size());

            splineY.vecVarG = std::vector<double>(n_points, 0.0);

            spline.splineY  = std::move(splineY);
        }
        if (mModel->compute_XYZ) {

            ColumnVectorD Yz = stdVectorToColumnVector(get_vector<double>(get_Yz, lEvents));
            ColumnVectorD mu_Yz = A * Yz;

            // simulation de la fonction f
            const ColumnVectorD fz = multinormal_sampling(mu_Yz, V);

            for (size_t i = 0; i < n_points; i++) {
                lEvents[i]-> mGz = fz[i];
            }

            // Calcul de la dérivée seconde de la fonction f
            ColumnVectorD vecGamma_z =  R_1Qt * fz;

            // La sauvegarde de theta, f et f'
            MCMCSplineComposante splineZ;
            splineZ.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents); ;
            splineZ.vecG = std::vector<double>(fz.data(), fz.data() + fz.size()); //fz;
            splineZ.vecGamma = std::vector<double>(vecGamma_z.data(), vecGamma_z.data() + vecGamma_z.size()); //vecGamma;

            splineZ.vecVarG = std::vector<double>(n_points, 0.0);

            spline.splineZ  = std::move(splineZ);
        }


    } catch (...) {
        std::cout << "[MCMCLoopCurve::samplingSpline_multi2] error" << std::endl;
    }
    return spline;
}

MCMCSpline MCMCLoopCurve::samplingSpline_multi_depth(std::vector<std::shared_ptr<Event> > &lEvents, const SparseMatrixD &R, const MatrixD &R_1Qt, const SparseMatrixD& Q)
{
    MCMCSpline spline;

    const size_t n_points = lEvents.size() ;
    try {

        const auto lambda = mModel->mLambdaSpline.mX;
        DiagonalMatrixD W_1 (lEvents.size()) ; // correspond à 1.0/mW

        std::transform(lEvents.begin(), lEvents.end(), W_1.diagonal().begin(), [](std::shared_ptr<Event> ev){return 1.0/ ev->mW;});// {return (ev->mSy*ev->mSy + ev->mVg.mX;});

        SparseMatrixD Qt = Q.transpose();
        SparseMatrixD B = R + lambda * Qt * W_1 * Q; // B est une padded matrice

        // Remplacer : MatrixLD B_1 = inverse_padded_matrix(B);
        // est plus stable que de calculer
        // Oui, résoudre le système Bx = y est plus stable que d’inverser B pour obtenir x = B⁻¹y.
        // C’est ce que recommandent toutes les bonnes pratiques numériques (y compris LAPACK, Eigen, NumPy, etc.).
        //Eigen::LDLT<MatrixLD> solver(B);

        // indices du sous-bloc utile (ici 1..n-2 si matrice n×n)
        int first = 1;
        int nsub  = B.rows() - 2;

        // Extraire le sous-bloc utile
        SparseMatrixD Bsub = B.block(first, first, nsub, nsub);
        SparseMatrixD Qtsub = Qt.middleRows(first, nsub);

        // Factorisation creuse LDLT
        Eigen::SimplicialLDLT<SparseMatrixD> solver; //  -> x = B_1 * y = B_1 * Qt
        solver.compute(Bsub);

        // Résolution
        MatrixD B_1Qtsub = solver.solve(MatrixD(Qtsub));
#ifdef DEBUG
        if (solver.info() != Eigen::Success) {
            std::cerr << "LDLT failed!" << std::endl;
        }
#endif
        // Recomposer le résultat complet
        MatrixD B_1Qt = MatrixD::Zero(B.rows(), Qt.cols());
        B_1Qt.middleRows(first, nsub) = B_1Qtsub;

        MatrixD QB_1Qt = Q * B_1Qt;

        DiagonalMatrixD I (W_1.rows());
        I.setIdentity();

        /** Simulation de la fonction f
         *   Σ = A * W_1
         * Ancien Calcule L = Cholesky(Σ) (via LLᵀ)
         * Nouveau Calcul Vroot via une SVD, plus stable
         * Tire f = μ + L·z, avec z ~ 𝒩(0, I)
         */

        // Calcul du vecteur moyen AY de la conditionnelle complète avec le valeur de Yx


        /**
        * Calcule A = I - λ * W_1 * Q * B_1 * Qᵀ * W_1
        *
        */

        // matrice de projection (ou opérateur de lissage).
        MatrixD A = I.toDenseMatrix() - lambda * W_1 * QB_1Qt; // forme classique

        // Matrice de Covaiance
        // Σ = A * W_1
        const MatrixD V =  A * W_1 ;

        ColumnVectorD Yx = stdVectorToColumnVector(get_vector<double>(get_Yx, lEvents));

        ColumnVectorD mu_Yx = A * Yx;

        const ColumnVectorD& lastY =  get_ColumnVector<double>(get_Gx, mModel->mEvents); //(Warm Start)

        const ColumnVectorD fx = multinormal_sampling_depth(mu_Yx, V, lastY);

        //std::cout << fx << std::endl << std::endl;

        for (size_t i = 0; i < n_points; i++) {
            lEvents[i]-> mGx = fx[i];
        }
        ColumnVectorD vecGamma_x = R_1Qt * fx; // Condition spline directe

        // La sauvegarde de theta, f et f'
        MCMCSplineComposante splineX;
        splineX.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents);
        splineX.vecG = std::vector<double>(fx.data(), fx.data() + fx.size());
        splineX.vecGamma = std::vector<double>(vecGamma_x.data(), vecGamma_x.data() + vecGamma_x.size());

        splineX.vecVarG = std::vector<double>(n_points, 0.0);

        spline.splineX = std::move(splineX);

/*
        if (mModel->compute_Y) {

            ColumnVectorD Yy = stdVectorToColumnVector(get_vector<double>(get_Yy, lEvents));
            ColumnVectorD mu_Yy = A * Yy;

            // simulation de la fonction f
            const ColumnVectorD fy = multinormal_sampling(mu_Yy, V);

            for (size_t i = 0; i < n_points; i++) {
                lEvents[i]-> mGy = fy[i];
            }

            // Calcul de la dérivée seconde de la fonction f
            ColumnVectorD vecGamma_y =  R_1Qt * fy;

            // La sauvegarde de theta, f et f'
            MCMCSplineComposante splineY;
            splineY.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents); ;
            splineY.vecG = std::vector<double>(fy.data(), fy.data() + fy.size());
            splineY.vecGamma = std::vector<double>(vecGamma_y.data(), vecGamma_y.data() + vecGamma_y.size());

            splineY.vecVarG = std::vector<double>(n_points, 0.0);

            spline.splineY  = std::move(splineY);
        }
        if (mModel->compute_XYZ) {

            ColumnVectorD Yz = stdVectorToColumnVector(get_vector<double>(get_Yz, lEvents));
            ColumnVectorD mu_Yz = A * Yz;

            // simulation de la fonction f
            const ColumnVectorD fz = multinormal_sampling(mu_Yz, V);

            for (size_t i = 0; i < n_points; i++) {
                lEvents[i]-> mGz = fz[i];
            }

            // Calcul de la dérivée seconde de la fonction f
            ColumnVectorD vecGamma_z =  R_1Qt * fz;

            // La sauvegarde de theta, f et f'
            MCMCSplineComposante splineZ;
            splineZ.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents); ;
            splineZ.vecG = std::vector<double>(fz.data(), fz.data() + fz.size()); //fz;
            splineZ.vecGamma = std::vector<double>(vecGamma_z.data(), vecGamma_z.data() + vecGamma_z.size()); //vecGamma;

            splineZ.vecVarG = std::vector<double>(n_points, 0.0);

            spline.splineZ  = std::move(splineZ);
        }
*/

    } catch (...) {
        std::cout << "[MCMCLoopCurve::samplingSpline_multi_depth] error" << std::endl;
    }
    return spline;
}

void MCMCLoopCurve::initialize_spline_for_depth(std::vector<std::shared_ptr<Event> > &lEvents, const SparseMatrixD &R, const MatrixD &R_1Qt, const SparseMatrixD& Q)
{
    MCMCSplineComposante splineX;

    MCMCSplineComposante last_Valid_splineX;
    const size_t n_points = lEvents.size() ;
    try {

        DiagonalMatrixD W_1 = current_splineMatrices_D.diagWInv ; // correspond à 1.0/mW

        SparseMatrixD Qt = current_splineMatrices_D.matQ.transpose();

        // init boucle lambda
        double lambda = 1.0E10; // peut correspondre à des cas avec des theta très proches

        int max_iterations = 30; // Limite maximale de tentatives donne un lambda= 1E-30
        int iteration_count = 0;
        bool depth_OK = true;
        double last_valid_lambda; // Stocke la dernière valeur valide

        DiagonalMatrixD I (W_1.rows());
        I.setIdentity();

        ColumnVectorD Yx = stdVectorToColumnVector(get_vector<double>(get_Yx, lEvents));

        // Ici dans la spline, on ne mets à jour que Gx et Gamma_X
        splineX.vecThetaReduced = get_vector<t_reduceTime>(get_ThetaReduced, lEvents);
        splineX.vecVarG = std::vector<double>(n_points, 0.0); // n'est plus utilisé
        ColumnVectorD lastY =  get_ColumnVector<double>(get_Yx, mModel->mEvents); //(Warm Start)

        // début boucle lambda
        while (depth_OK == true && iteration_count < max_iterations) {
            // Sauvegarder la valeur actuelle avant de la diviser
           // last_valid_lambda = lambda;
            iteration_count++;

            lambda = lambda / 10;

            SparseMatrixD B = R + lambda * Qt * W_1 * Q; // B est une padded matrice

            // Remplacer : MatrixLD B_1 = inverse_padded_matrix(B);
            // est plus stable que de calculer
            // Oui, résoudre le système Bx = y est plus stable que d’inverser B pour obtenir x = B⁻¹y.
            // C’est ce que recommandent toutes les bonnes pratiques numériques (y compris LAPACK, Eigen, NumPy, etc.).
            //Eigen::LDLT<MatrixLD> solver(B);

            // indices du sous-bloc utile (ici 1..n-2 si matrice n×n)
            int first = 1;
            int nsub  = B.rows() - 2;

            // Extraire le sous-bloc utile
            SparseMatrixD Bsub = B.block(first, first, nsub, nsub);
            SparseMatrixD Qtsub = Qt.middleRows(first, nsub);

            // Factorisation creuse LDLT
            Eigen::SimplicialLDLT<SparseMatrixD> solver; //  -> x = B_1 * y = B_1 * Qt
            solver.compute(Bsub);

            // Résolution
            MatrixD B_1Qtsub = solver.solve(MatrixD(Qtsub));
#ifdef DEBUG
            if (solver.info() != Eigen::Success) {
                std::cerr << "LDLT failed!" << std::endl;
            }
#endif
            // Recomposer le résultat complet
            MatrixD B_1Qt = MatrixD::Zero(B.rows(), Qt.cols());
            B_1Qt.middleRows(first, nsub) = B_1Qtsub;

            MatrixD QB_1Qt = Q * B_1Qt;

            /**
            * Calcule A = I - λ * W_1 * Q * B_1 * Qᵀ * W_1
            *
            */

            // matrice de projection (ou opérateur de lissage).
            MatrixD A = I.toDenseMatrix() - lambda * W_1 * QB_1Qt; // forme classique

            // Matrice de Covariance
            // Σ = A * W_1
            const MatrixD V =  A * W_1 ;

            // Calcul du vecteur moyen AY de la conditionnelle complète avec le valeur de Yx
            ColumnVectorD mu_Yx = A * Yx;

            /** Simulation de la fonction f
            * Ancien Calcule L = Cholesky(Σ) (via LLᵀ)
            * Nouveau Calcul Vroot via une SVD, plus stable
            * Tire f = μ + L·z, avec z ~ 𝒩(0, I)
            */
            const ColumnVectorD fx = multinormal_sampling_depth(mu_Yx, V, lastY);

            splineX.vecG = std::vector<double>(fx.data(), fx.data() + fx.size());

           // lastY =  fx; //(Warm Start)

            ColumnVectorD vecGamma_x = R_1Qt * fx; // Condition spline directe

            splineX.vecGamma = std::vector<double>(vecGamma_x.data(), vecGamma_x.data() + vecGamma_x.size());

            if (iteration_count == 1) { // On mémorise la première
                // les splines matrice ne changengt pas
                last_valid_lambda = lambda;
                // memo de la spline
                last_Valid_splineX = splineX;


            } else {
                // test positivité
                depth_OK = hasPositiveGPrimePlusConst(splineX, mModel->mSettings.mTmin, mModel->mSettings.mTmax, mCurveSettings.mThreshold);
                if (depth_OK) {
                    last_valid_lambda = lambda;
                    // memo de la spline
                    last_Valid_splineX = splineX;
                }
            }
        }
        // fin boucle
        bool positive_curve = !depth_OK && iteration_count>1;
        std::cout << " lambda for depth = " << last_valid_lambda << " Positive curve : " << (positive_curve ? "✅ YES" : "❌ NO")  << std::endl;

         // memo des Gx
        for (size_t i = 0; i < n_points; i++) {
            lEvents[i]->mGx = last_Valid_splineX.vecG[i];
        }

        // memo de la spline
        mModel->mSpline.splineX = last_Valid_splineX;

#ifdef DEBUG
        std::cout << "[MCMCLoopCurve::initialize_spline_for_depth] " << std::endl;
        for (size_t i = 0; i < n_points; i++) {
            std::cout << "Y= " << lEvents[i]->mYx << "\t Gx= "<< lEvents[i]->mGx << "\t theta= " << lEvents[i]->mTheta.mX << "\t theta_reduit= " << lEvents[i]->mThetaReduced << std::endl;
        }
#endif
        // Utiliser la dernière valeur valide trouvée
        mModel->mLambdaSpline.mX = last_valid_lambda;

    } catch (...) {
        std::cout << "[MCMCLoopCurve::initialize_spline_for_depth] error" << std::endl;
    }
    return;
}

double MCMCLoopCurve::Prior_F (const MatrixLD& K, const MatrixLD& K_new, const MCMCSpline &s,  const double lambdaSpline)
{

    const int n = (int)K.rows();

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


std::vector<double> MCMCLoopCurve::multiMatByVectCol0_KK(const MatrixLD &KKK, const std::vector<double> &gg)
{
    const int nl1 = (int)KKK.rows();
    // const int nc1 = matrix[0].size();
    const int nc2 = (int)gg.size();

    // nc1 doit etre égal à nc2
    std::vector<double> result (nl1, 0.0); //initVector(nl1);
    // const double* itMat1;
    double sum;

    for (int i = 0; i < nl1; ++i) {
        // itMat1 = begin(KKK[i]);

        sum = 0;
        for (int k = 0; k < nc2; ++k) {
            const int u = Signe_Number(KKK(i, k)) * Signe_Number(gg[k]) ;

            sum += u * pow(nl1, log_p(abs(KKK(i, k)), nl1) + log_p(abs(gg[k]), nl1)) ; //(*(itMat1 + k)) * gg[k];
        }

        result[i] = sum;
    }
    return result;
}


t_prob MCMCLoopCurve::h_exp_fX_theta (std::shared_ptr<Event> e, const MCMCSpline &s, unsigned idx)
{
    const t_prob h = -0.5 * (e->mW * pow( e->mYx - s.splineX.vecG[idx], 2.0));
    return exp(h);
}

t_prob MCMCLoopCurve::h_exp_fY_theta (std::shared_ptr<Event> e, const MCMCSpline &s, unsigned idx)
{
    const t_prob h = -0.5 * (e->mW * pow( e->mYy - s.splineY.vecG[idx], 2.0));
    return exp(h);
}

t_prob MCMCLoopCurve::h_exp_fZ_theta (std::shared_ptr<Event> e, const MCMCSpline &s, unsigned idx)
{
    const t_prob h = -0.5 * (e->mW * pow( e->mYz - s.splineZ.vecG[idx], 2.0));
    return exp(h);
}

std::vector<double> MCMCLoopCurve::sampling_spline (std::vector<std::shared_ptr<Event> > &lEvents, SplineMatricesLD matrices)
{
    int n = matrices.diagWInv.rows();

    std::vector< double> vecYx (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), vecYx.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

    std::vector<double> X;
    for(int i = 0; i < n; i++){
        X.push_back(Generator::normalDistribution(vecYx[i], sqrt(matrices.diagWInv.diagonal()[i])));
    }

    return X;
}
#ifdef KOMLAN
double MCMCLoopCurve::h_S02_Vg_K_old(const std::vector<std::shared_ptr<Event>> events, double S02_Vg, double try_Vg)
{
    constexpr int alp = 1;

    const double prior = exp((alp + 1)*log(S02_Vg / try_Vg)) * exp(-((S02_Vg - try_Vg) / (try_Vg*S02_Vg)));

    constexpr int a = 1;
    double prod_h_Vg = 1.;

    if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (auto& e : events) {
            prod_h_Vg *= (exp((a + 1)*log((S02_Vg + e->mVg.mX) / (try_Vg + e->mVg.mX))) * (try_Vg / S02_Vg)) ;
        }

        //const double pp = (prior * prod_h_Vg);


    } else {
        prod_h_Vg = (exp((a + 1)*log((S02_Vg + events[0]->mVg.mX) / (try_Vg + events[0]->mVg.mX))) * (try_Vg / S02_Vg));

        //const double pp = prior * prod_h_Vg;

    }
    return prior * prod_h_Vg;
}
#endif

std::vector<t_matrix> MCMCLoopCurve::multiMatByVectCol0(const MatrixLD& KKK, const std::vector<t_matrix>& gg)
{
    const size_t nl1 = KKK.rows();
    // const int nc1 = KKK.cols();
    const size_t nc2 = gg.size();

    // nc1 doit etre égal à nc2
    std::vector<t_matrix> result (nl1, 0.0);

    t_matrix sum;

    for (size_t i = 0; i < nl1; ++i) {

        sum = 0;
        for (size_t k = 0; k < nc2; ++k) {
            sum += KKK(i, k) * gg[k] ; //(*(itMat1 + k)) * gg[k];
        }

        result[i] = sum;
    }

    return result;
}

/**
 * @brief Multiplie une matrice par un vecteur colonne.
 *
 * Cette fonction effectue le produit d'une matrice `KKK` de dimensions
 * $ n \times m $ par un vecteur colonne `gg` de taille $ m $.
 * Pour chaque ligne de la matrice, elle calcule le produit scalaire
 * avec le vecteur `gg` et stocke le résultat dans un vecteur de type
 * `t_matrix`.
 *
 * @param KKK La matrice d'entrée de dimensions $ n \times m $.
 * @param gg Le vecteur colonne d'entrée de taille $ m $.
 * @return std::vector<t_matrix> Un vecteur contenant les résultats
 *         du produit pour chaque ligne de la matrice.
 */
/* std::vector<t_matrix> MCMCLoopCurve::multiMatByVectCol0(const MatrixLD& KKK, const std::vector<double>& gg)
{
    const size_t nl1 = KKK.rows();
    const size_t nc2 = gg.size();

    // nc1 doit etre égal à nc2
    std::vector<t_matrix> result (nl1, 0.0);

    t_matrix sum;

    for (size_t i = 0; i < nl1; ++i) {
        sum = 0.0;
        for (size_t k = 0; k < nc2; ++k) {
            sum += KKK(i, k) * static_cast<t_matrix>(gg[k]) ;
        }

        result[i] = sum;
    }

    return result;
} */
std::vector<t_matrix> MCMCLoopCurve::multiMatByVectCol0(const MatrixLD& KKK, const std::vector<double>& gg)
{
    const size_t nl1 = KKK.rows();
    const size_t nc2 = gg.size();

    // Vérification de cohérence
    assert(static_cast<size_t>(KKK.cols()) == nc2);

    std::vector<t_matrix> result(nl1, 0.0L);

    // OpenMP : parallélise sur les lignes (indépendantes)
#pragma omp parallel for schedule(static)
    for (Eigen::Index i = 0; i < static_cast<Eigen::Index>(nl1); ++i) {
        t_matrix sum = 0.0L;
        for (Eigen::Index k = 0; k < static_cast<Eigen::Index>(nc2); ++k) {
            sum += KKK(i, k) * static_cast<t_matrix>(gg[k]);
        }
        result[i] = sum;
    }
    return result;
}

std::vector<double> MCMCLoopCurve::multinormal_sampling(const std::vector<t_matrix>& mu, const MatrixLD& a)
{
    size_t N = mu.size();

    const auto L = robust_LLt(a);//cholesky_LLt_MoreSorensen(a);


    //  fr = matrix of the 1D normal distribution with mean 0 and variance 1.

    std::vector<double> fr;
    for (size_t i = 0; i< N; i++)
        fr.push_back(Generator::normalDistribution(0.0, 1.0));

    const auto &Lfr = multiMatByVectCol0(L, fr);


    std::vector<double> f;
    for(size_t i = 0; i < N; i++) {
#ifdef DEBUG
           if (Lfr[i] > 1000)
               std::cerr << "[MCMCLoopCurve::multinormal_sampling] Lfr[i] >> 1000" << std::endl;
#endif
        f.push_back(mu[i] + Lfr[i]);
    }

    return f;
}

/**
 * @brief MCMCLoopCurve::multinormal_sampling
 * @param mu
 * @param a
 * @return
 ✅ Ce qui est fait :

    Construction de  A = K + λ·diag(1/W)

    Ancien Calcule L = chol(A) (via LLᵀ)

    L (Vroot) en utilisant une décomposition SVD, c'est plus robuste
    Tire f = μ + L·z, avec z ~ 𝒩(0, I)
*/
ColumnVectorLD MCMCLoopCurve::multinormal_sampling (const ColumnVectorLD& mu, const MatrixLD& A)
{
    size_t N = mu.size();

    //const MatrixLD L = robust_LLt(A);

    const MatrixLD Vroot = robust_SVD(A);
    // Vroot : cette matrice n’est pas triangulaire, c'est normal
    // Cov(Lz) = L*Cov(z)*LT = L*I*LT = L*LT = V
    // Donc la distribution de fx est correcte quelle que soit la "forme" de L.
    // L’essentiel : L*LT doit être la covariance souhaitée.
    // La structure de L (triangulaire ou non) n’a pas d’impact sur la covariance.

    ColumnVectorLD z (N);
    for (size_t i = 0; i< N; i++)
        z.data()[i] = Generator::normalDistribution(0.0, 1.0);

    //auto Lz = L * z;
    auto Lz = Vroot * z;

    ColumnVectorLD f = mu + Lz;

#ifdef DEBUG
    for(size_t i = 0; i < N; i++) {
        if (Lz[i] > 1000)
               std::cerr << "[MCMCLoopCurve::multinormal_sampling] L * z >> 100" << std::endl;

    }
#endif
    //std::cout<< " f = μ + L·z = " << f << std::endl;

    return f;
}

// utiliser dans v3.3.5
ColumnVectorD MCMCLoopCurve::multinormal_sampling (const ColumnVectorD& mu, const MatrixD& A)
{
    size_t N = mu.size();

    const MatrixD L = robust_LLt(A);

    // Cov(Lz) = L*Cov(z)*LT = L*I*LT = L*LT = V
    // Donc la distribution de fx est correcte quelle que soit la "forme" de L.
    // L’essentiel : L*LT doit être la covariance souhaitée.
    // La structure de L (triangulaire ou non) n’a pas d’impact sur la covariance.

    ColumnVectorD z (N);
    for (size_t i = 0; i< N; i++)
        z.data()[i] = Generator::normalDistribution(0.0, 1.0);

    auto Lz = L * z;


    ColumnVectorD f = mu + Lz;

#ifdef DEBUG
    /*for(size_t i = 0; i < N; i++) {
        if (Lz[i] > 1000)
               std::cerr << "[MCMCLoopCurve::multinormal_sampling] L * z >> 100" << std::endl;

    }*/
#endif
    //std::cout<< " f = μ + L·z = " << f << std::endl;

    return f;
}

ColumnVectorLD MCMCLoopCurve::multinormal_sampling2 (const ColumnVectorLD& Y, const MatrixLD& A_1, const DiagonalMatrixLD& w_1)
{
    size_t N = Y.size();
    MatrixLD V = A_1 * w_1 * A_1;

    const MatrixLD L = robust_LLt(V);
    //const auto L = cholesky_LLt_MoreSorensen(a);

    ColumnVectorLD z (N);
    for (size_t i = 0; i< N; i++)
        z.data()[i] = Generator::normalDistribution(0.0, 1.0);

    auto Lz = L * z;

    ColumnVectorLD f = A_1 * Y + Lz;

#ifdef DEBUG
    for(size_t i = 0; i < N; i++) {
        if (Lz[i] > 1000)
               std::cerr << "[MCMCLoopCurve::multinormal_sampling2] L * z >> 1000" << std::endl;

    }
#endif
    //std::cout<< " f = μ + L·z = " << f << std::endl;

    return f;
}


ColumnVectorD sampleOrderedNaive(const ColumnVectorD& mu,
                                   const MatrixD& a)
{
    const int N = mu.size();
    const auto L = robust_LLt(a);

    ColumnVectorD fr(N), f;
    bool isIncreasing = false;
    //int counter = 0;
    //constexpr int MAX_ATTEMPTS = 100;

    //while (!isIncreasing && counter < MAX_ATTEMPTS) { // casse la statistique
    while (!isIncreasing ) {
        // Génère un vecteur gaussien standard
        for (Eigen::Index i = 0; i < N; ++i) {
            fr(i) = Generator::normalDistribution(0.0, 1.0);
        }

        // Transforme en vecteur corrélé
        f = mu + L * fr;

        // Vérifie si strictement croissant
        isIncreasing = true;
        for (Eigen::Index i = 0; i < N - 1; ++i) {
            if (f(i) >= f(i + 1)) {
                isIncreasing = false;
                break;
            }
        }

        //++counter;
    }

#ifdef DEBUG
    // Optionnel : vérifier que la condition a été remplie
    //if (!isIncreasing) {
      //  std::cout << "[multinormal_sampling_depth]: Failed to generate a strictly increasing sample after"
        //           << MAX_ATTEMPTS << "attempts." << std::endl;
        // ici tu pourrais renvoyer une version triée ou lever une exception si nécessaire
   // }
#endif

    return f;
}

ColumnVectorD sampleOrderedThreads(const ColumnVectorD& mu,
                                   const MatrixD& a,
                                   unsigned int globalSeed,
                                   int nThreadsPerBatch = 2)
{
    const int N = mu.size();
    const auto L = robust_LLt(a);
    ColumnVectorD result(N);
    std::atomic<bool> found(false);

    int batchId = 0;

    while (!found.load()) {
        std::vector<std::thread> threads;
        threads.reserve(nThreadsPerBatch);

        for (int t = 0; t < nThreadsPerBatch; ++t) {
            threads.emplace_back([&, t, batchId]() {
                ColumnVectorD fr(N), f(N);

                // RNG par thread, reproductible
                std::mt19937 rng(globalSeed + batchId * nThreadsPerBatch + t);
                std::normal_distribution<double> gauss(0.0, 1.0);

                while (!found.load()) {
                    for (int i = 0; i < N; ++i) fr(i) = gauss(rng);
                    f = mu + L * fr;

                    bool increasing = true;
                    for (int i = 0; i < N-1; ++i) {
                        if (f(i) >= f(i+1)) {
                            increasing = false;
                            break;
                        }
                    }

                    if (increasing) {
                        if (!found.exchange(true)) {
                            result = f; // seul le premier thread écrit
                        }
                        break; // stop thread
                    }
                }
            });
        }

        // Attend que tous les threads terminent leur batch
        for (auto& th : threads) th.join();
        ++batchId;
    }

    return result;
}
// hit and run
ColumnVectorD sampleOrderedOptimized(const ColumnVectorD& mu,
                                     const MatrixD& C,
                                     const ColumnVectorD& LastY,
                                     int burnIn = 200)
{
    const int N = mu.size();

    // 1. Cholesky (Obligatoire si C change)
    // ---------- 1️⃣  Cholesky (avec jitter si besoin) ----------
    MatrixD L;

    Eigen::LLT<MatrixD> llt(C);
    if (llt.info() != Eigen::Success) {
        const double jitter = 1e-8;
        MatrixD Creg = C + jitter * MatrixD::Identity(N,N);
        llt.compute(Creg);
        /*if (llt.info() != Eigen::Success) {
            qWarning() << "C n'est toujours pas SPD après jitter.";
            return ColumnVectorD();
        }*/
        // Fallback vers LDLT si la matrice n'est pas définie positive
        if (llt.info() != Eigen::Success) {
            Eigen::LDLT<MatrixD> ldlt(C);

            if (ldlt.info() == Eigen::Success) {

                L = ldlt.matrixL();
                Eigen::VectorXd D = ldlt.vectorD();

                // Convertir LDL^T → LL^T
                for (Eigen::Index i = 0; i < D.size(); ++i) {
                    L.col(i) *= (D(i) > 0) ? std::sqrt(D(i)) : 0.0;
                }
                //return L;
            }
        } else {
            L = llt.matrixL();
        }
    } else {

        L = llt.matrixL();
    }


    // ---------- 2️⃣  Warm‑start (projection) ----------
    // On ne rejette rien, on crée un point valide en triant mu ou en ajustant
    // on s'assure que lastY respecte les contraintes
    // Votre vecteur est trié de force (ex: [10, 5, 20] devient [10, 10.00001, 20]).
    //ColumnVectorD y = mu;
    ColumnVectorD y = LastY; // (Warm Start), même si C a changer, on peut penser que l'ancien résultat n'est pas loin de la cible
    bool was_valid = true;
    for (int i = 1; i < N; ++i) {
        if (y(i) <= y(i-1)) {
            y(i) = y(i-1) + 1e-6;
            was_valid = false;
        }
    }

    // Choix adaptatif du burn-in
    int currentBurnIn = was_valid ? 200 : 200;
    // ---------- 3️⃣  Passage dans l'espace standard ----------
    ColumnVectorD rhs = y - mu;               // = 0 si y == mu

    ColumnVectorD z = L.triangularView<Eigen::Lower>().solve(rhs);

    if (!z.allFinite()) { // depend option de compilation
#ifdef DEBUG
        std::cout << "[sampleOrderedOptimized] z became NaN → reset" << std::endl;
#endif
        z.setZero();

    }

    // ---------- 4️⃣  Pré‑calcul des contraintes ----------
    const int M = N - 1;
    MatrixD W(M, N);
    ColumnVectorD v(M);
    for (int i = 0; i < M; ++i) {
        W.row(i) = L.row(i+1) - L.row(i);
        v(i) = mu(i) - mu(i+1);
    }
    ColumnVectorD Wz = W * z; // O(N²) une fois


    // ---------- 5️⃣  Boucle Hit‑and‑Run ----------
    ColumnVectorD d(N), Wd(M);
    constexpr double INF = std::numeric_limits<double>::infinity();

    for (int iter = 0; iter < currentBurnIn + 1; ++iter) {
        // Tirage d'une direction d via masterEngine pour reproductibilité

        for(int i=0; i<N; ++i) d(i) = Generator::normalDistribution(0.0, 1.0);;
        d.normalize();

        Wd.noalias() = W * d;
        double a = -INF, b = INF;
        //double a = -1e150, b = 1e150;

        /*for (int i = 0; i < N - 1; ++i) {
            double den = Wd(i);
            double num = v(i) - Wz(i);
            if (den > 0)      a = std::max(a, num / den);
            else  b = std::min(b, num / den);
        }*/

        constexpr double eps = 1e-12;
        for (int i = 0; i < N - 1; ++i) {
                    double den = Wd(i);
                    double num = v(i) - Wz(i);
                    if (std::abs(den) < eps) {
                        if (num >= 0.0) { a = 1; b = 0; break; } // intervalle vide
                    } else continue;

                    if (den > 0)      a = std::max(a, num / den);
                    else  b = std::min(b, num / den);

                    double bound = num / den;
                    if (!std::isfinite(bound)) continue;
        }



        if (a < b) {
            double mu_lambda = -d.dot(z);  // ← signe « ‑ » correct
            // truncatedNormal utilise ici Generator en interne
            double lambda = Generator::truncatedNormal(mu_lambda, 1.0,
                                                       a + mu_lambda ,
                                                       b + mu_lambda);

            z.noalias() += lambda * d;
            Wz.noalias() += lambda * Wd; // Update O(N)
        }
    }
    // ---------- 6️⃣  Retour à l'espace original ----------
    return mu + L * z;
}

ColumnVectorD MCMCLoopCurve::multinormal_sampling_depth(const ColumnVectorD& mu, const MatrixD& a, const ColumnVectorD& lastY)
{
    const Eigen::Index N = mu.rows();
    if (N > 5) {

        auto hr = sampleOrderedOptimized(mu, a, lastY); //hit and run
        return hr;

    }

   /* else if (N > 10) {
        auto hr = sampleOrderedThreads(mu, a, 12 , 3);
        return hr;

    }*/
    else {
        return sampleOrderedNaive(mu, a);
    }

}



t_prob MCMCLoopCurve::rapport_Theta(const std::function <double (std::shared_ptr<Event>)> &fun, const std::vector<std::shared_ptr<Event> > &lEvents, const MatrixLD &K, const MatrixLD &K_new, const double lambdaSpline)
{
    const size_t n = K.rows();

    const auto &K1 = multiConstParMat0(K, -1.);

    const auto &KK = addMatEtMat0(K_new, K1) ;

    std::vector<double> vectfx;
    vectfx.resize(lEvents.size());

    std::transform(lEvents.begin(), lEvents.end(), vectfx.begin(), fun);

    const std::vector<t_matrix> &fKx = multiMatByVectCol0(KK, vectfx);

    double som = 0.0;
    for (size_t i = 0; i < n; ++i) {
        som += fKx[i] * vectfx[i];
    }
    //double dx = -0.5 * lambdaSpline * som;
    return exp(-0.5 * lambdaSpline * som);

}

// Calcul le rapport des formes quadratiques f^t*K*f ancien rate_Theta
t_prob MCMCLoopCurve::rate_ftKf(const MatrixLD &Y, const MatrixLD &K, const MatrixLD &Y_try, const MatrixLD &K_try, const double lambda)
{

    MatrixLD K_try_Y = K_try * Y_try;
    MatrixLD KY = K * Y;

    /* schoolbook code
     *     size_t n_points = Y.rows();
    size_t n_components = Y.cols();
    t_matrix som_try0 = 0.0;
    t_matrix som0 = 0.0;
    for (size_t i = 0; i < n_points; ++i) {
        for (size_t j = 0; j < n_components; ++j) {
            som_try0 += K_try_Y(i, j) * Y_try(i, j);
            som0 += KY(i, j) * Y(i, j);
        }
    }
    */

    // Produit élément par élément, puis somme
    t_matrix som_try = (K_try_Y.array() * Y_try.array()).sum();
    t_matrix som = (KY.array() * Y.array()).sum();

    return exp(-0.5 * lambda * (som_try - som) );

}
t_prob MCMCLoopCurve::rate_ftKf(const MatrixD &Y, const MatrixD &K, const MatrixD &Y_try, const MatrixD &K_try, const double lambda)
{

    MatrixD K_try_Y = K_try * Y_try;
    MatrixD KY = K * Y;

    /* schoolbook code*/
    /* Chronometer t1("boucle");
         size_t n_points = Y.rows();
    size_t n_components = Y.cols();
    double som_try0 = 0.0;
    double som0 = 0.0;
    for (size_t i = 0; i < n_points; ++i) {
        for (size_t j = 0; j < n_components; ++j) {
            som_try0 += K_try_Y(i, j) * Y_try(i, j);
            som0 += KY(i, j) * Y(i, j);
        }
    }
    t1.display();
    */

    // Produit élément par élément, puis somme

    double som_try = (K_try_Y.array() * Y_try.array()).sum();
    double som = (KY.array() * Y.array()).sum();

    return exp(-0.5 * lambda * (som_try - som) );

}

t_prob MCMCLoopCurve::rate_Theta_X(const std::vector<std::shared_ptr<Event>> &Events, const MatrixLD &K, const MatrixLD &K_try, const double lambdaSpline)
{
    const size_t n = K.rows();

    MatrixLD K_try_K = K_try - K;

    const std::vector<double>& vectfx = get_vector<double>(get_Yx, Events);

    const std::vector<t_matrix>& fxK = multiMatByVectCol0(K_try_K, vectfx);

    t_matrix som = 0.0;
    for (size_t i = 0; i < n; ++i) {
        som += fxK[i] * vectfx[i];
    }

    return exp(-0.5 * lambdaSpline * som);

}



t_prob MCMCLoopCurve::rate_Theta_XY(const std::vector<std::shared_ptr<Event>> &Events, const MatrixLD &K, const MatrixLD &K_try, const double lambdaSpline)
{
    const size_t n = K.rows();

    //const auto& K1 = multiConstParMat0(K, -1.);
    //const auto& K_try_K = addMatEtMat0(K_try, K1) ;

    MatrixLD K_try_K = K_try - K;

    const std::vector<double>& vectfx = get_vector<double>(get_Yx, Events);
    const std::vector<t_matrix>& fxK = multiMatByVectCol0(K_try_K, vectfx);

    const std::vector<double>& vectfy = get_vector<double>(get_Yy, Events);
    const std::vector<t_matrix>& fyK = multiMatByVectCol0(K_try_K, vectfy);

    double som = 0.0;
    for (size_t i = 0; i < n; ++i) {
        som += fxK[i] * vectfx[i];
        som += fyK[i] * vectfy[i];
    }

    return exp(-0.5 * lambdaSpline * som);

}

t_prob MCMCLoopCurve::rate_Theta_XYZ(const std::vector<std::shared_ptr<Event>> &Events, const MatrixLD &K, const MatrixLD &K_try, const double lambdaSpline)
{
    const size_t n = K.rows();

    //const auto& K1 = multiConstParMat0(K, -1.);
    //const auto& K_try_K = addMatEtMat0(K_try, K1) ;

    MatrixLD K_try_K = K_try - K;

    const std::vector<double>& vectfx = get_vector<double>(get_Yx, Events);
    const std::vector<t_matrix>& fxK = multiMatByVectCol0(K_try_K, vectfx);

    const std::vector<double>& vectfy = get_vector<double>(get_Yy, Events);
    const std::vector<t_matrix>& fyK = multiMatByVectCol0(K_try_K, vectfy);

    const std::vector<double>& vectfz = get_vector<double>(get_Yz, Events);
    const std::vector<t_matrix>& fzK = multiMatByVectCol0(K_try_K, vectfz);

    double som = 0.0;
    for (size_t i = 0; i < n; ++i) {
        som += fxK[i] * vectfx[i];
        som += fyK[i] * vectfy[i];
        som += fzK[i] * vectfz[i];
    }

    return exp(-0.5 * lambdaSpline * som);

}

#pragma mark usefull math function
double S02_lambda_WI(const SplineMatricesLD &matrices, const int nb_noeuds)
{
    const MatrixLD &R = matrices.matR;
    const MatrixLD &Q = matrices.matQ;
    const auto Qt = Q.transpose();

    // On pose W = matrice unité

    // calcul des termes diagonaux de W_1.K
    const std::pair<MatrixLD, DiagonalMatrixLD> decomp = decompositionCholesky(R, 3, 1);

    const MatrixLD matRInv = inverseMatSym_origin(decomp, 5, 1);

    const MatrixLD matK = multiplyMatrixBanded_Winograd(multiplyMatrixBanded_Winograd(Q, matRInv, 2), Qt, 0); // bandwith->k1=k2=0, car on peut utiliser que les diagonales pour calculer la digonale de matK

    double vm = 0.;
    for (size_t i = 0; i < static_cast<size_t>(nb_noeuds); ++i) {
        vm += matK(i, i);
    }

    return (nb_noeuds - 2) / vm;
}

// Utilise des matrices pleines
double rapport_detK_plus(const MatrixLD &Mat_old, const MatrixLD &Mat_new)
{
    const auto &current_decompK = choleskyLDLT(Mat_old, 0);// pas de test L<0// cholesky_LDLt_MoreSorensen(Mat_old);
    const auto &try_decompK = choleskyLDLT(Mat_new, 0); //cholesky_LDLt_MoreSorensen(Mat_new);

    // On trie les valeurs pour optimiser les rapports entre des valeurs de même ordre de grandeur
    std::vector<t_matrix> current_detPlus (current_decompK.second.diagonal().data(), current_decompK.second.diagonal().data()+current_decompK.second.diagonal().size());
    std::sort(current_detPlus.begin(), current_detPlus.end(), [] (double i, double j) { return (i>j);});

    //current_detPlus.resize(current_detPlus.rows() - 2, current_detPlus.cols() - 2 ); // ici
    current_detPlus.resize(current_detPlus.size() - 2);


    std::vector<t_matrix> try_detPlus (try_decompK.second.diagonal().data(), try_decompK.second.diagonal().data()+try_decompK.second.diagonal().size());
    std::sort(try_detPlus.begin(), try_detPlus.end(), [] (double i, double j) { return (i>j);});

    try_detPlus.resize(try_detPlus.size() - 2);

    const size_t n = Mat_old.rows() - 2;
    t_matrix rapport_detPlus = 1.0;

    for (size_t i = 0 ; i < n; i++) {
        rapport_detPlus *= try_detPlus[i] / current_detPlus[i]  ;
    }

    const t_matrix d = sqrt(rapport_detPlus);

    return d;
}

MatrixLD inverseMatSym_originKK(const MatrixLD &matrixLE,  const DiagonalMatrixLD& matrixDE, const int nbBandes, const int shift)
{
    long dim = matrixLE.rows();
    MatrixLD matInv (dim, dim);
    long bande = floor((nbBandes-1)/2);

    matInv(dim-1-shift, dim-1-shift) = 1.0 / matrixDE.diagonal()[dim-1-shift];

    if (dim >= 4) {
        matInv(dim-2-shift, dim-1-shift) = -matrixLE(dim-1-shift, dim-2-shift) * matInv(dim-1-shift, dim-1-shift);
        matInv(dim-2-shift, dim-2-shift) = (1.0 / matrixDE.diagonal()[dim-2-shift]) - matrixLE(dim-1-shift, dim-2-shift) * matInv(dim-2-shift, dim-1-shift);
    }

    // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes
    // La boucle suivante n'est executée que si dim >=5
    for (long i = dim-3-shift; i >= shift; --i) {
        matInv(i, i+2) = -matrixLE(i+1, i) * matInv(i+1, i+2) - matrixLE(i+2, i) * matInv(i+2, i+2);
        matInv(i, i+1) = -matrixLE(i+1, i) * matInv(i+1, i+1) - matrixLE(i+2, i) * matInv(i+1, i+2);
        matInv(i, i) = (1.0 / matrixDE.diagonal()[i]) - matrixLE(i+1, i) * matInv(i, i+1) - matrixLE(i+2, i) * matInv(i, i+2);

        if (bande >= 3)  {
            for (long k = 3; k <= bande; ++k) {
                if (i+k < (dim - shift))  {
                    matInv(i, i+k) = -matrixLE(i+1, i) * matInv(i+1, i+k) - matrixLE(i+2, i) * matInv(i+2, i+k);
                }
            }
        }
    }

    for (long i = shift; i < dim-shift; ++i)  {
        for (long j = i+1; j <= i+bande; ++j)  {
            if (j < (dim-shift))   {
                matInv(j, i) = matInv(i, j);
            }
        }
    }

    return matInv;
}


/* -----------------------------------------------------------------
   Truncated normal sampler (standard deviation = 1, mean = mu)
   ----------------------------------------------------------------- */
/*double truncatedNormal(double mu, double a, double b)
{
    // CDF de la N(0,1)
    auto Phi = [](double x) {
        return 0.5 * std::erfc(-x / std::sqrt(2.0));
    };
    // Inverse de la CDF (erfc⁻¹) – on utilise la fonction de la STL
    auto PhiInv = [](double p) {
        return std::sqrt(2.0) * erfcInv_approx(2.0 * (1.0 - p));
    };

    double alpha = (a - mu);          // limites dans l’espace standard
    double beta  = (b - mu);

    double Phi_a = Phi(alpha);
    double Phi_b = Phi(beta);

    // Cas où l’intervalle est infini d’un côté
    if (std::isinf(alpha)) Phi_a = 0.0;
    if (std::isinf(beta )) Phi_b = 1.0;

    //std::uniform_real_distribution<double> ud(Phi_a, Phi_b);
    //double u = ud(Generator::normalDistribution(0.0, 1.0));
    //double z = PhiInv(u);             // z ~ N(0,1) conditionnée
    //return mu + z;                    // on ajoute la moyenne

    double u = Generator::randomUniform(Phi_a, Phi_b);
    double z = PhiInv(u);             // z ~ N(0,1) conditionnée
    return mu + z;
}

double truncatedNormal(double mu, double a, double b)
{
    // Si l’intervalle est infini d’un côté, on le remplace par un très grand
    // nombre afin d’éviter les overflow dans la distribution normale.
    const double INF = 1e100;
    if (std::isinf(a)) a = -INF;
    if (std::isinf(b)) b =  INF;

    // Si l’intervalle est vide, on renvoie mu (c’est un cas qui ne devrait
    // jamais arriver, mais cela évite un boucle infinie).
    if (a >= b) return mu;

    double val;
    int attempts = 0;
    const int MAX_ATTEMPTS = 10000;   // garde‑en‑cas de mauvaise configuration

    do {
        val = Generator::normalDistribution(0.0, 1.0);
        ++attempts;
        if (attempts > MAX_ATTEMPTS) {
            // on abandonne le rejet et on renvoie la moyenne ; cela ne
            // corrompt pas la chaîne (le prochain pas sera tout de même valide).
            return mu;
        }
    } while (val < a || val > b);

    return val;
}
*/
/**
 * @brief Wrapper qui utilise Generator::gaussByDoubleExp pour
 *        générer une normale tronquée N(mu,1) sur [a,b].
 *
 * @param mu   moyenne (μ_λ)
 * @param a    borne inférieure (peut être -inf)
 * @param b    borne supérieure (peut être +inf)
 * @param rng  moteur aléatoire (pas utilisé directement, mais on le
 *             passe à Generator pour qu’il utilise le même source).
 * @return double  un échantillon λ.
 */
double truncatedNormal(double mu, double a, double b)
{
    if (!std::isfinite(mu) || !std::isfinite(a) || !std::isfinite(b)) {
        qWarning() << "truncatedNormal: paramètre non fini"
                   << "mu=" << mu << "a=" << a << "b=" << b;
        return 0.0;
    }
    // -----------------------------------------------------------------
    // 1️⃣ Gestion des bornes infinies
    // -----------------------------------------------------------------
    // gaussByDoubleExp ne sait pas gérer ±∞.  On remplace donc les
    // infinies par un très grand nombre (≈ 1e100) qui est « pratiquement ∞».
    const double INF = 1e100;
    double min = a;
    double max = b;
    if (std::isinf(a)) min = -INF;
    if (std::isinf(b)) max =  INF;

    // -----------------------------------------------------------------
    // 2️⃣ Cas où l’intervalle est vide (devrait déjà être filtré)
    // -----------------------------------------------------------------
    if (min >= max) {
        // Retour de la moyenne – cela ne corrompt pas la chaîne,
        // le prochain pas sera tout de même valide.
        return mu;
    }

    // -----------------------------------------------------------------
    // 3️⃣ Appel de la fonction existante
    // -----------------------------------------------------------------
    // sigma = 1.0 (variance = 1) – c’est exactement ce que l’on veut.
    // La fonction lance des exceptions si le domaine est incohérent,
    // on les laisse remonter.
    return Generator::gaussByDoubleExp(mu, 1.0, min, max);
}


/* -----------------------------------------------------------------
   Hit‑and‑Run → **un seul** vecteur colonne (taille = mu.size()).
   ----------------------------------------------------------------- */
/*
inline ColumnVectorD sampleOrdered_one(const ColumnVectorD& mu,
                                       const MatrixD& C,
                                       int nIterPerSample,   // itérations entre deux enregistrements
                                       int burnIn, // itérations à jeter avant le premier enregistrement
                                       int thin)    // sous‑échantillonnage
{
    const int N = mu.size();

    // -------------------------------------------------------------
    // 1️⃣  Décomposition de Cholesky (C = L Lᵀ)
    // -------------------------------------------------------------
    Eigen::LLT<MatrixD> llt(C);

    if (llt.info() != Eigen::Success) {
        // ajoute un petit terme diagonal pour rendre la matrice SPD
        const double jitter = 1e-8;
        MatrixD Creg = C + jitter * MatrixD::Identity(N,N);
        llt.compute(Creg);
        if (llt.info() != Eigen::Success) {
            qWarning() << "Even after jitter C is not SPD.";
            return ColumnVectorD(); // vecteur vide → échec
        }
    }
    const MatrixD L = llt.matrixL();

    // -------------------------------------------------------------
    // 2️⃣  Point de départ strictement croissant
    // -------------------------------------------------------------
    ColumnVectorD y = mu;
    const double eps = 1e-8;
    for (int i = 1; i < N; ++i)
        if (y(i) <= y(i-1))
            y(i) = y(i-1) + eps;   // garantit y₁ < … < y_N

    // -------------------------------------------------------------
    // 3️⃣  Passer dans l’espace standard : z = L⁻¹ (y‑mu)
    // -------------------------------------------------------------
    ColumnVectorD rhs = y - mu;               // = 0
    if (!rhs.allFinite()) {
        qWarning() << "rhs contains NaN/Inf → abort";
        return ColumnVectorD();
    }
    ColumnVectorD z = L.triangularView<Eigen::Lower>().solve(rhs);
    if (!z.allFinite()) {
        qWarning() << "Solution z contains NaN/Inf → abort";
        std::cerr << "L =\n" << L << "\n";
        std::cerr << "rhs = " << rhs.transpose() << "\n";
        return ColumnVectorD();
    }
    // -------------------------------------------------------------
    // 4️⃣  Boucle principale (Hit‑and‑Run)
    // -------------------------------------------------------------
    int iter = 0;
    int totalIter = 0;                     // compteur global (burn‑in + thinning)

    while (true) {
        // ---- nIterPerSample itérations de la chaîne ----
        for (int inner = 0; inner < nIterPerSample; ++inner, ++iter) {
            // ----- 5.1 Direction aléatoire (normale puis normalisation) -----
            ColumnVectorD d(N);
            for (int i = 0; i < N; ++i) d(i) = Generator::normalDistribution(0.0, 1.0);
            d.normalize();                    // d devient unitaire (colonne)

            // ----- 5.2 Bornes a et b pour λ -----
            double a = -std::numeric_limits<double>::infinity();   // -∞
            double b =  std::numeric_limits<double>::infinity();   // +∞

            for (int i = 0; i < N-1; ++i) {
                RowVectorD delta = L.row(i+1) - L.row(i);

                double denom = delta.dot(d);                     // Δ_i·d

                double num   = mu(i) - mu(i+1) - delta.dot(z);   // -(Δ_i·z) + (μ_i-μ_{i+1})

                if (std::abs(denom) < 1e-12) {
                    // Direction presque parallèle à la contrainte
                    if (num >= 0.0) {
                        a = 1.0;
                        b = 0.0;
                        break;
                    } // intervalle vide
                    else continue;                               // contrainte déjà satisfaite
                }

                double bound = num / denom;   // λ > bound si denom>0, λ < bound sinon
                // 1️⃣  Vérifier que le résultat est fini
                if (!std::isfinite(bound)) {
                    qWarning("bound = %g (num=%g, denom=%g) → non fini, on ignore la contrainte i=%d",
                             bound, num, denom, i);
                    continue;               // on saute cette contrainte
                }
                if (denom > 0.0)
                    a = std::max(a, bound);
                else
                    b = std::min(b, bound);
            }

            if (a >= b) continue;          // intervalle vide → on passe à la prochaine direction

            // ----- 5.3 Tirage de λ (normale tronquée) -----
            double mu_lambda = d.dot(z);   // moyenne de λ le long de la droite
            //double lambda    = truncatedNormal(mu_lambda, a, b);
            double lambda = truncatedNormal( 0.0,
                                             a - mu_lambda,
                                             b - mu_lambda
                                             );

            // ----- 5.4 Mise à jour du point latent -----
            z += lambda * d;
        }

        ++totalIter;                       // on a fini nIterPerSample itérations

        // ---- 5.5 Burn‑in / thinning ----
        if (totalIter >= burnIn && ((totalIter - burnIn) % thin == 0)) {
            // Transforme z → y = μ + L z  (colonne)
            ColumnVectorD result = mu + L * z;
            return result;                 // <-- un seul vecteur colonne
        }
    }
}
*/

// ne fonctionne pas avec notre MCMC, puisque l'interet et de garder C constant
inline ColumnVectorD sampleOrdered_one(const ColumnVectorD& mu,
                                       const MatrixD& C,
                                       int nIterPerSample,
                                       int burnIn,
                                       int thin)
{
    const int N = mu.size();

    // 1. Cholesky (Nécessaire car C change)
    Eigen::LLT<MatrixD> llt(C);
    if (llt.info() != Eigen::Success) {
        MatrixD Creg = C + 1e-8 * MatrixD::Identity(N, N);
        llt.compute(Creg);
        if (llt.info() != Eigen::Success) return ColumnVectorD();
    }
    const MatrixD L = llt.matrixL();

    // 2. Point de départ z (espace standard)
    // On part de y = mu ordonné, puis on résout Lz = y - mu
    ColumnVectorD y = mu;
    for (int i = 1; i < N; ++i) {
        if (y(i) <= y(i-1)) y(i) = y(i-1) + 1e-7;
    }
    ColumnVectorD z = L.triangularView<Eigen::Lower>().solve(y - mu);

    // 3. Pré-calcul des contraintes (W * z > v)
    // W est la matrice de différence des lignes de L (N-1 x N)
    // v est le vecteur de différence des éléments de mu (N-1)
    MatrixD W(N - 1, N);
    ColumnVectorD v(N - 1);
    for (int i = 0; i < N - 1; ++i) {
        W.row(i) = L.row(i+1) - L.row(i);
        v(i) = mu(i) - mu(i+1);
    }

    // Wz est maintenu à jour incrémentalement pour éviter le produit matriciel O(N^2)
    ColumnVectorD Wz = W * z;

    // 4. Boucle de génération
    int totalIter = burnIn + thin; // On boucle jusqu'à l'échantillon souhaité
    for (int iter = 0; iter < totalIter; ++iter) {

        for (int inner = 0; inner < nIterPerSample; ++inner) {
            // Direction aléatoire vectorisée
            ColumnVectorD d = ColumnVectorD::NullaryExpr(N, [&](){
                return Generator::normalDistribution(0.0, 1.0);
            });
            d.normalize();

            // Calcul vectorisé des bornes (Wd est O(N^2), mais Wz est déjà calculé)
            ColumnVectorD Wd = W * d;

            double a = -1e150;
            double b = 1e150;

            // Cette boucle est très simple et bien optimisée par le compilateur
            for (int i = 0; i < N - 1; ++i) {
                double den = Wd(i);
                double num = v(i) - Wz(i);
                if (std::abs(den) > 1e-13) {
                    double bound = num / den;
                    if (den > 0) a = std::max(a, bound);
                    else        b = std::min(b, bound);
                } else if (num > 0) { // Contrainte impossible
                    a = 1; b = 0; break;
                }
            }

            if (a < b) {
                double mu_lambda = -d.dot(z);
                double lambda = truncatedNormal(0.0, a - mu_lambda, b - mu_lambda);

                // MISE À JOUR INCRÉMENTALE : O(N) au lieu de O(N^2)
                z.noalias() += lambda * d;
                Wz.noalias() += lambda * Wd;
            }
        }
    }

    // 5. Retour au domaine original : y = mu + Lz
    return mu + L * z;
}

