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

#include "MetropolisVariable.h"
#include "Generator.h" // pour test ucv

#include "StdUtilities.h"
#include "QtUtilities.h"
#include "Functions.h"
#include "DateUtils.h"

#include "fftw3.h"

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
    mName("Empty MetropolisVariable"),
    mAcceptedStateCountByChain(),
    mX (0.0),
    mBurnAdaptTrace(std::make_shared<std::vector<double>>()),
    mAllAcquiredTrace(std::make_shared<std::vector<double>>()),
    is_curve_filtering(false),

    mDisplayAcquiredTrace(std::make_shared<std::vector<double>>()),
    mFormatedBurnAdaptTrace(std::make_shared<std::vector<double>>()),
    mFormatedAcquiredTrace(std::make_shared<std::vector<double>>()),
    mSupport (eR),
    mFormat (DateUtils::eNumeric),
    mFormatedKDE(),

    mChainsKDE(),
    mCorrelations(),
    mFormatedHPD(),
    mRawHPDintervals(),
    mExactCredibilityThreshold (0.0),
    mResults(),
    mChainsResults(),
    mfftLenUsed (-1),
    mBandwidthUsed (-1.0),
    mThresholdUsed (-1.0),
    mtminUsed (0.0),
    mtmaxUsed (0.0)
// Learning Prior
   /* mBurnInPriorTrace(std::make_shared<std::vector<double>>()),
    mEmpiricalPrior(),
    mEmpiricalPriorReady(false)*/
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
    // Learning Prior
   /* mBurnInPriorTrace = std::make_shared<std::vector<double>>(*origin.mBurnInPriorTrace);
    mEmpiricalPrior = origin.mEmpiricalPrior;
    mEmpiricalPriorReady = origin.mEmpiricalPriorReady; */

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

    mfftLenUsed = origin.mBandwidthUsed;
    mBandwidthUsed = origin.mBandwidthUsed;
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

    mfftLenUsed = origin.mBandwidthUsed;
    mBandwidthUsed = origin.mBandwidthUsed;
    mThresholdUsed = origin.mThresholdUsed;

    mtminUsed = origin.mtminUsed;
    mtmaxUsed = origin.mtmaxUsed;

    // Learning Prior
    /*mBurnInPriorTrace = std::make_shared<std::vector<double>>(*origin.mBurnInPriorTrace);
    mEmpiricalPrior = origin.mEmpiricalPrior;
    mEmpiricalPriorReady = origin.mEmpiricalPriorReady; */


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
        mBandwidthUsed = origin.mBandwidthUsed;
        mThresholdUsed = origin.mThresholdUsed;

        mtminUsed = origin.mtminUsed;
        mtmaxUsed = origin.mtmaxUsed;

        // Learning Prior
        /*if (mBurnInPriorTrace) {
            mBurnInPriorTrace = std::move(origin.mBurnInPriorTrace);
            mEmpiricalPrior = std::move(origin.mEmpiricalPrior);
            mEmpiricalPriorReady = origin.mEmpiricalPriorReady;
        }*/

        // Laisser l'objet source dans un état valide
        // (par exemple, réinitialiser les pointeurs ou les ressources)
        origin.mBurnAdaptTrace.reset();
        origin.mFormatedBurnAdaptTrace.reset();

        origin.mAllAcquiredTrace.reset();
        origin.mDisplayAcquiredTrace.reset();
        origin.mFormatedAcquiredTrace.reset();

        //origin.mBurnInPriorTrace.reset();

        // Réinitialiser d'autres membres si nécessaire
    }

    return *this;
}



void MetropolisVariable::clear()
{
    mBurnAdaptTrace->clear();
    mFormatedBurnAdaptTrace->clear();
    mAllAcquiredTrace->clear();
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
    // Learning Prior
    /*if (mBurnInPriorTrace) mBurnInPriorTrace->clear();

    mEmpiricalPrior.clear();
    mEmpiricalPriorReady = false;*/

}

