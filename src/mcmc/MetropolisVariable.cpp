/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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
    mX (0.),
    //mRawTrace(new std::vector<double>()),
    //mFormatedTrace(new std::vector<double>()),
    mRawTrace(std::make_shared<std::vector<double>>()),
    mFormatedTrace(std::make_shared<std::vector<double>>()),
    mSupport (eR),
    mFormat (DateUtils::eNumeric),
    mFormatedHisto(),
    mChainsHistos(),
    mCorrelations(),
    mFormatedHPD(),
    mRawHPDintervals(),
    mExactCredibilityThreshold (0.),
    mResults(),
    mChainsResults(),
    mfftLenUsed (-1),
    mBandwidthUsed (-1.),
    mThresholdUsed (-1.),
    mtminUsed (0.),
    mtmaxUsed (0.),
    _name("Empty MetropolisVariable")
{
   mRawCredibility = std::pair<double, double>(1, -1);
   mFormatedCredibility = std::pair<double, double>(1, -1);

}

/** Copy constructor */
MetropolisVariable::MetropolisVariable(const MetropolisVariable& origin):
    MetropolisVariable()
{
    mX = origin.mX;
    _name = origin._name;

    //mRawTrace = std::shared_ptr<std::vector<double>>(origin.mRawTrace);
    //mFormatedTrace = std::shared_ptr<std::vector<double>>(origin.mFormatedTrace);
    mRawTrace = std::make_shared<std::vector<double>>(*origin.mRawTrace);
    mFormatedTrace = std::make_shared<std::vector<double>>(*origin.mFormatedTrace);

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mFormatedHisto = origin.mFormatedHisto;
    mChainsHistos = origin.mChainsHistos;

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
    _name = origin._name;

    mRawTrace = std::make_shared<std::vector<double>>(*origin.mRawTrace);
    mFormatedTrace = std::make_shared<std::vector<double>>(*origin.mFormatedTrace);

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mFormatedHisto = origin.mFormatedHisto;
    mChainsHistos = origin.mChainsHistos;

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

    return *this;
}

/** Move assignment operator */
MetropolisVariable& MetropolisVariable::operator=(MetropolisVariable&& origin) noexcept
{
    if (this != &origin) { // Vérification de l'auto-assignement
        // Transférer les membres
        mX = std::move(origin.mX);
        _name = std::move(origin._name);

        mRawTrace = std::move(origin.mRawTrace);
        mFormatedTrace = std::move(origin.mFormatedTrace);

        mSupport = std::move(origin.mSupport);
        mFormat = std::move(origin.mFormat);

        mFormatedHisto = std::move(origin.mFormatedHisto);
        mChainsHistos = std::move(origin.mChainsHistos);

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

        // Laisser l'objet source dans un état valide
        // (par exemple, réinitialiser les pointeurs ou les ressources)
        origin.mRawTrace.reset();
        origin.mFormatedTrace.reset();
        // Réinitialiser d'autres membres si nécessaire
    }

    return *this;
}

void MetropolisVariable::memo()
{
   mRawTrace->push_back(mX);
}

void MetropolisVariable::memo(double* valueToSave)
{
    mRawTrace->push_back(*valueToSave);
}

void MetropolisVariable::clear()
{
    mRawTrace->clear();
    mFormatedTrace->clear();
    mFormatedHisto.clear();

    mChainsHistos.clear();
    mCorrelations.clear();
    mRawHPDintervals.clear();
    mFormatedHPD.clear();

    mChainsResults.clear();
    mRawCredibility = std::pair<double, double>(1, -1);
    mFormatedCredibility = std::pair<double, double>(1, -1);

}

void MetropolisVariable::shrink_to_fit() noexcept
{
    mRawTrace->shrink_to_fit();
    mFormatedTrace->shrink_to_fit();
    mChainsHistos.shrink_to_fit();
    mCorrelations.shrink_to_fit();
    mRawHPDintervals.shrink_to_fit();
    mChainsResults.shrink_to_fit();
}

