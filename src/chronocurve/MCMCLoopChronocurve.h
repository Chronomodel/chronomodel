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

#ifndef MCMCLOOPCHRONOCURVE_H
#define MCMCLOOPCHRONOCURVE_H

#include "MCMCLoop.h"
#include "ChronocurveSettings.h"
#include "ChronocurveUtilities.h"
#include "EventKnown.h"
#include <vector>

class Project;
class ModelChronocurve;
class Event;


class MCMCLoopChronocurve: public MCMCLoop
{
    Q_OBJECT
public:
    MCMCLoopChronocurve(ModelChronocurve* model, Project* project);
    ~MCMCLoopChronocurve();


    void orderEventsByTheta(QList<Event *> &lEvents);
    void orderEventsByThetaReduced(QList<Event *> &lEvents);
    void spreadEventsThetaReduced(QList<Event *> &lEvents, double minStep = 1e-9);
    void spreadEventsThetaReduced0(QList<Event *> &sortedEvents, double spreadSpan = 0.);


protected:
    virtual QString calibrate();
    virtual void initVariablesForChain();
    virtual QString initMCMC();
    virtual void update();
    virtual bool adapt();
    virtual void finalize();
    
private:
    long double h_YWI_AY (const SplineMatrices& matrices, const QList<Event *> &lEvents, const double alphaLissage, const std::vector<long double> &vecH);
  //  long double h_YWI_AY_composante(SplineMatrices& matrices, QList<Event*> lEvents, const double alphaLissage);
    long double h_YWI_AY_composanteX (const SplineMatrices& matrices, const QList<Event *> lEvents, const double alphaLissage, const std::vector<long double> &vecH);
    long double h_YWI_AY_composanteY (const SplineMatrices& matrices, const QList<Event *> lEvents, const double alphaLissage, const std::vector<long double> &vecH);
    long double h_YWI_AY_composanteZ (const SplineMatrices& matrices, const QList<Event *> lEvents, const double alphaLissage, const std::vector<long double> &vecH);
    long double h_alpha (const SplineMatrices &matrices, const int nb_noeuds, const double &alphaLissage);
    long double h_theta (const QList<Event *> lEvents);
    long double h_VG (const QList<Event *> lEvents);
    
   // double h_YWI_AYX(SplineMatrices& matrices, QList<double> & lX, const double alphaLissage);
 //   long double h_YWI_AY_composanteX(SplineMatrices& matrices, QList<double> lX, const double alphaLissage);
    //double h_alphaX(SplineMatrices& matrices, const int nb_noeuds, const double &alphaLissage);
  //  double h_thetaX(QList<double> lX);
   // double h_VGX(QList<double> lX);

    void prepareEventsY(const QList<Event *> & lEvents);
    void prepareEventY(Event * const event);
    void prepareEventY(EventKnown* const event);
    
    // Fonctions anciennement liées à do_cravate :
    std::vector<long double> createDiagWInv(const QList<Event *> &lEvents);


    long double minimalThetaDifference(QList<Event *>& lEvents);
    long double minimalThetaReducedDifference(QList<Event *> &lEvents);

    void spreadEventsTheta(QList<Event *> &lEvents, double minStep = 1e-6); // not used


    void reduceEventsTheta(QList<Event *> &lEvents);
    double reduceTime(double t);
    void saveEventsTheta(QList<Event *> &lEvents);
    void restoreEventsTheta(QList<Event *> &lEvents);
    std::map<int, double> mThetasMemo;
    
    
    
    std::vector<long double> calculVecH(const QList<Event *> &lEvents);
  //  std::vector<double> calculVecHX(const QList<double> &lX);

    std::vector<long double> getThetaEventVector(const QList<Event *>& lEvents);
    std::vector<long double> getYEventVector(const QList<Event *>& lEvents);
    

    std::vector<std::vector<long double>> calculMatR(std::vector<long double>& vecH);
    std::vector<std::vector<long double>> calculMatQ(std::vector<long double>& vecH);


    SplineMatrices prepareCalculSpline(const QList<Event *> & sortedEvents, std::vector<long double> &vecH);

    SplineResults calculSpline(const SplineMatrices &matrices, const std::vector<long double> &vecY, const double alpha, const std::vector<long double> &vecH);
    SplineResults calculSplineX(const SplineMatrices &matrices, const std::vector<long double> &vecH, std::pair<std::vector<std::vector<long double> >, std::vector<long double> >& decomp, const std::vector<std::vector<long double> > matB, const double alpha);
    SplineResults calculSplineY(const SplineMatrices &matrices, const std::vector<long double> &vecH, std::pair<std::vector<std::vector<long double> >, std::vector<long double> >& decomp, const std::vector<std::vector<long double> > matB, const double alpha);
    SplineResults calculSplineZ(const SplineMatrices &matrices, const std::vector<long double> &vecH, std::pair<std::vector<std::vector<long double> >, std::vector<long double> >& decomp, const std::vector<std::vector<long double> > matB, const double alpha);

    std::vector<long double> calculMatInfluence(const SplineMatrices& matrices, const std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const int nbBandes, const double alpha);
    std::vector<long double> calculSplineError(const SplineMatrices& matrices, const  std::pair<std::vector<std::vector<long double> >, std::vector<long double> > &decomp, const double alpha);
    
    double valeurG(const double t, const MCMCSplineComposante& spline);
    double valeurErrG(const double t, const MCMCSplineComposante& spline);
    double valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0);
    double valeurGSeconde(const double t, const MCMCSplineComposante& spline);

    void valeurs_G_ErrG_GP_GS(const double t, const MCMCSplineComposante& spline, long double& G, long double& ErrG, long double& GP, long double& GS, unsigned& i0);
    
    PosteriorMeanGComposante computePosteriorMeanGComposante(const std::vector<MCMCSplineComposante>& trace);
    std::vector<unsigned> listOfIterationsWithPositiveGPrime (const std::vector<MCMCSplineComposante> &splineTrace);


public:
    ModelChronocurve* mModel;
    ChronocurveSettings mChronocurveSettings;
};

#endif
