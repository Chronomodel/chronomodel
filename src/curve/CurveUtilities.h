/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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
#include <QDataStream>
#include <eigen_3.4.0/Eigen/Dense>



struct SilvermanParam
{
    // Enumération pour les types de lambda
    enum class lambda_type {
        Fixed,      // Type fixe
        Silverman,  // Type Silverman
        Kernel      // Type noyau
    };

    bool use_error_measure = true;         // Indique si la mesure d'erreur est utilisée
    double log_lambda_value = -6;           // Valeur logarithmique de lambda
    bool force_positive_curve = false;      // Force une courbe positive

    std::map<double, double> tab_CV;        // Tableau pour la validation croisée
    std::map<double, double> tab_GCV;       // Tableau pour la validation croisée généralisée
    std::string comment;                    // Commentaire associé

    lambda_type lambda_process = lambda_type::Silverman; // Type de processus lambda par défaut
};

typedef struct SplineMatricesLD
{
    DiagonalMatrixLD  diagWInv;
    SparseMatrixLD matR;
    SparseMatrixLD matQ;
    MatrixLD matQTW_1Q;
    MatrixLD matQTQ;

} SplineMatricesLD;

typedef struct SplineMatricesD
{
    DiagonalMatrixD  diagWInv;
    SparseMatrixD matR;
    SparseMatrixD matQ;
    MatrixD matQTW_1Q;
    MatrixD matQTQ;

} SplineMatricesD;

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
    void shrink_to_fit() {
        vecThetaReduced.shrink_to_fit();
        vecG.shrink_to_fit();
        vecGamma.shrink_to_fit();
        vecVarG.shrink_to_fit();
    }
    void clear_and_shrink() {
        vecThetaReduced.clear();
        vecG.clear();
        vecGamma.clear();
        vecVarG.clear();

        vecThetaReduced.shrink_to_fit();
        vecG.shrink_to_fit();
        vecGamma.shrink_to_fit();
        vecVarG.shrink_to_fit();
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
    void shrink_to_fit() {
        splineX.shrink_to_fit();
        splineY.shrink_to_fit();
        splineZ.shrink_to_fit();
    }
    void clear_and_shrink() {
        splineX.clear_and_shrink();
        splineY.clear_and_shrink();
        splineZ.clear_and_shrink();
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
    void shrink_to_fit() {
        vecG.shrink_to_fit();
        vecGP.shrink_to_fit();
        vecGS.shrink_to_fit();
        vecVarG.shrink_to_fit();
        vecVarianceG.shrink_to_fit();
        vecVarErrG.shrink_to_fit();
    }
    void clear_and_shrink() {
        vecG.clear();
        vecGP.clear();
        vecGS.clear();
        vecVarG.clear();
        vecVarianceG.clear();
        vecVarErrG.clear();

        vecG.shrink_to_fit();
        vecGP.shrink_to_fit();
        vecGS.shrink_to_fit();
        vecVarG.shrink_to_fit();
        vecVarianceG.shrink_to_fit();
        vecVarErrG.shrink_to_fit();

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
    void shrink_to_fit() {
        gx.shrink_to_fit();
        gy.shrink_to_fit();
        gz.shrink_to_fit();
    }
    void clear_and_shrink() {
        gx.clear_and_shrink();
        gy.clear_and_shrink();
        gz.clear_and_shrink();
    }
} PosteriorMeanG;

QDataStream &operator<<( QDataStream &stream, const PosteriorMeanG& pMeanG );
QDataStream &operator>>( QDataStream &stream, PosteriorMeanG& pMeanG );

std::vector<t_reduceTime> calculVecH(const std::vector<std::shared_ptr<Event>> &event);

MatrixLD calculMatR0(const std::vector<t_reduceTime>& vec_h);
//MatrixLD calculMatR(const std::vector<t_reduceTime>& vec_h);

SparseMatrixLD calculMatR_LD(const std::vector<t_reduceTime>& vec_h);
SparseMatrixD calculMatR_D(const std::vector<t_reduceTime>& vec_h);

MatrixLD calculMatQ00(const std::vector<t_reduceTime>& vec_h);
MatrixLD calculMatQ0(const std::vector<t_reduceTime>& vec_h);

SparseMatrixLD calculMatQ_LD(const std::vector<t_reduceTime>& vec_h);
SparseMatrixD calculMatQ_D(const std::vector<t_reduceTime>& vec_h);

std::pair<SparseMatrixLD, SparseMatrixLD> calculMatQR_LD(const std::vector<t_reduceTime>& vec_h);
std::pair<SparseMatrixD, SparseMatrixD> calculMatQR_D(const std::vector<t_reduceTime>& vec_h);

MatrixLD computeMatA_direct(const MatrixLD& Q, const MatrixLD& B1, const DiagonalMatrixLD& W1_diag, double lambda);
MatrixLD computeMatA_optimized_kahan(const MatrixLD& Q, const MatrixLD& B_1, const DiagonalMatrixLD& W_1, double lambda);
MatrixLD computeB_1_from_Q_W1_R_direct(const MatrixLD& Q, const DiagonalMatrixLD& W_1, const MatrixLD& R, double lambda);
MatrixLD compute_AxBxAT(const MatrixLD& A, const MatrixLD& B);

void convertToXYZ(double Inc, double Dec, double F, double& x, double& y, double& z);
void convertToIDF(double x, double y, double z, double& Inc, double& Dec, double& F);
void computeDerivatives(double gx, double gy, double gz, double gpx, double gpy, double gpz,
                        double& dIncdt, double& dDecdt, double& dFdt);
void computeSecondDerivatives(double gx, double gy, double gz, double gpx, double gpy, double gpz,
    double gamma_x, double gamma_y, double gamma_z,
    double& d2Inct2, double& d2Decdt2, double& d2Fdt2);

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

MCMCSpline currentSpline (std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const SplineMatricesLD &matrices, const double lambda, bool doY, bool doZ);
MCMCSpline currentSpline (std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const SplineMatricesD &matrices, const double lambda, bool doY, bool doZ);
//MCMCSpline currentSpline_WI (QList<Event *> &events, bool doSortAndSpreadTheta = false, const std::vector<t_reduceTime> &vecH = std::vector<t_reduceTime>(), const SplineMatricesLD &matrices = SplineMatricesLD(), bool doY, bool doZ, bool use_error);
MCMCSpline currentSpline_WI (std::vector<std::shared_ptr<Event> > &events, bool doY, bool doZ, bool use_error);

SplineMatricesLD prepare_calcul_spline_LD(const std::vector<std::shared_ptr<Event> >& sortedEvents, const std::vector<t_reduceTime> &vecH);
SplineMatricesD prepare_calcul_spline_D(const std::vector<std::shared_ptr<Event> >& sortedEvents, const std::vector<t_reduceTime> &vecH);

SplineMatricesLD prepare_calcul_spline_LD(const std::vector<t_reduceTime>& vecH, const DiagonalMatrixLD& W_1);
SplineMatricesD prepare_calcul_spline_D(const std::vector<t_reduceTime>& vecH, const DiagonalMatrixD& W_1);

SplineMatricesLD prepare_calcul_spline_WI_LD(const std::vector<t_reduceTime>& vecH);
SplineMatricesD prepare_calcul_spline_WI_D(const std::vector<t_reduceTime>& vecH);


SplineMatricesLD update_splineMatrice_with_vecH(SplineMatricesLD spline_matrices, const std::vector<t_reduceTime>& vecH);
SplineMatricesLD update_splineMatrice_with_mW(SplineMatricesLD spline_matrices, const std::vector<std::shared_ptr<Event> >& sortedEvents);

SplineMatricesLD prepareCalculSpline_Sy2(const std::vector<std::shared_ptr<Event>> &sortedEvents, const std::vector<t_reduceTime> &vecH);
//SplineMatricesLD prepareCalculSpline_WI(const QList<Event *> & sortedEvents, const std::vector<t_reduceTime> &vecH);



SplineResults do_spline(const std::function<t_matrix (std::shared_ptr<Event>)> &fun, const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_B, const double lambdaSpline);
SplineResults do_spline(const std::vector<double>& vec_Y, const SplineMatricesD& matrices, const std::vector<t_reduceTime>& vecH, const std::pair<MatrixD, DiagonalMatrixD> &decomp, const double lambdaSpline);

SplineResults doSplineX(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event>> &events, const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_B, const double lambdaSpline);
SplineResults doSplineY(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD> &decomp_B, const double lambdaSpline);
SplineResults doSplineZ(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event>> &events, const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_B, const double lambdaSpline);

SplineResults doSplineX(const SplineMatricesD &matrices, const std::vector<std::shared_ptr<Event>> &events, const std::vector<t_reduceTime> &vecH, const std::pair<MatrixD, DiagonalMatrixD > &decomp_B, const double lambdaSpline);
SplineResults doSplineY(const SplineMatricesD &matrices, const std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const std::pair<MatrixD, DiagonalMatrixD> &decomp_B, const double lambdaSpline);
SplineResults doSplineZ(const SplineMatricesD &matrices, const std::vector<std::shared_ptr<Event>> &events, const std::vector<t_reduceTime> &vecH, const std::pair<MatrixD, DiagonalMatrixD > &decomp_B, const double lambdaSpline);

DiagonalMatrixLD diagonal_influence_matrix_old(const SplineMatricesLD &matrices, const int nbBandes, const std::pair<MatrixLD, DiagonalMatrixLD> &decomp, const double lambda);

DiagonalMatrixLD diagonal_influence_matrix(const SplineMatricesLD &matrices, const int nbBandes, const std::pair<MatrixLD, DiagonalMatrixLD> &decomp, const double lambda);
DiagonalMatrixD diagonal_influence_matrix(const SplineMatricesD& matrices, const int nbBandes, const std::pair<MatrixD, DiagonalMatrixD> &decomp, const double lambda);

std::vector<double> doSplineError_origin(const SplineMatricesLD &matrices, const SplineResults &splines, const double lambdaSpline);

std::vector<double> calcul_spline_variance(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event> > &events, const std::pair<MatrixLD, DiagonalMatrixLD> &decomp, const double lambdaSpline);
std::vector<double> calcul_spline_variance(const SplineMatricesD& matrices, const std::vector<std::shared_ptr<Event>> &events, const std::pair<MatrixD, DiagonalMatrixD> &decomp, const double lambdaSpline);

double valeurG(const double t, const MCMCSplineComposante& spline, unsigned long &i0, Model &model);
double valeurErrG(const double t, const MCMCSplineComposante& spline, unsigned& i0, Model &model);
double valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0, Model &model);
double valeurGSeconde(const double t, const MCMCSplineComposante& spline, Model &model);

void valeurs_G_VarG_GP_GS(const double t, const MCMCSplineComposante &spline, double& G, double& varG, double& GP, double& GS, unsigned& i0, double tmin, double tmax);

void valeurs_G_GP_GS(const double t, const MCMCSplineComposante &spline, double& G, double& GP, double& GS, unsigned& i0, double tmin, double tmax); // used for version 3.3.5


#pragma mark Calcul Spline on vector Y

std::vector<t_matrix> do_vec_gamma(const std::vector<t_matrix>& vec_Y, const std::vector<t_reduceTime>& vec_H, const std::pair<MatrixLD, DiagonalMatrixLD>& decomp);
std::vector<t_matrix> do_vec_G(const SplineMatricesLD& matrices, const std::vector<t_matrix> &vec_Gamma, const std::vector<double> &vec_Y, const double lambdaSpline);


std::vector<double> do_vec_gamma(const std::vector<double>& vec_Y, const std::vector<t_reduceTime>& vec_H, const std::pair<MatrixD, DiagonalMatrixD>& decomp);
std::vector<double> do_vec_G(const SplineMatricesD& matrices, const std::vector<double> &vec_Gamma, const std::vector<double> &vec_Y, const double lambdaSpline);

std::vector<double> calcul_spline_variance(const SplineMatricesLD &matrices, const std::vector<double> &vec_W, const std::pair<MatrixLD, DiagonalMatrixLD> &decomp, const double lambda);
std::vector<QMap<double, double>> composante_to_curve(MCMCSplineComposante spline_compo, double tmin, double tmax, double step);

SplineResults do_spline(const std::vector<t_matrix> &vec_Y, const SplineMatricesLD &matrices,  const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp, const double lambdaSpline);
SplineResults do_spline(const std::vector<double>& vec_Y, const SplineMatricesD& matrices, const std::vector<t_reduceTime>& vecH, const std::pair<MatrixD, DiagonalMatrixD> &decomp, const double lambdaSpline);

std::pair<MCMCSpline, std::pair<double, double> > do_spline_kernel_composante(const std::vector<double> &vec_t, const std::vector<double> &vec_X, const std::vector<double> &vec_X_err, double tmin, double tmax, SilvermanParam &sv, const std::vector<double> &vec_Y = std::vector<double>(), const std::vector<double> &vec_Y_err = std::vector<double>(), const std::vector<double> &vec_Z = std::vector<double>(), const std::vector<double> &vec_Z_err = std::vector<double>()) ;

std::pair<MCMCSpline, std::pair<double, double> > do_spline_composante(const std::vector<double> &vec_t, const std::vector<double> &vec_X, const std::vector<double> &vec_X_err, double tmin, double tmax, SilvermanParam &sv, const std::vector<double> &vec_Y = std::vector<double>(), const std::vector<double> &vec_Y_err = std::vector<double>(), const std::vector<double> &vec_Z = std::vector<double>(), const std::vector<double> &vec_Z_err = std::vector<double>()) ;

long double compensated_sum(const std::vector<long double>& values);
long double cross_validation (const std::vector<t_matrix> &vec_Y, const SplineMatricesLD& matrices, const std::vector<t_reduceTime> &vecH, const double lambda);
long double general_cross_validation (const std::vector<t_matrix>& vec_Y,  const SplineMatricesLD& matrices, const std::vector<t_reduceTime>& vecH, const double lambda);

// pour la spline cubique « standard » de Wahba / Green & Silverman (smoothing spline), la base non pénalisée est {1,x}, donc  p=2.
long double restricted_likelihood (const std::vector<t_matrix> &vec_Y, const SplineMatricesLD& matrices, const std::vector<t_reduceTime> &vecH, const double lambda, const size_t p = 2);  // dimension de la partie polynomiale

double RSS(const std::vector<t_matrix> &vec_Y, const SplineMatricesLD &matrices, const std::vector<t_reduceTime> &vecH, const double lambda);

std::pair<double, double> initLambdaSplineByCV(const bool depth, const std::vector<t_matrix> &vec_X, const std::vector<t_matrix> &vec_X_err, const SplineMatricesLD &matrices, const std::vector<t_reduceTime> &vecH, const std::vector<t_matrix> &vec_Y = std::vector<t_matrix>(), const std::vector<t_matrix> &vec_Z = std::vector<t_matrix>(), const std::vector<t_matrix> &vec_Y_err = std::vector<t_matrix>(), const std::vector<t_matrix> &vec_Z_err = std::vector<t_matrix>());
std::pair<double, double> initLambdaSplineBySilverman(SilvermanParam &sv, const std::vector<double> &vec_X, const std::vector<double> &vec_X_err, const std::vector<t_reduceTime> &vecH, const std::vector<double> &vec_Y = std::vector<double>(), const std::vector<double> &vec_Y_err = std::vector<double>(), const std::vector<double> &vec_Z = std::vector<double>(), const std::vector<double> &vec_Z_err = std::vector<double>());

double var_residual(const std::vector<t_matrix> &vec_Y, const SplineMatricesLD &matrices, const std::vector<t_reduceTime> &vecH, const double lambda);
double var_residual(const std::vector<double>& vec_Y, const SplineMatricesD& matrices, const std::vector<t_reduceTime>& vecH, const double lambda);
std::vector<double> general_residual (const std::vector<t_matrix> &vec_Y, const SplineMatricesLD& matrices, const std::vector<t_reduceTime> &vecH, const double lambda);

double var_Gasser(const std::vector<double>& vec_t, const std::vector<double>& vec_Y);
t_matrix var_Gasser(const std::vector<t_matrix>& vec_t, const std::vector<long double>& vec_Y);

t_matrix var_Gasser(const std::vector<t_matrix>& vec_t, const MatrixLD& Y_mat);
double var_Gasser(const std::vector<double>& vec_t, const MatrixD& Y_mat);

double var_Gasser_2D(const std::vector<double>& vec_t, const std::vector<double>& vec_X, const std::vector<double>& vec_Y);
t_matrix var_Gasser_2D(const std::vector<t_matrix>& vec_t, const std::vector<t_matrix>& vec_X, const std::vector<t_matrix>& vec_Y);

double var_Gasser_3D(const std::vector<double> &vec_t, const std::vector<double>& vec_X, const std::vector<double> &vec_Y, const std::vector<double> &vec_Z);
t_matrix var_Gasser_3D(const std::vector<t_matrix> &vec_t, const std::vector<t_matrix>& vec_X, const std::vector<t_matrix> &vec_Y, const std::vector<t_matrix> &vec_Z);



bool  hasPositiveGPrimeByDet (const MCMCSplineComposante &splineComposante);
void spread_theta_reduced(std::vector<t_reduceTime> &sorted_t_red, t_reduceTime spread_span = 0.0);
std::vector<int> get_order(const std::vector<t_reduceTime>& vec);

std::pair<MatrixLD, DiagonalMatrixLD> decomp_matB (const SplineMatricesLD& matrices, const double lambdaSpline);
std::pair<MatrixD, DiagonalMatrixD> decomp_matB(const SplineMatricesD& matrices, const double lambdaSpline);

#pragma mark usefull math function for MCMCLoopCurve

t_prob h_YWI_AY(const SplineMatricesLD& matrices, const std::vector<std::shared_ptr<Event>> &events, const  double lambdaSpline, const std::vector< t_reduceTime> &vecH, const bool hasY = false, const bool hasZ = false);

t_prob h_YWI_AY_composanteX(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_matB, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_QTQ, const double lambdaSpline);
t_prob h_YWI_AY_composanteY(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_matB, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_QTQ, const double lambdaSpline);
t_prob h_YWI_AY_composanteZ(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_matB, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_QTQ, const double lambdaSpline);

t_prob h_YWI_AY_composanteZ_decomp(const SplineMatricesLD& matrices, const QList<Event *> &events, const double lambdaSpline, const std::vector< t_reduceTime> &vecH);

t_prob ln_h_YWI_3_update(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD> &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ);

t_prob ln_h_YWI_3_X(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_matB, const double lambdaSpline);
t_prob ln_h_YWI_3_Y(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_matB, const double lambdaSpline);
t_prob ln_h_YWI_3_Z(const SplineMatricesLD &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<MatrixLD, DiagonalMatrixLD > &decomp_matB, const double lambdaSpline);


double S02_lambda_WI(const SplineMatricesLD &matrices, const int nb_noeuds);

double rapport_detK_plus(const MatrixLD &Mat_old, const MatrixLD &Mat_new);

bool hasPositiveGPrimeByDet (const MCMCSplineComposante &splineComposante);
bool hasPositiveGPrimeByDerivate (const MCMCSplineComposante &splineComposante, const double k = 0.0);
bool hasPositiveGPrimePlusConst (const MCMCSplineComposante &splineComposante, const double tmin, const double tmax, const double dy_threshold = 0.0);

inline double log_p(const double x, const double n) {
    return log(x) / log(n) ;
}

MatrixLD inverseMatSym_originKK(const MatrixLD& matrixLE,  const DiagonalMatrixLD& matrixDE, const int nbBandes, const int shift);


t_prob detPlus(const std::pair<MatrixLD, DiagonalMatrixLD > &decomp);



/**
    * @brief Calculates the natural logarithm of the determinant of a matrix's diagonal elements
    *
    * @details This function computes:
    * @f[
    * \ln(\det^+) = \sum_{i=1}^{n-2} \ln(m_i)
    * @f]
    * where @f$m_i@f$ are the diagonal elements of the matrix (excluding first and last elements).
    * The "+" in @f$\det^+@f$ indicates that we're using a modified determinant calculation
    * that skips the first and last diagonal elements.
    *
    * @param decomp A pair containing:
    *        - First: A 2D matrix (MatrixLD)
    *        - Second: A diagonal matrix (MatrixDiag) whose logarithmic determinant we calculate
    *
    * @return t_prob The sum of the natural logarithms of the diagonal elements
    *
    * @note The function skips the first and last elements of the diagonal matrix
    * @note Uses parallel reduction for large matrices (> 1000 elements)
    */
t_prob ln_detPlus(const std::pair<MatrixLD, DiagonalMatrixLD>& decomp);

t_prob ln_h_YWI_1(const std::pair<MatrixLD, DiagonalMatrixLD> &decomp_QTQ);

t_prob ln_h_YWI_2(const std::pair<MatrixLD, DiagonalMatrixLD> &decomp_matB);

t_prob ln_h_YWI_1_2(const std::pair<MatrixLD, DiagonalMatrixLD>& decomp_QTQ, const std::pair<MatrixLD, DiagonalMatrixLD >& decomp_matB);


template <typename Derived1, typename Derived2>
double log_det_ratio(const Eigen::MatrixBase<Derived1>& new_diag,
                     const Eigen::MatrixBase<Derived2>& old_diag)
{
    assert(new_diag.size() == old_diag.size());
    if (new_diag.size() <= 2) return 0.0;

    auto new_seg = new_diag.segment(1, new_diag.size() - 2).template cast<double>();
    auto old_seg = old_diag.segment(1, old_diag.size() - 2).template cast<double>();

    return (new_seg.array().log() - old_seg.array().log()).sum();
}


t_prob ln_rate_det_B (const std::pair<MatrixLD, DiagonalMatrixLD>& try_decomp_B, const std::pair<MatrixLD, DiagonalMatrixLD >& current_decomp_B);

t_prob ln_rate_det_QtQ_det_B (const std::pair<MatrixLD, DiagonalMatrixLD>& try_decomp_QtQ, const std::pair<MatrixLD, DiagonalMatrixLD >& try_decomp_B,
                             const std::pair<MatrixLD, DiagonalMatrixLD>& current_decomp_QtQ, const std::pair<MatrixLD, DiagonalMatrixLD >& current_decomp_B);

inline double Signe_Number(const double &a)
{
    return std::copysign(1.0, a);
}

#endif
