/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE
    Komlan NOUKPOAPE

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

#ifndef MCMCLOOPCURVE_H
#define MCMCLOOPCURVE_H

#include "MCMCLoop.h"
#include "CurveSettings.h"
#include "CurveUtilities.h"
#include "Matrix.h"
#include "ModelCurve.h"
#include "Event.h"

#include <vector>


typedef double t_prob;

class MCMCLoopCurve: public MCMCLoop
{
    Q_OBJECT

public:
    MCMCLoopCurve(std::shared_ptr<ModelCurve> model);
    ~MCMCLoopCurve();

protected:
    // Variable for update function

    t_prob current_ln_h_YWI_2, current_ln_h_YWI_3, current_ln_h_YWI_1_2, current_h_theta, current_h_lambda, current_h_VG;

    SplineMatrices current_splineMatrices, current_matriceWI;
    SplineResults current_spline;
    std::vector<t_reduceTime> current_vecH;
    std::pair<Matrix2D, MatrixDiag> current_decomp_QTQ;
    std::pair<Matrix2D, MatrixDiag> current_decomp_matB;

    t_prob try_h_theta, try_h_lambda, try_h_VG;
    t_prob try_ln_h_YWI_2, try_ln_h_YWI_3, try_ln_h_YWI_1_2;

    std::vector<t_reduceTime> try_vecH;
    SplineMatrices try_splineMatrices;

    std::pair<Matrix2D, MatrixDiag> try_decomp_QTQ;
    std::pair<Matrix2D, MatrixDiag> try_decomp_matB;

    std::vector<Event*> initListEvents;

private:
    std::vector<Event*> mPointEvent;
    std::vector<Event*> mNodeEvent;
    qsizetype mFirstEventIndex; // Utile pour VG global, correspond au premier Event qui n'est pas un Bound
    double Var_residual_spline;
    double var_Y;

#pragma mark function
protected:

    void orderEventsByThetaReduced(std::vector<Event*> &event);
    void spreadEventsThetaReduced0(std::vector<Event*> &sortedEvents, t_reduceTime spreadSpan = 0.);
    //std::thread::id mTh_id_memoCurve;
    //std::thread mTh_memoCurve;

    static void memo_PosteriorG_3D(PosteriorMeanG &postG, MCMCSpline spline, CurveSettings::ProcessType &curveType, const int realyAccepted, ModelCurve &model);

    bool (MCMCLoopCurve::*updateLoop)();

    QString initialize_321();
    QString initialize_400();
    QString initialize_401();
    QString initialize_interpolate();
    QString initialize_Komlan();

    bool update_321();
    bool update_400();
    bool update_401();
    bool update_interpolate();

    bool update_Komlan();


protected:

    virtual QString calibrate();
   // virtual void initVariablesForChain();
    virtual QString initialize();
    virtual bool update();
    virtual bool adapt(const int batchIndex);
    virtual void memo();
    //virtual void memo_accept(const unsigned i_chain);
    virtual void finalize();
    
    
private:

    static t_prob h_YWI_AY(const SplineMatrices& matrices, const std::vector<Event *> &events, const  double lambdaSpline, const std::vector< t_reduceTime> &vecH, const bool hasY = false, const bool hasZ = false);

    static t_prob h_YWI_AY_composanteX(const SplineMatrices &matrices, const std::vector<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const std::pair<Matrix2D, MatrixDiag > &decomp_QTQ, const double lambdaSpline);
    static t_prob h_YWI_AY_composanteY(const SplineMatrices &matrices, const std::vector<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const std::pair<Matrix2D, MatrixDiag > &decomp_QTQ, const double lambdaSpline);
    static t_prob h_YWI_AY_composanteZ(const SplineMatrices &matrices, const std::vector<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const std::pair<Matrix2D, MatrixDiag > &decomp_QTQ, const double lambdaSpline);

