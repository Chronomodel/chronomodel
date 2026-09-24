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
private:

    inline static std::mt19937 sEngine{0};
    inline static std::uniform_real_distribution<double> sDoubleUniformDistribution{0.0, 1.0};
    inline static std::normal_distribution<double> sNormalDistribution{0.0, 1.0};

    inline static unsigned int sSeed{0};

public:
    Generator() noexcept { sSeed = 0; }
    virtual ~Generator();

    static unsigned createSeed()
    {
        // Distribution uniforme sur l’intervalle [1, 100000]
        static std::uniform_int_distribution<unsigned> dist(1, 100000);
        return dist(Generator::sEngine);   // valeur aléatoire dans la plage demandée
    }

    static void initGenerator (const unsigned int seed)
    {
        sSeed = seed;

        sEngine.seed(seed);
        sDoubleUniformDistribution.reset();
        sNormalDistribution.reset();

        xorshift64starSeed = static_cast<std::uint64_t>(seed);

    }
    static unsigned int seed()
    {
        return sSeed;
    }

    static inline double randomUniform(const double min = 0.0 , const double max = 1.0) noexcept
    {
        return min + sDoubleUniformDistribution(Generator::sEngine) * (max - min);
    }

    static inline int randomUniformInt(const int min = 0, const int max = 1) noexcept
    {
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(Generator::sEngine);
    }



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
                x = static_cast<double>(x_min + (x_max - x_min) * Generator::randomUniform());
                const double u = Generator::randomUniform();
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
            const double u = Generator::randomUniform();
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
            ur = Generator::randomUniform();
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
    static inline double sampleTail_old(double a, double b)
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
    static inline double truncatedNormal_old(const double mu, const double sigma, double low, double high)
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
                x = a + (b - a) * Generator::randomUniform();
                double u = Generator::randomUniform();
                // On accepte avec la probabilité exp(-x²/2) / exp(-mode²/2)
                // Pour être sûr, on utilise le point de l'intervalle le plus proche de 0
                double mode = (a > 0) ? a : ((b < 0) ? b : 0.0);
                if (std::log(u) <= 0.5 * (mode * mode - x * x)) {
                    return mu + x * sigma;
                }
            }
        }

        // --- CAS 2 : Queue de distribution (Robert, 1995) ---
        if (a > 0.6) {
            x = sampleTail(a, b);
        }
        else if (b < -0.6) {
            x = -sampleTail(-b, -a);
        }
        // --- CAS 3 : Centre de la cloche (Rejet simple) ---
        else {
            do {
                x = Generator::normalDistribution();
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
//--
    static inline double sampleTail(double a, double b)
    {
        // 1️⃣ Taux optimal de Robert (1995)
        const double lambda = 0.5 * (a + std::sqrt(a * a + 4.0));
        const double delta = b - a;
        const double z = lambda * delta;

        // 2️⃣ Calcul ultra-stable de 1 - exp(-lambda * (b - a)) via log1p / expm1
        double oneMinusExp;
        if (z > 700.0) {
            oneMinusExp = 1.0;
        } else {
            oneMinusExp = -std::expm1(-z);
        }

        while (true) {
            const double u = Generator::randomUniform(); // U(0,1)

            // 3️⃣ Tirage exponentiel tronqué dans [a, b] sans perte de précision
            // x = a - (1/lambda) * log1p( -u * (1 - exp(-z)) )
            const double arg = u * oneMinusExp;
            const double logInner = (arg >= 1.0) ? -700.0 : std::log1p(-arg);

            const double x = a - (1.0 / lambda) * logInner;

            // 4️⃣ Accept-Reject de Robert : g(x) = exp(-0.5 * (x - lambda)^2)
            const double diff = x - lambda;
            const double v = Generator::randomUniform();

            // Le ratio d'acceptation de Robert utilise (x - lambda), PAS (x - a)
            if (std::log(v) <= -0.5 * diff * diff) {
                return x; // Tirage parfaitement exact sur [a, b]
            }
        }
    }

    static inline double truncatedNormal(const double mu, const double sigma, double low, double high)
    {
        // 1. Normalisation
        const double a = (low - mu) / sigma;
        const double b = (high - mu) / sigma;

        double x;

        // --- CAS 1 : Queues excentrées à droite (a > 1.5) ---
        // Traite le cas excentré en premier, QUEL QUE SOIT (b - a)
        if (a > 1.5) {
            x = sampleTail(a, b);
        }
        // --- CAS 2 : Queues excentrées à gauche (b < -1.5) ---
        else if (b < -1.5) {
            x = -sampleTail(-b, -a);
        }
        // --- CAS 3 : Intervalle très étroit dans la cloche centrale (|a| <= 1.5, |b| <= 1.5) ---
        // Utilisation d'une proposition uniforme sécurisée pour éviter les boucles du rejet
        else if ((b - a) < 0.1) {
            const double mode = (a > 0.0) ? a : ((b < 0.0) ? b : 0.0);
            const double log_max_dens = -0.5 * mode * mode;

            while (true) {
                x = a + (b - a) * Generator::randomUniform();
                const double u = Generator::randomUniform();
                if (std::log(u) <= (-0.5 * x * x - log_max_dens)) {
                    break;
                }
            }
        }
        // --- CAS 4 : Centre de la cloche (Rejet simple direct) ---
        // Taux d'acceptation très élevé (> 20%) sur [-1.5, 1.5]
        else {
            do {
                x = Generator::normalDistribution();
            } while (x < a || x > b);
        }

        const double res = mu + x * sigma;

#ifdef DEBUG
        if (res < low || res > high) {
            std::cout << "[truncatedNormal] 🔄 "
                      << " low = " << low << " high = " << high
                      << ", res = " << res << " ❌ OUT OF BOUNDS"
                      << std::endl;
        }
#endif

        return res;
    }

//--
    static inline double gaussByBoxMuller(const double mean, const double sigma);

    /**
     * @brief  Uniformly distributed random numbers with Box-Muller transform see: https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
     *
     *  \f$ U_1 \in [0;1]\f$ and \f$ U_2 \in[0;1] \f$
     *
     *  \f$ Z_0=\sqrt{-2 \ln{U_1}} * cos(2  \Pi * U_2) \f$
    */
    // Obsolete
    static inline double boxMuller()
    {
        const double U1 = randomUniform();
        const double U2 = randomUniform();
        return sqrt(-2. * log(U1)) * cos(2. * M_PI * U2);
    }

    //static double shrinkage (const double variance, const double shrinkage); // obsolete; à controler

    static double xorshift64star(void);
    static uint64_t xorshift64starSeed;
    static inline double to_double(uint64_t x) {
       // const union { uint64_t i; double d; } u = { .i = UINT64_C(0x3FF) << 52 | x >> 12 }; // don't work with MSVC2015
       // return u.d - 1.0;
        return static_cast<double> (x);
    }

    static c_UUID UUID;

    /**
 * @brief Draws a random variate from the Uniform Shrinkage distribution,
 *        truncated to the interval [minV, maxV].
 *
 * @details The Uniform Shrinkage distribution (Christen, 1994; Lanos & Philippe, 2015)
 * has density
 * \f[
 *      p(x) = \frac{a}{(x+a)^2}, \qquad x \ge 0,
 * \f]
 * where \f$a\f$ = @p shrinkage plays the role of the scale parameter \f$s_0^2\f$
 * used as the prior on individual error variances \f$\sigma_i^2\f$ in the Event
 * model (see Lanos & Philippe, "Event model: a robust Bayesian tool for
 * chronological modeling", eq. (4)).
 *
 * Sampling is performed by exact inversion of the (truncated) CDF. Using the
 * bijective change of variable
 * \f[
 *      u = \frac{a}{x+a} \quad\Longleftrightarrow\quad x = a\,\frac{1-u}{u},
 * \f]
 * the bounds @p minV and @p maxV map to
 * \f[
 *      u_{max} = \frac{a}{minV + a}, \qquad u_{min} = \frac{a}{maxV + a},
 * \f]
 * and drawing \f$u \sim \mathcal U(u_{min}, u_{max})\f$ then setting
 * \f$x = a(1-u)/u\f$ yields exactly the Uniform Shrinkage density renormalized
 * on \f$[minV, maxV]\f$ — i.e. a true truncated sample, with no boundary atom
 * (no clamping is needed downstream).
 *
 * @param shrinkage Scale parameter \f$a = s_0^2\f$ of the Uniform Shrinkage
 *                  distribution. Must be strictly positive.
 * @param minV      Lower bound of the truncation interval. Must satisfy
 *                  \f$0 \le minV < maxV\f$.
 * @param maxV      Upper bound of the truncation interval. Must satisfy
 *                  \f$minV < maxV\f$.
 *
 * @return A random variate \f$x \in [minV, maxV]\f$ distributed according to
 *         the Uniform Shrinkage density truncated to that interval.
 *
 * @note This function is noexcept: it performs no validation of its arguments.
 *       The caller is responsible for ensuring @p shrinkage > 0 and
 *       0 <= minV < maxV.
 *
 * @see Christen, J. A. (1994). Summarizing a set of radiocarbon determinations:
 *      a robust approach. Applied Statistics, 43(3), 489-503.
 * @see Lanos, P. and Philippe, A. Event model: a robust Bayesian tool for
 *      chronological modeling, eq. (4).
 */
    static inline double shrinkageUniforme(const double shrinkage,
                                           const double minV = 1e-12,
                                           const double maxV = 1e10) noexcept
    {
        // u_min <-> X = maxV ;  u_max <-> X = minV
        const double u_min = shrinkage / (maxV + shrinkage);
        const double u_max = shrinkage / (minV + shrinkage);
        const double u     = Generator::randomUniform(u_min, u_max);
        return shrinkage * ((1.0 - u) / u);
    }
/*
    static inline double shrinkageUniform(const double s02)
    {
        const double u = Generator::randomUniform(0, 1);
        return (s02 * (1. - u) / u);
    }
 */

    static inline double gammaDistribution(const double alpha, const double beta) noexcept
    {
        std::gamma_distribution<double>  gamma(alpha, beta);
        return gamma(Generator::sEngine);
    }
    static inline double inverseGammaDistribution(const double alpha, const double beta) noexcept
    {
        // Pour obtenir une Inv-Gamma(alpha, beta),
        // on doit générer une Gamma(alpha, 1/beta) puis prendre l'inverse.

        // Ici, alpha = 2.0 et beta = 1.0/P
        // Le paramètre 'scale' de std::gamma_distribution doit donc être 1/beta,
        // ce qui revient à 1 / (1/P) = P.

        double scale = 1.0 / beta;
        std::gamma_distribution<double> gamma(alpha, scale);

        double sample = gamma(Generator::sEngine);

        // On retourne l'inverse pour transformer la Gamma en Inverse-Gamma
        return 1.0 / sample;
    }

    static inline double exponentialeDistribution(const double meanexp)
    {
        std::exponential_distribution<double> exponential(meanexp);
        return exponential(sEngine);
    }

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
        return mu + sigma * sNormalDistribution(Generator::sEngine);
    }


    //https://en.wikipedia.org/wiki/Xorshift


};

#endif
