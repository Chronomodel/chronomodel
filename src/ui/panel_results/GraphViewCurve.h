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

    inline QList<CurveRefPts> eventsPoints() const { return mEventsPoints;};
    inline QList<CurveRefPts> dataPoints() const { return mDataPoints;};
    
    inline void setScale(Scale & scale) {mScale = scale;};
    inline Scale scale() const {return mScale;} ;

    virtual void generateCurves(const graph_t typeGraph, const QList<variable_t> &variableList);
    void updateCurvesToShowForG(bool showAllChains, QList<bool> showChainList, const QList<variable_t>& showVariableList, const Scale scale);

private:
    PosteriorMeanGComposante mComposanteG;
    QList<PosteriorMeanGComposante> mComposanteGChains;

    QList<CurveRefPts> mEventsPoints;
    QList<CurveRefPts> mDataPoints;
    Scale mScale;

    CurveMap densityMap_2_hpdMap (const CurveMap& densityMap, int nb_iter);
    virtual void updateCurves(const graph_t typeGraph,
                              const QList<variable_t> &showList,
                              bool showAllChains,
                              const QList<bool> &showChainList);
    virtual void updateStatHTML();
};

#endif
