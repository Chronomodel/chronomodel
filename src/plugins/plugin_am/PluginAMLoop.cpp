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
        
        mSigma2.reset();
        mSigma2.reserve(initReserve);
        mSigma2.mLastAccepts.reserve(acceptBufferLen);
        mSigma2.mLastAcceptsLength = acceptBufferLen;
        
        mT.reset();
        mT.reserve(initReserve);
        mT.mLastAccepts.reserve(acceptBufferLen);
        mT.mLastAcceptsLength = acceptBufferLen;
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
        mSi = alpha95 / 2.4481l;
        mSo2 = mSi * mSi;
    }
    else if(mode == DATE_AM_MODE_IF){
        long double mSi = alpha95 / 2.4481l;
        long double mSf = sigmaF;
        mSo2 = 0.5l * (mSf * mSf + mSi * mSi * f * f * powf(M_PI / 180.l, 2.l));
    }
    else if(mode == DATE_AM_MODE_IDF){
        long double mSi = alpha95 / 2.4481l;
        long double mSf = sigmaF;
        mSo2 = (1.l / 3.l) * (mSf * mSf + 2 * mSi * mSi * f * f * powf(M_PI / 180.l, 2.l));
    }
    
    mSigma2.mX = shrinkageUniform(mSo2);
    mSigma2.memo();
    mSigma2.mLastAccepts.clear();
    
    mT.mX = mDate->getTminRefCurve() + (mDate->getTmaxRefCurve() - mDate->getTminRefCurve()) / 2.;
    mT.memo();
    mT.mLastAccepts.clear();
    
    emit stepProgressed(1);
    
    return QString();
}