void MetropolisVariable::shrink_to_fit() noexcept
{
    mBurnAdaptTrace->shrink_to_fit();
    mFormatedBurnAdaptTrace->shrink_to_fit();
    mAllAcquiredTrace->shrink_to_fit();
    mDisplayAcquiredTrace->shrink_to_fit();
    mFormatedAcquiredTrace->shrink_to_fit();

    mChainsKDE.shrink_to_fit();
    mCorrelations.shrink_to_fit();
    mRawHPDintervals.shrink_to_fit();
    mChainsResults.shrink_to_fit();

    // Learning Prior
    //mBurnInPriorTrace->shrink_to_fit();
    //mEmpiricalPrior.shrink_to_fit();
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
    // 2️⃣  Ré‑initialiser **seulement** la partie FunctionStat de chaque
    //     élément de mChainsResults (on garde traceAnalysis)
    // -----------------------------------------------------------------
    for (auto& chainResult : mChainsResults)
    {
        // ----- fonction : on met les champs à NaN pour signifier « non calculé » -----
        chainResult.funcAnalysis.max  = std::numeric_limits<type_data>::quiet_NaN();
        chainResult.funcAnalysis.mode = std::numeric_limits<type_data>::quiet_NaN();
        chainResult.funcAnalysis.mean = std::numeric_limits<type_data>::quiet_NaN();
        chainResult.funcAnalysis.std  = std::numeric_limits<type_data>::quiet_NaN();

        chainResult.funcAnalysis.quartiles.Q1 = std::numeric_limits<type_data>::quiet_NaN();
        chainResult.funcAnalysis.quartiles.Q2 = std::numeric_limits<type_data>::quiet_NaN();
        chainResult.funcAnalysis.quartiles.Q3 = std::numeric_limits<type_data>::quiet_NaN();

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

        // Position sur la grille, clampée dans [0, numPts-1]
        const double idx         = (dataSrc[s] - a) / delta;
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
/**
  @param[in] bandwidth corresponds to the bandwidth factor
  @param[in] dataSrc is the trace of the raw data
  @brief the FFTW function transform the area such that the area output is the area input multiplied by fftLen. So we have to corret it.
  The result is migth be not with regular step between value.
  @article {sheather_density_2004,
    title = {Density {Estimation}},
    volume = {19},
    issn = {0883-4237},
    url = {https://projecteuclid.org/journals/statistical-science/volume-19/issue-4/Density-Estimation/10.1214/088342304000000297.full},
    doi = {10.1214/088342304000000297},
    abstract = {This paper provides a practical description of density estimation based on kernel methods. An important aim is to encourage practicing statisticians to apply these methods to data. As such, reference is made to implementations of these methods in R, S-PLUS and SAS},
    number = {4},
    urldate = {2023-10-09},
    journal = {Statistical Science},
    author = {Sheather, Simon J.},
    month = nov,
    year = {2004},
 }

 **/

std::map<double, double> MetropolisVariable::generateKDE(const std::vector<double>& dataSrc, const int fftLen, const double coef_bandwidth, const double tmin, const double tmax)
{

    mfftLenUsed = fftLen;
    mBandwidthUsed = coef_bandwidth;
    mtmaxUsed = tmax;
    mtminUsed = tmin;

    std::map<double, double> result;

    auto N = dataSrc.size();
    if (N == 0) {
        return result;
    }

    if (N == 1) {
        // value. It can appear with a fixed variable
        result.emplace(dataSrc.at(0), 1.) ;
        return result;
    }


    double sigma = std_unbiais_Knuth(dataSrc);

    /* In the case of Vg and Vt (sigma_ti), there may be very large values that pull the mean.
    * It is preferable in this case, to evaluate an equivalent of the standard deviation using the quantiles at 15.85%, in the Gaussian case.
    */


    /* code version <3.2.6
    * if (mSupport == eRp || mSupport== eRpStar) {
        const Quartiles quartiles = quantilesType(dataSrc, 8, 0.1585);
        sigma = std::min(sigma, (quartiles.Q3 - quartiles.Q1)/1.34);
    }*/

    // Density Estimation - Simon J. Sheather, Statistical Science 2004, Vol. 19, No. 4, 588–597 DOI 10.1214/088342304000000297
    // Silverman’s rule of thumb. It is given by hSROT = 0.9An−1/5, where A = min{sample standard deviation, (sample interquartile range)/1.34}
    const Quartiles quartiles = quantilesType(dataSrc, 8, 0.1585);
    sigma = std::min(sigma, (quartiles.Q3 - quartiles.Q1)/1.34);


    if (sigma <= 0) {
        // if sigma is null and there are several values, it means: this is a constant value
        // This can occur at the Begin or End of a Phase with a Bound.
        result.emplace(dataSrc.at(0), 1.) ;
        qDebug()<<"[MetropolisVariable::generateKDE] Constant value = "<< dataSrc.at(0) << QString::fromStdString(mName);

        return result;
    }

//std::cout << '\n' << "name  = " << mName << '\n';


   /* const double h_silver = 1.06 * sigma * std::pow(static_cast<double>(N), -0.2);
    std::cout << "Silverman bandwidth = " << h_silver << '\n';

    const double h_opt = brent_minimize(dataSrc,  h_silver/30,  h_silver*3); // donne le mêm cacul que R pour bw.ucv
    double bw_equi = h_opt / (sigma * pow(static_cast<double>(N), -0.2) );
    std::cout << "Bandwidth (brent_minimize) = " << h_opt << " coef equivalent=" << bw_equi <<'\n';

    const double h_nai = bw_ucv_gaussian(dataSrc, h_silver/10,  h_silver*3, 50 );
    double coef_Nai_equi = h_nai / (sigma * pow(static_cast<double>(N), -0.2) );
    std::cout << "(bw_ucv_gaussian) Bandwidth  = " << h_nai<< " coef equivalent=" << coef_Nai_equi  << '\n';

*/
    double h = coef_bandwidth * sigma * pow(static_cast<double>(N), -0.2);
 //   std::cout << "ChronoModel Bandwidth  = " << h << '\n';
   // h=h_opt;


   // double h_sj_dpi = bw_SJ_dpi(dataSrc);   // rapide
  //  double h_sj_ste = bw_SJ_ste(dataSrc);   // plus précis
    //h = h_sj_ste;
   // std::cout << "SJ-DPI = " << h_sj_dpi << "\n";
    //std::cout << "SJ-STE = " << h_sj_ste << "\n";
  //  double coef_Si_equi = h_sj_ste / (sigma * pow(static_cast<double>(N), -0.2) );

   // std::cout << mName <<" (h_sj_ste) Bandwidth  = " << h_sj_ste<< " coef equivalent=" << coef_Si_equi  << '\n';

    //h=h_opt;

    const double a = range_min_value(dataSrc) - 4. * h;
    const double b = range_max_value(dataSrc) + 4. * h;

    // Préparation des buffers avec gestion RAII
    std::unique_ptr<double, decltype(&fftw_free)> input(
        static_cast<double*>(fftw_malloc(fftLen * sizeof(double))),
        fftw_free
        );
    std::unique_ptr<double, decltype(&fftw_free)> output(
        static_cast<double*>(fftw_malloc(2 * (fftLen / 2 + 1) * sizeof(double))),
        fftw_free
        );

    if (!input || !output) {
        throw std::runtime_error("Memory allocation failed");
    }

    // Génération du buffer
    generateBufferForHisto(input.get(), dataSrc, fftLen, a, b);

    // Plans FFTW avec gestion automatique
    fftw_plan plan_forward = fftw_plan_dft_r2c_1d(fftLen, input.get(),
                                                  reinterpret_cast<fftw_complex*>(output.get()), FFTW_ESTIMATE);

    // Exécution FFT
    fftw_execute(plan_forward);

    // Filtrage spectral
    const int outputSize = 2 * (fftLen / 2 + 1);
    for (int i = 0; i < outputSize / 2; ++i) {
        const double s = 2. * M_PI * i / (b - a);
        const double factor = std::exp(-0.5 * s * s * h * h);
        output.get()[2*i] *= factor;
        output.get()[2*i + 1] *= factor;
    }

    // Transformation inverse
    fftw_plan plan_backward = fftw_plan_dft_c2r_1d(fftLen,
                                                   reinterpret_cast<fftw_complex*>(output.get()), input.get(), FFTW_ESTIMATE);

    fftw_execute(plan_backward);

    // Nettoyage des plans
    fftw_destroy_plan(plan_forward);
    fftw_destroy_plan(plan_backward);

    // Calcul des bornes selon le support
    double tBegin = a, tEnd = b;
    switch(mSupport) {
    case eRp:
    case eRpStar: // on R+*
        tBegin = 0.0;
        break;
    case eRm:
    case eRmStar: // on R-*
        tEnd = 0.0;
        break;
    case eBounded: // on [tmin;tmax]
        tBegin = tmin;
        tEnd = tmax;
        break;
    case eR:
        break;
    }

    // Construction du résultat

    // ✅ Correct pour FFT (intervalle semi-ouvert [a, b[)
    const double delta = (b - a) / fftLen;  // pas fftLen - 1
    for (int i = 0; i < fftLen; ++i) {
        const double t = a + static_cast<double>(i) * delta;
        result[t] = std::max(0.0, input.get()[i]);
    }

    // normalisation
    result = getMapDataInRange(result, tBegin, tEnd);
    result = equal_areas(result, 1.0);

    return result;
}


void MetropolisVariable::generateKDE(const std::vector<ChainSpecs> &chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    //Q_ASSERT_X(!mFormatedBurnAdaptTrace->isEmpty(), "[MetropolisVariable::generateKDE]", "mFormatedBurnAdaptTrace.isEmpty()");
    if (mFormatedBurnAdaptTrace == nullptr || mFormatedBurnAdaptTrace->size() == 0)
        return;
    //const std::vector<double> &subFullTrace = fullRunFormatedTrace(chains);
    const std::vector<double>& trace = *mFormatedAcquiredTrace;
    mFormatedKDE = generateKDE(trace, fftLen, bandwidth, tmin, tmax);

    mChainsKDE.clear();
    for (size_t i = 0; i<chains.size(); ++i) {
        //const std::vector<double> &subTrace = runFormatedTraceForChain(chains, i);
        const std::vector<double> &subTrace = formatedAcquiredTraceforChain(chains, i);
        if (!subTrace.empty()) {
            mChainsKDE.push_back(generateKDE(subTrace, fftLen, bandwidth, tmin, tmax) );
        }
    }
}


/**
 * @brief Builds the CDF from mEmpiricalPrior by trapezoidal integration.
 *        Must be called once after buildEmpiricalPrior().
 *
 * @details
 * @f[
 *   F(x_k) = \sum_{i=0}^{k-1} \frac{f(x_i) + f(x_{i+1})}{2} \cdot (x_{i+1} - x_i)
 * @f]
 * Then normalized so that F(x_last) = 1.
 */
/*void MetropolisVariable::buildEmpiricalCDF()
{
    if (mPriorX.empty() || mPriorY.empty()) return;

    const int n = static_cast<int>(mPriorX.size());

    mPriorCDF_x.resize(n);
    mPriorCDF_y.resize(n);

    mPriorCDF_x[0] = mPriorX[0];
    mPriorCDF_y[0] = 0.0;

    // Intégration trapézoïdale
    for (int i = 1; i < n; ++i) {
        const double dx   = mPriorX[i] - mPriorX[i - 1];
        const double area = 0.5 * (mPriorY[i] + mPriorY[i - 1]) * dx;

        mPriorCDF_x[i] = mPriorX[i];
        mPriorCDF_y[i] = mPriorCDF_y[i - 1] + area;
    }

    // Normalisation : garantit CDF(x_last) == 1 exactement
    const double total = mPriorCDF_y.back();
    if (total > 0.0) {
        for (double& v : mPriorCDF_y)
            v /= total;
    }

    mPriorCDF_y.front() = 0.0;   // sécurité numérique
    mPriorCDF_y.back()  = 1.0;   // sécurité numérique

    mEmpiricalCDFReady = true;
}


void MetropolisVariable::buildEmpiricalPrior(const int fftLen,
                                             const double bandwidth,
                                             const double tmin,
                                             const double tmax)
{
    if (!mBurnInPriorTrace || mBurnInPriorTrace->empty()) return;

    auto histo = generateKDE(*mBurnInPriorTrace, fftLen, bandwidth, tmin, tmax);
    if (histo.empty()) return;

    mPriorX.clear();
    mPriorY.clear();
    mPriorX.reserve(histo.size());
    mPriorY.reserve(histo.size());

    for (auto& [x, y] : histo) {
        mPriorX.push_back(x);
        mPriorY.push_back(std::max(y, 0.0));
    }

    mEmpiricalPriorReady = true;

    // ✅ Construction immédiate de la CDF
    buildEmpiricalCDF();

    // Libération mémoire burn-in
    mBurnInPriorTrace->clear();
    mBurnInPriorTrace->shrink_to_fit();
}

// Évaluation de l'a priori en un point x (interpolation linéaire)
double MetropolisVariable::evalEmpiricalPrior(const double x) const
{
    if (!mEmpiricalPriorReady || mEmpiricalPrior.empty())
        return 1.0; // prior plat par défaut

    auto it = mEmpiricalPrior.lower_bound(x);

    if (it == mEmpiricalPrior.end())   return std::prev(it)->second;
    if (it == mEmpiricalPrior.begin()) return it->second;

    // Interpolation linéaire entre les deux points encadrants
    auto prev = std::prev(it);
    double t = (x - prev->first) / (it->first - prev->first);
    return prev->second + t * (it->second - prev->second);
}
*/

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
/*
double MetropolisVariable::sampleFromEmpiricalPrior(const double min,
                                                    const double max) const
{
    if (!mEmpiricalCDFReady || mPriorCDF_x.empty())
        return 0.5 * (min + max);   // fallback : milieu de l'intervalle

    // ----------------------------------------------------------------
    // 1. Évaluation de F(min) et F(max) par interpolation dans la CDF
    // ----------------------------------------------------------------
    auto evalCDF = [&](double xq) -> double
    {
        if (xq <= mPriorCDF_x.front()) return 0.0;
        if (xq >= mPriorCDF_x.back())  return 1.0;

        const auto it = std::lower_bound(mPriorCDF_x.begin(),
                                         mPriorCDF_x.end(), xq);
        const int k = static_cast<int>(
                          std::distance(mPriorCDF_x.begin(), it)) - 1;

        const double dx = mPriorCDF_x[k + 1] - mPriorCDF_x[k];
        if (dx < 1e-15) return mPriorCDF_y[k];

        const double t = (xq - mPriorCDF_x[k]) / dx;
        return mPriorCDF_y[k] + t * (mPriorCDF_y[k + 1] - mPriorCDF_y[k]);
    };

    const double cdf_min = evalCDF(min);
    const double cdf_max = evalCDF(max);

    // ----------------------------------------------------------------
    // 2. Masse disponible dans [min, max]
    //    Si nulle (prior nul sur cet intervalle) → milieu
    // ----------------------------------------------------------------
    const double mass = cdf_max - cdf_min;
    if (mass < 1e-15)
        return 0.5 * (min + max);

    // ----------------------------------------------------------------
    // 3. Tirage uniforme rescalé sur [F(min), F(max)]
    // ----------------------------------------------------------------
    const double u  = Generator::randomUniform();
    const double u_ = cdf_min + u * mass;     // u' ∈ [F(min), F(max)]

    // ----------------------------------------------------------------
    // 4. Inversion CDF standard sur u'
    // ----------------------------------------------------------------
    const auto it = std::lower_bound(mPriorCDF_y.begin(),
                                     mPriorCDF_y.end(), u_);

    if (it == mPriorCDF_y.begin()) return min;
    if (it == mPriorCDF_y.end())   return max;

    const int k = static_cast<int>(
                      std::distance(mPriorCDF_y.begin(), it)) - 1;

    const double y0 = mPriorCDF_y[k];
    const double y1 = mPriorCDF_y[k + 1];
    const double dy = y1 - y0;

    if (dy < 1e-15)
        return mPriorCDF_x[k];

    const double t = (u_ - y0) / dy;
    const double x = mPriorCDF_x[k] + t * (mPriorCDF_x[k + 1] - mPriorCDF_x[k]);

    // ----------------------------------------------------------------
    // 5. Clamp de sécurité (erreurs d'interpolation aux bords)
    // ----------------------------------------------------------------
    return std::clamp(x, min, max);
}
*/

void MetropolisVariable::memoHistoParameter(const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    mfftLenUsed = fftLen;
    mBandwidthUsed = bandwidth;
    mtminUsed = tmin;
    mtmaxUsed = tmax;
}

bool MetropolisVariable::HistoWithParameter(const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
   return ((mfftLenUsed == fftLen) &&  (mBandwidthUsed == bandwidth) && (mtminUsed == tmin) && (mtmaxUsed == tmax) ? true: false);
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

void MetropolisVariable::generateCredibility(const std::vector<ChainSpecs> &chains, double threshold)
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
    mResults.funcAnalysis = analyseFunction(mFormatedKDE);
    const std::vector<double>& trace = *mFormatedAcquiredTrace;
    mResults.traceAnalysis = traceStatistic(trace);   // on garde le trace global

    // ----- Résultats *par chaîne* (densité) -----
    // 1️⃣  S’assurer que le vecteur possède exactement le bon nombre d’éléments
    if (mChainsResults.size() != mChainsKDE.size())
        mChainsResults.resize(mChainsKDE.size());   // crée des objets « vide »

    // 2️⃣  Remplir uniquement le champ funcAnalysis (densité)
    for (size_t i = 0; i < mChainsKDE.size(); ++i) {
        mChainsResults[i].funcAnalysis = analyseFunction(mChainsKDE[i]);
        // on ne touche pas à traceAnalysis → il garde la valeur déjà présente
    }
}
/*
 * void MetropolisVariable::generateNumericalResults(const std::vector<ChainSpecs> &chains)
{
    // Results for chain concatenation
    if (mFormatedKDE.empty())
        return;
    mResults.funcAnalysis = analyseFunction(mFormatedKDE);
    const std::vector<double>& trace = *mFormatedAcquiredTrace;
    mResults.traceAnalysis = traceStatistic(trace); // fullRunFormatedTrace is the formated Traces

    // Results for individual chains
    mChainsResults.clear();

    for (size_t i = 0; i < mChainsKDE.size(); ++i) {
        DensityAnalysis result;
        result.funcAnalysis = analyseFunction(mChainsKDE[i]); // useless
        result.traceAnalysis = traceStatistic(runFormatedTraceForChain(chains, i)); // only to compute quartiles
        mChainsResults.push_back(result);
    }
}
 */
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

    mResults.traceAnalysis = traceStatistic(trace);   // analyse du trace global
    // ----- Résultats *par chaîne* (trace) -----
    // 1️⃣  S’assurer que le vecteur possède exactement le bon nombre d’éléments
    if (mChainsResults.size() != mChainsKDE.size())
        mChainsResults.resize(mChainsKDE.size());   // crée des objets « vide »

    // 2️⃣  Remplir uniquement le champ traceAnalysis (trace)
    for (size_t i = 0; i < mChainsKDE.size(); ++i) {
        mChainsResults[i].traceAnalysis =
            traceStatistic(runFormatedTraceForChain(chains, i));
        // on ne touche pas à funcAnalysis → il garde la valeur déjà présente
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

    QString result = densityAnalysisToString(mResults) + "<br>";


    result += "<i>"+ QObject::tr("Probabilities") + " </i><br>";

    // the mFormatedCredibility is already in the time scale, we don't need to convert
    if (mFormatedCredibility != std::pair<double, double>(1, -1))
        result += QObject::tr("Credibility Interval") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(mExactCredibilityThreshold * 100.),
                                                                                                 stringForLocal(mFormatedCredibility.first),
                                                                                                 stringForLocal(mFormatedCredibility.second),
                                                                                                 unit) + "<br>";
    if (!mRawHPDintervals.isEmpty()) {
        const QList<QPair<double, QPair<double, double>>>& intervals = mRawHPDintervals;

        const double total_thresh = std::accumulate(intervals.begin(), intervals.end(), 0., [](double sum, auto i) {return sum + i.first;});


        result += QObject::tr("HPD Region ( %1 %) :").arg(stringForLocal(total_thresh * 100.));
        if (mFormat == DateUtils::eNumeric) {

            for (auto&& interval : intervals) {
                const QString str_rate = stringForLocal(interval.first*100.);
                const QString str_tmin = stringForLocal(interval.second.first);
                const QString str_tmax = stringForLocal(interval.second.second);
                result +=  QString(" [ %2 ; %3 ] (%4 %) ").arg(str_tmin, str_tmax, str_rate);
            }

        } else if (DateUtils::is_date(mFormat)) {
            for (auto&& interval : intervals) {
                const QString str_rate = stringForLocal(interval.first*100.);
                const QString str_tmin = stringForLocal(DateUtils::convertToAppSettingsFormat(interval.second.first));
                const QString str_tmax = stringForLocal(DateUtils::convertToAppSettingsFormat(interval.second.second));
                result +=  QString(" [ %2 ; %3 ] (%4 %) ").arg(str_tmin, str_tmax, str_rate);
            }

        } else {
            for (auto interval = intervals.crbegin(); interval != intervals.crend(); interval++) {
                const QString str_rate = stringForLocal(interval->first*100.);
                const QString str_tmin = DateUtils::convertToAppSettingsFormatStr(interval->second.second);
                const QString str_tmax = DateUtils::convertToAppSettingsFormatStr(interval->second.first);
                result +=  QString(" [ %2 ; %3 ] (%4 %) ").arg(str_tmin, str_tmax, str_rate);
            }
        }
        result += unit + "<br>";
        //result += QObject::tr("Density Step : %1").arg(stringForLocal(std::abs(mFormatedKDE.lastKey() - mFormatedKDE.firstKey()) / mFormatedKDE.size())) + "<br>";
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
        list << locale.toString(mResults.funcAnalysis.mode, 'f', precision);
        list << locale.toString(mResults.funcAnalysis.mean, 'f', precision);
        list << locale.toString(mResults.funcAnalysis.std, 'f', precision);
        list << locale.toString(mResults.funcAnalysis.quartiles.Q1, 'f', precision);
        list << locale.toString(mResults.funcAnalysis.quartiles.Q2, 'f', precision);
        list << locale.toString(mResults.funcAnalysis.quartiles.Q3, 'f', precision);

        list << locale.toString(mExactCredibilityThreshold * 100., 'f', 2);
        list << locale.toString(mFormatedCredibility.first, 'f', precision);
        list << locale.toString(mFormatedCredibility.second, 'f', precision);

        const QList<QPair<double, QPair<double, double>>>& intervals = mRawHPDintervals;
        double min_inter = DateUtils::convertToAppSettingsFormat(intervals.at(0).second.first);
        double max_inter = DateUtils::convertToAppSettingsFormat(intervals.at(0).second.second);

        if (min_inter < max_inter) {
            for (auto&& interval : intervals) {
                list << locale.toString(interval.first * 100., 'f', 2);
                list << locale.toString(DateUtils::convertToAppSettingsFormat(interval.second.first), 'f', precision);
                list << locale.toString(DateUtils::convertToAppSettingsFormat(interval.second.second), 'f', precision);
            }

        } else {
            for (auto interval = intervals.crbegin(); interval != intervals.crend(); interval++) {
                list << locale.toString(interval->first * 100., 'f', 2);
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
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.funcAnalysis.mode), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.funcAnalysis.mean), 'f', precision);
        list << locale.toString(mResults.funcAnalysis.std, 'f', precision);

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
        qDebug() << "[MetropolisVariable::save_stream_v337] Error: "
                 << e.what()
                 << " ; stream.status()=" << stream.status();
        // Politique de gestion d'erreur selon vos besoins
        // Vous pouvez choisir de lancer, réinitialiser ou ignorer
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
        case 0 : mSupport = MetropolisVariable::eR; // on R
            break;
        case 1 : mSupport = MetropolisVariable::eRp; // on R+
            break;
        case 2 : mSupport = MetropolisVariable::eRm; // on R-
            break;
        case 3 : mSupport = MetropolisVariable::eRpStar; // on R+*
            break;
        case 4 : mSupport = MetropolisVariable::eRmStar; // on R-*
            break;
        case 5 : mSupport = MetropolisVariable::eBounded; // on bounded support
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
        case 0: mSupport = MetropolisVariable::eR; break;
        case 1: mSupport = MetropolisVariable::eRp; break;
        case 2: mSupport = MetropolisVariable::eRm; break;
        case 3: mSupport = MetropolisVariable::eRpStar; break;
        case 4: mSupport = MetropolisVariable::eRmStar; break;
        case 5: mSupport = MetropolisVariable::eBounded; break;
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
        case 0: mSupport = MetropolisVariable::eR; break;
        case 1: mSupport = MetropolisVariable::eRp; break;
        case 2: mSupport = MetropolisVariable::eRm; break;
        case 3: mSupport = MetropolisVariable::eRpStar; break;
        case 4: mSupport = MetropolisVariable::eRmStar; break;
        case 5: mSupport = MetropolisVariable::eBounded; break;
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
        case 0: mSupport = MetropolisVariable::eR; break;
        case 1: mSupport = MetropolisVariable::eRp; break;
        case 2: mSupport = MetropolisVariable::eRm; break;
        case 3: mSupport = MetropolisVariable::eRpStar; break;
        case 4: mSupport = MetropolisVariable::eRmStar; break;
        case 5: mSupport = MetropolisVariable::eBounded; break;
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

        stream >> is_curve_filtering;
        load_container_nullable(stream, mDisplayAcquiredTrace);

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Failed to read display Acquired trace");
        }

    } catch (const std::exception& e) {
        std::cout << "[MetropolisVariable::load_stream_v337] Error: "
                  << e.what()
                  << " ; stream.status()=" << stream.status()<< std::endl;

    }
}


QDataStream &operator>>( QDataStream& stream, MetropolisVariable& data )
{
    data.load_stream(stream);  // Chargement
    return stream;

}
