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

#include "MetropolisVariable.h"
//#include "Generator.h" // pour test ucv

#include "StdUtilities.h"
#include "QtUtilities.h"
#include "Functions.h"
#include "DateUtils.h"
#include "FFTWThread.h"
//#include "fftw3.h"


#include <QDebug>
#include <algorithm>

TValueStack::TValueStack():
    _value(0.),
    _comment("comment")
{
};

TValueStack::TValueStack(double value, std::string comment):
    _value(value),
    _comment(comment)
{
};

TValueStack::~TValueStack()
{
};




/** Default constructor */
MetropolisVariable::MetropolisVariable():
    mBandwidth (-1.0),
    mName("Empty MetropolisVariable"),
    mX (0.0),
    mSupport (eR),
    mFormat (DateUtils::eNumeric),
    mAcceptedStateCountByChain(),

    mBurnAdaptTrace(std::make_shared<std::vector<double>>()),
    mAllAcquiredTrace(std::make_shared<std::vector<double>>()),
    is_curve_filtering(false),
    mDisplayAcquiredTrace(std::make_shared<std::vector<double>>()),
    mFormatedBurnAdaptTrace(std::make_shared<std::vector<double>>()),
    mFormatedAcquiredTrace(std::make_shared<std::vector<double>>()),

    mFormatedKDE(),
    mChainsKDE(),
    mCorrelations(),
    mFormatedHPD(),
    mRawHPDintervals(),
    mExactCredibilityThreshold (0.0),
    mResults(),
    mChainsResults(),
    mfftLenUsed (-1),
    mThresholdUsed (-1.0),
    mtminUsed (0.0),
    mtmaxUsed (0.0)
{
   mRawCredibility = std::pair<double, double>(1, -1);
   mFormatedCredibility = std::pair<double, double>(1, -1);

}

/** Copy constructor */
MetropolisVariable::MetropolisVariable(const MetropolisVariable& origin):
    MetropolisVariable()
{
    mX = origin.mX;
    mName = origin.mName;
    mAcceptedStateCountByChain = origin.mAcceptedStateCountByChain;

    mBurnAdaptTrace = std::make_shared<std::vector<double>>(*origin.mBurnAdaptTrace);
    mFormatedBurnAdaptTrace = std::make_shared<std::vector<double>>(*origin.mFormatedBurnAdaptTrace);

    mAllAcquiredTrace = std::make_shared<std::vector<double>>(*origin.mAllAcquiredTrace);
    mDisplayAcquiredTrace = std::make_shared<std::vector<double>>(*origin.mDisplayAcquiredTrace);
    mFormatedAcquiredTrace = std::make_shared<std::vector<double>>(*origin.mFormatedAcquiredTrace);
    is_curve_filtering = origin.is_curve_filtering;

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mFormatedKDE = origin.mFormatedKDE;
    mChainsKDE = origin.mChainsKDE;

    mCorrelations = origin.mCorrelations;

    mFormatedHPD = origin.mFormatedHPD;
    mRawCredibility = origin.mRawCredibility;
    mRawHPDintervals = origin.mRawHPDintervals;
    mFormatedCredibility = origin.mFormatedCredibility;

    mExactCredibilityThreshold = origin.mExactCredibilityThreshold;

    mResults = origin.mResults;
    mChainsResults = origin.mChainsResults;

    mfftLenUsed = origin.mfftLenUsed;
    mBandwidth = origin.mBandwidth;
    mThresholdUsed = origin.mThresholdUsed;

    mtminUsed = origin.mtminUsed;
    mtmaxUsed = origin.mtmaxUsed;

}


/** Destructor */
MetropolisVariable::~MetropolisVariable()
{
}

/** Copy assignment operator */
MetropolisVariable& MetropolisVariable::operator=(const MetropolisVariable& origin)
{
    mX = origin.mX;
    mName = origin.mName;
    mAcceptedStateCountByChain = origin.mAcceptedStateCountByChain;

    mBurnAdaptTrace = std::make_shared<std::vector<double>>(*origin.mBurnAdaptTrace);
    mFormatedBurnAdaptTrace = std::make_shared<std::vector<double>>(*origin.mFormatedBurnAdaptTrace);

    mAllAcquiredTrace = std::make_shared<std::vector<double>>(*origin.mAllAcquiredTrace);
    mDisplayAcquiredTrace = std::make_shared<std::vector<double>>(*origin.mDisplayAcquiredTrace);
    mFormatedAcquiredTrace = std::make_shared<std::vector<double>>(*origin.mFormatedAcquiredTrace);
    is_curve_filtering = origin.is_curve_filtering;

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mFormatedKDE = origin.mFormatedKDE;
    mChainsKDE = origin.mChainsKDE;

    mCorrelations = origin.mCorrelations;

    mFormatedHPD = origin.mFormatedHPD;
    mRawCredibility = origin.mRawCredibility;
    mRawHPDintervals = origin.mRawHPDintervals;
    mFormatedCredibility = origin.mFormatedCredibility;

    mExactCredibilityThreshold = origin.mExactCredibilityThreshold;

    mResults = origin.mResults;
    mChainsResults = origin.mChainsResults;

    mfftLenUsed = origin.mfftLenUsed;
    mBandwidth = origin.mBandwidth;
    mThresholdUsed = origin.mThresholdUsed;

    mtminUsed = origin.mtminUsed;
    mtmaxUsed = origin.mtmaxUsed;

    return *this;
}

/** Move assignment operator */
MetropolisVariable& MetropolisVariable::operator=(MetropolisVariable&& origin) noexcept
{
    if (this != &origin) { // Vérification de l'auto-assignement
        // Transférer les membres
        mX = std::move(origin.mX);
        mName = std::move(origin.mName);
        mAcceptedStateCountByChain = std::move(origin.mAcceptedStateCountByChain);

        mBurnAdaptTrace = std::move(origin.mBurnAdaptTrace);
        mFormatedBurnAdaptTrace = std::move(origin.mFormatedBurnAdaptTrace);

        mAllAcquiredTrace = std::move(origin.mAllAcquiredTrace);
        mDisplayAcquiredTrace = std::move(origin.mDisplayAcquiredTrace);
        mFormatedAcquiredTrace = std::move(origin.mFormatedAcquiredTrace);
        is_curve_filtering = std::move(origin.is_curve_filtering);

        mSupport = std::move(origin.mSupport);
        mFormat = std::move(origin.mFormat);

        mFormatedKDE = std::move(origin.mFormatedKDE);
        mChainsKDE = std::move(origin.mChainsKDE);

        mCorrelations = std::move(origin.mCorrelations);

        mFormatedHPD = std::move(origin.mFormatedHPD);
        mRawCredibility = std::move(origin.mRawCredibility);
        mRawHPDintervals = std::move(origin.mRawHPDintervals);
        mFormatedCredibility = std::move(origin.mFormatedCredibility);

        mExactCredibilityThreshold = std::move(origin.mExactCredibilityThreshold);

        mResults = std::move(origin.mResults);
        mChainsResults = std::move(origin.mChainsResults);

        mfftLenUsed = origin.mfftLenUsed;
        mBandwidth = origin.mBandwidth;
        mThresholdUsed = origin.mThresholdUsed;

        mtminUsed = origin.mtminUsed;
        mtmaxUsed = origin.mtmaxUsed;

        origin.mBurnAdaptTrace.reset();
        origin.mFormatedBurnAdaptTrace.reset();

        origin.mAllAcquiredTrace.reset();
        origin.mDisplayAcquiredTrace.reset();
        origin.mFormatedAcquiredTrace.reset();

    }

    return *this;
}



void MetropolisVariable::clear()
{
    mBurnAdaptTrace->clear();
    mFormatedBurnAdaptTrace->clear();
    if (mAllAcquiredTrace) mAllAcquiredTrace->clear();
    mDisplayAcquiredTrace->clear();
    mFormatedAcquiredTrace->clear();

    mFormatedKDE.clear();
    mAcceptedStateCountByChain.clear();

    mChainsKDE.clear();
    mCorrelations.clear();
    mRawHPDintervals.clear();
    mFormatedHPD.clear();

    mChainsResults.clear();
    mRawCredibility = std::pair<double, double>(1, -1);
    mFormatedCredibility = std::pair<double, double>(1, -1);
    mExactCredibilityThreshold = 0.0;

    is_curve_filtering = false;

}

void MetropolisVariable::shrink_to_fit() noexcept
{
    mBurnAdaptTrace->shrink_to_fit();
    mFormatedBurnAdaptTrace->shrink_to_fit();
    if(mAllAcquiredTrace) mAllAcquiredTrace->shrink_to_fit();
    mDisplayAcquiredTrace->shrink_to_fit();
    mFormatedAcquiredTrace->shrink_to_fit();

    mChainsKDE.shrink_to_fit();
    mCorrelations.shrink_to_fit();
    mRawHPDintervals.shrink_to_fit();
    mChainsResults.shrink_to_fit();

}

void MetropolisVariable::clear_and_shrink() noexcept
{
    mAcceptedStateCountByChain.clear();
    mBurnAdaptTrace->clear();
    mBurnAdaptTrace->shrink_to_fit();

    mFormatedBurnAdaptTrace->clear();
    mFormatedBurnAdaptTrace->shrink_to_fit();

    mAllAcquiredTrace->clear();
    mAllAcquiredTrace->shrink_to_fit();

    mDisplayAcquiredTrace->clear();
    mDisplayAcquiredTrace->shrink_to_fit();

    mFormatedAcquiredTrace->clear();
    mFormatedAcquiredTrace->shrink_to_fit();


    mFormatedKDE.clear();
    mChainsKDE.shrink_to_fit();

    mChainsKDE.clear();
    mChainsKDE.shrink_to_fit();

    mCorrelations.clear();
    mCorrelations.shrink_to_fit();

    mRawHPDintervals.clear();
    mRawHPDintervals.shrink_to_fit();

    mFormatedHPD.clear();

    mChainsResults.clear();
    mChainsResults.shrink_to_fit();

    mRawCredibility = std::pair<double, double>(1, -1);
    mFormatedCredibility = std::pair<double, double>(1, -1);
    mExactCredibilityThreshold = 0.0;

    is_curve_filtering = false;

    // Learning Prior
    /*mBurnInPriorTrace->clear();
    mBurnInPriorTrace->shrink_to_fit();
    mEmpiricalPrior.clear();
    mEmpiricalPriorReady = false;*/

}


void MetropolisVariable::remove_smoothed_densities()
{
    // -----------------------------------------------------------------
    // 1️⃣  Nettoyage des conteneurs qui doivent être vidés complètement
    // -----------------------------------------------------------------
    // mBurnAdaptTrace->clear();          // not a posterior
    mFormatedBurnAdaptTrace->clear();    // OK à vider
    // mAllAcquiredTrace->clear();           // C'est à garder

    mFormatedAcquiredTrace->clear();     // OK à vider
    mFormatedKDE.clear();              // OK à vider
    mChainsKDE.clear();               // OK à vider
    // mCorrelations.clear();            // ne dépend pas de la fftw

    mRawHPDintervals.clear();            // OK à vider
    mFormatedHPD.clear();                // OK à vider

    // -----------------------------------------------------------------
    // 2️⃣  Ré‑initialiser **seulement** la partie DensityStat de chaque
    //     élément de mChainsResults (on garde traceAnalysis)
    // -----------------------------------------------------------------
    for (auto& chainResult : mChainsResults)
    {
        // ----- fonction : on met les champs à NaN pour signifier « non calculé » -----
        chainResult.densityAnalysis.max  = std::numeric_limits<type_data>::quiet_NaN();
        chainResult.densityAnalysis.mode = std::numeric_limits<type_data>::quiet_NaN();
        chainResult.densityAnalysis.mean = std::numeric_limits<type_data>::quiet_NaN();
        chainResult.densityAnalysis.std  = std::numeric_limits<type_data>::quiet_NaN();

        chainResult.densityAnalysis.quartiles.Q1 = std::numeric_limits<type_data>::quiet_NaN();
        chainResult.densityAnalysis.quartiles.Q2 = std::numeric_limits<type_data>::quiet_NaN();
        chainResult.densityAnalysis.quartiles.Q3 = std::numeric_limits<type_data>::quiet_NaN();

        // ----- traceAnalysis : **on ne touche pas** → il reste tel qu’il était
    }

    // -----------------------------------------------------------------
    // 3️⃣  Le reste des membres de la classe restent inchangés
    // -----------------------------------------------------------------
    mRawCredibility               = std::pair<double, double>(1, -1);
    mFormatedCredibility          = std::pair<double, double>(1, -1);
    mExactCredibilityThreshold    = 0.0;

    // Learning Prior
    /*mBurnInPriorTrace->clear();
    mBurnInPriorTrace->shrink_to_fit();
    mEmpiricalPrior.clear();
    mEmpiricalPriorReady = false;*/
}


