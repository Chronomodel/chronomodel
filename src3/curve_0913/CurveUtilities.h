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
#ifndef CurveUTILITIES_H
#define CurveUTILITIES_H

#include "Functions.h"
#include "Matrix.h"

#include <vector>
#include <string>
#include <QDataStream>



typedef struct SplineMatrices
{
    std::vector<double> diagWInv;
    Matrix2D matR;
    Matrix2D matQ;
    Matrix2D matQT;
    Matrix2D matQTW_1Q;
    Matrix2D matQTQ;
} SplineMatrices;

typedef struct SplineResults
{
    Matrix2D matB;
    Matrix2D matL;
    std::vector<double> matD;
    
    std::vector<double> vecG;
    std::vector<double> vecGamma;
    
} SplineResults;

typedef struct MCMCSplineComposante
{
    std::vector<double> vecThetaEvents; // le noeud ti
    std::vector<double> vecG;
    std::vector<double> vecGamma;
    std::vector<double> vecVarG;
    
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
    std::vector<double> vecG;
    std::vector<double> vecGP;
    std::vector<double> vecGS;
    std::vector<double> vecVarG;
    // inter spline error
    std::vector<double> vecVarianceG;
    // intra spline error
    std::vector<double> vecVarErrG;

    // inter derivate variance
    // std::vector<double> vecVarGP; // test

    // spline map
    CurveMap mapG;
    
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

std::vector<double> calculVecH(const std::vector< double> &vec);
Matrix2D calculMatR(const std::vector<double>& vec);
Matrix2D calculMatQ(const std::vector<double>& vec);

void conversionIDF(PosteriorMeanG& G);
PosteriorMeanG conversionIDF(const std::vector<double> &vecGx, const std::vector<double> &vecGy, const std::vector<double> &vecGz, const std::vector<double> &vecGxErr, const std::vector<double> &vecGyErr, const std::vector<double> &vecGzErr);

void conversionID(PosteriorMeanG& G);
PosteriorMeanG conversionID(const std::vector<double> &vecGx, const std::vector<double> &vecGy, const std::vector<double> &vecGz, const std::vector<double> &vecGxErr, const std::vector<double> &vecGyErr, const std::vector<double> &vecGzErr);

class CurveUtilities
{
public:
    std::vector<double> definitionNoeuds(const std::vector<double>& tabPts, const double minStep); // Obsolete

};


#endif