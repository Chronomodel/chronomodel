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

#include "ModelChronocurve.h"
#include "QFile"
#include "qapplication.h"
#include "Project.h"


ModelChronocurve::ModelChronocurve():Model()
{
    mAlphaLissage.mSupport = MetropolisVariable::eR;
    mAlphaLissage.mFormat = DateUtils::eNumeric;
}

ModelChronocurve::~ModelChronocurve()
{

}

QJsonObject ModelChronocurve::toJson() const
{
    QJsonObject json = Model::toJson();
    
    json[STATE_CHRONOCURVE] = mChronocurveSettings.toJson();
    
    return json;
}

void ModelChronocurve::fromJson(const QJsonObject& json)
{
    Model::fromJson(json);
    
    for (Event*& event: mEvents)
        event->mMethod = Event::eMHAdaptGauss;
    
    if (json.contains(STATE_CHRONOCURVE)) {
        const QJsonObject settings = json.value(STATE_CHRONOCURVE).toObject();
        mChronocurveSettings = ChronocurveSettings::fromJson(settings);
    }
}

// Date files read / write
/** @Brief Save .res file, the result of computation and compress it
*
* */
void ModelChronocurve::saveToFile(const QString& fileName)
{
   if (!mEvents.empty()) {
       // -----------------------------------------------------
       //  Create file
       // -----------------------------------------------------
       //QFileInfo info(fileName);
       // QFile file(info.path() + info.baseName() + ".~res"); // when we could do a compressed file
       //QFile file(info.path() + info.baseName() + ".res");
       QFile file(fileName);
       if (file.open(QIODevice::WriteOnly)) {

           QDataStream out(&file);
           out.setVersion(QDataStream::Qt_5_5);


           out << quint32 (out.version());// we could add software version here << quint16(out.version());
           out << qApp->applicationVersion();
           // -----------------------------------------------------
           //  Write info
           // -----------------------------------------------------
           out << quint32 (mPhases.size());
           out << quint32 (mEvents.size());

           int numDates = 0;
           for (Event*& ev : mEvents)
               numDates += ev->mDates.size();

           out << quint32 (numDates);

           out << quint32 (mChains.size());
           for (ChainSpecs& ch : mChains) {
               out << quint32 (ch.mBatchIndex);
               out << quint32 (ch.mBatchIterIndex);
               out << quint32 (ch.mBurnIterIndex);
               out << quint32 (ch.mMaxBatchs);
               out << ch.mMixingLevel;
               out << quint32 (ch.mNumBatchIter);
               out << quint32 (ch.mNumBurnIter);
               out << quint32 (ch.mNumRunIter);
               out << quint32 (ch.mRunIterIndex);
               out << qint32 (ch.mSeed);
               out << quint32 (ch.mThinningInterval);
               out << quint32 (ch.mTotalIter);
           }
           // -----------------------------------------------------
           //  Write phases data
           // -----------------------------------------------------
           for (Phase*& phase : mPhases) {
               out << phase->mAlpha;
               out << phase->mBeta;
               out << phase->mDuration;
           }
           // -----------------------------------------------------
           //  Write events data
           // -----------------------------------------------------
           for (Event*& event : mEvents)
               out << event->mTheta;

           // -----------------------------------------------------
           //  Write dates data
           // -----------------------------------------------------
           for (Event*& event : mEvents) {
               if (event->mType == Event::eDefault ) {
                   QList<Date> dates (event->mDates);
                   for (auto& d : dates) {
                       out << d.mTheta;
                       out << d.mSigma;
                       if (d.mDeltaType != Date::eDeltaNone)
                           out << d.mWiggle;

                       out << d.mDeltaFixed;
                       out << d.mDeltaMin;
                       out << d.mDeltaMax;
                       out << d.mDeltaAverage;
                       out << d.mDeltaError;

                       out << (qint32) d.mSettings.mTmin;
                       out << (qint32) d.mSettings.mTmax;
                       out <<  d.mSettings.mStep;
                       out << quint8 (d.mSettings.mStepForced==true? 1: 0);


                       out << d.getTminRefCurve();
                       out << d.getTmaxRefCurve();

                       //mCalibration and mWiggleCalibration are saved in to *.cal file

                       out << quint32 (d.mCalibHPD.size());
                       for (QMap<double, double>::const_iterator it = d.mCalibHPD.cbegin(); it!=d.mCalibHPD.cend(); ++it) {
                           out << it.key();
                           out << it.value();
                       }
                   }

               }
           }
           out << mLogModel;
           out << mLogMCMC;
           out << mLogResults;
           /* -----------------------------------------------------
            *   Write curve data
            * ----------------------------------------------------- */

           out << mAlphaLissage;

           out << (quint32) mMCMCSplines.size();
           for (auto& splin : mMCMCSplines)
               out << splin;

           out << mPosteriorMeanG;

           out << (quint32) mPosteriorMeanGByChain.size();
           for (auto& pMByChain : mPosteriorMeanGByChain)
               out << pMByChain;

           file.close();


       }
   }
}
/** @Brief Read the .res file, it's the result of the saved computation
*
* */
void ModelChronocurve::restoreFromFile(const QString& fileName)
{
/*    QFile fileDat(fileName);
   fileDat.open(QIODevice::ReadOnly);
   QByteArray compressedData (fileDat.readAll());
   fileDat.close();

   QByteArray uncompressedData (qUncompress(compressedData));
#ifdef DEBUG
      qDebug() << "Lecture fichier :"<< fileName;
      qDebug() << "TAILLE compressedData :" << compressedData.size();
      qDebug() << "TAILLE uncompresedData :" << uncompressedData.size();
#endif
   compressedData.clear();
*/
/*    QFileInfo info(fileName);
   QFile file(info.path() + info.baseName() + ".~dat"); // when we could compress the file

   file.open(QIODevice::WriteOnly);
   file.write(uncompressedData);
   file.close();
*/
  // QFileInfo info(fileName);
  // QFile file(info.path() + info.baseName() + ".res");

   QFile file(fileName);
   if (file.exists() && file.open(QIODevice::ReadOnly)) {
       QDataStream in(&file);

       int QDataStreamVersion;
       in >> QDataStreamVersion;
       in.setVersion(QDataStreamVersion);

       if (in.version()!= QDataStream::Qt_5_5)
           return;

       QString appliVersion;
       in >> appliVersion;
       // prepare the future
       //QStringList projectVersionList = appliVersion.split(".");
       if (appliVersion != qApp->applicationVersion())
           qDebug()<<file.fileName()<<" different version ="<<appliVersion<<" actual ="<<qApp->applicationVersion();


       // -----------------------------------------------------
       //  Read info
       // -----------------------------------------------------

       quint32 tmp32;
       in >> tmp32;
       //const int numPhases = (int)tmp32;
       in >> tmp32;
       //const int numEvents = (int)tmp32;
       in >> tmp32;
       //const int numdates = (int)tmp32;

       in >> tmp32;
       mChains.clear();
       mChains.reserve(int (tmp32));
       for (quint32 i=0 ; i<tmp32; ++i) {
           ChainSpecs ch;
           in >> ch.mBatchIndex;
           in >> ch.mBatchIterIndex;
           in >> ch.mBurnIterIndex;
           in >> ch.mMaxBatchs;
           in >> ch.mMixingLevel;
           in >> ch.mNumBatchIter;
           in >> ch.mNumBurnIter;
           in >> ch.mNumRunIter;
           in >> ch.mRunIterIndex;
           in >> ch.mSeed;
           in >> ch.mThinningInterval;
           in >> ch.mTotalIter;
           mChains.append(ch);
       }

       // -----------------------------------------------------
       //  Read phases data
       // -----------------------------------------------------

       for (auto&& p : mPhases) {
           in >> p->mAlpha;
           in >> p->mBeta;
           in >> p->mDuration;
       }
       // -----------------------------------------------------
       //  Read events data
       // -----------------------------------------------------

       for (auto&& e : mEvents)
           in >> e->mTheta;

       // -----------------------------------------------------
       //  Read dates data
       // -----------------------------------------------------

       for (auto&& event : mEvents) {
           if (event->mType == Event::eDefault )
               for (auto&& d : event->mDates) {
                   in >> d.mTheta;
                   in >> d.mSigma;
                   if (d.mDeltaType != Date::eDeltaNone)
                       in >> d.mWiggle;

                   in >> d.mDeltaFixed;
                   in >> d.mDeltaMin;
                   in >> d.mDeltaMax;
                   in >> d.mDeltaAverage;
                   in >> d.mDeltaError;
                   qint32 tmpInt32;
                   in >> tmpInt32;
                   d.mSettings.mTmin = int (tmpInt32);
                   in >> tmpInt32;
                   d.mSettings.mTmax = int (tmpInt32);
                   in >> d.mSettings.mStep;
                   quint8 btmp;
                   in >> btmp;
                   d.mSettings.mStepForced = (btmp==1);

                   // in >> d.mSubDates;
                   double tmp;
                   in >> tmp;
                   d.setTminRefCurve(tmp);
                   in >> tmp;
                   d.setTmaxRefCurve(tmp);

                   /* Check if the Calibration Curve exist*/

                   //           QMap<QString, CalibrationCurve>::const_iterator it = mProject->mCalibCurves.find (toFind);

                   // if no curve Create a new instance in mProject->mCalibration
                   //             if ( it == mProject->mCalibCurves.end())
                   //                 mProject->mCalibCurves.insert(toFind, CalibrationCurve());

                   //mProject->mCalibCurves.insert(toFind, CalibrationCurve());
                   //     qDebug()<<"Model:restoreFromFile insert a new mCalibration "<<toFind;

                   d.mCalibration = & (mProject->mCalibCurves[d.mUUID]);


                   quint32 tmpUint32;
                   in >> tmpUint32;
                   double tmpKey;
                   double tmpValue;
                   for (quint32 i= 0; i<tmpUint32; i++) {
                       in >> tmpKey;
                       in >> tmpValue;
                       d.mCalibHPD[tmpKey]= tmpValue;
                   }
#ifdef DEBUG

                   const QString toFind ("WID::"+ d.mUUID);

                   if (d.mWiggleCalibration==nullptr || d.mWiggleCalibration->mCurve.isEmpty()) {
                       qDebug()<<"Model::restoreFromFile vide";

                   } else {
                       d.mWiggleCalibration = & (mProject->mCalibCurves[toFind]);
                   }
#endif
               }
       }
       in >> mLogModel;
       in >> mLogMCMC;
       in >> mLogResults;

       generateCorrelations(mChains);
       // generatePosteriorDensities(mChains, 1024, 1);
       // generateNumericalResults(mChains);

       /* -----------------------------------------------------
        *   Read curve data
        * ----------------------------------------------------- */

       in >> mAlphaLissage;

       in >> tmp32;
       mMCMCSplines.resize(tmp32);
       for (auto& splin : mMCMCSplines)
           in >> splin;

       in >> mPosteriorMeanG;

       in >> tmp32;
       mPosteriorMeanGByChain.resize(tmp32);
       for (auto& pMByChain : mPosteriorMeanGByChain)
           in >> pMByChain;

       file.close();

   }

}