void MetropolisVariable::reserve(const size_t reserve)
{
    mBurnAdaptTrace->reserve(reserve);
    mFormatedBurnAdaptTrace->reserve(reserve);
    mAcceptedStateCountByChain.reserve(reserve);
}

void MetropolisVariable::setFormat(const DateUtils::FormatDate fm)
{
    if (mBurnAdaptTrace) {
        if (fm != mFormat || mFormatedBurnAdaptTrace->size() != mBurnAdaptTrace->size()) {
            updateFormatedTrace(fm); // fait aussi mAllAcquiredTrace
        }
    }

    updateFormatedCredibility(fm);

    if (mFormat != DateUtils::eNumeric)
        mFormat = fm;
}

/**
 * @brief MetropolisVariable::updateFormatedTrace, it's a slot that transforms or creates mFormatedBurnAdaptTrace
 * according to mFormat.
 */
void MetropolisVariable::updateFormatedTrace(const DateUtils::FormatDate fm)
{
    auto traceDisplay = traceToDisplay();
    if (fm == DateUtils::eNumeric || mFormat == DateUtils::eNumeric) {

        mFormatedBurnAdaptTrace = std::make_shared<std::vector<double>>(*mBurnAdaptTrace);
        mFormatedAcquiredTrace = std::make_shared<std::vector<double>>(*traceDisplay);

    //mFormatedBurnAdaptTrace = mBurnAdaptTrace// it's the same pointer, if you delete mFormatedBurnAdaptTrace, you delete mBurnAdaptTrace. If you change the format you change the value of mBurnAdaptTrace

    } else {
        mFormatedBurnAdaptTrace->resize(mBurnAdaptTrace->size());
        std::transform(mBurnAdaptTrace->cbegin(), mBurnAdaptTrace->cend(), mFormatedBurnAdaptTrace->begin(), [&fm](const double i) {return DateUtils::convertToFormat(i, fm);});

        mFormatedAcquiredTrace->resize(traceDisplay->size());
        std::transform(traceDisplay->cbegin(), traceDisplay->cend(), mFormatedAcquiredTrace->begin(), [&fm](const double i) {return DateUtils::convertToFormat(i, fm);});

    }


}

void MetropolisVariable::updateFormatedCredibility(const DateUtils::FormatDate fm)
{
   if (fm != DateUtils::eNumeric && mFormat != DateUtils::eNumeric) {
        const double t1 = DateUtils::convertToAppSettingsFormat(mRawCredibility.first);
        const double t2 = DateUtils::convertToAppSettingsFormat(mRawCredibility.second);
        if (t1<t2) {
            mFormatedCredibility.first = t1;
            mFormatedCredibility.second = t2;
        } else {
            mFormatedCredibility.first = t2;
            mFormatedCredibility.second = t1;
        }

    } else {
        mFormatedCredibility.first = mRawCredibility.first;
        mFormatedCredibility.second = mRawCredibility.second;
    }
}
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
void MetropolisVariable::generateBufferForHisto(double* input,
                                                const std::vector<double>& dataSrc,
                                                const int    numPts,
                                                const double a,
                                                const double b)
{
    // ✅ Grille semi-ouverte [a, b[ — cohérente avec la FFT
    const double delta = (b - a) / static_cast<double>(numPts);
    const double denum = static_cast<double>(dataSrc.size());
    const double Nm1   = static_cast<double>(numPts - 1);

    std::fill(input, input + numPts, 0.0);

    for (size_t s = 0; s < dataSrc.size(); ++s) {

        // Position sur la grille (non clampée pour l'instant)
        const double idx = (dataSrc[s] - a) / delta;

        // ⚠️ Garde-fou indispensable : std::clamp n'est PAS NaN-safe
        // (ses comparaisons v < lo / hi < v sont toutes deux fausses pour un NaN,
        // donc std::clamp(NaN, ...) renvoie NaN sans le clamper). Un idx non fini
        // ici (échantillon NaN/Inf dans dataSrc, ou delta == 0 avec dataSrc[s] == a)
        // se propage ensuite à travers floor()/min(), jusqu'au static_cast<int> final
        // qui est alors un comportement indéfini : sur x86 (cvttsd2si), il renvoie
        // INT_MIN, utilisé comme indice de input[] -> écriture hors bornes -> SIGSEGV.
        if (!std::isfinite(idx)) {
            continue; // échantillon inexploitable : on l'ignore plutôt que de planter
            // (equal_areas() dans generateKDE renormalise l'aire à 1 ensuite,
            // donc ignorer quelques échantillons ne biaise pas le résultat)
        }

        // Position sur la grille, clampée dans [0, numPts-1]
        const double idx_clamped = std::clamp(idx, 0.0, Nm1);
        const double idx_under   = std::floor(idx_clamped);
        const double idx_upper   = std::min(idx_under + 1.0, Nm1);

        const double frac          = idx_clamped - idx_under;
        const double contrib_under = (1.0 - frac) / denum;
        const double contrib_upper =        frac  / denum;

        const int k0 = static_cast<int>(idx_under);
        const int k1 = static_cast<int>(idx_upper);

        input[k0] += contrib_under;
        if (k1 != k0)              // évite le double ajout sur le dernier bin
            input[k1] += contrib_upper;
    }
}
// le bandwidth est calculé avant par les stats sur la trace

std::map<double, double> MetropolisVariable::generateKDE(const std::vector<double>& dataSrc, const int fftLen, const double tmin, const double tmax)
{
    mfftLenUsed = fftLen;
    mtmaxUsed = tmax;
    mtminUsed = tmin;

    std::map<double, double> result;

    const auto N = dataSrc.size();
    if (N == 0) {
        return result;
    }

    // --- Garde-fou : trace contenant des valeurs non finies (NaN / Inf) ---
    // Un sampler MCMC numériquement instable (ex. RW sur log10(V) qui s'emballe,
    // ou t_i qui diverge) peut occasionnellement injecter des NaN/Inf dans la trace.
    // Sans ce garde-fou, la conversion double -> int utilisée plus bas pour calculer
    // l'indice de bin de l'histogramme FFT (generateBufferForHisto) peut renvoyer
    // INT_MIN (comportement de cvttsd2si sur x86 pour NaN/Inf/valeur hors plage int),
    // ce qui provoque un accès mémoire complètement hors bornes -> SIGSEGV.
    if (!std::isfinite(mResults.traceAnalysis.std) ||
        std::any_of(dataSrc.begin(), dataSrc.end(), [](double v) { return !std::isfinite(v); })) {
        qWarning() << "[MetropolisVariable::generateKDE] valeurs non finies détectées dans la trace (N =" << N
                   << ") - KDE non calculée pour éviter un crash. Vérifier le sampler en amont. pour " << name();
        result.emplace(dataSrc.at(0), 1.0);
        return result;
    }

    if (N == 1 || mResults.traceAnalysis.std == 0.0) {
        result.emplace(dataSrc.at(0), 1.0);
        return result;
    }

    const double bandwidth = mBandwidth > 0 ? mBandwidth : 1.0;

    const double a = range_min_value(dataSrc) - 4.0 * bandwidth;
    const double b = range_max_value(dataSrc) + 4.0 * bandwidth;

    // --- Garde-fou : bornes [a, b] non finies ou dégénérées ---
    // (bandwidth aberrant -> overflow lors du calcul de a/b, ou a == b)
    if (!std::isfinite(a) || !std::isfinite(b) || b <= a) {
        qWarning() << "[MetropolisVariable::generateKDE] bornes [a, b] invalides (a =" << a
                   << ", b =" << b << ", bandwidth =" << bandwidth << ") - KDE non calculée.";
        result.emplace(dataSrc.at(0), 1.0);
        return result;
    }

    // --- Buffers et plans réutilisés (thread-local) ---
    auto buffers = FFTWThreadCache::get_buffers(fftLen);
    double* input = buffers.grid;
    fftw_complex* spectrum = buffers.spectrum;

    // Génération du buffer
    generateBufferForHisto(input, dataSrc, fftLen, a, b);

    // --- FFT Forward (r2c) ---
    fftw_plan plan_forward = FFTWThreadCache::forward(fftLen);
    fftw_execute_dft_r2c(plan_forward, input, spectrum);

    // --- Filtrage spectral ---
    const int complexSize = fftLen / 2 + 1;
    const double factor_base = 2.0 * M_PI / (b - a);

    for (int i = 0; i < complexSize; ++i) {
        const double s = factor_base * static_cast<double>(i);
        const double factor = std::exp(-0.5 * s * s * bandwidth * bandwidth);

        spectrum[i][0] *= factor; // Partie réelle
        spectrum[i][1] *= factor; // Partie imaginaire
    }

    // --- FFT Backward (c2r) ---
    fftw_plan plan_backward = FFTWThreadCache::backward(fftLen);
    fftw_execute_dft_c2r(plan_backward, spectrum, input);

    // Calcul des bornes selon le support
    double tBegin = a, tEnd = b;
    switch (mSupport) {
    case eRp:
    case eRpStar:
        tBegin = 0.0;
        break;
    case eRm:
    case eRmStar:
        tEnd = 0.0;
        break;
    case eBounded:
        tBegin = tmin;
        tEnd = tmax;
        break;
    case eR:
        break;
    }

    // Construction du résultat (intervalle [a, b[)
    const double delta = (b - a) / static_cast<double>(fftLen);
    for (int i = 0; i < fftLen; ++i) {
        const double t = a + static_cast<double>(i) * delta;
        result[t] = std::max(0.0, input[i]);
    }

    // Normalisation
    result = getMapDataInRange(result, tBegin, tEnd);
    result = equal_areas(result, 1.0);

    return result;
}

void MetropolisVariable::generateFormatedKDE(const std::vector<ChainSpecs> &chains, const int fftLen, const double tmin, const double tmax)
{
    if (mFormatedBurnAdaptTrace == nullptr || mFormatedBurnAdaptTrace->size() == 0)
        return;

    if (mFormatedAcquiredTrace == nullptr || mFormatedAcquiredTrace->size() == 0)
        return;


    const std::vector<double>& trace = *mFormatedAcquiredTrace;
    mFormatedKDE = generateKDE(trace, fftLen, tmin, tmax);

    mChainsKDE.clear();
    for (size_t i = 0; i<chains.size(); ++i) {
        const std::vector<double> &subTrace = formatedAcquiredTraceforChain(chains, i);
        if (!subTrace.empty()) {
            mChainsKDE.push_back(generateKDE(subTrace, fftLen, tmin, tmax) );
        }
    }
}


// obsolete
void MetropolisVariable::memoHistoParameter(const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    mfftLenUsed = fftLen;
    mBandwidth = bandwidth;
    mtminUsed = tmin;
    mtmaxUsed = tmax;
}
//obsolete
bool MetropolisVariable::HistoWithParameter(const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
   return ((mfftLenUsed == fftLen) &&  (mBandwidth == bandwidth) && (mtminUsed == tmin) && (mtmaxUsed == tmax) ? true: false);
}

