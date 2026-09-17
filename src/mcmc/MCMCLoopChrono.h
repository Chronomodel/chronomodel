/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2026

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

#ifndef MCMCLOOPCHRONO_H
#define MCMCLOOPCHRONO_H

#include "MCMCLoop.h"

class Project;
class ModelCurve;
class MCMCLoopCurve;

class MCMCLoopChrono: public MCMCLoop
{
    Q_OBJECT
public:
    MCMCLoopChrono(std::shared_ptr<ModelCurve> model);
    ~MCMCLoopChrono();

protected:
    virtual QString calibrate();

    virtual QString initialize();

    virtual bool update()
    {
        //return update_v3();
        return update_v3_block_simulated_annealing();

    };

    virtual bool adapt(const int batchIndex);

    virtual void recordBurnAdapt();
    virtual void recordMH();
    virtual void acquire();

    virtual void finalize();

    // Echantillonneur Gibbs sur toutes les variables, comme v2
    bool update_v3();
    void sampler_Gibbs(std::vector<std::shared_ptr<Event>> &events);

    //bool update_v4_simulated_annealing(); // avec changement de variable xi

    bool update_v3_block_simulated_annealing();

    // Echantillonneur par bloc sur ti, delta, sigma_ti et theta
    void sampler_339_4v(std::vector<std::shared_ptr<Event> > &events);
    void sampler_339_4vXi(std::vector<std::shared_ptr<Event>> &events);
    void sampler_339_SingleSite(std::vector<std::shared_ptr<Event>> &events); // fonctionne

    void sampler_339_SingleSite_bloc(std::vector<std::shared_ptr<Event>> &events); // fonctionne

    void sampler_339_SingleSite_bloc_delta(std::vector<std::shared_ptr<Event>> &events); // avec shift bloc

    double hmcSampleTheta_temporal(double theta0, long double mu, long double P,
                            double min, double max,
                                   int L, double epsilon);
    void sampler_339_SingleSite_bloc_2(std::vector<std::shared_ptr<Event>> &events);

    void sampler_339_3v(std::vector<std::shared_ptr<Event> > &events);
    void sampler_339_Couple(std::vector<std::shared_ptr<Event> > &events);


    void sampler_339_SingleSite_SliceSampling(std::vector<std::shared_ptr<Event>> &events); // trop lent
    void sampler_339_SingleSite_NonCentered(std::vector<std::shared_ptr<Event>> &events);

    void sampler_naif(std::vector<std::shared_ptr<Event>> &events);

    void tempering_339(std::vector<std::shared_ptr<Event> > &events, double T);
    void tempering_339_ti_marg(std::vector<std::shared_ptr<Event> > &events, double T);

    bool update_v4();

    friend class MCMCLoopCurve;
};

#endif
