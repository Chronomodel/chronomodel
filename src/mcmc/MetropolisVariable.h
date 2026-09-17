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

#ifndef METROPOLISVARIABLE_H
#define METROPOLISVARIABLE_H

//#include "Functions.h"
#include "DateUtils.h"
#include "MCMCSettings.h"

#include <QMap>
#include <QList>
#include <QDataStream>
#include <QObject>
#include <QList>


class TValueStack
{
public :
    TValueStack();
    explicit TValueStack(double value = 0.0, std::string comment = "");

    virtual ~TValueStack();

    inline double value() const {return _value;};
    inline std::string comment() const {return _comment;};

protected :
    double _value;
    std::string _comment ;
};

enum Support
{
    eR = 0, // on R
    eRp = 1, // on R+
    eRm = 2, // on R-
    eRpStar = 3, // on R+*
    eRmStar = 4, // on R-*
    eBounded = 5 // on bounded support
};

enum BandwidthType
{
    eBWUndefine = 0,
    eBWCustom = 1,
    eBWSJ = 2,
    eBWNRD0 = 3
};


typedef double type_data;

// ------------------------------------------------------------------
// Quartiles
// ------------------------------------------------------------------
struct Quartiles
{
    type_data Q1 = static_cast<type_data>(0.0);
    type_data Q2 = static_cast<type_data>(0.0);
    type_data Q3 = static_cast<type_data>(0.0);
};

// ------------------------------------------------------------------
// Statistiques de la fonction
// ------------------------------------------------------------------
struct DensityStat
{
    type_data bandwidth_used = static_cast<type_data>(0.0);
    type_data max      = static_cast<type_data>(0.0);
    type_data mode     = static_cast<type_data>(0.0);
    type_data mean     = static_cast<type_data>(0.0);
    type_data std      = static_cast<type_data>(0.0);
    Quartiles quartiles{};
};

// ------------------------------------------------------------------
// Statistiques de la trace
// ------------------------------------------------------------------
struct TraceStat
{
    bool updated = false;
    type_data min      = static_cast<type_data>(0.0);
    type_data max      = static_cast<type_data>(0.0);
    type_data mean     = static_cast<type_data>(0.0);
    type_data std      = static_cast<type_data>(0.0);
    type_data bw_SJ      = static_cast<type_data>(0.0);
    type_data bw_nrd0      = static_cast<type_data>(0.0);
    Quartiles quartiles{};
};

#pragma mark R_hat & ESS

namespace MCMCDiagnostic
{
// Seuils partagés par TOUS les diagnostics de convergence (calcul de synthèse ET
// affichage UI), pour qu'ils ne puissent plus diverger silencieusement comme entre
// computeConvergenceSummary (1.01/1.05) et l'ancien affichage Rhat (1.01/1.1).
// Alignés sur Vehtari, Gelman, Simpson, Carpenter & Bürkner (2021) pour le Rhat,
// et sur la règle usuelle (Stan/ArviZ/bayesplot) d'au moins ~400 tirages effectifs
// par quantité d'intérêt pour l'ESS.
namespace Threshold
{

constexpr double RhatGood    = 1.01; // en dessous : convergence satisfaisante
constexpr double RhatWarning = 1.05; // en dessous : convergence douteuse ; au-dessus : non convergé

constexpr double EssGood     = 400.; // au-dessus : ESS satisfaisant
constexpr double EssWarning  = 100.; // en dessous : ESS trop faible pour être exploitable

} // namespace Threshold
enum class ConvergenceStatus
{
    eGood,      // Convergence satisfaisante
    eWarning,   // Convergence douteuse, à surveiller
    eBad        // Non convergé
};

struct ConvergenceSummary
{
     double maxRHat = 0.;                       // max(R-hat) sur toutes les variables
     double meanRHat = 0.;                       // moyenne, à titre indicatif seulement (ne doit pas servir à décider)
     size_t nVariables = 0;                       // nombre de variables évaluées
     size_t nAboveGoodThreshold = 0;              // nb de variables avec R-hat >= goodThreshold (ou non finies)
     double fractionAboveGoodThreshold = 0.;      // proportion correspondante