void MetropolisVariable::generateHPD(const double threshold)
{
    if (!mFormatedKDE.empty())  {
        const double thresh = std::clamp(threshold, 0.0, 100.0);
        if (thresh == 100.) {
            mFormatedHPD = mFormatedKDE;
            return;

        } else if (thresh == 0.) {
            mFormatedHPD.clear();
            return;

        } else {
            QList<QPair<double, QPair<double, double> > > formated_intervals;
           
            auto tmp_HPD = std::map<double, double>(create_HPD_by_dichotomy(mFormatedKDE, formated_intervals, thresh));
            mFormatedHPD = tmp_HPD;
            mRawHPDintervals.clear();

            for (auto it = formated_intervals.begin(); it != formated_intervals.end(); ++it) {
                double tmin ,tmax ;
                if (mFormat == DateUtils::eNumeric || mFormat == DateUtils::eUnknown) {
                    tmin = it->second.first;
                    tmax = it->second.second;

                } else {
                    tmin = DateUtils::convertFromAppSettingsFormat(it->second.first);
                    tmax = DateUtils::convertFromAppSettingsFormat(it->second.second);
                }

                if (tmin>tmax)
                    std::swap(tmin, tmax);
                
                if (!mRawHPDintervals.isEmpty() && mRawHPDintervals.at(0).second.second < tmin) {
                    auto t_t = std::make_pair(tmin, tmax);
                    auto tmp_HPD = std::make_pair(it->first, t_t );
                    mRawHPDintervals.push_back(tmp_HPD);
                } else {
                    auto t_t = std::make_pair(tmin, tmax);
                    auto tmp_HPD = std::make_pair(it->first, t_t );
                    mRawHPDintervals.push_front(tmp_HPD);
                }

            }
        }


    } else {
        // This can happen on phase duration, if only one event inside.
        // alpha = beta => duration is always null !
        // We don't display the phase duration but we print the numerical HPD result.
        mFormatedHPD = std::map<double, double>();
        mRawHPDintervals.clear();
        qDebug() << "[MetropolisVariable::generateHPD] WARNING : Cannot generate HPD on empty histo with " << QString::fromStdString(mName);
    }
}

void MetropolisVariable::generateCredibility(const double threshold)
{
    if (mAllAcquiredTrace == nullptr || mAllAcquiredTrace->size() == 0)  {
        mRawCredibility = std::pair<double, double>(1, -1);

    } else if (mThresholdUsed != threshold || mExactCredibilityThreshold == 0.0) {
        const std::vector<double>& trace = *mAllAcquiredTrace;
        mRawCredibility = credibilityForTrace(trace, threshold, mExactCredibilityThreshold);//, "Compute credibility for "+getName());
    }
    updateFormatedCredibility(mFormat);

}

void MetropolisVariable::generateCorrelations(const std::vector<ChainSpecs> &chains)
{
    const int hmax = 40;
    if (!mCorrelations.empty())
        mCorrelations.clear();

    //mCorrelations.reserve(chains.size());
    //Chronometer ch ("[MetropolisVariable::generateCorrelations]");

    for (size_t i = 0; i<chains.size(); ++i) {
        // Return the acquisition part of the trace
        //const std::vector<double> &trace = runRawTraceForChain(chains, i);
        const std::vector<double>& trace = acquiredTraceforChain(chains, i);
        if (trace.size() < hmax)
            continue;

        const std::vector<double> &results = autocorrelation_schoolbook(trace);
        //QList<double> results = autocorrelation_by_convol(trace); // test

        // Correlation ajoutée à la liste (une courbe de corrélation par chaine)
        mCorrelations.push_back(results);
    }
    //ch.display();
}

/* --------------------------------------------------------------
   1️⃣  generateDensityNumericalResults
   -------------------------------------------------------------- */
void MetropolisVariable::generateDensityNumericalResults(const std::vector<ChainSpecs> &chains)
{
    // ----- Résultats globaux (concatenation de toutes les chaînes) -----
    if (mFormatedKDE.empty())
        return;
    mResults.densityAnalysis = analyseDensity(mFormatedKDE);
    mResults.densityAnalysis.bandwidth_used = mBandwidth;

}

/* --------------------------------------------------------------
   2️⃣  generateTraceNumericalResults
   -------------------------------------------------------------- */
void MetropolisVariable::generateTraceNumericalResults(const std::vector<ChainSpecs> &chains)
{
    // ----- Résultats globaux (concatenation de toutes les chaînes) -----
    const std::vector<double>& trace = *mFormatedAcquiredTrace;
    if (!mFormatedAcquiredTrace)
        return;

    if (mFormatedAcquiredTrace->empty())
        return;

    if (mResults.traceAnalysis.updated == true)
        return;

    mResults.traceAnalysis.updated = true;

    if (mAllAcquiredTrace->size() > chains.size() + 1) {
        // -----------------------------------------------------------------
        // 1. Recherche de la taille d'acquisition la plus petite
        // -----------------------------------------------------------------
        int sizeMin = std::numeric_limits<int>::max();
        for (std::size_t chain_index = 0; chain_index < chains.size(); ++chain_index)
            sizeMin = std::min(sizeMin, chains[chain_index].mRealyAccepted);

        // -----------------------------------------------------------------
        // 2. Construction du tableau de sous-chaînes à utiliser pour Rhat/ESS
        // -----------------------------------------------------------------
        std::vector<std::vector<double>> chainForRhat(chains.size());
        for (std::size_t chain_index = 0; chain_index < chains.size(); ++chain_index) {
            std::vector<double> fullTrace = runRawTraceForChain(chains, chain_index);
            chainForRhat[chain_index] = std::vector<double>(fullTrace.end() - sizeMin, fullTrace.end());
        }

        // -----------------------------------------------------------------
        // 3. Rhat + ESS
        // -----------------------------------------------------------------
        MCMCDiagnostic::RhatEssResult R_hat_ESS;
        try {
            R_hat_ESS = MCMCDiagnostic::computeRhatAndEss(chainForRhat);
        } catch (const std::invalid_argument&) {
            // chaînes vides ou trop courtes pour un split (N < 4) : "indéterminé"
            R_hat_ESS.rHat    = 0.0;
            R_hat_ESS.bulkESS = std::numeric_limits<double>::quiet_NaN();
            R_hat_ESS.tailESS = std::numeric_limits<double>::quiet_NaN();
        }
        mResults.RhatESS = R_hat_ESS;

    } else {
        // Variable fixe : Rhat conventionnellement à 1 (rien à comparer), ESS non
        // applicable plutôt que "trop faible" -> NaN pour un affichage neutre
        mResults.RhatESS.rHat    = 1.0;
        mResults.RhatESS.bulkESS = std::numeric_limits<double>::quiet_NaN();
        mResults.RhatESS.tailESS = std::numeric_limits<double>::quiet_NaN();
    }

    mResults.traceAnalysis = traceStatistic(trace);   // analyse du trace global
    // ----- Résultats *par chaîne* (trace) -----
    // 1️⃣  S’assurer que le vecteur possède exactement le bon nombre d’éléments
    if (mChainsResults.size() != chains.size())
        mChainsResults.resize(chains.size());   // crée des objets « vide »

    // 2️⃣  Remplir uniquement le champ traceAnalysis (trace)
    for (size_t i = 0; i < chains.size(); ++i) {
        auto tracetmp = runFormatedTraceForChain(chains, i);
        mChainsResults[i].traceAnalysis = traceStatistic(runFormatedTraceForChain(chains, i));
        // on ne touche pas à densityAnalysis → il garde la valeur déjà présente
    }
}

// Getters (no calculs)
std::map<double, double> &MetropolisVariable::fullHisto()
{
    return mFormatedKDE;
}

std::map<double, double> &MetropolisVariable::KDEForChain(const size_t index)
{
    Q_ASSERT(index < (size_t)mChainsKDE.size());
    return mChainsKDE[index];
}

// useless
std::vector<double>::iterator MetropolisVariable::findIter_element(const long unsigned iter, const std::vector<ChainSpecs> &chains, const size_t index ) const
{
    size_t shift = 0;
    for (size_t i = 0; i < index; ++i) {
        shift += 1 + chains[i].mIterPerBurn + (chains[i].mBatchIndex * chains[i].mIterPerBatch) + chains[i].mRealyAccepted;
    }
    shift += 1 + chains[index].mIterPerBurn +  (chains[index].mBatchIndex * chains[index].mIterPerBatch);
    return mBurnAdaptTrace->begin() + shift + iter;

}



/**
 * @brief MetropolisVariable::fullFormatedTraceForChain
 * @param chains QList of the ChainSpecs in the Model
 * @param index
 * @return The complet trace (init, Burn-in, adaptation, acquire) corresponding to chain n°index
 */
std::vector<double> MetropolisVariable::fullFormatedTraceForChain(
    const std::vector<ChainSpecs>& chains,
    std::size_t index) const noexcept
{
    // -------------------------------------------------------------
    // 0️⃣  Vérifications d’entrée
    // -------------------------------------------------------------
    if (index >= chains.size())
        return {};                     // indice invalide → vecteur vide

    // -------------------------------------------------------------
    // 1️⃣  Calcul de l’offset (déplacement) dans la trace *burn‑adapt*
    // -------------------------------------------------------------
    std::size_t burnOffset = 0;
    for (std::size_t i = 0; i < index; ++i) {
        burnOffset += 1ULL                                   // l’init
                      + chains[i].mIterPerBurn
                      + (chains[i].mBatchIndex * chains[i].mIterPerBatch);
    }

    const std::size_t burnSize = 1ULL
                                 + chains[index].mIterPerBurn
                                 + (chains[index].mBatchIndex * chains[index].mIterPerBatch);

    // -------------------------------------------------------------
    // 2️⃣  Calcul de l’offset dans la trace *acceptée*
    // -------------------------------------------------------------
    std::size_t acceptOffset = 0;
    for (std::size_t i = 0; i < index; ++i) {
        //acceptOffset += chains[i].mRealyAccepted;
        acceptOffset += chains[i].mIterDisplay;
    }

    //const std::size_t acceptSize = chains[index].mRealyAccepted;
    const std::size_t acceptSize = chains[index].mIterDisplay;

    // -------------------------------------------------------------
    // 3️⃣  Construction du vecteur résultat (une seule allocation)
    // -------------------------------------------------------------
    std::vector<double> result;
    result.reserve(burnSize + acceptSize);   // évite les réallocations

    // Copie de la partie burn‑adapt
    result.insert(result.end(),
                  mFormatedBurnAdaptTrace->begin() + static_cast<std::ptrdiff_t>(burnOffset),
                  mFormatedBurnAdaptTrace->begin() + static_cast<std::ptrdiff_t>(burnOffset + burnSize));

    // Copie de la partie acceptée
    /*result.insert(result.end(),
                  mAllAcquiredTrace->begin() + static_cast<std::ptrdiff_t>(acceptOffset),
                  mAllAcquiredTrace->begin() + static_cast<std::ptrdiff_t>(acceptOffset + acceptSize));
    */
    auto trace = mFormatedAcquiredTrace;

    result.insert(result.end(),
                  trace->begin() + static_cast<std::ptrdiff_t>(acceptOffset),
                  trace->begin() + static_cast<std::ptrdiff_t>(acceptOffset + acceptSize));


    return result;
}




std::vector<double> MetropolisVariable::correlationForChain(const size_t index)
{
    if (index < (size_t)mCorrelations.size())
        return mCorrelations.at(index);

    return std::vector<double>();
}


