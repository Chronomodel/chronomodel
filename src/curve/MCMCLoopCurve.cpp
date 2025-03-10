/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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
#include "StdUtilities.h"
#include "Date.h"
#include "Project.h"
#include "Matrix.h"
#include "MainWindow.h"
#include "QtUtilities.h"
#include <cfenv>


#ifdef DEBUG
#include "QtUtilities.h"
#endif

#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QTime>
#include <QProgressDialog>

#include <errno.h>      /* errno, EDOM */
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
#ifdef S02_BAYESIAN
        //return initialize_400();
        return initialize_401(); // experimental
        #define notCODE_KOMLAN
        //return initialize_Komlan();
#else
        //return initialize_321();
        return initialize_330();
#endif


}

QString MCMCLoopCurve::initialize_321()
{
    updateLoop = &MCMCLoopCurve::update_321;

    std::vector<std::shared_ptr<Event>> &allEvents (mModel->mEvents);

    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
        mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
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

    SplineMatrices matricesWI = prepareCalculSpline_WI(current_vecH);
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

                if (mModel->compute_Z) {
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
        if (mCurveSettings.mUseVarianceIndividual) {
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

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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

            if (mModel->compute_Z) {
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

QString MCMCLoopCurve::initialize_330()
{
    updateLoop = &MCMCLoopCurve::update_330;

    std::vector<std::shared_ptr<Event>> &allEvents (mModel->mEvents);

    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
        mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (std::shared_ptr<Event> &ev : allEvents) {
            if (mModel->is_curve && ev->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                ev->mTheta.mSamplerProposal = MHVariable::eMHAdaptGauss;
            }
            if (ev->mPointType == Event::eNode)
                mNodeEvent.push_back(ev);
            else
                mPointEvent.push_back(ev);

            ev->mS02Theta.mSamplerProposal = MHVariable::eFixe; // not yet integrate within update_330
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

    SplineMatrices matricesWI = prepareCalculSpline_WI(current_vecH);
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
            mModel->mLambdaSpline.accept_update(1E5);//1.0E-6); // default = 1E-6.

        }
        mModel->mLambdaSpline.mSigmaMH = 1.0; // default = 1.

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

            double var_residual_X, var_residual_Y, var_residual_Z;
            if (mCurveSettings.mUseErrMesure) {
                const auto& matrices_Sy2 = prepareCalculSpline_Sy2(mModel->mEvents, current_vecH);
                var_residual_X = S02_Vg_Yx(mModel->mEvents, matrices_Sy2, current_vecH, mModel->mLambdaSpline.mX);
                if (mModel->compute_Y) {
                    var_residual_Y = S02_Vg_Yy(mModel->mEvents, matrices_Sy2, current_vecH, mModel->mLambdaSpline.mX);

                }
                if (mModel->compute_Z) {
                    var_residual_Z = S02_Vg_Yz(mModel->mEvents, matrices_Sy2, current_vecH, mModel->mLambdaSpline.mX);

                }

            } else {

                const auto& matrices_wi = prepareCalculSpline_WI(current_vecH);
                var_residual_X = S02_Vg_Yx(mModel->mEvents, matrices_wi, current_vecH, mModel->mLambdaSpline.mX);
                if (mModel->compute_Y) {
                    var_residual_Y = S02_Vg_Yy(mModel->mEvents, matrices_wi, current_vecH, mModel->mLambdaSpline.mX);

                }
                if (mModel->compute_Z) {
                    var_residual_Z = S02_Vg_Yz(mModel->mEvents, matrices_wi, current_vecH, mModel->mLambdaSpline.mX);

                }
            }


            //std::cout<<" var_residu_X = " << var_residu_X;
            if (mModel->compute_X_only) {
                Var_residual_spline = var_residual_X;

            } else {

                if (mModel->compute_Z) {
                    Var_residual_spline = (var_residual_X + var_residual_Y + var_residual_Z) / 3.0;

                } else {
                    Var_residual_spline = (var_residual_X + var_residual_Y) / 2.0;
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
        if (mCurveSettings.mUseVarianceIndividual) {
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
    // Curve init S02 Vg = Var_residual_spline
    // ----------------------------------------------------------------
    //mModel->mS02Vg.mX = Var_residual_spline;
    mModel->mS02Vg.mLastAccepts.clear();


    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed) {
        mModel->mS02Vg.mSamplerProposal = MHVariable::eFixe;
        double memoS02 = sqrt(mModel->mS02Vg.mX);
        mModel->mS02Vg.memo(&memoS02);

    } else {



       /* if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
            //mModel->mSO2_beta = 1.0;
            mModel->mSO2_beta = 2.0 * Calcul_Variance_Rice(mModel->mEvents);

        } else {
        */
        /*const double mean_y = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0., [](double s0, auto e) { return s0 + e->mYx;}) / mModel->mEvents.size();
        const double var_y = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0., [mean_y](double s0, auto e) { return s0 + pow(e->mYx - mean_y, 2.);}) / mModel->mEvents.size();
        const double mean_var_sy = mModel->mEvents.size() / std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0., [](double s0, auto e) { return s0 + e->mSy *  e->mSy;}) ;
        */
        //const double mean_y = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0., [](double s0, auto e) { return s0 + e->mYx;}) / mModel->mEvents.size();
        //const double var_y = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0., [mean_y](double s0, auto e) { return s0 + pow(e->mYx - mean_y, 2.);}) / mModel->mEvents.size();
        //const double mean_var_sy = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0., [](double s0, auto e) { return s0 + e->mSy *  e->mSy;}) / mModel->mEvents.size();


        // Faut-il normaliser les valeurs avant de calculer beta ??
//  ---------------- code alternatif
 /*       double x_min = mModel->mEvents[0]->mYx;
        double x_max = mModel->mEvents[0]->mYx;
        for (const auto &e : mModel->mEvents) {
            if (e->mYx - e->mSy < x_min) x_min = e->mYx - e->mSy;
            if (e->mYx + e->mSy > x_max) x_max = e->mYx + e->mSy;
        }
        double x_range = x_max - x_min;
        double mean_y = 0.0;
        std::vector<double> x_norm, sx_norm;
        // normalisation
        for (const auto &e : mModel->mEvents) {
            double x = (e->mYx - x_min)/x_range;
            mean_y += x;
            x_norm.push_back(x);
            sx_norm.push_back( e->mSy/x_range);
        }
        mean_y /= mModel->mEvents.size();

        //const double mean_y = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0., [](double s0, auto e) { return s0 + e->mYx;}) / mModel->mEvents.size();
        const double var_y = std::accumulate(x_norm.begin(), x_norm.end(), 0., [mean_y](double s0, auto e) { return s0 + pow(e - mean_y, 2.);}) / mModel->mEvents.size();
        const double mean_var_sy = std::accumulate(sx_norm.begin(), sx_norm.end(), 0., [](double s0, auto s) { return s0 + s *  s;}) / mModel->mEvents.size();

        const double used_s = sqrt((var_y + mean_var_sy)/2.0 );
        const double beta = 1.004680139*(1 - exp(- 0.0000847244 * pow(used_s, 2.373548593)));
        // normalisation
        size_t i = 0;
        for (const auto &e : mModel->mEvents) {
            e->mYx = x_norm[i];
            e->mSy = sx_norm[i];
            i++;
        }

*/
         // ---------      code origine, sans normalisation des Yx
        //const double s_harmonique = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0.0, [](double s0, auto e) { return s0 + 1.0/pow(e->mSy, 2.0);});
 /*       const double s_harmonique = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), 0.0,
            [](double s0, const auto& e) {
                const double sy_inv = 1.0 / e->mSy;
                return s0 + sy_inv * sy_inv;
            }
        );
        const double mean_var_harmonique = static_cast<double>(mModel->mEvents.size())/ s_harmonique;
        //const double mean_harmonique = sqrt(static_cast<double>(mModel->mEvents.size())/ s_harmonique);

        //const double beta =  1.004680139 * (1.0 - exp(- 0.0000847244 * pow(mean_harmonique, 2.373548593))) ; // Formule de Komlan

        //mModel->mSO2_beta = std::clamp(beta, 1.0E-10, 1.0); //Est-ce que beta peut être supérieur à 1?
        mModel->mSO2_beta = mean_var_harmonique; // peut être supérieur à 1
    //    }

        const double S02Vg = 1.0 / Generator::gammaDistribution(1.0, 1.0 / mModel->mSO2_beta);
#ifdef DEBUG
        if (S02Vg == INFINITY)
            qDebug()<<"[MCMCLoopCurve::initialize_330]  S02Vg == INFINITY";
#endif


        mModel->mS02Vg.tryUpdate(S02Vg, 2.0); // var_residual ??
   */

        mModel->mS02Vg.accept_update(Var_residual_spline);
    }

    mModel->mS02Vg.mSigmaMH = 1.; // inutile mS02Vg n'est plus Bayesien



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
    //  correction Lambda Spline
    // ----------------------------------------------------------------
   /* if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth) {
        double try_lambda = mModel->mLambdaSpline.mX;

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        //mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
        bool ok = false;
        double rate_tmp = 1.;
        for ( ; try_lambda <= 1.; try_lambda *= 10.) {
            test_depth(mModel->mEvents, current_vecH, current_splineMatrices, try_lambda, rate_tmp, ok);
            if (ok) break;
        }
        if ( ok == false) {
            mAbortedReason = QString(tr("Can not init smoothing !"));
            return mAbortedReason;

        } else {
            mModel->mLambdaSpline.mX = try_lambda;
        }

        qDebug()<< "[MCMCLoopCurve::initialize_330] lambda = "<< mModel->mLambdaSpline.mX;
    }*/

    // --------------------------- Current spline ----------------------
    try {
        /* --------------------------------------------------------------
         *  Calcul de la spline g, g" pour chaque composante x y z
         *-------------------------------------------------------------- */

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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

            const auto e = 0.1 *(maxY -minY);

            minY = minY - e;
            maxY = maxY + e;

            Scale sc ;
            sc.findOptimal(minY, maxY);

            clearMeanG.gy.mapG.setRangeY(sc.min, sc.max);

            if (mModel->compute_Z) {
                clearMeanG.gz = clearCompo;

                minY = +INFINITY;
                maxY = -INFINITY;

                minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double z, std::shared_ptr<Event> e) {return std::min(e->mYz, z);});
                maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double z, std::shared_ptr<Event> e) {return std::max(e->mYz, z);});

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

            const double deg = 180.0 / M_PI ;
            // 1 - new extrenum
            const double gzFmax = sqrt(pow(clearMeanG.gx.mapG.maxY(), 2.0) + pow(clearMeanG.gy.mapG.maxY(), 2.0) + pow(clearMeanG.gz.mapG.maxY(), 2.0));
            const double gxIncMax = asin(clearMeanG.gz.mapG.maxY() / gzFmax) * deg;
            const double gyDecMax = atan2(clearMeanG.gy.mapG.maxY(), clearMeanG.gx.mapG.maxY()) * deg;

            const double gzFmin = sqrt(pow(clearMeanG.gx.mapG.minY(), 2.0) + pow(clearMeanG.gy.mapG.minY(), 2.0) + pow(clearMeanG.gz.mapG.minY(), 2.0));
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
        return QString("[MCMCLoopCurve::initialize_330] problem");
    }


    return QString();
}




// MCMC Reinsch
QString MCMCLoopCurve::initialize_400()
{
    updateLoop = &MCMCLoopCurve::update_400;
    std::vector<std::shared_ptr<Event>>& allEvents (mModel->mEvents);

    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
        mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
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
    SplineMatrices matricesWI = prepareCalculSpline_WI(current_vecH);
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

                const std::vector<double> &vec_theta_red = get_vector(get_ThetaReduced, mModel->mEvents);
                const std::vector<double> &vec_tmp_x = get_vector(get_Yx, mModel->mEvents);
                const std::vector<double> &vec_tmp_x_err = get_vector(get_Sy, mModel->mEvents);

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

                if (mModel->compute_Z) {
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
        if (mCurveSettings.mUseVarianceIndividual) {
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
        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
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

            if (mModel->compute_Z) {
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

    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
        mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
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
    SplineMatrices matricesWI = prepareCalculSpline_WI(current_vecH);
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

                const std::vector<double> &vec_theta_red = get_vector(get_ThetaReduced, mModel->mEvents);
                const std::vector<double> &vec_tmp_x = get_vector(get_Yx, mModel->mEvents);
                const std::vector<double> &vec_tmp_x_err = get_vector(get_Sy, mModel->mEvents);

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

                if (mModel->compute_Z) {
                    const auto var_residu_Z =  S02_Vg_Yz(mModel->mEvents, matricesWI, current_vecH, mModel->mLambdaSpline.mX);
                    Var_residual_spline = (var_residu_X + var_residu_Y + var_residu_Z)/3.;

                } else {
                    Var_residual_spline = (var_residu_X + var_residu_Y)/2.;
                }

            }


        }

        // memo Vg


        int i = 0;
        if (mCurveSettings.mUseVarianceIndividual) {
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
        const std::vector<double> &vecY = get_vector(get_Yx, mModel->mEvents);
        //std::vector<double> vecY (mModel->mEvents.size());
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

        var_Y = variance_Knuth( vecY);

    } else if (mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec) { // à controler
        const std::vector<double> &vecY = get_vector(get_Yx, mModel->mEvents);
        //std::vector< double> vecY (mModel->mEvents.size());
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        const std::vector<double> &vecYy = get_vector(get_Yy, mModel->mEvents);
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecYy);

        var_Y /= 2.;

    } else {
        const std::vector<double> &vecY = get_vector(get_Yx, mModel->mEvents);
        //std::vector< double> vecY (mModel->mEvents.size());
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});
        var_Y = variance_Knuth( vecY);

        const std::vector<double> &vecYy = get_vector(get_Yy, mModel->mEvents);
        //std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYy;});
        var_Y += variance_Knuth( vecYy);

        const std::vector<double> &vecYz = get_vector(get_Yz, mModel->mEvents);
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
        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
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

            if (mModel->compute_Z) {
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


// MCMC Multi-sampling gaussian
QString MCMCLoopCurve::initialize_Komlan()
{
    updateLoop = &MCMCLoopCurve::update_Komlan;
    std::vector<std::shared_ptr<Event>>& allEvents (mModel->mEvents);

    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
        mCurveSettings.mUseVarianceIndividual = false;

    mNodeEvent.clear();
    mPointEvent.clear();

    if (mCurveSettings.mUseVarianceIndividual && mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
        for (std::shared_ptr<Event> ev : allEvents) {
            if (ev->mPointType == Event::eNode)
                mNodeEvent.push_back(ev);
            else
                mPointEvent.push_back(ev);

            ev->mS02Theta.mSupport = MHVariable::eRpStar;
            ev->mS02Theta.mSamplerProposal = MHVariable::eMHAdaptGauss;
            ev->mS02Theta.mSigmaMH = 1.;
        }
    } else {
        for (std::shared_ptr<Event> ev : allEvents)
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
    const SplineMatrices &matricesWI = prepareCalculSpline_WI(current_vecH);
    try {
        if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed) {
            mModel->mLambdaSpline.mX = mCurveSettings.mLambdaSpline;
            mModel->mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
            double memoLambda = log10(mModel->mLambdaSpline.mX);
            mModel->mLambdaSpline.memo(&memoLambda);

        } else {

            current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH) ;

            auto current_math = current_splineMatrices ;

            const auto R = seedMatrix(current_splineMatrices.matR, 1); // dim n-2 * n-2
            const int np = (int)R.size();
            const int mm = 3*np ;

            const auto Q = remove_bands_Matrix(current_splineMatrices.matQ, 1); // dim n * n-2
            const auto QT = transpose0(Q);

            Matrix2D  matRInv ;
            if (np <= 3) {
                matRInv = inverseMatSym0(R, 0) ;
            } else {
                const std::pair<Matrix2D, std::vector<double>> &decomp = decompositionCholeskyKK(R, 3, 0);

                matRInv = inverseMatSym_originKK(decomp.first, decomp.second, mm, 0);
            }

            const auto R_1QT = multiMatParMat0(matRInv, QT);

            const auto K = multiMatParMat0(Q, R_1QT) ;

            const auto vecG = get_vector(get_Yx, mModel->mEvents) ;

            auto fK = multiMatByVectCol0(K, vecG);

            // produit fK*ft et la trace de la matrice K
            double som = 0.;
            for (size_t i = 0; i < K.size(); ++i) {
                som += fK[i] * vecG[i];

            }

            double meanEexp = 2. / som ;
            //const double meanEexp = som / 2.;


            if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth ) {
                bool has_positif = false;
                do {
                    std::for_each( mModel->mEvents.begin(), mModel->mEvents.end(), [](std::shared_ptr<Event>e) { e->mW = 1./e->mSy; });
                    const auto &spline_matrice = prepareCalculSpline(mModel->mEvents, current_vecH);

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

                if ( mModel->compute_Z) {
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
        if (mCurveSettings.mUseVarianceIndividual) {
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

    } else if (!mModel->compute_Z) {
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

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
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

            if (mModel->compute_Z) {
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

/**
 * @brief MCMCLoopCurve::initialize_interpolate pour l'initialisation, il n'y a pas de VG donc équivalent à un modèle avec que des nodes
 * @return
 */

QString MCMCLoopCurve::initialize_interpolate()
{
    updateLoop = &MCMCLoopCurve::update_interpolate;
    const std::vector<std::shared_ptr<Event>>& allEvents (mModel->mEvents);

    initListEvents.resize(mModel->mEvents.size());
    std::copy(mModel->mEvents.begin(), mModel->mEvents.end(), initListEvents.begin() );

    mCurveSettings.mUseVarianceIndividual = false;

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
    mModel->mS02Vg.mX = 0;
    mModel->mS02Vg.mLastAccepts.clear();
    mModel->mS02Vg.mSamplerProposal = MHVariable::eFixe;
    mModel->mS02Vg.memo();

    mModel->mS02Vg.mSigmaMH = 1.;

    if (mModel->compute_X_only) {
        std::vector<double> vecY (mModel->mEvents.size());
        std::transform(mModel->mEvents.begin(), mModel->mEvents.end(), vecY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

        var_Y = variance_Knuth( vecY);

    } else if (!mModel->compute_Z) {
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
        mModel->mSpline = currentSpline_WI(mModel->mEvents, mModel->compute_Y, mModel->compute_Z, mCurveSettings.mUseErrMesure);

        // init Posterior MeanG and map
        const int nbPoint = 300;// map size and curve size

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


        double maxVarY = *std::max_element(mModel->mSpline.splineX.vecVarG.begin(), mModel->mSpline.splineX.vecVarG.end());
        double spanY_X =  0;
        minY = minY - 1.96*sqrt(maxVarY) - spanY_X;
        maxY = maxY + 1.96*sqrt(maxVarY) + spanY_X;

        clearMeanG.gx.mapG.setRangeY(minY, maxY);
        clearMeanG.gx.mapGP.setRangeY(+INFINITY, -INFINITY);

        if ( mModel->compute_Y) {
            clearMeanG.gy = clearCompo;

            minY = +INFINITY;
            maxY = -INFINITY;

            minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYy, x);});
            maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYy, x);});

            maxVarY = *std::max_element(mModel->mSpline.splineY.vecVarG.begin(), mModel->mSpline.splineY.vecVarG.end());
            spanY_X = 0;
            minY = minY - 1.96*sqrt(maxVarY) - spanY_X;
            maxY = maxY + 1.96*sqrt(maxVarY) + spanY_X;

            clearMeanG.gy.mapG.setRangeY(minY, maxY);
            clearMeanG.gy.mapGP.setRangeY(+INFINITY, -INFINITY);

            if (mModel->compute_Z) {
                clearMeanG.gz = clearCompo;

                minY = +INFINITY;
                maxY = -INFINITY;

                maxVarY = *std::max_element(mModel->mSpline.splineZ.vecVarG.begin(), mModel->mSpline.splineZ.vecVarG.end());

                minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYz, x);});
                maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYz, x);});

                spanY_X = 0;
                minY = minY - 1.96*sqrt(maxVarY) - spanY_X;
                maxY = maxY + 1.96*sqrt(maxVarY) + spanY_X;

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


bool MCMCLoopCurve::update()
{
    return   (this->*updateLoop)();

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

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        current_decomp_QTQ = decompositionCholesky(current_splineMatrices.matQTQ, 5, 1); // used only with update Theta
        //auto current_decomp_2 = choleskyLDLT_Dsup0(current_splineMatrices.matQTQ, 5, 1);


        current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);

        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH
        current_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. :
                                 ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);

        if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
            current_h_lambda = 1;
        else
            current_h_lambda = h_lambda(current_splineMatrices,  (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX) ;


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
                            try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, try_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

                            if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
                                try_h_lambda = 1;
                            else
                                try_h_lambda = h_lambda( try_splineMatrices, (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX);

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
         *  D - Update S02 Vg / Curve Shrinkage
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
                    const double try_value_log = Generator::gaussByBoxMuller(log10(mModel->mS02Vg.mX), mModel->mS02Vg.mSigmaMH);
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

                const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mS02Vg.mSigmaMH);

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
                        try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
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
                        std::swap(current_splineMatrices, try_splineMatrices);
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

                const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mEvents.at(0)->mVg.mSigmaMH);
                const double try_value = pow(10, try_value_log);

                // Affectation temporaire pour évaluer la nouvelle proba
                // Dans le cas global pas de différence ente les Points et les Nodes
                for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                        ev->mVg.mX = try_value;
                        ev->updateW();
                }

                if (try_value_log >= logMin && try_value_log <= logMax) {

                    // Calcul du rapport : try_matrices utilise les temps reduits, elle est affectée par le changement de Vg
                    try_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);

                    try_decomp_matB = decomp_matB(try_splineMatrices, mModel->mLambdaSpline.mX);

                    try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);
                    try_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. : ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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
                        current_splineMatrices = std::move(try_splineMatrices);
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
                        try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                        //try_value_log = Generator::gaussByDoubleExp(log10(current_value), mModel->mLambdaSpline.mSigmaMH, logMin, logMax); //nouveau code
                        try_value = pow(10., try_value_log);
                        counter++;
                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            const auto try_spline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, try_value, mModel->compute_Y, mModel->compute_Z);
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
                    const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);

                    const double try_value = pow(10., try_value_log);

                    // --

                    if (try_value_log >= logMin && try_value_log <= logMax) {

                        // Calcul du rapport :
                        mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                        try_h_lambda = h_lambda(current_splineMatrices, (int)mModel->mEvents.size(), try_value) ;
                        try_decomp_matB = decomp_matB(current_splineMatrices, try_value);
                        try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_Z);
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
                    mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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
                    const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);

                    const double try_value = pow(10., try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {
                        //current_h_lambda n'a pas changer ,depuis update theta

                        // Calcul du rapport :
                        mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                        try_h_lambda = h_lambda(current_splineMatrices, (int)mModel->mEvents.size(), try_value) ;
                        try_decomp_matB = decomp_matB(current_splineMatrices, try_value);
                        try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_Z);
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
                    mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

                }
            // Pas bayésien : on sauvegarde la valeur constante dans la trace
            } else { // Rien à faire
                /* --------------------------------------------------------------
                 *  G - Update mModel->mSpline
                 * -------------------------------------------------------------- */
                // G.1- Calcul spline
                mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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

        //-- code d'origine
        // G.1- Calcul spline
        //mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);



        // G.2 - test GPrime positive
    /*    if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth  ) {
            return hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

        } else
            return true;
*/
        // fin code d'origine

    } catch (const char* e) {
        qWarning() << "[MCMCLoopCurve::update] char "<< e;

    } catch (const std::length_error& e) {
        qWarning() << "[MCMCLoopCurve::update] Length_error"<< e.what();

    } catch (const std::out_of_range& e) {
        qWarning() << "[MCMCLoopCurve::update] Out of range" <<e.what();

    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve_321::update]  "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update_321] Caught Exception!\n";
        return false;
    }

    return false;
}

