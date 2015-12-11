#include "MCMCLoopMain.h"

#include "Model.h"
#include "EventKnown.h"
#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "Date.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "../PluginAbstract.h"

#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QTime>


MCMCLoopMain::MCMCLoopMain(Model* model):MCMCLoop(),
mModel(model)
{
    if(mModel)
    {
        setMCMCSettings(mModel->mMCMCSettings);
    }
}

MCMCLoopMain::~MCMCLoopMain()
{

}

QString MCMCLoopMain::calibrate()
{
    if(mModel)
    {
        QList<Event*>& events = mModel->mEvents;
        
        //----------------- Calibrate measures --------------------------------------
        
        QList<Date*> dates;
        for(int i=0; i<events.size(); ++i)
        {
            int num_dates = (int)events[i]->mDates.size();
            for(int j=0; j<num_dates; ++j)
            {
                Date* date = &events[i]->mDates[j];
                date->mCalibration=events[i]->mDates[j].mCalibration;
                dates.push_back(date);
            }
        }
        
        if(isInterruptionRequested())
            return ABORTED_BY_USER;
        
        emit stepChanged(tr("Calibrating..."), 0, dates.size());
        
        for(int i=0; i<dates.size(); ++i)
        {
            //QTime startTime = QTime::currentTime();
            dates[i]->calibrate(mModel->mSettings);
         
            if(isInterruptionRequested())
                return ABORTED_BY_USER;
            
            emit stepProgressed(i);
            
            //QTime endTime = QTime::currentTime();
            //int timeDiff = startTime.msecsTo(endTime);
            //mLog += "Data \"" + dates[i]->mName + "\" (" + dates[i]->mPlugin->getName() + ") calibrated in " + QString::number(timeDiff) + " ms\n";
        }
        return QString();
    }
    return tr("Invalid model");
}

void MCMCLoopMain::initVariablesForChain()
{
    Chain& chain = mChains[mChainIndex];
    QList<Event*>& events = mModel->mEvents;
    
    int acceptBufferLen = chain.mNumBatchIter; //chainLen / 100;
    
    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        event->mTheta.mLastAccepts.clear();
        event->mTheta.mLastAccepts.reserve(acceptBufferLen);
        event->mTheta.mLastAcceptsLength = acceptBufferLen;

        //event->mTheta.mAllAccepts.clear(); //don't clean, avalable for cumulate chain
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            date.mTheta.mLastAccepts.clear();
            date.mTheta.mLastAccepts.reserve(acceptBufferLen);
            date.mTheta.mLastAcceptsLength = acceptBufferLen;
            date.mSigma.mLastAccepts.clear();
            date.mSigma.mLastAccepts.reserve(acceptBufferLen);
            date.mSigma.mLastAcceptsLength = acceptBufferLen;
        }
    }
}