QString MetropolisVariable::resultsString(const QString &noResultMessage, const QString &unit) const
{
    if (mFormatedKDE.empty())
        return noResultMessage;

    QString result = posteriorAnalysisToString(mResults) + "<br>";

    result += "<i>"+ QObject::tr("Probabilities") + " </i><br>";

    // the mFormatedCredibility is already in the time scale, we don't need to convert
    if (mFormatedCredibility != std::pair<double, double>(1, -1))
        result += QObject::tr("Credibility Interval") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(mExactCredibilityThreshold * 100.0),
                                                                                                 stringForLocal(mFormatedCredibility.first),
                                                                                                 stringForLocal(mFormatedCredibility.second),
                                                                                                 unit) + "<br>";
    if (!mRawHPDintervals.isEmpty()) {
        const QList<QPair<double, QPair<double, double>>>& intervals = mRawHPDintervals;

        const double total_thresh = std::accumulate(intervals.begin(), intervals.end(), 0.0, [](double sum, auto i) {return sum + i.first;});


        result += QObject::tr("HPD Region ( %1 %) :").arg(stringForLocal(total_thresh * 100.0));
        if (mFormat == DateUtils::eNumeric) {

            for (auto&& interval : intervals) {
                const QString str_rate = stringForLocal(interval.first * 100.0);
                const QString str_tmin = stringForLocal(interval.second.first);
                const QString str_tmax = stringForLocal(interval.second.second);
                result +=  QString(" [ %2 ; %3 ] (%4 %) ").arg(str_tmin, str_tmax, str_rate);
            }

        } else if (DateUtils::is_date(mFormat)) {
            for (auto&& interval : intervals) {
                const QString str_rate = stringForLocal(interval.first * 100.0);
                const QString str_tmin = stringForLocal(DateUtils::convertToAppSettingsFormat(interval.second.first));
                const QString str_tmax = stringForLocal(DateUtils::convertToAppSettingsFormat(interval.second.second));
                result +=  QString(" [ %2 ; %3 ] (%4 %) ").arg(str_tmin, str_tmax, str_rate);
            }

        } else {
            for (auto interval = intervals.crbegin(); interval != intervals.crend(); interval++) {
                const QString str_rate = stringForLocal(interval->first * 100.0);
                const QString str_tmin = DateUtils::convertToAppSettingsFormatStr(interval->second.second);
                const QString str_tmax = DateUtils::convertToAppSettingsFormatStr(interval->second.first);
                result +=  QString(" [ %2 ; %3 ] (%4 %) ").arg(str_tmin, str_tmax, str_rate);
            }
        }
        result += unit + "<br>";
    }

   return result;
}

QStringList MetropolisVariable::getResultsList(const QLocale locale, const int precision, const bool withDateFormat) const
{
    QStringList list;
    if (withDateFormat) {
        // Statistic Results on Trace
        list << locale.toString(mResults.traceAnalysis.mean, 'f', precision);
        list << locale.toString(mResults.traceAnalysis.std, 'f', precision);
        list << locale.toString(mResults.traceAnalysis.quartiles.Q1, 'f', precision);
        list << locale.toString(mResults.traceAnalysis.quartiles.Q2, 'f', precision);
        list << locale.toString(mResults.traceAnalysis.quartiles.Q3, 'f', precision);
        list << locale.toString(mResults.traceAnalysis.min, 'f', precision);
        list << locale.toString(mResults.traceAnalysis.max, 'f', precision);

         // Statistic Results on Density
        list << locale.toString(mResults.densityAnalysis.mode, 'f', precision);
        list << locale.toString(mResults.densityAnalysis.mean, 'f', precision);
        list << locale.toString(mResults.densityAnalysis.std, 'f', precision);
        list << locale.toString(mResults.densityAnalysis.quartiles.Q1, 'f', precision);
        list << locale.toString(mResults.densityAnalysis.quartiles.Q2, 'f', precision);
        list << locale.toString(mResults.densityAnalysis.quartiles.Q3, 'f', precision);

        list << locale.toString(mExactCredibilityThreshold * 100.0, 'f', 2);
        list << locale.toString(mFormatedCredibility.first, 'f', precision);
        list << locale.toString(mFormatedCredibility.second, 'f', precision);

        const QList<QPair<double, QPair<double, double>>>& intervals = mRawHPDintervals;
        double min_inter = DateUtils::convertToAppSettingsFormat(intervals[0].second.first);
        double max_inter = DateUtils::convertToAppSettingsFormat(intervals[0].second.second);

        if (min_inter < max_inter) {
            for (auto&& interval : intervals) {
                list << locale.toString(interval.first * 100.0, 'f', 2);
                list << locale.toString(DateUtils::convertToAppSettingsFormat(interval.second.first), 'f', precision);
                list << locale.toString(DateUtils::convertToAppSettingsFormat(interval.second.second), 'f', precision);
            }

        } else {
            for (auto interval = intervals.crbegin(); interval != intervals.crend(); interval++) {
                list << locale.toString(interval->first * 100.0, 'f', 2);
                min_inter = DateUtils::convertToAppSettingsFormat(interval->second.second);
                max_inter = DateUtils::convertToAppSettingsFormat(interval->second.first);
                list << locale.toString(min_inter, 'f', precision);
                list << locale.toString(max_inter, 'f', precision);
            }
        }


    } else {
          // Statistic Results on Trace
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.traceAnalysis.mean), 'f', precision);
        list << locale.toString(mResults.traceAnalysis.std, 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.traceAnalysis.min), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.traceAnalysis.max), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.traceAnalysis.quartiles.Q1), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.traceAnalysis.quartiles.Q2), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.traceAnalysis.quartiles.Q3), 'f', precision);
        list << locale.toString(mExactCredibilityThreshold * 100.0, 'f', 2);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mFormatedCredibility.first), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mFormatedCredibility.second), 'f', precision);
        // Statistic Results on Density
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.densityAnalysis.mode), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.densityAnalysis.mean), 'f', precision);
        list << locale.toString(mResults.densityAnalysis.std, 'f', precision);

        const QList<QPair<double, QPair<double, double>>>& intervals = mRawHPDintervals;

        if (DateUtils::is_date(mFormat)) {
            for (auto&& interval : intervals) {
                list << locale.toString(interval.first * 100.0, 'f', 2);
                list << locale.toString(DateUtils::convertToAppSettingsFormat(interval.second.first), 'f', precision);
                list << locale.toString(DateUtils::convertToAppSettingsFormat(interval.second.second), 'f', precision);
            }

        } else {
            for (auto interval = intervals.crbegin(); interval != intervals.crend(); interval++) {
                list << locale.toString(interval->first * 100.0, 'f', 2);
                const double min_inter = DateUtils::convertToAppSettingsFormat(interval->second.second);
                const double max_inter = DateUtils::convertToAppSettingsFormat(interval->second.first);
                list << locale.toString(min_inter, 'f', precision);
                list << locale.toString(max_inter, 'f', precision);
            }
        }
    }

    return list;
}

#pragma mark Write Data
void MetropolisVariable::save_stream_v330(QDataStream& stream) const
{
    try {
        // Vérification initiale du stream
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Initial stream error");
        }

        // Écriture du nom
        QString str = QString::fromStdString(mName);
        stream << str;

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write variable name");
        }

        // Écriture du support
        quint8 support;
        switch (mSupport) {
        case eR:        support = 0; break;
        case eRp:       support = 1; break;
        case eRm:       support = 2; break;
        case eRpStar:   support = 3; break;
        case eRmStar:   support = 4; break;
        case eBounded:  support = 5; break;
        default:
            throw std::runtime_error("Invalid support type");
        }
        stream << support;

        // Écriture du format de date
        qint16 formatDate;
        switch (mFormat) {
        case DateUtils::eUnknown:  formatDate = -2; break;
        case DateUtils::eNumeric:  formatDate = -1; break;
        case DateUtils::eBCAD:     formatDate = 0; break;
        case DateUtils::eCalBP:    formatDate = 1; break;
        case DateUtils::eCalB2K:   formatDate = 2; break;
        case DateUtils::eDatBP:    formatDate = 3; break;
        case DateUtils::eDatB2K:   formatDate = 4; break;
        case DateUtils::eBCECE:    formatDate = 5; break;
        case DateUtils::eKa:       formatDate = 6; break;
        case DateUtils::eMa:       formatDate = 7; break;
        default:
            throw std::runtime_error("Invalid date format");
        }
        stream << formatDate;

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write date format");
        }

        // Écriture du trace brut

        save_container_nullable(stream, mBurnAdaptTrace);
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write raw trace");
        }

    } catch (const std::exception& e) {
        qDebug() << "[MetropolisVariable::save_stream_v330] Error: "
                 << e.what()
                 << " ; stream.status()=" << stream.status();
        // Politique de gestion d'erreur selon vos besoins
        // Vous pouvez choisir de lancer, réinitialiser ou ignorer
    }
}

void MetropolisVariable::save_stream_v337(QDataStream& stream) const
{
    try {
        // Vérification initiale du stream
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Initial stream error");
        }

        // Écriture du nom
        QString str = QString::fromStdString(mName);
        stream << str;
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write variable name");
        }
        save_container(stream, mAcceptedStateCountByChain);


        // Écriture du support
        quint8 support;
        switch (mSupport) {
        case eR:        support = 0; break;
        case eRp:       support = 1; break;
        case eRm:       support = 2; break;
        case eRpStar:   support = 3; break;
        case eRmStar:   support = 4; break;
        case eBounded:  support = 5; break;
        default:
            throw std::runtime_error("Invalid support type");
        }
        stream << support;

        // Écriture du format de date
        qint16 formatDate;
        switch (mFormat) {
        case DateUtils::eUnknown:  formatDate = -2; break;
        case DateUtils::eNumeric:  formatDate = -1; break;
        case DateUtils::eBCAD:     formatDate = 0; break;
        case DateUtils::eCalBP:    formatDate = 1; break;
        case DateUtils::eCalB2K:   formatDate = 2; break;
        case DateUtils::eDatBP:    formatDate = 3; break;
        case DateUtils::eDatB2K:   formatDate = 4; break;
        case DateUtils::eBCECE:    formatDate = 5; break;
        case DateUtils::eKa:       formatDate = 6; break;
        case DateUtils::eMa:       formatDate = 7; break;
        default:
            throw std::runtime_error("Invalid date format");
        }
        stream << formatDate;

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write date format");
        }

        // Écriture du trace brut

        save_container_nullable(stream, mBurnAdaptTrace);
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write raw Burn Adapt trace");
        }

        save_container_nullable(stream, mAllAcquiredTrace);
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write raw Acquired trace");
        }

    } catch (const std::exception& e) {
        qDebug() << "[MetropolisVariable::save_stream_v337] Error: "
                 << e.what()
                 << " ; stream.status()=" << stream.status();
        // Politique de gestion d'erreur selon vos besoins
        // Vous pouvez choisir de lancer, réinitialiser ou ignorer
    }
}

void MetropolisVariable::save_stream_v338(QDataStream& stream) const
{
    try {
        // Vérification initiale du stream
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Initial stream error");
        }

        // Écriture du nom
        QString str = QString::fromStdString(mName);
        stream << str;
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write variable name");
        }
        save_container(stream, mAcceptedStateCountByChain);


        // Écriture du support
        quint8 support;
        switch (mSupport) {
        case eR:        support = 0; break;
        case eRp:       support = 1; break;
        case eRm:       support = 2; break;
        case eRpStar:   support = 3; break;
        case eRmStar:   support = 4; break;
        case eBounded:  support = 5; break;
        default:
            throw std::runtime_error("Invalid support type");
        }
        stream << support;

        // Écriture du format de date
        qint16 formatDate;
        switch (mFormat) {
        case DateUtils::eUnknown:  formatDate = -2; break;
        case DateUtils::eNumeric:  formatDate = -1; break;
        case DateUtils::eBCAD:     formatDate = 0; break;
        case DateUtils::eCalBP:    formatDate = 1; break;
        case DateUtils::eCalB2K:   formatDate = 2; break;
        case DateUtils::eDatBP:    formatDate = 3; break;
        case DateUtils::eDatB2K:   formatDate = 4; break;
        case DateUtils::eBCECE:    formatDate = 5; break;
        case DateUtils::eKa:       formatDate = 6; break;
        case DateUtils::eMa:       formatDate = 7; break;
        case DateUtils::eCustom:   formatDate = 8; break;
        default:
            throw std::runtime_error("Invalid date format");
        }
        stream << formatDate;

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write date format");
        }

        // Écriture du trace brut

        save_container_nullable(stream, mBurnAdaptTrace);
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write raw Burn Adapt trace");
        }

        save_container_nullable(stream, mAllAcquiredTrace);
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write raw Acquired trace");
        }

        stream << is_curve_filtering ;
        save_container_nullable(stream, mDisplayAcquiredTrace);

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to write display Acquired trace");
        }
    } catch (const std::exception& e) {
        qDebug() << "[MetropolisVariable::save_stream_v338] Error: "
                 << e.what()
                 << " ; stream.status()=" << stream.status();

    }
}


