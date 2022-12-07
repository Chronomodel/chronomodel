/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

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

#ifndef MCMCLOOPCURVE_H
#define MCMCLOOPCURVE_H

#include "MCMCLoop.h"
#include "CurveSettings.h"
#include "CurveUtilities.h"
#include "Matrix.h"
#include "ModelCurve.h"

#include <vector>

class Project;
class Event;

typedef double t_prob;

class MCMCLoopCurve: public MCMCLoop
{
    Q_OBJECT

public:
    ModelCurve* mModel;
    CurveSettings mCurveSettings;

    MCMCLoopCurve(ModelCurve* model, Project* project);
    ~MCMCLoopCurve();

protected:
    // Variable for update function
    double tminPeriod;
    double tmaxPeriod;
    bool hasY, hasZ;
    t_prob current_ln_h_YWI_2, current_ln_h_YWI_3, current_ln_h_YWI_1_2,current_h_theta, current_h_lambda, current_h_VG;

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

 #pragma mark function
    void orderEventsByThetaReduced(QList<Event *> &event);
    void spreadEventsThetaReduced0(QList<Event *> &sortedEvents, t_reduceTime spreadSpan = 0.);
    std::thread::id mTh_id_memoCurve;
    std::thread mTh_memoCurve;

    static void memo_PosteriorG_3D(PosteriorMeanG &postG, MCMCSpline spline, CurveSettings::ProcessType &curveType, const int realyAccepted, ModelCurve &model);

    QString initialize_time();
    QString initialize_321();
    QString initialize_interpolate();

    bool update_321();
    bool update_interpolate();
    bool (MCMCLoopCurve::*updateLoop)();

protected:

    virtual QString calibrate();
    virtual void initVariablesForChain();
    virtual QString initialize();
    virtual bool update();
    virtual bool adapt(const int batchIndex);
    virtual void memo();
    virtual void finalize();
    



private:

    QList<Event *> mPointEvent;
    QList<Event *> mNodeEvent;


    static t_prob h_YWI_AY(const SplineMatrices& matrices, const QList<Event *> &events, const  double lambdaSpline, const std::vector< t_reduceTime> &vecH, const bool hasY = false, const bool hasZ = false);

    static t_prob h_YWI_AY_composanteX(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const std::pair<Matrix2D, MatrixDiag > &decomp_QTQ, const double lambdaSpline);
    static t_prob h_YWI_AY_composanteY(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const std::pair<Matrix2D, MatrixDiag > &decomp_QTQ, const double lambdaSpline);
    static t_prob h_YWI_AY_composanteZ(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const std::pair<Matrix2D, MatrixDiag > &decomp_QTQ, const double lambdaSpline);

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

     static t_prob ln_h_YWI_3_update_ASYNC(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ);
     static t_prob ln_h_YWI_3_update(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ);

     static t_prob ln_h_YWI_3_X(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline);
     static t_prob ln_h_YWI_3_Y(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline);
     static t_prob ln_h_YWI_3_Z(const SplineMatrices &matrices, const QList<Event *> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp_matB, const double lambdaSpline);

     // ---- Ancienne numérotation

     static std::pair<Matrix2D, MatrixDiag> decomp_matB (const SplineMatrices &matrices, const double lambdaSpline)
     {
         const Matrix2D& matR = matrices.matR;
         Matrix2D matB;

         if (lambdaSpline != 0) {
             // Decomposition_Cholesky de matB en matL et matD
             // Si lambda global: calcul de Mat_B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D

             const Matrix2D &tmp = multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5);
             matB = addMatEtMat(matR, tmp, 5);

         } else {
             matB = matR;
         }

