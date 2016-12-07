#include "MCMCLoopMain.h"

//#include "Model.h"
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
//class EventKnown;
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
        //----------------- Calibrate measures --------------------------------------
        
        QList<Date*> dates;
        // find number of dates, to optimize memory space
        int nbDates = 0;
        foreach (const Event* e, events)
            nbDates += e->mDates.size();

        dates.reserve(nbDates);
        for (int i=0; i<events.size(); ++i) {
            int num_dates = events.at(i)->mDates.size();
            for (int j=0; j<num_dates; ++j) {
                Date* date = &events.at(i)->mDates[j];
                dates.push_back(date);
                date = nullptr;
            }
        }


        if(isInterruptionRequested())
            return ABORTED_BY_USER;
        
        emit stepChanged(tr("Calibrating..."), 0, dates.size());
        
        for(int i=0; i<dates.size(); ++i) {
            //QTime startTime = QTime::currentTime();
            if (dates.at(i)->mCalibration->mCurve.isEmpty())
                dates.at(i)->calibrate(mModel->mSettings, mProject);
         
            if(isInterruptionRequested())
                return ABORTED_BY_USER;
            
            emit stepProgressed(i);
            dates[i] = nullptr;
            //QTime endTime = QTime::currentTime();
            //int timeDiff = startTime.msecsTo(endTime);
            //mLog += "Data \"" + dates[i]->mName + "\" (" + dates[i]->mPlugin->getName() + ") calibrated in " + QString::number(timeDiff) + " ms\n";
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
    long int initReserve = 0;

    for (const ChainSpecs c: mChains)
       initReserve +=( 1 + (c.mMaxBatchs*c.mNumBatchIter) + c.mNumBurnIter + (c.mNumRunIter/c.mThinningInterval) );

    for (Event* event : mModel->mEvents) {
        event->mTheta.reset();
        event->mTheta.reserve(initReserve);

        event->mTheta.mLastAccepts.reserve(acceptBufferLen);
        event->mTheta.mLastAcceptsLength = acceptBufferLen;

        // event->mTheta.mAllAccepts.clear(); //don't clean, avalable for cumulate chain

        for(Date& date : event->mDates) {
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

    for(Phase* phase : mModel->mPhases) {
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
    const double step = mModel->mSettings.mStep;
    
    if(isInterruptionRequested())
        return ABORTED_BY_USER;

    // ----------------------------------------------------------------
    //  Reset Events
    // ----------------------------------------------------------------
    for (Event* ev : events)
        ev->mInitialized = false;

    // ----------------------------------------------------------------
    //  Init gamma
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases gaps..."), 0, phasesConstraints.size());
    for (int i=0; i<phasesConstraints.size(); ++i) {
        phasesConstraints.at(i)->initGamma();
        
        if (isInterruptionRequested())
            return ABORTED_BY_USER;
        
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Init tau
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases durations..."), 0, phases.size());
    for (int i=0; i<phases.size(); ++i) {
        phases.at(i)->initTau();
        
        if (isInterruptionRequested())
            return ABORTED_BY_USER;
        
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Init Bounds
    // - Définir des niveaux pour les faits
    // - Initialiser les bornes (uniquement, pas les faits) par niveaux croissants
    // => Init borne :
    //  - si valeur fixe, facile!
    //  - si intervalle : random uniform sur l'intervalle (vérifier si min < max pour l'intervalle qui a été modifié par la validation du modèle)
    // ----------------------------------------------------------------
    QVector<Event*> eventsByLevel = ModelUtilities::sortEventsByLevel(mModel->mEvents);
    int curLevel = 0;
    double curLevelMaxValue = mModel->mSettings.mTmin;
    double prevLevelMaxValue = mModel->mSettings.mTmin;
    
    for (int i=0; i<eventsByLevel.size(); ++i) {
        if (eventsByLevel.at(i)->type() == Event::eKnown) {
            EventKnown* bound = dynamic_cast<EventKnown*>(eventsByLevel[i]);

            if (bound) {
                if (curLevel != bound->mLevel) {
                    curLevel = bound->mLevel;
                    prevLevelMaxValue = curLevelMaxValue;
                    curLevelMaxValue = mModel->mSettings.mTmin;
                }
                
                if (bound->mKnownType == EventKnown::eFixed) {
                    bound->mTheta.mX = bound->mFixed;
                }
                else if (bound->mKnownType == EventKnown::eUniform) {
                    bound->mTheta.mX = Generator::randomUniform(qMax(bound->mUniformStart, prevLevelMaxValue),
                                                                bound->mUniformEnd);
                    qDebug()<<"in initMCMC(): init bound "+bound->mName+QString::number(bound->mTheta.mX);
                }
                curLevelMaxValue = qMax(curLevelMaxValue, bound->mTheta.mX);
                
                bound->mTheta.memo();
                bound->mTheta.mLastAccepts.clear();
                bound->mInitialized = true;
            }
            bound = 0;
        }
    }
    
    // ----------------------------------------------------------------
    //  Init theta f, ti, ...
    // ----------------------------------------------------------------

    QVector<Event*> unsortedEvents = ModelUtilities::unsortEvents(events);
    QVector<QVector<Event*> > eventBranches = ModelUtilities::getAllEventsBranches(events);
    QVector<QVector<Phase*> > phaseBranches = ModelUtilities::getAllPhasesBranches(phases, mModel->mSettings.mTmax - mModel->mSettings.mTmin);
       

    emit stepChanged(tr("Initializing events..."), 0, unsortedEvents.size());

    for (int i=0; i<unsortedEvents.size(); ++i) {
        if (unsortedEvents.at(i)->mType == Event::eDefault) {
            const double min = unsortedEvents.at(i)->getThetaMinRecursive(tmin, eventBranches, phaseBranches);
            const double max = unsortedEvents.at(i)->getThetaMaxRecursive(tmax, eventBranches, phaseBranches);
            
            unsortedEvents.at(i)->mTheta.mX = Generator::randomUniform(min, max);
            unsortedEvents.at(i)->mInitialized = true;
            
            qDebug() << "in initMCMC(): Event initialized : " << unsortedEvents[i]->mName << " : " << unsortedEvents[i]->mTheta.mX<<" between"<<min<<max;

            double s02_sum (0.);
            for (int j=0; j<unsortedEvents.at(i)->mDates.size(); ++j) {
                Date& date = unsortedEvents.at(i)->mDates[j];
                
                // 1 - Init ti
                double sigma;
                if (!date.mCalibration->mRepartition.isEmpty()) {
                    const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mCalibration->mRepartition);
                    date.mTheta.mX = date.mCalibration->mTmin + idx *mModel->mSettings.mStep;
                    qDebug()<<"MCMCLoopMain::Init mThe.mx="<<QString::number(date.mTheta.mX, 'g', 15);

                    FunctionAnalysis data = analyseFunction(vector_to_map(date.mCalibration->mCurve, tmin, tmax, step));
                    sigma = (double) data.stddev;
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
                date.mWiggle.saveCurrentAcceptRate();
                // 3 - Init sigma MH adaptatif of each Data ti
                date.mTheta.mSigmaMH = sigma;

                // 4 - Clear mLastAccepts array
                date.mTheta.mLastAccepts.clear();

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
            //unsortedEvents.at(i)->mTheta.mLastAccepts.clear();

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
    emit stepChanged(tr("Initializing variances..."), 0, events.size());
    
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
            date.mSigma.memo();
            date.mSigma.saveCurrentAcceptRate();

        }
        if (isInterruptionRequested())
            return ABORTED_BY_USER;
        
        emit stepProgressed(i);
    }
    // ----------------------------------------------------------------
    //  Init phases
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases..."), 0, phases.size());
    for (int i=0; i<phases.size(); ++i) {
        Phase* phase = phases[i];

        phase->updateAll(tmin, tmax);
        phase->memoAll();
        if (isInterruptionRequested())
            return ABORTED_BY_USER;
        
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Log Init
    // ----------------------------------------------------------------
    log += "<hr>";
    log += textBold("Events Initialisation (with their data)");
    
    int i = 0;
    foreach (const Event* event, events) {
        ++i;
        log += "<hr><br>";
        
        if (event->type() == Event::eKnown) {
             const EventKnown* bound = dynamic_cast<const EventKnown*>(event);
            if (bound) {
                log += line(textRed("Bound (" + QString::number(i) + "/" + QString::number(events.size()) + ") : " + bound->mName));
                log += line(textRed(" - theta (value) : " + DateUtils::convertToAppSettingsFormatStr(bound->mTheta.mX)+" "+ DateUtils::getAppSettingsFormat()));
                log += line(textRed(" - theta (sigma MH) : " + DateUtils::dateToString(bound->mTheta.mSigmaMH)));
            }
        }
        else {
            log += line(textBlue("Event (" + QString::number(i) + "/" + QString::number(events.size()) + ") : " + event->mName));
            log += line(textBlue(" - theta (value) : " + DateUtils::convertToAppSettingsFormatStr(event->mTheta.mX) +" "+ DateUtils::getAppSettingsFormat()));
            log += line(textBlue(" - theta (sigma MH) : " + DateUtils::dateToString(event->mTheta.mSigmaMH)));
            log += line(textBlue(" - SO2 : " + DateUtils::dateToString(event->mS02)));
        }
        
        
        int j = 0;
        foreach (const Date date, event->mDates) {
            ++j;
            log += "<br>";

            log += line(textBlack("Data (" + QString::number(j) + "/" + QString::number(event->mDates.size()) + ") : " + date.mName));
            log += line(textBlack(" - ti (value) : " + DateUtils::convertToAppSettingsFormatStr(date.mTheta.mX)+" "+ DateUtils::getAppSettingsFormat()));
            if (date.mMethod == Date::eMHSymGaussAdapt)
                log += line(textBlack(" - ti (sigma MH) : " + DateUtils::dateToString(date.mTheta.mSigmaMH)));

            log += line(textBlack(" - sigmai (value) : " + DateUtils::dateToString(date.mSigma.mX)));
            log += line(textBlack(" - sigmai (sigma MH) : " +DateUtils::dateToString(date.mSigma.mSigmaMH)));
            if (date.mDeltaType != Date::eDeltaNone)
                log += line(textBlack(" - deltai (value) : " + DateUtils::dateToString(date.mDelta)));

        }
    }
    
    if (phases.size() > 0) {
        log += "<hr>";
        log += textBold(tr("Phases Initialisation"));
        log += "<hr>";

        int i = 0;
        foreach (const Phase * phase, phases) {
            ++i;
            log += "<br>";
            log += line(textPurple("Phase (" + QString::number(i) + "/" + QString::number(phases.size()) + ") : " + phase->mName));
            log += line(textPurple(" - alpha : " + DateUtils::convertToAppSettingsFormatStr(phase->mAlpha.mX)+" "+ DateUtils::getAppSettingsFormat()));
            log += line(textPurple(" - beta : " + DateUtils::convertToAppSettingsFormatStr(phase->mBeta.mX)+" "+ DateUtils::getAppSettingsFormat()));
            log += line(textPurple(" - tau : " + DateUtils::dateToString(phase->mTau)));
        }
    }
    
    if (phasesConstraints.size() > 0) {

        log += "<hr>";
        log += textBold(tr("Phases Constraints Initialisation"));
        log += "<hr>";
        
        int i = 0;
        foreach (const PhaseConstraint* constraint, phasesConstraints) {
            ++i;
            log += "<br>";
            log += line("PhaseConstraint (" + QString::number(i) + "/" + QString::number(phasesConstraints.size()) + ") : " + QString::number(constraint->mId));
            log += line(" - gamma : " + DateUtils::dateToString(constraint->mGamma));
        }
    }
    
    mInitLog += "<hr>";
    mInitLog += textBold("INIT CHAIN " + QString::number(mChainIndex+1));
    mInitLog += "<hr>";
    mInitLog += log;
 
    return QString();
}

void MCMCLoopMain::update()
{
    const double t_min = mModel->mSettings.mTmin;
    const double t_max = mModel->mSettings.mTmax;

    ChainSpecs& chain = mChains[mChainIndex];
    
    const bool doMemo = (mState == eBurning) || (mState == eAdapting) || (chain.mTotalIter % chain.mThinningInterval == 0);
    

    //--------------------- Update Event -----------------------------------------

    for (Event* event : mModel->mEvents) {
        for ( Date& date : event->mDates )   {

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

        for (auto phInEv : event->mPhases)
            phInEv->updateAll(t_min, t_max);
    }


    //--------------------- Memo Phases -----------------------------------------
    if (doMemo) {
        for (auto ph : mModel->mPhases)
            ph->memoAll();
    }

    //--------------------- Update Phases constraints -----------------------------------------
    for (auto phConst : mModel->mPhaseConstraints )
        phConst->updateGamma();

}

bool MCMCLoopMain::adapt()
{
    ChainSpecs& chain = mChains[mChainIndex];
    QList<Event*>& events = mModel->mEvents;
    
    const double taux_min = 41.;           // taux_min minimal rate of acceptation=42
    const double taux_max = 47.;           // taux_max maximal rate of acceptation=46

    bool allOK = true;
    
    //--------------------- Adapt -----------------------------------------
    
    double delta = (chain.mBatchIndex < 10000) ? 0.01 : (1. / sqrt(chain.mBatchIndex));
    
    for (int i=0; i<events.size(); ++i) {
        Event* event = events[i];
        
        for (int j=0; j<event->mDates.size(); ++j) {
            Date& date = event->mDates[j];
            
            //--------------------- Adapt Sigma MH de Theta i -----------------------------------------
            
            if (date.mMethod == Date::eMHSymGaussAdapt) {
                const double taux = 100. * date.mTheta.getCurrentAcceptRate();
                if (taux <= taux_min || taux >= taux_max) {
                    allOK = false;
                    double sign = (taux <= taux_min) ? -1. : 1.;
                    date.mTheta.mSigmaMH *= pow(10., sign * delta);
                }
            }
            
            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            
            const double taux = 100. * date.mSigma.getCurrentAcceptRate();
            if (taux <= taux_min || taux >= taux_max) {
                allOK = false;
                double sign = (taux <= taux_min) ? -1. : 1.;
                date.mSigma.mSigmaMH *= pow(10., sign * delta);
            }
        }
        
        //--------------------- Adapt Sigma MH de Theta f -----------------------------------------
        
        if ((event->mType != Event::eKnown) && (event->mMethod == Event::eMHAdaptGauss) ) {
            const double taux = 100. * event->mTheta.getCurrentAcceptRate();
            qDebug()<<"MCMCLoopMain adapt"<< event->mTheta.mSigmaMH;
            if (taux <= taux_min || taux >= taux_max) {
                allOK = false;
                double sign = (taux <= taux_min) ? -1. : 1.;
                event->mTheta.mSigmaMH *= pow(10., sign * delta);
                qDebug()<<"MCMCLoopMain adapt"<< event->mTheta.mSigmaMH<<" delta="<<delta;
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


