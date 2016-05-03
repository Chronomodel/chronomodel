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
    for(int i=0; i<s.mNumChains; ++i)
    {
        ChainSpecs chain;
        
        if(i < s.mSeeds.size())
            chain.mSeed = s.mSeeds.at(i);
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
        chain.mMixingLevel = s.mMixingLevel;
        mChains.append(chain);
    }
}

const QList<ChainSpecs> &MCMCLoop::chains() const
{
    return mChains;
}

const QString& MCMCLoop::getChainsLog() const
{
    return mChainsLog;
}

const QString MCMCLoop::getMCMCSettingsLog() const
{
    QString log;
    int i =0;
    foreach (const ChainSpecs chain, mChains) {
            ++i;
            log += "<hr>";
            log += tr("Chain")+ " : " + QString::number(i)+"<br>";
            log += tr("Seed")+ " : " + QString::number(chain.mSeed)+"<br>";
            log += tr("Number of burning iterations")+ " : " + QString::number(chain.mBurnIterIndex)+"<br>";//+ " / " + QString::number(chain.mNumBurnIter)+"<br>";
            log += tr("Number of batches")+ " : " + QString::number(chain.mBatchIndex)+ " / " + QString::number(chain.mMaxBatchs)+"<br>";
            log += tr("Number of iterations per batches")+ " : " + QString::number(chain.mNumBatchIter)+"<br>";
            log += tr("Number of running iterations")+ " : " + QString::number(chain.mRunIterIndex)+"<br>";//+ " / " + QString::number(chain.mNumRunIter)+"<br>";
            log += tr("Thinning Interval")+ " : " + QString::number(chain.mThinningInterval)+"<br>";
            log += tr("Total iterations")+ " : " + QString::number(chain.mTotalIter)+"<br>";
            log += tr("Mixing level")+ " : " + QString::number(chain.mMixingLevel)+"<br>";
     }

    return log;
}
const QString MCMCLoop::getInitLog() const
{
    const QString log = getMCMCSettingsLog() + mInitLog;
    return log;
}