void MetropolisVariable::clear_and_shrink() noexcept
{
    mRawTrace->clear();
    mRawTrace->shrink_to_fit();

    mFormatedTrace->clear();
    mFormatedTrace->shrink_to_fit();

    mFormatedHisto.clear();
    mChainsHistos.shrink_to_fit();

    mChainsHistos.clear();
    mChainsHistos.shrink_to_fit();

    mCorrelations.clear();
    mCorrelations.shrink_to_fit();

    mRawHPDintervals.clear();
    mRawHPDintervals.shrink_to_fit();

    mFormatedHPD.clear();

    mChainsResults.clear();
    mChainsResults.shrink_to_fit();

    mRawCredibility = std::pair<double, double>(1, -1);
    mFormatedCredibility = std::pair<double, double>(1, -1);
}


void MetropolisVariable::remove_smoothed_densities()
{
    //mRawTrace->clear(); // not a posterior
    mFormatedTrace->clear();

    mFormatedHisto.clear();

    mChainsHistos.clear();

    //mCorrelations.clear(); // ne dépand pas de la fftw

    mRawHPDintervals.clear();

    mFormatedHPD.clear();

    mChainsResults.clear();

    mRawCredibility = std::pair<double, double>(1, -1);
    mFormatedCredibility = std::pair<double, double>(1, -1);

}

void MetropolisVariable::reserve(const size_t reserve)
{
    mRawTrace->reserve(reserve); // do memory leak
    mFormatedTrace->reserve(reserve);
}

void MetropolisVariable::setFormat(const DateUtils::FormatDate fm)
{
    if (mRawTrace) {
        if (fm != mFormat || mFormatedTrace->size() != mRawTrace->size()) {
            updateFormatedTrace(fm);
        }
    }
    updateFormatedCredibility(fm);

    if (mFormat != DateUtils::eNumeric)
        mFormat = fm;
}

/**
 * @brief MetropolisVariable::updateFormatedTrace, it's a slot that transforms or creates mFormatedTrace
 * according to mFormat.
 */