const double mixing_depth = 1.0;
void MCMCLoopCurve::test_depth(std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const SplineMatrices &matrices, const double lambda, double &rate, bool &ok)
{

    if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth  ) {

        const auto& test_spline = currentSpline(events, vecH, matrices, lambda, false, false);

        ok = hasPositiveGPrimePlusConst(test_spline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

        if (!ok) { // mixing depth
            const double uniform = Generator::randomUniform();
            rate = (uniform <= mixing_depth) ? rate : -1.;

        }



    } else {
        ok = true;
    }
}

bool MCMCLoopCurve::update_330()
{
#ifdef DEBUG
    int modif_value = 0;
#endif

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

        current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);

        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH
        current_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0.0 ? 0.0 :
                                 ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);

        if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
            current_h_lambda = 1.0;
        else
            current_h_lambda = h_lambda(current_splineMatrices,  (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX) ;


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
                qWarning() << "[MCMCLoopCurve::update_330] Ti : Caught Exception!\n"<<exc.what();

            }  catch (...) {
                qWarning() << "[MCMCLoopCurve::update_330] Ti : Caught Exception!";
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
                        current_h_theta = h_theta_Event(event);

                        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH

                        // On tire une nouvelle valeur :
                        const double try_value = Generator::gaussByBoxMuller(current_value, event->mTheta.mSigmaMH);
                        bool ok = true;

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
                            try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, try_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

                            if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
                                try_h_lambda = 1.0;
                            else
                                try_h_lambda = h_lambda( try_splineMatrices, (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX);

                            try_h_theta = h_theta_Event(event);

                            rate = (try_h_lambda * try_h_theta) / (current_h_lambda * current_h_theta) * exp(0.5 * ( try_ln_h_YWI_1_2 + try_ln_h_YWI_3
                                                                                                                     - current_ln_h_YWI_1_2 - current_ln_h_YWI_3));



                            //test_depth(mModel->mEvents, try_vecH, try_splineMatrices, mModel->mLambdaSpline.mX, rate, ok);

                            if (!ok)
                                qDebug()<<"rejetéé "<< event->name()<<rate;

                        } else {
                            rate = -1.;
                            ok = false;

                        }


                        // restore Theta to used try_update()
                        //event->mTheta.mX = current_value;
                        //event->mTheta.test_update(current_value, try_value, rate);
                        // update after tryUpdate or updateTheta
                        //event->mThetaReduced = mModel->reduceTime(event->mTheta.mX);

                        if ( event->mTheta.test_update(current_value, try_value, rate)) { // test_update() return true if accepted
#ifdef DEBUG
                        modif_value++;
#endif
                            // Pour l'itération suivante :
                            std::swap(current_ln_h_YWI_1_2, try_ln_h_YWI_1_2);
                            std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);

                            std::swap(current_vecH, try_vecH);
                            std::swap(current_splineMatrices, try_splineMatrices);
                            std::swap(current_h_lambda, try_h_lambda);
                            std::swap(current_decomp_matB, try_decomp_matB);
                            std::swap(current_decomp_QTQ, try_decomp_QTQ);

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

           // current_vecH = calculVecH(mModel->mEvents);
           // current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
           // current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);

           // current_ln_h_YWI_3 = ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
          //  current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB); // utile pour Vg



        /* --------------------------------------------------------------
         *  E - Update Vg
         *  D.1 - Update Vg for Points only
         *  D.2 - Update Vg Global
         * -------------------------------------------------------------- */
        try {
            constexpr double logMin = -10.0;
            constexpr double logMax = +20.0;

            /* --------------------------------------------------------------
            *  D - Update Vg
            * -------------------------------------------------------------- */

            if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

                current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB); // Has not been initialized yet

                /* --------------------------------------------------------------
                *  D.1 - Update Vg for Points only, not the node
                * -------------------------------------------------------------- */

                for (std::shared_ptr<Event>& event : mPointEvent)   {

                    const double current_value = event->mVg.mX;
                    current_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                   // current_vecH = calculVecH(mModel->mEvents); // utilise e-> thetareduce
                    current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
                    current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);

                    current_ln_h_YWI_3 = ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
                    current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB);

                    // On tire une nouvelle valeur :
                    const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), event->mVg.mSigmaMH);
                    const double try_value = pow(10., try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {
                        // On force la mise à jour de la nouvelle valeur pour calculer try_h
                        // A chaque fois qu'on modifie VG, W change !
                        event->mVg.mX = try_value;
                        event->updateW(); // used by prepareCalculSpline

                        // Calcul du rapport : matrices utilise les temps reduits, elle est affectée par le changement de VG

                        try_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH); // utilise vecH et e->mW
                        try_decomp_matB = decomp_matB(try_splineMatrices, mModel->mLambdaSpline.mX);

                        try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
                        try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);
                        try_h_VG = h_VG_Event(event, mModel->mS02Vg.mX);

                        rate = (try_h_VG * try_value) / (current_h_VG * current_value) * exp(0.5 * ( try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                    - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                    } else {
                        rate = -1.; // force reject // force to keep current state

                    }

                    bool ok = true;

                    //test_depth(mModel->mEvents, current_vecH, try_splineMatrices, mModel->mLambdaSpline.mX, rate, ok);


                    if (!ok)
                        qDebug()<<"rejetéé VG "<< rate;

                    // Metropolis Hastings update
                    // A chaque fois qu'on modifie VG, W change !
                    //event->mVg.mX = current_value;
                    //event->mVg.try_update(try_value, rate);
                    //event->updateW();

                    //if ( event->mVg.mLastAccepts.back() == true) {
                    if ( event->mVg.test_update(current_value, try_value, rate)) {
#ifdef DEBUG
                        modif_value++;
#endif
                        // Pour l'itération suivante : Car mVg a changé
                        std::swap(current_ln_h_YWI_2, try_ln_h_YWI_2);
                        std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);
                        std::swap(current_splineMatrices, try_splineMatrices);
                        std::swap(current_decomp_matB, try_decomp_matB);
                    }
                    event->updateW();
                }


            } else if (mCurveSettings.mVarianceType == CurveSettings::eModeGlobal) {
                current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB); // Has not been initialized yet

                /* --------------------------------------------------------------
                *  D.2 - Update Vg Global
                * -------------------------------------------------------------- */

                // On stocke l'ancienne valeur :
                const double current_value = mModel->mEvents[0]->mVg.mX;


                // On tire une nouvelle valeur :

                const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mEvents[0]->mVg.mSigmaMH);
                const double try_value = pow(10, try_value_log);

                if (try_value_log >= logMin && try_value_log <= logMax) {

                    current_h_VG = h_VG_Event(current_value, mModel->mS02Vg.mX);

                    // Affectation temporaire pour évaluer la nouvelle proba
                    // Dans le cas global pas de différence ente les Points et les Nodes
                    for (std::shared_ptr<Event>& ev : mModel->mEvents) {
                            ev->mVg.mX = try_value;
                            ev->updateW();
                    }

                    // Calcul du rapport : try_splineMatrices utilise les temps reduits, elle est affectée par le changement de Vg
                    try_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH); // utilise vecH et e->mW

                    try_decomp_matB = decomp_matB(try_splineMatrices, mModel->mLambdaSpline.mX);

                    try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);
                    try_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0.0 ? 0.0 : ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

                    try_h_VG = h_VG_Event(try_value, mModel->mS02Vg.mX);

                    const auto ln_rate = ( try_ln_h_YWI_2 + try_ln_h_YWI_3   - current_ln_h_YWI_2 - current_ln_h_YWI_3);
                    rate = (try_h_VG * try_value) / (current_h_VG * current_value) * exp(0.5 * ln_rate);

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

