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
#include <vector>

class Project;
class ModelChronocurve;
class Event;

typedef struct SplineMatrices
{
    std::vector<double> diagWInv;
    std::vector<std::vector<double>> matR;
    std::vector<std::vector<double>> matQ;
    std::vector<std::vector<double>> matQT;
    std::vector<std::vector<double>> matQTW_1Q;
    std::vector<std::vector<double>> matQTQ;
} SplineMatrices;

typedef struct SplineResults
{
    std::vector<std::vector<double>> matB;
    std::vector<std::vector<double>> matL;
    std::vector<std::vector<double>> matD;
    
    std::vector<double> vecG;
    std::vector<double> vecGamma;
    
} SplineResults;

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
    double h_YWI_AY(SplineMatrices& matrices);
    double h_YWI_AY_composante(SplineMatrices& matrices);
    double h_alpha(SplineMatrices& matrices);
    double h_theta();
    double h_VG();
    
    void prepareEventsY();
    void prepareEventY(Event* event);
    
    // Fonctions anciennement liées à do_cravate :
    std::vector<double> createDiagWInv();
    
    void orderEventsByTheta();
    void spreadEventsTheta(double minStep = 1e-6);
    void reduceEventsTheta();
    void saveEventsTheta();
    void restoreEventsTheta();
    std::map<int, double> mThetasMemo;
    
    
    
    std::vector<double> calculVecH();
    
    std::vector<double> initVecteur(const int dim);
    std::vector<std::vector<double>> initMatrice(const int rows, const int cols);
    
    std::vector<double> getThetaEventVector();
    std::vector<double> getYEventVector();
    
    std::vector<std::vector<double>> calculMatR();
    std::vector<std::vector<double>> calculMatQ();
    
    std::vector<std::vector<double>> transpose(const std::vector<std::vector<double>>& matrix, const int nbDiag);
    std::vector<std::vector<double>> multiMatParDiag(const std::vector<std::vector<double>>& matrix, const std::vector<double>& diag, const int nbBandes);
    std::vector<std::vector<double>> multiDiagParMat(const std::vector<double>& diag, const std::vector<std::vector<double>>& matrix, const int nbBandes);
    std::vector<double> multiMatParVec(const std::vector<std::vector<double>>& matrix, const std::vector<double>& vec, const int nbBandes);
    std::vector<std::vector<double>> addMatEtMat(const std::vector<std::vector<double>>& matrix1, const std::vector<std::vector<double>>& matrix2, const int nbBandes);
    std::vector<std::vector<double>> addIdentityToMat(const std::vector<std::vector<double>>& matrix);
    std::vector<std::vector<double>> multiConstParMat(const std::vector<std::vector<double>>& matrix, const double c, const int nbBandes);
    std::vector<std::vector<double>> multiMatParMat(const std::vector<std::vector<double>>& matrix1, const std::vector<std::vector<double>>& matrix2, const int nbBandes1, const int nbBandes2);
    std::vector<std::vector<double>> inverseMatSym(const std::vector<std::vector<double>>& matrix1, const std::vector<std::vector<double>>& matrix2, const int nbBandes, const int shift);
    double sumAllMatrix(const std::vector<std::vector<double>>& matrix);
    double sumAllVector(const std::vector<double>& matrix);
    
    std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> decompositionCholesky(const std::vector<std::vector<double>>& matrix, const int nbBandes, const int shift);
    
    std::vector<double> resolutionSystemeLineaireCholesky(std::vector<std::vector<double>> matL, std::vector<std::vector<double>> matD, std::vector<double> vecQtY, const int nbBandes, const int shift);
    
    SplineMatrices prepareCalculSpline();
    SplineResults calculSpline(SplineMatrices& matrices);
    
    std::vector<std::vector<double>> calculMatInfluence(const SplineMatrices& matrices, const SplineResults& splines, const int nbBandes);
    std::vector<double> calculSplineError(const SplineMatrices& matrices, const SplineResults& splines);

public:
    ModelChronocurve* mModel;
    ChronocurveSettings mChronocurveSettings;
};

#endif
