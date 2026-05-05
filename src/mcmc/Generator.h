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

#ifndef GENERATOR_H
#define GENERATOR_H

#ifdef DEBUG
#include <iostream>
#include <ostream>
#endif

#include <algorithm>
#include <random>

#ifndef M_PI
template<class T>
constexpr T M_PI = T(3.14159265358979323846264338327950288419716939937510582L); // variable template
#endif

//static std::default_random_engine CharGenerator (int(std::chrono::system_clock::now().time_since_epoch().count()));
#ifdef _WIN32
#include <chrono>
#endif

#ifdef __linux
#include <chrono>
#endif

struct randomChar {
    int _a, _b;
    std::uniform_int_distribution<int> CharDistribution;
    randomChar() {
        _a = 97;
        _b = 122;
        CharDistribution.param(std::uniform_int_distribution<int>::param_type(97, 122));
    }

    
    void reset () { CharDistribution.reset(); }
    
    int operator()() ;//{return CharDistribution(CharGenerator);}
} ;


struct c_unique {
    int current;
    c_unique() {current=0;}
    int operator()() {return ++current;}
    std::string  tostring () {return std::to_string(++current);}
} ;


struct c_UUID {
    c_unique UniqueNumber;
    randomChar CharGen;
    c_UUID() {
        c_unique UniqueNumber;
        static randomChar CharGen;
    }
    std::string operator()()
        {
        using std::chrono::system_clock;
        const system_clock::time_point now = system_clock::now();
          
        auto duration = now.time_since_epoch();
        
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
            duration -= hours;
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
            duration -= minutes;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
            duration -= seconds;
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
            duration -= milliseconds;
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
            duration -= microseconds;
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
         
        std::string uuidName (5, '?');
        
        std::generate_n (uuidName.begin(),5, CharGen);
        
        std::string uuid ;
        
        uuid += uuidName;
        uuid += ':' + std::to_string(hours.count());
        uuid += ':' + std::to_string(minutes.count());
        uuid += ':' + std::to_string(seconds.count());
        uuid += ':' + std::to_string(milliseconds.count());
        uuid += ':' + std::to_string(microseconds.count());
        uuid += ':' + std::to_string(nanoseconds.count());
        uuid += ':';
        uuid.append( UniqueNumber.tostring());
        
        return uuid;
        }

};



class Generator
{
public:
    Generator();
    virtual ~Generator();
    static unsigned createSeed();
    static void initGenerator (const unsigned seed);

    static inline double randomUniform(const double min = 0.0 , const double max = 1.0) noexcept
    {
        return min + sDoubleDistribution(sEngine) * (max - min);
    }

    static inline int randomUniformInt(const int min = 0, const int max = 1) noexcept
    {
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(sEngine);
    }

