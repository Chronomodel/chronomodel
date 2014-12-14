#include "MCMCLoopMain.h"

#include "Model.h"
#include "EventKnown.h"
#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "Date.h"
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
    setMCMCSettings(mModel->mMCMCSettings);
}

MCMCLoopMain::~MCMCLoopMain()
{

}

void MCMCLoopMain::calibrate()
{
    if(mModel)
    {
        QList<Event*>& events = mModel->mEvents;
        double tmin = mModel->mSettings.mTmin;
        double tmax = mModel->mSettings.mTmax;
        double step = mModel->mSettings.mStep;
        
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
            
            dates[i]->calibrate(tmin, tmax, step);
            
            emit stepProgressed(i);
            
            QTime endTime = QTime::currentTime();
            int timeDiff = startTime.msecsTo(endTime);
            mLog += "Data \"" + dates[i]->mName + "\" (" + dates[i]->mPlugin->getName() + ") calibrated in " + QString::number(timeDiff) + " ms\n";
        }
    }
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

void MCMCLoopMain::initMCMC2()
{
    QList<Event*>& events = mModel->mEvents;
    QList<Phase*>& phases = mModel->mPhases;
    QList<PhaseConstraint*>& phasesConstraints = mModel->mPhaseConstraints;
    float tmin = mModel->mSettings.mTmin;
    float tmax = mModel->mSettings.mTmax;
    
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
    //  Init theta f, ti, ...
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing events..."), 0, events.size());
    for(int i=0; i<events.size(); ++i)
    {
        float min = events[i]->getThetaMin(tmin);
        float max = events[i]->getThetaMax(tmax);
        
        events[i]->mTheta.mX = Generator::randomUniform(min, max);
        events[i]->mInitialized = true;
        
        float s02_sum = 0.f;
        for(int j=0; j<events[i]->mDates.size(); ++j)
        {
            Date& date = events[i]->mDates[j];
            
            // Init ti and its sigma
            FunctionAnalysis data = analyseFunction(date.mCalibration);
            date.mTheta.mX = map_interpolate_key_for_value(Generator::randomUniform(), date.mRepartition);
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
            float so = date.mTheta.mX - (events[i]->mTheta.mX - date.mDelta);
            date.mSigma.mX = shrinkageUniform(so * so);
            date.mSigma.mSigmaMH = 1.f;
        }
        emit stepProgressed(i);
    }
    // ----------------------------------------------------------------
    //  Init sigma i
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases..."), 0, events.size());
    for(int i=0; i<phases.size(); ++i)
    {
        Phase* phase = phases[i];
        phase->updateAll();
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Log Init
    // ----------------------------------------------------------------
    mLog += "---------------\n";
    mLog += "INIT\n";
    mLog += "---------------\n";
    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        
        if(event->type() == Event::eKnown)
        {
            mLog += ">> Bound : " + event->mName + "\n";
            mLog += "  - theta (value) : " + QString::number(event->mTheta.mX) + "\n";
            mLog += "  - theta (sigma MH) : " + QString::number(event->mTheta.mSigmaMH) + "\n";
        }
        else
        {
            mLog += ">> Event : " + event->mName + "\n";
            mLog += "  - theta (value) : " + QString::number(event->mTheta.mX) + "\n";
            mLog += "  - theta (sigma MH) : " + QString::number(event->mTheta.mSigmaMH) + "\n";
            mLog += "  - SO2 : " + QString::number(event->mS02) + "\n";
            mLog += "  - AShrinkage : " + QString::number(event->mAShrinkage) + "\n";
        }
        mLog += "---------------\n";
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            
            mLog += " > Data : " + date.mName + "\n";
            mLog += "  - theta (value) : " + QString::number(date.mTheta.mX) + "\n";
            mLog += "  - theta (sigma MH) : " + QString::number(date.mTheta.mSigmaMH) + "\n";
            mLog += "  - sigma (value) : " + QString::number(date.mSigma.mX) + "\n";
            mLog += "  - sigma (sigma MH) : " + QString::number(date.mSigma.mSigmaMH) + "\n";
            mLog += "  - delta (value) : " + QString::number(date.mDelta) + "\n";
            mLog += "--------\n";
        }
    }
    
    for(int i=0; i<phases.size(); ++i)
    {
        Phase* phase = phases[i];
        
        mLog += "---------------\n";
        mLog += ">> Phase : " + phase->mName + "\n";
        mLog += " - alpha : " + QString::number(phase->mAlpha.mX) + "\n";
        mLog += " - beta : " + QString::number(phase->mBeta.mX) + "\n";
        mLog += " - tau : " + QString::number(phase->mTau) + "\n";
    }
    
    for(int i=0; i<phasesConstraints.size(); ++i)
    {
        PhaseConstraint* constraint = phasesConstraints[i];
        
        mLog += "---------------\n";
        mLog += ">> PhaseConstraint : " + QString::number(constraint->mId) + "\n";
        mLog += " - gamma : " + QString::number(constraint->mGamma) + "\n";
    }
    qDebug() << mLog;
}