     // --- ESS (nouveau) ---
     bool hasEss = false;                         // ESS fourni et exploitable (essValues non vide)
     double minESS = 0.;                          // min(ESS) sur les variables évaluées
     double meanESS = 0.;                         // moyenne, à titre indicatif seulement (ne doit pas servir à décider)
     size_t nEssVariables = 0;                     // nombre de variables ESS évaluées
     size_t nBelowGoodEssThreshold = 0;            // nb de variables avec ESS < essGoodThreshold (ou non finies)
     double fractionBelowGoodEssThreshold = 0.;    // proportion correspondante

     // --- Sous-statuts (utiles pour un badge séparé par métrique dans l'UI/le log) ---
     ConvergenceStatus rHatStatus = ConvergenceStatus::eGood;
     ConvergenceStatus essStatus  = ConvergenceStatus::eGood;

     ConvergenceStatus status = ConvergenceStatus::eGood;
     QString label;                                // libellé qualitatif prêt à afficher dans l'UI
 };



struct RhatEssResult
{
    double rHat    = 0.;   // max(bulk-Rhat, tail-Rhat)
    double bulkESS = 0.;   // ESS sur les valeurs rang-normalisées (précision moyenne/médiane)
    double tailESS = 0.;   // min(ESS quantile 5 %, ESS quantile 95 %) (précision des queues)
};

/**
 * @brief Calcule Rhat (split, rang-normalisé et replié) ET l'ESS (bulk + tail) d'une
 *        variable échantillonnée en UNE seule passe, en partageant tout ce qui peut
 *        l'être entre les deux diagnostics : split en demi-chaînes, tri des valeurs
 *        regroupées (réutilisé pour la médiane ET les quantiles 5 %/95 %), passe de
 *        rang-normalisation "bulk" (réutilisée pour bulk-Rhat ET bulk-ESS, qui portent
 *        sur exactement les mêmes demi-chaînes transformées), statistiques W/B/var+
 *        par jeu de demi-chaînes (calculées une seule fois et réutilisées à la fois
 *        pour la formule du Rhat et pour la combinaison d'autocorrélation de l'ESS),
 *        et un unique objet Eigen::FFT réutilisé pour les 3 jeux d'autocovariances
 *        (bulk, indicatrice q05, indicatrice q95 — tous de même longueur après split).
 *
 *        Seul le repliement autour de la médiane (spécifique à tail-Rhat) et la
 *        transformation en indicatrices de quantile (spécifique à tail-ESS) restent
 *        des étapes distinctes : ce sont deux diagnostics différents qui ne partagent
 *        que le nom "tail" (Vehtari, Gelman, Simpson, Carpenter & Bürkner, 2021).
 *
 * @warning Reproduction fidèle de l'algorithme de Geyer/Stan pour l'ESS, mais non
 *          validée numériquement contre une référence externe (posterior::ess_bulk/
 *          ess_tail en R, ou Stan). À comparer sur quelques chaînes de test avant
 *          usage en production.
 *
 * @note Suppose vos fonctions averageRanks / invNormalCDF / rankNormalize déjà
 *       écrites. Le Rhat reproduit exactement la formule de votre gelmanRubin()
 *       (variante "coda", avec le terme correctif (M+1)/(M*N) sur B, et les mêmes
 *       gardes pour les chaînes dégénérées : W et B quasi nuls -> 1.0, W quasi nul
 *       seul -> infini, B négligeable devant W -> 1.0). L'ESS, en revanche,
 *       utilise en interne la variance globale "var+" SANS ce terme correctif
 *       (convention Stan/Vehtari standard) : le correctif (M+1)/M de votre
 *       gelmanRubin est spécifique au facteur de réduction d'échelle et n'a pas
 *       d'équivalent justifié dans la dérivation de l'ESS. Les deux var+ sont
 *       calculées séparément à partir des mêmes W/B (donc sans repasser sur les
 *       données), voir le commentaire dans le .cpp.
 *
 * @param chains  M chaînes de même longueur N (N >= 4 requis pour le split).
 */
RhatEssResult computeRhatAndEss(const std::vector<std::vector<double>>& chains);


/**
 * @brief Synthétise un ensemble de R-hat (un par variable échantillonnée) en un
 *        diagnostic de convergence unique, qualitatif.
 *
 * Principe : le pire cas (max R-hat) pilote la décision, pas la moyenne, car une
 * moyenne masque la seule variable qui n'a pas convergé. Un second critère (fraction
 * de variables dépassant goodThreshold) évite qu'un grand nombre de paramètres ne
 * fasse basculer le diagnostic en "Mauvais" à cause d'un simple bruit statistique
 * isolé, ce qui est fréquent dès qu'on a beaucoup de variables (ti, deltaI, sigmaTi...).
 *
 * Seuils par défaut alignés sur Vehtari, Gelman, Simpson, Carpenter, Bürkner (2021)
 * et l'usage courant (Stan), plus stricts que les seuils historiques 1.1/1.2 :
 *
 *   - maxRHat < goodThreshold                                          -> Bon
 *   - maxRHat < warningThreshold ET fraction <= maxFractionAboveGood   -> Limite
 *   - sinon (y compris toute valeur non finie)                        -> Mauvais
 *
 * À utiliser idéalement avec un split-R-hat (voire rank-normalized) plutôt qu'un
 * R-hat classique, et en complément d'un diagnostic d'ESS (bulk/tail), le R-hat
 * seul ne garantissant pas un échantillonnage suffisant des queues.
 *
 * @param rHatValues            R-hat de chaque variable échantillonnée
 * @param goodThreshold         seuil en-dessous duquel une variable est jugée convergée (défaut 1.01)
 * @param warningThreshold      seuil au-delà duquel le diagnostic global passe à "Mauvais" (défaut 1.05)
 * @param maxFractionAboveGood  fraction maximale tolérée de variables >= goodThreshold
 *                              avant de ne plus l'imputer au seul bruit (défaut 1 %)
 */

ConvergenceSummary computeConvergenceSummary(const std::vector<double>& rHatValues,
                                             const std::vector<double>& essValues = {},
                                             double goodThreshold = Threshold::RhatGood,
                                             double warningThreshold = Threshold::RhatWarning,
                                             double maxFractionAboveGood = 0.01,

                                             double essGoodThreshold = Threshold::EssWarning,
                                             double essWarningThreshold = Threshold::EssWarning,
                                             double essMaxFractionBelowGood = -1.,
                                             size_t minVariablesForBad = 4);
} // namespace MCMCDiagnostic


