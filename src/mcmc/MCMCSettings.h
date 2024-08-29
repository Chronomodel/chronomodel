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

#ifndef MCMCSETTINGS_H
#define MCMCSETTINGS_H

#include <QJsonObject>
#include <QList>

#define MCMC_NUM_CHAINS_DEFAULT 3
#define MCMC_NUM_RUN_DEFAULT 100000
#define MCMC_NUM_BURN_DEFAULT 1000
#define MCMC_MAX_ADAPT_BATCHES_DEFAULT 200
#define MCMC_ITER_PER_BATCH_DEFAULT 100
#define MCMC_THINNING_INTERVAL_DEFAULT 10

#define MCMC_MIXING_DEFAULT 0.99


struct ChainSpecs
{
    unsigned mSeed;
    int mIterPerBurn = MCMC_NUM_BURN_DEFAULT;
    int mBurnIterIndex = 0;
    int mMaxBatchs = MCMC_MAX_ADAPT_BATCHES_DEFAULT;
    int mIterPerBatch = MCMC_ITER_PER_BATCH_DEFAULT;
    int mBatchIterIndex = 0;
    int mBatchIndex = 0;
    int mIterPerAquisition = 0; //mNumRunIter;
    int mAquisitionIterIndex = 0; // mRunIterIndex;
    int mTotalIter = 0; // burn + adapt + run
    int mThinningInterval = MCMC_THINNING_INTERVAL_DEFAULT;

    int mRealyAccepted = 0;
    double mMixingLevel = MCMC_MIXING_DEFAULT;

    qint64 mInitElapsedTime = 0;
    qint64 burnElapsedTime = 0;
    qint64 mAdaptElapsedTime = 0;
    qint64 mAcquisitionElapsedTime = 0;

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

    std::vector<ChainSpecs> getChains() const;

    int mNumChains;
    int mIterPerAquisition;
    int mIterPerBurn;
    int mMaxBatches;
    int mIterPerBatch;
    int mThinningInterval;
    QList<unsigned> mSeeds;

    int mFinalBatchIndex;
    double mMixingLevel;
};

QDataStream &operator<<( QDataStream &stream, const MCMCSettings &data );
QDataStream &operator>>( QDataStream &stream, MCMCSettings &data );

#endif