void ModelChronocurve::generatePosteriorDensities(const QList<ChainSpecs> &chains, int fftLen, double bandwidth)
{
    Model::generatePosteriorDensities(chains, fftLen, bandwidth);

    for (Event*& event : mEvents) {
        event->mVG.updateFormatedTrace();
        event->mVG.generateHistos(chains, fftLen, bandwidth);
    }

    mAlphaLissage.updateFormatedTrace();
    mAlphaLissage.generateHistos(chains, fftLen, bandwidth);
}

void ModelChronocurve::generateCorrelations(const QList<ChainSpecs> &chains)
{
    Model::generateCorrelations(chains);
    mAlphaLissage.generateCorrelations(chains);
}

void ModelChronocurve::generateNumericalResults(const QList<ChainSpecs> &chains)
{
    Model::generateNumericalResults(chains);
    for (Event*& event : mEvents) {
        event->mVG.generateNumericalResults(chains);
    }
    mAlphaLissage.generateNumericalResults(chains);
}

void ModelChronocurve::clearThreshold()
{
    Model::clearThreshold();
    mThreshold = -1.;
    for (Event*& event : mEvents) {
        event->mVG.mThresholdUsed = -1.;
    }
    mAlphaLissage.mThresholdUsed = -1.;
}

void ModelChronocurve::generateCredibility(const double thresh)
{
    Model::generateCredibility(thresh);
    for (Event*& event : mEvents) {
        if (event->type() != Event::eKnown) {
            event->mVG.generateCredibility(mChains, thresh);
        }
    }
    mAlphaLissage.generateCredibility(mChains, thresh);
}

