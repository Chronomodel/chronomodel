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

#define MCMC_MIXING_DEFAULT 0.99


struct ChainSpecs
{
    int mSeed;
    int mNumBurnIter;
    int mBurnIterIndex;
    int mMaxBatchs;
    int mNumBatchIter;
    int mBatchIterIndex;
    int mBatchIndex;
    int mNumRunIter;
    int mRunIterIndex;
    int mTotalIter; // burn + adapt + run
    int mThinningInterval;
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
    int mNumRunIter;
    int mNumBurnIter;
    int mMaxBatches;
    int mNumBatchIter;
    int mThinningInterval;
    QList<int> mSeeds;
    
    int mFinalBatchIndex;
    double mMixingLevel;
};

QDataStream &operator<<( QDataStream &stream, const MCMCSettings &data );
QDataStream &operator>>( QDataStream &stream, MCMCSettings &data );

#endif // endif MCMCSettings_H