void MetropolisVariable::updateFormatedTrace(const DateUtils::FormatDate fm)
{
    if (fm == DateUtils::eNumeric || mFormat == DateUtils::eNumeric) {
        mFormatedTrace = std::make_shared<std::vector<double>>(*mRawTrace);

    //mFormatedTrace = mRawTrace// it's the same pointer, if you delete mFormatedTrace, you delete mRawTrace. If you change the format you change the value of mRawTrace

    } else {
        mFormatedTrace->resize(mRawTrace->size());
        std::transform(mRawTrace->cbegin(), mRawTrace->cend(), mFormatedTrace->begin(), [&fm](const double i) {return DateUtils::convertToFormat(i, fm);});
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
 @param[in] dataSrc is the trace, with for example one million data
 @remarks Produce a density with the area equal to 1. The smoothing is done with Hsilvermann method.
 **/
/*
void MetropolisVariable::generateBufferForHisto(double *input, const QList<double> &dataSrc, const int numPts, const double a, const double b)
{
    // Work with "double" precision here !
    // Otherwise, "denum" can be very large and lead to infinity contribs!

    const double delta = (b - a) / (numPts - 1);

    const double denum = dataSrc.size();

    for (int i=0; i<numPts; ++i)
        input[i]= 0.;

    QList<double>::const_iterator iter = dataSrc.cbegin();
    for (; iter != dataSrc.cend(); ++iter) {
        const double t = *iter;

        const double idx = (t - a) / delta;
        const double idx_under = std::clamp(floor(idx), 0., numPts-1.);
        const double idx_upper = std::clamp(idx_under + 1., 0., numPts-1.);

        const double contrib_under = (idx_upper - idx) / denum;
        const double contrib_upper = (idx - idx_under) / denum;
#ifdef DEBUG
        if (std::isinf(contrib_under) || std::isinf(contrib_upper))
            qDebug() << "FFT input : infinity contrib!";

        if (idx_under < 0 || idx_under >= numPts || idx_upper < 0 || idx_upper > numPts)
            qDebug() << "FFT input : Wrong index";
#endif
        if (idx_under < numPts)
            input[(int)idx_under] += contrib_under;

        if (idx_upper < numPts) // This is to handle the case when matching the last point index !
            input[(int)idx_upper] += contrib_upper;
    }

}
*/
void MetropolisVariable::generateBufferForHisto(double *input, const std::vector<double> &dataSrc, const int numPts, const double a, const double b)
{
    // Work with "double" precision here !
    // Otherwise, "denum" can be very large and lead to infinity contribs!

    const double delta = (b - a) / (numPts - 1);

    const double denum = dataSrc.size();

    for (int i=0; i<numPts; ++i)
        input[i]= 0.;

    std::vector<double>::const_iterator iter = dataSrc.cbegin();
    for (; iter != dataSrc.cend(); ++iter) {
        const double t = *iter;

        const double idx = (t - a) / delta;
        const double idx_under = std::clamp(floor(idx), 0., numPts-1.);
        const double idx_upper = std::clamp(idx_under + 1., 0., numPts-1.);

        const double contrib_under = (idx_upper - idx) / denum;
        const double contrib_upper = (idx - idx_under) / denum;
#ifdef DEBUG
        if (std::isinf(contrib_under) || std::isinf(contrib_upper))
            qDebug() << "FFT input : infinity contrib!";

        if (idx_under < 0 || idx_under >= numPts || idx_upper < 0 || idx_upper > numPts)
            qDebug() << "FFT input : Wrong index";
#endif
        if (idx_under < numPts)
            input[(int)idx_under] += contrib_under;

        if (idx_upper < numPts) // This is to handle the case when matching the last point index !
            input[(int)idx_upper] += contrib_upper;
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

std::map<double, double> MetropolisVariable::generateHisto(const std::vector<double>& dataSrc, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    mfftLenUsed = fftLen;
    mBandwidthUsed = bandwidth;
    mtmaxUsed = tmax;
    mtminUsed = tmin;

    std::map<double, double> result;

    if (dataSrc.size() == 1) {
        // value. It can appear with a fixed variable
        result.emplace(dataSrc.at(0), 1.) ;
        //qDebug()<<"[MetropolisVariable::generateHisto] One Value = "<< dataSrc.at(0) << _name;
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


    if (sigma == 0) {
        // if sigma is null and there are several values, it means: this is a constant value
        // This can occur at the Begin or End of a Phase with a Bound.
        result.emplace(dataSrc.at(0), 1.) ;
        qDebug()<<"[MetropolisVariable::generateHisto] Constant value = "<< dataSrc.at(0) << _name;

        return result;
    }

    const double h = bandwidth * sigma * pow(dataSrc.size(), -1./5.);
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
        tBegin = 0.;
        break;
    case eRm:
    case eRmStar: // on R-*
        tEnd = 0.;
        break;
    case eBounded: // on [tmin;tmax]
        tBegin = tmin;
        tEnd = tmax;
        break;
    case eR:
        break;
    }

    // Construction du résultat
    const double delta = (b - a) / (fftLen - 1);
    for (int i = 0; i < fftLen; ++i) {
        const double t = a + static_cast<double>(i) * delta;
        result[t] = std::max(0., input.get()[i]);
    }

    // normalisation
    result = getMapDataInRange(result, tBegin, tEnd);
    result = equal_areas(result, 1.);

    return result;
}


void MetropolisVariable::generateHistos(const std::vector<ChainSpecs> &chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    //Q_ASSERT_X(!mFormatedTrace->isEmpty(), "[MetropolisVariable::generateHistos]", "mFormatedTrace.isEmpty()");
    if (mFormatedTrace == nullptr || mFormatedTrace->size() == 0)
        return;
    const std::vector<double> &subFullTrace = fullRunFormatedTrace(chains);
    mFormatedHisto = generateHisto(subFullTrace, fftLen, bandwidth, tmin, tmax);

    mChainsHistos.clear();
    for (size_t i = 0; i<chains.size(); ++i) {
        const std::vector<double> &subTrace = runFormatedTraceForChain(chains, i);
        if (!subTrace.empty()) {
            mChainsHistos.push_back(generateHisto(subTrace, fftLen, bandwidth, tmin, tmax) );
        }
    }
}

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
    if (!mFormatedHisto.empty())  {
        const double thresh = std::clamp(threshold, 0.0, 100.0);
        if (thresh == 100.) {
            mFormatedHPD = mFormatedHisto;
            return;
        } else if (thresh == 0.) {
            mFormatedHPD.clear();
            return;
        } else {
            QList<QPair<double, QPair<double, double> > > formated_intervals;
           
            auto tmp_HPD = std::map<double, double>(create_HPD_by_dichotomy(mFormatedHisto, formated_intervals, thresh));
            mFormatedHPD = tmp_HPD;
            mRawHPDintervals.clear();

            for (const auto& h : formated_intervals) {
                double tmin ,tmax ;
                if (mFormat == DateUtils::eNumeric || mFormat == DateUtils::eUnknown) {
                    tmin = h.second.first;
                    tmax = h.second.second;

                } else {
                    tmin = DateUtils::convertFromAppSettingsFormat(h.second.first);
                    tmax = DateUtils::convertFromAppSettingsFormat(h.second.second);
                }

                if (tmin>tmax)
                    std::swap(tmin, tmax);
                
                if (!mRawHPDintervals.isEmpty() && mRawHPDintervals.at(0).second.second < tmin) {
                    auto t_t = std::make_pair(tmin, tmax);
                    auto tmp_HPD = std::make_pair(h.first, t_t );
                    mRawHPDintervals.push_back(tmp_HPD);
                } else {
                    auto t_t = std::make_pair(tmin, tmax);
                    auto tmp_HPD = std::make_pair(h.first, t_t );
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
        qDebug() << "[MetropolisVariable::generateHPD] WARNING : Cannot generate HPD on empty histo with " << _name;
    }
}

void MetropolisVariable::generateCredibility(const std::vector<ChainSpecs> &chains, double threshold)
{
    if (mRawTrace==nullptr || mRawTrace->size() == 0)  {
        mRawCredibility = std::pair<double, double>(1, -1);

    } else if (mThresholdUsed != threshold) {
        mRawCredibility = credibilityForTrace(fullRunRawTrace(chains), threshold, mExactCredibilityThreshold);//, "Compute credibility for "+getName());
    }
    updateFormatedCredibility(mFormat);

}

void MetropolisVariable::generateCorrelations(const std::vector<ChainSpecs> &chains)
{
    const int hmax = 40;
    if (!mCorrelations.empty())
        mCorrelations.clear();

    //mCorrelations = std::vector<std::vector<double>>(chains.size(), std::vector<double>(40, 1.)); // test
    //return;


    //mCorrelations.reserve(chains.size());
    //Chronometer ch ("[MetropolisVariable::generateCorrelations]");

    for (size_t i = 0; i<chains.size(); ++i) {
        // Return the acquisition part of the trace
        const std::vector<double> &trace = runRawTraceForChain(chains, i);
        if (trace.size() < hmax)
            continue;

        const std::vector<double> &results = autocorrelation_schoolbook(trace);
        //QList<double> results = autocorrelation_by_convol(trace); // test

        // Correlation ajoutée à la liste (une courbe de corrélation par chaine)
        mCorrelations.push_back(results);
    }
    //ch.display();
}

void MetropolisVariable::generateNumericalResults(const std::vector<ChainSpecs> &chains)
{
    // Results for chain concatenation
    if (mFormatedHisto.empty())
        return;
    mResults.funcAnalysis = analyseFunction(mFormatedHisto);
    mResults.traceAnalysis = traceStatistic(fullRunFormatedTrace(chains)); // fullRunFormatedTrace is the formated Traces

    // Results for individual chains
    mChainsResults.clear();

    for (size_t i = 0; i<mChainsHistos.size(); ++i) {
        DensityAnalysis result;
        result.funcAnalysis = analyseFunction(mChainsHistos.at(i)); // useless
        result.traceAnalysis = traceStatistic(runFormatedTraceForChain(chains, i)); // only to compute quartiles
        mChainsResults.push_back(result);
    }
}

// Getters (no calculs)
std::map<double, double> &MetropolisVariable::fullHisto()
{
    return mFormatedHisto;
}

std::map<double, double> &MetropolisVariable::histoForChain(const size_t index)
{
    Q_ASSERT(index < (size_t)mChainsHistos.size());
    return mChainsHistos[index];
}


std::vector<double>::iterator MetropolisVariable::findIter_element(const long unsigned iter, const std::vector<ChainSpecs> &chains, const size_t index ) const
{
    size_t shift = 0;
    for (size_t i = 0; i < index; ++i) {
        shift += 1 + chains[i].mIterPerBurn + (chains[i].mBatchIndex * chains[i].mIterPerBatch) + chains[i].mRealyAccepted;
    }
    shift += 1 + chains[index].mIterPerBurn +  (chains[index].mBatchIndex * chains[index].mIterPerBatch);
    return mRawTrace->begin() + shift + iter;

}



/**
 * @brief MetropolisVariable::fullTraceForChain
 * @param chains QList of the ChainSpecs in the Model
 * @param index
 * @return The complet trace (init, Burn-in, adaptation, acquire) corresponding to chain n°index
 */
std::vector<double> MetropolisVariable::fullTraceForChain(const std::vector<ChainSpecs> &chains, const size_t index)
{
    size_t shift = 0;

    for (size_t i = 0; i<chains.size(); ++i) {
        // We add 1 for the init
        const size_t traceSize = 1 + chains.at(i).mIterPerBurn + (chains.at(i).mBatchIndex * chains.at(i).mIterPerBatch ) + chains.at(i).mRealyAccepted;
        if (i == index) {
            return std::vector<double> (mFormatedTrace->begin()+shift, mFormatedTrace->begin()+shift+traceSize);
            break;
        }
        shift += traceSize;
    }
    return std::vector<double> (0);
}



std::vector<double> MetropolisVariable::correlationForChain(const size_t index)
{
    if (index < (size_t)mCorrelations.size())
        return mCorrelations.at(index);

    return std::vector<double>();
}


QString MetropolisVariable::resultsString(const QString &noResultMessage, const QString &unit) const
{
    if (mFormatedHisto.empty())
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
        const QList<QPair<double, QPair<double, double> > > &intervals = mRawHPDintervals;

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
        //result += QObject::tr("Density Step : %1").arg(stringForLocal(std::abs(mFormatedHisto.lastKey() - mFormatedHisto.firstKey()) / mFormatedHisto.size())) + "<br>";
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

        const QList<QPair<double, QPair<double, double> > > &intervals = mRawHPDintervals;
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
        list << locale.toString(mExactCredibilityThreshold * 100., 'f', 2);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mFormatedCredibility.first), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mFormatedCredibility.second), 'f', precision);
        // Statistic Results on Density
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.funcAnalysis.mode), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.funcAnalysis.mean), 'f', precision);
        list << locale.toString(mResults.funcAnalysis.std, 'f', precision);

        const QList<QPair<double, QPair<double, double> > > &intervals = mRawHPDintervals;

        if (DateUtils::is_date(mFormat)) {
            for (auto&& interval : intervals) {
                list << locale.toString(interval.first * 100., 'f', 2);
                list << locale.toString(DateUtils::convertToAppSettingsFormat(interval.second.first), 'f', precision);
                list << locale.toString(DateUtils::convertToAppSettingsFormat(interval.second.second), 'f', precision);
            }

        } else {
            for (auto interval = intervals.crbegin(); interval != intervals.crend(); interval++) {
                list << locale.toString(interval->first * 100., 'f', 2);
                const double min_inter = DateUtils::convertToAppSettingsFormat(interval->second.second);
                const double max_inter = DateUtils::convertToAppSettingsFormat(interval->second.first);
                list << locale.toString(min_inter, 'f', precision);
                list << locale.toString(max_inter, 'f', precision);
            }
        }
    }

    return list;
}

