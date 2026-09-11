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

#ifndef MHVARIABLE_H
#define MHVARIABLE_H

#include "MetropolisVariable.h"
#include "Generator.h"

/**
 * @brief Test d'acceptation Metropolis-Hastings basé sur un rapport donné
 *
 * Cette fonction évalue si une proposition doit être acceptée selon le critère
 * Metropolis-Hastings. Elle utilise un rapport fourni pour déterminer
 * l'acceptation, avec plusieurs cas spéciaux :
 *
 * - Si le rapport est négatif ou NaN : rejet forcé
 * - Si le rapport est égal à 2.0 : acceptation forcé
 * - Si le rapport est supérieur ou égal à 1.0 : acceptation certaine
 * - Si le rapport est compris entre 0 et 1 : acceptation probabiliste
 *   basée sur un tirage uniforme
 *
 * @param rate Le rapport d'acceptation utilisé pour le test
 *             - Si rate < 0.0 : rejet forcé (NaN ou valeur négative)
 *             - Si rate == 2.0 : acceptation forcé
 *             - Si rate >= 1.0 : acceptation certaine
 *             - Si 0.0 <= rate < 1.0 : acceptation probabiliste
 *
 * @return true si la proposition est acceptée, false sinon
 *
 * @note Dans le cas probabiliste, un nombre aléatoire uniforme est généré
 *       via Generator::randomUniform() et la proposition est acceptée si
 *       uniform < rate.
 *
 * @see Generator::randomUniform()
 */
inline bool MHAcceptanceTest(double rate)
{
    const bool accepted = (rate < 0.0) ?                     // NaN ou négatif → rejet
                              false
                                       : (rate == 2.0) ?               // force-accept
                                             true
                                                       : (rate >= 1.0) ?               // acceptation certaine
                                                             true
                                                                       : ([&]()->bool{                // cas probabiliste 0 ≤ rate < 1
                                                                             const double uniform = Generator::randomUniform(); // ∈[0,1)
#ifdef DEBUG
                                                                             if (uniform == 0.0)
                                                                                 qDebug() << "[" << __func__ << "]  uniform == 0";
#endif
                                                                             return (uniform < rate);
                                                                         }());
    return accepted;
}

/**
 * @brief Test d'acceptation Metropolis-Hastings basé sur le logarithme du rapport
 *
 * Cette fonction évalue si une proposition doit être acceptée selon le critère
 * Metropolis-Hastings en utilisant le logarithme du rapport plutôt que le rapport lui-même.
 * Cette approche est numériquement plus stable lorsqu'on travaille avec des très petits rapports.
 *
 * Les différents cas sont traités comme suit :
 *
 * - Si le logarithme du rapport est NaN ou -∞ : rejet forcé
 * - Si le logarithme du rapport est supérieur ou égal à 0 : acceptation certaine
 *   (correspondant à un rapport >= 1)
 * - Si le logarithme du rapport est négatif : acceptation probabiliste
 *   basée sur la comparaison des logarithmes pour éviter les sous-débordements
 *
 * @param log_rate Le logarithme du rapport d'acceptation utilisé pour le test
 *             - Si log_rate est NaN ou -INFINITY : rejet forcé
 *             - Si log_rate >= 0.0 : acceptation certaine
 *             - Si log_rate < 0.0 : acceptation probabiliste
 *
 * @return true si la proposition est acceptée, false sinon
 *
 * @note Cette version utilise le logarithme du rapport pour une meilleure stabilité
 *       numérique. L'acceptation probabiliste compare log(u) < log_rate,
 *       ce qui est équivalent à u < exp(log_rate) mais évite les problèmes
 *       d'underflow avec des très petits rapports.
 *
 * @see Generator::randomUniform()
 */