// ------------------------------------------------------------------
// Analyse combinée (fonction + trace)
// ------------------------------------------------------------------
struct PosteriorAnalysis
{
    DensityStat densityAnalysis{};
    TraceStat   traceAnalysis{};
    MCMCDiagnostic::RhatEssResult RhatESS {};
    // constructeur qui met des NaN pour indiquer « non calculé »
    PosteriorAnalysis()
    {
        // ----- fonction -----
        densityAnalysis.max  = std::numeric_limits<type_data>::quiet_NaN();
        densityAnalysis.mode = std::numeric_limits<type_data>::quiet_NaN();
        densityAnalysis.mean = std::numeric_limits<type_data>::quiet_NaN();
        densityAnalysis.std  = std::numeric_limits<type_data>::quiet_NaN();
        // les quartiles restent à 0.0 (ou vous pouvez les mettre à NaN aussi)
        densityAnalysis.quartiles.Q1 = std::numeric_limits<type_data>::quiet_NaN();
        densityAnalysis.quartiles.Q2 = std::numeric_limits<type_data>::quiet_NaN();
        densityAnalysis.quartiles.Q3 = std::numeric_limits<type_data>::quiet_NaN();
        // ----- trace -----
        traceAnalysis.min  = std::numeric_limits<type_data>::quiet_NaN();
        traceAnalysis.max  = std::numeric_limits<type_data>::quiet_NaN();
        traceAnalysis.mean = std::numeric_limits<type_data>::quiet_NaN();
        traceAnalysis.std  = std::numeric_limits<type_data>::quiet_NaN();
        // idem pour les quartiles de la trace
        traceAnalysis.quartiles.Q1 = std::numeric_limits<type_data>::quiet_NaN();
        traceAnalysis.quartiles.Q2 = std::numeric_limits<type_data>::quiet_NaN();
        traceAnalysis.quartiles.Q3 = std::numeric_limits<type_data>::quiet_NaN();
    }
};