/** Write Data
 */
QDataStream &operator<<( QDataStream &stream, const MetropolisVariable &data )
{
    stream << data.getQStringName(); // since 2024_08_23
    switch (data.mSupport) {
       case MetropolisVariable::eR : stream << quint8(0); // on R
        break;
       case MetropolisVariable::eRp: stream << quint8(1); // on R+
          break;
       case MetropolisVariable::eRm : stream << quint8(2); // on R-
          break;
       case MetropolisVariable::eRpStar : stream << quint8(3); // on R+*
          break;
       case MetropolisVariable::eRmStar : stream << quint8(4); // on R-*
          break;
       case  MetropolisVariable::eBounded : stream << quint8(5); // on bounded support
          break;
    }

   switch (data.mFormat) { // useless, only for compatibility
       case DateUtils::eUnknown : stream << qint16(-2);
        break;
       case DateUtils::eNumeric : stream << qint16(-1);
          break;
       case DateUtils::eBCAD : stream << qint16(0);
          break;
       case DateUtils::eCalBP : stream << qint16(1);
          break;
       case DateUtils::eCalB2K : stream << qint16(2);
          break;
       case  DateUtils::eDatBP : stream << qint16(3);
          break;
       case DateUtils::eDatB2K : stream << qint16(4);
          break;
       case DateUtils::eBCECE : stream << qint16(5);
          break;
       case  DateUtils::eKa : stream << qint16(6);
          break;
       case DateUtils::eMa : stream << qint16(7);
          break;
    }

    save_container(stream, *data.mRawTrace);

    // *out << this->mFormatedTrace; // useless

    return stream;
}

