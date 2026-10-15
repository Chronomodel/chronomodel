/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#ifndef MCMCSETTINGS_H
#define MCMCSETTINGS_H

#include <QJsonObject>
#include <QList>

#define MCMC_NUM_CHAINS_DEFAULT 4
#define MCMC_NUM_RUN_DEFAULT 100000
#define MCMC_NUM_BURN_DEFAULT 1000
#define MCMC_MAX_ADAPT_BATCHES_DEFAULT 200
#define MCMC_ITER_PER_BATCH_DEFAULT 100
#define MCMC_THINNING_INTERVAL_DEFAULT 10

#define MCMC_ANNEAL_TEMP 10
#define MCMC_ANNEAL_RECURRENCE 1000
#define MCMC_ANNEAL_DWELL 1000

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
    int mIterDisplay = 0; // nombre d'iterations affichées, peut changer après le filtre sur courbe
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
    bool operator==(const MCMCSettings& other) const;

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

    double mAnnealTemp       = MCMC_ANNEAL_TEMP;
    int    mAnnealRecurrence = MCMC_ANNEAL_RECURRENCE;
    int    mAnnealDwell      = MCMC_ANNEAL_DWELL;

};

QDataStream &operator<<( QDataStream &stream, const MCMCSettings &data );
QDataStream &operator>>( QDataStream &stream, MCMCSettings &data );

#endif