inline bool MHAcceptanceTest_log(double log_rate)
{
    bool accepted;
    // --------------------------------------------------------------
    // 1️⃣ Cas pathologiques (NaN, -inf) → rejet
    // --------------------------------------------------------------
    if (std::isnan(log_rate) || log_rate == -INFINITY) {

        accepted = false;
    }
    // --------------------------------------------------------------
    // 2️⃣  Acceptation forcée (rate >= 1  ⇔  log_rate >= 0)
    // --------------------------------------------------------------
    else if (log_rate >= 0.0) {
        // couvre rate == 2.0, rate > 1.0 et le cas exact rate == 1.0
        accepted = true;
    }
    // --------------------------------------------------------------
    // 3️⃣  Acceptation probabiliste (0 < rate < 1  ⇔  log_rate < 0)
    // --------------------------------------------------------------
    else {

        const double u = Generator::randomUniform();          // 0 < u < 1
        const double log_u = std::log(u);    // toujours < 0
        accepted = (log_u < log_rate);       // équivalent à u < exp(log_rate)
#ifdef DEBUG
        if (u == 0.0)
            std::cerr << "[" << __func__ << "] uniform == 0\n";
#endif
    }
    return accepted;
}


enum class SamplerProposal : int
{
    // ---------- Event ----------
    eNone        = -2,   // use with S02VG
    eFixe        = -1,   // use with Type==eBound
    eDoubleExp   = 0,    // default method for Event->theta for EDM1
    eEventPrior  = 1,    // default method for Event->theta for EDM2
    eRWAdaptGauss = 2,   // also for data
    // ---------- Data ----------
    eDatePrior   = 3,
    eLikelihood   = 4,
    ePrior       = 5,
    // eMHSymGaussAdapt = 5   // (commenté – valeur dupliquée)
};

class MHVariable: public MetropolisVariable
{
public:

    double mSigmaMH;

    // Buffer glissant de la taille d'un batch pour calculer la courbe d'évolution
    // du taux d'acceptation chaine par chaine

    std::deque<bool> mLastMHAccepts;
    std::size_t mLastMHAcceptsLength;


    // Nombre d'acceptations cumulées pour toutes les chaines
    // sur les parties acquisition uniquement.
    // A stocker dans le fichier résultats .res !

    long long mMHAcceptcountSinceAquire;// old mAllAccept


    // Computed at the end as numerical result :
    double mGlobalAcceptationPerCent;

    // Buffer contenant tous les taux d'acceptation calculés (1 par batch)
    // On en affiche des sous-parties (correspondant aux chaines) dans la vue des résultats
    // A stocker dans les résultats!

    std::shared_ptr<std::vector<double>> mHistoryAcceptRateMH;

    SamplerProposal mSamplerProposal;

    MHVariable();
    explicit MHVariable(const MHVariable& origin);
    /** move constructor */
    MHVariable(MHVariable&& other) noexcept;

    explicit MHVariable(const MetropolisVariable& origin);
    virtual ~MHVariable();
    void shrink_to_fit() noexcept override;

    void clear() override;
    void clear_and_shrink() noexcept override;

    void remove_smoothed_densities() override;
    void reserve(const size_t reserve) override;

    MHVariable& operator=(const MHVariable& origin);

    inline double getCurrentAcceptRate() const
    {
        if (mLastMHAccepts.empty())
            return 0.0;

        std::size_t trueCount = std::count(mLastMHAccepts.begin(),
                                           mLastMHAccepts.end(),
                                           true);
        return static_cast<double>(trueCount) / mLastMHAccepts.size();
    }

    inline void saveCurrentAcceptRate()
    {
        mHistoryAcceptRateMH->push_back(100. * getCurrentAcceptRate());
    }

    /**
 * @brief MHVariable::tryUpdate
 * @param x : Value proposed and, if applicable, accepted
 * @param rate : Force reject with rate < 0 or accept with rate = 2.
 * @ref https://fr.wikipedia.org/wiki/Algorithme_de_Metropolis-Hastings
 * @return
 */
    inline bool try_update(const double x, const double rate)
    {
        bool accepted = MHAcceptanceTest(rate);

        // --------------------------------------------------------------
        //  1️⃣  Mise à jour de la valeur si accepted
        // --------------------------------------------------------------
        if (accepted) {
            mX = x;
        }
        // --------------------------------------------------------------
        //  2️⃣  Historique
        // --------------------------------------------------------------
        if (mSamplerProposal == SamplerProposal::eRWAdaptGauss) {
            if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
                // La fenêtre est pleine → on enlève l'élément le plus ancien
                mLastMHAccepts.pop_front();
            }

            mLastMHAccepts.push_back(accepted);
        }

