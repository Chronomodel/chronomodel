/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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
#include "CurveUtilities.h"
#include "Matrix.h"
#include "ModelCurve.h"
#include "Event.h"
#include "version.h"

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
    std::pair<Matrix2D, DiagonalMatrixLD> current_decomp_QTQ;
    std::pair<Matrix2D, DiagonalMatrixLD> current_decomp_matB;

    t_prob try_h_theta, try_h_lambda, try_h_VG;
    t_prob try_ln_h_YWI_2, try_ln_h_YWI_3, try_ln_h_YWI_1_2;

    std::vector<t_reduceTime> try_vecH;
    SplineMatrices try_splineMatrices;

    std::pair<Matrix2D, DiagonalMatrixLD> try_decomp_QTQ;
    std::pair<Matrix2D, DiagonalMatrixLD> try_decomp_matB;

    std::vector<std::shared_ptr<Event>> initListEvents;

private:
    std::vector<std::shared_ptr<Event>> mPointEvent;
    std::vector<std::shared_ptr<Event>> mNodeEvent;
    qsizetype mFirstEventIndex; // Utile pour VG global, correspond au premier Event qui n'est pas un Bound
    double Var_residual_spline;
    double var_Y;

#pragma mark function
protected:

    void orderEventsByThetaReduced(std::vector<std::shared_ptr<Event> > &event);
    void spreadEventsThetaReduced0(std::vector<std::shared_ptr<Event>> &sortedEvents, t_reduceTime spreadSpan = 0.0);
    std::vector<double> spreadEventsTheta0(std::vector<std::shared_ptr<Event>>& Events, t_reduceTime spreadSpan = 0.0);
    std::vector<double> unclumpTheta(const std::vector<std::shared_ptr<Event>>& events, double spreadSpan = 1e-8);

    bool (MCMCLoopCurve::*updateLoop)();;
#if VERSION_MAJOR == 3 && VERSION_MINOR == 2 && VERSION_PATCH == 1
#pragma mark Version 3.2.1

    QString initialize_321();
    bool update_321();

#elif VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH == 0
#pragma mark Version 3.3.0
    QString initialize_330();
    bool update_330();

#elif VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH >= 5
#pragma mark Version 3.3.5

    QString initialize_335();
    bool update_335(); // estimation de G as Komlan

#elif VERSION_MAJOR == 4 && VERSION_MINOR >= 0 && VERSION_PATCH >= 0
#pragma mark Version 4
    QString initialize_400();
    QString initialize_401();
    bool update_400();
    bool update_401();


#endif
#pragma mark Interpolate
    QString initialize_interpolate();
    bool update_interpolate();

    void test_depth(std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const SplineMatrices &matrices, const double lambda, double &rate, bool &ok);

    QString initialize_Komlan();
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

    static t_prob h_YWI_AY(const SplineMatrices& matrices, const std::vector<std::shared_ptr<Event>> &events, const  double lambdaSpline, const std::vector< t_reduceTime> &vecH, const bool hasY = false, const bool hasZ = false);

    static t_prob h_YWI_AY_composanteX(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_matB, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_QTQ, const double lambdaSpline);
    static t_prob h_YWI_AY_composanteY(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_matB, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_QTQ, const double lambdaSpline);
    static t_prob h_YWI_AY_composanteZ(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_matB, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_QTQ, const double lambdaSpline);

    t_prob h_YWI_AY_composanteZ_decomp(const SplineMatrices& matrices, const QList<Event *> &events, const double lambdaSpline, const std::vector< t_reduceTime> &vecH);


