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
                dates.push_back(date);
            }
        }
        
        emit stepChanged(tr("Calibrating..."), 0, dates.size());
        
        for(int i=0; i<dates.size(); ++i)
        {
            if(isInterruptionRequested())
                return ABORTED_BY_USER;
            
            //QTime startTime = QTime::currentTime();
            
            dates[i]->calibrate(mModel->mSettings);
            if(dates[i]->mCalibSum == 0)
            {
                return tr("The following data cannot be nul : ") + dates[i]->mName;
            }
            
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
        event->mTheta.mLastAcceptsLength = acceptBufferLen;
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            date.mTheta.mLastAccepts.clear();
            date.mTheta.mLastAcceptsLength = acceptBufferLen;
            date.mSigma.mLastAccepts.clear();
            date.mSigma.mLastAcceptsLength = acceptBufferLen;
        }
    }
}

void MCMCLoopMain::initMCMC()
{
    QList<Event*>& events = mModel->mEvents;
    QList<Phase*>& phases = mModel->mPhases;
    QList<PhaseConstraint*>& phasesConstraints = mModel->mPhaseConstraints;
    
    double tmin = mModel->mSettings.mTmin;
    double tmax = mModel->mSettings.mTmax;
    double step = mModel->mSettings.mStep;
    
    // ----------------------------------------------------------------
    //  Init gamma
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases gaps..."), 0, events.size());
    for(int i=0; i<phasesConstraints.size(); ++i)
    {
        phasesConstraints[i]->initGamma();
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Init tau
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases durations..."), 0, events.size());
    for(int i=0; i<phases.size(); ++i)
    {
        phases[i]->initTau();
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Init Bounds
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
    // - Définir des niveaux pour les faits
    // - Initialiser les bornes (uniquement, pas les faits) par niveaux croissants
    // => Init borne :
    //  - si valeur fixe, facile!
    //  - si intervalle : random uniform sur l'intervalle (vérifier si min < max pour l'intervalle qui a été modifié par la validation du modèle)
    
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
            
            //qDebug() << "--> Event initialized : " << events[i]->mName << " : " << events[i]->mTheta.mX;
            
            double s02_sum = 0.f;
            for(int j=0; j<unsortedEvents[i]->mDates.size(); ++j)
            {
                Date& date = unsortedEvents[i]->mDates[j];
                
                // Init ti and its sigma
                double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mRepartition);
                date.mTheta.mX = tmin + idx * step;
                
                FunctionAnalysis data = analyseFunction(vector_to_map(date.mCalibration, tmin, tmax, step));
                //date.mTheta.mSigmaMH = data.stddev;
                date.mTheta.mSigmaMH = abs(date.mTheta.mX-unsortedEvents[i]->mTheta.mX);
                date.initDelta(unsortedEvents[i]);
                
                s02_sum += 1.f / (data.stddev * data.stddev);
            }
            unsortedEvents[i]->mS02 = unsortedEvents[i]->mDates.size() / s02_sum;
            unsortedEvents[i]->mTheta.mSigmaMH = unsortedEvents[i]->mS02;
            unsortedEvents[i]->mAShrinkage = 1.;
        }
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Init sigma i
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing variances..."), 0, events.size());
    for(int i=0; i<events.size(); ++i)
    {
        for(int j=0; j<events[i]->mDates.size(); ++j)
        {
            Date& date = events[i]->mDates[j];
            
            //double so = date.mTheta.mX - (events[i]->mTheta.mX - date.mDelta);
            
            date.mSigma.mX = sqrt(shrinkageUniform(events[i]->mS02));
            //date.mSigma.mX = sqrt(shrinkageUniform(so * so));
            //date.mSigma.mX = date.mTheta.mX - (events[i]->mTheta.mX - date.mDelta);
            
            date.mSigma.mSigmaMH = 1.;
        }
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
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Log Init
    // ----------------------------------------------------------------
    QString log;
    
    log += line(textBold("Events Initialisation (with their data)"));
    log += "<br>";
    
    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        
        log += line("--------");
        
        if(event->type() == Event::eKnown)
        {
            EventKnown* bound = dynamic_cast<EventKnown*>(event);
            if(bound)
            {
                log += line(textRed("Bound (" + QString::number(i+1) + "/" + QString::number(events.size()) + ") : " + bound->mName));
                log += line(textRed(" - theta (value) : " + QString::number(bound->mTheta.mX)));
                log += line(textRed(" - theta (sigma MH) : " + QString::number(bound->mTheta.mSigmaMH)));
                log += line(textRed("--------"));
            }
        }
        else
        {
            log += line(textBlue("Event (" + QString::number(i+1) + "/" + QString::number(events.size()) + ") : " + event->mName));
            log += line(textBlue(" - theta (value) : " + QString::number(event->mTheta.mX)));
            log += line(textBlue(" - theta (sigma MH) : " + QString::number(event->mTheta.mSigmaMH)));
            log += line(textBlue(" - SO2 : " + QString::number(event->mS02)));
            log += line(textBlue("--------"));
        }
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            
            log += line(textGreen("Data (" + QString::number(j+1) + "/" + QString::number(event->mDates.size()) + ") : " + date.mName));
            log += line(textGreen(" - ti (value) : " + QString::number(date.mTheta.mX)));
            log += line(textGreen(" - ti (sigma MH) : " + QString::number(date.mTheta.mSigmaMH)));
            log += line(textGreen(" - sigmai (value) : " + QString::number(date.mSigma.mX)));
            log += line(textGreen(" - sigmai (sigma MH) : " + QString::number(date.mSigma.mSigmaMH)));
            log += line(textGreen(" - deltai (value) : " + QString::number(date.mDelta)));
            log += line(textGreen("--------"));
        }
    }
    
    if(phases.size() > 0)
    {
        log += "<br>";
        log += line(textBold("Phases Initialisation"));
        log += "<br>";
        
        for(int i=0; i<phases.size(); ++i)
        {
            Phase* phase = phases[i];
            
            log += line(textPurple("Phase (" + QString::number(i+1) + "/" + QString::number(phases.size()) + ") : " + phase->mName));
            log += line(textPurple(" - alpha : " + QString::number(phase->mAlpha.mX)));
            log += line(textPurple(" - beta : " + QString::number(phase->mBeta.mX)));
            log += line(textPurple(" - tau : " + QString::number(phase->mTau)));
            log += line(textPurple("--------"));
        }
    }
    
    if(phasesConstraints.size() > 0)
    {
        log += "<br>";
        log += line(textBold("Phases Constraints Initialisation"));
        log += "<br>";
        
        for(int i=0; i<phasesConstraints.size(); ++i)
        {
            PhaseConstraint* constraint = phasesConstraints[i];
            
            log += line("PhaseConstraint (" + QString::number(i+1) + "/" + QString::number(phasesConstraints.size()) + ") : " + QString::number(constraint->mId));
            log += line(" - gamma : " + QString::number(constraint->mGamma));
        }
    }
    
    mInitLog += line(textBold("------------------------------------"));
    mInitLog += line(textBold("Init Chain " + QString::number(mChainIndex+1)));
    mInitLog += line(textBold("------------------------------------"));
    mInitLog += log;
    
    //qDebug() << log;
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
    
    //qDebug() << mState << " : " << chain.mTotalIter << " : " << doMemo;
    
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
            
            //qDebug() << "it #" << chain.mTotalIter << " | date " << counter;
            
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
        //qDebug() << "it #" << chain.mTotalIter << " | event " << i;
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
    mModel->mChains = mChains;
    
    mModel->generateCorrelations(mChains);
    mModel->generatePosteriorDensities(mChains, 1024, 1);
    mModel->generateNumericalResults(mChains);
}


