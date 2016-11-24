#include "MCMCSettings.h"
#include "Generator.h"
#include <QVariant>
#include <QJsonArray>
#include <QDataStream>


MCMCSettings::MCMCSettings():
mNumChains(MCMC_NUM_CHAINS_DEFAULT),
mNumRunIter(MCMC_NUM_RUN_DEFAULT),
mNumBurnIter(MCMC_NUM_BURN_DEFAULT),
mMaxBatches(MCMC_MAX_ADAPT_BATCHES_DEFAULT),
mNumBatchIter(MCMC_ITER_PER_BATCH_DEFAULT),
mThinningInterval(MCMC_THINNING_INTERVAL_DEFAULT),
mFinalBatchIndex(0),
mMixingLevel(MCMC_MIXING_DEFAULT)
{
    
}

MCMCSettings::MCMCSettings(const MCMCSettings& s)
{
    copyFrom(s);
}

MCMCSettings& MCMCSettings::operator=(const MCMCSettings& s)
{
    copyFrom(s);
    return *this;
}

void MCMCSettings::copyFrom(const MCMCSettings& s)
{
    mNumChains = s.mNumChains;
    mNumRunIter = s.mNumRunIter;
    mNumBurnIter = s.mNumBurnIter;
    mMaxBatches = s.mMaxBatches;
    mNumBatchIter = s.mNumBatchIter;
    mSeeds = s.mSeeds;
    mThinningInterval = s.mThinningInterval;
    mFinalBatchIndex = s.mFinalBatchIndex;
    
    mMixingLevel = s.mMixingLevel;
}

MCMCSettings::~MCMCSettings()
{
    
}

void MCMCSettings::restoreDefault()
{
    mNumChains = MCMC_NUM_CHAINS_DEFAULT;
    mNumRunIter =  MCMC_NUM_RUN_DEFAULT;
    mNumBurnIter =  MCMC_NUM_BURN_DEFAULT;
    mMaxBatches =  MCMC_MAX_ADAPT_BATCHES_DEFAULT;
    mNumBatchIter = MCMC_ITER_PER_BATCH_DEFAULT;
    mThinningInterval =  MCMC_THINNING_INTERVAL_DEFAULT;
    mMixingLevel =  MCMC_MIXING_DEFAULT;
    mFinalBatchIndex= 0;

}


MCMCSettings MCMCSettings::fromJson(const QJsonObject& json)
{
    MCMCSettings settings;
    settings.mNumChains = json.contains(STATE_MCMC_NUM_CHAINS) ? json.value(STATE_MCMC_NUM_CHAINS).toInt() : MCMC_NUM_CHAINS_DEFAULT;
    settings.mNumRunIter = json.contains(STATE_MCMC_NUM_RUN_ITER) ? json.value(STATE_MCMC_NUM_RUN_ITER).toInt() : MCMC_NUM_RUN_DEFAULT;
    settings.mNumBurnIter = json.contains(STATE_MCMC_NUM_BURN_ITER) ? json.value(STATE_MCMC_NUM_BURN_ITER).toInt() : MCMC_NUM_BURN_DEFAULT;
    settings.mMaxBatches = json.contains(STATE_MCMC_MAX_ADAPT_BATCHES) ? json.value(STATE_MCMC_MAX_ADAPT_BATCHES).toInt() : MCMC_MAX_ADAPT_BATCHES_DEFAULT;
    settings.mNumBatchIter = json.contains(STATE_MCMC_ITER_PER_BATCH) ? json.value(STATE_MCMC_ITER_PER_BATCH).toInt() : MCMC_ITER_PER_BATCH_DEFAULT;
    settings.mThinningInterval = json.contains(STATE_MCMC_THINNING_INTERVAL) ? json.value(STATE_MCMC_THINNING_INTERVAL).toInt() : MCMC_THINNING_INTERVAL_DEFAULT;
    settings.mMixingLevel = json.contains(STATE_MCMC_MIXING) ? json.value(STATE_MCMC_MIXING).toDouble() : MCMC_MIXING_DEFAULT;
    QJsonArray seeds = json.value(STATE_MCMC_SEEDS).toArray();
    for (int i=0; i<seeds.size(); ++i)
        settings.mSeeds.append(seeds.at(i).toInt());
    
    return settings;
}

