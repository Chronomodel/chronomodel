#include "PluginAMLoop.h"

#include "PluginAM.h"
#include "Date.h"
#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "CalibrationCurve.h"

#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QTime>


PluginAMLoop::PluginAMLoop(const Date* date, const ProjectSettings& settings):MCMCLoop(),
mDate(date),
mSettings(settings),
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
    
    // -----------------------------------------------------------------------
    //  Init mSigma2
    // -----------------------------------------------------------------------
    mSigma2.mX = shrinkageUniform(mSo2);
    mSigma2.memo();
    mSigma2.mLastAccepts.clear();
    
    
    // -----------------------------------------------------------------------
    //  Init mT
    // -----------------------------------------------------------------------
    double tminRefCurve = mDate->getTminRefCurve();
    double tmaxRefCurve = mDate->getTmaxRefCurve();
    double tminCal;
    double tmaxCal;
    
    if (tmaxRefCurve > tminRefCurve)
    {
        QVector<double> calibrationTemp;
        QVector<double> repartitionTemp;
        
        const double nbRefPts = 1. + round((tmaxRefCurve - tminRefCurve) / (double)mSettings.mStep);
        PluginAM* plugin = (PluginAM*)mDate->mPlugin;
        
        long double v = 0;
        if(mode == DATE_AM_MODE_ID){
            v = plugin->getLikelihoodForMode(tminRefCurve, data, DATE_AM_MODE_I) +
                plugin->getLikelihoodForMode(tminRefCurve, data, DATE_AM_MODE_D);
        }
        else if(mode == DATE_AM_MODE_IF){
            v = plugin->getLikelihoodForMode(tminRefCurve, data, DATE_AM_MODE_I) +
                plugin->getLikelihoodForMode(tminRefCurve, data, DATE_AM_MODE_F);
        }
        else if(mode == DATE_AM_MODE_IDF){
            v = plugin->getLikelihoodForMode(tminRefCurve, data, DATE_AM_MODE_I) +
                plugin->getLikelihoodForMode(tminRefCurve, data, DATE_AM_MODE_D) +
                plugin->getLikelihoodForMode(tminRefCurve, data, DATE_AM_MODE_F);
        }
        
        calibrationTemp.append(v);
        repartitionTemp.append(0);
        long double lastRepVal = v;
        
        // we use long double type because
        // after several sums, the repartion can be in the double type range
        for(int i = 1; i < nbRefPts; ++i) {
            const double t = tminRefCurve + (double)i * (double)mSettings.mStep;
            long double lastV = v;
            
            if(mode == DATE_AM_MODE_ID){
                v = plugin->getLikelihoodForMode(t, data, DATE_AM_MODE_I) +
                plugin->getLikelihoodForMode(t, data, DATE_AM_MODE_D);
            }
            else if(mode == DATE_AM_MODE_IF){
                v = plugin->getLikelihoodForMode(t, data, DATE_AM_MODE_I) +
                plugin->getLikelihoodForMode(t, data, DATE_AM_MODE_F);
            }
            else if(mode == DATE_AM_MODE_IDF){
                v = plugin->getLikelihoodForMode(t, data, DATE_AM_MODE_I) +
                plugin->getLikelihoodForMode(t, data, DATE_AM_MODE_D) +
                plugin->getLikelihoodForMode(t, data, DATE_AM_MODE_F);
            }
            
            calibrationTemp.append(v);
            long double rep = lastRepVal;
            if(v != 0. && lastV != 0.)
                rep = lastRepVal + (long double) mSettings.mStep * (lastV + v) / 2.;
            
            repartitionTemp.append((double)rep);
            lastRepVal = rep;
        }
        
        // ------------------------------------------------------------------
        //  Restrict the calib and repartition vectors to where data are
        // ------------------------------------------------------------------
        if (repartitionTemp.last() > 0.) {
            const double threshold = 0.00005;
            const int minIdx = (int)floor(vector_interpolate_idx_for_value(threshold * lastRepVal, repartitionTemp));
            const int maxIdx = (int)ceil(vector_interpolate_idx_for_value((1 - threshold) * lastRepVal, repartitionTemp));
            
            tminCal = tminRefCurve + minIdx * mSettings.mStep;
            tmaxCal = tminRefCurve + maxIdx * mSettings.mStep;
            
            // Truncate both functions where data live
            mDate->mCalibration->mCurve = calibrationTemp.mid(minIdx, (maxIdx - minIdx) + 1);
            mDate->mCalibration->mRepartition = repartitionTemp.mid(minIdx, (maxIdx - minIdx) + 1);
            
            // NOTE ABOUT THIS APPROMIATION :
            // By truncating the calib and repartition, the calib density's area is not 1 anymore!
            // It is now 1 - 2*threshold = 0,99999... We consider it to be 1 anyway!
            // By doing this, calib and repartition are stored on a restricted number of data
            // instead of storing them on the whole reference curve's period (as done for calibrationTemp & repartitionTemp above).
            
            // Stretch repartition curve so it goes from 0 to 1
            mDate->mCalibration->mRepartition = stretch_vector(mDate->mCalibration->mRepartition, 0., 1.);
            
            // Approximation : even if the calib has been truncated, we consider its area to be = 1
            mDate->mCalibration->mCurve = equal_areas(mDate->mCalibration->mCurve, mSettings.mStep, 1.);
            
        }
        // ------------------------------------------------------------------
        //  Measure is very far from Ref curve on the whole ref curve preriod!
        //  => Calib values are very small, considered as being 0 even using "double" !
        //  => lastRepVal = 0, and impossible to truncate using it....
        //  => So,
        // ------------------------------------------------------------------
        else  {
            tminCal = tminRefCurve;
            tmaxCal = tmaxRefCurve;
        }
        
        mDate->mCalibration->mTmin = tminCal;
        mDate->mCalibration->mTmax = tmaxCal;
    }
    
    double u = Generator::randomUniform();
    const double idx = vector_interpolate_idx_for_value(u, mDate->mCalibration->mRepartition);
    
    mT.mX = mDate->mCalibration->mTmin + idx * mDate->mSettings.mStep;
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
    const double tminCalib = mDate->mCalibration->mTmin;
    const double tmaxCalib = mDate->mCalibration->mTmax;
    const double level = plugin->mMCMCSettings.mMixingLevel;
    const double u = Generator::randomUniform();
    double t_new;
    
    if(u < level){
        const double idx = vector_interpolate_idx_for_value(u, mDate->mCalibration->mRepartition);
        t_new = tminCalib + idx + mSettings.mStep;
    }
    else{
        t_new = Generator::gaussByBoxMuller(mT.mX, (tmaxCalib - tminCalib) / 2);
    }
    
    long double likelihood = 0.;
    long double likelihood_new = 0.;
    
    if(t_new < tminCalib || t_new > tmaxCalib){
        likelihood_new = 0;
    }
    else if(mode == DATE_AM_MODE_ID)
    {
        long double git = plugin->getRefCurveValueAt(curveI, mT.mX);
        long double sigma_git = plugin->getRefCurveErrorAt(curveI, mT.mX);
        long double gdt = plugin->getRefCurveValueAt(curveD, mT.mX);
        long double sigma_gdt = plugin->getRefCurveErrorAt(curveD, mT.mX);
        long double vi = mSi * mSi + mSigma2.mX + sigma_git * sigma_git;
        long double vd = mSd * mSd + mSigma2.mX / cosl(i * M_PI / 180.l) + sigma_gdt * sigma_gdt;
        long double wi = 1 / vi;
        long double wd = 1 / vd;
        
        double prop = (mT.mX - tminCalib) / (tmaxCalib - tminCalib);
        double idx = prop * (mDate->mCalibration->mRepartition.size() - 1);
        int idxUnder = (int) floor(idx);
        double q1 = (mDate->mCalibration->mRepartition[idxUnder+1] - mDate->mCalibration->mRepartition[idxUnder]) / mSettings.mStep;
        const double sigma = (tmaxCalib - tminCalib) / 2;
        double q2 = exp(-0.5 * pow((mT.mX - t_new) / sigma, 2)) / (sigma * sqrt(2 * M_PI));
        double proposal = (level * q1 + (1 - level) * q2);
        
        likelihood = sqrtl(wi) * expl(-0.5 * wi * (i - git) * (i - git)) *
                    sqrtl(wd) * expl(-0.5 * wd * (d - gdt) * (d - gdt)) * proposal;
        
        long double git_new = plugin->getRefCurveValueAt(curveI, t_new);
        long double sigma_git_new = plugin->getRefCurveErrorAt(curveI, t_new);
        long double gdt_new = plugin->getRefCurveValueAt(curveD, t_new);
        long double sigma_gdt_new = plugin->getRefCurveErrorAt(curveD, t_new);
        long double vi_new = mSi * mSi + mSigma2.mX + sigma_git_new * sigma_git_new;
        long double vd_new = mSd * mSd + mSigma2.mX / cosl(i * M_PI / 180.l) + sigma_gdt_new * sigma_gdt_new;
        long double wi_new = 1 / vi_new;
        long double wd_new = 1 / vd_new;
        
        prop = (t_new - tminCalib) / (tmaxCalib - tminCalib);
        idx = prop * (mDate->mCalibration->mRepartition.size() - 1);
        idxUnder = (int) floor(idx);
        q1 = (mDate->mCalibration->mRepartition[idxUnder+1] - mDate->mCalibration->mRepartition[idxUnder]) / mSettings.mStep;
        q2 = exp(-0.5 * pow((t_new - mT.mX) / sigma, 2)) / (sigma * sqrt(2 * M_PI));
        double proposal_new = (level * q1 + (1 - level) * q2);
        
        likelihood_new = sqrtl(wi_new) * expl(-0.5 * wi_new * (i - git_new) * (i - git_new)) *
        sqrtl(wd_new) * expl(-0.5 * wd_new * (d - gdt_new) * (d - gdt_new)) * proposal_new;
    }
    else if(mode == DATE_AM_MODE_IF)
    {
        
    }
    else if(mode == DATE_AM_MODE_IDF)
    {
        
    }
    
    double rapport = (likelihood_new / likelihood);
    mT.tryUpdate(t_new, rapport);
    mT.memo();
    mT.saveCurrentAcceptRate();
    
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage sigma :  MH avec marcheur gaussien adaptatif sur le log de vi
    // ------------------------------------------------------------------------------------------
    const int logVMin = -6;
    const int logVMax = 100;
    
    const double V1 = mSigma2.mX * mSigma2.mX;
    const double logV2 = Generator::gaussByBoxMuller(log10(V1), mSigma2.mSigmaMH);
    const double sigma2_new = pow(10, logV2);
    rapport = 0;
    
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


