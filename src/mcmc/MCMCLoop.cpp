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
#include "Project.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"

#include <QDebug>
#include <QTime>

MCMCLoop::MCMCLoop():
    mChainIndex (0),
    mState (eBurning),
    mProject (nullptr)
{

}

MCMCLoop::MCMCLoop(Project *project):
    mChainIndex (0),
    mState (eBurning),
    mProject (project)
{
    mProject->mLoop = this;
}

MCMCLoop::~MCMCLoop()
{
    mProject->mLoop = nullptr;
    mProject = nullptr;
}

void MCMCLoop::setMCMCSettings(const MCMCSettings &s)
{
    mChains.clear();
    for (auto i=0; i<s.mNumChains; ++i) {
        ChainSpecs chain;

        if (i < s.mSeeds.size())
            chain.mSeed = s.mSeeds.at(i);
        else
            chain.mSeed = Generator::createSeed();

        chain.mIterPerBurn = s.mIterPerBurn;
        chain.mBurnIterIndex = 0;
        chain.mMaxBatchs = s.mMaxBatches;
        chain.mIterPerBatch = s.mIterPerBatch;
        chain.mBatchIterIndex = 0;
        chain.mBatchIndex = 0;
        chain.mIterPerAquisition = s.mIterPerAquisition;
        chain.mAquisitionIterIndex = 0;
        chain.mTotalIter = 0;
        chain.mThinningInterval = s.mThinningInterval;
        chain.mRealyAccepted = 0;
        chain.mMixingLevel = s.mMixingLevel;
        mChains.append(chain);
    }
}

const QList<ChainSpecs> &MCMCLoop::chains() const
{
    return mChains;
}

