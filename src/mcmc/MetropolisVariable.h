/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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

#include "Functions.h"
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

class MetropolisVariable
{
public:
    enum Support
    {
        eR = 0, // on R
        eRp = 1, // on R+
        eRm = 2, // on R-
        eRpStar = 3, // on R+*
        eRmStar = 4, // on R-*
        eBounded = 5 // on bounded support
    };

    std::string mName;

    std::vector<long long> mAcceptedStateCountByChain; //Number of State accepted by chain

    double mX;
    std::shared_ptr<std::vector<double>> mBurnAdaptTrace; // all the trace for all chain in the burnin state and the addapt state, in raw format
    std::shared_ptr<std::vector<double>> mAcquiredTrace; // all the trace for all chain in the Aquire state, in raw format

    std::shared_ptr<std::vector<double>> mFormatedBurnAdaptTrace;
    std::shared_ptr<std::vector<double>> mFormatedAcquiredTrace;



    // If we use std::vector we can not use QDataStream to save,
    // because QDataStream provides support for multi system and takes account of endians
    Support mSupport;
    DateUtils::FormatDate mFormat;

    // Posterior density results.
    // mFormatedHisto is calculated using all run parts of all chains traces.
    // mChainsHistos constains posterior densities for each chain, computed using only the "run" part of the trace.
    // This needs to be re-calculated each time we change fftLength or bandwidth.
    // See generateHistos() for more.
    std::map<double, double> mFormatedHisto;
    std::vector<std::map<double, double>> mChainsHistos;

    // List of correlations for each chain.
    // They are calculated once, when the MCMC is ready, from the run part of the trace.
    std::vector<std::vector<double>> mCorrelations;

    std::map<double, double> mFormatedHPD;
    QList<QPair<double, QPair<double, double>>> mRawHPDintervals;

    std::pair<double, double> mRawCredibility;
    std::pair<double, double> mFormatedCredibility;

    double mExactCredibilityThreshold;

    DensityAnalysis mResults;
    std::vector<DensityAnalysis> mChainsResults;

    int mfftLenUsed;
    double mBandwidthUsed;
    double mThresholdUsed;

    double mtminUsed;
    double mtmaxUsed;

#pragma mark Learning
public:

    // Vecteur brut des valeurs enregistrées pendant le burn-in (par chaîne)
    std::shared_ptr<std::vector<double>> mBurnInPriorTrace;

    // Stockage sous forme de vecteurs pour l'interpolation
    std::vector<double> mPriorX;   // abscisses (grille régulière issue du KDE)
    std::vector<double> mPriorY;   // densités

    // A priori empirique : densité KDE issue du burn-in
    std::map<double, double> mEmpiricalPrior;

    // Indique si l'a priori empirique est prêt à être utilisé
    bool mEmpiricalPriorReady = false;

    // CDF empirique précalculée (construite une fois depuis mEmpiricalPrior)
    std::vector<double> mPriorCDF_x;    // abscisses (identiques à mPriorX)
    std::vector<double> mPriorCDF_y;    // valeurs CDF ∈ [0, 1]
    bool mEmpiricalCDFReady = false;
    void buildEmpiricalCDF();
    /**
 * @brief Draws one sample from the empirical prior truncated to [min, max].
 *
 * @details
 * Uses truncated CDF inversion — no rejection sampling needed:
 * @f[
 *   u \sim \mathcal{U}[0,1], \quad
 *   u' = F(\text{min}) + u \cdot [F(\text{max}) - F(\text{min})], \quad
 *   x  = F^{-1}(u')
 * @f]
 * This guarantees the result is strictly in [min, max] in O(log n).
 *
 * @param min  Lower bound of the truncation interval.
 * @param max  Upper bound of the truncation interval.
 * @return     A sample in [min, max] drawn from the truncated empirical prior,
 *             or the midpoint (min+max)/2 if the CDF has no mass in [min, max].
 */
    double sampleFromEmpiricalPrior(double min, double max) const;



    // Enregistrement pendant le burn-in (à appeler dans recordBurnAdapt)
    inline void recordForPrior() {
        mBurnInPriorTrace->push_back(mX);
    }