class MetropolisVariable
{
private:
    BandwidthType mBandwidthType;
    double mBandwidth;

    std::string mName;
    double mX;


public:

    Support mSupport;
    DateUtils::FormatDate mFormat;

    std::vector<long long> mAcceptedStateCountByChain; //Number of State accepted by chain

    std::shared_ptr<std::vector<double>> mBurnAdaptTrace; // all the trace for all chain in the burnin state and the addapt state, in raw format
    std::shared_ptr<std::vector<double>> mAllAcquiredTrace; // all the trace for all chain in the Aquire state, in raw format
    bool is_curve_filtering = false;
    std::shared_ptr<std::vector<double>> mDisplayAcquiredTrace;

    inline std::shared_ptr<std::vector<double>> traceToDisplay() const {
        if (is_curve_filtering)
            return mDisplayAcquiredTrace;
        else
            return mAllAcquiredTrace;
    }

    std::shared_ptr<std::vector<double>> mFormatedBurnAdaptTrace;
    std::shared_ptr<std::vector<double>> mFormatedAcquiredTrace;


    // If we use std::vector we can not use QDataStream to save,
    // because QDataStream provides support for multi system and takes account of endians




    // Posterior density results.
    // mFormatedKDE is calculated using all run parts of all chains traces.
    // mChainsKDE constains posterior densities for each chain, computed using only the "run" part of the trace.
    // This needs to be re-calculated each time we change fftLength or bandwidth.
    // See generateKDE() for more.
    std::map<double, double> mFormatedKDE;
    std::vector<std::map<double, double>> mChainsKDE;

    // List of correlations for each chain.
    // They are calculated once, when the MCMC is ready, from the run part of the trace.
    std::vector<std::vector<double>> mCorrelations;

    std::map<double, double> mFormatedHPD;
    QList<QPair<double, QPair<double, double>>> mRawHPDintervals;

    std::pair<double, double> mRawCredibility;
    std::pair<double, double> mFormatedCredibility;

    double mExactCredibilityThreshold;

    PosteriorAnalysis mResults;
    std::vector<PosteriorAnalysis> mChainsResults;

    int mfftLenUsed;

    double mThresholdUsed;

    double mtminUsed;
    double mtmaxUsed;


public:
    MetropolisVariable();
    explicit MetropolisVariable(const MetropolisVariable& origin);

    virtual ~MetropolisVariable();
    virtual MetropolisVariable& operator=(const MetropolisVariable& origin);
    virtual MetropolisVariable& operator=(MetropolisVariable&& origin) noexcept;

    virtual void clear();
    virtual void shrink_to_fit() noexcept;
    virtual void clear_and_shrink() noexcept;

    virtual void remove_smoothed_densities();
    virtual void reserve(const size_t reserve);

    void setFormat(const DateUtils::FormatDate fm);

    inline QString getQStringName() const {return QString::fromStdString(mName);}
    inline std::string name() const noexcept {return mName;}

    inline void setName(const std::string name) {mName = name;}
    inline void setName(const QString name) {mName = name.toStdString();}

    inline void setValue(const double v) noexcept {mX= v;}
    inline double value() const noexcept {return mX;}

    inline void setBandwidth(BandwidthType bwt, double h = 1)
    {
        switch (bwt) {
        case BandwidthType::eBWCustom :
            mBandwidth = h;
            break;
        case BandwidthType::eBWSJ :
            mBandwidth = mResults.traceAnalysis.bw_SJ;
            break;
        case BandwidthType::eBWNRD0 :
            mBandwidth = mResults.traceAnalysis.bw_nrd0;
            break;
        default:
            break;
        }
    }

