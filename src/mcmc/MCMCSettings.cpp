#include "MCMCSettings.h"
#include <QVariant>
#include <QJsonArray>


MCMCSettings::MCMCSettings():
mNumChains(3),
mNumRunIter(100000),
mNumBurnIter(1000),
mMaxBatches(100),
mIterPerBatch(100),
mThinningInterval(10),
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
    mNumChains = s.mNumChains;
    mNumRunIter = s.mNumRunIter;
    mNumBurnIter = s.mNumBurnIter;
    mMaxBatches = s.mMaxBatches;
    mIterPerBatch = s.mIterPerBatch;
    mSeeds = s.mSeeds;
    mThinningInterval = s.mThinningInterval;
    mFinalBatchIndex = s.mFinalBatchIndex;
}

MCMCSettings::~MCMCSettings()
{
    
}

MCMCSettings MCMCSettings::fromJson(const QJsonObject& json)
{
    MCMCSettings settings;
    settings.mNumChains = json["num_proc"].toInt();
    settings.mNumRunIter = json["num_iter"].toInt();
    settings.mNumBurnIter = json["num_burn"].toInt();
    settings.mMaxBatches = json["max_batches"].toInt();
    settings.mIterPerBatch = json["iter_per_batch"].toInt();
    settings.mThinningInterval = json["thinning_interval"].toInt();
    
    QJsonArray seeds = json["seeds"].toArray();
    for(int i=0; i<seeds.size(); ++i)
        settings.mSeeds.append(seeds[i].toInt());
    
    return settings;
}

QJsonObject MCMCSettings::toJson() const
{
    QJsonObject mcmc;
    mcmc["num_proc"] = QJsonValue::fromVariant(mNumChains);
    mcmc["num_iter"] = QJsonValue::fromVariant(mNumRunIter);
    mcmc["num_burn"] = QJsonValue::fromVariant(mNumBurnIter);
    mcmc["max_batches"] = QJsonValue::fromVariant(mMaxBatches);
    mcmc["iter_per_batch"] = QJsonValue::fromVariant(mIterPerBatch);
    mcmc["thinning_interval"] = QJsonValue::fromVariant(mThinningInterval);
    
    QJsonArray seeds;
    for(int i=0; i<mSeeds.size(); ++i)
        seeds.append(QJsonValue::fromVariant(mSeeds[i]));
    mcmc["seeds"] = seeds;
    
    return mcmc;
}
