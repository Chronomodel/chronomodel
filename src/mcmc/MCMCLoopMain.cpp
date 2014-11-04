#include "MCMCLoopMain.h"

#include "Model.h"
#include "EventKnown.h"
#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"

#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>


MCMCLoopMain::MCMCLoopMain(Model& model):MCMCLoop(),
mModel(model)
{
    mSettings = mModel.mMCMCSettings;
}

MCMCLoopMain::~MCMCLoopMain()
{

}

void MCMCLoopMain::initModel()
{
    
    for(int i=0; i<mModel.mEvents.size(); ++i)
    {
        mModel.mEvents[i].reset();
        for(int j=0; j<(int)mModel.mEvents[i].mDates.size(); ++j)
        {
            mModel.mEvents[i].mDates[j].reset();
        }
    }
}

void MCMCLoopMain::calibrate()
{
    QList<Event>& events = mModel.mEvents;
    double tmin = mModel.mSettings.mTmin;
    double tmax = mModel.mSettings.mTmax;
    double step = mModel.mSettings.mStep;
    
    //----------------- Calibrate measures --------------------------------------
    
    QList<Date*> dates;
    for(int i=0; i<events.size(); ++i)
    {
        int num_dates = (int)events[i].mDates.size();
        for(int j=0; j<num_dates; ++j)
        {
            Date* date = &events[i].mDates[j];
            dates.push_back(date);
        }
    }
    
    emit stepChanged(tr("Calibrating..."), 0, dates.size());
    
    for(int i=0; i<dates.size(); ++i)
    {
        dates[i]->calibrate(tmin, tmax, step);
        emit stepProgressed(i);
    }
}

