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
#ifndef CurveUTILITIES_H
#define CurveUTILITIES_H

#include "Matrix.h"
#include "Event.h"

#include <vector>
#include <string>
#include <QDataStream>


struct SilvermanParam
{
    bool use_error_measure = true;
    bool lambda_fixed = false;
    double log_lambda_value = -6;
    bool force_positive_curve = false;

    std::map<double, double> tab_CV;
    std::map<double, double> tab_GCV;
};

typedef struct SplineMatrices
{
    MatrixDiag diagWInv;
    Matrix2D matR;
    Matrix2D matQ;
    Matrix2D matQT;
    Matrix2D matQTW_1Q;
    Matrix2D matQTQ;

} SplineMatrices;

typedef struct SplineResults
{
    std::vector<double> vecG;
    std::vector<double> vecGamma;
    
} SplineResults;

typedef struct MCMCSplineComposante
{
    std::vector<double> vecThetaReduced; // le noeud ti reduce
    std::vector<double> vecG;
    std::vector<double> vecGamma;
    std::vector<double> vecVarG;
    void clear() {
        vecThetaReduced.clear();
        vecG.clear();
        vecGamma.clear();
        vecVarG.clear();
    }
} MCMCSplineComposante;

QDataStream &operator<<( QDataStream& stream, const MCMCSplineComposante& spline );
QDataStream &operator>>( QDataStream& stream, MCMCSplineComposante& spline );

typedef struct MCMCSpline
{
    MCMCSplineComposante splineX;
    MCMCSplineComposante splineY;
    MCMCSplineComposante splineZ;
    void clear() {
        splineX.clear();
        splineY.clear();
        splineZ.clear();
    }
    
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

    // spline density
    CurveMap mapG;
    CurveMap mapGP;
    void clear() {
        vecG.clear();
        vecGP.clear();
        vecGS.clear();
        vecVarG.clear();
        vecVarianceG.clear();
        vecVarErrG.clear();
        mapG.clear();
        mapGP.clear();
    }
    
} PosteriorMeanGComposante;

QDataStream &operator<<( QDataStream& stream, const PosteriorMeanGComposante& pMGComposante );
QDataStream &operator>>( QDataStream& stream, PosteriorMeanGComposante& pMGComposante );

typedef struct PosteriorMeanG
{
    PosteriorMeanGComposante gx;
    PosteriorMeanGComposante gy;
    PosteriorMeanGComposante gz;
    void clear() {
        gx.clear();
        gy.clear();
        gz.clear();
    }
} PosteriorMeanG;

QDataStream &operator<<( QDataStream &stream, const PosteriorMeanG& pMeanG );
QDataStream &operator>>( QDataStream &stream, PosteriorMeanG& pMeanG );

std::vector<t_reduceTime> calculVecH(const QList<Event *> &event);

Matrix2D calculMatR(const std::vector<t_reduceTime>& rVecH);
Matrix2D calculMatQ(const std::vector<t_reduceTime> &rVecH);

std::vector<double> createDiagWInv(const QList<Event *> &events);


void conversionIDF(PosteriorMeanG& G);
PosteriorMeanG conversionIDF(const std::vector<double> &vecGx, const std::vector<double> &vecGy, const std::vector<double> &vecGz, const std::vector<double> &vecGxErr, const std::vector<double> &vecGyErr, const std::vector<double> &vecGzErr);

void conversionID(PosteriorMeanG& G);
PosteriorMeanG conversionID(const std::vector<double> &vecGx, const std::vector<double> &vecGy, const std::vector<double> &vecGz, const std::vector<double> &vecGxErr, const std::vector<double> &vecGyErr, const std::vector<double> &vecGzErr);

class CurveUtilities
{
public:
    std::vector<double> definitionNoeuds(const std::vector<double>& tabPts, const double minStep); // Obsolete

};

#pragma mark Calcul Spline on Event

MCMCSpline currentSpline (QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const SplineMatrices &matrices, const double lambda, bool doY, bool doZ);
//MCMCSpline currentSpline_WI (QList<Event *> &events, bool doSortAndSpreadTheta = false, const std::vector<t_reduceTime> &vecH = std::vector<t_reduceTime>(), const SplineMatrices &matrices = SplineMatrices(), bool doY, bool doZ, bool use_error);
MCMCSpline currentSpline_WI (QList<Event *> &events, bool doY, bool doZ, bool use_error);

SplineMatrices prepareCalculSpline(const QList<Event *> & sortedEvents, const std::vector<t_reduceTime> &vecH);
//SplineMatrices prepareCalculSpline_WI(const QList<Event *> & sortedEvents, const std::vector<t_reduceTime> &vecH);



SplineResults do_spline(const std::function <double (Event*)> &fun, const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline);

