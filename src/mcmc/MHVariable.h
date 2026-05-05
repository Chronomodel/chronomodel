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

class MHVariable: public MetropolisVariable
{
public:
    enum SamplerProposal {
        // Event
        eNone = -2, // use with S02VG
        eFixe = -1,  //<  use with Type==eBound
        eDoubleExp = 0, //<  The default method for Event->theta
        eBoxMuller = 1,
        eMHAdaptGauss = 2, // also for data
        // Data
        eMHPrior = 3,
        eInversion = 4,
        //eMHSymGaussAdapt = 5
    };

    double mSigmaMH;

    // Buffer glissant de la taille d'un batch pour calculer la courbe d'évolution
    // du taux d'acceptation chaine par chaine

    //std::vector<bool> mLastMHAccepts;
    //decltype(mLastMHAccepts.size()) mLastMHAcceptsLength;
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
    //MHVariable& copy(MHVariable const& origin);
    MHVariable& operator=(const MHVariable& origin);

    double getCurrentAcceptRate() const;
    void saveCurrentAcceptRate();

    bool try_update(const double x, const double rate);
    bool try_update_log(const double x, const double log_rate);
    bool test_update(const double current_value, const double try_value, const double rate);
    bool test_update_log(double current_value, double try_value, double log_rate);
    void accept_update(const double x);
    void reject_update();

    //bool adapt (const double coef_min = 0.42, const double coef_max = 0.46, const double delta = 0.01);

   // bool adapt(const double coef_min = 0.42, const double coef_max = 0.46, double delta = 0.01, double sigma_min = 1e-4, double sigma_max = 10.0);


    bool adapt(double coef_min = 0.42, const double coef_max = 0.46,
                           size_t batchIndex = 100,
                           double sigma_min = 1e-4, double sigma_max = 10.0,
                           double c = 0.5, double kappa = 0.6, double t0 = 10.0);

    inline bool acceptMH_buffer_full() {return mLastMHAccepts.size() == mLastMHAcceptsLength;};

    inline void count_MH_accepted () {
        if (!mLastMHAccepts.empty()) {
            if (mLastMHAccepts.back()) {
                ++mMHAcceptcountSinceAquire;
            }
        }
    }


    inline virtual void acquire() override {
        mAcquiredTrace->push_back(mX);
        count_MH_accepted();
    }
    inline virtual void acquire(double* valueToSave) override {
        mAcquiredTrace->push_back(*valueToSave);
        count_MH_accepted();
    }

    virtual inline void recordMH()
    {
        mHistoryAcceptRateMH->push_back(getCurrentAcceptRate());
    }

    std::vector<double> acceptationForChain(const std::vector<ChainSpecs>& chains, size_t index);
    void generateGlobalRunAcceptation(const std::vector<ChainSpecs>& chains);

    //void generateNumericalResults(const std::vector<ChainSpecs> &chains) override;

    virtual void generateDensityNumericalResults(const std::vector<ChainSpecs>& chains) override;
    virtual void generateTraceNumericalResults(const std::vector<ChainSpecs>& chains) override;

    QString resultsString(const QString &noResultMessage = QObject::tr("No result to display"),
                          const QString &unit = QString()) const override;

    static QString getSamplerProposalText(const MHVariable::SamplerProposal sp);
    static MHVariable::SamplerProposal getSamplerProposalFromText(const QString &text);


    inline void load_stream(QDataStream& stream) {load_stream_v337(stream);};

private:
    void load_stream_v328(QDataStream& stream);
    void load_stream_v327(QDataStream& stream);
    void load_stream_v330(QDataStream& stream);
    void load_stream_v337(QDataStream& stream);


};

QDataStream &operator<<( QDataStream &stream, const MHVariable &data );

QDataStream &operator>>( QDataStream &stream, MHVariable &data );

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
                                                           qDebug() << "[MHAcceptanceTest] uniform == 0";
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
            std::cerr << "[MHAcceptanceTest_log] uniform == 0\n";
#endif
    }
    return accepted;
}

#endif