    //static double gaussByDoubleExp(const double mean, const double sigma, const double min, const double max) ;
    static inline double gaussByDoubleExp(const double mean, const double sigma, const double min, const double max)
    {
        // -----------------------------------------------------------------
        //  Normalisation de l’intervalle
        // -----------------------------------------------------------------
        const long double x_min = (min  - mean) / sigma;
        const long double x_max = (max  - mean) / sigma;
        double x = 0.0;

        // -----------------------------------------------------------------
        //  Cas 1 – intervalle très étroit (rejet uniforme)
        // -----------------------------------------------------------------
        if ((x_max - x_min) < 0.1L) {
            while (true) {
                x = static_cast<double>(x_min + (x_max - x_min) * randomUniform());
                const double u = randomUniform();
                // mode = point de l’intervalle le plus proche de 0
                const double mode = (x_min > 0.0) ? static_cast<double>(x_min)
                                                  : ((x_max < 0.0) ? static_cast<double>(x_max) : 0.0);
                // On accepte avec la probabilité exp(-x²/2) / exp(-mode²/2)
                if (std::log(u) <= 0.5 * (mode * mode - x * x))
                    return std::clamp(mean + x * sigma, min, max);
            }
        }

        // -----------------------------------------------------------------
        //  Pré‑calculs pour le rejet double‑exponential
        // -----------------------------------------------------------------
        constexpr long double SQRT_E = 1.64872127070012814689L;   // sqrt(e)
        double exp_x_min = 0.0, exp_x_max = 0.0;
        double exp_minus_x_min = 0.0, exp_minus_x_max = 0.0;
        double c = 0.0, f0 = 0.0;

        // fonction d’exponentielle protégée contre le débordement
        auto safeExp = [&](long double v) -> double {
            constexpr long double EXP_LIMIT = 700.0L;
            if (v >  EXP_LIMIT) return std::numeric_limits<double>::infinity();
            if (v < -EXP_LIMIT) return 0.0;
            return std::exp(v);
        };

        if ((x_min < 0.0L) && (x_max > 0.0L)) {
            exp_x_min      = safeExp(x_min);
            exp_minus_x_max= safeExp(-x_max);
            c   = 1.0 - 0.5 * (exp_x_min + exp_minus_x_max);
            f0  = 0.5 * (1.0 - exp_x_min) / c;
        }
        else {
            if (x_min >= 0.0L) {
                exp_minus_x_min = safeExp(-x_min);
                exp_minus_x_max = safeExp(-x_max);
            } else {
                exp_x_min = safeExp(x_min);
                exp_x_max = safeExp(x_max);
            }
        }


        // -----------------------------------------------------------------
        //  Boucle d’accept‑reject
        // -----------------------------------------------------------------
        double ur = 1.0;
        long double rap = 0.0;
        int trials = 0;
        constexpr int LIMIT = 100000;   // garde‑fou (devrait jamais être atteint)
        while (rap < ur && trials < LIMIT) {
            const double u = randomUniform();
            // ---- proposition x dans [x_min , x_max] ----
            if (x_min < 0.0L && x_max > 0.0L) {          // intervalle qui coupe 0
                if (u <= f0)
                    x = std::log(exp_x_min + 2.0 * c * u);
                else
                    x = -std::log(1.0 - 2.0 * c * (u - f0));
            }
            else {
                if (x_min >= 0.0L) {                     // intervalle entièrement positif
                    x = -std::log(exp_minus_x_min - u * (exp_minus_x_min - exp_minus_x_max));
                } else {                                 // intervalle entièrement négatif
                    x =  std::log(exp_x_min - u * (exp_x_min - exp_x_max));
                }
            }
            // ---- tirage d’un u de comparaison ----
            ur = randomUniform();
            // ---- facteur d’acceptation (rap) ----
            if (x_min >= 1.0L) {
                rap = std::exp(0.5L * (x_min * x_min - x * x) + x - x_min);
            }
            else if (x_max <= -1.0L) {
                rap = std::exp(0.5L * (x_max * x_max - x * x) + x_max - x);
            }
            else {
                rap = std::exp(-0.5L * x * x + std::fabs(x)) / SQRT_E;
            }
            ++trials;
        }

        // -----------------------------------------------------------------
        //  Résultat final (clamp pour éliminer les erreurs d’arrondi)
        // -----------------------------------------------------------------
        double res = mean + x * sigma;
        res = std::clamp(res, min, max);
#ifdef DEBUG
        if (res <= min || res >= max) {
            std::cout << "[gaussByDoubleExp]  "
                      << " min=" << min << " max=" << max
                      << " → res=" << res
                      << ((res >= min && res <= max) ? "✅ YES" : "❌ NO")
                      << std::endl;
        }
#endif
        return res;
    }

    // Helper pour les queues (Exponentielle translatée)
    static inline double sampleTail(double a, double b)
    {
        // Preconditions : a > 0, b > a
        const double z = a * (b - a);               // produit qui apparaît dans exp(-z)

        // 1️⃣  Calcul stable de (1 - exp(-z))
        double oneMinusExp;
        if (z > 700.0) {                            // seuil pratique pour éviter le sous‑débordement
            oneMinusExp = 1.0;                      // exp(-z) ≈ 0  → 1 - 0 = 1
        }
        else {
            // exp(-z) = 1 + expm1(-z)  → 1 - exp(-z) = -expm1(-z)
            oneMinusExp = -std::expm1(-z);
        }

        double x;
        while (true) {
            const double u = Generator::randomUniform();   // U(0,1)

            // 2️⃣  Tirage exponentiel tronqué (formule stable)
            //    x = a - (1/a) * log( 1 - u * (1 - exp(-z)) )
            const double inner = 1.0 - u * oneMinusExp;    // toujours dans (0,1]
            // protection contre le cas où inner == 0 à cause d’un arrondi
            const double logInner = (inner <= 0.0) ? -std::numeric_limits<double>::infinity()
                                                   : std::log(inner);
            x = a - (1.0 / a) * logInner;

            // 3️⃣  Accept‑reject (gaussienne)
            const double v = Generator::randomUniform();
            if (std::log(v) <= -0.5 * (x - a) * (x - a)) {
                return x;          // x ∈ [a , b] (sauf erreurs numériques très faibles)
            }
            // sinon on recommence
        }
    }