QDataStream &operator<<( QDataStream &stream, const MetropolisVariable& data )
{
    data.save_stream(stream);  // Sauvegarde
    return stream;

}

#pragma mark Read Data

void MetropolisVariable::load_stream_v328(QDataStream& stream)
{
    QString qstr;
    stream >> qstr; // since 2024_08_23
    mName = qstr.toStdString();

    quint8 support;
    stream >> support;
    switch (int (support)) {
        case 0 : mSupport = Support::eR; // on R
            break;
        case 1 : mSupport = Support::eRp; // on R+
            break;
        case 2 : mSupport = Support::eRm; // on R-
            break;
        case 3 : mSupport = Support::eRpStar; // on R+*
            break;
        case 4 : mSupport = Support::eRmStar; // on R-*
            break;
        case 5 : mSupport = Support::eBounded; // on bounded support
            break;
    }

    qint16 formatDate;
    stream >> formatDate; // to keep compatibility

    mFormat = DateUtils::eUnknown; // to keep compatibility and force updateFormat

    load_container_nullable(stream, mBurnAdaptTrace);
}

void MetropolisVariable::load_stream_v330(QDataStream& stream)
{
    // Initial stream check
    if (stream.status() != QDataStream::Ok) {
        std::cout << "[MetropolisVariable::load_stream_v330] Initial stream error" << std::endl;
        return;
    }

    try {
        // Read name
        QString str;
        stream >> str;

        if (stream.status() != QDataStream::Ok) {
            std::cout << "[MetropolisVariable::load_stream_v330] Failed to read variable (name)" << std::endl;
            throw std::runtime_error("Failed to read variable name");
        }

        //std::cout << "[MetropolisVariable::load_stream_v330] name str = " << str.toStdString() << std::endl;
        mName = str.toStdString();

        // Read support
        quint8 support;
        stream >> support;
        switch (int(support)) {
        case 0: mSupport = Support::eR; break;
        case 1: mSupport = Support::eRp; break;
        case 2: mSupport = Support::eRm; break;
        case 3: mSupport = Support::eRpStar; break;
        case 4: mSupport = Support::eRmStar; break;
        case 5: mSupport = Support::eBounded; break;
        default:
            throw std::runtime_error("Invalid support type");
        }

        // Read date format
        qint16 formatDate;
        stream >> formatDate;
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read date format");
        }

        // Convert date format
        switch (formatDate) {
        case -2: mFormat = DateUtils::eUnknown; break;
        case -1: mFormat = DateUtils::eNumeric; break;
        case 0: mFormat = DateUtils::eBCAD; break;
        case 1: mFormat = DateUtils::eCalBP; break;
        case 2: mFormat = DateUtils::eCalB2K; break;
        case 3: mFormat = DateUtils::eDatBP; break;
        case 4: mFormat = DateUtils::eDatB2K; break;
        case 5: mFormat = DateUtils::eBCECE; break;
        case 6: mFormat = DateUtils::eKa; break;
        case 7: mFormat = DateUtils::eMa; break;
        default:
            throw std::runtime_error("Invalid date format");
        }


        // Read raw trace
        load_container_nullable(stream, mBurnAdaptTrace);

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read raw trace");
        }

    } catch (const std::exception& e) {
        std::cout << "[MetropolisVariable::load_stream_v330] Error: "
                 << e.what()
                  << " ; stream.status()=" << stream.status()<< std::endl;

    }
}

// In this version, we add mAcceptedStateCount
void MetropolisVariable::load_stream_v337(QDataStream& stream)
{
    // Initial stream check
    if (stream.status() != QDataStream::Ok) {
        std::cout << "[MetropolisVariable::load_stream_v337] Initial stream error" << std::endl;
        return;
    }

    try {
        // Read name
        QString str;
        stream >> str;

        if (stream.status() != QDataStream::Ok) {
            std::cout << "[MetropolisVariable::load_stream_v337] Failed to read variable (name)" << std::endl;
            throw std::runtime_error("Failed to read variable name");
        }

        mName = str.toStdString();

        load_container(stream, mAcceptedStateCountByChain);

        // Read support
        quint8 support;
        stream >> support;
        switch (int(support)) {
        case 0: mSupport = Support::eR; break;
        case 1: mSupport = Support::eRp; break;
        case 2: mSupport = Support::eRm; break;
        case 3: mSupport = Support::eRpStar; break;
        case 4: mSupport = Support::eRmStar; break;
        case 5: mSupport = Support::eBounded; break;
        default:
            throw std::runtime_error("Invalid support type");
        }

        // Read date format
        qint16 formatDate;
        stream >> formatDate;
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read date format");
        }

        // Convert date format
        switch (formatDate) {
        case -2: mFormat = DateUtils::eUnknown; break;
        case -1: mFormat = DateUtils::eNumeric; break;
        case 0: mFormat = DateUtils::eBCAD; break;
        case 1: mFormat = DateUtils::eCalBP; break;
        case 2: mFormat = DateUtils::eCalB2K; break;
        case 3: mFormat = DateUtils::eDatBP; break;
        case 4: mFormat = DateUtils::eDatB2K; break;
        case 5: mFormat = DateUtils::eBCECE; break;
        case 6: mFormat = DateUtils::eKa; break;
        case 7: mFormat = DateUtils::eMa; break;
        default:
            throw std::runtime_error("Invalid date format");
        }


        // Read raw trace
        load_container_nullable(stream, mBurnAdaptTrace);

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read Burn Adapt traces");
        }

        load_container_nullable(stream, mAllAcquiredTrace);

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read raw Acquired trace");
        }
    } catch (const std::exception& e) {
        std::cout << "[MetropolisVariable::load_stream_v337] Error: "
                  << e.what()
                  << " ; stream.status()=" << stream.status()<< std::endl;

    }
}

void MetropolisVariable::load_stream_v338(QDataStream& stream)
{
    // Initial stream check
    if (stream.status() != QDataStream::Ok) {
        std::cout << "[MetropolisVariable::load_stream_v338] Initial stream error" << std::endl;
        return;
    }

    try {
        // Read name
        QString str;
        stream >> str;

        if (stream.status() != QDataStream::Ok) {
            std::cout << "[MetropolisVariable::load_stream_v338] Failed to read variable (name)" << std::endl;
            throw std::runtime_error("Failed to read variable name");
        }

        mName = str.toStdString();

        load_container(stream, mAcceptedStateCountByChain);

        // Read support
        quint8 support;
        stream >> support;
        switch (int(support)) {
        case 0: mSupport = Support::eR; break;
        case 1: mSupport = Support::eRp; break;
        case 2: mSupport = Support::eRm; break;
        case 3: mSupport = Support::eRpStar; break;
        case 4: mSupport = Support::eRmStar; break;
        case 5: mSupport = Support::eBounded; break;
        default:
            throw std::runtime_error("[MHVariable::load_stream_v338] Invalid support type");
        }

        // Read date format
        qint16 formatDate;
        stream >> formatDate;
        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("[MHVariable::load_stream_v338] Failed to read date format");
        }

        // Convert date format
        switch (formatDate) {
        case -2: mFormat = DateUtils::eUnknown; break;
        case -1: mFormat = DateUtils::eNumeric; break;
        case 0: mFormat = DateUtils::eBCAD; break;
        case 1: mFormat = DateUtils::eCalBP; break;
        case 2: mFormat = DateUtils::eCalB2K; break;
        case 3: mFormat = DateUtils::eDatBP; break;
        case 4: mFormat = DateUtils::eDatB2K; break;
        case 5: mFormat = DateUtils::eBCECE; break;
        case 6: mFormat = DateUtils::eKa; break;
        case 7: mFormat = DateUtils::eMa; break;
        case 8: mFormat = DateUtils::eCustom; break;
        default:
            throw std::runtime_error("[MHVariable::load_stream_v338] Invalid date format");
        }


        // Read raw trace
        load_container_nullable(stream, mBurnAdaptTrace);

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("[MHVariable::load_stream_v338] Failed to read Burn Adapt traces");
        }

        load_container_nullable(stream, mAllAcquiredTrace);

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("[MHVariable::load_stream_v338] Failed to read raw Acquired trace");
        }

        stream >> is_curve_filtering;
        load_container_nullable(stream, mDisplayAcquiredTrace);

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("[MHVariable::load_stream_v338] Failed to read display Acquired trace");
        }

    } catch (const std::exception& e) {
        std::cout << "[MetropolisVariable::load_stream_v338] Error: "
                  << e.what()
                  << " ; stream.status()=" << stream.status()<< std::endl;

    }
}


QDataStream &operator>>( QDataStream& stream, MetropolisVariable& data )
{
    data.load_stream(stream);  // Chargement
    return stream;

}

#pragma mark R_hat & ESS

// Vos fonctions existantes (averageRanks / invNormalCDF / rankNormalize telles que
// partagées). ATTENTION : dans votre code elles sont déclarées `static`, donc à
// liaison interne — si ce fichier n'est pas le même .cpp que ces définitions, il
// faudra soit retirer `static` et les déclarer dans un header commun, soit déplacer
// le contenu ci-dessous dans ce même fichier.

#pragma mark Gelman Rubin

// chains[m][n] = n-ième échantillon de la chaîne m
double gelmanRubin0(const std::vector<std::vector<double>>& chains)
{
    const int M = chains.size();
    if (M < 2)
        return 0.0;

    const int N = chains[0].size();
    for (const auto& c : chains)
        if ((int)c.size() != N)
            throw std::invalid_argument("[Function::gelmanRubin] Toutes les chaînes doivent avoir la même longueur");

    if (N < 2)
        return 0.0;

    // 1. Moyennes par chaîne
    std::vector<double> chain_mean(M);
    for (int m = 0; m < M; ++m)
        chain_mean[m] = std::accumulate(chains[m].begin(), chains[m].end(), 0.0) / N;

    // 2. Moyenne globale
    double grand_mean = std::accumulate(chain_mean.begin(), chain_mean.end(), 0.0) / M;

    // 3. Variance inter-chaînes B
    double B = 0.0;
    for (int m = 0; m < M; ++m) {
        double d = chain_mean[m] - grand_mean;
        B += d * d;
    }
    B *= static_cast<double>(N) / (M - 1);

    // 4. Variance intra-chaîne W
    double W = 0.0;
    for (int m = 0; m < M; ++m) {
        double s2 = 0.0;
        for (int n = 0; n < N; ++n) {
            double d = chains[m][n] - chain_mean[m];
            s2 += d * d;
        }
        W += s2 / (N - 1);
    }
    W /= M;

    // Cas dégénérés
    if (W < 1e-15)        // chaînes constantes
        return 1.0;
    if (B < 1e-15 * W)   // chaînes indiscernables
        return 1.0;

    // 5. Variance marginale estimée
    double V_hat = ((N - 1.0) / N) * W + ((M + 1.0) / (M * N)) * B;

    // 6. R-hat
    return std::sqrt(V_hat / W);
}