    // Construction de l'a priori à partir de la trace burn-in
    void buildEmpiricalPrior(const int fftLen = 1024,
                             const double bandwidth = 0.9,
                             const double tmin = 0.,
                             const double tmax = 0.);


    // Évaluation de l'a priori en un point x (interpolation linéaire)
    double evalEmpiricalPrior(const double x) const;


public:
    MetropolisVariable();
    explicit MetropolisVariable(const MetropolisVariable& origin);

    virtual ~MetropolisVariable();
    virtual MetropolisVariable& operator=(const MetropolisVariable& origin);
    virtual MetropolisVariable& operator=(MetropolisVariable&& origin) noexcept;

    //virtual void memo();
    //virtual void memo(double* valueToSave);
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

    // -----
    //  These functions are time consuming!
    // -----
    void generateCorrelations(const std::vector<ChainSpecs> &chains);

    void generateHistos(const std::vector<ChainSpecs> &chains, const int fftLen = 1024, const double bandwidth = 0.9, const double tmin = 0., const double tmax = 0.);
    void memoHistoParameter(const int fftLen = 1024, const double bandwidth = 0.9, const double tmin = 0., const double tmax = 0.);
    bool HistoWithParameter(const int fftLen = 1024, const double bandwidth = 0.9, const double tmin = 0., const double tmax = 0.);

    void generateHPD(const double threshold = 95);
    void generateCredibility(const std::vector<ChainSpecs> &chains, double threshold = 95.);


    // Virtual because MHVariable subclass adds some information
    virtual void generateDensityNumericalResults(const std::vector<ChainSpecs>& chains);
    virtual void generateTraceNumericalResults(const std::vector<ChainSpecs>& chains);

    void updateFormatedCredibility(const DateUtils::FormatDate fm);


   // QMap<double, double> generateKDE(const QList<double>& data, const int fftLen, const  double bandwidth, const double tmin = 0., const double tmax = 0.);
    std::map<double, double> generateKDE(const std::vector<double> &dataSrc, const int fftLen, const double coef_bandwidth, const double tmin, const double tmax);

    // -----
    // These functions do not make any calculation
    // -----
    std::map<double, double> &fullHisto();
    std::map<double, double> &histoForChain(const size_t index);

    // Full trace for the chain (burn + adapt + run)
    std::vector<double> fullTraceForChain( const std::vector<ChainSpecs>& chains, std::size_t index) const noexcept;


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
            shift += chains[i].mRealyAccepted;
        // -------------------------------------------------------------
        // 4️⃣  Nombre d’échantillons à extraire pour la chaîne demandée
        // -------------------------------------------------------------
        const std::size_t nbValue = chains[chain_index].mRealyAccepted;
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

    inline std::vector<double> acquiredTraceforChain(const std::vector<ChainSpecs>& chains, std::size_t chain_index)
    {
        return extractTraceForChain(mAcquiredTrace, chains, chain_index);
    }

    inline std::vector<double> formatedAcquiredTraceforChain(const std::vector<ChainSpecs>& chains, std::size_t chain_index)
    {
        return extractTraceForChain(mFormatedAcquiredTrace, chains, chain_index);
    }

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
        const std::vector<double> &trace = extractTraceForChain(mAcquiredTrace, chains, index);
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
        mAcquiredTrace->push_back(mX);
    }
    inline virtual void acquire(double* valueToSave) {
        mAcquiredTrace->push_back(*valueToSave);
    }

    inline void load_stream(QDataStream& stream) {load_stream_v337(stream);}
    inline void save_stream(QDataStream& stream) const {save_stream_v337(stream);}

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

    void load_stream_v328(QDataStream& stream);
    void load_stream_v330(QDataStream& stream);
    void load_stream_v337(QDataStream& stream);
    //void load_stream_v327(QDataStream& stream);


friend class MHVariable;
};

QDataStream &operator<<( QDataStream& stream, const MetropolisVariable& data );

QDataStream &operator>>( QDataStream& stream, MetropolisVariable& data );
#endif
