/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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
#include "EventKnown.h"
#include "Matrix.h"
#include <vector>

class Project;
class ModelCurve;
class Event;


class MCMCLoopCurve: public MCMCLoop
{
    Q_OBJECT

public:
    ModelCurve* mModel;
    CurveSettings mCurveSettings;

public:
    MCMCLoopCurve(ModelCurve* model, Project* project);
    ~MCMCLoopCurve();

   // void orderEventsByTheta(QList<Event *> &lEvents); // Obsolete
    void orderEventsByThetaReduced(QList<Event *> &lEvents);
    void spreadEventsThetaReduced(QList<Event *> &lEvents, double minStep = 1e-9);
    void spreadEventsThetaReduced0(QList<Event *> &sortedEvents, long double spreadSpan = 0.);

protected:
    virtual QString calibrate();
    virtual void initVariablesForChain();
    virtual QString initialize();
    virtual bool update();
    virtual bool adapt(const int batchIndex);
    virtual void memo();
    virtual void finalize();
    
private:
    long double h_YWI_AY (const SplineMatrices& matrices, const QList<Event *> &lEvents, const long double lambdaSpline, const std::vector<long double> &vecH);
  //  long double h_YWI_AY_composante(SplineMatrices& matrices, QList<Event*> lEvents, const double alphaLissage);
    long double h_YWI_AY_composanteX (const SplineMatrices& matrices, const QList<Event *> lEvents, const double lambdaSpline, const std::vector<long double> &vecH);
    long double h_YWI_AY_composanteX_origin (const SplineMatrices& matrices, const QList<Event *> lEvents, const double lambdaSpline, const std::vector<long double> &vecH);

    long double h_YWI_AY_composanteY (const SplineMatrices& matrices, const QList<Event *> lEvents, const long double lambdaSpline, const std::vector<long double> &vecH);
    long double h_YWI_AY_composanteZ (const SplineMatrices& matrices, const QList<Event *> lEvents, const long double lambdaSpline, const std::vector<long double> &vecH);
    long double h_lambda (const SplineMatrices &matrices, const int nb_noeuds, const long double &lambdaSpline);
    long double h_theta (const QList<Event *> lEvents);
    long double h_theta_Event (const Event * e);
    long double h_VG (const QList<Event *> lEvents);
    
    long double S02_lambda (const SplineMatrices &matrices, const int nb_noeuds);
    double Calcul_Variance_Rice (const QList<Event *> lEvents);
   // double h_YWI_AYX(SplineMatrices& matrices, QList<double> & lX, const double alphaLissage);
 //   long double h_YWI_AY_composanteX(SplineMatrices& matrices, QList<double> lX, const double alphaLissage);
    //double h_lambdaX(SplineMatrices& matrices, const int nb_noeuds, const double &alphaLissage);
  //  double h_thetaX(QList<double> lX);
   // double h_VGX(QList<double> lX);

    void prepareEventsY(const QList<Event *> & lEvents);
    void prepareEventY(Event * const event);

    std::vector<long double> createDiagWInv(const QList<Event *> &lEvents);

    long double minimalThetaDifference(QList<Event *>& lEvents);
    long double minimalThetaReducedDifference(QList<Event *> &lEvents);

    void spreadEventsTheta(QList<Event *> &lEvents, double minStep = 1e-6); // not used

    inline void reduceEventsTheta(QList<Event *> &lEvents);
    //long double reduceTime(double t);
    inline long double yearTime(double reduceTime) ;
    void saveEventsTheta(QList<Event *> &lEvents);
    void restoreEventsTheta(QList<Event *> &lEvents);



    std::map<int, double> mThetasMemo;
    
    std::vector<long double> calculVecH(const QList<Event *> &lEvents);
  //  std::vector<double> calculVecHX(const QList<double> &lX);

    std::vector<long double> getThetaEventVector(const QList<Event *>& lEvents);
    std::vector<long double> getYEventVector(const QList<Event *>& lEvents);

    Matrix2D calculMatR(std::vector<long double>& vecH);
    Matrix2D calculMatQ(std::vector<long double>& vecH);

    MCMCSpline currentSpline (QList<Event *> &lEvents, bool doSortAndSpreadTheta = false, std::vector<long double> vecH = std::vector<long double>(), SplineMatrices matrices = SplineMatrices());

