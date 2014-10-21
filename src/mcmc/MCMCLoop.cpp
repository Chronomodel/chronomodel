#include "MCMCLoop.h"
#include "Generator.h"
#include <iostream>
#include <QDebug.h>
using namespace std;

MCMCLoop::MCMCLoop():
mState(eBurning),
mProcIndex(0),
mBurnIterIndex(0),
mBatchIterIndex(0),
mBatchIndex(0),
mRunIterIndex(0),
mTotalIter(0),
mFinalBatchIndex(0)
{
    
}

MCMCLoop::~MCMCLoop()
{
    
}

void MCMCLoop::run()
{
    mTotalIter = 0;
    
    if(!this->initModel())
        return;
    
    emit stepChanged(tr("Calibrating..."), 0, 0);
    this->calibrate();
    
    for(mProcIndex = 0; mProcIndex < mSettings.mNumProcesses; ++mProcIndex)
    {
        Generator::changeSeed();
        
        mBurnIterIndex = 0;
        mBatchIterIndex = 0;
        mBatchIndex = 0;
        mRunIterIndex = 0;
        
        emit stepChanged("Process " + QString::number(mProcIndex+1) + " " + tr("Initializing"), 0, 0);
        this->initMCMC();
        
        //----------------------- Burning --------------------------------------
        
        emit stepChanged("Process " + QString::number(mProcIndex+1) + " " + tr("Burning"), 0, mSettings.mNumBurnIter);
        mState = eBurning;
        
        while(mBurnIterIndex < mSettings.mNumBurnIter)
        {
            if(isInterruptionRequested())
                return;
            
            this->update();
            
            ++mBurnIterIndex;
            ++mTotalIter;
            
            emit stepProgressed(mBurnIterIndex);
        }
        
        //----------------------- Adapting --------------------------------------
        
        emit stepChanged("Process " + QString::number(mProcIndex+1) + " " + tr("Adapting"), 0, mSettings.mMaxBatches * mSettings.mIterPerBatch);
        mState = eAdapting;
        
        while(mBatchIndex * mSettings.mIterPerBatch < mSettings.mMaxBatches * mSettings.mIterPerBatch)
        {
            mBatchIterIndex = 0;
            while(mBatchIterIndex < mSettings.mIterPerBatch)
            {
                if(isInterruptionRequested())
                    return;
                
                this->update();
                ++mBatchIterIndex;
                ++mTotalIter;
                emit stepProgressed(mBatchIndex * mSettings.mIterPerBatch + mBatchIterIndex);
            }
            ++mBatchIndex;
            if(adapt())
            {
                qDebug() << "Adaptation OK at batch " << mBatchIndex << "/" << mSettings.mMaxBatches;
                break;
            }
        }
        mFinalBatchIndex = mBatchIndex;
        
        //----------------------- Running --------------------------------------
        
        emit stepChanged("Process " + QString::number(mProcIndex+1) + " " + tr("Running"), 0, mSettings.mNumRunIter);
        mState = eRunning;
        
        while(mRunIterIndex < mSettings.mNumRunIter)
        {
            if(isInterruptionRequested())
                return;
            
            this->update();
            ++mRunIterIndex;
            ++mTotalIter;
            emit stepProgressed(mRunIterIndex);
        }
    }
    
    this->finalize();
}