    /**
 * @brief Générateur de distribution normale tronquée hautement optimisé.
 * * @details
 * Implémente une stratégie de rejet hybride pour garantir un taux d'acceptation
 * optimal (\f$ \approx 60-100\% \f$) même dans les cas critiques :
 * - **Intervalle étroit :** Rejet sur distribution uniforme (évite le blocage).
 * - **Queues de distribution :** Algorithme de Christian Robert (enveloppe exponentielle) 1995.
 * - **Centre de masse :** Utilisation de la normale standard (Zigghurat).
 * @param mu    Moyenne de la distribution originale.
 * @param sigma Écart-type de la distribution originale.
 * @param low   Borne inférieure de troncature.
 * @param high  Borne supérieure de troncature.
 *
 * * @return double Valeur échantillonnée dans l'intervalle [low, high].
 */
    static inline double truncatedNormal(const double mu, const double sigma, double low, double high)
    {
        // 1. Normalisation
        const double a = (low - mu) / sigma;
        const double b = (high - mu) / sigma;

        double x;

        // --- CAS 1 : Intervalle étroit ---
        // Si l'intervalle est plus petit que 0.1 sigma, la densité est presque uniforme.
        // On utilise un rejet sur l'uniforme, très efficace ici.
        if ((b - a) < 0.1) {
            while (true) {
                x = a + (b - a) * randomUniform();
                double u = randomUniform();
                // On accepte avec la probabilité exp(-x²/2) / exp(-mode²/2)
                // Pour être sûr, on utilise le point de l'intervalle le plus proche de 0
                double mode = (a > 0) ? a : ((b < 0) ? b : 0.0);
                if (std::log(u) <= 0.5 * (mode * mode - x * x)) {
                    return mu + x * sigma;
                }
            }
        }

        // --- CAS 2 : Queue de distribution (Robert, 1995) ---
        if (a > 0.5) {
            x = sampleTail(a, b);
        }
        else if (b < -0.5) {
            x = -sampleTail(-b, -a);
        }
        // --- CAS 3 : Centre de la cloche (Rejet simple) ---
        else {
            do {
                x = sNormalDistribution(sEngine);
            } while (x < a || x > b);
        }
#ifdef DEBUG
        auto res = mu + x * sigma;
        if (res < low || high < res) {
            std::cout << "[truncatedNormal] 🔄 "
                      << " low = " << low << " high = " << high
                      << ", res = " << res
                      << ((low < res && res < high) ? "✅ YES" : "❌ NO")
                      << std::endl;
        }
#endif
        return mu + x * sigma;
    }

    static inline double gaussByBoxMuller(const double mean, const double sigma);
    //static double shrinkage (const double variance, const double shrinkage); // obsolete; à controler

    static double xorshift64star(void);
    static uint64_t xorshift64starSeed;
    static inline double to_double(uint64_t x) {
       // const union { uint64_t i; double d; } u = { .i = UINT64_C(0x3FF) << 52 | x >> 12 }; // don't work with MSVC2015
       // return u.d - 1.0;
        return static_cast<double> (x);
    }

    static c_UUID UUID;

    static inline double shrinkageUniforme(const double shrinkage)
    {
        const double u = Generator::randomUniform();
        const double x = shrinkage * ((1 - u) / u);
        return x;
    }

    static inline double gammaDistribution(const double alpha, const double beta);
    static inline double exponentialeDistribution(const double meanexp);

    /** @brief Retourne un nombre gaussien N(mu, sigma²).
     *
     *  La distribution N(0,1) est créée une fois par thread et ré‑utilisée.
     *  La transformation mu + sigma * Z (Z~N(0,1)) évite la reconstruction
     *  de l’objet distribution à chaque appel.
     *
     *  @param mu    moyenne souhaitée
     *  @param sigma écart‑type souhaité (sigma==0 → retourne mu)
     *  @return      valeur tirée selon N(mu, sigma²)
     */
    // ---------------------------------------------------------------------
    // Implémentation inline (définie dans le .cpp pour garder le code
    // centralisé, mais marquée `inline` afin que le compilateur l’inligne).
    // ---------------------------------------------------------------------
    static inline double normalDistribution(double mu = 0, double sigma = 1) noexcept
    {
        // Cas trivial : sigma == 0 → la loi dégénérée en mu.
        if (sigma == 0.0) return mu;

        // Distribution N(0,1) créée une fois par thread.
        // Le signe « +[] » force la lambda à être constexpr (C++17) → le
        // compilateur peut la transformer en fonction inline.

        //static thread_local std::normal_distribution<double> dist01(0.0, 1.0);
        // Box‑Muller (ou autre algorithme interne) produit deux valeurs.
        // En conservant l’objet, le deuxième nombre est ré‑utilisé automatiquement.
        return mu + sigma * sNormalDistribution(sEngine);
    }

private:
    
    static double boxMuller() ;
    static std::mt19937 sEngine;
    static std::uniform_real_distribution<double> sDoubleDistribution;
    //static thread_local std::normal_distribution<double> sNormalDistribution;
    static std::normal_distribution<double> sNormalDistribution;

    //https://en.wikipedia.org/wiki/Xorshift


};

#endif
