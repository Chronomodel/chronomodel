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

#include "ModelCurve.h"
#include "ModelUtilities.h"
#include "QFile"
#include "qapplication.h"
#include "Project.h"

#include "qdatastream.h"


ModelCurve::ModelCurve():Model()
{
    mLambdaSpline.mSupport = MetropolisVariable::eR;
    mLambdaSpline.mFormat = DateUtils::eNumeric;
    mLambdaSpline.mSamplerProposal = MHVariable::eMHAdaptGauss;
}

ModelCurve::~ModelCurve()
{

}

QJsonObject ModelCurve::toJson() const
{
    QJsonObject json = Model::toJson();
    
    json[STATE_CURVE] = mCurveSettings.toJson();
    
    return json;
}

void ModelCurve::fromJson(const QJsonObject& json)
{
    Model::fromJson(json);
    
    if (json.contains(STATE_CURVE)) {
        const QJsonObject settings = json.value(STATE_CURVE).toObject();
        mCurveSettings = CurveSettings::fromJson(settings);
    }

    for (Event*& event: mEvents) {
        if (event->type() ==  Event::eKnown ||
            mCurveSettings.mTimeType == CurveSettings::eModeFixed)
               event->mTheta.mSamplerProposal = MHVariable::eFixe;

        else if (event->type() ==  Event::eDefault)
                event->mTheta.mSamplerProposal = MHVariable::eMHAdaptGauss;

        if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
            event->mVG.mSamplerProposal = MHVariable::eFixe;
        else
            event->mVG.mSamplerProposal = MHVariable::eMHAdaptGauss;

    }

    if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed)
        mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
    else
        mLambdaSpline.mSamplerProposal = MHVariable::eMHAdaptGauss;

}

// Date files read / write
/** @Brief Save .res file, the result of computation and compress it
*
* */
void ModelCurve::saveToFile(QDataStream *out)
{
    Model::saveToFile(out);


    /* -----------------------------------------------------
    *   Write curve data
    * ----------------------------------------------------- */
    // -----------------------------------------------------
    //  Write events VG
    // -----------------------------------------------------

    for (Event*& event : mEvents)
        *out << event->mVG;

    *out << mLambdaSpline;

    *out << (quint32) mSplinesTrace.size();
    for (auto& splin : mSplinesTrace)
        *out << splin;

    *out << mPosteriorMeanG;

    *out << (quint32) mPosteriorMeanGByChain.size();
    for (auto& pMByChain : mPosteriorMeanGByChain)
        *out << pMByChain;



}
/** @Brief Read the .res file, it's the result of the saved computation
*
* */
void ModelCurve::restoreFromFile(QDataStream* in)
{

    Model::restoreFromFile(in);

    generateCorrelations(mChains);

    /* -----------------------------------------------------
    *  Read events VG
    *----------------------------------------------------- */

    for (auto&& e : mEvents)
        *in >> e->mVG;

    /* -----------------------------------------------------
    *   Read curve data
    * ----------------------------------------------------- */
    quint32 tmp32;
    *in >> mLambdaSpline;

    *in >> tmp32;
    mSplinesTrace.resize(tmp32);
    for (auto& splin : mSplinesTrace)
        *in >> splin;

    *in >> mPosteriorMeanG;

    *in >> tmp32;
    mPosteriorMeanGByChain.resize(tmp32);
    for (auto& pMByChain : mPosteriorMeanGByChain)
        *in >> pMByChain;


}

void  ModelCurve::generateResultsLog()
{
    Model::generateResultsLog();

    QString log;
    log += ModelUtilities::curveResultsHTML(this);
    log += "<hr>";

    mLogResults += log;
}

void ModelCurve::generatePosteriorDensities(const QList<ChainSpecs> &chains, int fftLen, double bandwidth)
{
    Model::generatePosteriorDensities(chains, fftLen, bandwidth);

    for (Event*& event : mEvents) {
        event->mVG.updateFormatedTrace();
        event->mVG.generateHistos(chains, fftLen, bandwidth);
    }

    mLambdaSpline.updateFormatedTrace();
    mLambdaSpline.generateHistos(chains, fftLen, bandwidth);
}

void ModelCurve::generateCorrelations(const QList<ChainSpecs> &chains)
{
    Model::generateCorrelations(chains);
    for (auto&& event : mEvents )
        event->mVG.generateCorrelations(chains);

    mLambdaSpline.generateCorrelations(chains);
}

void ModelCurve::generateNumericalResults(const QList<ChainSpecs> &chains)
{
    Model::generateNumericalResults(chains);
    for (Event*& event : mEvents)
        event->mVG.generateNumericalResults(chains);

    mLambdaSpline.generateNumericalResults(chains);
}

void ModelCurve::clearThreshold()
{
    Model::clearThreshold();
   // mThreshold = -1.;
    for (Event*& event : mEvents)
        event->mVG.mThresholdUsed = -1.;

    mLambdaSpline.mThresholdUsed = -1.;
}

void ModelCurve::generateCredibility(const double& thresh)
{
    Model::generateCredibility(thresh);
    for (Event*& event : mEvents) {
        event->mVG.generateCredibility(mChains, thresh);
    }
    mLambdaSpline.generateCredibility(mChains, thresh);
}

void ModelCurve::generateHPD(const double thresh)
{
    Model::generateHPD(thresh);
    for (Event*& event : mEvents) {
        if (event->type() != Event::eKnown) {
            event->mVG.generateHPD(thresh);
        }
    }
    
    mLambdaSpline.generateHPD(thresh);
}

void ModelCurve::clearPosteriorDensities()
{
    Model::clearPosteriorDensities();
    
    for (Event*& event : mEvents) {
        if (event->type() != Event::eKnown) {
            event->mVG.mHisto.clear();
            event->mVG.mChainsHistos.clear();
        }
    }
    
    mLambdaSpline.mHisto.clear();
    mLambdaSpline.mChainsHistos.clear();
}

void ModelCurve::clearCredibilityAndHPD()
{
    Model::clearCredibilityAndHPD();
    
    for (Event*& event : mEvents) {
        if (event->type() != Event::eKnown) {
            event->mVG.mHPD.clear();
            event->mVG.mCredibility = QPair<double, double>();
        }
    }
    
    mLambdaSpline.mHPD.clear();
    mLambdaSpline.mCredibility = QPair<double, double>();
}

void ModelCurve::clearTraces()
{
    Model::clearTraces();
  /*  for (Event*& event : mEvents)
        event->mVG.reset();
*/
    mLambdaSpline.reset();
}


void ModelCurve::setThresholdToAllModel(const double threshold)
{
    Model::setThresholdToAllModel(threshold);
    
    for (Event*& event : mEvents)
        event->mVG.mThresholdUsed = mThreshold;

    mLambdaSpline.mThresholdUsed = mThreshold;
}



QList<PosteriorMeanGComposante> ModelCurve::getChainsMeanGComposanteX()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gx);

    return composantes;
}

QList<PosteriorMeanGComposante> ModelCurve::getChainsMeanGComposanteY()
{
    QList<PosteriorMeanGComposante> composantes;

    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gy);

    return composantes;
}

QList<PosteriorMeanGComposante> ModelCurve::getChainsMeanGComposanteZ()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gz);

    return composantes;
}
