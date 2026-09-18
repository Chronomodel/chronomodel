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
#include <thread>

class FFTWThreadCache
{
public:
    struct Buffers { double* grid = nullptr; fftw_complex* spectrum = nullptr; };

    static fftw_plan backward(int M) { return backwardCache().get_plan(M); }
    static fftw_plan forward(int M)  { return forwardCache().get(M); }
    static Buffers get_buffers(int M){ return bufferCache().get(M); }

private:
    static std::mutex& planMutex()   { static std::mutex m; return m; }
    static std::mutex& bufferMutex() { static std::mutex m; return m; }

    struct PlanCacheBackward {
        std::unordered_map<int, fftw_plan> map;
        fftw_plan get_plan(int M)
        {
            if (M <= 0) {
                throw std::invalid_argument("FFT size must be positive");
            }

            std::lock_guard<std::mutex> lock(FFTWThreadCache::planMutex());

            auto it = map.find(M);
            if (it != map.end())
                return it->second;

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

            map[M] = p;
            return p;
        }

    };
    struct PlanCache {
        std::unordered_map<int, fftw_plan> map;
        fftw_plan get(int M)
        {
            if (M <= 0) {
                throw std::invalid_argument("FFT size must be positive");
            }

            std::lock_guard<std::mutex> lock(FFTWThreadCache::planMutex());

            auto it = map.find(M);
            if (it != map.end())
                return it->second;

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

            map[M] = p;
            return p;
        }
    };

    struct BufferCache
    {
        int current_M = 0;
        double* grid = nullptr;
        fftw_complex* spectrum = nullptr;

        Buffers get(int M)
        {
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

    // Remplace le thread_local : une map globale clé = id de thread,
    // protégée par son propre mutex. Détruite une seule fois, à la fin
    // du programme, dans le thread principal — jamais dans un TLS callback.
    static PlanCacheBackward& backwardCache()
    {
        static std::mutex mapMutex;
        static std::unordered_map<std::thread::id, std::unique_ptr<PlanCacheBackward>> perThread;
        std::lock_guard<std::mutex> lock(mapMutex);
        auto& slot = perThread[std::this_thread::get_id()];
        if (!slot) slot = std::make_unique<PlanCacheBackward>();
        return *slot;
    }

    static PlanCache& forwardCache()
    {
        static std::mutex mapMutex;
        static std::unordered_map<std::thread::id, std::unique_ptr<PlanCache>> perThread;
        std::lock_guard<std::mutex> lock(mapMutex);
        auto& slot = perThread[std::this_thread::get_id()];
        if (!slot) slot = std::make_unique<PlanCache>();
        return *slot;
    }

    static BufferCache& bufferCache()
    {
        static std::mutex mapMutex;
        static std::unordered_map<std::thread::id, std::unique_ptr<BufferCache>> perThread;
        std::lock_guard<std::mutex> lock(mapMutex);
        auto& slot = perThread[std::this_thread::get_id()];
        if (!slot) slot = std::make_unique<BufferCache>();
        return *slot;
    }
};


#endif // FFTWTHREAD_H
