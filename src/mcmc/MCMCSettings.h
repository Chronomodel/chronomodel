#ifndef MCMCSettings_H
#define MCMCSettings_H

#include <QJsonObject>
#include <QList>
#include "StateKeys.h"

#define MCMC_NUM_CHAINS_DEFAULT 3
#define MCMC_NUM_RUN_DEFAULT 100000
#define MCMC_NUM_BURN_DEFAULT 1000
#define MCMC_MAX_ADAPT_BATCHES_DEFAULT 100
#define MCMC_ITER_PER_BATCH_DEFAULT 100
#define MCMC_THINNING_INTERVAL_DEFAULT 10

struct Chain
{
    int mSeed;
    unsigned long mNumBurnIter;
    unsigned long mBurnIterIndex;
    unsigned long mMaxBatchs;
    unsigned long mNumBatchIter;
    unsigned long mBatchIterIndex;
    unsigned long mBatchIndex;
    unsigned long mNumRunIter;
    unsigned long mRunIterIndex;
    unsigned long mTotalIter; // burn + adapt + run
    unsigned long mThinningInterval;
};


class MCMCSettings
{
public:
    MCMCSettings();
    MCMCSettings(const MCMCSettings& s);
    MCMCSettings& operator=(const MCMCSettings& s);
    void copyFrom(const MCMCSettings& s);
    ~MCMCSettings();
    
    static MCMCSettings fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QList<Chain> getChains() const;
    
    unsigned long long mNumChains;
    unsigned long long mNumRunIter;
    unsigned long long mNumBurnIter;
    unsigned long long mMaxBatches;
    unsigned long long mNumBatchIter;
    unsigned int mThinningInterval;
    QList<int> mSeeds;
    
    unsigned long mFinalBatchIndex;
};

#endif
