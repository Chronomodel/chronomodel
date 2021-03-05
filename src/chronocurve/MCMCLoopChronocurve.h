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

protected:
    virtual QString calibrate();
    virtual void initVariablesForChain();
    virtual QString initMCMC();
    virtual void update();
    virtual bool adapt();
    virtual void finalize();
    
private:
    long double h_YWI_AY(SplineMatrices& matrices, QList<Event *> & lEvents, const double alphaLissage);
    long double h_YWI_AY_composante(SplineMatrices& matrices, QList<Event*> lEvents, const double alphaLissage);
    double h_alpha(SplineMatrices& matrices, const int nb_noeuds, const double &alphaLissage);
    double h_theta(QList<Event*> lEvents);
    double h_VG(QList<Event*> lEvents);
    
   // double h_YWI_AYX(SplineMatrices& matrices, QList<double> & lX, const double alphaLissage);
    long double h_YWI_AY_composanteX(SplineMatrices& matrices, QList<double> lX, const double alphaLissage);
    //double h_alphaX(SplineMatrices& matrices, const int nb_noeuds, const double &alphaLissage);
  //  double h_thetaX(QList<double> lX);
   // double h_VGX(QList<double> lX);

    void prepareEventsY(QList<Event *> & lEvents);
    void prepareEventY(Event* event);
    void prepareEventY(EventKnown* event);
    
    // Fonctions anciennement liées à do_cravate :
    std::vector<double> createDiagWInv(const QList<Event*>& lEvents);
    
    void orderEventsByTheta(QList<Event *> & lEvents);
    void spreadEventsTheta(QList<Event *> & lEvents, double minStep = 1e-6);
    void reduceEventsTheta(QList<Event *> & lEvents);
    double reduceTime(double t);
    void saveEventsTheta(QList<Event *> & lEvents);
    void restoreEventsTheta(QList<Event *> & lEvents);
    std::map<int, double> mThetasMemo;
    
    
    
    std::vector<double> calculVecH(const QList<Event *> &lEvents);
  //  std::vector<double> calculVecHX(const QList<double> &lX);

    std::vector<double> getThetaEventVector(const QList<Event *> & lEvents);
    std::vector<double> getYEventVector(const QList<Event *> &lEvents);
    

    std::vector<std::vector<double>> calculMatR(const QList<Event *> & lEvents);
    std::vector<std::vector<double>> calculMatQ(const QList<Event *> &lEvents);
 //   std::vector<std::vector<double>> calculMatRX(const QList<double> &lX);
 //   std::vector<std::vector<double>> calculMatQX(const QList<double> &lX);


    std::vector<std::vector<double>> transpose(const std::vector<std::vector<double>>& matrix, const int nbDiag);
    std::vector<std::vector<double>> multiMatParDiag(const std::vector<std::vector<double>>& matrix, const std::vector<double>& diag, const int nbBandes);
    std::vector<std::vector<double>> multiDiagParMat(const std::vector<double>& diag, const std::vector<std::vector<double>>& matrix, const int nbBandes);
    std::vector<double> multiMatParVec(const std::vector<std::vector<double>>& matrix, const std::vector<double>& vec, const int nbBandes);
    std::vector<std::vector<double>> addMatEtMat(const std::vector<std::vector<double>>& matrix1, const std::vector<std::vector<double>>& matrix2, const int nbBandes);
    std::vector<std::vector<double>> addIdentityToMat(const std::vector<std::vector<double>>& matrix);
    std::vector<std::vector<double>> multiConstParMat(const std::vector<std::vector<double>>& matrix, const double c, const int nbBandes);
    std::vector<std::vector<double>> multiMatParMat(const std::vector<std::vector<double>>& matrix1, const std::vector<std::vector<double>>& matrix2, const int nbBandes1, const int nbBandes2);
    std::vector<std::vector<double>> inverseMatSym(const std::vector<std::vector<double>>& matrix1, const std::vector<double>& matrix2, const int nbBandes, const int shift);
    double sumAllMatrix(const std::vector<std::vector<double>>& matrix);
    double sumAllVector(const std::vector<double>& matrix);
    
    std::pair<std::vector<std::vector<double> >, std::vector<double> > decompositionCholesky(const std::vector<std::vector<double>>& matrix, const int nbBandes, const int shift);
    
    std::vector<double> resolutionSystemeLineaireCholesky(std::vector<std::vector<double>> matL, std::vector<double> matD, std::vector<double> vecQtY);
    
    SplineMatrices prepareCalculSpline(const QList<Event *> & sortedEvents);
    SplineMatrices prepareCalculSplineX(const QList<double>& lX);
    SplineResults calculSpline(SplineMatrices& matrices);
    
    std::vector<double> calculMatInfluence(const SplineMatrices& matrices, const SplineResults& splines, const int nbBandes);
    std::vector<double> calculSplineError(const SplineMatrices& matrices, const SplineResults& splines);
    
    double valeurG(const double t, const MCMCSplineComposante& spline, const int n);
    double valeurErrG(const double t, const MCMCSplineComposante& spline, const int n);
    double valeurGPrime(const double t, const MCMCSplineComposante& spline, const int n);
    double valeurGSeconde(const double t, const MCMCSplineComposante& spline, const int n);
    
    PosteriorMeanGComposante computePosteriorMeanGComposante(const std::vector<MCMCSplineComposante>& trace);

public:
    ModelChronocurve* mModel;
    ChronocurveSettings mChronocurveSettings;
};

#endif