#pragma mark Optimization

     static t_prob detPlus(const std::pair<Matrix2D, DiagonalMatrixLD > &decomp)
     {
         return std::accumulate(decomp.second.diagonal().cbegin(), decomp.second.diagonal().cend(), 0., [](double prod, const double m){return prod + m;});
     }

    /* static t_prob ln_detPlus(const std::pair<Matrix2D, MatrixDiag > &decomp)
     {
         return std::accumulate(decomp.second.cbegin()+1, decomp.second.cend()-1, 0., [](double sum, const double m){return sum + log(m);});
     }
    */
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
    *        - First: A 2D matrix (Matrix2D)
    *        - Second: A diagonal matrix (MatrixDiag) whose logarithmic determinant we calculate
    *
    * @return t_prob The sum of the natural logarithms of the diagonal elements
    *
    * @note The function skips the first and last elements of the diagonal matrix
    * @note Uses parallel reduction for large matrices (> 1000 elements)
    */
     static t_prob ln_detPlus(const std::pair<Matrix2D, DiagonalMatrixLD>& decomp)
     {
        const auto& diag = decomp.second.diagonal();
        const size_t size = diag.rows();

        // Handle edge cases , yet tested before
        // if (size <= 2) return 0.0;

        t_prob sum = 0.0;
        // Pour les petites matrices, utiliser la version séquentielle
        if (size < 1000) {
            for (size_t i = 1; i < size - 1; ++i) {
                sum += std::log(diag[i]);
            }
            return sum;
        }

        // Pour les grandes matrices, utiliser la parallélisation
#pragma omp parallel reduction(+:sum)
        {
#pragma omp for nowait
            for (size_t i = 1; i < size - 1; ++i) {
                sum += std::log(diag[i]);
            }
        }

        return sum;
     }

     static t_prob ln_h_YWI_1(const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_QTQ)
     {
         return ln_detPlus(decomp_QTQ);
     }

     static t_prob ln_h_YWI_2(const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB)
     {
         return -ln_detPlus(decomp_matB);
     }

     static t_prob ln_h_YWI_1_2(const std::pair<Matrix2D, DiagonalMatrixLD>& decomp_QTQ, const std::pair<Matrix2D, DiagonalMatrixLD >& decomp_matB)
     {
        const auto& diagDq = decomp_QTQ.second.diagonal();  // Eigen::Matrix<t_matrix, Dynamic, 1>
        const auto& diagD  = decomp_matB.second.diagonal();   // même type

        return std::transform_reduce(PAR diagDq.cbegin()+1, diagDq.cend()-1, diagD.cbegin()+1, 0., std::plus{}, [](double val1,  double val2) { return std::log(val1) - std::log(val2); });

     }

     static t_prob ln_h_YWI_3_update_ASYNC(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ);
     static t_prob ln_h_YWI_3_update(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ);

     static t_prob ln_h_YWI_3_X(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_matB, const double lambdaSpline);
     static t_prob ln_h_YWI_3_Y(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_matB, const double lambdaSpline);
     static t_prob ln_h_YWI_3_Z(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_matB, const double lambdaSpline);


    t_prob h_lambda_321(const SplineMatrices &matrices, const int nb_noeuds, const  double lambdaSpline) ;
    t_prob h_lambda_330(const double lambdaSpline);
    t_prob h_theta (const QList<std::shared_ptr<Event>> &events) const;
    static double h_theta_Event (const std::shared_ptr<Event> e);


     inline t_prob h_VG_Event(const std::shared_ptr<Event> &e, const double S02_Vg) const;
     t_prob h_VG_Event(const double Vg, const double S02_Vg) const;

     t_prob h_S02_Vg(const std::vector<std::shared_ptr<Event> > &events, double S02_Vg) const;
     double h_S02_Vg_K_old(const std::vector<std::shared_ptr<Event> > events, double S02_Vg, double try_Vg);

     t_prob rate_h_S02_Vg_test(const std::vector<std::shared_ptr<Event>> &events, double S02_Vg, double try_S02) const;
     t_prob rate_h_S02_Vg(const std::vector<std::shared_ptr<Event> > &pointEvents, double S02_Vg, double try_S02) const;

     double S02_Vg_Yx(std::vector<std::shared_ptr<Event>> &events, const SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline);
     double S02_Vg_Yy(std::vector<std::shared_ptr<Event>> &events, const SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline);
     double S02_Vg_Yz(std::vector<std::shared_ptr<Event>> &events, const SplineMatrices &matricesWI, std::vector<t_reduceTime> &vecH, const double lambdaSpline);

     static double S02_lambda_WI(const SplineMatrices &matrices, const int nb_noeuds);

     double Calcul_Variance_Rice (const std::vector<std::shared_ptr<Event>> &events) const;

     void prepareEventsY(const std::vector<std::shared_ptr<Event>> &events);
     void prepareEventY(std::shared_ptr<Event> const event);


     double minimalThetaDifference(std::vector<std::shared_ptr<Event>> &events);
     double minimalThetaReducedDifference(std::vector<std::shared_ptr<Event>> &events);

     void spreadEventsTheta(std::vector<std::shared_ptr<Event>> &events, double minStep = 1e-6); // not used

    inline  double yearTime(double reduceTime) ;

    std::map<int, double> mThetasMemo;

   // double initLambdaSpline();
   // double initLambdaSplineByCV();
    double initLambdaSplineBy_h_YWI_AY();


    bool hasPositiveGPrimeByDet (const MCMCSplineComposante &splineComposante);
    bool hasPositiveGPrimeByDerivate (const MCMCSplineComposante &splineComposante, const double k = 0.);
    bool hasPositiveGPrimePlusConst (const MCMCSplineComposante &splineComposante, const double dy_threshold = 0.);

    //void memo_PosteriorG(PosteriorMeanGComposante &postGCompo, MCMCSplineComposante &splineComposante, const int realyAccepted);