struct WelfordStats {
    double mean = 0.0, M2 = 0.0;
    int    count = 0;

    void update(double x) {
        ++count;
        double delta = x - mean;
        mean += delta / count;
        M2   += delta * (x - mean);
    }

    // Fusion de deux accumulateurs (Chan 1979)
    static WelfordStats merge(const WelfordStats& a, const WelfordStats& b) {
        if (a.count == 0 && b.count == 0) return WelfordStats{};
        if (a.count == 0) return b;
        if (b.count == 0) return a;
        WelfordStats r;
        r.count = a.count + b.count;
        double delta = b.mean - a.mean;
        //r.mean = (a.count * a.mean + b.count * b.mean) / r.count;
        r.mean = a.mean + delta * b.count / r.count;
        r.M2   = a.M2 + b.M2 + delta * delta * a.count * b.count / r.count;
        return r;
    }

    double variance() const { return M2 / (count - 1); }
};

// version accélérer avec knuth et la formule de Chan (WelfordStats)
double gelmanRubin(const std::vector<std::vector<double>>& chains)
{
    const int M = chains.size();
    if (M < 2) return 0.0;
    const int N = chains[0].size();
    for (const auto& c : chains)
        if ((int)c.size() != N)
            throw std::invalid_argument("Toutes les chaînes doivent avoir la même longueur");
    if (N < 2) return 0.0;

    std::vector<double> chain_mean(M);
    std::vector<double> chain_var(M);

    // Parallélisation sur N par chaîne (M chaînes séquentielles)
    for (int m = 0; m < M; ++m) {
        const auto& ch = chains[m];
        WelfordStats global;

#pragma omp parallel
        {
            WelfordStats local;

#pragma omp for schedule(static) nowait
            for (int n = 0; n < N; ++n)
                local.update(ch[n]);

#pragma omp critical
            global = WelfordStats::merge(global, local);
        }

        chain_mean[m] = global.mean;
        chain_var[m]  = global.variance();
    }

    // Moyenne globale et variance inter B
    double grand_mean = 0.0;
    for (int m = 0; m < M; ++m)
        grand_mean += chain_mean[m];
    grand_mean /= M;

    double B = 0.0;
    for (int m = 0; m < M; ++m) {
        double d = chain_mean[m] - grand_mean;
        B += d * d;
    }
    B *= static_cast<double>(N) / (M - 1);

    double W = 0.0;
    for (int m = 0; m < M; ++m)
        W += chain_var[m];
    W /= M;

    if (W < 1e-15 && B < 1e-15) return 1.0;   // chaînes toutes constantes et identiques : cas trivial
    if (W < 1e-15)               return std::numeric_limits<double>::infinity(); // W→0 mais B≠0 : chaînes bloquées sur des valeurs différentes → non-convergence sévère
    if (B < 1e-15 * W)           return 1.0;

    double V_hat = ((N - 1.0) / N) * W + ((M + 1.0) / (M * N)) * B;
    return std::sqrt(V_hat / W);
}

// Version multi-paramètres : retourne un R-hat par paramètre
// chains[m][n][p] = paramètre p, échantillon n, chaîne m
std::vector<double> gelmanRubinMulti(
    const std::vector<std::vector<std::vector<double>>>& chains)
{
    const int M = chains.size();
    const int N = chains[0].size();
    const int P = chains[0][0].size();

    std::vector<std::vector<double>> param_chains(M, std::vector<double>(N));
    std::vector<double> r_hats(P);

    for (int p = 0; p < P; ++p) {
        for (int m = 0; m < M; ++m)
            for (int n = 0; n < N; ++n)
                param_chains[m][n] = chains[m][n][p];
        r_hats[p] = gelmanRubin(param_chains);
    }
    return r_hats;
}

#pragma mark splitRhatVehtari
// -------------------------------------------------------------
//  Inverse de la CDF normale standard (algorithme d'Acklam),
//  affinée par un pas de Newton. Précision ~1e-9, largement
//  suffisante pour une transformation en scores normaux.
// -------------------------------------------------------------
double invNormalCDF(double p)
{
    static const double a[] = {-3.969683028665376e+01,  2.209460984245205e+02,
                               -2.759285104469687e+02,  1.383577518672690e+02,
                               -3.066479806614716e+01,  2.506628277459239e+00};
    static const double b[] = {-5.447609879822406e+01,  1.615858368580409e+02,
                               -1.556989798598866e+02,  6.680131188771972e+01,
                               -1.328068155288572e+01};
    static const double c[] = {-7.784894002430293e-03, -3.223964580411365e-01,
                               -2.400758277161838e+00, -2.549732539343734e+00,
                               4.374664141464968e+00,  2.938163982698783e+00};
    static const double d[] = { 7.784695709041462e-03,  3.224671290700398e-01,
                               2.445134137142996e+00,  3.754408661907416e+00};

    if (p <= 0.0) return -std::numeric_limits<double>::infinity();
    if (p >= 1.0) return  std::numeric_limits<double>::infinity();

    const double p_low  = 0.02425;
    const double p_high = 1.0 - p_low;
    double q, r, x;

    if (p < p_low) {
        q = std::sqrt(-2.0 * std::log(p));
        x = (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
            ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1.0);
    } else if (p <= p_high) {
        q = p - 0.5;
        r = q * q;
        x = (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q /
            (((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1.0);
    } else {
        q = std::sqrt(-2.0 * std::log(1.0 - p));
        x = -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
            ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1.0);
    }

    // Raffinement Newton (recommandé par Acklam pour une précision ~1e-9)
    const double e = 0.5 * std::erfc(-x / std::sqrt(2.0)) - p;
    const double u = e * std::sqrt(2.0 * M_PI) * std::exp(x * x / 2.0);
    return x - u / (1.0 + x * u / 2.0);
}

// -------------------------------------------------------------
//  Rangs moyens (fractional ranking, gère les ex-aequo comme
//  R : rank(x, ties.method = "average"))
// -------------------------------------------------------------
static std::vector<double> averageRanks(const std::vector<double>& v)
{
    const int S = static_cast<int>(v.size());
    std::vector<int> idx(S);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](int i, int j) { return v[i] < v[j]; });

    std::vector<double> ranks(S);
    int i = 0;
    while (i < S) {
        int j = i;
        while (j + 1 < S && v[idx[j + 1]] == v[idx[i]]) ++j;
        const double avg_rank = 0.5 * ((i + 1) + (j + 1)); // rangs 1-based
        for (int k = i; k <= j; ++k)
            ranks[idx[k]] = avg_rank;
        i = j + 1;
    }
    return ranks;
}

// -------------------------------------------------------------
//  Transformation en scores normaux (Blom) : z = Phi^-1((rang - 3/8)/(S - 1/4))
// -------------------------------------------------------------
static std::vector<double> rankNormalize(const std::vector<double>& pooled)
{
    const int S = static_cast<int>(pooled.size());
    const std::vector<double> ranks = averageRanks(pooled);
    std::vector<double> z(S);
    for (int i = 0; i < S; ++i)
        z[i] = invNormalCDF((ranks[i] - 3.0 / 8.0) / (static_cast<double>(S) - 1.0 / 4.0));
    return z;
}

/**
 * @brief Split-Rhat rang-normalisé et replié, suivant Vehtari, Gelman,
 *        Simpson, Carpenter & Bürkner (2021), "Rank-normalization, folding,
 *        and localization: An improved R-hat for assessing convergence of
 *        MCMC", Bayesian Analysis.
 *
 * @details
 *   1. Chaque chaîne est coupée en deux moitiés (2M demi-chaînes), ce qui
 *      rend le diagnostic sensible à une dérive à l'intérieur d'une même
 *      chaîne (non-stationnarité que le Rhat classique sur chaînes entières
 *      ne détecte pas).
 *   2. "Bulk-Rhat" : Rhat classique (réutilise gelmanRubin) calculé sur les
 *      valeurs rang-normalisées — robuste aux queues lourdes / distributions
 *      non gaussiennes, contrairement au Rhat classique sur les valeurs brutes.
 *   3. "Tail-Rhat" : même procédure sur les valeurs repliées autour de la
 *      médiane globale (|x - médiane|) — détecte une non-convergence des
 *      queues/variances même quand les moyennes ont déjà convergé.
 *   4. Rhat final = max(bulk-Rhat, tail-Rhat).
 *
 * @param chains  M chaînes de même longueur N (N >= 4 requis pour un split
 *                significatif).
 * @return Le split-Rhat rang-normalisé et replié.
 */
double splitRhatVehtari(const std::vector<std::vector<double>>& chains)
{
    const int M = static_cast<int>(chains.size());
    if (M < 1) return std::numeric_limits<double>::infinity();
    const int N = static_cast<int>(chains[0].size());
    for (const auto& c : chains)
        if (static_cast<int>(c.size()) != N)
            throw std::invalid_argument("Toutes les chaînes doivent avoir la même longueur");
    if (N < 4)
        throw std::invalid_argument("Chaque chaîne doit contenir au moins 4 valeurs pour un split-Rhat");

    // 1) Split : chaque chaîne -> 2 demi-chaînes de longueur floor(N/2)
    //    (l'échantillon central est ignoré si N est impair — convention
    //    standard, identique à celle du package R 'posterior')
    const int Nh = N / 2;
    std::vector<std::vector<double>> halves;
    halves.reserve(2 * M);
    for (const auto& c : chains) {
        halves.emplace_back(c.begin(), c.begin() + Nh);
        halves.emplace_back(c.end() - Nh, c.end());
    }
    const int S = 2 * M * Nh;

    // 2) Pool global (pour rangs et médiane)
    std::vector<double> pooled;
    pooled.reserve(S);
    for (const auto& h : halves)
        pooled.insert(pooled.end(), h.begin(), h.end());

    std::vector<double> sorted_pooled = pooled;
    std::sort(sorted_pooled.begin(), sorted_pooled.end());
    const double median = (S % 2)
                              ? sorted_pooled[S / 2]
                              : 0.5 * (sorted_pooled[S / 2 - 1] + sorted_pooled[S / 2]);

    // 3) Bulk : rang-normalisation directe
    const std::vector<double> z_bulk = rankNormalize(pooled);
    std::vector<std::vector<double>> bulk_halves(2 * M, std::vector<double>(Nh));
    for (int i = 0, off = 0; i < 2 * M; ++i)
        for (int j = 0; j < Nh; ++j)
            bulk_halves[i][j] = z_bulk[off++];

    // 4) Tail : repliement autour de la médiane, puis rang-normalisation
    std::vector<double> folded(S);
    for (int i = 0; i < S; ++i)
        folded[i] = std::abs(pooled[i] - median);
    const std::vector<double> z_tail = rankNormalize(folded);
    std::vector<std::vector<double>> tail_halves(2 * M, std::vector<double>(Nh));
    for (int i = 0, off = 0; i < 2 * M; ++i)
        for (int j = 0; j < Nh; ++j)
            tail_halves[i][j] = z_tail[off++];

    // 5) Rhat classique sur chaque ensemble transformé
    const double rhat_bulk = gelmanRubin(bulk_halves);
    const double rhat_tail = gelmanRubin(tail_halves);

    return std::max(rhat_bulk, rhat_tail);
}




//std::vector<double> rankNormalize(const std::vector<double>& pooled);