#ifdef DEBUG
                        modif_value++;
#endif
                        std::swap(try_ln_h_YWI_2, current_ln_h_YWI_2);
                        std::swap(try_ln_h_YWI_3, current_ln_h_YWI_3);
                        std::swap(try_splineMatrices, current_splineMatrices);
                        std::swap(try_decomp_matB, current_decomp_matB);

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

        //current_vecH = calculVecH(mModel->mEvents);
        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH); // utilise vecH et e->mW
        current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);


        bool ok = true;

        try {
            constexpr double lambda_logMin = -20.0;
            constexpr double lambda_logMax = +10.0;

            if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {

                current_ln_h_YWI_3 = ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
                current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB);

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
                        try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                        //try_value_log = Generator::gaussByDoubleExp(log10(current_value), mModel->mLambdaSpline.mSigmaMH, logMin, logMax); //nouveau code
                        try_value = pow(10., try_value_log);
                        counter++;
                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            const auto try_spline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, try_value, mModel->compute_Y, mModel->compute_Z);
                            ok = hasPositiveGPrimePlusConst(try_spline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

                            if (!ok)
                                logMin = try_value_log;

                        }


                    } while (!ok && counter<10);
                    logMin = -20.;
                    */

                    //-- Code origine


                    // On stocke l'ancienne valeur :
                    const double current_value = mModel->mLambdaSpline.mX;

                    // On tire une nouvelle valeur :
                    const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);

                    const double try_value = pow(10., try_value_log);

                    // --

                    if (try_value_log >= lambda_logMin && try_value_log <= lambda_logMax) {

                        // Calcul du rapport :
                        mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                        try_h_lambda = h_lambda(current_splineMatrices, (int)mModel->mEvents.size(), try_value) ;
                        try_decomp_matB = decomp_matB(current_splineMatrices, try_value);
                        try_ln_h_YWI_3 = try_value == 0.0 ? 0.0 : ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_Z);
                        try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);

                        const auto n = mModel->mEvents.size();

                        rate = (try_h_lambda * try_value) / (current_h_lambda * current_value)  * exp( 0.5 *  ( (n-2)*log(try_value/current_value)
                                                                                                            + try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                            - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                        //test_depth(mModel->mEvents, current_vecH, current_splineMatrices, try_value, rate, ok);

                        if (!ok)
                            qDebug()<<"rejeter lambda "<< rate;

                    } else {
                        rate = -1.; // force reject
                    }



                    //mModel->mLambdaSpline.mX = current_value;
                   // mModel->mLambdaSpline.try_update(try_value, rate);
                    mModel->mLambdaSpline.test_update(current_value, try_value, rate);

                    if (mModel->mLambdaSpline.mLastAccepts.back() == true) {
#ifdef DEBUG
                        modif_value++;
#endif

                       // std::swap(current_ln_h_YWI_2, try_ln_h_YWI_2);
                       // std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);
                       // std::swap(current_splineMatrices, try_splineMatrices);
                       // std::swap(current_decomp_matB, try_decomp_matB);

                    }

//qDebug()<<"modif_value =" << modif_value << (mModel->mLambdaSpline.mLastAccepts.back() == (mModel->mLambdaSpline.mX == try_value));

                    // il faut refaire le test car on ne sait pas si l'ancien lambda donnait positif
                    // G.1- Calcul spline
                    //test_depth(rate, ok); // pas utile car toujours vérifié
                    //if (!ok)
                    //    qDebug()<<"rejetéé lambda confirme "<< rate;

                    //qDebug()<<"[MCMCLoopCurve::update_330] Depth counter="<<counter;



                } else {  // Not Depth


                    // On stocke l'ancienne valeur :
                    const double current_value = mModel->mLambdaSpline.mX;

                    // On tire une nouvelle valeur :
                    const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);

                    const double try_value = pow(10.0, try_value_log);

                    if (try_value_log >= lambda_logMin && try_value_log <= lambda_logMax) {
                        //current_h_lambda n'a pas changer, depuis update theta

                        // Calcul du rapport :

                        const int n = mModel->mEvents.size();

                        try_h_lambda = h_lambda(current_splineMatrices, n, try_value) ;

                        try_decomp_matB = decomp_matB(current_splineMatrices, try_value);
                        try_ln_h_YWI_3 = try_value == 0.0 ? 0.0 : ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_Z);
                        try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);



                        rate = (try_h_lambda * try_value) / (current_h_lambda * current_value)  * exp( 0.5 *  ( (n-2)*log(try_value/current_value)
                                                                                                            + try_ln_h_YWI_2 + try_ln_h_YWI_3
                                                                                                            - current_ln_h_YWI_2 - current_ln_h_YWI_3));

                        //mModel->mLambdaSpline.mX = current_value;
                        //mModel->mLambdaSpline.try_update(try_value, rate);
                         mModel->mLambdaSpline.test_update(current_value, try_value, rate);


                    } else {
                        //mModel->mLambdaSpline.mX = current_value;
                        mModel->mLambdaSpline.reject_update();
                       // rate = -1.0; // force reject
                    }



                    /* dernier update, les matrices de calcul ne sont plus utiles
                    if (mModel->mLambdaSpline.mLastAccepts.back() == true) { // utile pour
                    //if ( true) {
                        modif_value++;
                        std::swap(current_ln_h_YWI_2, try_ln_h_YWI_2);
                        std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);
                        //std::swap(current_splineMatrices, try_splineMatrices); // déjà à jour
                        std::swap(current_decomp_matB, try_decomp_matB);

                    }
                    */
                    //qDebug()<<"modif_value="<<modif_value << (mModel->mLambdaSpline.mLastAccepts.back() == (mModel->mLambdaSpline.mX == try_value));

                   // ok = true;
                    /* --------------------------------------------------------------
                     *  G - Update mModel->mSpline
                     * -------------------------------------------------------------- */
                    // G.1- Calcul spline
                    //mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

                }


            }


        /* --------------------------------------------------------------
        * F - Update mS02Vg
        * -------------------------------------------------------------- */


            if (mCurveSettings.mVarianceType != CurveSettings::eModeFixed) {
                const auto& mat_WI = prepareCalculSpline_WI(current_vecH);
                const auto var_residu_X = S02_Vg_Yx(mModel->mEvents, mat_WI, current_vecH, mModel->mLambdaSpline.mX);

                qDebug()<< "mS02Vg="<< var_residu_X;
                mModel->mS02Vg.accept_update(var_residu_X);
            }
#ifdef DEBUG
           // const auto& mat_sy = prepareCalculSpline_Sy2(mModel->mEvents, current_vecH);
           // const auto var_residu_X = S02_Vg_Yx(mModel->mEvents, mat_sy, current_vecH, mModel->mLambdaSpline.mX);
           // qDebug()<< "var residuel"<< var_residu_X;