    // -----
    //  These functions are time consuming!
    // -----
    void generateCorrelations(const std::vector<ChainSpecs> &chains);

    void generateFormatedKDE(const std::vector<ChainSpecs> &chains, const int fftLen = 1024, const double tmin = 0., const double tmax = 0.);

    // obsolete
    void memoHistoParameter(const int fftLen = 1024, const double bandwidth = 0.9, const double tmin = 0., const double tmax = 0.);
    bool HistoWithParameter(const int fftLen = 1024, const double bandwidth = 0.9, const double tmin = 0., const double tmax = 0.);

    void generateHPD(const double threshold = 95.0);
    void generateCredibility(const double threshold = 95.0);


    // Virtual because MHVariable subclass adds some information
    virtual void generateDensityNumericalResults(const std::vector<ChainSpecs>& chains);
    virtual void generateTraceNumericalResults(const std::vector<ChainSpecs>& chains);

    void updateFormatedCredibility(const DateUtils::FormatDate fm);

    std::map<double, double> generateKDE(const std::vector<double> &dataSrc, const int fftLen, const double tmin, const double tmax);

    // -----
    // These functions do not make any calculation
    // -----
    std::map<double, double> &fullHisto();
    std::map<double, double> &KDEForChain(const size_t index);

    // Full trace for the chain (burn + adapt + run)
    std::vector<double> fullFormatedTraceForChain( const std::vector<ChainSpecs>& chains, std::size_t index) const noexcept;


    // Trace for run part as a vector
    template <template<typename...> class C, typename T>
    C<T> full_run_trace(C<T>* trace, const std::vector<ChainSpecs>& chains)
    {
        if (trace == nullptr || trace->size() == 0)
            return C<T>(0);

        else if (trace->size() == chains.size()) // Cas des variables fixes
            return C<T>(*trace);

        // Calcul reserve space
        int reserveSize = 0;

        for (const ChainSpecs& chain : chains)
            reserveSize += chain.mRealyAccepted;

        C<T> result(reserveSize);

        int shift = 0;
        int shiftTrace = 0;

        for (const ChainSpecs& chain : chains) {
            // we add 1 for the init
            const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
            const int runTraceSize = chain.mRealyAccepted;
            const int firstRunPosition = shift + burnAdaptSize;
            std::copy(trace->begin() + firstRunPosition , trace->begin() + firstRunPosition + runTraceSize , result.begin() + shiftTrace);

            shiftTrace += runTraceSize;
            shift = firstRunPosition +runTraceSize;
        }
        return result;
    }


    template <typename T>
    std::vector<T> full_run_trace(std::shared_ptr<std::vector<T>> trace, const std::vector<ChainSpecs>& chains)
    {
        if (trace == nullptr || trace->size() == 0)
            return std::vector<T>(0);

        else if (trace->size() == chains.size()) // Cas des variables fixes
            return std::vector<T>(trace->begin(), trace->end());

        // Calcul reserve space
        int reserveSize = 0;

        for (const ChainSpecs& chain : chains)
            reserveSize += chain.mRealyAccepted;

        std::vector<T> result(reserveSize);

        int shift = 0;
        int shiftTrace = 0;

        for (const ChainSpecs& chain : chains) {
            // we add 1 for the init
            const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
            const int runTraceSize = chain.mRealyAccepted;
            const int firstRunPosition = shift + burnAdaptSize;
            std::copy(trace->begin() + firstRunPosition , trace->begin() + firstRunPosition + runTraceSize , result.begin() + shiftTrace);

            shiftTrace += runTraceSize;
            shift = firstRunPosition +runTraceSize;
        }
        return result;
    }

    //inline std::vector<double> fullRunFormatedTrace(const std::vector<ChainSpecs>& chains) {return full_run_trace(mFormatedBurnAdaptTrace, chains);}
    //inline std::vector<double> fullRunRawTrace(const std::vector<ChainSpecs>& chains) {return full_run_trace(mBurnAdaptTrace, chains);}




