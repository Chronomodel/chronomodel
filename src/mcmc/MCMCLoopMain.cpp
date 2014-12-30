#include "MCMCLoopMain.h"

#include "Model.h"
#include "EventKnown.h"
#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "Date.h"
#include "ModelUtilities.h"
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
            QTime startTime = QTime::currentTime();
            
            dates[i]->calibrate(mModel->mSettings);
            if(dates[i]->mCalibSum == 0)
            {
                return tr("The following data cannot be nul : ") + dates[i]->mName;
            }
            
            emit stepProgressed(i);
            
            QTime endTime = QTime::currentTime();
            int timeDiff = startTime.msecsTo(endTime);
            mLog += "Data \"" + dates[i]->mName + "\" (" + dates[i]->mPlugin->getName() + ") calibrated in " + QString::number(timeDiff) + " ms\n";
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

QString MCMCLoopMain::initMCMC()
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
            EventKnown* bound = dynamic_cast<EventKnown*>(mModel->mEvents[i]);
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
    for(int i=0; i<events.size(); ++i)
    {
        double min = events[i]->getThetaMin(tmin);
        double max = events[i]->getThetaMax(tmax);
        
        events[i]->mTheta.mX = Generator::randomUniform(min, max);
        events[i]->mInitialized = true;
        
        qDebug() << "--> Event initialized : " << events[i]->mName << " : " << events[i]->mTheta.mX;
        
        double s02_sum = 0.f;
        for(int j=0; j<events[i]->mDates.size(); ++j)
        {
            Date& date = events[i]->mDates[j];
            
            // Init ti and its sigma
            double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mRepartition);
            date.mTheta.mX = tmin + idx * step;
            
            FunctionAnalysis data = analyseFunction(vector_to_map(date.mCalibration, tmin, tmax, step));
            date.mTheta.mSigmaMH = data.stddev;
            date.initDelta(events[i]);
            
            
            s02_sum += 1.f / (date.mTheta.mSigmaMH * date.mTheta.mSigmaMH);
        }
        events[i]->mS02 = events[i]->mDates.size() / s02_sum;
        events[i]->mAShrinkage = 1.;
        
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
            double so = date.mTheta.mX - (events[i]->mTheta.mX - date.mDelta);
            date.mSigma.mX = shrinkageUniform(so * so);
            date.mSigma.mSigmaMH = 1.f;
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
    QString initLog;
    
    initLog += "---------------\n";
    initLog += "INIT\n";
    initLog += "---------------\n";
    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        
        if(event->type() == Event::eKnown)
        {
            initLog += ">> Bound : " + event->mName + "\n";
            initLog += "  - theta (value) : " + QString::number(event->mTheta.mX) + "\n";
            initLog += "  - theta (sigma MH) : " + QString::number(event->mTheta.mSigmaMH) + "\n";
        }
        else
        {
            initLog += ">> Event : " + event->mName + "\n";
            initLog += "  - theta (value) : " + QString::number(event->mTheta.mX) + "\n";
            initLog += "  - theta (sigma MH) : " + QString::number(event->mTheta.mSigmaMH) + "\n";
            initLog += "  - SO2 : " + QString::number(event->mS02) + "\n";
            initLog += "  - AShrinkage : " + QString::number(event->mAShrinkage) + "\n";
        }
        mLog += "---------------\n";
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            
            initLog += " > Data : " + date.mName + "\n";
            initLog += "  - theta (value) : " + QString::number(date.mTheta.mX) + "\n";
            initLog += "  - theta (sigma MH) : " + QString::number(date.mTheta.mSigmaMH) + "\n";
            initLog += "  - sigma (value) : " + QString::number(date.mSigma.mX) + "\n";
            initLog += "  - sigma (sigma MH) : " + QString::number(date.mSigma.mSigmaMH) + "\n";
            initLog += "  - delta (value) : " + QString::number(date.mDelta) + "\n";
            initLog += "--------\n";
        }
    }
    
    for(int i=0; i<phases.size(); ++i)
    {
        Phase* phase = phases[i];
        
        initLog += "---------------\n";
        initLog += ">> Phase : " + phase->mName + "\n";
        initLog += " - alpha : " + QString::number(phase->mAlpha.mX) + "\n";
        initLog += " - beta : " + QString::number(phase->mBeta.mX) + "\n";
        initLog += " - tau : " + QString::number(phase->mTau) + "\n";
    }
    
    for(int i=0; i<phasesConstraints.size(); ++i)
    {
        PhaseConstraint* constraint = phasesConstraints[i];
        
        initLog += "---------------\n";
        initLog += ">> PhaseConstraint : " + QString::number(constraint->mId) + "\n";
        initLog += " - gamma : " + QString::number(constraint->mGamma) + "\n";
    }
    return initLog;
}

void MCMCLoopMain::update()
{
    QList<Event*>& events = mModel->mEvents;
    QList<Phase*>& phases = mModel->mPhases;
    QList<PhaseConstraint*>& phasesConstraints = mModel->mPhaseConstraints;
    
    double t_min = mModel->mSettings.mTmin;
    double t_max = mModel->mSettings.mTmax;
    
    Chain& chain = mChains[mChainIndex];
    
    bool doMemo = (chain.mTotalIter % chain.mThinningInterval == 0);
    
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
    
    double delta = (chain.mBatchIndex < 10000) ? 0.01f : (1 / sqrtf(chain.mBatchIndex));
    
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
                    date.mTheta.mSigmaMH *= powf(10.f, sign * delta);
                }
            }
            
            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            
            double taux = 100.f * date.mSigma.getCurrentAcceptRate();
            if(taux <= taux_min || taux >= taux_max)
            {
                allOK = false;
                double sign = (taux <= taux_min) ? -1.f : 1.f;
                date.mSigma.mSigmaMH *= powf(10.f, sign * delta);
            }
        }
        
        //--------------------- Adapt Theta MH de Theta f -----------------------------------------
        
        if(event->mMethod == Event::eMHAdaptGauss)
        {
            double taux = 100.f * event->mTheta.getCurrentAcceptRate();
            if(taux <= taux_min || taux >= taux_max)
            {
                allOK = false;
                double sign = (taux <= taux_min) ? -1.f : 1.f;
                event->mTheta.mSigmaMH *= powf(10.f, sign * delta);
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