QString MCMCLoopMain::initMCMC()
{
    QList<Event*>& events = mModel->mEvents;
    QList<Phase*>& phases = mModel->mPhases;
    QList<PhaseConstraint*>& phasesConstraints = mModel->mPhaseConstraints;
    
    double tmin = mModel->mSettings.mTmin;
    double tmax = mModel->mSettings.mTmax;
    double step = mModel->mSettings.mStep;
    
    if(isInterruptionRequested())
        return ABORTED_BY_USER;
    
    // ----------------------------------------------------------------
    //  Init gamma
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases gaps..."), 0, events.size());
    for(int i=0; i<phasesConstraints.size(); ++i)
    {
        phasesConstraints[i]->initGamma();
        
        if(isInterruptionRequested())
            return ABORTED_BY_USER;
        
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Init tau
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases durations..."), 0, events.size());
    for(int i=0; i<phases.size(); ++i)
    {
        phases[i]->initTau();
        
        if(isInterruptionRequested())
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
    
    for(int i=0; i<eventsByLevel.size(); ++i)
    {
        if(eventsByLevel[i]->mType == Event::eKnown)
        {
            EventKnown* bound = dynamic_cast<EventKnown*>(eventsByLevel[i]);
            if(bound)
            {
                if(curLevel != bound->mLevel)
                {
                    curLevel = bound->mLevel;
                    prevLevelMaxValue = curLevelMaxValue;
                    curLevelMaxValue = mModel->mSettings.mTmin;
                }
                
                if(bound->mKnownType == EventKnown::eFixed)
                {
                    bound->mTheta.mX = bound->mFixed;
                }
                else if(bound->mKnownType == EventKnown::eUniform)
                {
                    bound->mTheta.mX = Generator::randomUniform(qMax(bound->mUniformStart, prevLevelMaxValue),
                                                                bound->mUniformEnd);
                }
                curLevelMaxValue = qMax(curLevelMaxValue, bound->mTheta.mX);
                
                bound->mInitialized = true;
            }
        }
    }
    
    // ----------------------------------------------------------------
    //  Init theta f, ti, ...
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing events..."), 0, events.size());
    QVector<Event*> unsortedEvents = ModelUtilities::unsortEvents(events);
    
    QVector<QVector<Event*> > eventBranches = ModelUtilities::getAllEventsBranches(events);
    QVector<QVector<Phase*> > phaseBranches = ModelUtilities::getAllPhasesBranches(phases, mModel->mSettings.mTmax - mModel->mSettings.mTmin);
    
    /*qDebug() << "==================";
    for(int i=0; i<phaseBranches.size(); ++i)
    {
        qDebug() << "----------- branch " << i;
        for(int j=0; j<phaseBranches[i].size(); ++j)
        {
            qDebug() << phaseBranches[i][j]->mName << " => ";
        }
    }*/
    
    for(int i=0; i<unsortedEvents.size(); ++i)
    {
        if(unsortedEvents[i]->mType == Event::eDefault)
        {
            double min = unsortedEvents[i]->getThetaMinRecursive(tmin, eventBranches, phaseBranches);
            double max = unsortedEvents[i]->getThetaMaxRecursive(tmax, eventBranches, phaseBranches);
            
            unsortedEvents[i]->mTheta.mX = Generator::randomUniform(min, max);
            unsortedEvents[i]->mInitialized = true;
            
            //qDebug() << "--> Event initialized : " << unsortedEvents[i]->getName() << " : " << unsortedEvents[i]->mTheta.mX;
            
            double s02_sum = 0.f;
            for(int j=0; j<unsortedEvents[i]->mDates.size(); ++j)
            {
                Date& date = unsortedEvents[i]->mDates[j];
                
                // Init ti and its sigma
                double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mRepartition);
                date.mTheta.mX = tmin + idx * step;
                
                FunctionAnalysis data = analyseFunction(vector_to_map(date.mCalibration, tmin, tmax, step));
                date.mTheta.mSigmaMH = data.stddev; // computed in RenDateModel and ChronoModel V1.1
                //date.mTheta.mSigmaMH = fabs(date.mTheta.mX-unsortedEvents[i]->mTheta.mX);
                date.initDelta(unsortedEvents[i]);
                
                s02_sum += 1.f / (data.stddev * data.stddev);
            }
            unsortedEvents[i]->mS02 = unsortedEvents[i]->mDates.size() / s02_sum;
            unsortedEvents[i]->mTheta.mSigmaMH = sqrt(unsortedEvents[i]->mS02);
            unsortedEvents[i]->mAShrinkage = 1.;
        }
        
        if(isInterruptionRequested())
            return ABORTED_BY_USER;
        
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Init sigma i
    // ----------------------------------------------------------------
    QString log;
    emit stepChanged(tr("Initializing variances..."), 0, events.size());
    
    for(int i=0; i<events.size(); ++i)
    {
        for(int j=0; j<events[i]->mDates.size(); ++j)
        {
            Date& date = events[i]->mDates[j];
            
            
           // date.mSigma.mX = sqrt(shrinkageUniform(events[i]->mS02)); // modif the 2015/05/19 with PhL
           date.mSigma.mX = fabs(date.mTheta.mX-(events[i]->mTheta.mX-date.mDelta));
           
            if(date.mSigma.mX<=1E-6){
               date.mSigma.mX=1E-6; // Add control the 2015/06/15 with PhL
               log += line(date.mName + textBold("Sigma indiv. <=1E-6 set to 1E-6"));
            }
            date.mSigma.mSigmaMH = 1.;
        }
        if(isInterruptionRequested())
            return ABORTED_BY_USER;
        
        emit stepProgressed(i);
    }
    // ----------------------------------------------------------------
    //  Init phases
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases..."), 0, events.size());
    for(int i=0; i<phases.size(); ++i)
    {
        Phase* phase = phases[i];
        phase->updateAll(tmin, tmax);
        
        if(isInterruptionRequested())
            return ABORTED_BY_USER;
        
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Log Init
    // ----------------------------------------------------------------
    log += "<hr>";
    log += textBold("Events Initialisation (with their data)");
    
    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        log += "<hr><br>";
        
        if(event->type() == Event::eKnown)
        {
            EventKnown* bound = dynamic_cast<EventKnown*>(event);
            if(bound)
            {
                log += line(textRed("Bound (" + QString::number(i+1) + "/" + QString::number(events.size()) + ") : " + bound->mName));
                log += line(textRed(" - theta (value) : " + QString::number(bound->mTheta.mX)));
                log += line(textRed(" - theta (sigma MH) : " + QString::number(bound->mTheta.mSigmaMH)));
            }
        }
        else
        {
            log += line(textBlue("Event (" + QString::number(i+1) + "/" + QString::number(events.size()) + ") : " + event->mName));
            log += line(textBlue(" - theta (value) : " + QString::number(event->mTheta.mX)));
            log += line(textBlue(" - theta (sigma MH) : " + QString::number(event->mTheta.mSigmaMH)));
            log += line(textBlue(" - SO2 : " + QString::number(event->mS02)));
        }
        
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            log += "<br>";
            Date& date = event->mDates[j];
            
            log += line(textGreen("Data (" + QString::number(j+1) + "/" + QString::number(event->mDates.size()) + ") : " + event->mDates[j].mName));
            log += line(textGreen(" - ti (value) : " + QString::number(date.mTheta.mX)));
            if(date.mMethod == Date::eMHSymGaussAdapt){
                log += line(textGreen(" - ti (sigma MH) : " + QString::number(date.mTheta.mSigmaMH)));
            }
            log += line(textGreen(" - sigmai (value) : " + QString::number(date.mSigma.mX)));
            log += line(textGreen(" - sigmai (sigma MH) : " + QString::number(date.mSigma.mSigmaMH)));
            log += line(textGreen(" - deltai (value) : " + QString::number(date.mDelta)));
        }
    }
    
    if(phases.size() > 0)
    {
        log += "<hr>";
        log += textBold("Phases Initialisation");
        log += "<hr>";
        
        for(int i=0; i<phases.size(); ++i)
        {
            Phase* phase = phases[i];
            
            log += "<br>";
            log += line(textPurple("Phase (" + QString::number(i+1) + "/" + QString::number(phases.size()) + ") : " + phase->mName));
            log += line(textPurple(" - alpha : " + QString::number(phase->mAlpha.mX)));
            log += line(textPurple(" - beta : " + QString::number(phase->mBeta.mX)));
            log += line(textPurple(" - tau : " + QString::number(phase->mTau)));
        }
    }
    
    if(phasesConstraints.size() > 0)
    {
        log += "<hr>";
        log += textBold("Phases Constraints Initialisation");
        log += "<hr>";
        
        for(int i=0; i<phasesConstraints.size(); ++i)
        {
            PhaseConstraint* constraint = phasesConstraints[i];
            
            log += "<br>";
            log += line("PhaseConstraint (" + QString::number(i+1) + "/" + QString::number(phasesConstraints.size()) + ") : " + QString::number(constraint->mId));
            log += line(" - gamma : " + QString::number(constraint->mGamma));
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
    QList<Event*>& events = mModel->mEvents;
    QList<Phase*>& phases = mModel->mPhases;
    QList<PhaseConstraint*>& phasesConstraints = mModel->mPhaseConstraints;
    
    double t_min = mModel->mSettings.mTmin;
    double t_max = mModel->mSettings.mTmax;
    
    Chain& chain = mChains[mChainIndex];
    
    bool doMemo = (mState == eBurning) || (mState == eAdapting) || (chain.mTotalIter % chain.mThinningInterval == 0);
    
    //--------------------- Update Dates -----------------------------------------
    
    int counter = 0;
    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = events[i]->mDates[j];
            
            date.updateDelta(event);
            date.updateTheta(event);
            date.updateSigma(event);
            date.updateWiggle();
            
            if(doMemo)
            {
                date.mTheta.memo();
                date.mSigma.memo();
                date.mWiggle.memo();
                
                date.mTheta.saveCurrentAcceptRate();
                date.mSigma.saveCurrentAcceptRate();
            }
            ++counter;
        }
    }

    //--------------------- Update Events -----------------------------------------

    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        
        event->updateTheta(t_min, t_max);
        if(doMemo)
        {
            event->mTheta.memo();
            event->mTheta.saveCurrentAcceptRate();
        }
    }

    //--------------------- Update Phases -----------------------------------------

    for(int i=0; i<phases.size(); ++i)
    {
        phases[i]->updateAll(t_min, t_max);
        if(doMemo)
            phases[i]->memoAll();
    }
    
    //--------------------- Update Phases constraints -----------------------------------------
    
    for(int i=0; i<phasesConstraints.size(); ++i)
    {
        phasesConstraints[i]->updateGamma();
    }
}

