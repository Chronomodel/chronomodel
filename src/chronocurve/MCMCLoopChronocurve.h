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

struct Record
{
    double ti;
    double yi;
    double wi;
    double mi;
    double si2;
};

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
    double h_YWI_AY();
    double h_YWI_AY_composante();
    double h_alpha();
    double h_theta();
    double h_VG();
    
    void prepareEventsY();
    void prepareEventY(Event* event);
    
    // Fonctions anciennement liées à do_cravate :
    std::vector<double> createDiagW1();
    
    void orderEventsByTheta();
    void spreadEventsTheta(double minStep = 1e-6);
    void saveEventsTheta();
    void restoreEventsTheta();
    std::map<int, double> mThetasMemo;
    
    // Fonctions anciennement liées à calcul_spline :
    void calcul_spline(bool newTime);
    std::vector<double> calculVecH(const std::vector<Record>& records);

public:
    ModelChronocurve* mModel;
    ChronocurveSettings mChronocurveSettings;
};

#endif