/** Read Data
 */
void MetropolisVariable::load_stream_v328(QDataStream& stream)
{
    QString str;
    stream >> str; // since 2024_08_23
    setName(str);

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

    reload_shared_ptr(mRawTrace, stream);
}

QDataStream &operator>>( QDataStream& stream, MetropolisVariable& data )
{
    QString str;
    stream >> str; // since 2024_08_23
    data.setName(str);

    quint8 support;
    stream >> support;
    switch (int (support)) {
      case 0 : data.mSupport = MetropolisVariable::eR; // on R
       break;
      case 1 : data.mSupport = MetropolisVariable::eRp; // on R+
         break;
      case 2 : data.mSupport = MetropolisVariable::eRm; // on R-
         break;
      case 3 : data.mSupport = MetropolisVariable::eRpStar; // on R+*
         break;
      case 4 : data.mSupport = MetropolisVariable::eRmStar; // on R-*
         break;
      case 5 : data.mSupport = MetropolisVariable::eBounded; // on bounded support
         break;
   }

    qint16 formatDate;
    stream >> formatDate; // to keep compatibility
 /*   switch (formatDate) {
      case -2 : data.mFormat = DateUtils::eUnknown;
       break;
      case -1 : data.mFormat = DateUtils::eNumeric;
         break;
      case 0 :  data.mFormat = DateUtils::eBCAD;
         break;
      case 1 : data.mFormat = DateUtils::eCalBP;
        break;
      case 2 : data.mFormat = DateUtils::eCalB2K;
        break;
      case 3 : data.mFormat = DateUtils::eDatBP;
         break;
      case 4 : data.mFormat = DateUtils::eDatB2K;
         break;
      case 5 : data.mFormat = DateUtils::eBCECE;
       break;

      case 6 : data.mFormat = DateUtils::eKa;
         break;
      case 7 : data.mFormat = DateUtils::eMa;
        break;
   }
*/
    data.mFormat = DateUtils::eUnknown; // to keep compatibility and force updateFormat

    reload_shared_ptr(data.mRawTrace, stream);
    
    // regeneration of this->mFormatedTrace
  

    return stream;

}
