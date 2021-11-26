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

#ifndef MODELCURVE_H
#define MODELCURVE_H

#include "Model.h"
#include "CurveUtilities.h"
#include "CurveSettings.h"
#include "Matrix.h"


class ModelCurve: public Model
{
public:
    ModelCurve();
    virtual ~ModelCurve();
    
    virtual void saveToFile(QDataStream *out);
    virtual void restoreFromFile(QDataStream *in);

    virtual QJsonObject toJson() const;
    virtual void fromJson( const QJsonObject& json);
    
    virtual void generateResultsLog();

    void generatePosteriorDensities(const QList<ChainSpecs> &chains, int fftLen, double bandwidth);
    virtual void generateCorrelations(const QList<ChainSpecs> &chains);
    void generateNumericalResults(const QList<ChainSpecs> &chains);
    virtual void generateCredibility(const double &thresh);
    void generateHPD(const double thresh);
    
    virtual void clearThreshold();
    void clearPosteriorDensities();
    void clearCredibilityAndHPD();
    void clearTraces();
    
    virtual void setThresholdToAllModel(const double threshold);
    
    QList<PosteriorMeanGComposante> getChainsMeanGComposanteX();
    QList<PosteriorMeanGComposante> getChainsMeanGComposanteY();
    QList<PosteriorMeanGComposante> getChainsMeanGComposanteZ();


public:
    CurveSettings mCurveSettings;
    
    MHVariable mLambdaSpline;
    MCMCSpline mSpline; // valeurs courrantes de la spline

    std::vector<MCMCSpline> mSplinesTrace; // memo des valeurs aux noeuds
    
    PosteriorMeanG mPosteriorMeanG; // valeurs en tout t
    std::vector<PosteriorMeanG> mPosteriorMeanGByChain; // valeurs en tout t par chaine

};

#endif
