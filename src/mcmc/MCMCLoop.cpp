#include "MCMCLoop.h"
#include "Generator.h"
#include "QtUtilities.h"
#include <QDebug>
#include <QTime>


MCMCLoop::MCMCLoop():
mChainIndex(0),
mState(eBurning)
{
    
}

MCMCLoop::~MCMCLoop()
{
    
}

void MCMCLoop::setMCMCSettings(const MCMCSettings& s)
{
    mChains.clear();
    for(int i=0; i<(int)s.mNumChains; ++i)
    {
        Chain chain;
        
        if(i < s.mSeeds.size())
            chain.mSeed = s.mSeeds[i];
        else
            chain.mSeed = Generator::createSeed();
        
        chain.mNumBurnIter = s.mNumBurnIter;
        chain.mBurnIterIndex = 0;
        chain.mMaxBatchs = s.mMaxBatches;
        chain.mNumBatchIter = s.mNumBatchIter;
        chain.mBatchIterIndex = 0;
        chain.mBatchIndex = 0;
        chain.mNumRunIter = s.mNumRunIter;
        chain.mRunIterIndex = 0;
        chain.mTotalIter = 0;
        chain.mThinningInterval = s.mThinningInterval;
        mChains.append(chain);
    }
}

const QList<Chain>& MCMCLoop::chains()
{
    return mChains;
}

const QString& MCMCLoop::getChainsLog() const
{
    return mChainsLog;
}

const QString& MCMCLoop::getInitLog() const
{
    return mInitLog;
}

void MCMCLoop::run()
{
    QString log;
    log += line(textBold("MCMC Chains Log"));
    
    int timeDiff = 0;
    
    QTime startTotalTime = QTime::currentTime();
    
    //----------------------- Calibrating --------------------------------------
    
    emit stepChanged(tr("Calibrating data..."), 0, 0);
    
    QTime startCalibTime = QTime::currentTime();
    
    mAbortedReason = this->calibrate();
    if(!mAbortedReason.isEmpty())
    {
        return;
    }
    
    QTime endCalibTime = QTime::currentTime();
    timeDiff = startCalibTime.msecsTo(endCalibTime);
    //log += line("Calib done in " + QString::number(timeDiff) + " ms");
    
    //----------------------- Chains --------------------------------------
    
    QStringList seeds;
    
    mInitLog = QString();
    
    for(mChainIndex = 0; mChainIndex < mChains.size(); ++mChainIndex)
    {
        log += line("--------------------------");
        log += line("Chain : " + QString::number(mChainIndex + 1));
        
        QTime startChainTime = QTime::currentTime();
        
        Chain& chain = mChains[mChainIndex];
        Generator::initGenerator(chain.mSeed);
        
        log += line("Seed : " + QString::number(chain.mSeed));
        seeds << QString::number(chain.mSeed);
        
        this->initVariablesForChain();
        
        //----------------------- Initializing --------------------------------------
        
        emit stepChanged("Chain " + QString::number(mChainIndex+1) + " : " + tr("Initializing MCMC"), 0, 0);
        
        QTime startInitTime = QTime::currentTime();
        
        this->initMCMC();
        
        QTime endInitTime = QTime::currentTime();
        timeDiff = startInitTime.msecsTo(endInitTime);
        
        //log += "=> Init done in " + QString::number(timeDiff) + " ms\n";
        
        //----------------------- Burning --------------------------------------
        
        emit stepChanged("Chain " + QString::number(mChainIndex+1) + " : " + tr("Burning"), 0, chain.mNumBurnIter);
        mState = eBurning;
        
        QTime startBurnTime = QTime::currentTime();
        
        while(chain.mBurnIterIndex < chain.mNumBurnIter)
        {
            if(isInterruptionRequested())
                return;
            
            this->update();
            
            ++chain.mBurnIterIndex;
            ++chain.mTotalIter;
            
            emit stepProgressed(chain.mBurnIterIndex);
        }
        
        QTime endBurnTime = QTime::currentTime();
        timeDiff = startBurnTime.msecsTo(endBurnTime);
        //log += "=> Burn done in " + QString::number(timeDiff) + " ms\n";
        
        //----------------------- Adapting --------------------------------------
        
        emit stepChanged("Chain " + QString::number(mChainIndex+1) + " : " + tr("Adapting"), 0, chain.mMaxBatchs * chain.mNumBatchIter);
        mState = eAdapting;
        
        QTime startAdaptTime = QTime::currentTime();
        
        while(chain.mBatchIndex * chain.mNumBatchIter < chain.mMaxBatchs * chain.mNumBatchIter)
        {
            if(isInterruptionRequested())
                return;
            
            chain.mBatchIterIndex = 0;
            while(chain.mBatchIterIndex < chain.mNumBatchIter)
            {
                if(isInterruptionRequested())
                    return;
                
                this->update();
                
                ++chain.mBatchIterIndex;
                ++chain.mTotalIter;
                
                emit stepProgressed(chain.mBatchIndex * chain.mNumBatchIter + chain.mBatchIterIndex);
            }
            ++chain.mBatchIndex;
            
            if(adapt())
            {
                break;
            }
        }
        log += line("Adapt OK at batch : " + QString::number(chain.mBatchIndex) + "/" + QString::number(chain.mMaxBatchs));
        
        QTime endAdaptTime = QTime::currentTime();
        timeDiff = startAdaptTime.msecsTo(endAdaptTime);
        //log += "=> Adapt done in " + QString::number(timeDiff) + " ms\n";
        
        //----------------------- Running --------------------------------------
        
        emit stepChanged("Chain " + QString::number(mChainIndex+1) + " : " + tr("Running"), 0, chain.mNumRunIter);
        mState = eRunning;
        
        QTime startRunTime = QTime::currentTime();
        
        while(chain.mRunIterIndex < chain.mNumRunIter)
        {
            if(isInterruptionRequested())
                return;
            
            this->update();
            
            ++chain.mRunIterIndex;
            ++chain.mTotalIter;
            
            emit stepProgressed(chain.mRunIterIndex);
        }
        
        QTime endRunTime = QTime::currentTime();
        timeDiff = startRunTime.msecsTo(endRunTime);
        //log += "=> Acquire done in " + QString::number(timeDiff) + " ms\n";
        
        //-----------------------------------------------------------------------
        
        QTime endChainTime = QTime::currentTime();
        timeDiff = startChainTime.msecsTo(endChainTime);
        //log += "=> Chain done in " + QString::number(timeDiff) + " ms\n";
    }
    
    log += line("--------------------------");
    log += line("All Seeds (to be used in MCMC Settings dialog) :<br>" + seeds.join(";"));
    
    
    QTime endTotalTime = QTime::currentTime();
    timeDiff = startTotalTime.msecsTo(endTotalTime);
    //log += "=> MCMC done in " + QString::number(timeDiff) + " ms\n";
    
    //-----------------------------------------------------------------------
    
    QTime startFinalizeTime = QTime::currentTime();
    
    emit stepChanged(tr("Computing posterior distributions and numerical results (HPD, credibility, ...)"), 0, 0);
    
    this->finalize();

    QTime endFinalizeTime = QTime::currentTime();
    timeDiff = startFinalizeTime.msecsTo(endFinalizeTime);
    //log += "=> Histos and results computed in " + QString::number(timeDiff) + " ms\n";
    
    //log += "End time : " + endFinalizeTime.toString() + "\n";
    
    //-----------------------------------------------------------------------
    
    mChainsLog = log;
}

