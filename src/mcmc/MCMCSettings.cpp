#include "MCMCSettings.h"
#include <QVariant>
#include <QJsonArray>


MCMCSettings::MCMCSettings():
mNumChains(MCMC_NUM_CHAINS_DEFAULT),
mNumRunIter(MCMC_NUM_RUN_DEFAULT),
mNumBurnIter(MCMC_NUM_BURN_DEFAULT),
mMaxBatches(MCMC_MAX_ADAPT_BATCHES_DEFAULT),
mNumBatchIter(MCMC_ITER_PER_BATCH_DEFAULT),
mThinningInterval(MCMC_THINNING_INTERVAL_DEFAULT),
mFinalBatchIndex(0),
mFFTLength(MCMC_FFT_LEN_DEFAULT),
mCalibStep(MCMC_CALIB_STEP_DEFAULT)
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
    mFFTLength = s.mFFTLength;
    mCalibStep = s.mCalibStep;
}

MCMCSettings::~MCMCSettings()
{
    
}

MCMCSettings MCMCSettings::fromJson(const QJsonObject& json)
{
    MCMCSettings settings;
    settings.mNumChains = json.contains(STATE_MCMC_NUM_CHAINS) ? json[STATE_MCMC_NUM_CHAINS].toInt() : MCMC_NUM_CHAINS_DEFAULT;
    settings.mNumRunIter = json.contains(STATE_MCMC_NUM_RUN_ITER) ? json[STATE_MCMC_NUM_RUN_ITER].toInt() : MCMC_NUM_RUN_DEFAULT;
    settings.mNumBurnIter = json.contains(STATE_MCMC_NUM_BURN_ITER) ? json[STATE_MCMC_NUM_BURN_ITER].toInt() : MCMC_NUM_BURN_DEFAULT;
    settings.mMaxBatches = json.contains(STATE_MCMC_MAX_ADAPT_BATCHES) ? json[STATE_MCMC_MAX_ADAPT_BATCHES].toInt() : MCMC_MAX_ADAPT_BATCHES_DEFAULT;
    settings.mNumBatchIter = json.contains(STATE_MCMC_ITER_PER_BATCH) ? json[STATE_MCMC_ITER_PER_BATCH].toInt() : MCMC_ITER_PER_BATCH_DEFAULT;
    settings.mThinningInterval = json.contains(STATE_MCMC_THINNING_INTERVAL) ? json[STATE_MCMC_THINNING_INTERVAL].toInt() : MCMC_THINNING_INTERVAL_DEFAULT;
    settings.mFFTLength = json.contains(STATE_MCMC_FFT_LEN) ? json[STATE_MCMC_FFT_LEN].toInt() : MCMC_FFT_LEN_DEFAULT;
    settings.mCalibStep = json.contains(STATE_MCMC_CALIB_STEP) ? json[STATE_MCMC_CALIB_STEP].toInt() : MCMC_CALIB_STEP_DEFAULT;
    
    QJsonArray seeds = json[STATE_MCMC_SEEDS].toArray();
    for(int i=0; i<seeds.size(); ++i)
        settings.mSeeds.append(seeds[i].toInt());
    
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
    mcmc[STATE_MCMC_FFT_LEN] = QJsonValue::fromVariant(mFFTLength);
    mcmc[STATE_MCMC_CALIB_STEP] = QJsonValue::fromVariant(mCalibStep);
    
    QJsonArray seeds;
    for(int i=0; i<mSeeds.size(); ++i)
        seeds.append(QJsonValue::fromVariant(mSeeds[i]));
    mcmc[STATE_MCMC_SEEDS] = seeds;
    
    return mcmc;
}