void MCMCLoop::run()
{    
    QString mDate =QDateTime::currentDateTime().toString("dddd dd MMMM yyyy");
    QTime startTime = QTime::currentTime();
    QString log= "Start " +mDate+" ->>> " +startTime.toString("hh:mm:ss.zzz");
    

    //----------------------- Calibrating --------------------------------------
    
    emit stepChanged(tr("Calibrating data..."), 0, 0);
    
    mAbortedReason = this->calibrate();
    if(!mAbortedReason.isEmpty())
    {
        return;
    }
    

    //----------------------- Chains --------------------------------------
    
    QStringList seeds;
    
    mInitLog = QString();
    
    for (mChainIndex = 0; mChainIndex < mChains.size(); ++mChainIndex) {
        log += "<hr>";
        //log += line("Chain : " + QString::number(mChainIndex + 1) + "/" + QString::number(mChains.size()));

        ChainSpecs& chain = mChains[mChainIndex];
        Generator::initGenerator(chain.mSeed);
        
        log += line("Seed : " + QString::number(chain.mSeed));
        seeds << QString::number(chain.mSeed);
        
        this->initVariablesForChain();
        
        //----------------------- Initializing --------------------------------------
        
        if (isInterruptionRequested()) {
            mAbortedReason = ABORTED_BY_USER;
            return;
        }
        
        emit stepChanged("Chain " + QString::number(mChainIndex+1) + "/" + QString::number(mChains.size()) + " : " + tr("Initializing MCMC"), 0, 0);
        
        //QTime startInitTime = QTime::currentTime();
        
        mAbortedReason = this->initMCMC();
        if(!mAbortedReason.isEmpty())
            return;
        
        
        /*QTime endInitTime = QTime::currentTime();
        timeDiff = startInitTime.msecsTo(endInitTime);
        
        log += "=> Init done in " + QString::number(timeDiff) + " ms\n";*/
        
        //----------------------- Burning --------------------------------------
        
        emit stepChanged("Chain " + QString::number(mChainIndex+1) + "/" + QString::number(mChains.size()) + " : " + tr("Burning"), 0, chain.mNumBurnIter);
        mState = eBurning;
        
        //QTime startBurnTime = QTime::currentTime();
        
        while (chain.mBurnIterIndex < chain.mNumBurnIter) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }
            
            try {
                this->update();
            } catch (QString error) {
                mAbortedReason = error;
                return;
            }
            

            ++chain.mBurnIterIndex;
            ++chain.mTotalIter;
            
            emit stepProgressed(chain.mBurnIterIndex);
        }
        
        /*QTime endBurnTime = QTime::currentTime();
        timeDiff = startBurnTime.msecsTo(endBurnTime);
        log += "=> Burn done in " + QString::number(timeDiff) + " ms\n";*/
        
        //----------------------- Adapting --------------------------------------
        
        emit stepChanged("Chain " + QString::number(mChainIndex+1) + "/" + QString::number(mChains.size()) + " : " + tr("Adapting"), 0, chain.mMaxBatchs * chain.mNumBatchIter);
        mState = eAdapting;
        
        //QTime startAdaptTime = QTime::currentTime();
        
        while (chain.mBatchIndex * chain.mNumBatchIter < chain.mMaxBatchs * chain.mNumBatchIter) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }
            
            chain.mBatchIterIndex = 0;
            while (chain.mBatchIterIndex < chain.mNumBatchIter) {
                if (isInterruptionRequested()) {
                    mAbortedReason = ABORTED_BY_USER;
                    return;
                }

                try {
                    this->update();
                } catch (QString error) {
                    mAbortedReason = error;
                    return;
                }
                
                ++chain.mBatchIterIndex;
                ++chain.mTotalIter;
                
                emit stepProgressed(chain.mBatchIndex * chain.mNumBatchIter + chain.mBatchIterIndex);
            }
            ++chain.mBatchIndex;
            
            if(adapt())
                break;

        }
        //log += line("Adapt OK at batch : " + QString::number(chain.mBatchIndex) + "/" + QString::number(chain.mMaxBatchs));
        
       /* QTime endAdaptTime = QTime::currentTime();
        timeDiff = startAdaptTime.msecsTo(endAdaptTime);
        log += "=> Adapt done in " + QString::number(timeDiff) + " ms\n";*/
        
        //----------------------- Running --------------------------------------
        
        emit stepChanged("Chain " + QString::number(mChainIndex+1) + "/" + QString::number(mChains.size()) + " : " + tr("Running"), 0, chain.mNumRunIter);
        mState = eRunning;
        
        //QTime startRunTime = QTime::currentTime();
        
        while (chain.mRunIterIndex < chain.mNumRunIter) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }

            try {
                this->update();
            } catch (QString error) {
                mAbortedReason = error;
                return;
            }
            
            ++chain.mRunIterIndex;
            ++chain.mTotalIter;

            emit stepProgressed(chain.mRunIterIndex);
        }
        
        /*QTime endRunTime = QTime::currentTime();
        timeDiff = startRunTime.msecsTo(endRunTime);
        log += "=> Acquire done in " + QString::number(timeDiff) + " ms\n";*/
        
        //-----------------------------------------------------------------------
        
       /* QTime endChainTime = QTime::currentTime();
        timeDiff = startChainTime.msecsTo(endChainTime);
        log += "=> Chain done in " + QString::number(timeDiff) + " ms\n";*/
    }
    
    log += line(tr("List of used chains seeds (to be copied for re-use in MCMC Settings) : ") + seeds.join(";"));
    
    
    /*QTime endTotalTime = QTime::currentTime();
    timeDiff = startTotalTime.msecsTo(endTotalTime);
    log += "=> MCMC done in " + QString::number(timeDiff) + " ms\n";


    QTime startFinalizeTime = QTime::currentTime();*/
    //-----------------------------------------------------------------------

    emit stepChanged(tr("Computing posterior distributions and numerical results (HPD, credibility, ...)"), 0, 0);
    
    try {
        this->finalize();
    } catch (QString error) {
        mAbortedReason = error;
        return;
    }

    QTime endTime = QTime::currentTime();

    QTime timeDiff(0,0,0,1);
    timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);

    log += line(tr("Model computed") );
    log += line(tr("finish at ") + endTime.toString("hh:mm:ss.zzz") );
    log += line(tr("time elapsed ")+QString::number(timeDiff.hour())+"h "+QString::number(timeDiff.minute())+"m "+QString::number(timeDiff.second())+"s "+QString::number(timeDiff.msec())+"ms" );


    //-----------------------------------------------------------------------
    
    mChainsLog = log;
}