void MCMCLoopMain::initMCMC()
{
    QList<Event*>& events = mModel->mEvents;
    QList<Phase*>& phases = mModel->mPhases;
    float tmin = mModel->mSettings.mTmin;
    float tmax = mModel->mSettings.mTmax;
    
    // ----------------------------------------------------------------
    //  Thetas des Mesures
    //  mTheta = mode max de g(theta i) = fonction de calibration
    //  mSigmaThetaMH = variance de G(theta i) = vraissemblance,
    //  (différent de g(theta i) = fonction de calibration)
    // ----------------------------------------------------------------
    int numDates = 0;
    for(int i=0; i<events.size(); ++i)
        numDates += events[i]->mDates.size();
    
    emit stepChanged(tr("Initializing dates..."), 0, numDates);

    for(int i=0; i<events.size(); ++i)
    {
        for(int j=0; j<events[i]->mDates.size(); ++j)
        {
            Date& date = events[i]->mDates[j];
            
            // TODO Init mieux que ça!
            date.updateDelta(events[i]);
            
            FunctionAnalysis data = analyseFunction(date.mCalibration);
            
            date.mTheta.mX = data.mode;
            date.mTheta.mSigmaMH = data.stddev;
            
            emit stepProgressed(i);
        }
    }
    
    // ----------------------------------------------------------------
    //  Thetas des Faits
    //  mTheta = moyenne des thetas des dates.
    //  mS02 = moyenne harmonique de variances de G(theta i).
    //  Choix du shrinkage uniforme : 1
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing events..."), 0, events.size());
    for(int i=0; i<events.size(); ++i)
    {
        if(events[i]->type() == Event::eKnown)
        {
            EventKnown& ek = (EventKnown&)events[i];
            switch (ek.knownType())
            {
                case EventKnown::eFixed:
                    events[i]->mTheta.mX = ek.fixedValue();
                    break;
                case EventKnown::eUniform:
                    events[i]->mTheta.mX = ek.uniformStart() + (ek.uniformEnd() - ek.uniformStart())/2;
                    break;
                default:
                    break;
            }
            qDebug() << "Init bound : " << events[i]->mName << " : " << events[i]->mTheta.mX;
        }
        else
        {
            double theta_sum = 0;
            double s02_sum = 0;
            for(int j=0; j<events[i]->mDates.size(); ++j)
            {
                theta_sum += events[i]->mDates[j].mTheta.mX;
                
                // SO2 est la moyenne harmonique des variances sur les dates calibrées a posteriori
                // On doit donc utiliser "sigma(calib)" (variance de la date calibrée), qui a déjà servi à initialiser mTheta.mSigmaMH
                double sigmaCalib = events[i]->mDates[j].mTheta.mSigmaMH;
                s02_sum += 1 / (sigmaCalib * sigmaCalib);
            }
            
            events[i]->mS02 = events[i]->mDates.size() / s02_sum;
            events[i]->mAShrinkage = 1.;
            
            // Moyenne des theta i
            events[i]->mTheta.mX = theta_sum / events[i]->mDates.size();
            
            qDebug() << "Init event : " << events[i]->mName << " : " << events[i]->mTheta.mX;
            
            // Si la moyenne n'est pas dans le support du fait, on prend la moyenne des theta i qui s'y trouvent
            /*if(events[i].mTheta.mX < events[i].mTmin || events[i].mTheta.mX > events[i].mTmax)
             {
             theta_sum = 0;
             int counter = 0;
             for(unsigned int j=0; j<events[i].mDates.size(); ++j)
             {
             double thetaDate = events[i].mDates[j].mTheta.mX;
             if(thetaDate >= events[i].mTmin && thetaDate <= events[i].mTmax)
             {
             theta_sum += thetaDate;
             ++counter;
             }
             }
             if(counter == 0)
             {
             // Aucun theta i dans le support du fait ! => on prend le centre du support
             events[i].mTheta.mX = (events[i].mTmax - events[i].mTmin) / 2;
             }
             else
             {
             // Moyenne des theta i se trouvant dans le support
             events[i].mTheta.mX = theta_sum / counter;
             }
             }*/
            
            // Init sigmaMH de theta f
            events[i]->mTheta.mSigmaMH = sqrtf(events[i]->mS02);
        }
        
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Theta des Phases
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases..."), 0, phases.size());
    for(int i=0; i<phases.size(); ++i)
    {
        phases[i]->mAlpha.mX = Generator::randomUniform(tmin, phases[i]->getMinThetaEvents());
        phases[i]->mBeta.mX = Generator::randomUniform(phases[i]->getMaxThetaEvents(), tmax);
        
        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    // Vérifier thetas des Faits et alpha beta TODO !!!!!!
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing events with constraints ..."), 0, phases.size());
    bool verif = false;
    while(!verif)
    {
        if(isInterruptionRequested())
            return;
        
        verif = true;
        for(int i=0; i<events.size(); ++i)
        {
            Event* event = events[i];
            
            double thetaMin = event->getThetaMin(tmin);
            double thetaMax = event->getThetaMax(tmax);
            
            qDebug() << "Init constraint for : " << event->mName << " : " << thetaMin << " < " << event->mTheta.mX << " < " << thetaMax;
            
            if(thetaMin > thetaMax)
            {
                qDebug() << "No init possible!";
            }
            
            if(event->mTheta.mX <= thetaMin || event->mTheta.mX >= thetaMax)
            {
                verif = false;
                double theta = thetaMin + (thetaMax - thetaMin) * Generator::randomUniform();
                event->mTheta.mX = theta;
                
                qDebug() << "Corrigé : " << thetaMin << " < " << event->mTheta.mX << " < " << thetaMax;
            }
        }
    }
    
    // ----------------------------------------------------------------
    //  Variance des Mesures
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing dates variances..."), 0, numDates);
    
    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            
            double diff = abs(date.mTheta.mX - event->mTheta.mX);
            if(diff != 0)
                date.mSigma.mX = diff;
            else
                date.mSigma.mX = date.mTheta.mSigmaMH;
            
            date.mSigma.mSigmaMH = 1.;
            
            emit stepProgressed(i);
        }
    }
    // ----------------------------------------------------------------
    
    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        
        if(event->type() == Event::eKnown)
        {
            mLog += ">> Bound : " + event->mName + "\n";
            mLog += "  - theta (value) : " + QString::number(event->mTheta.mX) + "\n";
            mLog += "  - theta (sigma MH) : " + QString::number(event->mTheta.mSigmaMH) + "\n";
        }
        else
        {
            mLog += ">> Event : " + event->mName + "\n";
            mLog += "  - theta (value) : " + QString::number(event->mTheta.mX) + "\n";
            mLog += "  - theta (sigma MH) : " + QString::number(event->mTheta.mSigmaMH) + "\n";
            mLog += "  - SO2 : " + QString::number(event->mS02) + "\n";
            mLog += "  - AShrinkage : " + QString::number(event->mAShrinkage) + "\n";
        }
        mLog += "---------------\n";
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            
            mLog += " > Data : " + date.mName + "\n";
            mLog += "  - theta (value) : " + QString::number(date.mTheta.mX) + "\n";
            mLog += "  - theta (sigma MH) : " + QString::number(date.mTheta.mSigmaMH) + "\n";
            mLog += "  - sigma (value) : " + QString::number(date.mSigma.mX) + "\n";
            mLog += "  - sigma (sigma MH) : " + QString::number(date.mSigma.mSigmaMH) + "\n";
            mLog += "  - delta (value) : " + QString::number(date.mDelta) + "\n";
            mLog += "--------\n";
        }
    }
    
    for(int i=0; i<phases.size(); ++i)
    {
        Phase* phase = phases[i];
        
        mLog += "---------------\n";
        mLog += ">> Phase : " + phase->mName + "\n";
        mLog += " - alpha : " + QString::number(phase->mAlpha.mX) + "\n";
        mLog += " - beta : " + QString::number(phase->mBeta.mX) + "\n";
        mLog += " - tau : " + QString::number(phase->mTau) + "\n";
    }
    //qDebug() << mLog;
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
            date.updateTheta(t_min, t_max, event);
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
        phases[i]->updateAll();
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
    
    const double taux_min = 42.;           // taux_min minimal rate of acceptation=42
    const double taux_max = 46.;           // taux_max maximal rate of acceptation=46
    
    bool allOK = true;
    
    //--------------------- Adapt -----------------------------------------
    
    float delta = (chain.mBatchIndex < 10000) ? 0.01f : (1 / sqrtf(chain.mBatchIndex));
    
    for(int i=0; i<events.size(); ++i)
    {
        Event* event = events[i];
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            
            //--------------------- Adapt Sigma MH de Theta i -----------------------------------------
            
            if(date.mMethod == Date::eMHSymGaussAdapt)
            {
                float taux = 100.f * date.mTheta.getCurrentAcceptRate();
                if(taux <= taux_min || taux >= taux_max)
                {
                    allOK = false;
                    float sign = (taux <= taux_min) ? -1.f : 1.f;
                    date.mTheta.mSigmaMH *= powf(10.f, sign * delta);
                }
            }
            
            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            
            float taux = 100.f * date.mSigma.getCurrentAcceptRate();
            if(taux <= taux_min || taux >= taux_max)
            {
                allOK = false;
                float sign = (taux <= taux_min) ? -1.f : 1.f;
                date.mSigma.mSigmaMH *= powf(10.f, sign * delta);
            }
        }
        
        //--------------------- Adapt Theta MH de Theta f -----------------------------------------
        
        if(event->mMethod == Event::eMHAdaptGauss)
        {
            float taux = 100.f * event->mTheta.getCurrentAcceptRate();
            if(taux <= taux_min || taux >= taux_max)
            {
                allOK = false;
                float sign = (taux <= taux_min) ? -1.f : 1.f;
                event->mTheta.mSigmaMH *= powf(10.f, sign * delta);
            }
        }
    }
    return allOK;
}

void MCMCLoopMain::finalize()
{
    mModel->generateCorrelations(mChains);
    mModel->generatePosteriorDensities(mChains, mModel->mMCMCSettings.mFFTLength);
    mModel->generateNumericalResults(mChains);
}


