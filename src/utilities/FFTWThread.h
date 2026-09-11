/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2026 - 2026

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


#ifndef FFTWTHREAD_H
#define FFTWTHREAD_H


#include "fftw3.h"
#include <mutex>
#include <unordered_map>
#include <memory>

class FFTWThreadCache
{
public:

    /**
     * @brief Structure regroupant les buffers de travail r2c réutilisables.
     */
    struct Buffers {
        double* grid = nullptr;
        fftw_complex* spectrum = nullptr;
    };

    static fftw_plan backward(int M)
    {
        thread_local PlanCacheBackward cache;
        return cache.get_plan(M);
    }

    /**
     * @brief   Retourne un plan FFTW « forward » (real‑to‑complex) de taille @p M.
     *
     * Le plan est stocké dans un cache **thread‑local**.  La première fois que
     * la fonction est appelée dans un thread donné, le plan est créé avec
     * `FFTW_MEASURE`.  Les appels suivants renvoient le même objet, sans frais
     * supplémentaire.
     *
     * @param[in] M  Taille de la transformée (nombre de points dans le domaine
     *               temporel).  La valeur doit être strictement positive.
     *
     * @return  Un handle `fftw_plan` valide que l’on peut passer à
     *          `fftw_execute_dft_r2c()` ou à d’autres fonctions d’exécution.
     *
     * @throw std::runtime_error  Si la création du plan échoue (par ex.
     *                             allocation insuffisante ou erreur interne de
     *                             FFTW).
     *
     * @note    Le plan est détruit automatiquement lorsque le thread se termine
     *          grâce au destructeur de `PlanCache`.  Il n’est **pas** nécessaire
     *          d’appeler `fftw_destroy_plan()` manuellement.
     *
     * @since   1.0
     */
    static fftw_plan forward(int M)
    {
        thread_local PlanCache cache;
        return cache.get(M);
    }

    /**
     * @brief Retourne les buffers (grid & spectrum) dimensionnés pour la taille @p M.
     *        Alloue la mémoire uniquement au premier appel ou si M change.
     */
    static Buffers get_buffers(int M)
    {
        thread_local BufferCache cache;
        return cache.get(M);
    }


private:
    static std::mutex& planMutex() {
        static std::mutex& m = *new std::mutex();
        return m;
    }

    static std::mutex& bufferMutex() {
        static std::mutex& m = *new std::mutex();
        return m;
    }
    // --- Cache des plans backward ---
    struct PlanCacheBackward
    {
        std::unordered_map<int, std::shared_ptr<fftw_plan_s>> map;

        fftw_plan get_plan(int M)
        {
            // Vérification de la taille valide
            if (M <= 0) {
                throw std::invalid_argument("FFT size must be positive");
            }

            std::lock_guard<std::mutex> lock(FFTWThreadCache::planMutex());

            auto it = map.find(M);
            if (it != map.end())
                return it->second.get(); // Retourne le plan via le shared_ptr

            // Création du plan
            fftw_complex* tmp_in = static_cast<fftw_complex*>(fftw_malloc((M / 2 + 1) * sizeof(fftw_complex)));
            double* tmp_out = static_cast<double*>(fftw_malloc(M * sizeof(double)));

            if (!tmp_in || !tmp_out) {
                fftw_free(tmp_in);
                fftw_free(tmp_out);
                throw std::bad_alloc();
            }

            fftw_plan p = fftw_plan_dft_c2r_1d(M, tmp_in, tmp_out, FFTW_MEASURE);

            fftw_free(tmp_in);
            fftw_free(tmp_out);

            if (!p) {
                throw std::runtime_error("FFTW backward plan creation failed");
            }

            // Stockage dans le cache avec un shared_ptr
            map[M] = std::shared_ptr<fftw_plan_s>(p, [](fftw_plan plan) {
                fftw_destroy_plan(plan);
            });

            return map[M].get();
        }

