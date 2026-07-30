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

class MCMCLoopChrono: public MCMCLoop
{
    Q_OBJECT
public:
    MCMCLoopChrono(std::shared_ptr<ModelCurve> model);
    ~MCMCLoopChrono();

protected:
    virtual QString calibrate();

    virtual QString initialize();

    //virtual bool update() {return update_v338_simulated_annealing();};
    virtual bool update() {return update_v3_block_simulated_annealing();};


    //virtual bool update() {return update_v3();};
    virtual bool adapt(const int batchIndex);

    virtual void recordBurnAdapt();
    virtual void recordMH();
    virtual void acquire();

    virtual void finalize();

    bool update_v3();

    bool update_v4_simulated_annealing(); // avec changement de variable xi
    bool update_v338_simulated_annealing(); // variables theta et ti trop correlées, mauvaise convergence
    bool update_v3_block_simulated_annealing();

    /**
 * @brief Échantillonneur Metropolis-Hastings conjoint par blocs (Collapsed Block MH) pour les événements.
 *
 * Effectue la mise à jour simultanée du vecteur de dates \f$ \mathbf{t} = (t_1, \dots, t_N) \f$
 * et de la position de l'événement \f$ \theta \f$.
 *
 * @details L'algorithme décompose le pas MCMC comme suit :
 * - **Proposition des dates \f$ \mathbf{t}^* \f$** : Chaque date \f$ t_i \f$ est échantillonnée via un
 *   mélange entre la courbe de répartition de calibration et une marche aléatoire gaussienne.
 * - **Ratio d'acceptation** : Calcul de la vraisemblance \f$ L \f$, du ratio de Hastings \f$ q \f$,
 *   de la dispersion \f$ S(t) \f$ et de la constante de troncature \f$ Z \f$ sur l'intervalle \f$ [\text{min}, \text{max}] \f$.
 * - **Tirage de \f$ \theta^* \f$** : Échantillonnage exact selon sa loi conditionnelle
 *   \f$ p(\theta \mid \mathbf{t}^*) \f$ (loi normale tronquée).
 * - **Décision MCMC** : Test de Metropolis-Hastings conjoint sur le bloc \f$ (\mathbf{t}^*, \theta^*) \f$.
 * - **Mise à jour dépendante** : Actualisation des deltas, sigmas, wiggles, EDM2 et bornes de phases.
 *
 * @param[in,out] events Liste des événements à échantillonner au cours de l'itération.
 *
 * @note Comme \f$ \theta^* \f$ est tiré selon sa loi conditionnelle exacte, sa densité s'annule
 *       algébriquement dans le ratio de Metropolis-Hastings final.
 */
    void sampler_339(std::vector<std::shared_ptr<Event> > &events);

    void tempering_339(std::vector<std::shared_ptr<Event> > &events, double T);

    bool update_v4();
};

#endif