void ModelChronocurve::generateHPD(const double thresh)
{
    Model::generateHPD(thresh);
    for (Event*& event : mEvents) {
        if (event->type() != Event::eKnown) {
            event->mVG.generateHPD(thresh);
        }
    }
    
    mAlphaLissage.generateHPD(thresh);
}

void ModelChronocurve::clearPosteriorDensities()
{
    Model::clearPosteriorDensities();
    
    for (Event*& event : mEvents) {
        if (event->type() != Event::eKnown) {
            event->mVG.mHisto.clear();
            event->mVG.mChainsHistos.clear();
        }
    }
    
    mAlphaLissage.mHisto.clear();
    mAlphaLissage.mChainsHistos.clear();
}

void ModelChronocurve::clearCredibilityAndHPD()
{
    Model::clearCredibilityAndHPD();
    
    for (Event*& event : mEvents) {
        if (event->type() != Event::eKnown) {
            event->mVG.mHPD.clear();
            event->mVG.mCredibility = QPair<double, double>();
        }
    }
    
    mAlphaLissage.mHPD.clear();
    mAlphaLissage.mCredibility = QPair<double, double>();
}

void ModelChronocurve::clearTraces()
{
    Model::clearTraces();
    // event->reset() already resets mVG
    mAlphaLissage.reset();
}

void ModelChronocurve::setThresholdToAllModel(const double threshold)
{
    Model::setThresholdToAllModel(threshold);
    
    for (Event*& event : mEvents)
        event->mVG.mThresholdUsed = mThreshold;

    mAlphaLissage.mThresholdUsed = mThreshold;
}






QList<PosteriorMeanGComposante> ModelChronocurve::getChainsMeanGComposanteX()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gx);

    return composantes;
}

QList<PosteriorMeanGComposante> ModelChronocurve::getChainsMeanGComposanteY()
{
    QList<PosteriorMeanGComposante> composantes;

    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gy);

    return composantes;
}

QList<PosteriorMeanGComposante> ModelChronocurve::getChainsMeanGComposanteZ()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gz);

    return composantes;
}
