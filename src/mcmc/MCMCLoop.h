#ifndef MCMCLOOP_H
#define MCMCLOOP_H

#include <QThread>
#include "MCMCSettings.h"


class MCMCLoop : public QThread
{
    Q_OBJECT
public:
    enum State
    {
        eBurning = 0,
        eAdapting = 0,
        eRunning = 0
    };
    
    MCMCLoop();
    virtual ~MCMCLoop();
    
    void run();
    
signals:
    void stepChanged(QString title, int min, int max);
    void stepProgressed(int value);
    
protected:
    virtual void initModel() = 0;
    virtual void calibrate() = 0;
    virtual void initMCMC() = 0;
    virtual void update() = 0;
    virtual void finalize() = 0;
    virtual bool adapt() = 0;
    
protected:
    MCMCSettings mSettings;
    State mState;
    
    unsigned long mProcIndex;
    unsigned long long mBurnIterIndex;
    unsigned long long mBatchIterIndex;
    unsigned long mBatchIndex;
    unsigned long long mRunIterIndex;
    unsigned long long mTotalIter;
    
    unsigned long mFinalBatchIndex;
};

#endif