void MCMCLoop::run()
{
#if DEBUG
    qDebug()<<"MCMCLoop::run";
#endif

    QElapsedTimer startTime;
    startTime.start();

    QString mDate = QDateTime::currentDateTime().toString("dddd dd MMMM yyyy");


    QString log = "Start " + mDate + " -> " + QTime::currentTime().toString("hh:mm:ss.zzz");


    //----------------------- Calibrating --------------------------------------

    emit stepChanged(tr("Calibrating data..."), 0, 0);

    mAbortedReason = this->calibrate();
    if (!mAbortedReason.isEmpty())
        return;



    //----------------------- Chains --------------------------------------

    // initVariableForChain() reserve memory space
    initVariablesForChain();

    mProject->mModel->mLogInit += ModelUtilities::getMCMCSettingsLog(mProject->mModel);

    QStringList seeds;
    for (auto& chain : chains())
         seeds << QString::number(chain.mSeed);

     mProject->mModel->mLogInit += line(tr("List of used chain seeds (to be copied for re-use in MCMC Settings) : ") + seeds.join(";"));


     // copie la liste des pointeurs, pour garder l'ordre initiale des Events;
     // le mécanisme d'initialisation pour les courbes modifie cette liste, hors il faut la réablir pour les chaines suivantes
     std::vector<Event*> initListEvents (mProject->mModel->mEvents.size());
     std::copy(mProject->mModel->mEvents.begin(), mProject->mModel->mEvents.end(), initListEvents.begin() );

    unsigned estimatedTotalIter = mChains.size() *(mChains.at(0).mIterPerBurn + mChains.at(0).mIterPerBatch*mChains.at(0).mMaxBatchs + mChains.at(0).mIterPerAquisition);
    unsigned iterDone = 0;
    for (mChainIndex = 0; mChainIndex < mChains.size(); ++mChainIndex) {
      /*  if (mChainIndex > 0) {
            // rétablissement de l'ordre des Events, indispensable en cas de calcul de courbe. Car le update modifie l'ordre des events
            std::copy(initListEvents.begin(), initListEvents.end(), mProject->mModel->mEvents.begin() );
        }
      */
        log += "<hr>";

        ChainSpecs& chain = mChains[mChainIndex];
        Generator::initGenerator(chain.mSeed);

        //----------------------- Initialization --------------------------------------

        if (isInterruptionRequested()) {
            mAbortedReason = ABORTED_BY_USER;
            return;
        }

        emit stepChanged(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size()))  + " : " + tr("Initialising MCMC"), 0, 0);

        QElapsedTimer initTime;
        initTime.start();
        mAbortedReason = this->initialize();

        if (!mAbortedReason.isEmpty())
            return;

        this->memo();
        chain.mInitElapsedTime = initTime.elapsed();
        initTime.~QElapsedTimer();

        mProject->mModel->mLogInit += "<hr>";
        mProject->mModel->mLogInit += line(textBold(tr("INIT CHAIN %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size()))));
        mProject->mModel->mLogInit += line("Seed : " + QString::number(chain.mSeed));

        mProject->mModel->mLogInit += ModelUtilities:: modelStateDescriptionHTML(static_cast<ModelCurve*>(mProject->mModel) );
        /*
// Save mLogInit for debug
#ifdef DEBUG
        mProject->mModel->mChains[mChainIndex].mInitElapsedTime = mChains[mChainIndex].mInitElapsedTime;// only to take the time
        QString dirPath = "../../../../..";//QFileInfo(".").absolutePath() ;
        QFile file(dirPath + "/Last_Project_Initialization_DEBUG.html");

        const QString projectName = tr("Project filename : %1").arg(mProject->mName);
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream output(&file);
            output<<"<!DOCTYPE html>"<< Qt::endl;
            output<<"<html>"<< Qt::endl;
            output<<"<body>"<< Qt::endl;

            output<<"<h2>"<< projectName+ "</h2>" << Qt::endl;
            output<<"<hr>";
            output<<mProject->mModel->getInitLog();

            output<<"</body>"<< Qt::endl;
            output<<"</html>"<< Qt::endl;
        }
        file.close();
#endif
*/
        //----------------------- Burnin --------------------------------------

        emit stepChanged(tr("Chain : %1 / %2").arg(QString::number(mChainIndex + 1), QString::number(mChains.size()))  + " : " + tr("Burn-in"), 0, chain.mIterPerBurn);
        mState = eBurning;

        QElapsedTimer burningTime;
        burningTime.start();

        qint64 interTime;
        while (chain.mBurnIterIndex < chain.mIterPerBurn) {
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

            this->memo();
            ++chain.mBurnIterIndex;
            ++chain.mTotalIter;

            ++iterDone;
            interTime = burningTime.elapsed() * (double)(estimatedTotalIter - iterDone) / (double)chain.mBurnIterIndex;
            emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size()) + " : " + "Burning ; Estimated time left " + DHMS(interTime)));

            emit stepProgressed(chain.mBurnIterIndex);
        }
        chain.burnElapsedTime = burningTime.elapsed();
        burningTime.~QElapsedTimer();

        //----------------------- Adaptation --------------------------------------

        emit stepChanged(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size()))  + " : " + tr("Adapting"), 0, chain.mMaxBatchs * chain.mIterPerBatch);
        mState = eAdapting;

        QElapsedTimer adaptTime;
        adaptTime.start();


        while ( chain.mBatchIndex < chain.mMaxBatchs) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }

            chain.mBatchIterIndex = 0;
            while (chain.mBatchIterIndex < chain.mIterPerBatch) {
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
                this->memo();
                ++chain.mBatchIterIndex;
                ++chain.mTotalIter;

                emit stepProgressed(chain.mBatchIndex * chain.mIterPerBatch + chain.mBatchIterIndex);
            }
            ++chain.mBatchIndex;

            iterDone += chain.mIterPerBatch;
            interTime = adaptTime.elapsed() * (double)(estimatedTotalIter - iterDone)/ (double)(chain.mIterPerBatch*chain.mBatchIndex);

            emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size()) + " : " + "Adapting ; Estimated time left " + DHMS(interTime)));


            if (adapt(chain.mBatchIndex))
                    break;


        }
        // Fix Total iteration if adaptation break before the end
        estimatedTotalIter -= (chain.mMaxBatchs-chain.mBatchIndex)*chain.mIterPerBatch;
        interTime = adaptTime.elapsed() * (double)(estimatedTotalIter - iterDone)/ (double)(chain.mIterPerBatch*chain.mBatchIndex);

        emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size()) + " : " + "Adapting ; Estimated time left " + DHMS(interTime)));


        mProject->mModel->mLogAdapt += "<hr>";
        mProject->mModel->mLogAdapt += line(textBold(tr("ADAPTATION FOR CHAIN %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size()))) );

        if (chain.mBatchIndex * chain.mIterPerBatch < chain.mMaxBatchs * chain.mIterPerBatch) {
            mProject->mModel->mLogAdapt += line("Adapt OK at batch : " + QString::number(chain.mBatchIndex) + "/" + QString::number(chain.mMaxBatchs));

        } else {
            mProject->mModel->mLogAdapt += line(textRed("Warning : Not adapted after " + QString::number(chain.mBatchIndex) + " batches"));
        }

        mProject->mModel->mLogAdapt += ModelUtilities::modelStateDescriptionHTML(static_cast<ModelCurve*>(mProject->mModel) );
        mProject->mModel->mLogAdapt += "<hr>";

        chain.mAdaptElapsedTime = adaptTime.elapsed();
        adaptTime.~QElapsedTimer();

        //----------------------- Aquisition --------------------------------------

        emit stepChanged(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size())) + " : " + tr("Aquisition"), 0, chain.mIterPerAquisition);
        mState = eAquisition;
        QElapsedTimer aquisitionTime;
        aquisitionTime.start();

        int thinningIdx = 1;
        int batchIdx = 1;
        int totalBacth = chain.mBatchIndex;
        bool OkToMemo;
        chain.mRealyAccepted = 0;

        while (chain.mAquisitionIterIndex < chain.mIterPerAquisition) {
            if (isInterruptionRequested()) {
                mAbortedReason = ABORTED_BY_USER;
                return;
            }
            ++iterDone;



            try {
                OkToMemo =  this->update();

            } catch (QString error) {
                mAbortedReason = error;
                return;
            }

            if (thinningIdx == chain.mThinningInterval) {
                thinningIdx = 1;

                if (OkToMemo) {
                    this->memo();
                     ++chain.mRealyAccepted;
                }

            } else {
                    thinningIdx++;
            }

            if (batchIdx == chain.mIterPerBatch) {
                adapt(totalBacth);
                batchIdx = 1;
                totalBacth++;

            } else {
                batchIdx++;
            }

            ++chain.mAquisitionIterIndex;
            ++chain.mTotalIter;

         //   if (!(chain.mAquisitionIterIndex % chain.mIterPerBatch)) {

                interTime = aquisitionTime.elapsed() * (double) (estimatedTotalIter - iterDone) / (double) chain.mAquisitionIterIndex;

                emit setMessage(tr("Chain %1 / %2").arg(QString::number(mChainIndex+1), QString::number(mChains.size()) + " : Aquisition ; Estimated time left " + DHMS(interTime)));
         //   }


            emit stepProgressed(chain.mAquisitionIterIndex);
        }
        chain.mAcquisitionElapsedTime = aquisitionTime.elapsed();
        aquisitionTime.~QElapsedTimer();
        mProject->mModel->mLogResults += line(tr("Acquisition time elapsed %1").arg(DHMS(chain.mAcquisitionElapsedTime)));

        // rétablissement de l'ordre des Events, indispensable en cas de calcul de courbe. Car le update modifie l'ordre des events et utile pour la sauvegarde de ChronoModel_Bash
        std::copy(initListEvents.begin(), initListEvents.end(), mProject->mModel->mEvents.begin() );
    }

    mProject->mModel->mChains = mChains;

    //-----------------------------------------------------------------------

    emit stepChanged(tr("Computing posterior distributions and numerical results (HPD, credibility, ...)"), 0, 0);

    try {
        this->finalize();

    } catch (QString error) {
        mAbortedReason = error;
        return;
    }


#ifdef DEBUG
    QTime endTime = QTime::currentTime();

    qDebug()<<"Model computed";
    qDebug()<<tr("finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug()<<tr("Total time elapsed %1").arg(QString(DHMS(startTime.elapsed())));
#endif


}