    inline std::vector<double> extractTraceForChain(
        const std::shared_ptr<std::vector<double>>& trace_ptr,              // pas de shared_ptr
        const std::vector<ChainSpecs>& chains,
        std::size_t chain_index) noexcept
    {
        // -------------------------------------------------------------
        // Cas trivials
        // -------------------------------------------------------------
        if (!trace_ptr || trace_ptr->empty())
            return {0};

        const std::vector<double>& trace = *trace_ptr;
        // -------------------------------------------------------------
        // 1️⃣  Cas des variables fixes (une valeur par chaîne)
        // -------------------------------------------------------------
        if (trace.size() == chains.size())
            return trace;

        // -------------------------------------------------------------
        // 3️⃣  Calcul du décalage (shift) – somme pré‑fixe jusqu’à chain_index
        // -------------------------------------------------------------
        // On ne parcourt que les éléments précédents, pas tout le tableau.
        std::size_t shift = 0;
        for (std::size_t i = 0; i < chain_index; ++i)
            //shift += chains[i].mRealyAccepted;
            shift += chains[i].mIterDisplay;
        // -------------------------------------------------------------
        // 4️⃣  Nombre d’échantillons à extraire pour la chaîne demandée
        // -------------------------------------------------------------
        //const std::size_t nbValue = chains[chain_index].mRealyAccepted;
        const std::size_t nbValue = chains[chain_index].mIterDisplay;
        // Protection contre les incohérences d’index (débordement)
        if (shift + nbValue > trace.size())
            return {0};                     // ou lancer une exception
        // -------------------------------------------------------------
        // 5️⃣  Construction directe du sous‑vecteur (une seule passe)
        // -------------------------------------------------------------
        // std::vector possède un constructeur qui accepte deux itérateurs.
        // Cela crée le vecteur et copie les éléments en une seule opération.
        return std::vector<double>(trace.begin() + shift,
                                   trace.begin() + shift + nbValue);
    }

    // used by generateCorrelation
    inline std::vector<double> acquiredTraceforChain(const std::vector<ChainSpecs>& chains, std::size_t chain_index)
    {
        return extractTraceForChain(mAllAcquiredTrace, chains, chain_index);
    }
    // use by generatePosteriorDensities et generateKDE
    inline std::vector<double> formatedAcquiredTraceforChain(const std::vector<ChainSpecs>& chains, std::size_t chain_index)
    {
        return extractTraceForChain(mFormatedAcquiredTrace, chains, chain_index);
    }

    // useless
    std::vector<double>::iterator findIter_element(const long unsigned iter, const std::vector<ChainSpecs>& chains, const size_t index ) const;

    // Trace for run part of the chain as a vector

    template <template<typename...> class C, typename T>
    C<T> run_trace_for_chain(C<T>* trace, const std::vector<ChainSpecs>& chains, const size_t index) {

        if (!trace || trace->size() == 0) {
            return C<T>(0);

        } else if (trace->size() == chains.size()) { // Cas des variables fixes
            return C<T>(trace->at(index));
            //return C<T>(*trace);

        } else  {

            int shift = 0;
            for (size_t i = 0; i<chains.size(); ++i)  {
                const ChainSpecs& chain = chains.at(i);
                // We add 1 for the init
                const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
                const int traceSize = chain.mRealyAccepted;

                if (i == index) {
                    return C<T> (trace->begin() + shift + burnAdaptSize, trace->begin() + shift + burnAdaptSize + traceSize );
                    break;
                }
                shift += traceSize + burnAdaptSize ;
            }
            return C<T>(0);
        }
    }
    template <template<typename...> class C, typename T>
    C<T> run_trace_for_chain(std::shared_ptr<C<T>> trace, const std::vector<ChainSpecs>& chains, const size_t index) {

        if (!trace || trace->size() == 0) {
            return C<T>(0);

        } else if (trace->size() == chains.size()) { // Cas des variables fixes
            return C<T> (trace->begin() + index, trace->begin() + index + 1 );


        } else  {

            int shift = 0;
            for (size_t i = 0; i<chains.size(); ++i)  {
                const ChainSpecs& chain = chains.at(i);
                // We add 1 for the init
                const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
                const int traceSize = chain.mRealyAccepted;

                if (i == index) {
                    return C<T> (trace->begin() + shift + burnAdaptSize, trace->begin() + shift + burnAdaptSize + traceSize );
                    break;
                }
                shift += traceSize + burnAdaptSize ;
            }
            return C<T>(0);
        }
    }