         return decompositionCholesky(matB, 5, 1);

     }


     static t_prob h_lambda(const SplineMatrices &matrices, const int nb_noeuds, const  double lambdaSpline) ;
     t_prob h_theta (const QList<Event *> &events) const;
     static double h_theta_Event (const Event * e);

     qsizetype mFirstEventIndex; // Utile pour VG global, correspond au premier Event qui n'est pas un Bound    

     inline t_prob h_VG_Event(const Event * e, double S02_Vg) const;

     t_prob h_S02_Vg(const QList<Event *> &events, double S02_Vg) const;
     t_prob rate_h_S02_Vg_test(const QList<Event *> &events, double S02_Vg, double try_S02) const;
     t_prob rate_h_S02_Vg(const QList<Event *> &pointEvents, double S02_Vg, double try_S02) const;

     double S02_Vg_Yx(QList<Event *> &events, SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline);
     double S02_Vg_Yy(QList<Event *> &events, SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline);
     double S02_Vg_Yz(QList<Event *> &events, SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline);

     double Var_residual_spline;
     double var_Y;

     static double S02_lambda_WI(const SplineMatrices &matrices, const int nb_noeuds);

     double Calcul_Variance_Rice (const QList<Event *> &events) const;

     void prepareEventsY(const QList<Event *> &events);
     void prepareEventY(Event * const event);

     std::vector<double> createDiagWInv(const QList<Event *> &events);

     double minimalThetaDifference(QList<Event *> &events);
     double minimalThetaReducedDifference(QList<Event *> &events);

     void spreadEventsTheta(QList<Event *> &events, double minStep = 1e-6); // not used

    inline  double yearTime(double reduceTime) ;

    std::map<int, double> mThetasMemo;
    
    std::vector<t_reduceTime> calculVecH(const QList<Event *> &event);

    std::vector< double> getThetaEventVector(const QList<Event *> &events);

    Matrix2D calculMatR(const std::vector<t_reduceTime> &vecH);
    Matrix2D calculMatQ(const std::vector<t_reduceTime> &vecH);

    MCMCSpline currentSpline (QList<Event *> &events, bool doSortAndSpreadTheta = false, const std::vector<t_reduceTime> &vecH = std::vector<t_reduceTime>(), const SplineMatrices &matrices = SplineMatrices());
    MCMCSpline currentSpline_WI (QList<Event *> &events, bool doSortAndSpreadTheta = false, const std::vector<t_reduceTime> &vecH = std::vector<t_reduceTime>(), const SplineMatrices &matrices = SplineMatrices());

    SplineMatrices prepareCalculSpline(const QList<Event *> & sortedEvents, const std::vector<t_reduceTime> &vecH);
    SplineMatrices prepareCalculSpline_WI(const QList<Event *> & sortedEvents, const std::vector<t_reduceTime> &vecH);

    static SplineResults calculSplineX(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline);
    static SplineResults calculSplineY(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline);
    static SplineResults calculSplineZ(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag >& decomp, const double lambdaSpline);

    std::vector< double> calculSplineError0(const SplineMatrices &matrices, const Matrix2D &matB, const double lambdaSpline);

    std::vector<double> calculMatInfluence_origin(const SplineMatrices &matrices, const int nbBandes, const std::pair<Matrix2D, MatrixDiag >& decomp, const double lambdaSpline);
    std::vector<double> calculSplineError_origin(const SplineMatrices &matrices, const SplineResults &splines, const double lambdaSpline);
    std::vector<double> calcul_spline_variance(const SplineMatrices &matrices, const QList<Event *> &events, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline);

    double valeurG(const double t, const MCMCSplineComposante& spline, unsigned long &i0);
    double valeurErrG(const double t, const MCMCSplineComposante& spline, unsigned& i0);
    double valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0);
    double valeurGSeconde(const double t, const MCMCSplineComposante& spline);

    void valeurs_G_ErrG_GP_GS(const double t, const MCMCSplineComposante& spline,  double& G,  double& ErrG, double& GP, double& GS, unsigned& i0);

   // double initLambdaSpline();
   // double initLambdaSplineByCV();
    double initLambdaSplineBy_h_YWI_AY();


    bool hasPositiveGPrimeByDet (const MCMCSplineComposante &splineComposante);
    bool hasPositiveGPrimeByDerivate (const MCMCSplineComposante &splineComposante, const double k = 0.);
    bool hasPositiveGPrimePlusConst (const MCMCSplineComposante &splineComposante, const double dy_threshold = 0.);

    void memo_PosteriorG(PosteriorMeanGComposante& postGCompo, MCMCSplineComposante &splineComposante, const int realyAccepted);


};

#endif