        return accepted;

    }
    /** -----------------------------------------------------------------
*  Implémentation « log‑rate »
*  Identique à test_update_log, mais ici on ne donne pas la valeur courante
* -----------------------------------------------------------------
*/
    inline bool try_update_log(const double x, const double log_rate)
    {

        bool accepted = false;
        // ------------------------------------------------------------------
        // 2️⃣  Cas pathologiques (NaN, -inf, etc.)
        // ------------------------------------------------------------------
        if (std::isnan(log_rate) || log_rate == -INFINITY) {
            // log_rate = -inf ↔ rate = 0  → rejet systématique
            accepted = false;
#ifdef DEBUG
            /* if (std::isnan(log_rate))
            std::cerr << "[MHVariable::try_update_log] log_rate = NaN -> reject : " << mName << '\n';
        else {
            std::cerr << "[MHVariable::try_update_log] log_rate = -inf -> reject : " << mName << '\n';
        }*/
#endif
        }
        // ------------------------------------------------------------------
        // 3️⃣  Acceptation forcée (rate ≥ 1 ↔ log_rate ≥ 0)
        // ------------------------------------------------------------------
        else if (log_rate >= 0.0) {
            // cela couvre le cas spécial rate==2 (log(2) ≈ 0.693) ainsi que tout
            // r > 1.  Le comportement « force accept » est donc conservé.
            accepted = true;
        }
        // ------------------------------------------------------------------
        // 4️⃣  Acceptation probabiliste (0 < rate < 1 ↔ log_rate < 0)
        // ------------------------------------------------------------------
        else {
            // u ~ Uniform(0,1)  →  log(u) ∈ (‑∞,0)
            const double u = Generator::randomUniform();          // 0 < u < 1
            const double log_u = std::log(u);       // toujours négatif
            accepted = (log_u < log_rate);          // équivalent à u < exp(log_rate)
#ifdef DEBUG
            if (u == 0.0)
                std::cerr << "[" << __func__ << "] uniform == 0\n";
#endif
        }
        // ------------------------------------------------------------------
        // 5️⃣  Mise à jour de l’état et de l’historique
        // ------------------------------------------------------------------
        if (accepted) mX = x;
        if (mSamplerProposal == SamplerProposal::eRWAdaptGauss) {
            // ------------------------------------------------------------------
            // 1️⃣  Gestion de l’historique pour le taux d'acceptation
            // ------------------------------------------------------------------
            if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
                // La fenêtre est pleine → on enlève l'élément le plus ancien
                mLastMHAccepts.pop_front();
            }
            mLastMHAccepts.push_back(accepted);
        }

        return accepted;
    }

    /**
 * @brief MHVariable::test_update determines whether to accept a new value for mX based on a given acceptance rate.
 *
 * This function implements a Metropolis-Hastings acceptance criterion. It compares the rate
 * (typically, the ratio of the target distribution at try_value to current_value) against
 * a uniformly distributed random number to decide whether to accept the new value.
 *
 * - If the rate is 1.0 or higher, the new value (try_value) is unconditionally accepted.
 * - If the rate is within [0.0, 1.0), a random number is generated and compared to the rate to decide acceptance.
 * - If the rate is less than 0.0, the new value is rejected outright.
 * - The function maintains a history of the last few acceptance/rejection outcomes in mLastMHAccepts.
 *
 * @param current_value The current value of the variable.
 * @param try_value The proposed new value to be tested.
 * @param rate The acceptance rate, typically the ratio pi(try_value)/pi(current_value).
 * @return bool True if the new value is accepted, false otherwise.
 */
    inline bool test_update(const double current_value, const double try_value, const double rate)
    {

        bool accepted = MHAcceptanceTest(rate);
        // --------------------------------------------------------------
        //  1️⃣  Mise à jour de la valeur
        // --------------------------------------------------------------
        mX = accepted ? try_value : current_value;

        // --------------------------------------------------------------
        //  2️⃣  Historique
        // --------------------------------------------------------------
        if (mSamplerProposal == SamplerProposal::eRWAdaptGauss) {
            if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
                // La fenêtre est pleine → on enlève l'élément le plus ancien
                mLastMHAccepts.pop_front();
            }

            mLastMHAccepts.push_back(accepted);
        }

        return accepted;
    }
    /*======================================================================
 *  Implémentation principale – travaille en log‑espace
 *  Identique à try_update_log, mais ici on donne la valeur courante
 *====================================================================*/

    inline bool test_update_log(double current_value,
                                     double try_value,
                                     double log_rate)
    {
        bool accepted = MHAcceptanceTest_log(log_rate);
        // --------------------------------------------------------------
        //  1️⃣  Mise à jour de la valeur
        // --------------------------------------------------------------
        mX = accepted ? try_value : current_value;

        // --------------------------------------------------------------
        //  2️⃣  Historique
        // --------------------------------------------------------------
        if (mSamplerProposal == SamplerProposal::eRWAdaptGauss) {
            if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
                // La fenêtre est pleine → on enlève l'élément le plus ancien
                mLastMHAccepts.pop_front();
            }

            mLastMHAccepts.push_back(accepted);
        }

        return accepted;
    }
    /**
     * @brief MHVariable::accept_update force setting mX with the value of x.
     * And append a true value to mLastAccept
     * @param x
     */
    inline void accept_update(const double x)
    {
        // --------------------------------------------------------------
        //  1️⃣  Mise à jour de la valeur
        // --------------------------------------------------------------
        mX = x;

        // --------------------------------------------------------------
        //  2️⃣  Historique
        // --------------------------------------------------------------
        if (mSamplerProposal == SamplerProposal::eRWAdaptGauss) {
            if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
                // La fenêtre est pleine → on enlève l'élément le plus ancien
                mLastMHAccepts.pop_front();
            }

            mLastMHAccepts.push_back(true);
        }

    }

    /**
     * @brief MHVariable::reject_update no update of mX, but append a false value to mLastAccept
     */
    inline void reject_update()
    {
        // --------------------------------------------------------------
        //  1️⃣ Historique
        // --------------------------------------------------------------
        if (mSamplerProposal == SamplerProposal::eRWAdaptGauss) {
            if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
                // La fenêtre est pleine → on enlève l'élément le plus ancien
                mLastMHAccepts.pop_front();
            }

            mLastMHAccepts.push_back(false);
        }

    }

    bool adapt(double coef_min = 0.42, const double coef_max = 0.46,
                           size_t batchIndex = 100,
                           double sigma_min = 1e-10, double sigma_max = 100000.0,
                           double c = 0.5, double kappa = 0.6, double t0 = 10.0);

    // Nouveau prototype : batchIndex est obligatoire et vient en premier
    bool adapt_Robbins_Monro(size_t batchIndex,
               double targetAcceptRate = 0.44,
               double sigma_min = 1e-10,
               double sigma_max = 100000.0,
               double c = 0.5,
               double kappa = 0.6,
               double t0 = 10.0);

    inline bool acceptMH_buffer_full()
    {
        return mLastMHAccepts.size() == mLastMHAcceptsLength;
    }

    inline void count_MH_accepted () {
        if (!mLastMHAccepts.empty()) {
            if (mLastMHAccepts.back()) {
                ++mMHAcceptcountSinceAquire;
            }
        }
    }


    inline virtual void acquire() override
    {
        mAllAcquiredTrace->push_back(mX);
        count_MH_accepted();
    }

    inline virtual void acquire(double* valueToSave) override
    {
        mAllAcquiredTrace->push_back(*valueToSave);
        count_MH_accepted();
    }

    virtual inline void recordMH()
    {
        mHistoryAcceptRateMH->push_back(getCurrentAcceptRate());
    }

    std::vector<double> acceptationForChain(const std::vector<ChainSpecs>& chains, size_t index);
    void generateGlobalRunAcceptation(const std::vector<ChainSpecs>& chains);

    virtual void generateDensityNumericalResults(const std::vector<ChainSpecs>& chains) override;
    virtual void generateTraceNumericalResults(const std::vector<ChainSpecs>& chains) override;

    QString resultsString(const QString &noResultMessage = QObject::tr("No result to display"),
                          const QString &unit = QString()) const override;

    static QString getSamplerProposalText(const SamplerProposal sp) ;
    static SamplerProposal getSamplerProposalFromText(const QString &text);


    inline void load_stream(QDataStream& stream) {load_stream_v338(stream);};


    void load_stream_v328(QDataStream& stream);
    void load_stream_v327(QDataStream& stream);
    void load_stream_v330(QDataStream& stream);
    void load_stream_v337(QDataStream& stream);

private:
    void load_stream_v338(QDataStream& stream);


};

QDataStream &operator<<( QDataStream &stream, const MHVariable &data );

QDataStream &operator>>( QDataStream &stream, MHVariable &data );



#endif