bool MCMCLoopMain::adapt()
{
    Chain& chain = mChains[mChainIndex];
    QList<Event*>& events = mModel->mEvents;
    
    const double taux_min = 41.;           // taux_min minimal rate of acceptation=42
    const double taux_max = 47.;           // taux_max maximal rate of acceptation=46
    
    bool allOK = true;
    
    //--------------------- Adapt -----------------------------------------
    
    double delta = (chain.mBatchIndex < 10000) ? 0.01f : (1 / sqrt(chain.mBatchIndex));
    
    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            
            //--------------------- Adapt Sigma MH de Theta i -----------------------------------------
            
            if(date.mMethod == Date::eMHSymGaussAdapt)
            {
                double taux = 100.f * date.mTheta.getCurrentAcceptRate();
                if(taux <= taux_min || taux >= taux_max)
                {
                    allOK = false;
                    double sign = (taux <= taux_min) ? -1.f : 1.f;
                    date.mTheta.mSigmaMH *= pow(10.f, sign * delta);
                }
            }
            
            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            
            double taux = 100.f * date.mSigma.getCurrentAcceptRate();
            if(taux <= taux_min || taux >= taux_max)
            {
                allOK = false;
                double sign = (taux <= taux_min) ? -1.f : 1.f;
                date.mSigma.mSigmaMH *= pow(10.f, sign * delta);
            }
        }
        
        //--------------------- Adapt Sigma MH de Theta f -----------------------------------------
        
        if(event->mMethod == Event::eMHAdaptGauss)
        {
            double taux = 100.f * event->mTheta.getCurrentAcceptRate();
            if(taux <= taux_min || taux >= taux_max)
            {
                allOK = false;
                double sign = (taux <= taux_min) ? -1.f : 1.f;
                event->mTheta.mSigmaMH *= pow(10.f, sign * delta);
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