#endif

        /* --------------------------------------------------------------
        *  G - Update mModel->mSpline
        * -------------------------------------------------------------- */
            // G.1- Calcul spline
            mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);


            // G.2 - test GPrime positive
            if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth  ) {
                ok = hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation
                if (!ok)
                    qDebug()<<"[MCMCLoopCurve::update_330] rejetéé GPrime positive  "<< rate;

            } else
                ok = true;

        } catch(...) {
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

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        current_decomp_QTQ = decompositionCholesky(current_splineMatrices.matQTQ, 5, 1); // used only with update Theta

        current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);

        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH
        current_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. :
                                 ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);

        if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
            current_h_lambda = 1;
        else
            current_h_lambda = h_lambda(current_splineMatrices,  (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX) ;


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

                const Matrix2D &R = seedMatrix(current_splineMatrices.matR, 1); // dim n-2 * n-2
                const int np = (int)R.size();
                const int mm = 3*np ;

                const Matrix2D &Q = remove_bands_Matrix(current_splineMatrices.matQ, 1); // dim n * n-2
                const Matrix2D &QT = transpose0(Q);

                Matrix2D  matRInv ;
                if (np <= 3) {
                    matRInv = inverseMatSym0(R, 0) ;

                } else {
                    const std::pair<Matrix2D, std::vector<double>> &decomp = decompositionCholeskyKK(R, 3, 0);
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
                            try_splineMatrices = prepareCalculSpline(mModel->mEvents, try_vecH);

                            const Matrix2D &try_R = seedMatrix(try_splineMatrices.matR, 1); // dim n-2 * n-2
                            const Matrix2D &try_Q = remove_bands_Matrix(try_splineMatrices.matQ, 1); // dim n * n-2

                            const Matrix2D &try_QT = transpose0(try_Q);

                            const int try_np = (int) try_R.size();
                            Matrix2D  try_matRInv ;
                            if (try_np <= 3) {
                                try_matRInv = inverseMatSym0(try_R, 0);

                            } else {
                                const std::pair<Matrix2D, MatrixDiag> &try_decomp = decompositionCholesky(try_R, 3, 0);
                                try_matRInv = inverseMatSym_origin(try_decomp, mm, 0);

                            }

                            const auto &try_K = multiMatParMat0(multiMatParMat0(try_Q, try_matRInv), try_QT);

                            const double try_h_lambda_komlan = h_lambda_Komlan(K, try_K, (int)mModel->mEvents.size(), mModel->mLambdaSpline.mX);
                            double try_fTKf = rapport_Theta(get_Gx, mModel->mEvents, K, try_K, mModel->mLambdaSpline.mX) ;

                            if (mModel->compute_Y)
                                try_fTKf *= rapport_Theta(get_Gy, mModel->mEvents, K, try_K, mModel->mLambdaSpline.mX) ;

                            if (mModel->compute_Z)
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
                            //current_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, try_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

                            std::swap(current_vecH, try_vecH);
                            std::swap(current_splineMatrices, try_splineMatrices);
                            //current_splineMatrices = prepareCalculSpline(mModel->mEvents, try_vecH);

                           // std::swap(current_h_lambda, try_h_lambda);
                            //std::swap(current_decomp_matB, try_decomp_matB);
                            //current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);
                            //std::swap(current_decomp_QTQ, try_decomp_QTQ);

                            //current_decomp_QTQ = decompositionCholesky(current_splineMatrices.matQTQ, 5, 1);
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

        current_decomp_QTQ = decompositionCholesky(current_splineMatrices.matQTQ, 5, 1);
        current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);
        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);
        current_ln_h_YWI_3 = ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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

                    const double try_value_log = Generator::gaussByBoxMuller(log10(mModel->mS02Vg.mX), mModel->mS02Vg.mSigmaMH);
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

                            try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
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
                            std::swap(current_splineMatrices, try_splineMatrices);
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

                    const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mEvents.at(0)->mVg.mSigmaMH);
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
                        try_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);

                        try_decomp_matB = decomp_matB(try_splineMatrices, mModel->mLambdaSpline.mX);

                        try_ln_h_YWI_2 = ln_h_YWI_2(try_decomp_matB);
                        try_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. : ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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
                            current_splineMatrices = std::move(try_splineMatrices);
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
                        // On stocke l'ancienne valeur :
                        current_h_lambda = h_lambda(current_splineMatrices, (int)mModel->mEvents.size(), current_value) ;// current_h_lambda provient h_lambda_komlan() donc faire une maj

                        // -- Nouveau code
/*                         int counter = 10; // pour test standard counter=10


                        do {

                            // On tire une nouvelle valeur :
                            //try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                            try_value_log = Generator::gaussByDoubleExp(log10(current_value), mModel->mLambdaSpline.mSigmaMH, logMin, logMax); //nouveau code
                            try_value = pow(10., try_value_log);

                            counter++;
                            if (try_value_log >= logMin && try_value_log <= logMax) {
                                mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                                const auto try_spline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, try_value, mModel->compute_Y, mModel->compute_Z);
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
                        try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);

                        //try_value_log = Generator::gaussByDoubleExp(log10(current_value), mModel->mLambdaSpline.mSigmaMH, logMin, logMax);
                        try_value = pow(10., try_value_log);

                        mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                        const auto try_spline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, try_value, mModel->compute_Y, mModel->compute_Z);
                        ok = hasPositiveGPrimePlusConst(try_spline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

                        // -- Fin ancien code


                        logMin = -20.;
                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            // Calcul du rapport :
                            mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                            try_h_lambda = h_lambda(current_splineMatrices, (int)mModel->mEvents.size(), try_value) ;
                            try_decomp_matB = decomp_matB(current_splineMatrices, try_value);
                            try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_Z);
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
                        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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
                    current_h_lambda = h_lambda(current_splineMatrices, (int)mModel->mEvents.size(), current_value) ;// current_h_lambda provient h_lambda_komlan() donc faire une maj

                    // On tire une nouvelle valeur :
                     const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                     const double try_value = pow(10., try_value_log);

                    //const double try_value_log = Generator::gaussByBoxMuller(log2(current_value), mModel->mLambdaSpline.mSigmaMH); // pour test komlan
                    //const double try_value = exp2(try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {

                            // Calcul du rapport :
                            mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                            try_h_lambda = h_lambda(current_splineMatrices, (int)mModel->mEvents.size(), try_value) ;
                            try_decomp_matB = decomp_matB(current_splineMatrices, try_value);
                            try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_Z);
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
                    mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

                    ok = true;

                }


            } else { // Nothing to do
                // Pas bayésien : on sauvegarde la valeur constante dans la trace
                // G.1- Calcul spline
                mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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
        qWarning() << "[MCMCLoopCurve::update] out of range" <<e.what();

    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve::update]  "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update] Caught Exception!\n";
        return false;
    }

    return false;
}

/* Obsolete*/
/*
bool MCMCLoopCurve::update_401()
{
    try {

        t_prob rate;

        // init the current state
        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);

        current_vecH = calculVecH(mModel->mEvents);

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);
        current_decomp_QTQ = decompositionCholesky(current_splineMatrices.matQTQ, 5, 1); // used only with update Theta

        current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);

        //La partie h_YWI_3 = exp(ln_h_YWI_3) est placée dans le rapport MH
        current_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. :
                                 ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);

        if (mModel->mLambdaSpline.mSamplerProposal == MHVariable::eFixe)
            current_h_lambda = 1;
        else
            current_h_lambda = h_lambda(current_splineMatrices,  mModel->mEvents.size(), mModel->mLambdaSpline.mX) ;


        if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {



            try {

                const Matrix2D &R = seedMatrix(current_splineMatrices.matR, 1); // dim n-2 * n-2
                const int np = R.size();
                const int mm = 3*np ;

                const Matrix2D &Q = remove_bands_Matrix(current_splineMatrices.matQ, 1); // dim n * n-2
                const Matrix2D &QT = transpose0(Q);

                Matrix2D  matRInv ;
                if (np <= 5) {
                    matRInv = inverseMatSym0(R, 0) ;

                } else {
                    const std::pair<Matrix2D, std::vector<double>> &decomp = decompositionCholeskyKK(R, 3, 0);
                    matRInv = inverseMatSym_originKK(decomp.first, decomp.second, mm, 0);
                }

                auto K = multiMatParMat0(multiMatParMat0(Q, matRInv), QT) ;

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
                        //current_h_theta = h_theta_Event(event);

                        //----- Nouveau code


                        // tirage de theta
                        //const double try_value  = Generator::gaussByBoxMuller(current_value, event->mTheta.mSigmaMH);
                        const double try_value  = sample_in_repartition(event->mMixingCalibrations, min, max);
                        qDebug()<<"try"<< try_value;
                        rate = 1.;


                        if (try_value >= min && try_value <= max) {

                            for (auto&& date : event->mDates )   {
                                rate *= date.mCalibration->interpolate(date.xi_current*date.mSigmaTi.mX + try_value) / date.mCalibration->interpolate(date.xi_current*date.mSigmaTi.mX + event->mTheta.mX);
                            }

                            // On force la mise à jour de la nouvelle valeur pour calculer h_new

                            event->mTheta.mX = try_value; // Utile pour h_theta_Event()
                            event->mThetaReduced = mModel->reduceTime(try_value);

                            orderEventsByThetaReduced(mModel->mEvents); // On réordonne les Events suivant les thetas Réduits croissants
                            spreadEventsThetaReduced0(mModel->mEvents); // On espace les temps si il y a égalité de date

                            try_vecH = calculVecH(mModel->mEvents);
                            try_splineMatrices = prepareCalculSpline(mModel->mEvents, try_vecH);

                            const Matrix2D &try_R = seedMatrix(try_splineMatrices.matR, 1); // dim n-2 * n-2
                            const Matrix2D &try_Q = remove_bands_Matrix(try_splineMatrices.matQ, 1); // dim n * n-2

                            const Matrix2D &try_QT = transpose0(try_Q);

                            const int try_np = try_R.size();
                            Matrix2D  try_matRInv ;
                            if (try_np <= 3) {
                                try_matRInv = inverseMatSym0(try_R, 0);

                            } else {
                                const std::pair<Matrix2D, MatrixDiag> &try_decomp = decompositionCholesky(try_R, 3, 0);
                                try_matRInv = inverseMatSym_origin(try_decomp, mm, 0);

                            }

                            const auto &try_K = multiMatParMat0(multiMatParMat0(try_Q, try_matRInv), try_QT);

                            const double try_h_lambda_komlan = h_lambda_Komlan(K, try_K, mModel->mEvents.size(), mModel->mLambdaSpline.mX);
                            double try_fTKf = rapport_Theta(get_Gx, mModel->mEvents, K, try_K, mModel->mLambdaSpline.mX) ;

                            if (mModel->compute_Y)
                                try_fTKf *= rapport_Theta(get_Gy, mModel->mEvents, K, try_K, mModel->mLambdaSpline.mX) ;

                            if (mModel->compute_Z)
                                try_fTKf *= rapport_Theta(get_Gz, mModel->mEvents, K, try_K, mModel->mLambdaSpline.mX) ;


                            const double rate_detPlus = rapport_detK_plus(K,  try_K) ;

                            rate *=  rate_detPlus * try_fTKf * try_h_lambda_komlan ;

                        } else {
                            rate = -1.;

                        }

                        // restore Theta to used function tryUpdate

                        event->mTheta.mX = current_value;
                        event->mTheta.tryUpdate(try_value, rate);


                        if ( event->mTheta.mLastAccepts.last() == true) {
                            // Pour l'itération suivante :
                            //std::swap(current_ln_h_YWI_1_2, try_ln_h_YWI_1_2);
                            //std::swap(current_ln_h_YWI_3, try_ln_h_YWI_3);
                            //current_ln_h_YWI_1_2 = ln_h_YWI_1_2(try_decomp_QTQ, try_decomp_matB);
                            //current_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, try_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

                            std::swap(current_vecH, try_vecH);
                            std::swap(current_splineMatrices, try_splineMatrices);
                            //current_splineMatrices = prepareCalculSpline(mModel->mEvents, try_vecH);

                           // std::swap(current_h_lambda, try_h_lambda);
                            //std::swap(current_decomp_matB, try_decomp_matB);
                            //current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);
                            //std::swap(current_decomp_QTQ, try_decomp_QTQ);

                            //current_decomp_QTQ = decompositionCholesky(current_splineMatrices.matQTQ, 5, 1);
                        }

                        // -------------


                        // -------------------------------------------------
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


                    //  Update Phases -set mAlpha and mBeta ; they coud be used by the Event in the other Phase ----------------------------------------
                    std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (Phase* p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});

                } // End of loop initListEvents

                //  Update Phases Tau; they coud be used by the Event in the other Phase ----------------------------------------
                std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (Phase* p) {p->update_Tau (tminPeriod, tmaxPeriod);});
              std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (PhaseConstraint* pc) {pc->updateGamma();});

            } catch(std::exception& exc) {
                qWarning() << "[MCMCLoopCurve::update] Theta : Caught Exception!\n"<<exc.what();
            }

        } else { // Pas bayésien : Tous les temps sont fixes

        }

        current_decomp_QTQ = decompositionCholesky(current_splineMatrices.matQTQ, 5, 1);
        current_decomp_matB = decomp_matB(current_splineMatrices, mModel->mLambdaSpline.mX);
        current_ln_h_YWI_1_2 = ln_h_YWI_1_2(current_decomp_QTQ, current_decomp_matB);
        current_ln_h_YWI_3 = ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, current_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);


        const double logMin = -20.;
        const double logMax = +20.;
        try {

            try {
                if (mCurveSettings.mVarianceType != CurveSettings::eModeFixed) {
                    // On stocke l'ancienne valeur :
                    const double  current_value = mModel->mS02Vg.mX;

                    const double try_value_log = Generator::gaussByBoxMuller(log10(mModel->mS02Vg.mX), mModel->mS02Vg.mSigmaMH);
                    const double try_value = pow(10., try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {

                        rate = h_S02_Vg_K(initListEvents, mModel->mEvents, current_value, try_value);//, sy);

                        rate *=  (try_value / current_value) ;
                    } else {
                        rate = -1.;
                    }

                    mModel->mS02Vg.tryUpdate(try_value, rate);
                }

            } catch (std::exception& e) {
                qWarning()<< "[MCMCLoopCurve::update_400] S02 Vg : exception caught: " << e.what() << '\n';

            }

            if (mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {

                current_ln_h_YWI_2 = ln_h_YWI_2(current_decomp_matB); // Has not been initialized yet

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

                            try_ln_h_YWI_3 = ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
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


            } else if (mCurveSettings.mVarianceType == CurveSettings::eModeGlobal)  {

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
                        try_ln_h_YWI_3 = mModel->mLambdaSpline.mX == 0 ? 0. : ln_h_YWI_3_update(try_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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



                // Not bayesian
            } else { // nothing to do : mCurveSettings.mVarianceType == CurveSettings::eFixed

            }

        } catch (std::exception& e) {
            qWarning()<< "[MCMCLoopCurve::update_400] VG : exception caught: " << e.what() << '\n';

        } catch(...) {
            qWarning() << "[MCMCLoopCurve::update_400] VG Event Caught Exception!\n";

        }


        bool ok = true;
        try {
            if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {
                if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth  ) {
                        const double current_value = mModel->mLambdaSpline.mX;
                         double logMax = +10.;
                        double logMin = -20.;

                        double try_value_log ;
                        double try_value;
                        // On stocke l'ancienne valeur :
                        current_h_lambda = h_lambda(current_splineMatrices, mModel->mEvents.size(), current_value) ;// current_h_lambda provient h_lambda_komlan() donc faire une maj

                        // -- Nouveau code


                        //  -- Ancien code
                        //try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);

                        try_value_log = Generator::gaussByDoubleExp(log10(current_value), mModel->mLambdaSpline.mSigmaMH, logMin, logMax);
                        try_value = pow(10., try_value_log);

                        mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                        const auto try_spline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, try_value, mModel->compute_Y, mModel->compute_Z);
                        ok = hasPositiveGPrimePlusConst(try_spline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation

                        // -- Fin ancien code


                        logMin = -20.;
                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            // Calcul du rapport :
                            mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                            try_h_lambda = h_lambda(current_splineMatrices, mModel->mEvents.size(), try_value) ;
                            try_decomp_matB = decomp_matB(current_splineMatrices, try_value);
                            try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_Z);
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

                        // G.1- Calcul spline
                        mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

                        // Il faut refaire le test car on ne sait pas si l'ancien lambda donne positif, avec les nouveaux theta.
                        if ( mModel->mLambdaSpline.mLastAccepts.last() == false) {
                            ok = hasPositiveGPrimePlusConst(mModel->mSpline.splineX, mCurveSettings.mThreshold); // si dy > mCurveSettings.mThreshold = pas d'acceptation
                        }
                        //qDebug()<<"[MCMCLoopCurve::update_400] Depth counter="<<counter <<" positif="<<ok<<" accept="<< ( mModel->mLambdaSpline.mLastAccepts.last() == true);

                        // Not Depth
                } else { // Not Depth
                    const double logMin = -20.;
                    const double logMax = +10.;

                    // On stocke l'ancienne valeur :
                    const double current_value = mModel->mLambdaSpline.mX;
                    current_h_lambda = h_lambda(current_splineMatrices, mModel->mEvents.size(), current_value) ;// current_h_lambda provient h_lambda_komlan() donc faire une maj

                    // On tire une nouvelle valeur :
                     const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mLambdaSpline.mSigmaMH);
                     const double try_value = pow(10., try_value_log);

                    //const double try_value_log = Generator::gaussByBoxMuller(log2(current_value), mModel->mLambdaSpline.mSigmaMH); // pour test komlan
                    //const double try_value = exp2(try_value_log);

                    if (try_value_log >= logMin && try_value_log <= logMax) {

                            // Calcul du rapport :
                            mModel->mLambdaSpline.mX = try_value; // utilisé dans currentSpline dans S02_Vg

                            try_h_lambda = h_lambda(current_splineMatrices, mModel->mEvents.size(), try_value) ;
                            try_decomp_matB = decomp_matB(current_splineMatrices, try_value);
                            try_ln_h_YWI_3 = try_value == 0 ? 0. : ln_h_YWI_3_update(current_splineMatrices, mModel->mEvents, current_vecH, try_decomp_matB, try_value, mModel->compute_Y, mModel->compute_Z);
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

                    // G.1- Calcul spline
                    mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

                    ok = true;

                }


            } else { // Nothing to do
                // Pas bayésien : on sauvegarde la valeur constante dans la trace
                // G.1- Calcul spline
                mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);

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
        qWarning() << "[MCMCLoopCurve::update] out of range" <<e.what();

    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve::update]  "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update] Caught Exception!\n";
        return false;
    }

    return false;
}

*/