        ~PlanCacheBackward() {
            // Le destructeur est géré automatiquement par shared_ptr
            map.clear();
        }
    };

    // --- Cache des plans ---
    /**
     * @brief   Structure interne qui stocke les plans FFTW d’un thread.
     *
     * Chaque instance de `PlanCache` possède une map `std::unordered_map<int,
     * fftw_plan>` où la clé est la taille du tableau (`M`) et la valeur le plan
     * correspondant.  Le destructeur parcourt la map et libère chaque plan avec
     * `fftw_destroy_plan()`.
     *
     * @note    Cette structure n’est jamais exposée à l’extérieur de la classe
     *          `FFTWThreadCache`; elle sert uniquement de conteneur privé.
     *
     * @since   1.0
     */
    struct PlanCache
    {
        /** @brief  Map <taille, plan> gérée par le thread. */
        std::unordered_map<int, std::shared_ptr<fftw_plan_s>> map;

        fftw_plan get(int M)
        {
            // Vérification de la taille valide
            if (M <= 0) {
                throw std::invalid_argument("FFT size must be positive");
            }

            std::lock_guard<std::mutex> lock(FFTWThreadCache::planMutex());

            auto it = map.find(M);
            if (it != map.end())
                return it->second.get(); // Retourne le plan via le shared_ptr

            // Création du plan
            double* tmp_in = static_cast<double*>(fftw_malloc(M * sizeof(double)));
            fftw_complex* tmp_out = static_cast<fftw_complex*>(fftw_malloc((M / 2 + 1) * sizeof(fftw_complex)));

            if (!tmp_in || !tmp_out) {
                fftw_free(tmp_in);
                fftw_free(tmp_out);
                throw std::bad_alloc();
            }

            fftw_plan p = fftw_plan_dft_r2c_1d(M, tmp_in, tmp_out, FFTW_MEASURE);

            fftw_free(tmp_in);
            fftw_free(tmp_out);

            if (!p) {
                throw std::runtime_error("FFTW forward plan creation failed");
            }

            // Stockage dans le cache avec un shared_ptr
            map[M] = std::shared_ptr<fftw_plan_s>(p, [](fftw_plan plan) {
                fftw_destroy_plan(plan);
            });

            return map[M].get();
        }

        /**
         * @brief   Destructeur : libère tous les plans détenus par le thread.
         *
         * Le destructeur est invoqué automatiquement lorsque le thread se
         * termine (ou à la fin du programme pour le thread principal).  Chaque
         * plan stocké dans `map` est détruit avec `fftw_destroy_plan()`.
         *
         * @since   1.0
         */
        ~PlanCache() {
            // Le destructeur est géré automatiquement par shared_ptr
            map.clear();
        }
    };

    // --- Cache des buffers de travail ---
    struct BufferCache
    {
        int current_M = 0;
        double* grid = nullptr;
        fftw_complex* spectrum = nullptr;

        Buffers get(int M)
        {
            // Vérification de la taille valide
            if (M <= 0) {
                throw std::invalid_argument("Buffer size must be positive");
            }

            std::lock_guard<std::mutex> lock(FFTWThreadCache::bufferMutex());

            if (current_M != M) {
                if (grid) fftw_free(grid);
                if (spectrum) fftw_free(spectrum);

                current_M = M;
                grid = static_cast<double*>(fftw_malloc(M * sizeof(double)));
                spectrum = static_cast<fftw_complex*>(fftw_malloc((M / 2 + 1) * sizeof(fftw_complex)));

                if (!grid || !spectrum) {
                    fftw_free(grid);
                    fftw_free(spectrum);
                    throw std::bad_alloc();
                }
            }
            return { grid, spectrum };
        }

        ~BufferCache() {
            if (grid) fftw_free(grid);
            if (spectrum) fftw_free(spectrum);
        }
    };
};



#endif // FFTWTHREAD_H