void PluginAMLoop::update()
{
    Q_ASSERT(mDate);
    PluginAM* plugin = (PluginAM*) mDate->mPlugin;
    
    // -------------------------------------------------------------------------
    //  Read Date params
    // -------------------------------------------------------------------------
    QJsonObject data = mDate->mData;
    QString mode = data.value(DATE_AM_MODE).toString();
    double i = data.value(DATE_AM_I).toDouble();
    double d = data.value(DATE_AM_D).toDouble();
    double f = data.value(DATE_AM_F).toDouble();
    double alpha95 = data.value(DATE_AM_ALPHA_95).toDouble();
    double sigmaF = data.value(DATE_AM_SIGMA_F).toDouble();
    QString curveI = data.value(DATE_AM_CURVE_I).toString().toLower();
    QString curveD = data.value(DATE_AM_CURVE_D).toString().toLower();
    QString curveF = data.value(DATE_AM_CURVE_F).toString().toLower();
    
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage t
    // ------------------------------------------------------------------------------------------
    
    
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage sigma :  MH avec marcheur gaussien adaptatif sur le log de vi
    // ------------------------------------------------------------------------------------------
    const int logVMin = -6;
    const int logVMax = 100;
    
    const double V1 = mSigma2.mX * mSigma2.mX;
    const double logV2 = Generator::gaussByBoxMuller(log10(V1), mSigma2.mSigmaMH);
    const double sigma2_new = pow(10, logV2);
    double rapport = 0;
    
    if(logV2 >= logVMin && logV2 <= logVMax)
    {
        long double likelihood = 0.l;
        long double likelihood_new = 0.l;
        
        if(mode == DATE_AM_MODE_ID)
        {
            long double git = plugin->getRefCurveValueAt(curveI, mT.mX);
            long double sigma_git = plugin->getRefCurveErrorAt(curveI, mT.mX);
            
            long double gdt = plugin->getRefCurveValueAt(curveD, mT.mX);
            long double sigma_gdt = plugin->getRefCurveErrorAt(curveD, mT.mX);
            
            long double vi = mSi * mSi + mSigma2.mX + sigma_git * sigma_git;
            long double vd = mSd * mSd + mSigma2.mX / cosl(i * M_PI / 180.l) + sigma_gdt * sigma_gdt;
            
            long double wi = 1 / vi;
            long double wd = 1 / vd;
            
            likelihood = sqrtl(wi) * expl(-0.5l * wi * (i - git) * (i - git)) *
            sqrtl(wd) * expl(-0.5l * wd * (d - gdt) * (d - gdt)) *
            mSo2 / powl(mSigma2.mX + mSo2, 2.l);
            
            long double vi_new = mSi * mSi + sigma2_new + sigma_git * sigma_git;
            long double vd_new = mSd * mSd + sigma2_new / cosl(i * M_PI / 180.l) + sigma_gdt * sigma_gdt;
            
            long double wi_new = 1 / vi_new;
            long double wd_new = 1 / vd_new;
            
            likelihood_new = sqrtl(wi_new) * expl(-0.5l * wi_new * (i - git) * (i - git)) *
            sqrtl(wd_new) * expl(-0.5l * wd_new * (d - gdt) * (d - gdt)) *
            mSo2 / powl(mSigma2.mX + mSo2, 2.l);
        }
        else if(mode == DATE_AM_MODE_IF)
        {
            long double git = plugin->getRefCurveValueAt(curveI, mT.mX);
            long double sigma_git = plugin->getRefCurveErrorAt(curveI, mT.mX);
            
            long double gft = plugin->getRefCurveValueAt(curveF, mT.mX);
            long double sigma_gft = plugin->getRefCurveErrorAt(curveF, mT.mX);
            
            long double vi = mSi * mSi + mSigma2.mX / (f * f) + sigma_git * sigma_git;
            long double vf = mSf * mSf + mSigma2.mX + sigma_gft * sigma_gft;
            
            long double wi = 1 / vi;
            long double wf = 1 / vf;
            
            likelihood = sqrtl(wi) * expl(-0.5l * wi * (i - git) * (i - git)) *
            sqrtl(wf) * expl(-0.5l * wf * (f - gft) * (f - gft)) *
            mSo2 / powl(mSigma2.mX + mSo2, 2.l);
            
            long double vi_new = mSi * mSi + sigma2_new / (f * f) + sigma_git * sigma_git;
            long double vf_new = mSf * mSf + sigma2_new + sigma_gft * sigma_gft;
            
            long double wi_new = 1 / vi_new;
            long double wf_new = 1 / vf_new;
            
            likelihood_new = sqrtl(wi_new) * expl(-0.5l * wi_new * (i - git) * (i - git)) *
            sqrtl(wf_new) * expl(-0.5l * wf_new * (f - gft) * (f - gft)) *
            mSo2 / powl(mSigma2.mX + mSo2, 2.l);
        }
        else if(mode == DATE_AM_MODE_IDF)
        {
            long double git = plugin->getRefCurveValueAt(curveI, mT.mX);
            long double sigma_git = plugin->getRefCurveErrorAt(curveI, mT.mX);
            
            long double gdt = plugin->getRefCurveValueAt(curveD, mT.mX);
            long double sigma_gdt = plugin->getRefCurveErrorAt(curveD, mT.mX);
            
            long double gft = plugin->getRefCurveValueAt(curveF, mT.mX);
            long double sigma_gft = plugin->getRefCurveErrorAt(curveF, mT.mX);
            
            long double vi = mSi * mSi + mSigma2.mX / (f * f) + sigma_git * sigma_git;
            long double vd = mSd * mSd + mSigma2.mX / (f * f * cosl(i * M_PI / 180.l)) + sigma_gdt * sigma_gdt;
            long double vf = mSf * mSf + mSigma2.mX + sigma_gft * sigma_gft;
            
            long double wi = 1 / vi;
            long double wd = 1 / vd;
            long double wf = 1 / vf;
            
            likelihood = sqrtl(wi) * expl(-0.5l * wi * (i - git) * (i - git)) *
            sqrtl(wd) * expl(-0.5l * wd * (d - gdt) * (d - gdt)) *
            sqrtl(wf) * expl(-0.5l * wf * (f - gft) * (f - gft)) *
            mSo2 / powl(mSigma2.mX + mSo2, 2.l);
            
            long double vi_new = mSi * mSi + sigma2_new / (f * f) + sigma_git * sigma_git;
            long double vd_new = mSd * mSd + sigma2_new / (f * f * cosl(i * M_PI / 180.l)) + sigma_gdt * sigma_gdt;
            long double vf_new = mSf * mSf + sigma2_new + sigma_gft * sigma_gft;
            
            long double wi_new = 1 / vi_new;
            long double wd_new = 1 / vd_new;
            long double wf_new = 1 / vf_new;
            
            likelihood_new = sqrtl(wi_new) * expl(-0.5l * wi_new * (i - git) * (i - git)) *
            sqrtl(wd_new) * expl(-0.5l * wd_new * (d - gdt) * (d - gdt)) *
            sqrtl(wf_new) * expl(-0.5l * wf_new * (f - gft) * (f - gft)) *
            mSo2 / powl(mSigma2.mX + mSo2, 2.l);
        }
        
        rapport = (likelihood_new / likelihood) * (sigma2_new / mSigma2.mX);
    }
    mSigma2.tryUpdate(sigma2_new, rapport);
    mSigma2.memo();
    mSigma2.saveCurrentAcceptRate();
}

bool PluginAMLoop::adapt()
{
    ChainSpecs& chain = mChains[mChainIndex];
    
    const double taux_min = 41.;           // taux_min minimal rate of acceptation=42
    const double taux_max = 47.;           // taux_max maximal rate of acceptation=46
    
    bool allOK = true;
    double delta = (chain.mBatchIndex < 10000) ? 0.01 : (1. / sqrt(chain.mBatchIndex));
    
    const double taux = 100. * mSigma2.getCurrentAcceptRate();
    if (taux <= taux_min || taux >= taux_max) {
        allOK = false;
        const double sign = (taux <= taux_min) ? -1. : 1.;
        mSigma2.mSigmaMH *= pow(10., sign * delta);
    }
    
    return allOK;
}

void PluginAMLoop::finalize()
{
    //mSigma.generateHistos(mChains, 1024, 1);
}


