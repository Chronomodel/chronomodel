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

#ifndef PARALLELFOR_H
#define PARALLELFOR_H

#include <QThreadPool>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <vector>

/**
 * @brief Exécute en parallèle une liste de tâches indépendantes (post-traitement).
 *
 * - Threads du QThreadPool global : threads persistants, donc les caches
 *   FFTWThreadCache (plans + buffers par thread) restent valides d'un appel
 *   à l'autre — pas de re-planification FFTW_MEASURE à chaque appel.
 * - Ordonnancement dynamique par compteur atomique : les variables ont des
 *   coûts très inégaux (θ vs σ, longueur des traces, etc.).
 * - Le thread appelant participe au travail.
 * - tryStart() et non start() : si le pool est saturé (ou si l'appelant est
 *   lui-même un thread du pool), on ne met rien en file d'attente, l'appelant
 *   fait le travail -> aucun interblocage possible.
 * - La première exception levée par une tâche est relancée dans l'appelant
 *   (les QString lancées par le code de calcul sont préservées).
 *
 * Pré-requis : les tâches n'écrivent que dans des objets disjoints
 * (chaque MetropolisVariable n'écrit que dans ses propres membres).
 */
inline void parallel_run(std::vector<std::function<void()>>& tasks)
{
    const std::size_t N = tasks.size();
    if (N == 0)
        return;

    QThreadPool* pool = QThreadPool::globalInstance();
    const int maxHelpers = std::min<int>(pool->maxThreadCount(), static_cast<int>(N)) - 1;

    if (maxHelpers <= 0) {
        for (auto& t : tasks)
            t();
        return;
    }

    std::atomic<std::size_t> next {0};
    std::exception_ptr firstError;
    std::mutex errMutex;

    auto work = [&] {
        for (std::size_t i = next.fetch_add(1); i < N; i = next.fetch_add(1)) {
            try {
                tasks[i]();
            } catch (...) {
                std::lock_guard<std::mutex> lock(errMutex);
                if (!firstError)
                    firstError = std::current_exception();
            }
        }
    };

    // Compteur de fin des threads auxiliaires (évite <latch>, indisponible
    // selon la cible de déploiement macOS)
    std::mutex doneMutex;
    std::condition_variable doneCv;
    int running = 0;

    for (int h = 0; h < maxHelpers; ++h) {
        {
            std::lock_guard<std::mutex> lock(doneMutex);
            ++running;
        }
        const bool started = pool->tryStart([&] {
            work();
            std::lock_guard<std::mutex> lock(doneMutex);
            if (--running == 0)
                doneCv.notify_all();
        });
        if (!started) {
            std::lock_guard<std::mutex> lock(doneMutex);
            --running;
            break; // pool saturé : l'appelant fera le reste
        }
    }

    work();

    {
        std::unique_lock<std::mutex> lock(doneMutex);
        doneCv.wait(lock, [&] { return running == 0; });
    }

    if (firstError)
        std::rethrow_exception(firstError);
}

#endif // PARALLELFOR_H
