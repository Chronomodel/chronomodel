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
    unsigned int mNumBurnIter;
    unsigned int mBurnIterIndex;
    unsigned int mMaxBatchs;
    unsigned int mNumBatchIter;
    unsigned int mBatchIterIndex;
    unsigned int mBatchIndex;
    unsigned int mNumRunIter;
    unsigned int mRunIterIndex;
    unsigned int mTotalIter; // burn + adapt + run
    unsigned int mThinningInterval;
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
    
    unsigned int mNumChains;
    unsigned int mNumRunIter;
    unsigned int mNumBurnIter;
    unsigned int mMaxBatches;
    unsigned int mNumBatchIter;
    unsigned int mThinningInterval;
    QList<int> mSeeds;
    
    unsigned int mFinalBatchIndex;
    double mMixingLevel;
};

#endif
