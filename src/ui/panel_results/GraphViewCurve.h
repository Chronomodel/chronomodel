/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#ifndef GRAPHVIEWCURVE_H
#define GRAPHVIEWCURVE_H

#include "GraphViewResults.h"
#include "GraphCurve.h"
#include "CurveUtilities.h"

class GraphViewCurve: public GraphViewResults
{
    Q_OBJECT
public:
    explicit GraphViewCurve(QWidget *parent = nullptr);
    virtual ~GraphViewCurve();

    void setComposanteG(const PosteriorMeanGComposante& composante);
    void setComposanteGChains(const QList<PosteriorMeanGComposante>& composanteChains);

    inline void setEventsPoints(const QList<CurveRefPts>& rfPts) { mEventsPoints = rfPts;};
    inline void setDataPoints(const QList<CurveRefPts>& rfPts) { mDataPoints = rfPts;};
    
    virtual void generateCurves(const graph_t typeGraph, const QList<variable_t> &variableList);
    void updateCurvesToShowForG(bool showAllChains, QList<bool> showChainList, const QList<variable_t>& showVariableList, const Scale scale);


private:
    PosteriorMeanGComposante mComposanteG;
    QList<PosteriorMeanGComposante> mComposanteGChains;

    QList<CurveRefPts> mEventsPoints;
    QList<CurveRefPts> mDataPoints;

    CurveMap densityMap_2_hpdMap (const CurveMap& densityMap, int nb_iter);
    void densityMap_2_thresholdIndices_optimized(const CurveMap& densityMap,
                                            double threshold,
                                            std::vector<int>& min_indices,
                                            std::vector<int>& max_indices);

};

#endif
