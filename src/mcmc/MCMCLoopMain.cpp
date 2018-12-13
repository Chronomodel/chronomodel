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
#include "EventKnown.h"
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
        int nbDates (0);
        for (auto &&e : events)
            nbDates += e->mDates.size();

        dates.reserve(nbDates);
        for (auto &&ev : events) {
            int num_dates = ev->mDates.size();
            for (int j=0; j<num_dates; ++j) {
                Date* date = &ev->mDates[j];
                dates.push_back(date);
            }
        }


        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepChanged(tr("Calibrating..."), 0, dates.size());

        int i (0);
        for (auto &&date : dates) {
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

void MCMCLoopMain::initVariablesForChain()
{
    // today we have the same acceptBufferLen for every chain
    const int acceptBufferLen =  mChains[0].mNumBatchIter;
    int initReserve (0);

    for (const ChainSpecs c: mChains)
       initReserve += ( 1 + (c.mMaxBatchs*c.mNumBatchIter) + c.mNumBurnIter + (c.mNumRunIter/c.mThinningInterval) );

    for (Event* event : mModel->mEvents) {
        event->mTheta.reset();
        event->mTheta.reserve(initReserve);

        event->mTheta.mLastAccepts.reserve(acceptBufferLen);
        event->mTheta.mLastAcceptsLength = acceptBufferLen;

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

    for (Phase* phase : mModel->mPhases) {
        phase->mAlpha.reset();
        phase->mBeta.reset();
        phase->mDuration.reset();

        phase->mAlpha.mRawTrace->reserve(initReserve);
        phase->mBeta.mRawTrace->reserve(initReserve);
        phase->mDuration.mRawTrace->reserve(initReserve);
   }
}

QString MCMCLoopMain::initMCMC()
{
    QList<Event*>& events (mModel->mEvents);
    QList<Phase*>& phases (mModel->mPhases);
    QList<PhaseConstraint*>& phasesConstraints (mModel->mPhaseConstraints);

    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;

    if (isInterruptionRequested())
        return ABORTED_BY_USER;

    // ---------------------- Reset Events ---------------------------
    for (Event* ev : events)
        ev->mInitialized = false;

    // -------------------------- Init gamma ------------------------------
    emit stepChanged(tr("Initializing Phase Gaps..."), 0, phasesConstraints.size());
    int i (0);
    for (auto && phC : phasesConstraints) {
        phC->initGamma();
        if (isInterruptionRequested())
            return ABORTED_BY_USER;
        ++i;
        emit stepProgressed(i);
    }

    // ----------------------- Init tau -----------------------------------------
    emit stepChanged(tr("Initializing Phase Durations..."), 0, phases.size());
    i = 0;
    for (auto && ph : phases) {
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

    for (int i=0; i<eventsByLevel.size(); ++i) {
        if (eventsByLevel.at(i)->type() == Event::eKnown) {
            EventKnown* bound = dynamic_cast<EventKnown*>(eventsByLevel[i]);

            if (bound) {
                if (curLevel != bound->mLevel) {
                    curLevel = bound->mLevel;
                    curLevelMaxValue = mModel->mSettings.mTmin;
                }

                bound->mTheta.mX = bound->mFixed;

                curLevelMaxValue = qMax(curLevelMaxValue, bound->mTheta.mX);

                bound->mTheta.memo();
                bound->mTheta.mLastAccepts.clear();
                bound->mTheta.mLastAccepts.push_back(1.);
                bound->mTheta.saveCurrentAcceptRate();
                bound->mInitialized = true;
            }
            bound = nullptr;
        }
    }

    // ----------------------------------------------------------------
    //  Init theta f, ti, ...
    // ----------------------------------------------------------------

    QVector<Event*> unsortedEvents = ModelUtilities::unsortEvents (events);

    emit stepChanged(tr("Initializing Events..."), 0, unsortedEvents.size());

    for (auto e : unsortedEvents) {
        mModel->initNodeEvents();
        QString circularEventName = "";
        QList< Event*> startEvents = QList<Event*>();

        const bool ok (e->getThetaMaxPossible (e, circularEventName, startEvents));
         qDebug() << " MCMCLoopMain::InitMCMC check constraint" << e->mName << ok;
        if (!ok) {
            mAbortedReason = QString(tr("Warning : Find Circular Constraint Path %1  %2 ")).arg (e->mName, circularEventName);
            return mAbortedReason;
        }
    }

    for (int i (0); i<unsortedEvents.size(); ++i) {
        if (unsortedEvents.at(i)->mType == Event::eDefault) {

            qDebug() << "in initMCMC(): ---------------------------------";
            mModel->initNodeEvents();
            QString circularEventName = "";
            QList< Event*> startEvents = QList<Event*>();
            const double min (unsortedEvents.at(i)->getThetaMinRecursive (tmin));
            if (!circularEventName.isEmpty()) {
                mAbortedReason = QString(tr("Warning : Find Circular constraint with %1  bad path  %2 ")).arg(unsortedEvents.at(i)->mName, circularEventName);
                return mAbortedReason;
            }

            //qDebug() << "in initMCMC(): Event initialised min : " << unsortedEvents[i]->mName << " : "<<" min"<<min<<tmin;
            mModel->initNodeEvents();
            const double max ( unsortedEvents.at(i)->getThetaMaxRecursive(tmax) );
#ifdef DEBUG
            if (min >= max)
                qDebug() << tr("-----Error Init for event : %1 : min = %2 : max = %3-------").arg(unsortedEvents.at(i)->mName, QString::number(min), QString::number(max));
#endif
            unsortedEvents.at(i)->mTheta.mX = Generator::randomUniform(min, max);
            unsortedEvents.at(i)->mInitialized = true;

            //qDebug() << "in initMCMC(): Event initialized : " << unsortedEvents[i]->mName << " : " << unsortedEvents[i]->mTheta.mX<<" between"<<min<<max;

            double s02_sum (0.);
            for (int j=0; j<unsortedEvents.at(i)->mDates.size(); ++j) {
                Date& date = unsortedEvents.at(i)->mDates[j];

                // 1 - Init ti
                double sigma;
                if (!date.mCalibration->mRepartition.isEmpty()) {
                    const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mCalibration->mRepartition);
                    date.mTheta.mX = date.mCalibration->mTmin + idx *date.mCalibration->mStep;
                  //  qDebug()<<"MCMCLoopMain::Init"<<date.mName <<" mThe.mx="<<QString::number(date.mTheta.mX, 'g', 15);

                    FunctionAnalysis data = analyseFunction(vector_to_map(date.mCalibration->mCurve, tmin, tmax, date.mCalibration->mStep));
                    sigma = double (data.stddev);
                }
                else { // in the case of mRepartion curve is null, we must init ti outside the study period
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
                date.mWiggle.mLastAccepts.push_back(1.);
                date.mWiggle.saveCurrentAcceptRate();

                // 3 - Init sigma MH adaptatif of each Data ti
                date.mTheta.mSigmaMH = sigma;

                // 4 - Clear mLastAccepts array and set this init at 100%
                date.mTheta.mLastAccepts.clear();
                date.mTheta.mLastAccepts.push_back(1.);

                // 5 - Memo
                date.mTheta.memo();

                date.mTheta.saveCurrentAcceptRate();

                // intermediary calculus for the harmonic average
                s02_sum += 1. / (sigma * sigma);
            }

            // 4 - Init S02 of each Event
            unsortedEvents.at(i)->mS02 = unsortedEvents.at(i)->mDates.size() / s02_sum;

            // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
            unsortedEvents.at(i)->mTheta.mSigmaMH = sqrt(unsortedEvents.at(i)->mS02);
            unsortedEvents.at(i)->mAShrinkage = 1.;

            // 6- Clear mLastAccepts array
            unsortedEvents.at(i)->mTheta.mLastAccepts.clear();
            unsortedEvents.at(i)->mTheta.mLastAccepts.push_back(1.);
            // 7 - Memo
            unsortedEvents.at(i)->mTheta.memo();
            unsortedEvents.at(i)->mTheta.saveCurrentAcceptRate();
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

    for (int i=0; i<events.size(); ++i) {
        for (int j=0; j<events.at(i)->mDates.size(); ++j) {
            Date& date = events.at(i)->mDates[j];

            // date.mSigma.mX = sqrt(shrinkageUniform(events[i]->mS02)); // modif the 2015/05/19 with PhL
            date.mSigma.mX = std::abs(date.mTheta.mX - (events.at(i)->mTheta.mX - date.mDelta));

            if (date.mSigma.mX<=1E-6) {
               date.mSigma.mX = 1E-6; // Add control the 2015/06/15 with PhL
               log += line(date.mName + textBold("Sigma indiv. <=1E-6 set to 1E-6"));
            }
            date.mSigma.mSigmaMH = 1.;

            date.mSigma.mLastAccepts.clear();
            date.mSigma.mLastAccepts.push_back(1.);

            date.mSigma.memo();
            date.mSigma.saveCurrentAcceptRate();

        }
        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepProgressed(i);
    }
    // --------------------------- Init phases ----------------------
    emit stepChanged(tr("Initialzsing Phases..."), 0, phases.size());

    i = 0;
    for (Phase* phase : phases ) {
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
    for (const Event* event : events) {
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


        int j (0);
        for (const Date & date : event->mDates) {
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
        for (const Phase * phase : phases) {
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
        for (const PhaseConstraint* constraint : phasesConstraints) {
            ++i;
            log += "<br>";
            log += line(textGreen(tr("Succession ( %1 / %2) : from %3 to %4").arg(QString::number(i),QString::number(phasesConstraints.size()),constraint->mPhaseFrom->mName, constraint->mPhaseTo->mName)));
            log += line(textGreen(tr(" - Gamma : %1").arg(stringForLocal(constraint->mGamma))));
        }
    }

    mInitLog += "<hr>";
    mInitLog += textBold(tr("INIT CHAIN %1").arg(QString::number(mChainIndex+1)));
    mInitLog += "<hr>";
    mInitLog += log;

    return QString();
}

void MCMCLoopMain::update()
{
    //qDebug()<<"MCMCLoopMain::update()";
    const double t_max (mModel->mSettings.mTmax);
    const double t_min (mModel->mSettings.mTmin);

    ChainSpecs& chain = mChains[mChainIndex];

    const bool doMemo = (mState == eBurning) || (mState == eAdapting) || (chain.mTotalIter % chain.mThinningInterval == 0);


    //--------------------- Update Event -----------------------------------------

    for (auto&& event : mModel->mEvents) {
        for ( auto&& date : event->mDates )   {
            date.updateDelta(event);
            date.updateTheta(event);
            date.updateSigma(event);
            date.updateWiggle();

            if (doMemo) {
                date.mTheta.memo();
                date.mSigma.memo();
                date.mWiggle.memo();

                date.mTheta.saveCurrentAcceptRate();
                date.mSigma.saveCurrentAcceptRate();
             }

        }
        //--------------------- Update Events -----------------------------------------
#ifdef TEST
      //  event->mTheta.mX = 0.;
#else
        event->updateTheta(t_min,t_max);
#endif
        if (doMemo) {
           event->mTheta.memo();
           event->mTheta.saveCurrentAcceptRate();
        }

        //--------------------- Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------


       for (auto&& phInEv : event->mPhases)
            phInEv->updateAll(t_min, t_max);
    }


    //--------------------- Memo Phases -----------------------------------------
    if (doMemo) {
        for (auto && ph : mModel->mPhases)
            ph->memoAll();
    }

    //--------------------- Update Phases constraints -----------------------------------------
        for (auto && phConst : mModel->mPhaseConstraints )
           phConst->updateGamma();

}

bool MCMCLoopMain::adapt()
{
    ChainSpecs& chain = mChains[mChainIndex];

    const double taux_min = 41.;           // taux_min minimal rate of acceptation=42
    const double taux_max = 47.;           // taux_max maximal rate of acceptation=46

    bool allOK = true;

    //--------------------- Adapt -----------------------------------------

    double delta = (chain.mBatchIndex < 10000) ? 0.01 : (1. / sqrt(chain.mBatchIndex));

    for (auto && event : mModel->mEvents ) {

       for (auto && date : event->mDates) {

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

        if ((event->mType != Event::eKnown) && ( event->mMethod == Event::eMHAdaptGauss) ) {
            const double taux = 100. * event->mTheta.getCurrentAcceptRate();
            if (taux <= taux_min || taux >= taux_max) {
                allOK = false;
                const double sign = (taux <= taux_min) ? -1. : 1.;
                event->mTheta.mSigmaMH *= pow(10., sign * delta);
            }
        }
    }
    return allOK;
}

void MCMCLoopMain::finalize()
{
    // This is not a copy od data!
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
    //mModel->generateNumericalResults(mChains);
}
