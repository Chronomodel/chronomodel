#include "PluginAMLoop.h"

#include "PluginAM.h"
#include "Date.h"
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
#include <QTime>


PluginAMLoop::PluginAMLoop(const Date* date):MCMCLoop(),
mDate(date),
mSo2(0)
{
    PluginAM* plugin = (PluginAM*) mDate->mPlugin;
    setMCMCSettings(plugin->mMCMCSettings);
}

PluginAMLoop::~PluginAMLoop()
{
    
}

QString PluginAMLoop::calibrate()
{
    return QString();
}

void PluginAMLoop::initVariablesForChain()
{
    if(mChains.count() > 0)
    {
        const int acceptBufferLen = mChains[0].mNumBatchIter;
        long int initReserve = 0;
        
        for (const ChainSpecs c: mChains){
            initReserve +=( 1 + (c.mMaxBatchs*c.mNumBatchIter) + c.mNumBurnIter + (c.mNumRunIter/c.mThinningInterval) );
        }
        
        mSigma.reset();
        mSigma.reserve(initReserve);
        mSigma.mLastAccepts.reserve(acceptBufferLen);
        mSigma.mLastAcceptsLength = acceptBufferLen;
    }
}

QString PluginAMLoop::initMCMC()
{
    emit stepChanged(tr("Initializing data for MCMC calibration..."), 0, 1);
    emit stepProgressed(0);
    
    if(isInterruptionRequested())
        return ABORTED_BY_USER;
    
    Q_ASSERT(mDate);
    
    QJsonObject data = mDate->mData;
    QString mode = data.value(DATE_AM_MODE).toString();
    //double i = data.value(DATE_AM_I).toDouble();
    //double d = data.value(DATE_AM_D).toDouble();
    double f = data.value(DATE_AM_F).toDouble();
    double alpha95 = data.value(DATE_AM_ALPHA_95).toDouble();
    double sigmaF = data.value(DATE_AM_SIGMA_F).toDouble();
    
    if(mode == DATE_AM_MODE_ID){
        long double si = alpha95 / 2.4481l;
        mSo2 = si * si;
    }
    else if(mode == DATE_AM_MODE_IF){
        long double si = alpha95 / 2.4481l;
        long double sf = sigmaF;
        mSo2 = 0.5l * (sf * sf + si * si * f * f * powf(M_PI / 180.l, 2.l));
    }
    else if(mode == DATE_AM_MODE_IDF){
        long double si = alpha95 / 2.4481l;
        long double sf = sigmaF;
        mSo2 = (1.l / 3.l) * (sf * sf + 2 * si * si * f * f * powf(M_PI / 180.l, 2.l));
    }
    
    mSigma.mX = shrinkageUniform(mSo2);
    mSigma.memo();
    mSigma.mLastAccepts.clear();
    
    emit stepProgressed(1);
    
    return QString();
}

void PluginAMLoop::update()
{
    
    //mSigma.tryUpdate(<#const double x#>, <#const double rapportToTry#>)
    mSigma.memo();
    mSigma.saveCurrentAcceptRate();
}

bool PluginAMLoop::adapt()
{
    return true;
}

void PluginAMLoop::finalize()
{
    //mSigma.generateHistos(mChains, 1024, 1);
}