    SplineMatrices prepareCalculSpline(const QList<Event *> & sortedEvents, std::vector<long double> &vecH);

    SplineResults calculSpline(const SplineMatrices &matrices, const std::vector<long double> &vecY, const double lambdaSpline, const std::vector<long double> &vecH);
    SplineResults calculSplineX(const SplineMatrices &matrices, const std::vector<long double> &vecH, std::pair<Matrix2D, std::vector<long double> > &decomp, const Matrix2D matB, const double lambdaSpline);
    SplineResults calculSplineY(const SplineMatrices &matrices, const std::vector<long double> &vecH, std::pair<Matrix2D, std::vector<long double> >& decomp, const Matrix2D matB, const double lambdaSpline);
    SplineResults calculSplineZ(const SplineMatrices &matrices, const std::vector<long double> &vecH, std::pair<Matrix2D, std::vector<long double> >& decomp, const Matrix2D matB, const double lambdaSpline);

   // std::vector<long double> calculMatInfluence(const SplineMatrices& matrices, const std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const int nbBandes, const double lambdaSpline);
   // std::vector<long double> calculMatInfluence0(const SplineMatrices& matrices, const std::vector<std::vector<long double>> &matB , const int nbBandes, const double lambdaSpline);

  //  std::vector<long double> calculSplineError(const SplineMatrices& matrices, const  std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const double lambdaSpline);
    std::vector<long double> calculSplineError0(const SplineMatrices& matrices, const Matrix2D &matB, const double lambdaSpline);

    std::vector<long double> calculMatInfluence_origin(const SplineMatrices& matrices, const SplineResults &splines , const int nbBandes, const double lambdaSpline);
    std::vector<long double> calculSplineError_origin(const SplineMatrices& matrices, const SplineResults& splines, const double lambdaSpline);
    std::vector<long double> calcul_spline_variance(const SplineMatrices& matrices, const SplineResults& splines, const double lambdaSpline);

    double valeurG(const double t, const MCMCSplineComposante& spline, unsigned& i0);
    double valeurErrG(const double t, const MCMCSplineComposante& spline, unsigned& i0);
    double valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0);
    double valeurGSeconde(const double t, const MCMCSplineComposante& spline);

    void valeurs_G_ErrG_GP_GS(const double t, const MCMCSplineComposante& spline, long double& G, long double& ErrG, long double& GP, long double& GS, unsigned& i0);
    void valeurs_G_VarG_GP_GS(const double t, const MCMCSplineComposante& spline, long double& G, long double& VarG, long double& GP, long double& GS, unsigned& i0);


    long double initLambdaSpline();

    long double initLambdaSplineByCV();
    long double cross_validation (const SplineMatrices& matrices, const std::vector<long double> &vecH, const double lambdaSpline);
    long double general_cross_validation (const SplineMatrices& matrices, const std::vector<long double> &vecH, const double lambdaSpline);

    PosteriorMeanGComposante computePosteriorMeanGComposante(const std::vector<MCMCSplineComposante>& trace, const QString& ProgressBarText); // Obsolete
    PosteriorMeanGComposante compute_posterior_mean_G_composante(const std::vector<MCMCSplineComposante>& trace, const QString& ProgressBarText);

    PosteriorMeanGComposante compute_posterior_mean_map_G_composante(const std::vector<MCMCSplineComposante>& trace, const long double ymin, const long double ymax, const unsigned gridLength, const QString& ProgressBarText);

    PosteriorMeanGComposante computePosteriorMeanGComposante_chain_allchain(const std::vector<MCMCSplineComposante>& trace, PosteriorMeanGComposante& meanGAllChain, int prevChainSize);// Obsolete

    CurveMap compute_posterior_map_G_composante(const std::vector<MCMCSplineComposante>& trace, const long double ymin, const long double ymax, const unsigned gridLength);
    // Obsolete
    //bool  hasPositiveGPrime (const MCMCSplineComposante& splineComposante);

    bool  hasPositiveGPrimeByDet (const MCMCSplineComposante& splineComposante);

    void memo_PosteriorG(PosteriorMeanGComposante& postGCompo, MCMCSplineComposante &splineComposante, const int realyAccepted);
    // Obsolete
    //std::vector<unsigned> listOfIterationsWithPositiveGPrime (const std::vector<MCMCSplineComposante> &splineTrace);


};

#endif
