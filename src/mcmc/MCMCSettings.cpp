#include "MCMCSettings.h"
#include <QVariant>


MCMCSettings::MCMCSettings():
mNumProcesses(3),
mNumRunIter(100000),
mNumBurnIter(1000),
mMaxBatches(100),
mIterPerBatch(100),
mSeed(10),
mDownSamplingFactor(100000),
mFinalBatchIndex(0)
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
    mNumProcesses = s.mNumProcesses;
    mNumRunIter = s.mNumRunIter;
    mNumBurnIter = s.mNumBurnIter;
    mMaxBatches = s.mMaxBatches;
    mIterPerBatch = s.mIterPerBatch;
    mSeed = s.mSeed;
    mDownSamplingFactor = s.mDownSamplingFactor;
    mFinalBatchIndex = s.mFinalBatchIndex;
}

MCMCSettings::~MCMCSettings()
{
    
}

MCMCSettings MCMCSettings::fromJson(const QJsonObject& json)
{
    MCMCSettings settings;
    settings.mNumProcesses = json["num_proc"].toInt();
    settings.mNumRunIter = json["num_iter"].toInt();
    settings.mNumBurnIter = json["num_burn"].toInt();
    settings.mMaxBatches = json["max_batches"].toInt();
    settings.mIterPerBatch = json["iter_per_batch"].toInt();
    settings.mSeed = json["seed"].toInt();
    settings.mDownSamplingFactor = json["down_sampling_factor"].toInt();
    return settings;
}

QJsonObject MCMCSettings::toJson() const
{
    QJsonObject mcmc;
    mcmc["num_proc"] = QJsonValue::fromVariant(mNumProcesses);
    mcmc["num_iter"] = QJsonValue::fromVariant(mNumRunIter);
    mcmc["num_burn"] = QJsonValue::fromVariant(mNumBurnIter);
    mcmc["max_batches"] = QJsonValue::fromVariant(mMaxBatches);
    mcmc["iter_per_batch"] = QJsonValue::fromVariant(mIterPerBatch);
    mcmc["seed"] = QJsonValue::fromVariant(mSeed);
    mcmc["down_sampling_factor"] = QJsonValue::fromVariant(mDownSamplingFactor);
    return mcmc;
}
