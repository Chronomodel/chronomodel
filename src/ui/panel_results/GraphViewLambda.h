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

#ifndef GRAPHVIEWLAMBDA_H
#define GRAPHVIEWLAMBDA_H

#include "GraphViewResults.h"

class ModelCurve;

class GraphViewLambda: public GraphViewResults
{
    Q_OBJECT
public:
    explicit GraphViewLambda(QWidget* parent = nullptr);
    virtual ~GraphViewLambda();

    virtual void generateCurves(const graph_t typeGraph, const QList<variable_t>& showList);
    void updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QList<variable_t>& showList);

/*protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* );
*/
private:
    void generatePosterior();
    void generateHistory();
    void generateAcceptation();
    void generateCorrelation();
    void updateStatHTML();
};

#endif