/**
 * @brief MCMCLoopCurve::update_Komlan, valble uniquement en univariate
 * @return
 */

bool MCMCLoopCurve::update_Komlan()
{
    try {

        // --------------------------------------------------------------
        //  A - Update ti Dates (idem chronomodel)
        // --------------------------------------------------------------
        try {
            for (std::shared_ptr<Event>& event : mModel->mEvents) {
                for (auto&& date : event->mDates) {
                    date.updateDelta(event->mTheta.mX);
                    date.updateTi(event->mTheta.mX);
                    date.updateSigmaShrinkage(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);
                    date.updateWiggle();

                }
            }

        }  catch (...) {
            qWarning() <<"update Date ???";
        }
        // Variable du MH de la spline

        double current_value, current_h, current_h_YWI = 0.0, current_h_VG;


        double try_value, try_h, try_h_YWI , try_h_lambda, try_h_VG ;

        Matrix2D R, Q, QT, try_R, try_Q, try_QT, K, try_K, R_1QT, try_R_1QT;
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

        std::vector<double> vectY;
        vectY.resize(initListEvents.size());
        std::transform(initListEvents.begin(), initListEvents.end(), vectY.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

        std::vector<double> vectstd;
        vectstd.resize(initListEvents.size());
        std::transform(initListEvents.begin(), initListEvents.end(), vectstd.begin(), [](std::shared_ptr<Event> ev) {return sqrt(pow(ev->mSy, 2) + ev->mVg.mX);});

        orderEventsByThetaReduced(mModel->mEvents);
        spreadEventsThetaReduced0(mModel->mEvents);

        current_vecH = calculVecH(mModel->mEvents);

        current_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH) ;


        auto current_math = current_splineMatrices ;

        R = seedMatrix(current_splineMatrices.matR, 1); // dim n-2 * n-2
        const int np = (int)R.size();
        const int mm = 3*np ;

        Q = remove_bands_Matrix(current_splineMatrices.matQ, 1); // dim n * n-2
        QT = transpose0(Q);

        Matrix2D  matRInv ;
        if (np <= 3) {
            matRInv = inverseMatSym0(R, 0) ;
        } else {
            const std::pair<Matrix2D, std::vector<double>> &decomp = decompositionCholeskyKK(R, 3, 0);

            matRInv = inverseMatSym_originKK(decomp.first, decomp.second, mm, 0);
        }

        R_1QT = multiMatParMat0(matRInv, QT);

        K = multiMatParMat0(Q, R_1QT) ;

        //MathK_init = K ;

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
                            try_splineMatrices = prepareCalculSpline(mModel->mEvents, try_vecH);

                            try_R = seedMatrix(try_splineMatrices.matR, 1); // dim n-2 * n-2
                            try_Q = remove_bands_Matrix(try_splineMatrices.matQ, 1); // dim n * n-2

                            try_QT = transpose0(try_Q);

                            const int try_np = (int)try_R.size();
                            Matrix2D  try_matRInv ;
                            if (try_np <= 3) {

                                try_matRInv = inverseMatSym0(try_R, 0);

                            } else {

                                const std::pair<Matrix2D, MatrixDiag> &try_decomp = decompositionCholesky(try_R, 3, 0);

                                try_matRInv = inverseMatSym_origin(try_decomp, mm, 0);

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
                            current_splineMatrices = std::move(try_splineMatrices);
                            current_h_lambda = std::move(try_h_lambda);
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
        std::for_each(mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (std::shared_ptr<PhaseConstraint> pc) {pc->updateGamma();});

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

                        const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mS02Vg.mSigmaMH);

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

                    // Fin maj SO2 Vg


                    if (mCurveSettings.mUseVarianceIndividual) {

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

                                //const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), event->mVg.mSigmaMH);
                                //try_value = pow(10., try_value_log);
                                const double Min = 0.;
                                const double Max = 1E+10;
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
                                    current_splineMatrices = std::move(try_splineMatrices);
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

                        const double try_value_log = Generator::gaussByBoxMuller(log10(current_value), mModel->mEvents.at(0)->mVg.mSigmaMH);
                        try_value = pow(10., try_value_log);

                        // affectation temporaire pour evaluer la nouvelle proba
                        for (std::shared_ptr<Event>& ev : initListEvents) {
                            ev->mVg.mX = try_value;
                            ev->updateW();
                        }

                        long double rapport = 0.;
                        if (try_value_log >= logMin && try_value_log <= logMax) {

                            // Calcul du rapport : try_matrices utilise les temps reduits, elle est affectée par le changement de VG
                            try_splineMatrices = prepareCalculSpline(mModel->mEvents, current_vecH);

                            try_h_YWI = h_YWI_AY(try_splineMatrices, mModel->mEvents, mModel->mLambdaSpline.mX, current_vecH);

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
                                current_splineMatrices = std::move(try_splineMatrices);
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
                const int n = (int)K.size();
                double som = 0;
                double vm = 0.;

                // On stocke l'ancienne valeur :
                current_value = mModel->mLambdaSpline.mX;


                auto fK = multiMatByVectCol0(K, mModel->mSpline.splineX.vecG);

                // produit fK*ft et la trace de la matrice K

                for (int i = 0; i < n; ++i) {

                    som += fK[i] * mModel->mSpline.splineX.vecG[i];

                    vm += K[i][i];
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

        mModel->mSpline = samplingSpline_multi(mModel->mEvents, initListEvents, vectY, vectstd, R, R_1QT, Q, QT, K) ;

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
        qWarning() << "[MCMCLoopCurve::update_Komlan] out_of_range" <<e.what();
    } catch (const std::exception& e) {
        qWarning() << "[MCMCLoopCurve::update_Komlan] "<< e.what();

    } catch(...) {
        qWarning() << "[MCMCLoopCurve::update_Komlan] Caught Exception!\n";
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

                        const double try_value = Generator::gaussByBoxMuller(current_value, event->mTheta.mSigmaMH);

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
            mModel->mSpline = currentSpline(mModel->mEvents, current_vecH, current_splineMatrices, mModel->mLambdaSpline.mX, mModel->compute_Y, mModel->compute_Z);
        else {

            mModel->mSpline = currentSpline_WI(mModel->mEvents, mModel->compute_Y, mModel->compute_Z, mCurveSettings.mUseErrMesure);
        }

        // G.2 - test GPrime positive
        // si dy > mCurveSettings.mThreshold = pas d'acceptation
        if (mCurveSettings.mProcessType == CurveSettings::eProcess_Depth)
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
            if (date.mTi.mSamplerProposal == MHVariable::eMHAdaptGauss)
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
     *  D -  Memo S02 Vg
     * -------------------------------------------------------------- */
    if (mModel->mS02Vg.mSamplerProposal != MHVariable::eFixe) {
        double memoS02 = sqrt(mModel->mS02Vg.mX);
#ifdef DEBUG
        if (memoS02 == INFINITY)
            qDebug()<<"in memo memoS02 == INFINITY";
#endif
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

        //double minY_X = 0., minY_Y = 0., minY_Z = 0.;
        //double maxY_X = 0., maxY_Y = 0., maxY_Z = 0.;

        double minY_GP_X = 0., minY_GP_Y = 0., minY_GP_Z = 0.;
        double maxY_GP_X = 0., maxY_GP_Y = 0., maxY_GP_Z = 0.;

        //minY_X = meanG->gx.mapG.minY();
        //maxY_X = meanG->gx.mapG.maxY();

        minY_GP_X = meanG->gx.mapGP.minY();
        maxY_GP_X = meanG->gx.mapGP.maxY();

        if (mModel->compute_Y) {
            //minY_Y = meanG->gy.mapG.minY();
            //maxY_Y = meanG->gy.mapG.maxY();

            minY_GP_Y = meanG->gy.mapGP.minY();
            maxY_GP_Y = meanG->gy.mapGP.maxY();

            if (mModel->compute_Z) {
                //minY_Z = meanG->gz.mapG.minY();
                //maxY_Z = meanG->gz.mapG.maxY();

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
            //const double deg = 180. / M_PI ;

            for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
                t = (double)idxT * stepT + mModel->mSettings.mTmin ;
                // Le premier calcul avec splineX évalue i0, qui est retoiurné, à la bonne position pour les deux autres splines
                valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineX, gx_the, varGx, gpx, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineY, gy_the, varGy, gpy, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineZ, gz_the, varGz, gpz, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);

                /*
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
                */
                minY_GP_X = std::min(gpx, minY_GP_X);
                maxY_GP_X = std::max(gpx, maxY_GP_X);

                minY_GP_Y = std::min(gpy, minY_GP_Y);
                maxY_GP_Y = std::max(gpy, maxY_GP_Y);

                minY_GP_Z = std::min(gpz, minY_GP_Z);
                maxY_GP_Z = std::max(gpz, maxY_GP_Z);
            }

        }  else {
            for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
                t = (double)idxT * stepT + mModel->mSettings.mTmin ;
                valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineX, gx_the, varGx, gpx, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                //minY_X = std::min(gx_the - 1.96 * sqrt(varGx), minY_X);
                //maxY_X = std::max(gx_the + 1.96 * sqrt(varGx), maxY_X);

                minY_GP_X = std::min(gpx, minY_GP_X);
                maxY_GP_X = std::max(gpx, maxY_GP_X);

                if (mModel->compute_Y) {
                    valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineY, gy_the, varGy, gpy, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                    //minY_Y = std::min(gy_the - 1.96 * sqrt(varGy), minY_Y);
                    //maxY_Y = std::max(gy_the + 1.96 * sqrt(varGy), maxY_Y);

                    minY_GP_Y = std::min(gpy, minY_GP_Y);
                    maxY_GP_Y = std::max(gpy, maxY_GP_Y);

                    if (mModel->compute_Z) {
                        valeurs_G_VarG_GP_GS(t, mModel->mSpline.splineZ, gz_the, varGz, gpz, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);
                        //minY_Z = std::min(gz_the - 1.96 * sqrt(varGy), minY_Z);
                        //maxY_Z = std::max(gz_the + 1.96 * sqrt(varGz), maxY_Z);

                        minY_GP_Z = std::min(gpz, minY_GP_Z);
                        maxY_GP_Z = std::max(gpz, maxY_GP_Z);
                    }
                }


            }
        }


        if (mChainIndex == 0 ) {// do not change the Y range between several chain
            //minY_X = std::min(minY_X, meanG->gx.mapG.minY());
            //maxY_X = std::max(maxY_X, meanG->gx.mapG.maxY());
            //meanG->gx.mapG.setRangeY(minY_X, maxY_X);

            minY_GP_X = std::min(minY_GP_X, meanG->gx.mapGP.minY());
            maxY_GP_X = std::max(maxY_GP_X, meanG->gx.mapGP.maxY());
            meanG->gx.mapGP.setRangeY(minY_GP_X, maxY_GP_X);

        } else {
            //minY_X = std::min(minY_X, chainG->gx.mapG.minY());
            //maxY_X = std::max(maxY_X, chainG->gx.mapG.maxY());

            minY_GP_X = std::min(minY_GP_X, chainG->gx.mapGP.minY());
            maxY_GP_X = std::max(maxY_GP_X, chainG->gx.mapGP.maxY());
         }

        //chainG->gx.mapG.setRangeY(minY_X, maxY_X);
        chainG->gx.mapGP.setRangeY(minY_GP_X, maxY_GP_X);

        if (mModel->compute_Y) {
            if (mChainIndex == 0 ) {// do not change the Y range between several chain
                //minY_Y = std::min(minY_Y, meanG->gy.mapG.minY());
                //maxY_Y = std::max(maxY_Y, meanG->gy.mapG.maxY());
                //meanG->gy.mapG.setRangeY(minY_Y, maxY_Y);

                minY_GP_Y = std::min(minY_GP_Y, meanG->gy.mapGP.minY());
                maxY_GP_Y = std::max(maxY_GP_Y, meanG->gy.mapGP.maxY());
                meanG->gy.mapGP.setRangeY(minY_GP_Y, maxY_GP_Y);

            } else {
                //minY_Y = std::min(minY_Y, chainG->gy.mapG.minY());
                //maxY_Y = std::max(maxY_Y, chainG->gy.mapG.maxY());

                minY_GP_Y = std::min(minY_GP_Y, meanG->gy.mapGP.minY());
                maxY_GP_Y = std::max(maxY_GP_Y, meanG->gy.mapGP.maxY());
            }
            //chainG->gy.mapG.setRangeY(minY_Y, maxY_Y);
            chainG->gy.mapGP.setRangeY(minY_GP_Y, maxY_GP_Y);

            if (mModel->compute_Z) {
                if (mChainIndex == 0 ) {// do not change the Y range between several chain
                    //minY_Z = std::min(minY_Z, meanG->gz.mapG.minY());
                    //maxY_Z = std::max(maxY_Z, meanG->gz.mapG.maxY());
                    //meanG->gz.mapG.setRangeY(minY_Z, maxY_Z);

                    minY_GP_Z = std::min(minY_GP_Z, meanG->gz.mapGP.minY());
                    maxY_GP_Z = std::max(maxY_GP_Z, meanG->gz.mapGP.maxY());
                    meanG->gz.mapGP.setRangeY(minY_GP_Z, maxY_GP_Z);

                } else {
                    //minY_Z = std::min(minY_Z, chainG->gz.mapG.minY());
                    //maxY_Z = std::max(maxY_Z, chainG->gz.mapG.maxY());

                    minY_GP_Z = std::min(minY_GP_Z, meanG->gz.mapGP.minY());
                    maxY_GP_Z = std::max(maxY_GP_Z, meanG->gz.mapGP.maxY());
                }
                //chainG->gz.mapG.setRangeY(minY_Z, maxY_Z);
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

    } else if (!mModel->compute_Z) {
        mModel->memo_PosteriorG( mModel->mPosteriorMeanGByChain[mChainIndex].gx, mModel->mSpline.splineX, iterAccepted );
        mModel->memo_PosteriorG( mModel->mPosteriorMeanGByChain[mChainIndex].gy, mModel->mSpline.splineY, iterAccepted );
        if (mLoopChains.size() > 1) {
            mModel->memo_PosteriorG( mModel->mPosteriorMeanG.gx, mModel->mSpline.splineX, totalIterAccepted);
            mModel->memo_PosteriorG( mModel->mPosteriorMeanG.gy, mModel->mSpline.splineY, totalIterAccepted);
        }

    }
    else {

        mModel->memo_PosteriorG_3D( mModel->mPosteriorMeanGByChain[mChainIndex], mModel->mSpline, mCurveSettings.mProcessType, iterAccepted);

        if (mLoopChains.size() > 1)
            mModel->memo_PosteriorG_3D( mModel->mPosteriorMeanG, mModel->mSpline, mCurveSettings.mProcessType, totalIterAccepted);
    }


}


/* C'est le même algorithme que ModelCurve::buildCurveAndMap()
 */
// obsolete
void MCMCLoopCurve::memo_PosteriorG(PosteriorMeanGComposante& postGCompo, MCMCSplineComposante& splineComposante, const int realyAccepted)
{
    CurveMap& curveMap = postGCompo.mapG;
    const int nbPtsX = curveMap.column();
    const int nbPtsY = curveMap.row();

    const double ymin = curveMap.minY();
    const double ymax = curveMap.maxY();

    const double stepT = (mModel->mSettings.mTmax - mModel->mSettings.mTmin) / (nbPtsX - 1);
    const double stepY = (ymax - ymin) / (nbPtsY - 1);

    CurveMap& curveMapGP = postGCompo.mapGP;
    const double yminGP = curveMapGP.minY();
    const double ymaxGP = curveMapGP.maxY();
    const double stepYGP = (ymaxGP - yminGP) / (curveMapGP.row() - 1);

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
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précèdent
    for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
        double gp, gs;
        double gx, varGx;
        const double t = (double)idxT * stepT + mModel->mSettings.mTmin ;
        valeurs_G_VarG_GP_GS(t, splineComposante, gx, varGx, gp, gs, i0, mModel->mSettings.mTmin, mModel->mSettings.mTmax);

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

        const int idxYErrMin = std::clamp( int((gx - 3.*stdG - ymin) / stepY), 0, nbPtsY-1);
        const int idxYErrMax = std::clamp( int((gx + 3.*stdG - ymin) / stepY), 0, nbPtsY-1);

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY-1) {
#ifdef DEBUG
            curveMap(idxT, idxYErrMin) = curveMap.at(idxT, idxYErrMin) + 1.;
          /*  if ((curveMap.row()*idxT + idxYErrMin) < (curveMap.row()*curveMap.column()))
                curveMap(idxT, idxYErrMin) = curveMap.at(idxYErrMin, idxYErrMin) + 1; // correction à faire dans finalize() + 1./nbIter;
            else
                qDebug()<<"pb in MCMCLoopCurve::memo_PosteriorG";
*/
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


        // Memo mapGP

        const int idxYGP = std::clamp( int((gp - yminGP) / stepYGP), 0, nbPtsY-1);
        curveMapGP(idxT, idxYGP) = curveMapGP.at(idxT, idxYGP) + 1.;

        curveMapGP.max_value = std::max(curveMapGP.max_value, curveMapGP.at(idxT, idxYGP));

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

        double gx = 0, gpx = 0, gsx = 0, varGx = 0;
        double gy = 0, gpy = 0, gsy = 0, varGy = 0;
        double gz = 0, gpz = 0, gsz = 0, varGz = 0;

        const double t = (double)idxT * stepT + model.mSettings.mTmin ;
        valeurs_G_VarG_GP_GS(t, spline.splineX, gx, varGx, gpx, gsx, i0, model.mSettings.mTmin, model.mSettings.mTmax);
        valeurs_G_VarG_GP_GS(t, spline.splineY, gy, varGy, gpy, gsy, i0, model.mSettings.mTmin, model.mSettings.mTmax);

        if (model.compute_Z)
            valeurs_G_VarG_GP_GS(t, spline.splineZ, gz, varGz, gpz, gsz, i0, model.mSettings.mTmin, model.mSettings.mTmax);

        // Conversion IDF
        if (curveType == CurveSettings::eProcess_Vector ||  curveType == CurveSettings::eProcess_Spherical) {
            const double F = sqrt(pow(gx, 2.) + pow(gy, 2.) + pow(gz, 2.));
            const double Inc = asin(gz / F);
            const double Dec = atan2(gy, gx);

            const double ErrF = sqrt((varGx + varGy + varGz)/3.);

            const double ErrI = ErrF / F ;
            const double ErrD = ErrF / (F * cos(Inc)) ;

            gx = Inc * deg;
            gy = Dec * deg;
            gz = F;

            varGx = pow(ErrI * deg, 2.);
            varGy = pow(ErrD * deg, 2.);
            varGz = pow(ErrF, 2.);
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
        idxYErrMin = std::clamp( int((gx - k*stdGx - ymin_XInc) / stepY_XInc), 0, nbPtsY_XInc-1);
        idxYErrMax = std::clamp( int((gx + k*stdGx - ymin_XInc) / stepY_XInc), 0, nbPtsY_XInc-1);

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
        idxYErrMin = std::clamp( int((gy - k*stdGy - ymin_YDec) / stepY_YDec), 0, nbPtsY_YDec -1);
        idxYErrMax = std::clamp( int((gy + k*stdGy - ymin_YDec) / stepY_YDec), 0, nbPtsY_YDec -1);

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


        if (model.compute_Z) {

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
            idxYErrMin = std::clamp( int((gz - k*stdGz - ymin_ZF) / stepY_ZF), 0, nbPtsY_ZF-1);
            idxYErrMax = std::clamp( int((gz + k*stdGz - ymin_ZF) / stepY_ZF), 0, nbPtsY_ZF-1);

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

            if (mModel->compute_Z) {
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

            if (mModel->compute_Z) {
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

/**
 * Calcul de h_YWI_AY pour toutes les composantes de Y event (suivant la configuration univarié, spérique ou vectoriel)
 */
t_prob MCMCLoopCurve::h_YWI_AY(const SplineMatrices& matrices, const std::vector<std::shared_ptr<Event>> &events, const double lambdaSpline, const std::vector< double>& vecH, const bool hasY, const bool hasZ)
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


t_prob MCMCLoopCurve::h_YWI_AY_composanteX(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp_matB, const std::pair<Matrix2D, MatrixDiag> &decomp_QTQ, const double lambdaSpline)
{
    if (lambdaSpline == 0) {
        return 1.;
    }

    const SplineResults &spline = doSplineX(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector< double> &vecG = spline.vecG;
    const MatrixDiag &matD = decomp_matB.second;

   // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    const int n = (int)events.size();
    // Schoolbook algo
    /*
    t_prob h_exp = 0.;
    int i = 0;
    for (const auto& e : events) {
        h_exp  +=  e->mW * e->mYx * (e->mYx - vecG.at(i++));
    }
    */
    // C++ algo
    const t_prob h_exp = std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYx * (e->mYx - g); });

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

t_prob MCMCLoopCurve::h_YWI_AY_composanteY(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const std::pair<Matrix2D, MatrixDiag > &decomp_QTQ, const double lambdaSpline)
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

    const int n = (int)events.size();

    // Schoolbook algo
    /*
    t_prob h_exp = 0.;
    int i = 0;
    for (const auto& e : events) {
        h_exp  +=  e->mW * e->mYy * (e->mYy - vecG.at(i++));
    }
    */
    // C++ algo
    const t_prob h_exp = std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYy * (e->mYy - g); });

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

t_prob MCMCLoopCurve::h_YWI_AY_composanteZ(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime>& vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const std::pair<Matrix2D, MatrixDiag> &decomp_QTQ, const double lambdaSpline)
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

    const int n = (int)events.size();

    /* Naive code
    t_prob h_exp = 0.;
    int i = 0;
    for (const auto& e : events) {
        h_exp  +=  e->mW * e->mYz * (e->mYz - vecG.at(i++));
    }
    */
    // C++ algo
    const t_prob h_exp = std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYz * (e->mYz - g); });

    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    const MatrixDiag& matDq = decomp_QTQ.second;

    /* Naive code
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
t_prob MCMCLoopCurve::ln_h_YWI_3_update_ASYNC(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ)
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

t_prob MCMCLoopCurve::ln_h_YWI_3_update(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ)
{
    Q_ASSERT_X(lambdaSpline!=0, "[MCMCLoopCurve::ln_h_YWI_3_update]", "lambdaSpline=0");
    const t_prob try_ln_h_YWI_3_X = ln_h_YWI_3_X(matrices, events, vecH, decomp_matB, lambdaSpline);

    const t_prob try_ln_h_YWI_3_Y = hasY ? ln_h_YWI_3_Y(matrices, events, vecH, decomp_matB, lambdaSpline) : 0.;

    const t_prob try_ln_h_YWI_3_Z = hasZ ? ln_h_YWI_3_Z( matrices, events, vecH, decomp_matB, lambdaSpline) : 0.;

    return try_ln_h_YWI_3_X + try_ln_h_YWI_3_Y + try_ln_h_YWI_3_Z;

}

t_prob MCMCLoopCurve:: ln_h_YWI_3_X(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline)
{
    const SplineResults& spline = doSplineX(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector<double>& vecG = spline.vecG;

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    return -std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYx * (e->mYx - g); });

}

t_prob MCMCLoopCurve::ln_h_YWI_3_Y(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline)
{
    const SplineResults &spline = doSplineY(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector<double> &vecG = spline.vecG;
    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    return -std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYy * (e->mYy - g); });
}

t_prob MCMCLoopCurve::ln_h_YWI_3_Z(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp_matB, const double lambdaSpline)
{
    const SplineResults &spline = doSplineZ(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector<double> &vecG = spline.vecG;

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    return -std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYz * (e->mYz - g); });
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
    /* naive code
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
qDebug()<<" MCMCLoopCurve::h_lambda c="<<c;
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


/*t_prob MCMCLoopCurve::h_S02_Vg_K(const std::vector<std::shared_ptr<Event>>& events, const double S02_Vg, const double try_S02)
{

    const int alpha = 1;

    t_prob n_DefaultEvent = mPointEvent.size();
    const double beta = mModel->mSO2_beta; // test beta=1 ici
    const int a = 1;
    double prod_h_Vg = 0.;
    //double tmp_prod_h_Vg ;
    if (mCurveSettings.mUseVarianceIndividual) {     
     /
        // C++ optimization, Can be parallelized
        const t_prob prod_h_Vg_accumulate = std::accumulate(events.begin(), events.end(), 0., [S02_Vg, try_S02] (t_prob som, auto e) {
            return  (e->mPointType == Event::ePoint ? som + log((S02_Vg + e->mVg.mX)/(try_S02 + e->mVg.mX)) : som);
        });
        const t_prob ln_prod_h_Vg  = n_DefaultEvent * log(try_S02/S02_Vg) + (a + 1) * prod_h_Vg_accumulate ;



        prod_h_Vg =  exp(ln_prod_h_Vg);

    } else {
        prod_h_Vg = (exp((a + 1)*log((S02_Vg + events[0]->mVg.mX) / (try_S02 + events[0]->mVg.mX))) * (try_S02 / S02_Vg));
    }

    const double prior = exp((alpha + 1)*log(S02_Vg / try_S02)) * exp(-beta * ((S02_Vg - try_S02) / (try_S02 * S02_Vg)));

    return prior * prod_h_Vg;

}
*/

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

    if (mCurveSettings.mUseVarianceIndividual) {
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

        if (mCurveSettings.mUseVarianceIndividual) {
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
double MCMCLoopCurve::S02_Vg_Yx(std::vector<std::shared_ptr<Event>> &events, const SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline)
{
    // const MCMCSpline &splineWI = currentSpline(events, vecH, matricesWI, lambdaSpline, mModel->compute_Y, mModel->compute_Z); // peut-être utiliser doSplineX()
    // const std::vector< double> &vecG = splineWI.splineX.vecG;
    //  Mat_B = R + alpha * Qt * W-1 * Q
    //const Matrix2D matB = addMatEtMat(matricesWI.matR, multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    //std::pair<Matrix2D, std::vector< Matrix2D::value_type::value_type>> decomp = decompositionCholesky(matB, 5, 1);

    // ---
    const std::vector<double> &vec_theta_red = get_vector(get_ThetaReduced, events);

    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    Matrix2D matB; //matR;

    if (lambdaSpline != 0.0) {
        const Matrix2D &tmp = multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5);
        matB = addMatEtMat(matricesWI.matR, tmp, 5);

    } else {
        matB = matricesWI.matR;
    }

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(matB, 5, 1);


    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédent ne change pas

    const SplineResults &sx = doSplineX(matricesWI, events, vecH, decomp, lambdaSpline);
    const std::vector< double> &vecG = sx.vecG;
    //--



    double S02 = 0.0;
    // schoolbook
    /*
    for (unsigned long i = 0; i< vecG.size(); i++) {
        S02 += pow(_events[i]->mYx - vecG[i], 2.);
    }
    */
    // C++ optimization
    auto g = vecG.begin();
    for (auto& ev : events) {
        S02 += pow(ev->mYx - *g++, 2.0);
    }

    const std::vector<t_matrix>& matA = calculMatInfluence_origin(matricesWI, 1, decomp, lambdaSpline);
    // Calcul DLE
    const double traceA = std::accumulate(matA.begin(), matA.end(), 0.0);

    S02 /= (double)(vecG.size()) - traceA;
    return std::move(S02);

}


double MCMCLoopCurve::S02_Vg_Yy(std::vector<std::shared_ptr<Event> > &events, const SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline)
{
    const MCMCSpline &splineWI = currentSpline(events,  vecH, matricesWI, lambdaSpline, mModel->compute_Y, mModel->compute_Z);
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

    const std::vector<t_matrix> &matA = calculMatInfluence_origin(matricesWI, 1, decomp, lambdaSpline);

    const double traceA = std::accumulate(matA.begin(), matA.end(), 0.0L);

    S02 /= (double)(vecG.size()) - traceA;
    return std::move(S02);

}

double MCMCLoopCurve::S02_Vg_Yz(std::vector<std::shared_ptr<Event>> &events, const SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline)
{
    const MCMCSpline& splineWI = currentSpline(events, vecH, matricesWI, lambdaSpline, mModel->compute_Y, mModel->compute_Z);
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
    const Matrix2D& matB = addMatEtMat(matricesWI.matR, multiConstParMat(matricesWI.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    const std::pair<Matrix2D, MatrixDiag>& decomp = decompositionCholesky(matB, 5, 1);

    //const SplineResults &s = doSplineZ (matricesWI, events, vecH, decomp, lambdaSpline);

    const std::vector<t_matrix>& matA = calculMatInfluence_origin(matricesWI, 1, decomp, lambdaSpline);

    const double traceA = std::accumulate(matA.begin(), matA.end(), 0.0L);

    S02 /= (double)(vecG.size()) - traceA;
    return std::move(S02);

}
/**
 * @brief Les calculs sont faits avec les dates (theta event, ti dates, delta, sigma) exprimées en années.
 */
// voir U-cmt_MCMC ligne 105 calcul_h
double MCMCLoopCurve::h_theta_Event (const std::shared_ptr<Event> e)
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

t_prob MCMCLoopCurve::h_theta(const QList<std::shared_ptr<Event> > &events) const
{
    if (mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
        t_prob h = 1.;
        for (std::shared_ptr<Event> e : events) {
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
    unsigned nbEgal = 0;

    if (spreadSpan == 0.) {
        spreadSpan = 1.E-8; //std::numeric_limits<double>::epsilon() * 1.E12;//1.E6;// epsilon = 1E-16
    }

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

double MCMCLoopCurve::yearTime(double reduceTime)
{
    return mModel->yearTime(reduceTime) ;
}




#pragma mark Calcul Spline




bool MCMCLoopCurve::hasPositiveGPrimeByDerivate (const MCMCSplineComposante& splineComposante, const double k)
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
* @brief MCMCLoopCurve::hasPositiveGPrimePlusConst
* @param splineComposante
* @param dy in Y unit per time unit ex meter/Year
* @return
 */
bool MCMCLoopCurve::hasPositiveGPrimePlusConst(const MCMCSplineComposante& splineComposante, const double dy_threshold)
{
    const double dY = - dy_threshold * (mModel->mSettings.mTmax - mModel->mSettings.mTmin); // On doit inverser le signe et passer en temps réduit

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


// obsolete
/*
double MCMCLoopCurve::rate_h_lambda_K(const MCMCSpline &s, const double current_lambda, const double try_lambda, const Matrix2D &K)
{
    const int mu = 3;
    const int n = K.size();
    double som = 0;
    double vm = 0.;

    const double lambda = try_lambda - current_lambda ;

    // auto Klambda_dif = multiConstParMat0(K, lambda) ;

    auto fK = multiMatByVectCol0(K, s.splineX.vecG);

    // produit fK*ft

    for (int i = 0; i < n; ++i) {

        //  const int u = Signe_Number(fK[i]) * Signe_Number(s.splineX.vecG[i]) ;

        som += fK[i] * s.splineX.vecG[i];// u * pow(n, log_p(abs(fK[i]), n) + log_p(abs(s.splineX.vecG[i]), n)) ;

        vm += K[i][i];
    }

    const double c = (n - 2) / vm;

    const double p = -0.5 * lambda * som + 0.5*(n - 2)*log(try_lambda / current_lambda) + (mu + 1)*log((c + current_lambda) / (c + try_lambda));

    const double prod = exp(p) ;

    const double produit = prod *  (try_lambda / current_lambda) ;

    return produit;
}
*/

double MCMCLoopCurve::rate_h_lambda_K(const double current_lambda, const double try_lambda, const double schrinkageLambda, const int n)
{
     const int mu = 3;

     const double c = (n - 2) / schrinkageLambda;

     const double p = (0.5*(n - 2)*log(try_lambda / current_lambda)) + (mu + 1)*log((c + current_lambda) / (c + try_lambda));

     const double produit = exp(p) ;

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

    const double cnew = S02_lambda_WIK(K_new, nb_noeuds);

    const double prior1 = exp((mu + 1)*log(c / (c + lambdaSpline)));
    const double prior2 = exp((mu + 1)*log(cnew / (cnew + lambdaSpline)));

    const double prior = (prior2 / prior1) * (c / cnew) ;

    // prior "shrinkage"

    // prior "shrinkage"

   // const double prior = exp(mu*log(c_new / c) + (mu + 1)*log((c + lambdaSpline) / (c_new + lambdaSpline))) ;

    return prior;
}

MatrixDiag MCMCLoopCurve::createDiagWInv_Vg0(const std::vector<std::shared_ptr<Event> > &lEvents)
{
    MatrixDiag diagWInv (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), diagWInv.begin(), [](std::shared_ptr<Event> ev) {return pow(ev->mSy, 2.);});

    return diagWInv;
}


SplineMatrices MCMCLoopCurve::prepareCalculSpline_W_Vg0(const std::vector<std::shared_ptr<Event>> &sortedEvents, std::vector<double>& vecH)
{
    Matrix2D matR = calculMatR(vecH);
    Matrix2D matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    Matrix2D matQT = transpose(matQ, 3);

    // Diag Winv est égale à la diagonale des variances
    MatrixDiag diagWInv (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.begin(), [](std::shared_ptr<Event>ev) {return pow(ev->mSy, 2.);});

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


MCMCSpline MCMCLoopCurve::samplingSpline_multi(std::vector<std::shared_ptr<Event> > &lEvents, std::vector<std::shared_ptr<Event> > &lEventsinit, std::vector<double> vecYx, std::vector<double> vecYstd, const Matrix2D &RR, const Matrix2D &R_1QT, const Matrix2D &Q, const Matrix2D &QT, const Matrix2D &matK)
{
    MCMCSpline spline;

    const int n = (int)matK.size() ;

    Matrix2D W = initMatrix2D(n, n);
    Matrix2D W_1 = initMatrix2D(n, n);
    for (int i = 0; i <n; i++) {
        W[i][i] = 1. / pow(vecYstd[i], 2);

        W_1[i][i] = pow(vecYstd[i], 2);
    }


    const Matrix2D &QTW_1Q = multiMatParMat0(QT, multiMatParMat0(W_1, Q));

    const Matrix2D &lambdaQTW_1Q = multiConstParMat0(QTW_1Q, mModel->mLambdaSpline.mX);

    const Matrix2D &B = addMatEtMat0(RR, lambdaQTW_1Q);

    const int nb = (int)B.size();
    Matrix2D B_1;
    if (nb <=3) {
        B_1 = inverseMatSym0(B, 0);
    }else {
        const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(B, B.size(), 0);

        B_1 = inverseMatSym_origin(decomp, 4*B.size(), 0);
    }

    const Matrix2D &QB_1QT = multiMatParMat0(Q, multiMatParMat0(B_1, QT));

    const Matrix2D &W_1QB_1QT = multiMatParMat0(W_1, QB_1QT);

    const Matrix2D &lambdaW_1QB_1QT = multiConstParMat0(W_1QB_1QT, - mModel->mLambdaSpline.mX);

    const Matrix2D  &A = addIdentityToMat(lambdaW_1QB_1QT);

    const Matrix2D  &WlambdaK_1 = multiMatParMat0(A, W_1);


    // Calcul du vecteur moyen AY de la conditionnelle complète
    const std::vector<double>  &mu = multiMatByVectCol0(A, vecYx);

    // simulation de la fonction f
    const std::vector<double> &vecfx = multinormal_sampling(mu, WlambdaK_1);

    for (int i = 0; i < n; i++) {
        lEventsinit[i]-> mGx = vecfx[i];
    }

    // Calcul de la dérivée seconde de la fonction f

    // const std::vector<double> &QTfx = multiMatByVectCol0(QT, vecfx) ;

    std::vector<double> vecGamma =  multiMatByVectCol0(R_1QT, vecfx);
    vecGamma.push_back(0.);
    vecGamma.insert(vecGamma.begin(), 0.);

    // La sauvegarde de theta, f et f'
    MCMCSplineComposante splineX;
    splineX.vecThetaReduced = get_vector(get_ThetaReduced, lEvents);;
    splineX.vecG = vecfx;
    splineX.vecGamma = vecGamma;

    splineX.vecVarG = std::vector<double>(n, 0.0000000001) ;

    spline.splineX = std::move(splineX);

    return spline;
}

std::vector<double> MCMCLoopCurve::splines_prior(const Matrix2D &KK, std::vector<double> &g, std::vector<double> &g_new)
{

    const double lambda = mModel->mLambdaSpline.mX ;

    const auto &KKlambda = multiConstParMat0(KK, lambda);

    int n = (int)KK.size();

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

    const int n = (int)K.size();

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


std::vector<double> MCMCLoopCurve::multiMatByVectCol0_KK(const Matrix2D &KKK, const std::vector<double> &gg)
{
    const int nl1 = (int)KKK.size();
    // const int nc1 = matrix[0].size();
    const int nc2 = (int)gg.size();

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


double MCMCLoopCurve::h_exp_fX_theta (std::shared_ptr<Event> e, const MCMCSpline &s, unsigned idx)
{
    const double h = -0.5 * (e->mW * pow( e->mYx - s.splineX.vecG[idx] ,2.));
    return exp(h);
}

std::vector<double> MCMCLoopCurve::sampling_spline (std::vector<std::shared_ptr<Event> > &lEvents, SplineMatrices matrices)
{
    int n = (int)matrices.diagWInv.size();
    /*  std::vector<double> W; // matrice diagonale
    for (auto &w_1 : matrices.diagWInv ) {
        W.push_back(w_1);
    }*/

    std::vector< double> vecYx (lEvents.size());
    std::transform(lEvents.begin(), lEvents.end(), vecYx.begin(), [](std::shared_ptr<Event> ev) {return ev->mYx;});

    std::vector<double> X;
    for(int i = 0; i < n; i++){
        X.push_back(Generator::gaussByBoxMuller(vecYx[i], sqrt(matrices.diagWInv[i])));
    }

    return X;
}

double MCMCLoopCurve::h_S02_Vg_K_old(const std::vector<std::shared_ptr<Event>> events, double S02_Vg, double try_Vg)
{
    const int alp = 1;

    const double prior = exp((alp + 1)*log(S02_Vg / try_Vg)) * exp(-((S02_Vg - try_Vg) / (try_Vg*S02_Vg)));

    const int a = 1;
    double prod_h_Vg = 1.;

    if (mCurveSettings.mUseVarianceIndividual) {
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

std::vector<double> MCMCLoopCurve::multiMatByVectCol0(const Matrix2D& KKK, const std::vector<double>& gg)
{
    const size_t nl1 = KKK.size();
    // const int nc1 = matrix[0].size();
    const size_t nc2 = gg.size();

    // nc1 doit etre égal à nc2
    std::vector<double> result = initVector(nl1);
    // const double* itMat1;
    double sum;

    for (size_t i = 0; i < nl1; ++i) {
        // itMat1 = begin(KKK[i]);

        sum = 0;
        for (size_t k = 0; k < nc2; ++k) {
            sum += KKK[i][k] * gg[k] ; //(*(itMat1 + k)) * gg[k];
        }

        result[i] = sum;
    }

    return result;
}

std::vector<double> MCMCLoopCurve::multinormal_sampling (const std::vector<double>& mu, const Matrix2D& a)
{
    size_t N = mu.size();

    //  Compute the upper triangular Cholesky factor R of the variance-covariance  matrix.

    const auto& LT = choleskyLL0(a) ;

    //  Y = matrix of the 1D normal distribution with mean 0 and variance 1.

    std::vector<double> Y;
    for (size_t i = 0; i< N; i++)
        Y.push_back(Generator::gaussByBoxMuller(0., 1.));

    //  Compute X = MU + Lt * Y.

    // const auto &Lt = transpose0(L);

    const auto &LtY = multiMatByVectCol0(LT, Y);

    std::vector<double> X;
    for(size_t i = 0; i < N; i++)
        X.push_back(mu[i] + LtY[i]);

    return X;
}

std::pair<Matrix2D, std::vector<double>> MCMCLoopCurve::decompositionCholeskyKK(const Matrix2D &matrix, const int nbBandes, const int shift)
{
    errno = 0;
        //if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);

    const int dim = (int)matrix.size();
    Matrix2D matL = initMatrix2D(dim, dim);
    std::vector< double> matD (dim);

    if (dim - 2*shift == 1) { // cas des splines avec 3 points
        matD[1] = matrix[1][1];;
        matL[1][1] = 1.;

    } else {

        // const int bande = floor((nbBandes-1)/2);

        // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes constituées de zéro
        for (int i = shift; i < dim-shift; ++i) {
            matL[i][i] = 1.;
        }
        matD[shift] = matrix[shift][shift];

        try {
            for (int i = shift+1; i < dim-shift; ++i) {
                matL[i][shift] = matrix[i][shift] / matD[shift];
                /*   avec bande */
                for (int j = shift+1; j < i; ++j) {
                    if (abs(i - j) <= nbBandes) {
                        double sum = 0.;
                        for (int k = shift; k < j; ++k) {
                            if (abs(i - k) <= nbBandes) {
                                sum += matL[i][k] * matD.at(k) * matL[j][k];
                            }
                        }
                        matL[i][j] = (matrix[i][j] - sum) / matD.at(j);
                    }
                }

                double sum = 0.;
                for (int k = shift; k < i; ++k) {
                    if (abs(i - k) <= nbBandes) {
                        sum += std::pow(matL[i][k], 2.) * matD[k];
                    }
                }

                matD[i] = matrix[i][i] - sum; // doit être positif

            }

            for (int i = shift; i < dim-shift; ++i) {
                if (matD[i] < 0) {
                    qDebug() << "[Function::decompositionCholeskyKK] : matD <0 change to 0"<< matD[i];
                    matD[i] = 0;
                }
            }
            // matL : Par exemple pour n = 5 et shift =0:
            // 1 0 0 0 0
            // X 1 0 0 0
            // X X 1 0 0
            // X X X 1 0
            // X X X X 1

            // matL : Par exemple pour n = 5 et shift =1:
            // 0 0 0 0 0
            // 0 1 0 0 0
            // 0 X 1 0 0
            // 0 X X 1 0
            // 0 0 0 0 0

        } catch(...) {
            qDebug() << "Function::decompositionCholesky : Caught Exception!\n";
        }
    }

    return std::pair<Matrix2D, std::vector<double>>(matL, matD);
}

double MCMCLoopCurve::rapport_Theta(const std::function <double (std::shared_ptr<Event>)> &fun, const std::vector<std::shared_ptr<Event> > &lEvents, const Matrix2D &K, const Matrix2D &K_new, const double lambdaSpline)
{
    const size_t n = K.size();

    const auto &K1 = multiConstParMat0(K, -1.);

    const auto &KK = addMatEtMat0(K_new, K1) ;

    std::vector<double> vectfx;
    vectfx.resize(lEvents.size());
    //std::transform(lEvents.begin(), lEvents.end(), vectfx.begin(), [](Event* ev) {return ev->mGx;});
    std::transform(lEvents.begin(), lEvents.end(), vectfx.begin(), fun);

    const std::vector<double> &fKx = multiMatByVectCol0(KK, vectfx);

    double som = 0.;
    for (size_t i = 0; i < n; ++i) {
        som += fKx[i] * vectfx[i];
    }
    //double dx = -0.5 * lambdaSpline * som;
    return exp(-0.5 * lambdaSpline * som);

}


#pragma mark usefull math function

double rapport_detK_plus(const Matrix2D &Mat_old, const Matrix2D &Mat_new)
{
    const auto &current_decompK = decompositionCholesky(Mat_old, (int)Mat_old.size(), 0) ;
    std::vector<long double> current_detPlus = (current_decompK.second);
    std::sort(current_detPlus.begin(), current_detPlus.end(), [] (double i, double j) { return (i>j);});
    current_detPlus.resize(current_detPlus.size() - 2);

    const auto &try_decompK = decompositionCholesky(Mat_new, (int)Mat_new.size() , 0);
    std::vector<long double> try_detPlus (try_decompK.second);
    std::sort(try_detPlus.begin(), try_detPlus.end(), [] (double i, double j) { return (i>j);});

    try_detPlus.resize(try_detPlus.size() - 2);
    const size_t n = Mat_old.size() - 2;
    long double rapport_detPlus = 1.;

    for (size_t i = 0 ; i < n; i++) {
        rapport_detPlus *= try_detPlus[i] / current_detPlus[i]  ;
    }

    const long double d = sqrt(rapport_detPlus);

    return d;
}

Matrix2D inverseMatSym_originKK(const Matrix2D &matrixLE,  const std::vector<double> &matrixDE, const int nbBandes, const int shift)
{
    int dim = (int)matrixLE.size();
    Matrix2D matInv = initMatrix2D(dim, dim);
    int bande = floor((nbBandes-1)/2);

    matInv[dim-1-shift][dim-1-shift] = 1. / matrixDE[dim-1-shift];

    if (dim >= 4) {
        matInv[dim-2-shift][dim-1-shift] = -matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-1-shift][dim-1-shift];
        matInv[dim-2-shift][dim-2-shift] = (1. / matrixDE[dim-2-shift]) - matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-2-shift][dim-1-shift];
    }

    // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes
    // La boucle suivante n'est executée que si dim >=5
    for (int i = dim-3-shift; i>=shift; --i) {
        matInv[i][i+2] = -matrixLE[i+1][i] * matInv[i+1][i+2] - matrixLE[i+2][i] * matInv[i+2][i+2];
        matInv[i][i+1] = -matrixLE[i+1][i] * matInv[i+1][i+1] - matrixLE[i+2][i] * matInv[i+1][i+2];
        matInv[i][i] = (1. / matrixDE[i]) - matrixLE[i+1][i] * matInv[i][i+1] - matrixLE[i+2][i] * matInv[i][i+2];

        if (bande >= 3)  {
            for (int k=3; k<=bande; ++k) {
                if (i+k < (dim - shift))  {
                    matInv[i][i+k] = -matrixLE[i+1][i] * matInv[i+1][i+k] - matrixLE[i+2][i] * matInv[i+2][i+k];
                }
            }
        }
    }

    for (int i=shift; i<dim-shift; ++i)  {
        for (int j=i+1; j<=i+bande; ++j)  {
            if (j < (dim-shift))   {
                matInv[j][i] = matInv[i][j];
            }
        }
    }

    return matInv;
}