namespace MCMCDiagnostic
{
namespace detail
{

// W (variance intra-chaîne moyenne) et B (variance inter-chaîne, déjà multipliée
// par N/(M-1), comme dans votre gelmanRubin) pour un même jeu de demi-chaînes.
// Calculées une seule fois et réutilisées à la fois pour Rhat et pour l'ESS.
struct HalfChainStats
{
    double W = 0.;
    double B = 0.;
};

HalfChainStats computeHalfChainStats(const std::vector<std::vector<double>>& halves)
{
    const int M = static_cast<int>(halves.size());
    const int N = static_cast<int>(halves.front().size());

    std::vector<double> chainMean(M), chainVar(M);
    for (int m = 0; m < M; ++m) {
        const double mean = std::accumulate(halves[m].begin(), halves[m].end(), 0.) / N;
        double ss = 0.;
        for (double v : halves[m])
            ss += (v - mean) * (v - mean);
        chainMean[m] = mean;
        chainVar[m] = ss / (N - 1);
    }

    HalfChainStats stats;
    stats.W = std::accumulate(chainVar.begin(), chainVar.end(), 0.) / M;

    const double grandMean = std::accumulate(chainMean.begin(), chainMean.end(), 0.) / M;
    double B = 0.;
    for (double m : chainMean)
        B += (m - grandMean) * (m - grandMean);
    stats.B = B * static_cast<double>(N) / (M - 1);

    return stats;
}

// Rhat = reproduction EXACTE de la formule de votre gelmanRubin() ("coda", avec le
// terme correctif (M+1)/(M*N) sur B) et de ses gardes pour les cas dégénérés.
double rhatFromStats(const HalfChainStats& s, int M, int N)
{
    if (M < 2 || N < 2)
        return 0.0;

    const double W = s.W, B = s.B;

    if (W < 1e-15 && B < 1e-15) return 1.0;                                    // chaînes constantes et identiques
    if (W < 1e-15)               return std::numeric_limits<double>::infinity(); // W->0, B!=0 : bloquées sur des valeurs différentes
    if (B < 1e-15 * W)           return 1.0;                                    // chaînes essentiellement identiques

    const double V_hat = ((N - 1.0) / N) * W + ((M + 1.0) / (M * N)) * B;
    return std::sqrt(V_hat / W);
}

// var+ "Stan/Vehtari" (SANS le correctif (M+1)/M) : c'est cette version, et
// uniquement celle-ci, qui est mathématiquement justifiée dans la combinaison
// d'autocorrélation de l'ESS (Geyer). Le correctif (M+1)/M de gelmanRubin est
// propre au facteur de réduction d'échelle (correction à degrés de liberté finis)
// et n'a pas d'équivalent dans la dérivation de l'ESS — donc calcul séparé, à
// partir des mêmes W/B (pas de repasse sur les données brutes).
double varPlusForEss(const HalfChainStats& s, int N)
{
    return ((N - 1.0) / N) * s.W + s.B / N;
}

// Autocovariance biaisée d'une chaîne via FFT (Wiener-Khinchin), normalisation par N
// (convention Geyer/Stan), calculée avec FFTW3 et le cache de plans/buffers
// thread-local FFTWThreadCache (déjà utilisé ailleurs dans ChronoModel), plutôt
// qu'Eigen::FFT. Plus besoin de faire circuler d'objet FFT entre les appels : le
// cache est tenu par taille (Npad) en interne à FFTWThreadCache.
std::vector<double> autocovarianceFFT(const std::vector<double>& x)
{
    const int N = static_cast<int>(x.size());
    int Npad = 1;
    while (Npad < 2 * N)
        Npad <<= 1;

    const double mean = std::accumulate(x.begin(), x.end(), 0.) / static_cast<double>(N);

    FFTWThreadCache::Buffers buf = FFTWThreadCache::get_buffers(Npad);

    // Centrage + zero-padding directement dans le buffer réel réutilisable
    for (int i = 0; i < N; ++i)
        buf.grid[i] = x[i] - mean;
    for (int i = N; i < Npad; ++i)
        buf.grid[i] = 0.;

    fftw_plan planForward = FFTWThreadCache::forward(Npad);
    fftw_execute_dft_r2c(planForward, buf.grid, buf.spectrum);

    // Densité spectrale de puissance : spectrum *= conj(spectrum) (partie imaginaire -> 0)
    const int nFreq = Npad / 2 + 1;
    for (int i = 0; i < nFreq; ++i) {
        const double re = buf.spectrum[i][0];
        const double im = buf.spectrum[i][1];
        buf.spectrum[i][0] = re * re + im * im;
        buf.spectrum[i][1] = 0.;
    }

    fftw_plan planBackward = FFTWThreadCache::backward(Npad);
    fftw_execute_dft_c2r(planBackward, buf.spectrum, buf.grid);
    // FFTW ne normalise pas ses transformées : l'aller-retour r2c puis c2r
    // multiplie le résultat par Npad -> on divise par (Npad * N) pour obtenir
    // l'autocovariance biaisée normalisée par N (convention Geyer/Stan).

    std::vector<double> result(N);
    for (int t = 0; t < N; ++t)
        result[t] = buf.grid[t] / (static_cast<double>(Npad) * static_cast<double>(N));

    return result;
}

// ESS (estimateur de Geyer, séquence positive puis monotone initiale) à partir de
// demi-chaînes déjà transformées et de leurs statistiques W/B déjà calculées.
double essFromHalves(const std::vector<std::vector<double>>& halves,
                     const HalfChainStats& stats)
{
    const int M = static_cast<int>(halves.size());
    const int N = static_cast<int>(halves.front().size());
    const double varPlus = varPlusForEss(stats, N);

    if (M < 2 || N < 2 || !(varPlus > 0.) || !std::isfinite(varPlus))
        return std::numeric_limits<double>::quiet_NaN();

    std::vector<std::vector<double>> acov(M);
    for (int m = 0; m < M; ++m)
        acov[m] = autocovarianceFFT(halves[m]);

    std::vector<double> rhoHat(N, 0.);
    rhoHat[0] = 1.;
    for (int t = 1; t < N; ++t) {
        double meanCov = 0.;
        for (int m = 0; m < M; ++m)
            meanCov += acov[m][t];
        meanCov /= M;
        rhoHat[t] = 1. - (stats.W - meanCov) / varPlus;
    }

    // tau = 1 + 2*sum(rho_t) : paires consécutives, arrêt à la première paire
    // négative (séquence positive initiale), rendues monotones (non-croissantes)
    // pour réduire la variance de l'estimateur (séquence monotone initiale, Geyer 1992).
    double tau = 1.;
    double prevPairSum = std::numeric_limits<double>::infinity();
    int t = 1;
    while (t + 1 < N) {
        double pairSum = rhoHat[t] + rhoHat[t + 1];
        if (pairSum < 0.)
            break;
        pairSum = std::min(pairSum, prevPairSum);
        tau += 2. * pairSum;
        prevPairSum = pairSum;
        t += 2;
    }

    const double nTotal = static_cast<double>(M) * static_cast<double>(N);
    return std::min(nTotal / tau, nTotal);
}

} // namespace detail

RhatEssResult computeRhatAndEss(const std::vector<std::vector<double>>& chains)
{
    if (chains.empty())
        throw std::invalid_argument("Au moins une chaîne est requise");

    const int N = static_cast<int>(chains.front().size());
    for (const auto& c : chains)
        if (static_cast<int>(c.size()) != N)
            throw std::invalid_argument("Toutes les chaînes doivent avoir la même longueur");
    if (N < 4)
        throw std::invalid_argument("Chaque chaîne doit contenir au moins 4 valeurs pour un split-Rhat/ESS");

    // --- Étape commune 1 : split en demi-chaînes (une seule fois pour tout le calcul) ---
    const int Nh = N / 2;
    std::vector<std::vector<double>> halves;
    halves.reserve(2 * chains.size());
    for (const auto& c : chains) {
        halves.emplace_back(c.begin(), c.begin() + Nh);
        halves.emplace_back(c.end() - Nh, c.end());
    }
    const int M = static_cast<int>(halves.size());
    const int S = M * Nh;

    // --- Étape commune 2 : pool + tri une seule fois, réutilisés pour la médiane
    //     (tail-Rhat) ET les quantiles 5 %/95 % (tail-ESS) ---
    std::vector<double> pooled;
    pooled.reserve(S);
    for (const auto& h : halves)
        pooled.insert(pooled.end(), h.begin(), h.end());

    std::vector<double> sortedPooled = pooled;
    std::sort(sortedPooled.begin(), sortedPooled.end());
    auto quantile = [&](double p) {
        const double idx = p * static_cast<double>(S - 1);
        const size_t lo = static_cast<size_t>(std::floor(idx));
        const size_t hi = static_cast<size_t>(std::ceil(idx));
        const double frac = idx - static_cast<double>(lo);
        return sortedPooled[lo] * (1. - frac) + sortedPooled[hi] * frac;
    };
    const double median = quantile(0.5);
    const double q05 = quantile(0.05);
    const double q95 = quantile(0.95);

    // Les 3 jeux d'autocovariances (bulk, q05, q95) ont tous la même longueur Nh :
    // FFTWThreadCache met déjà en cache les plans/buffers par taille en interne,
    // pas besoin de faire circuler d'objet FFT ici.

    // --- Bulk : rang-normalisation directe, une seule fois, réutilisée pour
    //     bulk-Rhat ET bulk-ESS (mêmes demi-chaînes transformées) ---
    const std::vector<double> zBulk = rankNormalize(pooled);
    std::vector<std::vector<double>> bulkHalves(M, std::vector<double>(Nh));
    for (int i = 0, off = 0; i < M; ++i)
        for (int j = 0; j < Nh; ++j)
            bulkHalves[i][j] = zBulk[off++];

    const detail::HalfChainStats bulkStats = detail::computeHalfChainStats(bulkHalves);
    const double rhatBulk = detail::rhatFromStats(bulkStats, M, Nh);
    const double bulkESS  = detail::essFromHalves(bulkHalves, bulkStats);

    // --- Tail-Rhat : repliement autour de la médiane globale, puis rang-normalisation
    //     (spécifique à Rhat — ne sert PAS pour l'ESS) ---
    std::vector<double> folded(S);
    for (int i = 0; i < S; ++i)
        folded[i] = std::abs(pooled[i] - median);
    const std::vector<double> zTail = rankNormalize(folded);
    std::vector<std::vector<double>> tailRhatHalves(M, std::vector<double>(Nh));
    for (int i = 0, off = 0; i < M; ++i)
        for (int j = 0; j < Nh; ++j)
            tailRhatHalves[i][j] = zTail[off++];

    const detail::HalfChainStats tailRhatStats = detail::computeHalfChainStats(tailRhatHalves);
    const double rhatTail = detail::rhatFromStats(tailRhatStats, M, Nh);

    // --- Tail-ESS : indicatrices 0/1 aux quantiles 5 %/95 %, construites directement
    //     à partir des demi-chaînes déjà splittées (spécifique à l'ESS — transformation
    //     différente du repliement ci-dessus, cf. Vehtari et al. 2021, §4.3) ---
    std::vector<std::vector<double>> indicator05Halves(M, std::vector<double>(Nh));
    std::vector<std::vector<double>> indicator95Halves(M, std::vector<double>(Nh));
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < Nh; ++j) {
            indicator05Halves[i][j] = (halves[i][j] <= q05) ? 1. : 0.;
            indicator95Halves[i][j] = (halves[i][j] <= q95) ? 1. : 0.;
        }
    }
    const detail::HalfChainStats stats05 = detail::computeHalfChainStats(indicator05Halves);
    const detail::HalfChainStats stats95 = detail::computeHalfChainStats(indicator95Halves);
    const double essQ05 = detail::essFromHalves(indicator05Halves, stats05);
    const double essQ95 = detail::essFromHalves(indicator95Halves, stats95);

    RhatEssResult result;
    result.rHat    = std::max(rhatBulk, rhatTail);
    result.bulkESS = bulkESS;
    result.tailESS = std::min(essQ05, essQ95);
    return result;
}


