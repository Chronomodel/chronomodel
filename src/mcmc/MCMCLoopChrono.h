/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
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
    //void sampler_339_Couple(std::vector<std::shared_ptr<Event> > &events);


    void sampler_339_SingleSite_SliceSampling(std::vector<std::shared_ptr<Event>> &events); // trop lent
    void sampler_339_SingleSite_NonCentered(std::vector<std::shared_ptr<Event>> &events);

    void sampler_naif(std::vector<std::shared_ptr<Event>> &events);

    void tempering_339(std::vector<std::shared_ptr<Event> > &events, double T);
    void tempering_339_ti_marg(std::vector<std::shared_ptr<Event> > &events, double T);

    bool update_v4();

    friend class MCMCLoopCurve;
};

#endif
