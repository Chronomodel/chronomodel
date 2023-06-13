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

#include "MCMCLoopChrono.h"

#include "Project.h"
#include "Bound.h"
#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "Date.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "CalibrationCurve.h"

#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QTime>

#define NOTEST //TEST

class Model;
class Project;

MCMCLoopChrono::MCMCLoopChrono(Model* model, Project* project):
    MCMCLoop(project)
{
    mModel = model;
    if (mModel)
        setMCMCSettings(mModel->mMCMCSettings);

    mCurveSettings.mTimeType = CurveSettings::eModeBayesian;
}

MCMCLoopChrono::~MCMCLoopChrono()
{
    mModel = nullptr;
}

QString MCMCLoopChrono::calibrate()
{
    if (mModel) {
        QList<Event*>& events = mModel->mEvents;
        events.reserve(mModel->mEvents.size());
        //----------------- Calibrate measurements --------------------------------------

        QList<Date*> dates;
        // find number of dates, to optimize memory space
        int nbDates = 0;
        for (auto&&e : events)
            nbDates += e->mDates.size();

        dates.reserve(nbDates);
        for (Event* ev : events) {
            unsigned long num_dates = ev->mDates.size();
            for (unsigned long j = 0; j<num_dates; ++j) {
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

                if (date->mCalibration->mVector.size() < 5) {
                    const double new_step = date->mCalibration->mStep/5.;
                    date->mCalibration->mVector.clear();
                    date->mCalibration->mMap.clear();
                    date->mCalibration->mRepartition.clear();
                    date->mCalibration = nullptr;

                    const QString mes = tr("Definition of the calibration curve insufficient for the Event %1 \r Decrease the study period step to %2").arg(date->mName, QString::number(new_step));
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


QString MCMCLoopChrono::initialize()
{
    return initialize_time(mModel);
}

QString MCMCLoopChrono::initialize0()
{
    QList<Phase*>& phases (mModel->mPhases);
    QList<PhaseConstraint*>& phasesConstraints (mModel->mPhaseConstraints);

    const double tminPeriod = mModel->mSettings.mTmin;
    const double tmaxPeriod = mModel->mSettings.mTmax;

    if (isInterruptionRequested())
        return ABORTED_BY_USER;

    // ---------------------- Reset Events ---------------------------
    for (auto&& ev : mModel->mEvents) {
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

    // ----------------------- Init tau -----------------------------------------
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

    /* -------------- Init Bounds --------------
    * => Init borne :
    *  - si valeur fixe, facile!
    * ---------------------------------------------------------------- */
    try {
        for (Event* ev : mModel->mEvents) {

            if (ev->type() == Event::eBound) {
                Bound* bound = dynamic_cast<Bound*>(ev);

                if (bound) {
                    bound->mTheta.mX = bound->mFixed;
                    bound->mThetaReduced = mModel->reduceTime(bound->mTheta.mX);
                    bound->mTheta.memo();
                    bound->mTheta.mLastAccepts.clear();
                    //bound->mTheta.tryUpdate(bound->mTheta.mX, 2.);
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
    // ----------------------------------------------------------------
    //  Init theta event, ti, ...
    // ----------------------------------------------------------------

    QVector<Event*> unsortedEvents = ModelUtilities::unsortEvents (mModel->mEvents);

    emit stepChanged(tr("Initializing Events..."), 0, unsortedEvents.size());

    // Check Strati constraint
    for (auto&& e : unsortedEvents) {
        mModel->initNodeEvents();
        QString circularEventName = "";
        QList< Event*> startEvents = QList<Event*>();

        const bool ok (e->getThetaMaxPossible (e, circularEventName, startEvents));
         qDebug() << " MCMCLoopChrono::initialize check constraint" << e->mName << ok;
        if (!ok) {
            mAbortedReason = QString(tr("Warning : Find Circular Constraint Path %1  %2 ")).arg (e->mName, circularEventName);
            return mAbortedReason;
        }
    }

    i = 0;

    for (Event* uEvent : unsortedEvents) {
        if (uEvent->mType == Event::eDefault) {

            mModel->initNodeEvents();
            QString circularEventName = "";

            const double min = uEvent->getThetaMinRecursive (tminPeriod);
            if (!circularEventName.isEmpty()) {
                mAbortedReason = QString(tr("Warning : Find Circular constraint with %1  bad path  %2 ")).arg(uEvent->mName, circularEventName);
                return mAbortedReason;
            }

            //qDebug() << "in initialize(): Event initialised min : " << unsortedEvents[i]->mName << " : "<<" min"<<min<<tmin;
            mModel->initNodeEvents();
            const double max = uEvent->getThetaMaxRecursive(tmaxPeriod);
#ifdef DEBUG
            if (min >= max) {
                qDebug() << tr("-----Error Init for event : %1 : min = %2 : max = %3-------").arg(uEvent->mName, QString::number(min), QString::number(max));
            }
#endif
            if (min >= max) {
                mAbortedReason = QString(tr("Error Init for event : %1 : min = %2 : max = %3-------").arg(uEvent->mName, QString::number(min, 'f', 30), QString::number(max, 'f', 30)));
                return mAbortedReason;
            }

            /* In ChronoModel 2.0, we initialize the theta uniformly between tmin and tmax possible.
             * Now, we use the cumulative date density distribution function.
             */

                /* Given the stratigraphic constraints and the possibility of having dates outside the study period.
                 * The maximum of the distribution curve can be different from the number of dates
                 * and the minimum can be different from 0.
                 */



            sampleInCumulatedRepartition(uEvent, mModel->mSettings, min, max);

            uEvent->mInitialized = true;

            qDebug() << "---------------->      in initialize(): Event initialized : " << uEvent->mName << " : " << uEvent->mTheta.mX <<" between"<< min << max;

            double s02_sum = 0.;
            //for (int j = 0; j < uEvent->mDates.size(); ++j) {
            //    Date& date = uEvent->mDates[j];
            for (Date& date : uEvent->mDates ) {

                // 1 - Init ti
                double sigma;
                // modif du 2021-06-16 pHd
                //const FunctionStat &data = analyseFunction(vector_to_map(date.mCalibration->mCurve, date.mCalibration->mTmin, date.mCalibration->mTmax, date.mCalibration->mStep));
                const FunctionStat &data = analyseFunction(date.mCalibration->mMap);
                sigma = double (data.std);
                //
                if (!date.mCalibration->mRepartition.isEmpty()) {
                    const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mCalibration->mRepartition);
                    date.mTi.mX = date.mCalibration->mTmin + idx * date.mCalibration->mStep;


                } else { // in the case of mRepartion curve is null, we must init ti outside the study period
                       // For instance we use a gaussian random sampling
                    sigma = tmaxPeriod - tminPeriod;
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


                // intermediary calculus for the harmonic average
                s02_sum += 1. / (sigma * sigma);
                
            }

            // 4 - Init S02 of each Event
            uEvent->mS02.mX = (uEvent->mDates.size() / s02_sum); //unsortedEvents.at(i)->mDates.size() / s02_sum;// /100;
            uEvent->mS02.mLastAccepts.clear();
            uEvent->mS02.tryUpdate(uEvent->mS02.mX, 2.);

            qDebug()<<"MCMCLoopChrono::Init"<<uEvent->mName <<" mS02="<<uEvent->mS02.mX;
            
            // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
            uEvent->mTheta.mSigmaMH = sqrt(uEvent->mS02.mX);
            uEvent->mAShrinkage = 1.;

            // 6- Clear mLastAccepts array
            uEvent->mTheta.mLastAccepts.clear();
            //unsortedEvents.at(i)->mTheta.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
            uEvent->mTheta.tryUpdate(uEvent->mTheta.mX, 2.);


        }

        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepProgressed(++i);
    }

    // ----------------------------------------------------------------
    //  Init sigma i and its sigma MH
    // ----------------------------------------------------------------
    QString log;
    emit stepChanged(tr("Initializing Variances..."), 0, mModel->mEvents.size());

    for (Event* ev : mModel->mEvents) {

        for (auto&& date : ev->mDates) {
            date.mSigmaTi.mX = std::abs(date.mTi.mX - (ev->mTheta.mX - date.mDelta)) ;

            if (date.mSigmaTi.mX<=1E-6) {
               date.mSigmaTi.mX = 1E-6; // Add control the 2015/06/15 with PhL
               log += line(date.mName + textBold("Sigma indiv. <=1E-6 set to 1E-6"));
            }
            date.mSigmaTi.mSigmaMH = 1.27;  //1.; // modif pHd 2021-01-20 : Empiric formula Here mTheta.mSigmaMH is equal to the standard deviation on the date density

            date.mSigmaTi.mLastAccepts.clear();
            date.mSigmaTi.mLastAccepts.push_back(1.);

           // date.mSigma.memo();
           // date.mSigma.saveCurrentAcceptRate();

        }
        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepProgressed(i);
    }
    // --------------------------- Init phases ----------------------
    emit stepChanged(tr("Initializing Phases..."), 0, phases.size());

    i = 0;
    for (auto&& phase : phases ) {
        //phase->updateAll(tminPeriod, tmaxPeriod);
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

        emit stepProgressed(++i);
    }

    return QString();
}

bool MCMCLoopChrono::update()
{

    /* --------------------------------------------------------------
     *  A - Update ti Dates
     *  B - Update Theta Events
     *  C.1 - Update Alpha, Beta & Duration Phases
     *  C.2 - Update Tau Phase
     *  C.3 - Update Gamma Phases
     * ---------------------------------------------------------------------- */

    // --------------------------------------------------------------
    //  A - Update ti Dates
    // --------------------------------------------------------------

    for (auto& event : mModel->mEvents) {
        for (auto&& date : event->mDates )   {
            date.updateDate(event);
        }
    }

 /* plus lent
  *   std::vector<std::thread> tab_th_updateDate;
    for (Event*& event : mModel->mEvents) {
        for (auto&& date : event->mDates) {
            tab_th_updateDate.push_back( std::thread ([&event, &date] () {date.updateDate(event);} )  );
        }
    }
    for (auto& th : tab_th_updateDate) {
        th.join();
    }
*/

    // --------------------------------------------------------------
    //  B - Update theta Events
    // --------------------------------------------------------------
    for (auto&& event : mModel->mEvents) {

        event->updateTheta(tminPeriod, tmaxPeriod);
        if (event->mS02.mSamplerProposal != MHVariable::eFixe)
            event->updateS02();

        //--------------------- Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
        /* --------------------------------------------------------------
         * C.1 - Update Alpha, Beta & Duration Phases
         * -------------------------------------------------------------- */
        //  Update Phases -set mAlpha and mBeta ; they coud be used by the Event in the other Phase ----------------------------------------
        std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (Phase* p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});

    }

    //  Update Phases Tau; they coud be used by the Event in the other Phase ----------------------------------------
    /* --------------------------------------------------------------
     *  C.2 - Update Tau Phases
     * -------------------------------------------------------------- */
    std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (Phase* p) {p->update_Tau (tminPeriod, tmaxPeriod);});


    /* --------------------------------------------------------------
     *  C.3 - Update Phases constraints
     * -------------------------------------------------------------- */
    std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (PhaseConstraint* pc) {pc->updateGamma();});


   return true;

}



bool MCMCLoopChrono::adapt(const int batchIndex) //original code
{
/*  const double taux_min = 41.;          // taux_min minimal rate of acceptation=42
    const double taux_max = 47.;           // taux_max maximal rate of acceptation=46
*/

    const double taux_min = 0.42;           // taux_min minimal rate of acceptation=42
    const double taux_max = 0.46;           // taux_max maximal rate of acceptation=46

    bool noAdapt = true;

    //--------------------- Adapt -----------------------------------------

    const double delta = (batchIndex < 10000) ? 0.01 : (1. / sqrt(batchIndex));

    for (const auto& event : mModel->mEvents) {
       for (auto& date : event->mDates) {

            //--------------------- Adapt Sigma MH de t_i -----------------------------------------
            if (date.mTi.mSamplerProposal == MHVariable::eMHSymGaussAdapt)
                noAdapt = date.mTi.adapt(taux_min, taux_max, delta) && noAdapt;

            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            noAdapt = date.mSigmaTi.adapt(taux_min, taux_max, delta) && noAdapt;

        }

        //--------------------- Adapt Sigma MH de Theta Event -----------------------------------------
       if ((event->mType != Event::eBound) && ( event->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss) )
           noAdapt = event->mTheta.adapt(taux_min, taux_max, delta) && noAdapt;

       if ( event->mS02.mSamplerProposal == MHVariable::eMHAdaptGauss)
            noAdapt = event->mS02.adapt(taux_min, taux_max, delta) && noAdapt;

    }


    return noAdapt;
}


void MCMCLoopChrono::memo()
{
    for (const auto& event : mModel->mEvents) {
        //--------------------- Memo Events -----------------------------------------
        event->mTheta.memo();
        event->mTheta.saveCurrentAcceptRate();

        if (event->mS02.mSamplerProposal != MHVariable::eFixe) {
            event->mS02.memo();
            event->mS02.saveCurrentAcceptRate();
        }


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
    for (const auto& ph : mModel->mPhases)
            ph->memoAll();


}


void MCMCLoopChrono::finalize()
{
#ifdef DEBUG
    qDebug()<<QString("MCMCLoopChrono::finalize");
    QElapsedTimer startTime;
    startTime.start();
#endif

    // This is not a copy of all data!
    // Chains only contain description of what happened in the chain (numIter, numBatch adapt, ...)
    // Real data are inside mModel members (mEvents, mPhases, ...)
    mModel->mChains = mChains;

    // This is called here because it is calculated only once and will never change afterwards
    // This is very slow : it is for this reason that the results display may be long to appear at the end of MCMC calculation.
    /** @todo Find a way to make it faster !
     */

    emit setMessage(tr("Computing posterior distributions and numerical results - Correlations"));
    mModel->generateCorrelations(mChains);

    emit setMessage(tr("Computing posterior distributions and numerical results - Densities"));
    mModel->initDensities();

    // This should not be done here because it uses resultsView parameters
    // ResultView will trigger it again when loading the model
    //mModel->generatePosteriorDensities(mChains, 1024, 1);

    // Generate numerical results of :
    // - MHVariables (global acceptation)
    // - MetropolisVariable : analysis of Posterior densities and quartiles from traces.
    // This also should be done in results view...
    //mModel->generateNumericalResults(mChains);

#ifdef DEBUG
    QTime endTime = QTime::currentTime();

    qDebug()<<"Model computed";
    qDebug()<<QString("MCMCLoopChrono::finalize() finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug()<<QString("Total time elapsed %1").arg(DHMS(startTime.elapsed()));
#endif
}
