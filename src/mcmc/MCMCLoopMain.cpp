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

#include "MCMCLoopMain.h"
#include "Bound.h"
#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "Date.h"
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

#define NOTEST //TEST

class Model;
class Project;

MCMCLoopMain::MCMCLoopMain(Model* model, Project* project):MCMCLoop(),
mModel(model)
{
    MCMCLoop::mProject = project;
    if (mModel)
        setMCMCSettings(mModel->mMCMCSettings);

}

MCMCLoopMain::~MCMCLoopMain()
{
    mModel = nullptr;
    mProject = nullptr;
}

QString MCMCLoopMain::calibrate()
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
                if (date->mCalibration->mCurve.isEmpty())
                    date->calibrate(mModel->mSettings, mProject);
                  //date->calibrate(mProject);
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

void MCMCLoopMain::initVariablesForChain()
{
    // today we have the same acceptBufferLen for every chain
    const int acceptBufferLen =  mChains[0].mIterPerBatch;
    int initReserve = 0;

    for (auto&& c: mChains)
       initReserve += ( 1 + (c.mMaxBatchs*c.mIterPerBatch) + c.mIterPerBurn + (c.mIterPerAquisition/c.mThinningInterval) );

    for (auto&& event : mModel->mEvents) {
        event->mTheta.reset();
        event->mTheta.reserve(initReserve);

        event->mTheta.mLastAccepts.reserve(acceptBufferLen);
        event->mTheta.mLastAcceptsLength = acceptBufferLen;

        // event->mTheta.mAllAccepts.clear(); //don't clean, avalable for cumulate chain

        for (auto&& date : event->mDates) {
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
        //phase->mTau.reset();
        phase->mDuration.reset();

        phase->mAlpha.mRawTrace->reserve(initReserve);
        phase->mBeta.mRawTrace->reserve(initReserve);
        //phase->mTau.mRawTrace->reserve(initReserve);
        phase->mDuration.mRawTrace->reserve(initReserve);
   }
}

QString MCMCLoopMain::initialize()
{
    QList<Event*>& events (mModel->mEvents);
    QList<Phase*>& phases (mModel->mPhases);
    QList<PhaseConstraint*>& phasesConstraints (mModel->mPhaseConstraints);

    const double tminPeriod = mModel->mSettings.mTmin;
    const double tmaxPeriod = mModel->mSettings.mTmax;

    if (isInterruptionRequested())
        return ABORTED_BY_USER;

    // ---------------------- Reset Events ---------------------------
    for (auto&& ev : events)
        ev->mInitialized = false;

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
        ph->initTau(tminPeriod, tmaxPeriod);

        if (isInterruptionRequested())
            return ABORTED_BY_USER;
        ++i;
        emit stepProgressed(i);
    }

    /* -------------- Init Bounds --------------
    * => Init borne :
    *  - si valeur fixe, facile!
    * ---------------------------------------------------------------- */

    for (int i = 0; i < events.size(); ++i) {
        if (events.at(i)->type() == Event::eBound) {
            Bound* bound = dynamic_cast<Bound*>(events[i]);

            if (bound) {
                bound->mTheta.mX = bound->mFixed;
                bound->mTheta.memo();
                bound->mTheta.mLastAccepts.clear();
                bound->mTheta.tryUpdate(bound->mTheta.mX, 2.);

                bound->mInitialized = true;
            }
            bound = nullptr;
        }
    }

    // ----------------------------------------------------------------
    //  Init theta event, ti, ...
    // ----------------------------------------------------------------

    QVector<Event*> unsortedEvents = ModelUtilities::unsortEvents (events);

    emit stepChanged(tr("Initializing Events..."), 0, unsortedEvents.size());

    // Check Strati constraint
    for (auto&& e : unsortedEvents) {
        mModel->initNodeEvents();
        QString circularEventName = "";
        QList< Event*> startEvents = QList<Event*>();

        const bool ok (e->getThetaMaxPossible (e, circularEventName, startEvents));
         qDebug() << " MCMCLoopMain::initialize check constraint" << e->mName << ok;
        if (!ok) {
            mAbortedReason = QString(tr("Warning : Find Circular Constraint Path %1  %2 ")).arg (e->mName, circularEventName);
            return mAbortedReason;
        }
    }

    for (int i = 0; i < unsortedEvents.size(); ++i) {
        if (unsortedEvents.at(i)->mType == Event::eDefault) {

            mModel->initNodeEvents();
            QString circularEventName = "";

            const double min = unsortedEvents.at(i)->getThetaMinRecursive (tminPeriod);
            if (!circularEventName.isEmpty()) {
                mAbortedReason = QString(tr("Warning : Find Circular constraint with %1  bad path  %2 ")).arg(unsortedEvents.at(i)->mName, circularEventName);
                return mAbortedReason;
            }

            //qDebug() << "in initialize(): Event initialised min : " << unsortedEvents[i]->mName << " : "<<" min"<<min<<tmin;
            mModel->initNodeEvents();
            const double max = unsortedEvents.at(i)->getThetaMaxRecursive(tmaxPeriod);
#ifdef DEBUG
            if (min >= max) {
                qDebug() << tr("-----Error Init for event : %1 : min = %2 : max = %3-------").arg(unsortedEvents.at(i)->mName, QString::number(min), QString::number(max));
            }
#endif
            if (min >= max) {
                mAbortedReason = QString(tr("Error Init for event : %1 : min = %2 : max = %3-------").arg(unsortedEvents.at(i)->mName, QString::number(min, 'f', 30), QString::number(max, 'f', 30)));
                return mAbortedReason;
            }

            /* In ChronoModel 2.0, we initialize the theta uniformly between tmin and tmax possible.
             * Now, we use the cumulative date density distribution function.
             */

                /* Given the stratigraphic constraints and the possibility of having dates outside the study period.
                 * The maximum of the distribution curve can be different from the number of dates
                 * and the minimum can be different from 0.
                 */



            sampleInCumulatedRepartition(unsortedEvents.at(i), mModel->mSettings, min, max);

            unsortedEvents.at(i)->mInitialized = true;

            qDebug() << "---------------->      in initialize(): Event initialized : " << unsortedEvents[i]->mName << " : " << unsortedEvents[i]->mTheta.mX <<" between"<< min << max;

            double s02_sum = 0.;
            for (int j = 0; j < unsortedEvents.at(i)->mDates.size(); ++j) {
                Date& date = unsortedEvents.at(i)->mDates[j];

                // 1 - Init ti
                double sigma;
                // modif du 2021-06-16 pHd
                FunctionStat data = analyseFunction(vector_to_map(date.mCalibration->mCurve, date.mCalibration->mTmin, date.mCalibration->mTmax, date.mCalibration->mStep));
                sigma = double (data.std);
                //
                if (!date.mCalibration->mRepartition.isEmpty()) {
                    const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mCalibration->mRepartition);
                    date.mTi.mX = date.mCalibration->mTmin + idx * date.mCalibration->mStep;
                  //  qDebug()<<"MCMCLoopMain::Init"<<date.mName <<" mTheta.mx="<<QString::number(date.mTheta.mX, 'g', 15)<<date.mCalibration->mTmin<<date.mCalibration->mStep;
                  //  qDebug()<<"MCMCLoopMain::Init"<<date.mName <<" sigma="<<sigma;

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


                // intermediary calculus for the harmonic average
                s02_sum += 1. / (sigma * sigma);
                
            }

            // 4 - Init S02 of each Event
            unsortedEvents.at(i)->mS02 = (unsortedEvents.at(i)->mDates.size() / s02_sum); //unsortedEvents.at(i)->mDates.size() / s02_sum;// /100;
            qDebug()<<"MCMCLoopMain::Init"<<unsortedEvents.at(i)->mName <<" mS02="<<unsortedEvents.at(i)->mS02;
            
            // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
            unsortedEvents.at(i)->mTheta.mSigmaMH = sqrt(unsortedEvents.at(i)->mS02);
            unsortedEvents.at(i)->mAShrinkage = 1.;

            // 6- Clear mLastAccepts array
            unsortedEvents.at(i)->mTheta.mLastAccepts.clear();
            //unsortedEvents.at(i)->mTheta.mAllAccepts->clear(); //don't clean, avalable for cumulate chain
            unsortedEvents.at(i)->mTheta.tryUpdate(unsortedEvents.at(i)->mTheta.mX, 2.);

            // 7 - Memo
            unsortedEvents.at(i)->mVG.mAllAccepts->clear();
            unsortedEvents.at(i)->mVG.mLastAccepts.clear();
        }

        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepProgressed(i);
    }

    // ----------------------------------------------------------------
    //  Init sigma i and its sigma MH
    // ----------------------------------------------------------------
    QString log;
    emit stepChanged(tr("Initializing Variances..."), 0, events.size());

    for (int i = 0; i < events.size(); ++i) {
        for (auto&& date : events.at(i)->mDates) {
            // date.mSigma.mX = sqrt(shrinkageUniform(events[i]->mS02)); // modif the 2015/05/19 with PhL
            date.mSigmaTi.mX = std::abs(date.mTi.mX - (events.at(i)->mTheta.mX - date.mDelta)) ;

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
        ++i;

        emit stepProgressed(i);
    }

    return QString();
}

bool MCMCLoopMain::update()
{
    const double tminPeriod = mModel->mSettings.mTmin;
    const double tmaxPeriod = mModel->mSettings.mTmax;

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

        //--------------------- Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
        for (auto&& phInEv : event->mPhases)
            phInEv->updateAll(tminPeriod, tmaxPeriod);
    }



    // --------------------------------------------------------------
    //  C - Update Phases constraints
    // --------------------------------------------------------------
    std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (PhaseConstraint* pc) {pc->updateGamma();});


   return true;

}



bool MCMCLoopMain::adapt(const int batchIndex) //original code
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

    }


    return noAdapt;
}


void MCMCLoopMain::memo()
{
    for (const auto& event : mModel->mEvents) {
        //--------------------- Memo Events -----------------------------------------
        event->mTheta.memo();
        event->mTheta.saveCurrentAcceptRate();

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

void MCMCLoopMain::finalize()
{
#ifdef DEBUG
    qDebug()<<QString("MCMCLoopMain::finalize");
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
    mModel->generateCorrelations(mChains);

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
    qDebug()<<QString("MCMCLoopMain::finalize() finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug()<<QString("Total time elapsed %1").arg(DHMS(startTime.elapsed()));
#endif
}