    t_prob h_YWI_AY_composanteZ_decomp(const SplineMatrices& matrices, const QList<Event *> &events, const double lambdaSpline, const std::vector< t_reduceTime> &vecH);


#pragma mark Optimization

     static t_prob detPlus(const std::pair<Matrix2D, MatrixDiag > &decomp)
     {
         return std::accumulate(decomp.second.cbegin(), decomp.second.cend(), 0., [](double prod, const double m){return prod + m;});
     }

     static t_prob ln_detPlus(const std::pair<Matrix2D, MatrixDiag > &decomp)
     {
         return std::accumulate(decomp.second.cbegin()+1, decomp.second.cend()-1, 0., [](double sum, const double m){return sum + log(m);});
     }


     static t_prob ln_h_YWI_1(const std::pair<Matrix2D, MatrixDiag> &decomp_QTQ)
     {
         return ln_detPlus(decomp_QTQ);
     }

     static t_prob ln_h_YWI_2(const std::pair<Matrix2D, MatrixDiag> &decomp_matB)
     {
         return -ln_detPlus(decomp_matB);
     }

     static t_prob ln_h_YWI_1_2(const std::pair<Matrix2D, MatrixDiag> &decomp_QTQ, const std::pair<Matrix2D, MatrixDiag > &decomp_matB)
     {
         const MatrixDiag &matDq = decomp_QTQ.second;
         const MatrixDiag &matD = decomp_matB.second;
         return std::transform_reduce(PAR matDq.cbegin()+1, matDq.cend()-1, matD.begin()+1, 0., std::plus{}, [](double val1,  double val2) { return log(val1/val2); });
     }

     static t_prob ln_h_YWI_3_update_ASYNC(const SplineMatrices &matrices, const std::vector<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ);
     static t_prob ln_h_YWI_3_update(const SplineMatrices &matrices, const std::vector<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ);

     static t_prob ln_h_YWI_3_X(const SplineMatrices &matrices, const std::vector<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline);
     static t_prob ln_h_YWI_3_Y(const SplineMatrices &matrices, const std::vector<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline);
     static t_prob ln_h_YWI_3_Z(const SplineMatrices &matrices, const std::vector<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline);

     static std::pair<Matrix2D, MatrixDiag> decomp_matB (const SplineMatrices &matrices, const double lambdaSpline)
     {
         Q_ASSERT(lambdaSpline != 0);
         /*
          * if lambdaSpline == 0, it is interpolatation
          * return decompositionCholesky(matrices.matR, 5, 1);
          */

         // Decomposition_Cholesky de matB en matL et matD
         // Si lambda global: calcul de Mat_B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
         const Matrix2D &tmp = multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5);

         const Matrix2D &rMatR = matrices.matR;
         const Matrix2D &rMatB = addMatEtMat(rMatR, tmp, 5);
         return decompositionCholesky(rMatB, 5, 1);

     }


     static t_prob h_lambda(const SplineMatrices &matrices, const int nb_noeuds, const  double lambdaSpline) ;
     t_prob h_theta (const QList<Event *> &events) const;
     static double h_theta_Event (const Event * e);


     inline t_prob h_VG_Event(const Event * e, double S02_Vg) const;

     t_prob h_S02_Vg(const std::vector<Event *> &events, double S02_Vg) const;
     double h_S02_Vg_K_old(const std::vector<Event *> events, double S02_Vg, double try_Vg);

     t_prob rate_h_S02_Vg_test(const std::vector<Event *> &events, double S02_Vg, double try_S02) const;
     t_prob rate_h_S02_Vg(const std::vector<Event *> &pointEvents, double S02_Vg, double try_S02) const;

     double S02_Vg_Yx(std::vector<Event *> &events, const SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline);
     double S02_Vg_Yy(std::vector<Event *> &events, const SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline);
     double S02_Vg_Yz(std::vector<Event *> &events, const SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline);

     static double S02_lambda_WI(const SplineMatrices &matrices, const int nb_noeuds);

     double Calcul_Variance_Rice (const std::vector<Event *> &events) const;

     void prepareEventsY(const std::vector<Event *> &events);
     void prepareEventY(Event * const event);


     double minimalThetaDifference(std::vector<Event *> &events);
     double minimalThetaReducedDifference(std::vector<Event *> &events);

     void spreadEventsTheta(std::vector<Event *> &events, double minStep = 1e-6); // not used

    inline  double yearTime(double reduceTime) ;

    std::map<int, double> mThetasMemo;

   // double initLambdaSpline();
   // double initLambdaSplineByCV();
    double initLambdaSplineBy_h_YWI_AY();


    bool hasPositiveGPrimeByDet (const MCMCSplineComposante &splineComposante);
    bool hasPositiveGPrimeByDerivate (const MCMCSplineComposante &splineComposante, const double k = 0.);
    bool hasPositiveGPrimePlusConst (const MCMCSplineComposante &splineComposante, const double dy_threshold = 0.);

    void memo_PosteriorG(PosteriorMeanGComposante &postGCompo, MCMCSplineComposante &splineComposante, const int realyAccepted);