#pragma mark import_Komlan

    double rate_h_lambda_K(const double current_lambda, const double try_lambda, const double tr_K, const int n);
    t_prob rate_h_lambda_X_335(const double current_lambda, const double try_lambda, const t_prob n_points);
    t_prob rate_h_lambda_XY_335(const double current_lambda, const double try_lambda, const t_prob n_points);
    t_prob rate_h_lambda_XYZ_335(const double current_lambda, const double try_lambda, const t_prob n_points);

    double S02_lambda_WIK (const Matrix2D &K, const int nb_noeuds);
    double h_lambda_Komlan(const Matrix2D &K, const Matrix2D &K_new, const int nb_noeuds, const double &lambdaSpline);
    t_prob rapport_Theta(const std::function<double (std::shared_ptr<Event>)> &fun, const std::vector<std::shared_ptr<Event>> &lEvents, const Matrix2D &K, const Matrix2D &K_new, const double lambdaSpline);
    // rapport avec utilisation des matrices Y(x, y, z)
    t_prob rate_Theta(const Matrix2D &Y, const Matrix2D &K, const Matrix2D &K_try, const double lambda);

    t_prob rate_Theta_X(const std::vector<std::shared_ptr<Event>> &Events, const Matrix2D &K, const Matrix2D &K_try, const double lambdaSpline);
    t_prob rate_Theta_XY(const std::vector<std::shared_ptr<Event>> &Events, const Matrix2D &K, const Matrix2D &K_try, const double lambdaSpline);
    t_prob rate_Theta_XYZ(const std::vector<std::shared_ptr<Event>> &Events, const Matrix2D &K, const Matrix2D &K_try, const double lambdaSpline);

    SplineMatrices prepareCalculSpline_W_Vg0(const std::vector<std::shared_ptr<Event> > &sortedEvents, std::vector<double> &vecH);

    MCMCSpline samplingSpline_multi(std::vector<std::shared_ptr<Event>> &lEvents, std::vector<std::shared_ptr<Event>> &lEventsinit, std::vector<t_matrix> vecYx, std::vector<double> vecYstd, const Matrix2D &R, const Matrix2D &R_1QT, const Matrix2D &Q, const Matrix2D &QT);
    MCMCSpline samplingSpline_multi2(std::vector<std::shared_ptr<Event> > &lEvents, const Matrix2D &R, const Matrix2D &R_1Qt, const Matrix2D& Q);
    MCMCSpline samplingSpline_multi_depth(std::vector<std::shared_ptr<Event> > &lEvents, const Matrix2D &R, const Matrix2D &R_1QT, const Matrix2D& Q);

    std::vector<double> multinormal_sampling (const std::vector<t_matrix>& mu, const Matrix2D& a);
    ColumnVectorLD multinormal_sampling (const ColumnVectorLD& mu, const Matrix2D& a);

    ColumnVectorLD multinormal_sampling2 (const ColumnVectorLD& Y, const Matrix2D& A_1, const DiagonalMatrixLD& w_1);

    ColumnVectorLD multinormal_sampling_depth (const ColumnVectorLD& mu, const Matrix2D& a);

    std::vector<double> splines_prior(const Matrix2D &KK, std::vector<double> &g, std::vector<double> &g_new);
    inline double Signe_Number(const double &a);
    double Prior_F (const Matrix2D &K, const Matrix2D &K_new, const MCMCSpline &s,  const double lambdaSpline);
    std::vector<double> multiMatByVectCol0_KK(const Matrix2D &KKK, const std::vector<double> &gg);

    std::vector<t_matrix> multiMatByVectCol0(const Matrix2D &KKK, const std::vector<t_matrix> &gg);
    std::vector<t_matrix> multiMatByVectCol0(const Matrix2D &KKK, const std::vector<double> &gg);


    t_prob h_exp_fX_theta (std::shared_ptr<Event> e, const MCMCSpline &s, unsigned idx);
    t_prob h_exp_fY_theta (std::shared_ptr<Event> e, const MCMCSpline &s, unsigned idx);
    t_prob h_exp_fZ_theta (std::shared_ptr<Event> e, const MCMCSpline &s, unsigned idx);


    std::vector<double> sampling_spline (std::vector<std::shared_ptr<Event>> &lEvents, SplineMatrices matrices);
    t_prob h_S02_Vg_K(const std::vector<std::shared_ptr<Event>>& events, const double S02_Vg, const double try_S02) const;

    double rapport_detK_plus(const Matrix2D &Mat_old, const Matrix2D &Mat_new);

};

#pragma mark usefull math function

inline double log_p(const double x, const double n) {
    return log(x) / log(n) ;
}

Matrix2D inverseMatSym_originKK(const Matrix2D& matrixLE,  const DiagonalMatrixLD& matrixDE, const int nbBandes, const int shift);

#endif