    // Obsolete
    inline std::vector<double> runRawTraceForChain(const std::vector<ChainSpecs>& chains, const size_t index) {
        const std::vector<double> &trace = extractTraceForChain(mAllAcquiredTrace, chains, index);
        return std::vector<double>(trace.begin(), trace.end());
    };


    inline std::vector<double> runFormatedTraceForChain(const std::vector<ChainSpecs>& chains, const size_t index) {
        const std::vector<double> &trace = extractTraceForChain(mFormatedAcquiredTrace, chains, index);
        return std::vector<double>(trace.begin(), trace.end());
    };

    std::vector<double> correlationForChain(const size_t index);

    virtual QString resultsString(const QString& noResultMessage = QObject::tr("No result to display"),
                                  const QString& unit = QString()) const;

    QStringList getResultsList(const QLocale locale, const int precision = 0, const bool withDateFormat = true) const;

    void updateFormatedTrace(const DateUtils::FormatDate fm);

    // Je sauvegarde le nombre d'état accepté pour la chaine, indépendament du thinning,
    // indépendament du nombre d'états retenus dans l'historique qui dépend du thinning.
    inline void memoNbAcceptedState(const unsigned i_chain) { ++mAcceptedStateCountByChain[i_chain];}

    // On mémorise tout le processus d'appentissage Burn et Adaptation
    inline void recordBurnAdapt() {mBurnAdaptTrace->push_back(mX);}
    inline void recordBurnAdapt(double* valueToSave) { mBurnAdaptTrace->push_back(*valueToSave); }

    // mémorisation des états retenus dans l'historique qui dépend du thinning.
    inline virtual void acquire() {
        mAllAcquiredTrace->push_back(mX);
    }
    inline virtual void acquire(double* valueToSave) {
        mAllAcquiredTrace->push_back(*valueToSave);
    }

    inline void load_stream(QDataStream& stream) {load_stream_v338(stream);}
    inline void save_stream(QDataStream& stream) const {save_stream_v338(stream);}

    void load_stream_v328(QDataStream& stream);
    void load_stream_v330(QDataStream& stream);
    void load_stream_v337(QDataStream& stream);
    void load_stream_v338(QDataStream& stream);

private:
    /**
     * @brief Fills a binning buffer using linear binning — O(n).
 *
 * @details
 * Each observation is distributed between its two neighbouring grid points
 * proportionally to its distance (linear binning). The buffer is normalized
 * so that its sum equals 1/delta (i.e., the FFT will produce a proper density).
 *
 * The grid uses a **semi-open** interval [a, b[ with step delta = (b-a)/numPts,
 * consistent with the periodicity assumption of the FFT.
 *
 * @param[out] input    Pre-allocated buffer of size numPts (fftw_malloc).
 * @param[in]  dataSrc  Input data sample (trace).
 * @param[in]  numPts   FFT grid size (power of 2).
 * @param[in]  a        Left bound of the grid (= min(data) - 4h).
 * @param[in]  b        Right bound of the grid (= max(data) + 4h).
 */
    void generateBufferForHisto(double* input, const std::vector<double> &dataSrc, const int numPts, const double a, const double b);

    QMap<double, double> bufferToMap(const double* buffer);

    void save_stream_v330(QDataStream& stream) const;
    void save_stream_v337(QDataStream& stream) const;
    void save_stream_v338(QDataStream& stream) const;

    friend class MHVariable;
    friend class Phase;

};

QDataStream &operator<<( QDataStream& stream, const MetropolisVariable& data );

QDataStream &operator>>( QDataStream& stream, MetropolisVariable& data );








#endif