// ============================================================================
// Hypothèses retenues (voir échange) :
//  - Combinaison R-hat / ESS : "le pire des deux" -> le statut global est le
//    plus mauvais des deux sous-statuts (calculés indépendamment avec une
//    logique miroir : eBad si non-fini ou sous le seuil d'alerte, eWarning si
//    en zone limite mais peu de variables concernées, eGood sinon).
//  - Seuils ESS symétriques à ceux de R-hat : essGoodThreshold (ESS jugé bon
//    au-dessus), essWarningThreshold (ESS critique en dessous). La tolérance
//    de fraction (essMaxFractionBelowGood) réutilise maxFractionAboveGood par
//    défaut (-1) pour ne pas multiplier les paramètres, mais peut être
//    surchargée indépendamment.
//  - essValues est optionnel (valeur par défaut : vecteur vide) => tous les
//    appels existants qui ne connaissent pas encore l'ESS continuent de
//    compiler et de se comporter EXACTEMENT comme avant (statut piloté par
//    R-hat seul, hasEss=false).
//
// À adapter si besoin : noms des nouveaux champs de ConvergenceSummary,
// libellés, décimales affichées, etc.
// ============================================================================

// --- Ajouts à faire dans la définition de ConvergenceSummary (header) -----
//
// struct ConvergenceSummary
// {
//     ... champs existants inchangés (nVariables, status, label, maxRHat,
//         meanRHat, nAboveGoodThreshold, fractionAboveGoodThreshold) ...
//
//     // --- Nouveaux champs ESS ---
//     bool hasEss = false;                       // ESS fourni et exploitable
//     size_t nEssVariables = 0;
//     double minESS = std::numeric_limits<double>::quiet_NaN();
//     double meanESS = std::numeric_limits<double>::quiet_NaN();
//     size_t nBelowGoodEssThreshold = 0;
//     double fractionBelowGoodEssThreshold = 0.;
//
//     // --- Sous-statuts (utiles pour afficher un badge par métrique dans l'UI) ---
//     ConvergenceStatus rHatStatus = ConvergenceStatus::eGood;
//     ConvergenceStatus essStatus  = ConvergenceStatus::eGood;
// };

namespace {

// Combine deux sous-statuts en gardant le plus mauvais des deux, sans
// dépendre de l'ordre numérique des valeurs de l'enum.
ConvergenceStatus worseStatus(ConvergenceStatus a, ConvergenceStatus b)
{
    if (a == ConvergenceStatus::eBad || b == ConvergenceStatus::eBad)
        return ConvergenceStatus::eBad;
    if (a == ConvergenceStatus::eWarning || b == ConvergenceStatus::eWarning)
        return ConvergenceStatus::eWarning;
    return ConvergenceStatus::eGood;
}

} // namespace

ConvergenceSummary computeConvergenceSummary(const std::vector<double>& rHatValues,
                                             const std::vector<double>& essValues,
                                             double goodThreshold,
                                             double warningThreshold,
                                             double maxFractionAboveGood,

                                             double essGoodThreshold,
                                             double essWarningThreshold,
                                             double essMaxFractionBelowGood,
                                             size_t minVariablesForBad)
{
    // minVariablesForBad : safety‑check for very small models. With N = 4
    // variables, a single borderline variable already accounts for 25 % — far
    // above a fraction tolerance that is meant for large N (e.g. maxFractionAboveGood
    // = 5 %). Without this guard, a 4‑variable model where only one variable is
    // “just borderline” (⚠ Insufficient, not individually “Not converged”) would
    // incorrectly be classified as globally “Not converged”. Therefore we require
    // that AT LEAST minVariablesForBad variables be affected before the fraction
    // can push the status to eBad; below that we stay in eWarning regardless of the
    // fraction.
    ConvergenceSummary summary;
    summary.nVariables = rHatValues.size();

    if (rHatValues.empty()) {
        summary.status = ConvergenceStatus::eBad;
        summary.label = QObject::tr("No variables to evaluate");
        return summary;
    }

    // ------------------------------------------------------------------
    // R‑hat (logic unchanged from the previous version)
    // ------------------------------------------------------------------

    // Isolate non‑finite values (NaN/Inf), which can occur for a degenerate
    // variable (zero intra‑chain variance, quasi‑fixed parameter, etc.).
    // They are counted as “out‑of‑threshold” but excluded from the max/mean
    // calculations.
    std::vector<double> finiteValues;
    finiteValues.reserve(rHatValues.size());
    size_t nNonFinite = 0;

    for (double r : rHatValues) {
        if (std::isfinite(r))
            finiteValues.push_back(r);
        else
            ++nNonFinite;
    }

    if (finiteValues.empty()) {
        summary.status = ConvergenceStatus::eBad;
        summary.label = QObject::tr("R\xCC\x82 not computable for all variables");
        return summary;
    }

    summary.maxRHat = *std::max_element(finiteValues.begin(), finiteValues.end());
    summary.meanRHat = std::accumulate(finiteValues.begin(), finiteValues.end(), 0.)
                       / static_cast<double>(finiteValues.size());

    summary.nAboveGoodThreshold = nNonFinite + static_cast<size_t>(
                                      std::count_if(finiteValues.begin(), finiteValues.end(),
                                                    [goodThreshold](double r) { return r >= goodThreshold; }));

    summary.fractionAboveGoodThreshold =
        static_cast<double>(summary.nAboveGoodThreshold) / static_cast<double>(summary.nVariables);

    const bool anyNonFiniteOrAboveWarningRHat = (nNonFinite > 0) ||
                                                std::any_of(finiteValues.begin(), finiteValues.end(),
                                                            [warningThreshold](double r) { return r >= warningThreshold; });

    ConvergenceStatus rHatStatus;
    if (anyNonFiniteOrAboveWarningRHat) {
        rHatStatus = ConvergenceStatus::eBad;

    } else if (summary.maxRHat >= goodThreshold) {
        // max R‑hat lies in the “borderline” zone (between goodThreshold and
        // warningThreshold): we only switch to Bad if too many variables are
        // affected for the deviation to be attributable to sampling noise — AND
        // there are at least minVariablesForBad of them (small‑N safety‑check,
        // see comment above).
        rHatStatus = (summary.nAboveGoodThreshold < minVariablesForBad ||
                      summary.fractionAboveGoodThreshold <= maxFractionAboveGood)
                         ? ConvergenceStatus::eWarning
                         : ConvergenceStatus::eBad;

    } else {
        rHatStatus = ConvergenceStatus::eGood;
    }
    summary.rHatStatus = rHatStatus;

    // ------------------------------------------------------------------
    // ESS (new) — mirror logic of R‑hat, except that for ESS a LOW value is
    // problematic (the opposite of R‑hat).
    // ------------------------------------------------------------------

    summary.hasEss = !essValues.empty();
    ConvergenceStatus essStatus = ConvergenceStatus::eGood;
    size_t nNonFiniteEss = 0; // propagated to the eBad label

    if (summary.hasEss) {
        summary.nEssVariables = essValues.size();

        std::vector<double> finiteEss;
        finiteEss.reserve(essValues.size());

        for (double ess : essValues) {
            if (std::isfinite(ess))
                finiteEss.push_back(ess);
            else
                ++nNonFiniteEss;
        }

        if (finiteEss.empty()) {
            // No calculable ESS: treat it as critical, just like a completely
            // non‑computable R‑hat.
            summary.minESS = std::numeric_limits<double>::quiet_NaN();
            summary.meanESS = std::numeric_limits<double>::quiet_NaN();
            summary.nBelowGoodEssThreshold = summary.nEssVariables;
            summary.fractionBelowGoodEssThreshold = 1.;
            essStatus = ConvergenceStatus::eBad;

        } else {
            summary.minESS = *std::min_element(finiteEss.begin(), finiteEss.end());
            summary.meanESS = std::accumulate(finiteEss.begin(), finiteEss.end(), 0.)
                              / static_cast<double>(finiteEss.size());

            summary.nBelowGoodEssThreshold = nNonFiniteEss + static_cast<size_t>(
                                                 std::count_if(finiteEss.begin(), finiteEss.end(),
                                                               [essGoodThreshold](double ess) { return ess < essGoodThreshold; }));

            summary.fractionBelowGoodEssThreshold =
                static_cast<double>(summary.nBelowGoodEssThreshold) / static_cast<double>(summary.nEssVariables);

            const bool anyNonFiniteOrBelowWarningEss = (nNonFiniteEss > 0) ||
                                                       std::any_of(finiteEss.begin(), finiteEss.end(),
                                                                   [essWarningThreshold](double ess) { return ess < essWarningThreshold; });

            // essMaxFractionBelowGood < 0 => no dedicated value supplied:
            // fall back to the tolerance defined for R‑hat.
            const double effectiveEssMaxFraction =
                (essMaxFractionBelowGood >= 0.) ? essMaxFractionBelowGood : maxFractionAboveGood;

            if (anyNonFiniteOrBelowWarningEss) {
                essStatus = ConvergenceStatus::eBad;

            } else if (summary.minESS < essGoodThreshold) {
                essStatus = (summary.nBelowGoodEssThreshold < minVariablesForBad ||
                             summary.fractionBelowGoodEssThreshold <= effectiveEssMaxFraction)
                                ? ConvergenceStatus::eWarning
                                : ConvergenceStatus::eBad;

            } else {
                essStatus = ConvergenceStatus::eGood;
            }
        }
    }
    summary.essStatus = essStatus;

    // ------------------------------------------------------------------
    // Global status = the worst of the two metrics
    // ------------------------------------------------------------------

    summary.status = summary.hasEss ? worseStatus(rHatStatus, essStatus) : rHatStatus;

    // ------------------------------------------------------------------
    // Label
    // ------------------------------------------------------------------

    switch (summary.status) {
    case ConvergenceStatus::eGood:
        summary.label = summary.hasEss
                            ? QObject::tr("Satisfactory convergence (max R\xCC\x82 = %1, min ESS = %2)")
                                  .arg(summary.maxRHat, 0, 'f', 4)
                                  .arg(summary.minESS, 0, 'f', 0)
                            : QObject::tr("Satisfactory convergence (maxR\xCC\x82 = %1)")
                                  .arg(summary.maxRHat, 0, 'f', 4);
        break;

    case ConvergenceStatus::eWarning: {
        QStringList reasons;
        if (rHatStatus != ConvergenceStatus::eGood) {
            reasons << QObject::tr("max R\xCC\x82 = %1, %2 % of variables ≥ %3")
                           .arg(summary.maxRHat, 0, 'f', 4)
                           .arg(summary.fractionAboveGoodThreshold * 100., 0, 'f', 2)
                           .arg(goodThreshold, 0, 'f', 2);
        }
        if (summary.hasEss && essStatus != ConvergenceStatus::eGood) {
            reasons << QObject::tr("min ESS = %1, %2 % of variables &lt; %3")
                           .arg(summary.minESS, 0, 'f', 0)
                           .arg(summary.fractionBelowGoodEssThreshold * 100., 0, 'f', 2)
                           .arg(essGoodThreshold, 0, 'f', 0);
        }
        summary.label = QObject::tr("Questionable convergence (%1)")
                            .arg(reasons.join(QStringLiteral(" ; ")));
        break;
    }

    case ConvergenceStatus::eBad: {
        QStringList reasons;
        reasons << QObject::tr("max R\xCC\x82 = %1%2")
                       .arg(summary.maxRHat, 0, 'f', 4)
                       .arg(nNonFinite > 0
                                ? QObject::tr(", %1 variable(s) with non‑computable R\xCC\x82")
                                      .arg(nNonFinite)
                                : QString());
        if (summary.hasEss) {
            reasons << QObject::tr("min ESS = %1%2")
            .arg(std::isfinite(summary.minESS) ? QString::number(summary.minESS, 'f', 0)
                                               : QObject::tr("non‑computable"))
                .arg(nNonFiniteEss > 0
                         ? QObject::tr(", %1 variable(s) with non‑computable ESS")
                               .arg(nNonFiniteEss)
                         : QString());
        }
        summary.label = QObject::tr("Not converged (%1)")
                            .arg(reasons.join(QStringLiteral(" ; ")));
        break;
    }
    }

    return summary;
}


} // namespace MCMCDiagnostic
