/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "MCMCLoop.h"
#include "Generator.h"
#include "QtUtilities.h"

#include <QDebug>
#include <QTime>

MCMCLoop::MCMCLoop():
mChainIndex(0),
mState(eBurning),
mProject(nullptr)
{

}

MCMCLoop::~MCMCLoop()
{
    mProject = nullptr;
}

void MCMCLoop::setMCMCSettings(const MCMCSettings& s)
{
    mChains.clear();
    for (int i=0; i<s.mNumChains; ++i) {
        ChainSpecs chain;

        if (i < s.mSeeds.size())
            chain.mSeed = s.mSeeds.at(i);
        else
            chain.mSeed = Generator::createSeed();

        chain.mNumBurnIter = s.mNumBurnIter;
        chain.mBurnIterIndex = 0;
        chain.mMaxBatchs = s.mMaxBatches;
        chain.mNumBatchIter = s.mNumBatchIter;
        chain.mBatchIterIndex = 0;
        chain.mBatchIndex = 0;
        chain.mNumRunIter = s.mNumRunIter;
        chain.mRunIterIndex = 0;
        chain.mTotalIter = 0;
        chain.mThinningInterval = s.mThinningInterval;
        chain.mMixingLevel = s.mMixingLevel;
        mChains.append(chain);
    }
}

const QList<ChainSpecs> &MCMCLoop::chains() const
{
    return mChains;
}

const QString& MCMCLoop::getChainsLog() const
{
    return mChainsLog;
}

const QString MCMCLoop::getMCMCSettingsLog() const
{
    QString log;
    int i (0);
    foreach (const ChainSpecs chain, mChains) {
            ++i;
            log += "<hr>";
            log += tr("Chain %1").arg(QString::number(i)) +"<br>";
            log += tr("Seed %1").arg(QString::number(chain.mSeed))+"<br>";
            log += tr("Number of burn-in iterations : %1").arg(QString::number(chain.mBurnIterIndex)) + "<br>";
            log += tr("Number of batches : %1 / %2").arg(QString::number(chain.mBatchIndex), QString::number(chain.mMaxBatchs)) + "<br>";
            log += tr("Number of iterations per batches : %1").arg(QString::number(chain.mNumBatchIter)) + "<br>";
            log += tr("Number of running iterations : %1").arg(QString::number(chain.mRunIterIndex)) + "<br>";
            log += tr("Thinning Interval : %1").arg(QString::number(chain.mThinningInterval)) + "<br>";
            log += tr("Total iterations : %1").arg(QString::number(chain.mTotalIter)) + "<br>";
            log += tr("Mixing level : %1").arg(QString::number(chain.mMixingLevel)) + "<br>";
     }

    return log;
}
const QString MCMCLoop::getInitLog() const
{
    const QString log = getMCMCSettingsLog() + mInitLog + mAdaptLog;
    return log;
}