SplineResults doSplineX(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline);
SplineResults doSplineY(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline);
SplineResults doSplineZ(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline);

std::vector<double> calculMatInfluence_origin(const SplineMatrices &matrices, const int nbBandes, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambda);
std::vector<double> doSplineError_origin(const SplineMatrices &matrices, const SplineResults &splines, const double lambdaSpline);

std::vector<double> calcul_spline_variance(const SplineMatrices &matrices, const QList<Event *> &events, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline);

double valeurG(const double t, const MCMCSplineComposante& spline, unsigned long &i0, Model &model);
double valeurErrG(const double t, const MCMCSplineComposante& spline, unsigned& i0, Model &model);
double valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0, Model &model);
double valeurGSeconde(const double t, const MCMCSplineComposante& spline, Model &model);

void valeurs_G_VarG_GP_GS(const double t, const MCMCSplineComposante &spline, double& G, double& varG, double& GP, double& GS, unsigned& i0, double tmin, double tmax);


#pragma mark Calcul Spline on vector Y

SplineMatrices prepareCalculSpline_WI(const std::vector<t_reduceTime> &vecH);
SplineMatrices prepare_calcul_spline(const std::vector<t_reduceTime> &vecH, const std::vector<double> W_1);

std::vector<double> calcul_spline_variance(const SplineMatrices &matrices, const std::vector<double> &vec_W, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambda);
std::vector<QMap<double, double>> composante_to_curve(MCMCSplineComposante spline_compo, double tmin, double tmax, double step);

SplineResults do_spline(const std::vector<double> &vec_Y, const SplineMatrices &matrices,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline);

std::pair<MCMCSpline, std::pair<double, double> > do_spline_composante(const std::vector<double> &vec_t, const std::vector<double> &vec_X, const std::vector<double> &vec_X_err, double tmin, double tmax, SilvermanParam &sv, const std::vector<double> &vec_Y = std::vector<double>(), const std::vector<double> &vec_Y_err = std::vector<double>(), const std::vector<double> &vec_Z = std::vector<double>(), const std::vector<double> &vec_Z_err = std::vector<double>()) ;

double cross_validation (const std::vector< double>& vec_Y, const SplineMatrices& matrices, const std::vector< double>& vecH, const double lambda);
double general_cross_validation (const std::vector< double>& vec_Y,  const SplineMatrices& matrices, const std::vector<double>& vecH, const double lambda);
double RSS(const std::vector<double> &vec_Y, const SplineMatrices &matrices, const std::vector<t_reduceTime> &vecH, const double lambda);

std::pair<double, double> initLambdaSplineByCV(const bool depth, const std::vector< double> &vec_X, const std::vector< double> &vec_X_err, const SplineMatrices &matrices, const std::vector< double> &vecH, const std::vector<double> &vec_Y = std::vector< double>(), const std::vector<double> &vec_Y_err = std::vector< double>(), const std::vector<double> &vec_Z = std::vector< double>(), const std::vector<double> &vec_Z_err = std::vector< double>());
std::pair<double, double> initLambdaSplineBySilverman(SilvermanParam &sv, const std::vector< double> &vec_X, const std::vector< double> &vec_X_err, const std::vector< double> &vecH, const std::vector<double> &vec_Y = std::vector< double>(), const std::vector<double> &vec_Y_err = std::vector< double>(), const std::vector<double> &vec_Z = std::vector< double>(), const std::vector<double> &vec_Z_err = std::vector< double>());


double initLambdaSplineByCV_VgFixed_old(const std::vector< double> &vec_Y, const std::vector< double> &vec_Y_err, const std::vector< double> &vecH, const double Vg);
double initLambdaSplineByCV_VgFixed(const double Vg, const bool depth, const std::vector< double> &vec_X, const std::vector< double> &vec_X_err, const SplineMatrices &matrices, const std::vector< double> &vecH, const std::vector<double> &vec_Y = std::vector< double>(), const std::vector<double> &vec_Y_err = std::vector< double>(), const std::vector<double> &vec_Z = std::vector< double>(), const std::vector<double> &vec_Z_err = std::vector< double>());

double general_cross_validation (const std::vector< double>& vec_Y,  const SplineMatrices& matrices, const std::vector<double>& vecH, const double lambdaSpline);
double var_residual(const std::vector<double> &vec_Y,const SplineMatrices &matrices,const std::vector<t_reduceTime> &vecH, const double lambda);
std::vector<double> general_residual (const std::vector<double>& vec_Y,  const SplineMatrices& matrices, const std::vector<double>& vecH, const double lambda);

bool  hasPositiveGPrimeByDet (const MCMCSplineComposante &splineComposante);
void spread_theta_reduced(std::vector<double> &sorted_t_red, double spread_span = 0.);
std::vector<int> get_order(const std::vector<double> &vec);

#endif
