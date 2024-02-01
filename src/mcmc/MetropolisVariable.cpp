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

/** Default constructor */
MetropolisVariable::MetropolisVariable():
    mX (0.),
    mRawTrace (nullptr),
    mFormatedTrace (nullptr),
    mSupport (eR),
    mFormat (DateUtils::eNumeric),
    mExactCredibilityThreshold (0.),
    mfftLenUsed (-1),
    mBandwidthUsed (-1.),
    mThresholdUsed (-1.),
    mtminUsed (0.),
    mtmaxUsed (0.)
{
    // will not throw exception,(std::nothrow) in C++ Programmin Language B. Stroustrup Section 19.4.5
    mRawTrace = new QList<double>();
    mFormatedTrace = new QList<double>();

    mRawCredibility = std::pair<double, double>(1, -1);
    mFormatedCredibility = std::pair<double, double>(1, -1);
   // mFormatedHPD = QMap<double, double>();
   // QObject::connect(this, &MetropolisVariable::formatChanged, this, &MetropolisVariable::updateFormatedTrace);
}

/** Copy constructor */
MetropolisVariable::MetropolisVariable(const MetropolisVariable &origin):
    MetropolisVariable()
{
    mX = origin.mX;
    mRawTrace->resize(origin.mRawTrace->size());
    std::copy(origin.mRawTrace->begin(), origin.mRawTrace->end(), mRawTrace->begin());

    mFormatedTrace->resize(origin.mFormatedTrace->size());
    std::copy(origin.mFormatedTrace->begin(), origin.mFormatedTrace->end(), mFormatedTrace->begin());

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mFormatedHisto = origin.mFormatedHisto;
    mChainsHistos = origin.mChainsHistos;

    mCorrelations = origin.mCorrelations;

    mFormatedHPD = origin.mFormatedHPD;
    mRawCredibility = origin.mRawCredibility;
    mFormatedCredibility = origin.mFormatedCredibility;

    mExactCredibilityThreshold = origin.mExactCredibilityThreshold;

    mResults = origin.mResults;
    mChainsResults = origin.mChainsResults;

    mfftLenUsed = origin.mBandwidthUsed;
    mBandwidthUsed = origin.mBandwidthUsed;
    mThresholdUsed = origin.mThresholdUsed;

    mtminUsed = origin.mtminUsed;
    mtmaxUsed = origin.mtmaxUsed;

    // QObject::connect(this, &MetropolisVariable::formatChanged, this, &MetropolisVariable::updateFormatedTrace);

}




/** Destructor */
MetropolisVariable::~MetropolisVariable()
{
    mRawTrace->clear();
    mRawTrace = nullptr;
    mFormatedTrace->clear();
    mFormatedTrace = nullptr;
    mFormatedHPD.clear();
}

