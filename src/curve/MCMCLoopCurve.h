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

#elif VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH < 5
#pragma mark Version 3.3.0
    QString initialize_330();
    bool update_330();

#elif VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH >= 5
#pragma mark Version 3.3.5

    QString initialize_335();
    bool update_335(); // estimation de G as Komlan Thesis

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

#if VERSION_MAJOR == KOMLAN
    QString initialize_Komlan();
    bool update_Komlan();
#endif

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



#pragma mark Optimization




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


     double Calcul_Variance_Rice (const std::vector<std::shared_ptr<Event>> &events) const;

     void prepareEventsY(const std::vector<std::shared_ptr<Event>> &events);
     void prepareEventY(std::shared_ptr<Event> const event);


     double minimalThetaDifference(std::vector<std::shared_ptr<Event>> &events);
     double minimalThetaReducedDifference(std::vector<std::shared_ptr<Event>> &events);

     void spreadEventsTheta(std::vector<std::shared_ptr<Event>> &events, double minStep = 1e-6); // not used

    inline  double yearTime(double reduceTime) ;

    std::map<int, double> mThetasMemo;

#pragma mark import_Komlan

    double rate_h_lambda_K(const double current_lambda, const double try_lambda, const double tr_K, const int n);
    t_prob rate_h_lambda_X_335(const double current_lambda, const double try_lambda, const t_prob n_points);
    t_prob rate_h_lambda_XY_335(const double current_lambda, const double try_lambda, const t_prob n_points);
    t_prob rate_h_lambda_XYZ_335(const double current_lambda, const double try_lambda, const t_prob n_points);

    double S02_lambda_WIK (const Matrix2D &K, const int nb_noeuds);
    double h_lambda_Komlan(const Matrix2D &K, const Matrix2D &K_new, const int nb_noeuds, const double &lambdaSpline);
    t_prob rapport_Theta(const std::function<double (std::shared_ptr<Event>)> &fun, const std::vector<std::shared_ptr<Event>> &lEvents, const Matrix2D &K, const Matrix2D &K_new, const double lambdaSpline);
    // rapport Hasting avec utilisation des matrices Y(x, y, z)
    t_prob rate_ftKf(const Matrix2D &Y, const Matrix2D &K, const Matrix2D &Y_try, const Matrix2D &K_try, const double lambda);

    t_prob rate_Theta_X(const std::vector<std::shared_ptr<Event>> &Events, const Matrix2D &K, const Matrix2D &K_try, const double lambdaSpline);
    t_prob rate_Theta_XY(const std::vector<std::shared_ptr<Event>> &Events, const Matrix2D &K, const Matrix2D &K_try, const double lambdaSpline);
    t_prob rate_Theta_XYZ(const std::vector<std::shared_ptr<Event>> &Events, const Matrix2D &K, const Matrix2D &K_try, const double lambdaSpline);

    SplineMatrices prepareCalculSpline_W_Vg0(const std::vector<std::shared_ptr<Event> > &sortedEvents, std::vector<double> &vecH);

    MCMCSpline samplingSpline_multi(std::vector<std::shared_ptr<Event>> &lEvents, std::vector<std::shared_ptr<Event>> &lEventsinit, std::vector<t_matrix> vecYx, std::vector<double> vecYstd, const SparseMatrixLD &R, const Matrix2D &R_1QT, const SparseMatrixLD &Q);
    MCMCSpline samplingSpline_multi2(std::vector<std::shared_ptr<Event> > &lEvents, const SparseMatrixLD &R, const Matrix2D &R_1Qt, const SparseMatrixLD &Q);
    MCMCSpline samplingSpline_multi_depth(std::vector<std::shared_ptr<Event> > &lEvents, const SparseMatrixLD &R, const Matrix2D &R_1QT, const SparseMatrixLD& Q);

    std::vector<double> multinormal_sampling (const std::vector<t_matrix>& mu, const Matrix2D& a);
    ColumnVectorLD multinormal_sampling (const ColumnVectorLD& mu, const Matrix2D& a);

    ColumnVectorLD multinormal_sampling2 (const ColumnVectorLD& Y, const Matrix2D& A_1, const DiagonalMatrixLD& w_1);

    ColumnVectorLD multinormal_sampling_depth (const ColumnVectorLD& mu, const Matrix2D& a);

    std::vector<double> splines_prior(const Matrix2D &KK, std::vector<double> &g, std::vector<double> &g_new);

    double Prior_F (const Matrix2D &K, const Matrix2D &K_new, const MCMCSpline &s,  const double lambdaSpline);
    std::vector<double> multiMatByVectCol0_KK(const Matrix2D &KKK, const std::vector<double> &gg);

    std::vector<t_matrix> multiMatByVectCol0(const Matrix2D &KKK, const std::vector<t_matrix> &gg);
    std::vector<t_matrix> multiMatByVectCol0(const Matrix2D &KKK, const std::vector<double> &gg);

    t_prob h_exp_fX_theta (std::shared_ptr<Event> e, const MCMCSpline &s, unsigned idx);
    t_prob h_exp_fY_theta (std::shared_ptr<Event> e, const MCMCSpline &s, unsigned idx);
    t_prob h_exp_fZ_theta (std::shared_ptr<Event> e, const MCMCSpline &s, unsigned idx);

    std::vector<double> sampling_spline (std::vector<std::shared_ptr<Event>> &lEvents, SplineMatrices matrices);
    t_prob h_S02_Vg_K(const std::vector<std::shared_ptr<Event>>& events, const double S02_Vg, const double try_S02) const;


};



#endif
