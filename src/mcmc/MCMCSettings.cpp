/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "MCMCSettings.h"

#include "StateKeys.h"
#include "Generator.h"
#include <QVariant>
#include <QJsonArray>
#include <QDataStream>


MCMCSettings::MCMCSettings():
    mNumChains(MCMC_NUM_CHAINS_DEFAULT),
    mIterPerAquisition(MCMC_NUM_RUN_DEFAULT),
    mIterPerBurn(MCMC_NUM_BURN_DEFAULT),
    mMaxBatches(MCMC_MAX_ADAPT_BATCHES_DEFAULT),
    mIterPerBatch(MCMC_ITER_PER_BATCH_DEFAULT),
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
    mIterPerAquisition = s.mIterPerAquisition;
    mIterPerBurn = s.mIterPerBurn;
    mMaxBatches = s.mMaxBatches;
    mIterPerBatch = s.mIterPerBatch;
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
    mIterPerAquisition =  MCMC_NUM_RUN_DEFAULT;
    mIterPerBurn =  MCMC_NUM_BURN_DEFAULT;
    mMaxBatches =  MCMC_MAX_ADAPT_BATCHES_DEFAULT;
    mIterPerBatch = MCMC_ITER_PER_BATCH_DEFAULT;
    mThinningInterval =  MCMC_THINNING_INTERVAL_DEFAULT;
    mMixingLevel =  MCMC_MIXING_DEFAULT;
    mFinalBatchIndex= 0;

}


MCMCSettings MCMCSettings::fromJson(const QJsonObject& json)
{
    MCMCSettings settings;
    settings.mNumChains = json.contains(STATE_MCMC_NUM_CHAINS) ? json.value(STATE_MCMC_NUM_CHAINS).toInt() : MCMC_NUM_CHAINS_DEFAULT;
    settings.mIterPerAquisition = json.contains(STATE_MCMC_NUM_RUN_ITER) ? json.value(STATE_MCMC_NUM_RUN_ITER).toInt() : MCMC_NUM_RUN_DEFAULT;
    settings.mIterPerBurn = json.contains(STATE_MCMC_NUM_BURN_ITER) ? json.value(STATE_MCMC_NUM_BURN_ITER).toInt() : MCMC_NUM_BURN_DEFAULT;
    settings.mMaxBatches = json.contains(STATE_MCMC_MAX_ADAPT_BATCHES) ? json.value(STATE_MCMC_MAX_ADAPT_BATCHES).toInt() : MCMC_MAX_ADAPT_BATCHES_DEFAULT;
    settings.mIterPerBatch = json.contains(STATE_MCMC_ITER_PER_BATCH) ? json.value(STATE_MCMC_ITER_PER_BATCH).toInt() : MCMC_ITER_PER_BATCH_DEFAULT;
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
    mcmc[STATE_MCMC_NUM_RUN_ITER] = QJsonValue::fromVariant(mIterPerAquisition);
    mcmc[STATE_MCMC_NUM_BURN_ITER] = QJsonValue::fromVariant(mIterPerBurn);
    mcmc[STATE_MCMC_MAX_ADAPT_BATCHES] = QJsonValue::fromVariant(mMaxBatches);
    mcmc[STATE_MCMC_ITER_PER_BATCH] = QJsonValue::fromVariant(mIterPerBatch);
    mcmc[STATE_MCMC_THINNING_INTERVAL] = QJsonValue::fromVariant(mThinningInterval);

    mcmc[STATE_MCMC_MIXING] = QJsonValue::fromVariant(mMixingLevel);

    QJsonArray seeds;
    for (int i=0; i<mSeeds.size(); ++i)
        seeds.append(QJsonValue::fromVariant(mSeeds.at(i)) );
    mcmc[STATE_MCMC_SEEDS] = seeds;

    return mcmc;
}

std::vector<ChainSpecs> MCMCSettings::getChains() const
{
    std::vector<ChainSpecs> chains;

    for (int i=0; i<(int)mNumChains; ++i) {
        ChainSpecs chain;

        if (i < mSeeds.size())
            chain.mSeed = mSeeds.at(i);
        else
            chain.mSeed = Generator::createSeed();

        chain.mIterPerBurn = mIterPerBurn;
        chain.mBurnIterIndex = 0;
        chain.mMaxBatchs = mMaxBatches;
        chain.mIterPerBatch = mIterPerBatch;
        chain.mBatchIterIndex = 0;
        chain.mBatchIndex = 0;
        chain.mIterPerAquisition = mIterPerAquisition;
        chain.mAquisitionIterIndex = 0;
        chain.mRealyAccepted = 0;
        chain.mTotalIter = 0;
        chain.mThinningInterval = mThinningInterval;
        chain.mMixingLevel = mMixingLevel;
        chains.push_back(chain);
    }
    return chains;
}

QDataStream &operator<<( QDataStream &stream, const MCMCSettings &data )
{
    stream << quint8 (data.mNumChains);
    stream << (quint32) data.mIterPerAquisition;
    stream << (quint32) data.mIterPerBurn;
    stream << (quint32) data.mMaxBatches;
    stream << (quint32) data.mIterPerBatch;
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
    data.mIterPerAquisition = int(tmp32);

    stream >> tmp32;
    data.mIterPerBurn = int(tmp32);

    stream >> tmp32;
    data.mMaxBatches = int(tmp32);

    stream >> tmp32;
    data.mIterPerBatch = int(tmp32);

    stream >> tmp32;
    data.mThinningInterval = int(tmp32);

    stream >> data.mSeeds;

    stream >> tmp32;
    data.mFinalBatchIndex = int(tmp32);

    stream >> data.mMixingLevel;

    return stream;

}
