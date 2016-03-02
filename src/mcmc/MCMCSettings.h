#ifndef MCMCSettings_H
#define MCMCSettings_H

#include <QJsonObject>
#include <QList>
#include "StateKeys.h"

#define MCMC_NUM_CHAINS_DEFAULT 3
#define MCMC_NUM_RUN_DEFAULT 100000
#define MCMC_NUM_BURN_DEFAULT 1000
#define MCMC_MAX_ADAPT_BATCHES_DEFAULT 20
#define MCMC_ITER_PER_BATCH_DEFAULT 500
#define MCMC_THINNING_INTERVAL_DEFAULT 10

#define MCMC_MIXING_DEFAULT 0.99f


struct ChainSpecs
{
    int mSeed;
    unsigned long mNumBurnIter;
    unsigned long mBurnIterIndex;
    unsigned int mMaxBatchs;
    unsigned int mNumBatchIter;
    unsigned long mBatchIterIndex;
    unsigned int mBatchIndex;
    unsigned long mNumRunIter;
    unsigned long mRunIterIndex;
    unsigned long mTotalIter; // burn + adapt + run
    unsigned long mThinningInterval;
    double mMixingLevel;
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
    void restoreDefault();
    QJsonObject toJson() const;
    
    QList<ChainSpecs> getChains() const;
    
    int mNumChains;
    unsigned long long mNumRunIter;
    unsigned long long mNumBurnIter;
    unsigned int mMaxBatches;
    unsigned long long mNumBatchIter;
    unsigned int mThinningInterval;
    QList<int> mSeeds;
    
    unsigned int mFinalBatchIndex;
    double mMixingLevel;
};

#endif
