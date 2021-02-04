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


ModelChronocurve::ModelChronocurve():Model()
{

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
    {
        event->mMethod = Event::eMHAdaptGauss;
    }
    
    if (json.contains(STATE_CHRONOCURVE))
    {
        const QJsonObject settings = json.value(STATE_CHRONOCURVE).toObject();
        mChronocurveSettings = ChronocurveSettings::fromJson(settings);
    }
}

void ModelChronocurve::generatePosteriorDensities(const QList<ChainSpecs> &chains, int fftLen, double bandwidth)
{
    Model::generatePosteriorDensities(chains, fftLen, bandwidth);
    
    //const double tmin = mSettings.getTminFormated();
    //const double tmax = mSettings.getTmaxFormated();
    
    for(Event*& event : mEvents)
    {
        event->mVG.updateFormatedTrace();
        event->mVG.generateHistos(chains, fftLen, bandwidth);
    }
    
    //qDebug() << *(mAlphaLissage.mRawTrace);
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

    for (Event*& event : mEvents)
    {
        event->mVG.generateNumericalResults(chains);
    }
    mAlphaLissage.generateNumericalResults(chains);
}

void ModelChronocurve::clearThreshold()
{
    Model::clearThreshold();
    mThreshold = -1.;
    for (Event*& event : mEvents)
    {
        event->mVG.mThresholdUsed = -1.;
    }
    mAlphaLissage.mThresholdUsed = -1.;
}

void ModelChronocurve::generateCredibility(const double thresh)
{
    Model::generateCredibility(thresh);
    
    for (Event*& event : mEvents)
    {
        if(event->type() != Event::eKnown)
        {
            event->mVG.generateCredibility(mChains, thresh);
        }
    }
    mAlphaLissage.generateCredibility(mChains, thresh);
}

void ModelChronocurve::generateHPD(const double thresh)
{
    Model::generateHPD(thresh);
    
    for(Event*& event : mEvents)
    {
        if(event->type() != Event::eKnown)
        {
            event->mVG.generateHPD(thresh);
        }
    }
    
    mAlphaLissage.generateHPD(thresh);
}

void ModelChronocurve::clearPosteriorDensities()
{
    Model::clearPosteriorDensities();
    
    for(Event*& event : mEvents)
    {
        if(event->type() != Event::eKnown)
        {
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
    
    for(Event*& event : mEvents)
    {
        if(event->type() != Event::eKnown)
        {
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

void ModelChronocurve::setThresholdToAllModel()
{
    Model::setThresholdToAllModel();
    
    for (Event*& event : mEvents)
    {
        event->mVG.mThresholdUsed = mThreshold;
    }
    
    mAlphaLissage.mThresholdUsed = mThreshold;
}






QList<PosteriorMeanGComposante> ModelChronocurve::getChainsMeanGComposanteX()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for(unsigned int i=0; i<mPosteriorMeanGByChain.size(); ++i)
    {
        composantes.append(mPosteriorMeanGByChain[i].gx);
    }
    return composantes;
}

QList<PosteriorMeanGComposante> ModelChronocurve::getChainsMeanGComposanteY()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for(unsigned int i=0; i<mPosteriorMeanGByChain.size(); ++i)
    {
        composantes.append(mPosteriorMeanGByChain[i].gy);
    }
    return composantes;
}

QList<PosteriorMeanGComposante> ModelChronocurve::getChainsMeanGComposanteZ()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for(unsigned int i=0; i<mPosteriorMeanGByChain.size(); ++i)
    {
        composantes.append(mPosteriorMeanGByChain[i].gz);
    }
    return composantes;
}