void MCMCLoopMain::initMCMC()
{
    QList<Event>& events = mModel.mEvents;
    QList<Phase>& phases = mModel.mPhases;
    double t_min = mModel.mSettings.mTmin;
    double t_max = mModel.mSettings.mTmax;
    
    // ----------------------------------------------------------------
    //  Thetas des Mesures
    //  mTheta = mode max de g(theta i) = fonction de calibration
    //  mSigmaThetaMH = variance de G(theta i) = vraissemblance,
    //  (différent de g(theta i) = fonction de calibration)
    // ----------------------------------------------------------------
    int numDates = 0;
    for(int i=0; i<events.size(); ++i)
        numDates += events[i].mDates.size();
    
    emit stepChanged(tr("Initializing dates..."), 0, numDates);

    for(int i=0; i<events.size(); ++i)
    {
        for(int j=0; j<events[i].mDates.size(); ++j)
        {
            Date& date = events[i].mDates[j];
            
            // TODO Init mieux que ça!
            date.updateDelta();
            
            FunctionAnalysis data = analyseFunction(date.mCalibration);
            
            date.mTheta.mX = data.mode;
            date.mTheta.mSigmaMH = sqrt(data.variance);
            
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
        if(events[i].type() == Event::eKnown)
        {
            EventKnown& ek = (EventKnown&)events[i];
            switch (ek.knownType())
            {
                case EventKnown::eFixed:
                    events[i].mTheta.mX = ek.fixedValue();
                    break;
                case EventKnown::eUniform:
                    events[i].mTheta.mX = ek.uniformStart() + (ek.uniformEnd() - ek.uniformStart())/2;
                    break;
                case EventKnown::eGauss:
                    events[i].mTheta.mX = ek.gaussMeasure();
                    break;
                default:
                    break;
            }
        }
        else
        {
            double theta_sum = 0;
            double s02_sum = 0;
            for(int j=0; j<events[i].mDates.size(); ++j)
            {
                theta_sum += events[i].mDates[j].mTheta.mX;
                
                // SO2 est la moyenne harmonique des variances sur les dates calibrées a posteriori
                // On doit donc utiliser "sigma(calib)" (variance de la date calibrée), qui a déjà servi à initialiser mTheta.mSigmaMH
                double sigmaCalib = events[i].mDates[j].mTheta.mSigmaMH;
                s02_sum += 1 / (sigmaCalib * sigmaCalib);
            }
            
            events[i].mS02 = events[i].mDates.size() / s02_sum;
            events[i].mAShrinkage = 1.;
            
            // Moyenne des theta i
            events[i].mTheta.mX = theta_sum / events[i].mDates.size();
            
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
            events[i].mTheta.mSigmaMH = sqrt(events[i].mS02);
        }
        
        emit stepProgressed(i);
        
        qDebug() << QString::number(i+1) + "/" + QString::number(events.size()) + " (" + events[i].mName + ") theta = " + QString::number(events[i].mTheta.mX) + ", SO2 = " + QString::number(events[i].mS02)+ ", AShrinkage = " + QString::number(events[i].mAShrinkage);
    }
    
    // ----------------------------------------------------------------
    //  Theta des Phases
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing phases..."), 0, phases.size());
    for(int i=0; i<phases.size(); ++i)
    {
        phases[i].mAlpha.mX = Generator::randomUniform(t_min, phases[i].getMinThetaEvents());
        phases[i].mBeta.mX = Generator::randomUniform(phases[i].getMaxThetaEvents(), t_max);
        
        emit stepProgressed(i);
        
        qDebug() << QString::number(i+1) + "/" + QString::number(phases.size()) + " (" + phases[i].mName + ") alpha = " + QString::number(phases[i].mAlpha.mX) + ", beta = " + QString::number(phases[i].mBeta.mX);
    }
    
    // ----------------------------------------------------------------
    // Vérifier thetas des Faits et alpha beta TODO !!!!!!
    // ----------------------------------------------------------------
    bool verif = false;
    while(!verif)
    {
        verif = true;
        for(int i=0; i<events.size(); ++i)
        {
            // ----------------------------------------------------------------
            //  start = max des theta f en contrainte avant en tenant compte de leur phi éventuel.
            //  Ces thetas peuvent etre dans des phases en contrainte avec la phase à laquelle on appartient
            //  et aussi avoir un gap.
            //  end : pareil de l'autre coté.
            // ----------------------------------------------------------------
            // Faux:
            //double start = max(events[i].getMaxThetaOfStartEvents(t_min), events[i].getMaxThetaBeginOfPhases(t_min));
            //double end = min(events[i].getMinThetaOfEndEvents(t_max), events[i].getMinThetaEndOfPhases(t_max));
            
            Event& event = events[i];
            
            double start = event.getMaxEventThetaBackward(t_min);
            double end = event.getMinEventThetaForward(t_max);
            
            if(event.mTheta.mX <= start || event.mTheta.mX >= end)
            {
                verif = false;
                double theta = (start + end) / 2; // random_uniform mieux pour s'en sortir !!!
                event.mTheta.mX = theta;
            }
        }
    }
    
    // ----------------------------------------------------------------
    //  Variance des Mesures
    // ----------------------------------------------------------------
    emit stepChanged(tr("Initializing dates variances..."), 0, numDates);
    
    for(int i=0; i<events.size(); ++i)
    {
        Event& event = events[i];
        for(int j=0; j<event.mDates.size(); ++j)
        {
            Date& date = event.mDates[j];
            
            double diff = abs(date.mTheta.mX - event.mTheta.mX);
            if(diff != 0)
                date.mSigma.mX = diff;
            else
                date.mSigma.mX = date.mTheta.mSigmaMH;
            
            date.mSigma.mSigmaMH = 1.;
            
            emit stepProgressed(i);
            
            qDebug() << QString::number(j+1) + "/" + QString::number(numDates) + " (" + date.mName + ") sigma = " + QString::number(date.mSigma.mX) + ", sigmaMH(sigma) = " + QString::number(date.mSigma.mSigmaMH);
        }
    }
}

void MCMCLoopMain::update()
{
    QList<Event>& events = mModel.mEvents;
    QList<Phase>& phases = mModel.mPhases;
    double t_min = mModel.mSettings.mTmin;
    double t_max = mModel.mSettings.mTmax;

    bool doMemo = false;
    if(mState == eRunning)
    {
        // Using a down-sampling eventor :
        //doMemo = (mIterIndex % mSettings.mDownSamplingFactor == 0);
        
        // Using num of iters to keep :
        int step = 1;
        if(mSettings.mDownSamplingFactor < mSettings.mNumRunIter)
            step = floor(mSettings.mNumRunIter / mSettings.mDownSamplingFactor);
        
        doMemo = (mRunIterIndex % step == 0);
    }
    
    doMemo = true;
    
    
    //--------------------- Update Dates -----------------------------------------
    
    for(int i=0; i<events.size(); ++i)
    {
        Event& event = events[i];
        for(int j=0; j<event.mDates.size(); ++j)
        {
            Date& date = events[i].mDates[j];
            
            date.updateDelta();
            if(doMemo)
                date.mDelta.memo();
            
            date.updateTheta(t_min, t_max, event);
            if(doMemo)
                date.mTheta.memo();

            date.updateSigma(event);
            if(doMemo)
                date.mSigma.memo();
            
            // Calculate Acceptation Rate :
            // TODO : calculer le taux sur les 100 dernières iterations minimum
            
            double taux = 100 * date.mTheta.mAcceptMHTotal / (mTotalIter + 1);
            date.mTheta.mHistoryAcceptRateMH.push_back(taux);
            date.mTheta.mHistorySigmaMH.push_back(date.mTheta.mSigmaMH);
            
            //qDebug() << "iter : " << (mTotalIter + 1) << ", num accept : " << date.mTheta.mAcceptMHTotal << ", taux : " << taux;
            
            taux = 100 * date.mSigma.mAcceptMHTotal / (mTotalIter + 1);
            date.mSigma.mHistoryAcceptRateMH.push_back(taux);
            date.mSigma.mHistorySigmaMH.push_back(date.mSigma.mSigmaMH);
        }
    }

    //--------------------- Update Events -----------------------------------------

    for(int i=0; i<events.size(); ++i)
    {
        events[i].updateTheta(t_min, t_max);
        if(doMemo)
            events[i].mTheta.memo();
        
        // Calculate Acceptation Rate :
        double taux = 100 * events[i].mTheta.mAcceptMHTotal / (mTotalIter + 1);
        events[i].mTheta.mHistoryAcceptRateMH.push_back(taux);
        events[i].mTheta.mHistorySigmaMH.push_back(events[i].mTheta.mSigmaMH);
    }

    //--------------------- Update Phases -----------------------------------------

    for(int i=0; i<phases.size(); ++i)
    {
        phases[i].update(t_min, t_max);
        if(doMemo)
            phases[i].memoAll();
    }
}

bool MCMCLoopMain::adapt()
{
    QList<Event>& events = mModel.mEvents;
    
    const double taux_min = 42.;           // taux_min minimal rate of acceptation=42
    const double taux_max = 46.;           // taux_max maximal rate of acceptation=46
    
    bool allOK = true;
    
    //--------------------- Adapt -----------------------------------------
    
    for(int i=0; i<events.size(); ++i)
    {
        Event& event = events[i];
        double delta = (mBatchIndex < 10000) ? 0.01 : (1 / sqrt(mBatchIndex));
        
        for(int j=0; j<event.mDates.size(); ++j)
        {
            Date& date = event.mDates[j];
            
            //--------------------- Adapt Sigma MH de Theta i -----------------------------------------
            
            if(date.mMethod == Date::eMHSymGaussAdapt)
            {
                double taux = 100 * date.mTheta.mAcceptMHBatch / mModel.mMCMCSettings.mIterPerBatch;
                if(taux <= taux_min || taux >= taux_max)
                {
                    allOK = false;
                    double sign = (taux <= taux_min) ? -1 : 1;
                    date.mTheta.mSigmaMH *= pow(10, sign * delta);
                }
                date.mTheta.mAcceptMHBatch = 0;
            }
            
            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            
            double taux = 100 * date.mSigma.mAcceptMHBatch / mModel.mMCMCSettings.mIterPerBatch;
            if(taux <= taux_min || taux >= taux_max)
            {
                allOK = false;
                double sign = (taux <= taux_min) ? -1 : 1;
                date.mSigma.mSigmaMH *= pow(10, sign * delta);
            }
            date.mSigma.mAcceptMHBatch = 0;
        }
        
        //--------------------- Adapt Theta MH de Theta f -----------------------------------------
        
        if(event.mMethod == Event::eMHAdaptGauss)
        {
            double taux = 100 * event.mTheta.mAcceptMHBatch / mModel.mMCMCSettings.mIterPerBatch;
            if(taux <= taux_min || taux >= taux_max)
            {
                allOK = false;
                double sign = (taux <= taux_min) ? -1 : 1;
                event.mTheta.mSigmaMH *= pow(10, sign * delta);
            }
            event.mTheta.mAcceptMHBatch = 0;
        }
    }
    return allOK;
}

void MCMCLoopMain::finalize()
{
    // TODO Generate HPD here ?
    
    mModel.mMCMCSettings.mFinalBatchIndex = mFinalBatchIndex;
    
    float tmin = mModel.mSettings.mTmin;
    float tmax = mModel.mSettings.mTmax;
    
    QList<Event>& events = mModel.mEvents;
    QList<Phase>& phases = mModel.mPhases;
    
    for(int i=0; i<events.size(); ++i)
    {
        Event& event = events[i];
        
        for(int j=0; j<event.mDates.size(); ++j)
        {
            Date& date = event.mDates[j];
            
            date.mTheta.generateFullHisto(tmin, tmax);
            date.mSigma.generateFullHisto(tmin, tmax);
            date.mDelta.generateFullHisto(tmin, tmax);
            
            date.mTheta.generateHistos(mSettings.mNumProcesses, tmin, tmax);
            date.mSigma.generateHistos(mSettings.mNumProcesses, tmin, tmax);
            date.mDelta.generateHistos(mSettings.mNumProcesses, tmin, tmax);
            
            FunctionAnalysis data = analyseFunction(date.mTheta.fullHisto());
            date.mTheta.mHistoMode = data.mode;
            date.mTheta.mHistoMean = data.mean;
            date.mTheta.mHistoVariance = data.variance;
        }
        
        event.mTheta.generateFullHisto(tmin, tmax);
        event.mTheta.generateHistos(mSettings.mNumProcesses, tmin, tmax);
        
        FunctionAnalysis data = analyseFunction(event.mTheta.fullHisto());
        event.mTheta.mHistoMode = data.mode;
        event.mTheta.mHistoMean = data.mean;
        event.mTheta.mHistoVariance = data.variance;
    }
    
    for(int i=0; i<phases.size(); ++i)
    {
        Phase& phase = phases[i];
        
        phase.mAlpha.generateFullHisto(tmin, tmax);
        phase.mBeta.generateFullHisto(tmin, tmax);
        phase.mThetaPredict.generateFullHisto(tmin, tmax);
        
        phase.mAlpha.generateHistos(mSettings.mNumProcesses, tmin, tmax);
        phase.mBeta.generateHistos(mSettings.mNumProcesses, tmin, tmax);
        phase.mThetaPredict.generateHistos(mSettings.mNumProcesses, tmin, tmax);
    }
}


