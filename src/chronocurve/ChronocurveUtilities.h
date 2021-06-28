/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2021

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
#ifndef CHRONOCURVEUTILITIES_H
#define CHRONOCURVEUTILITIES_H

#include "Functions.h"

#include <vector>
#include <string>
#include <QDataStream>


typedef struct SplineMatrices
{
    std::vector<long double> diagWInv;
    std::vector<std::vector<long double>> matR;
    std::vector<std::vector<long double>> matQ;
    std::vector<std::vector<long double>> matQT;
    std::vector<std::vector<long double>> matQTW_1Q;
    std::vector<std::vector<long double>> matQTQ;
} SplineMatrices;

typedef struct SplineResults
{
    std::vector<std::vector<long double>> matB;
    std::vector<std::vector<long double>> matL;
    std::vector<long double> matD;
    
    std::vector<long double> vecG;
    std::vector<long double> vecGamma;
    
} SplineResults;

typedef struct MCMCSplineComposante
{
    std::vector<long double> vecThetaEvents; // le noeud ti
    std::vector<long double> vecG;
    std::vector<long double> vecGamma;
    std::vector<long double> vecVarG;
    
} MCMCSplineComposante;

QDataStream &operator<<( QDataStream& stream, const MCMCSplineComposante& spline );
QDataStream &operator>>( QDataStream& stream, MCMCSplineComposante& spline );

typedef struct MCMCSpline
{
    MCMCSplineComposante splineX;
    MCMCSplineComposante splineY;
    MCMCSplineComposante splineZ;
    
} MCMCSpline;

QDataStream &operator<<( QDataStream& stream, const MCMCSpline& spline );
QDataStream &operator>>( QDataStream& stream, MCMCSpline& spline );

typedef struct PosteriorMeanGComposante
{
    std::vector<long double> vecG;
    std::vector<long double> vecGP;
    std::vector<long double> vecGS;
    std::vector<long double> vecVarG;
    
} PosteriorMeanGComposante;

QDataStream &operator<<( QDataStream& stream, const PosteriorMeanGComposante& pMGComposante );
QDataStream &operator>>( QDataStream& stream, PosteriorMeanGComposante& pMGComposante );

typedef struct PosteriorMeanG
{
    PosteriorMeanGComposante gx;
    PosteriorMeanGComposante gy;
    PosteriorMeanGComposante gz;
    
} PosteriorMeanG;

QDataStream &operator<<( QDataStream &stream, const PosteriorMeanG& pMeanG );
QDataStream &operator>>( QDataStream &stream, PosteriorMeanG& pMeanG );

std::vector<long double> calculVecH(const std::vector<long double> &vec);
std::vector<std::vector<long double>> calculMatR(const std::vector<long double>& vec);
std::vector<std::vector<long double>> calculMatQ(const std::vector<long double>& vec);

void conversionIDF(PosteriorMeanG& G);
PosteriorMeanG conversionIDF(const std::vector<long double> &vecGx, const std::vector<long double> &vecGy, const std::vector<long double> &vecGz, const std::vector<long double> &vecGErr);

class ChronocurveUtilities
{
public:
    std::vector<long double> definitionNoeuds(const std::vector<long double>& tabPts, const double minStep);

};


#endif