void MCMCLoop::run()
{
    QString mDate = QDateTime::currentDateTime().toString("dddd dd MMMM yyyy");
    QElapsedTimer startTime;
    startTime.start();

    QString log= "Start " + mDate + " -> " + QTime::currentTime().toString("hh:mm:ss.zzz");


    //----------------------- Calibrating --------------------------------------

    emit stepChanged(tr("Calibrating data..."), 0, 0);

    mAbortedReason = this->calibrate();
    if (!mAbortedReason.isEmpty())
        return;



    //----------------------- Chains --------------------------------------

    QStringList seeds;

    mInitLog = QString();

    // initVariableForChain() reserve memory space
    initVariablesForChain();

    for (mChainIndex = 0; mChainIndex < mChains.size(); ++mChainIndex) {
        log += "<hr>";
        //log += line("Chain : " + QString::number(mChainIndex + 1) + "/" + QString::number(mChains.size()));

        ChainSpecs& chain = mChains[mChainIndex];
        Generator::initGenerator(chain.mSeed);

        log += line("Seed : " + QString::number(chain.mSeed));
        seeds << QString::number(chain.mSeed);

        //----------------------- Initialising --------------------------------------

        if (isInterruptionRequested()) {
            mAbortedReason = ABORTED_BY_USER;
            return;
        }

        emit stepChanged(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size()))  + " : " + tr("Initialising MCMC"), 0, 0);

        //QTime startInitTime = QTime::currentTime();

        mAbortedReason = this->initMCMC();
        if (!mAbortedReason.isEmpty())
            return;


        //----------------------- Burn-in --------------------------------------

        emit stepChanged(tr("Chain : %1 / %2").arg(QString::number(mChainIndex + 1), QString::number(mChains.size()))  + " : " + tr("Burn-in"), 0, chain.mNumBurnIter);
        mState = eBurning;

        //QTime startBurnTime = QTime::currentTime();

        while (chain.mBurnIterIndex < chain.mNumBurnIter) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }

            try {
                this->update();
            } catch (QString error) {
                mAbortedReason = error;
                return;
            }

            ++chain.mBurnIterIndex;
            ++chain.mTotalIter;

            emit stepProgressed(chain.mBurnIterIndex);
        }


        //----------------------- Adapting --------------------------------------

        emit stepChanged(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size()))  + " : " + tr("Adapting"), 0, chain.mMaxBatchs * chain.mNumBatchIter);
        mState = eAdapting;

        while (chain.mBatchIndex * chain.mNumBatchIter < chain.mMaxBatchs * chain.mNumBatchIter) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }

            chain.mBatchIterIndex = 0;
            while (chain.mBatchIterIndex < chain.mNumBatchIter) {
                if (isInterruptionRequested()) {
                    mAbortedReason = ABORTED_BY_USER;
                    return;
                }

                try {
                    this->update();

                } catch (QString error) {
                    mAbortedReason = error;
                    return;
                }

                ++chain.mBatchIterIndex;
                ++chain.mTotalIter;

                emit stepProgressed(chain.mBatchIndex * chain.mNumBatchIter + chain.mBatchIterIndex);
            }
            ++chain.mBatchIndex;

            if (adapt()) {
                     break;
            }

        }
        if (chain.mBatchIndex * chain.mNumBatchIter < chain.mMaxBatchs * chain.mNumBatchIter) {
            mAdaptLog += line("Adapt OK at batch : " + QString::number(chain.mBatchIndex) + "/" + QString::number(chain.mMaxBatchs));

        } else {
            mAdaptLog += line("Not adapted after : " + QString::number(chain.mBatchIndex) + " batches");
        }




       /* QTime endAdaptTime = QTime::currentTime();
        timeDiff = startAdaptTime.msecsTo(endAdaptTime);
        log += "=> Adapt done in " + QString::number(timeDiff) + " ms\n";*/

        //----------------------- Running --------------------------------------

        emit stepChanged(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size())) + " : " + tr("Running"), 0, chain.mNumRunIter);
        mState = eRunning;


        while (chain.mRunIterIndex < chain.mNumRunIter) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }

            try {
                this->update();
            } catch (QString error) {
                mAbortedReason = error;
                return;
            }

            ++chain.mRunIterIndex;
            ++chain.mTotalIter;

            emit stepProgressed(chain.mRunIterIndex);
        }

        /*QTime endRunTime = QTime::currentTime();
        timeDiff = startRunTime.msecsTo(endRunTime);
        log += "=> Acquire done in " + QString::number(timeDiff) + " ms\n";*/

        //-----------------------------------------------------------------------

       /* QTime endChainTime = QTime::currentTime();
        timeDiff = startChainTime.msecsTo(endChainTime);
        log += "=> Chain done in " + QString::number(timeDiff) + " ms\n";*/



    }

    log += line(tr("List of used chain seeds (to be copied for re-use in MCMC Settings) : ") + seeds.join(";"));


    //-----------------------------------------------------------------------

    emit stepChanged(tr("Computing posterior distributions and numerical results (HPD, credibility, ...)"), 0, 0);

    try {
        this->finalize();
    } catch (QString error) {
        mAbortedReason = error;
        return;
    }

    QTime endTime = QTime::currentTime();

    QTime timeDiff(0,0,0, (int)startTime.elapsed());
    //timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);

    log += line(tr("Model computed") );
    log += line(tr("finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) );
    log += line(tr("time elapsed %1 h %2 m %3 s %4 ms").arg(QString::number(timeDiff.hour()),
                                                            QString::number(timeDiff.minute()),
                                                            QString::number(timeDiff.second()),
                                                            QString::number(timeDiff.msec()) ));


    //-----------------------------------------------------------------------

    mChainsLog = log;
}