QJsonObject MCMCSettings::toJson() const
{
    QJsonObject mcmc;
    mcmc[STATE_MCMC_NUM_CHAINS] = QJsonValue::fromVariant(mNumChains);
    mcmc[STATE_MCMC_NUM_RUN_ITER] = QJsonValue::fromVariant(mNumRunIter);
    mcmc[STATE_MCMC_NUM_BURN_ITER] = QJsonValue::fromVariant(mNumBurnIter);
    mcmc[STATE_MCMC_MAX_ADAPT_BATCHES] = QJsonValue::fromVariant(mMaxBatches);
    mcmc[STATE_MCMC_ITER_PER_BATCH] = QJsonValue::fromVariant(mNumBatchIter);
    mcmc[STATE_MCMC_THINNING_INTERVAL] = QJsonValue::fromVariant(mThinningInterval);
    
    mcmc[STATE_MCMC_MIXING] = QJsonValue::fromVariant(mMixingLevel);
    
    QJsonArray seeds;
    for (int i=0; i<mSeeds.size(); ++i)
        seeds.append(QJsonValue::fromVariant(mSeeds.at(i)) );
    mcmc[STATE_MCMC_SEEDS] = seeds;
    
    return mcmc;
}

QList<ChainSpecs> MCMCSettings::getChains() const
{
    QList<ChainSpecs> chains;
    
    for (int i=0; i<(int)mNumChains; ++i) {
        ChainSpecs chain;
        
        if (i < mSeeds.size())
            chain.mSeed = mSeeds.at(i);
        else
            chain.mSeed = Generator::createSeed();
        
        chain.mNumBurnIter = mNumBurnIter;
        chain.mBurnIterIndex = 0;
        chain.mMaxBatchs = mMaxBatches;
        chain.mNumBatchIter = mNumBatchIter;
        chain.mBatchIterIndex = 0;
        chain.mBatchIndex = 0;
        chain.mNumRunIter = mNumRunIter;
        chain.mRunIterIndex = 0;
        chain.mTotalIter = 0;
        chain.mThinningInterval = mThinningInterval;
        chain.mMixingLevel = mMixingLevel;
        chains.append(chain);
    }
    return chains;
}

QDataStream &operator<<( QDataStream &stream, const MCMCSettings &data )
{
    stream << quint8 (data.mNumChains);
    stream << (quint32) data.mNumRunIter;
    stream << (quint32) data.mNumBurnIter;
    stream << (quint32) data.mMaxBatches;
    stream << (quint32) data.mNumBatchIter;
    stream << (quint32) data.mThinningInterval;
    stream << data.mSeeds;
    stream << (quint32) data.mFinalBatchIndex;
    stream << data.mMixingLevel;

    return stream;

}

QDataStream &operator>>( QDataStream &stream, MCMCSettings &data )
{
    quint8 tmp8;
    stream >> tmp8;
    data.mNumChains = tmp8;

    quint32 tmp32;
    stream >> tmp32;
    data.mNumRunIter = int(tmp32);

    stream >> tmp32;
    data.mNumBurnIter = int(tmp32);

    stream >> tmp32;
    data.mMaxBatches = int(tmp32);

    stream >> tmp32;
    data.mNumBatchIter = int(tmp32);

    stream >> tmp32;
    data.mThinningInterval = int(tmp32);

    stream >> data.mSeeds;

    stream >> tmp32;
    data.mFinalBatchIndex = int(tmp32);

    stream >> data.mMixingLevel;

    return stream;

}