#pragma mark import_Komlan

    // obsolete double rate_h_lambda_K(const MCMCSpline &s, const double current_lambda, const double try_lambda, const Matrix2D &K);
    double rate_h_lambda_K(const double current_lambda, const double try_lambda, const double schrinkageLambda, const int n);
    double S02_lambda_WIK (const Matrix2D &K, const int nb_noeuds);
    double h_lambda_Komlan(const Matrix2D &K, const Matrix2D &K_new, const int nb_noeuds, const double &lambdaSpline);
    double rapport_Theta(const std::function<double (Event *)> &fun, const std::vector<Event *> &lEvents, const Matrix2D &K, const Matrix2D &K_new, const double lambdaSpline);

    MatrixDiag createDiagWInv_Vg0(const std::vector<Event*>& lEvents);

    SplineMatrices prepareCalculSpline_W_Vg0(const std::vector<Event *> &sortedEvents, std::vector<double> &vecH);
    MCMCSpline samplingSpline_multi(std::vector<Event *> &lEvents, std::vector<Event *> &lEventsinit, std::vector<double> vecYx, std::vector<double> vecYstd, const Matrix2D &RR, const Matrix2D &R_1QT, const Matrix2D &Q, const Matrix2D &QT, const Matrix2D &matK);

    std::vector<double> multinormal_sampling (const std::vector<double> &mu, const Matrix2D &a);

    std::vector<double> splines_prior(const Matrix2D &KK, std::vector<double> &g, std::vector<double> &g_new);
    inline double Signe_Number(const double &a);
    double Prior_F (const Matrix2D &K, const Matrix2D &K_new, const MCMCSpline &s,  const double lambdaSpline);
    std::vector<double> multiMatByVectCol0_KK(const Matrix2D &KKK, const std::vector<double> &gg);
    std::vector<double> multiMatByVectCol0(const Matrix2D &KKK, const std::vector<double> &gg);


    double h_exp_fX_theta (Event* e, const MCMCSpline &s, unsigned idx);
    std::vector<double> sampling_spline (std::vector<Event *> &lEvents, SplineMatrices matrices);
    t_prob h_S02_Vg_K(const std::vector<Event *> events, const double S02_Vg, const double try_Vg);

    std::pair<Matrix2D, std::vector<double>> decompositionCholeskyKK(const Matrix2D &matrix, const int nbBandes, const int shift);

};

#pragma mark usefull math function

inline double log_p(const double x, const double n) {
    return log(x) / log(n) ;
}

double rapport_detK_plus(const Matrix2D &Mat_old, const Matrix2D &Mat_new);
Matrix2D inverseMatSym_originKK(const Matrix2D &matrixLE,  const std::vector<double> &matrixDE, const int nbBandes, const int shift);

#endif