/** Copy assignment operator */
MetropolisVariable& MetropolisVariable::operator=(const MetropolisVariable & origin)
{
    mX = origin.mX;
    mRawTrace->resize(origin.mRawTrace->size());
    std::copy(origin.mRawTrace->begin(),origin.mRawTrace->end(),mRawTrace->begin());

    mFormatedTrace->resize(origin.mFormatedTrace->size());
    std::copy(origin.mFormatedTrace->begin(),origin.mFormatedTrace->end(),mFormatedTrace->begin());

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mFormatedHisto = origin.mFormatedHisto;
    mChainsHistos = origin.mChainsHistos;

    mCorrelations = origin.mCorrelations;

    mFormatedHPD = origin.mFormatedHPD;
    mRawCredibility = origin.mRawCredibility;
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

void MetropolisVariable::memo()
{
   mRawTrace->push_back(mX);
}

void MetropolisVariable::memo(double* valueToSave)
{
    mRawTrace->push_back(*valueToSave);
}

void MetropolisVariable::reset()
{
    if (mRawTrace != nullptr) {
        mRawTrace->clear();
        mRawTrace->squeeze();
        mFormatedTrace->clear();
        mFormatedTrace->squeeze();

    } else {
        mRawTrace = new QList<double>();
        mFormatedTrace = new QList<double>();
    }
    mFormatedHisto.clear();
    mChainsHistos.clear();

    mCorrelations.clear();

    mFormatedHPD.clear();

    mChainsResults.clear();

    mRawCredibility = std::pair<double, double>(1, -1);
    mFormatedCredibility = std::pair<double, double>(1, -1);
}

void MetropolisVariable::reserve( const int reserve)
{
    mRawTrace->reserve(reserve);
    mFormatedTrace->reserve(reserve);
}

void MetropolisVariable::setFormat(const DateUtils::FormatDate fm)
{
    if (fm != mFormat || mFormatedTrace->size() != mRawTrace->size()) {
        updateFormatedTrace(fm);
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
    if (!mRawTrace)
        return;

    if (mRawTrace->size() == 0)
       return;

    if (fm == DateUtils::eNumeric || mFormat == DateUtils::eNumeric) {
        mFormatedTrace->resize(mRawTrace->size());
        std::copy(mRawTrace->cbegin(), mRawTrace->cend(), mFormatedTrace->begin());

    //mFormatedTrace = mRawTrace// it's the same pointer, if you delete mFormatedTrace, you delete mRawTrace. If you change the format you change the value of mRawTrace

    } else {
        mFormatedTrace->resize(mRawTrace->size());
        std::transform(mRawTrace->cbegin(), mRawTrace->cend(), mFormatedTrace->begin(), [&fm](const double i) {return DateUtils::convertToFormat(i, fm);});
    }
    return;

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
QMap<double, double> MetropolisVariable::generateHisto(const QList<double> &dataSrc, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    mfftLenUsed = fftLen;
    mBandwidthUsed = bandwidth;
    mtmaxUsed = tmax;
    mtminUsed = tmin;

    QMap<double, double> result;

    if (dataSrc.size() == 1) {
        // value. It can appear with a fixed variable
        result.insert(dataSrc.at(0), 1.) ;
        qDebug()<<"[MetropolisVariable::generateHisto] One Value = "<< dataSrc.at(0) << mName;
        return result;
    }


    const int inputSize = fftLen;
    const int outputSize = 2 * (inputSize / 2 + 1);

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
        result.insert(dataSrc.at(0), 1.) ;
        qDebug()<<"[MetropolisVariable::generateHisto] Constant value = "<< dataSrc.at(0) << mName;

        return result;
    }

    const double h = bandwidth * sigma * pow(dataSrc.size(), -1./5.);
    const double a = range_min_value(dataSrc) - 4. * h;
    const double b = range_max_value(dataSrc) + 4. * h;

    double* input = (double*) fftw_malloc(inputSize * sizeof(double));
    generateBufferForHisto(input, dataSrc, inputSize, a, b);

    double* output = (double*) fftw_malloc(outputSize * sizeof(double));

    if (input != nullptr) {
        // ----- FFT -----
        // http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
        //https://jperalta.wordpress.com/2006/12/12/using-fftw3/
        fftw_plan plan_forward = fftw_plan_dft_r2c_1d(inputSize, input, (fftw_complex*)output, FFTW_ESTIMATE);
        fftw_execute(plan_forward);

        for (int i=0; i<outputSize/2; ++i) {
            const double s = 2. * M_PI * i / (b-a);
            const double factor = exp(-0.5 * s * s * h * h);

            output[2*i] *= factor;
            output[2*i + 1] *= factor;
        }

        fftw_plan plan_backward = fftw_plan_dft_c2r_1d(inputSize, (fftw_complex*)output, input, FFTW_ESTIMATE);
        fftw_execute(plan_backward);

        // ----- FFT Buffer to result map -----

        double tBegin = a;
        double tEnd = b;
        switch(mSupport)
        {
              case eR :// on R
                  // nothing to do already done by default
              break;
              case eRp : // on R+
                  tBegin = 0.;
              break;
              case eRm :// on R-
                  tEnd = 0;;
              break;
              case eRpStar : // on R+*
                  tBegin = 0.;
              break;
              case eRmStar :// on R-*
                  tEnd = 0.;
              break;
              case eBounded : // on [tmin;tmax]
                  tBegin = tmin;
                  tEnd = tmax;
              break;
        }
        const double delta = (b - a) / (inputSize-1);

        for (int i = 0; i<inputSize; ++i) {
             const double t = a + (double)i * delta;
             result[t] = std::max(0., input[i]); // the histogram must not have a negative value
        }

        result = getMapDataInRange(result, tBegin, tEnd);

        fftw_free(input);
        fftw_free(output);
        input = nullptr;
        output = nullptr;
        fftw_destroy_plan(plan_forward);
        fftw_destroy_plan(plan_backward);

        result = equal_areas(result, 1.); // normalize the output area du to the fftw and the case (t >= tmin && t<= tmax)
    }
    return result; // return a map between a and b with a step delta = (b - a) / fftLen;
}


void MetropolisVariable::generateHistos(const QList<ChainSpecs> &chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    Q_ASSERT_X(!mFormatedTrace->isEmpty(), "[MetropolisVariable::generateHistos]", "mFormatedTrace.isEmpty()");
    const QList<double> &subFullTrace = fullRunFormatedTrace(chains);
    mFormatedHisto = generateHisto(subFullTrace, fftLen, bandwidth, tmin, tmax);

    mChainsHistos.clear();
    for (int i = 0; i<chains.size(); ++i) {
        const QList<double> &subTrace = runFormatedTraceForChain(chains, i);
        if (!subTrace.isEmpty()) {
            mChainsHistos.append(generateHisto(subTrace, fftLen, bandwidth, tmin, tmax) );
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
    if (!mFormatedHisto.isEmpty())  {
        const double thresh = std::clamp(threshold, 0.0, 100.0);
        if (thresh == 100.) {
            mFormatedHPD = mFormatedHisto;
            return;
        } else if (thresh == 0.) {
            mFormatedHPD.clear();
            return;
        } else {
            QList<QPair<double, QPair<double, double> > > formated_intervals;
            mFormatedHPD = QMap<double, double>(create_HPD_by_dichotomy(mFormatedHisto, formated_intervals, thresh));
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

                if (!mRawHPDintervals.isEmpty() && mRawHPDintervals.at(0).second.second < tmin)
                    mRawHPDintervals.push_back(std::make_pair(h.first, std::make_pair(tmin, tmax)));
                else
                    mRawHPDintervals.push_front(std::make_pair(h.first, std::make_pair(tmin, tmax)));

            }
        }


    } else {
        // This can happen on phase duration, if only one event inside.
        // alpha = beta => duration is always null !
        // We don't display the phase duration but we print the numerical HPD result.
        mFormatedHPD = QMap<double, double>();
        mRawHPDintervals.clear();
        qDebug() << "[MetropolisVariable::generateHPD] WARNING : Cannot generate HPD on empty histo with " << mName;
    }
}

void MetropolisVariable::generateCredibility(const QList<ChainSpecs> &chains, double threshold)
{
    if (mRawTrace->isEmpty())  {
        mRawCredibility = std::pair<double, double>(1, -1);

    } else if (mThresholdUsed != threshold) {
        mRawCredibility = credibilityForTrace(fullRunRawTrace(chains), threshold, mExactCredibilityThreshold);//, "Compute credibility for "+getName());
    }
    updateFormatedCredibility(mFormat);

}

void MetropolisVariable::generateCorrelations(const QList<ChainSpecs> &chains)
{
    const int hmax = 40;
    if (!mCorrelations.isEmpty())
        mCorrelations.clear();

    mCorrelations.reserve(chains.size());
    //Chronometer ch ("[MetropolisVariable::generateCorrelations]");

    for (int c = 0; c<chains.size(); ++c) {
        // Return the acquisition part of the trace
        const QList<double> &trace = runRawTraceForChain(chains, c);
        if (trace.size() < hmax)
            continue;

        const QList<double> &results = autocorrelation_schoolbook(trace);
        //QList<double> results = autocorrelation_by_convol(trace);

        // Correlation ajoutée à la liste (une courbe de corrélation par chaine)
        mCorrelations.append(results);

    }
    //ch.display();
}

void MetropolisVariable::generateNumericalResults(const QList<ChainSpecs> &chains)
{
    // Results for chain concatenation
    if (mFormatedHisto.isEmpty())
        return;
    mResults.funcAnalysis = analyseFunction(mFormatedHisto);
    mResults.traceAnalysis = traceStatistic(fullRunFormatedTrace(chains)); // fullRunFormatedTrace is the formated Traces

    // Results for individual chains
    mChainsResults.clear();
    for (auto i = 0; i<mChainsHistos.size(); ++i) {
        DensityAnalysis result;
        result.funcAnalysis = analyseFunction(mChainsHistos.at(i));
        result.traceAnalysis =  traceStatistic(runFormatedTraceForChain(chains, i)); //quartilesForTrace(runFormatedTraceForChain(chains, i));
        mChainsResults.append(result);
    }
}

// Getters (no calculs)
QMap<double, double> &MetropolisVariable::fullHisto()
{
    return mFormatedHisto;
}

QMap<double, double> &MetropolisVariable::histoForChain(const int index)
{
    Q_ASSERT(index < mChainsHistos.size());
    return mChainsHistos[index];
}


QList<double>::Iterator MetropolisVariable::findIter_element(const long unsigned iter, const QList<ChainSpecs>& chains, const int chainIndex ) const
{
    int shift = 0;
    for (int i = 0; i < chainIndex; ++i) {
        shift += 1 + chains[i].mIterPerBurn + (chains[i].mBatchIndex * chains[i].mIterPerBatch) + chains[i].mRealyAccepted;
    }
    shift += 1 + chains[chainIndex].mIterPerBurn +  (chains[chainIndex].mBatchIndex * chains[chainIndex].mIterPerBatch);
    return mRawTrace->begin() + shift + iter;

}



/**
 * @brief MetropolisVariable::fullTraceForChain
 * @param chains QList of the ChainSpecs in the Model
 * @param index
 * @return The complet trace (init, Burn-in, adaptation, acquire) corresponding to chain n°index
 */
QList<double> MetropolisVariable::fullTraceForChain(const QList<ChainSpecs> &chains, const int index)
{
    int shift = 0;

    for (int i = 0; i<chains.size(); ++i) {
        // We add 1 for the init
        const unsigned long traceSize = 1 + chains.at(i).mIterPerBurn + (chains.at(i).mBatchIndex * chains.at(i).mIterPerBatch ) + chains.at(i).mRealyAccepted;
        if (i == index) {
            return QList<double> (mFormatedTrace->begin()+shift, mFormatedTrace->begin()+shift+traceSize);
            break;
        }
        shift += traceSize;
    }
    return QList<double> (0);
}



QList<double> MetropolisVariable::correlationForChain(const int index)
{
    if (index < mCorrelations.size())
        return mCorrelations.at(index);

    return QList<double>();
}


QString MetropolisVariable::resultsString(const QString &noResultMessage, const QString &unit) const
{
    if (mFormatedHisto.isEmpty())
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
        result += QObject::tr("Density Step : %1").arg(stringForLocal(std::abs(mFormatedHisto.lastKey() - mFormatedHisto.firstKey()) / mFormatedHisto.size())) + "<br>";
    }

   return result;
}

QStringList MetropolisVariable::getResultsList(const QLocale locale, const int precision, const bool withDateFormat)
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

    stream << (quint32) data.mRawTrace->size();
    for (QVector<double>::const_iterator v = data.mRawTrace->cbegin(); v != data.mRawTrace->cend(); ++v)
        stream << *v;


    // *out << this->mFormatedTrace; // useless

    return stream;
}

/** Read Data
 */
QDataStream &operator>>( QDataStream &stream, MetropolisVariable &data )
{
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

    quint32 siz;
    double v;

    stream >> siz;
    if (data.mRawTrace)
        data.mRawTrace->clear();
    else
        data.mRawTrace = new QList<double>();
    data.mRawTrace->reserve(siz);

    for (quint32 i = 0; i < siz; ++i) {
        stream >> v;
        data.mRawTrace->push_back(v);
    }
    // regeneration of this->mFormatedTrace
    if (data.mFormatedTrace)
        data.mFormatedTrace->clear();
    else
        data.mFormatedTrace = new QList<double>();
   // data.updateFormatedTrace();

    return stream;

}
