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

#include "Matrix.h"
#include "Event.h"

#include <vector>
#include <string>
#include <QDataStream>



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
    //std::vector<double> vecThetaEvents; // le noeud ti
    std::vector<double> vecThetaReduced; // le noeud ti reduce
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

    // spline density
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


template< class Fun>
static SplineResults doSpline(Fun* get_param, const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline)
{
    /*
          * MatB doit rester en copie
          */
    SplineResults spline;
    try {
        // calcul de: R + alpha * Qt * W-1 * Q = Mat_B


        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = events.size();

        std::vector<double> vecG;
        std::vector<double> vecQtY;

        // VecQtY doit être de taille n, donc il faut mettre un zéro au début et à la fin
        vecQtY.push_back(0.);
        for (size_t i = 1; i < n-1; ++i) {
            const double term1 = (get_param(events[i+1]) - get_param(events[i])) / vecH[i];
            const double term2 = (get_param(events[i]) - get_param(events[i-1])) / vecH[i-1];
            vecQtY.push_back(term1 - term2);
        }
        vecQtY.push_back(0.);

        // Calcul du vecteur Gamma
        const decltype(SplineResults::vecGamma) &vecGamma = resolutionSystemeLineaireCholesky(decomp, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - lamnbda * W-1 * Q * gamma
        if (lambdaSpline != 0) {
            const std::vector<double> &vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            const MatrixDiag &diagWInv = matrices.diagWInv;

            for (unsigned i = 0; i < n; ++i) {
                vecG.push_back( get_param(events[i]) - lambdaSpline * diagWInv[i] * vecTmp2[i]) ;
            }

        } else {
            vecG.resize(n);
            std::transform(events.begin(), events.end(), vecG.begin(), get_param);
        }


        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);

    } catch(...) {
        qCritical() << "[MCMCLoopCurve::doSpline] : Caught Exception!\n";
    }

    return spline;
}

static SplineResults doSplineX(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline)
{return doSpline(get_Yx, matrices, events, vecH, decomp, lambdaSpline);}

static SplineResults doSplineY(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline)
{return doSpline(get_Yy, matrices, events, vecH, decomp, lambdaSpline);}

static SplineResults doSplineZ(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag >& decomp, const double lambdaSpline)
{return doSpline(get_Yz, matrices, events, vecH, decomp, lambdaSpline);}


std::vector<double> calculMatInfluence_origin(const SplineMatrices &matrices, const int nbBandes, const std::pair<Matrix2D, MatrixDiag >& decomp, const double lambdaSpline);
std::vector<double> doSplineError_origin(const SplineMatrices &matrices, const SplineResults &splines, const double lambdaSpline);

std::vector<double> calcul_spline_variance(const SplineMatrices &matrices, const QList<Event *> &events, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline);

double valeurG(const double t, const MCMCSplineComposante& spline, unsigned long &i0, Model &model);
double valeurErrG(const double t, const MCMCSplineComposante& spline, unsigned& i0, Model &model);
double valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0, Model &model);
double valeurGSeconde(const double t, const MCMCSplineComposante& spline, Model &model);

void valeurs_G_VarG_GP_GS(const double t, const MCMCSplineComposante &spline, double& G, double& varG, double& GP, double& GS, unsigned& i0, double tmin, double tmax);



#pragma mark Calcul Spline on vector Y

SplineMatrices prepareCalculSpline_WI(const std::vector<t_reduceTime> &vecH);
SplineMatrices prepare_calcul_spline(const std::vector<t_reduceTime> &vecH, const std::vector<double> vec_Vg_Si2);

std::vector<double> calcul_spline_variance(const SplineMatrices& matrices, const std::vector<double> &vec_W, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline);
std::vector<QMap<double, double>> composante_to_curve(MCMCSplineComposante spline_compo, double tmin, double tmax, double step);

static SplineResults do_spline(const std::vector<double> &vec_Y, const SplineMatrices &matrices,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline);

MCMCSpline do_spline_composante(const QMap<double, double> &data_X, const QMap<double, double> &data_X_err, double tmin, double tmax, const CurveSettings &cs, const QMap<double, double> &data_Y = QMap<double, double>(), const QMap<double, double> &data_Y_err = QMap<double, double>(), const QMap<double, double> &data_Z = QMap<double, double>(), const QMap<double, double> &data_Z_err = QMap<double, double>()) ;

double cross_validation (const std::vector< double>& vec_Y, const SplineMatrices& matrices, const std::vector< double>& vecH, const double lambdaSpline);
double initLambdaSplineByCV(const std::vector< double> &vec_X, const std::vector< double> &vec_X_err, const SplineMatrices &matrices, const std::vector< double> &vecH, const std::vector<double> &vec_Y = std::vector< double>(), const std::vector<double> &vec_Y_err = std::vector< double>(), const std::vector<double> &vec_Z = std::vector< double>(), const std::vector<double> &vec_Z_err = std::vector< double>());
double initLambdaSplineByCV_VgFixed(const std::vector< double> &vec_Y, const std::vector< double> &vec_Y_err, const std::vector< double> &vecH, const double Vg);

double general_cross_validation (const std::vector< double>& vec_Y,  const SplineMatrices& matrices, const std::vector<double>& vecH, const double lambdaSpline);
double var_residual(const std::vector<double> &vec_Y,const SplineMatrices &matrices,const std::vector<t_reduceTime> &vecH, const double lambda);

#endif
