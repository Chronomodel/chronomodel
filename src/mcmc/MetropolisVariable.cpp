#include "MetropolisVariable.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "Functions.h"
#include "DateUtils.h"

#if USE_FFT
#include "fftw3.h"
#endif

#include <QDebug>
#include <algorithm>
#include <assert.h>

/** Default constructor */
MetropolisVariable::MetropolisVariable(QObject *parent):QObject(parent),
mX(0.),
mRawTrace(nullptr),
mFormatedTrace(nullptr),
mSupport(eR),
mFormat(DateUtils::eNumeric),
mExactCredibilityThreshold(0.),
mfftLenUsed(-1),
mBandwidthUsed(-1.),
mThresholdUsed(-1.),
mtminUsed(0.),
mtmaxUsed(0.)
{
    // will not throw exception,(std::nothrow) in C++ Programmin Language B. Stroustrup Section 19.4.5
    mRawTrace = new QVector<double>();
    mFormatedTrace = new QVector<double>();

    mCredibility = QPair<double, double>();
    mHPD = QMap<double, double>();
    QObject::connect(this, &MetropolisVariable::formatChanged, this, &MetropolisVariable::updateFormatedTrace);
}

/** Copy constructor */
MetropolisVariable::MetropolisVariable (const MetropolisVariable& origin)
{
    mX = origin.mX;
    mRawTrace = new QVector<double>(origin.mRawTrace->size());
    std::copy(origin.mRawTrace->begin(),origin.mRawTrace->end(),mRawTrace->begin());

    mFormatedTrace= new QVector<double>(origin.mFormatedTrace->size());
    std::copy(origin.mFormatedTrace->begin(),origin.mFormatedTrace->end(),mFormatedTrace->begin());

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mHisto = origin.mHisto;
    mChainsHistos = origin.mChainsHistos;

    mCorrelations = origin.mCorrelations;

    mHPD = origin.mHPD;
    mCredibility = origin.mCredibility;

    mExactCredibilityThreshold = origin.mExactCredibilityThreshold;

    mResults = origin.mResults;
    mChainsResults = origin.mChainsResults;

    mfftLenUsed = origin.mBandwidthUsed;
    mBandwidthUsed = origin.mBandwidthUsed;
    mThresholdUsed = origin.mThresholdUsed;

    mtminUsed = origin.mtminUsed;
    mtmaxUsed = origin.mtmaxUsed;

     QObject::connect(this, &MetropolisVariable::formatChanged, this, &MetropolisVariable::updateFormatedTrace);

}




/** Destructor */
MetropolisVariable::~MetropolisVariable()
{
    mRawTrace->~QVector();
    mFormatedTrace->~QVector();

    mHPD.clear();
    QObject::disconnect(this, &MetropolisVariable::formatChanged, this, &MetropolisVariable::updateFormatedTrace);

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

    mHisto = origin.mHisto;
    mChainsHistos = origin.mChainsHistos;

    mCorrelations = origin.mCorrelations;

    mHPD = origin.mHPD;
    mCredibility = origin.mCredibility;

    mExactCredibilityThreshold = origin.mExactCredibilityThreshold;

    mResults = origin.mResults;
    mChainsResults = origin.mChainsResults;

    mfftLenUsed = origin.mBandwidthUsed;
    mBandwidthUsed = origin.mBandwidthUsed;
    mThresholdUsed = origin.mThresholdUsed;

    mtminUsed = origin.mtminUsed;
    mtmaxUsed = origin.mtmaxUsed;

    QObject::connect(this, &MetropolisVariable::formatChanged, this, &MetropolisVariable::updateFormatedTrace);

    return *this;
}

/** Move assignment operator */
/*MetropolisVariable& MetropolisVariable::operator=(MetropolisVariable && origin)
{
    reset();
    MetropolisVariable tmp(origin);
    *this = std::move(tmp);
    origin.~MetropolisVariable();
    return *this;
}*/

void MetropolisVariable::memo()
{
    mRawTrace->push_back(mX);
}

void MetropolisVariable::reset()
{
    if (mRawTrace != nullptr) {
        mRawTrace->clear();
        mRawTrace->squeeze();
        mFormatedTrace->clear();
        mFormatedTrace->squeeze();
    } else {
        mRawTrace = new QVector<double>();
        mFormatedTrace = new QVector<double>();
    }
    mHisto.clear();
    mChainsHistos.clear();
    
    mCorrelations.clear();
    
    mHPD.clear();
    
    mChainsResults.clear();
}

void MetropolisVariable::reserve( const int reserve)
{
    mRawTrace->reserve(reserve);
    mFormatedTrace->reserve(reserve);
}

void MetropolisVariable::setFormat(const DateUtils::FormatDate fm)
{
    if (fm != mFormat) {
        mFormat = fm;
        emit formatChanged();
    }
}

/**
 * @brief MetropolisVariable::updateFormatedTrace, it's a slot that transform or create mFormatedTrace
 * according to mFormat.
 */
void MetropolisVariable::updateFormatedTrace()
{
    if (!mRawTrace)
        return;

    mFormatedTrace->clear();
    if (mRawTrace->size() == 0)
       return;

    mFormatedTrace->reserve(mRawTrace->size());

    if (mFormat == DateUtils::eNumeric)
        mFormatedTrace = mRawTrace;

    else {
        mFormatedTrace->resize(mRawTrace->size());
        std::transform(mRawTrace->cbegin(),mRawTrace->cend(),mFormatedTrace->begin(),[this](const double i){return DateUtils::convertToFormat(i,this->mFormat);});
    }

}

/**
 @param[in] dataSrc is the trace, with for example one million data
 @remarks Produce a density with the area equal to 1. The smoothing is done with Hsilvermann method.
 **/
void MetropolisVariable::generateBufferForHisto(double *input, const QVector<double> &dataSrc, const int numPts, const double a, const double b)
{
    // Work with "double" precision here !
    // Otherwise, "denum" can be very large and lead to infinity contribs!
    
    const double delta = (b - a) / (numPts - 1);

    const double denum = dataSrc.size();
    
    //float* input = (float*) fftwf_malloc(numPts * sizeof(float));
    
    //memset(input, 0.f, numPts);
    for (int i=0; i<numPts; ++i)
        input[i]= 0.;
    
    QVector<double>::const_iterator iter = dataSrc.cbegin();
    for (; iter != dataSrc.cend(); ++iter) {
        const double t = *iter;
        
        const double idx = (t - a) / delta;
        const double idx_under = floor(idx);
        const double idx_upper = idx_under + 1.;
        
        const double contrib_under = (idx_upper - idx) / denum;
        const double contrib_upper = (idx - idx_under) / denum;
        
        if (std::isinf(contrib_under) || std::isinf(contrib_upper))
            qDebug() << "FFT input : infinity contrib!";

        if (idx_under < 0 || idx_under >= numPts || idx_upper < 0 || idx_upper > numPts)
            qDebug() << "FFT input : Wrong index";

        if (idx_under < numPts)
            input[(int)idx_under] += contrib_under;

        if (idx_upper < numPts) // This is to handle the case when matching the last point index !
            input[(int)idx_upper] += contrib_upper;
    }
    //return input;
}

/**
  @param[in] bandwidth corresponds to the bandwidth factor
  @param[in] dataSrc is the trace of the raw data
  @brief the FFTW function transform the area such that the area output is the area input multiplied by fftLen. So we have to corret it.
  The result is migth be not with regular step between value.
 **/
QMap<double, double> MetropolisVariable::generateHisto(const QVector<double>& dataSrc, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    mfftLenUsed = fftLen;
    mBandwidthUsed = bandwidth;
    mtmaxUsed = tmax;
    mtminUsed = tmin;

    const int inputSize (fftLen);
    const int outputSize = 2 * (inputSize / 2 + 1);

    const double sigma = dataStd(dataSrc);
    QMap<double, double> result;



    if (sigma == 0) {
        qDebug()<<"MetropolisVariable::generateHisto sigma == 0"<<mName;
        if (dataSrc.size()>0) {
            // if sigma is null and there are several values, it means: this is the same
            // value. It can appear with a bound fixed
            result.insert(dataSrc.at(0), 1.) ;
            qDebug()<<"MetropolisVariable::generateHisto result = "<< (dataSrc.at(0))<<mName;
        }
        return result;
    }
    // DEBUG
   /*QVector<double> histo = vector_to_histo(dataSrc,tmin,tmax,fftLen);
    const double step = (tmax-tmin)/fftLen;
    result = vector_to_map(histo, tmin, tmax, step);
    return result;
    */// /// DEBUG

     const double h = bandwidth * sigma * pow(dataSrc.size(), -1./5.);
     const double a = vector_min_value(dataSrc) - 4. * h;
     const double b = vector_max_value(dataSrc) + 4. * h;

     double* input = (double*) fftw_malloc(fftLen * sizeof(double));
     generateBufferForHisto(input, dataSrc, fftLen, a, b);

     double* output = (double*) fftw_malloc(outputSize * sizeof(double));
    
    if (input != nullptr) {
        // ----- FFT -----
        // http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
        //https://jperalta.wordpress.com/2006/12/12/using-fftw3/
        fftw_plan plan_forward = fftw_plan_dft_r2c_1d(inputSize, input, (fftw_complex*)output, FFTW_ESTIMATE);
        fftw_execute(plan_forward);

        for (int i=0; i<outputSize/2; ++i) {
            const double s = 2. * M_PI * i / (b-a);
            const double factor = expf(-0.5 * s * s * h * h);

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
        const double delta = (b - a) / fftLen;

        for (int i=0; i<inputSize; ++i) {
             const double t = a + (double)i * delta;
             result[t] = input[i];
        }

        result = getMapDataInRange(result,tBegin,tEnd);

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


void MetropolisVariable::generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    const QVector<double> subFullTrace (fullRunTrace(chains));
    mHisto = generateHisto(subFullTrace, fftLen, bandwidth, tmin, tmax);

    mChainsHistos.clear();
    for (int i=0; i<chains.size(); ++i) {
        const QVector<double> subTrace ( runFormatedTraceForChain(chains, i));
        if (!subTrace.isEmpty()) {
            const QMap<double, double> histo (generateHisto(subTrace, fftLen, bandwidth, tmin, tmax) );
            mChainsHistos.append(histo);
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
   return ((mfftLenUsed == fftLen) &&  (mBandwidthUsed == bandwidth) &&   (mtminUsed == tmin) &&  (mtmaxUsed == tmax) ? true: false);
}

void MetropolisVariable::generateHPD(const double threshold)
{
    if (!mHisto.isEmpty())  {
        const double thresh = qBound(0.0, threshold, 100.0);
        if (thresh == 100.) {
            mHPD = mHisto;
            return;
        }
        //threshold = (threshold < 0 ? threshold = 0.0 : threshold);
        if (thresh == 0.) {
            mHPD.clear();
            return;
        }
        mHPD = create_HPD(mHisto, thresh);
        
        // No need to have HPD for all chains !
        //mChainsHPD.clear();
        //for(int i=0; i<mChainsHistos.size(); ++i)
        //  mChainsHPD.append(create_HPD(mChainsHistos[i], 1, threshold));
    }
    else {
        // This can happen on phase duration, if only one event inside.
        // alpha = beta => duration is always null !
        // We don't display the phase duration but we print the numerical HPD result.
        mHPD = QMap<double, double>();

        qDebug() << "WARNING : Cannot generate HPD on empty histo in MetropolisVariable::generateHPD";
    }
}

void MetropolisVariable::generateCredibility(const QList<ChainSpecs> &chains, double threshold)
{
    if (!mHisto.isEmpty())
        mCredibility = credibilityForTrace(fullRunTrace(chains), threshold, mExactCredibilityThreshold,"Compute credibility for "+getName());
    else
        mCredibility = QPair<double,double>();

}

void MetropolisVariable::generateCorrelations(const QList<ChainSpecs>& chains)
{
    const int hmax = 40;
    if (!mCorrelations.isEmpty())
        mCorrelations.clear();

    mCorrelations.reserve(chains.size());


    for (int c=0; c<chains.size(); ++c) {
        // Return the acquisition part of the trace
        const QVector<double> trace (runRawTraceForChain(chains, c));
        if (trace.size() < hmax)
            continue;
        QVector<double> results;
        results.reserve(hmax);

        const int n = trace.size();
        
        const double s = sum(trace);
        const double m = s / (double)n;
        const double s2 = sum2Shifted(trace, -m);
        
        // Correlation pour cette chaine

        for (int h=0; h<=hmax; ++h) {
            double sH = 0.;
            for (QVector<double>::const_iterator iter = trace.cbegin(); iter != trace.cbegin() + (n-h); ++iter)
                sH += (*iter - m) * (*(iter + h) - m);

            
            const double result = sH / s2;
            results.append(result);
        }
        // Correlation ajoutée à la liste (une courbe de corrélation par chaine)
        results.squeeze();
        mCorrelations.append(results);

    }
}

void MetropolisVariable::generateNumericalResults(const QList<ChainSpecs> &chains)
{
    // Results for chain concatenation
    mResults.analysis = analyseFunction(mHisto);
    mResults.quartiles = quartilesForTrace(fullRunTrace(chains));
    
    // Results for individual chains
    mChainsResults.clear();
    for (int i = 0; i<mChainsHistos.size(); ++i) {
        DensityAnalysis result;
        result.analysis = analyseFunction(mChainsHistos.at(i));
        result.quartiles = quartilesForTrace(runFormatedTraceForChain(chains, i));
        mChainsResults.append(result);
    }
}

// Getters (no calculs)
QMap<double, double> &MetropolisVariable::fullHisto()
{
    return mHisto;
}

QMap<double, double> &MetropolisVariable::histoForChain(const int index)
{

    Q_ASSERT(index < mChainsHistos.size());    
    return mChainsHistos[index];
}

/**
 * @brief MetropolisVariable::fullTraceForChain
 * @param chains QList of the ChainSpecs in the Model
 * @param index
 * @return The complet trace (Burning, adaptation, acquire) corresponding to chain n°index
 */
QVector<double> MetropolisVariable::fullTraceForChain(const QList<ChainSpecs>& chains, const int index)
{
   // const int reserveSize = (int) ceil( chains.at(index).mNumBurnIter + (chains.at(index).mBatchIndex * chains.at(index).mNumBatchIter) + (chains.at(index).mNumRunIter /chains.at(index).mThinningInterval ) );
   // trace.reserve(reserveSize);

    //QVector<float> trace(reserveSize);
    std::vector<double> trace;
    int shift = 0;
    
    for (int i=0; i<chains.size(); ++i) {
        // We add 1 for the init
        const int traceSize = 1 + chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter ) + (int) (chains.at(i).mNumRunIter / chains.at(i).mThinningInterval);
        trace.resize(traceSize);
        if (i == index) {
            std::copy(mFormatedTrace->begin()+shift, mFormatedTrace->begin()+shift+traceSize, trace.begin());
            //trace = mFormatedTrace.mid(shift , traceSize);
            break;
        }
        shift += traceSize;
    }
    return QVector<double>::fromStdVector(trace);
}

/**
 * @brief MetropolisVariable::fullRunRawTrace use by timeRangeFromTraces() and gapRangeFromTraces()
 * @param chains
 * @return
 */
QVector<double> MetropolisVariable::fullRunRawTrace(const QList<ChainSpecs>& chains)
{
    // calcul reserve space
    int reserveSize (0);

    for (const ChainSpecs& chain : chains)
        reserveSize += (int) ceil(chain.mNumRunIter / chain.mThinningInterval);

    QVector<double> trace(reserveSize);

    int shift (0);
    int shiftTrace (0);

    for (const ChainSpecs& chain : chains) {
        // we add 1 for the init
        const int burnAdaptSize = 1 + chain.mNumBurnIter + (int) (chain.mBatchIndex * chain.mNumBatchIter);
        const int runTraceSize = (int)(chain.mNumRunIter / chain.mThinningInterval);
        const int firstRunPosition = shift + burnAdaptSize;
        std::copy(mRawTrace->begin()+ firstRunPosition, mRawTrace->begin() + firstRunPosition + runTraceSize, trace.begin()+ shiftTrace);

        shiftTrace += runTraceSize;
        shift = firstRunPosition +runTraceSize;
    }
    return trace;
}

QVector<double> MetropolisVariable::fullRunTrace(const QList<ChainSpecs>& chains)
{
    // calcul reserve space
    int reserveSize (0);

    for (const ChainSpecs& chain : chains)
        reserveSize += (int) ceil(chain.mNumRunIter / chain.mThinningInterval);

    QVector<double> trace(reserveSize);

    int shift (0);
    int shiftTrace (0);

    for (const ChainSpecs& chain : chains) {
        // we add 1 for the init
        const int burnAdaptSize = 1 + chain.mNumBurnIter + (int) (chain.mBatchIndex * chain.mNumBatchIter);
        const int runTraceSize = (int)(chain.mNumRunIter / chain.mThinningInterval);
        const int firstRunPosition = shift + burnAdaptSize;
        std::copy(mFormatedTrace->begin()+ firstRunPosition, mFormatedTrace->begin() + firstRunPosition + runTraceSize, trace.begin()+ shiftTrace);

        shiftTrace += runTraceSize;
        shift = firstRunPosition +runTraceSize;
    }
    return trace;
}
/**
 * @brief MetropolisVariable::runTraceForChain
 * @param chains
 * @param index the number of the Trace to extract
 * @return a QVector containing juste the acquisition Trace for one chaine n° index
 */
QVector<double> MetropolisVariable::runRawTraceForChain(const QList<ChainSpecs> &chains, const int index)
{

    if (mRawTrace->isEmpty()) {
        //qDebug() << "in MetropolisVariable::runRawTraceForChain -> mRawTrace empty";
        return QVector<double>() ;
        
    } else {

        int shift (0);
        //const int reserveSize = (int) ceil(chains.at(index).mNumRunIter /chains.at(index).mThinningInterval );
        //QVector<float> trace;//(reserveSize);
        std::vector<double> trace;

        for (int i=0; i<chains.size(); ++i) {
            const ChainSpecs& chain = chains.at(i);
            // We add 1 for the init
            const int burnAdaptSize = 1 + int(chain.mNumBurnIter + chain.mBatchIndex * chain.mNumBatchIter);
            const int traceSize = int(chain.mNumRunIter / chain.mThinningInterval);

            if (i == index) {
                trace.resize(traceSize);
                std::copy(mRawTrace->begin()+shift+ burnAdaptSize, mRawTrace->begin()+shift+ burnAdaptSize +traceSize, trace.begin());
                break;
            }
            shift += burnAdaptSize + traceSize;
        }
        //return trace;
        return QVector<double>::fromStdVector(trace);
    }
}

QVector<double> MetropolisVariable::runFormatedTraceForChain(const QList<ChainSpecs> &chains, const int index)
{
    std::vector<double> trace;
    if (mFormatedTrace->empty()) {
        qDebug() << "in MetropolisVariable::runFormatedTraceForChain -> mFormatedTrace empty";
        return QVector<double>(0);//trace ;
    }  else  {
        //const int reserveSize = (int) ceil(chains.at(index).mNumRunIter /chains.at(index).mThinningInterval );
        //trace.resize(reserveSize);

        int shift = 0;
        for (int i=0; i<chains.size(); ++i)  {
            const ChainSpecs& chain = chains.at(i);
            // We add 1 for the init
            const int burnAdaptSize = 1 + chain.mNumBurnIter + (int) (chain.mBatchIndex * chain.mNumBatchIter);
            const int traceSize = (int) (chain.mNumRunIter / chain.mThinningInterval);

            if (i == index) {
                trace.resize(traceSize);
                std::copy(mFormatedTrace->begin()+shift+ burnAdaptSize, mFormatedTrace->begin()+shift + burnAdaptSize +traceSize, trace.begin());
                break;
            }
            shift += traceSize + burnAdaptSize ;
        }
        return QVector<double>::fromStdVector(trace);
    }
}


QVector<double> MetropolisVariable::correlationForChain(const int index)
{
    if (index < mCorrelations.size())
        return mCorrelations.at(index);
    return QVector<double>();
}


QString MetropolisVariable::resultsString(const QString& nl, const QString& noResultMessage, const QString& unit, FormatFunc formatFunc, const bool forcePrecision) const
{
    if (mHisto.isEmpty())
        return noResultMessage;

    QString result = densityAnalysisToString(mResults, nl, forcePrecision) + nl;
    
    if (!mHPD.isEmpty())
        result += tr("HPD Region") + QString(" ( %1 %) : %2").arg(stringWithAppSettings(mThresholdUsed, forcePrecision), getHPDText(mHPD, mThresholdUsed, unit, formatFunc, forcePrecision)) + nl;

    
    if (mCredibility != QPair<double, double>()) {
        if (formatFunc)
            result += tr("Credibility Interval") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringWithAppSettings(mExactCredibilityThreshold * 100., forcePrecision),
                                                                              formatFunc(mCredibility.first, forcePrecision),
                                                                              formatFunc(mCredibility.second, forcePrecision),
                                                                              unit);
        else
            result += tr("Credibility Interval") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringWithAppSettings(mExactCredibilityThreshold * 100., forcePrecision),
                                                                               stringWithAppSettings(mCredibility.first, forcePrecision),
                                                                               stringWithAppSettings(mCredibility.second, forcePrecision),
                                                                               unit);

   }
   return result;
}

QStringList MetropolisVariable::getResultsList(const QLocale locale, const int precision, const bool withDateFormat)
{
    QStringList list;
    if (withDateFormat) {
        list << locale.toString(mResults.analysis.mode, 'f', precision);
        list << locale.toString(mResults.analysis.mean, 'f', precision);
        list << locale.toString(mResults.analysis.stddev, 'f', 2);
        list << locale.toString(mResults.quartiles.Q1, 'f', precision);
        list << locale.toString(mResults.quartiles.Q2, 'f', precision);
        list << locale.toString(mResults.quartiles.Q3, 'f', precision);
        list << locale.toString(mExactCredibilityThreshold * 100., 'f', 2);
        list << locale.toString(mCredibility.first, 'f', precision);
        list << locale.toString(mCredibility.second, 'f', precision);

        const QList<QPair<double, QPair<double, double> > > intervals = intervalsForHpd(mHPD, mThresholdUsed);

        for (auto&& interval : intervals) {
            list << locale.toString(interval.first, 'f', 2);
            list << locale.toString(interval.second.first, 'f', precision);
            list << locale.toString(interval.second.second, 'f', precision);
        }

    } else {
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.analysis.mode), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.analysis.mean), 'f', precision);
        list << locale.toString(mResults.analysis.stddev, 'f', 2);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.quartiles.Q1), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.quartiles.Q2), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.quartiles.Q3), 'f', precision);
        list << locale.toString(mExactCredibilityThreshold * 100., 'f', 2);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mCredibility.first), 'f', precision);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mCredibility.second), 'f', precision);

        const QList<QPair<double, QPair<double, double> > > intervals = intervalsForHpd(mHPD, mThresholdUsed);

        for (auto&& interval : intervals) {
            list << locale.toString(interval.first, 'f', 2);
            list << locale.toString(DateUtils::convertFromAppSettingsFormat(interval.second.first), 'f', precision);
            list << locale.toString(DateUtils::convertFromAppSettingsFormat(interval.second.second), 'f', precision);
        }
    }

    return list;
}
QDataStream &operator<<( QDataStream &stream, const MetropolisVariable &data )
{
    switch (data.mSupport) {
       case MetropolisVariable::eR : stream << (quint8)(0); // on R
        break;
       case MetropolisVariable::eRp: stream << (quint8)(1); // on R+
          break;
       case MetropolisVariable::eRm : stream << (quint8)(2); // on R-
          break;
       case MetropolisVariable::eRpStar : stream << (quint8)(3); // on R+*
          break;
       case MetropolisVariable::eRmStar : stream << (quint8)(4); // on R-*
          break;
       case  MetropolisVariable::eBounded : stream << (quint8)(5); // on bounded support
          break;
    }

    switch (data.mFormat) {

       case DateUtils::eUnknown : stream << (qint16)(-2);
        break;
       case DateUtils::eNumeric : stream << (qint16)(-1);
          break;
       case DateUtils::eBCAD : stream << (qint16)(0);
          break;

       case DateUtils::eCalBP : stream << (qint16)(1);
          break;
       case DateUtils::eCalB2K : stream << (qint16)(2);
          break;
       case  DateUtils::eDatBP : stream << (qint16)(3);
          break;
       case DateUtils::eDatB2K : stream << (qint16)(4);
          break;
       case DateUtils::eBCECE : stream << (qint16)(5);
          break;

       case  DateUtils::eKa : stream << (qint16)(6);
          break;
       case DateUtils::eMa : stream << (qint16)(7);
          break;
    }

    stream << data.mRawTrace->size();
    for (QVector<double>::const_iterator v = data.mRawTrace->cbegin(); v != data.mRawTrace->cend(); ++v)
        stream << *v;

   // qDebug()<<"&operator<<( QDataStream &stream, const MetropolisVariable &data )"<<data.mRawTrace->size();

    // *out << this->mFormatedTrace; // useless

    return stream;
}

QDataStream &operator>>( QDataStream &stream, MetropolisVariable &data )
{
    quint8 support;
    stream >> support;
    switch ((int) support) {
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
    stream >> formatDate;
    switch (formatDate) {
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

    quint32 siz;
    stream >> siz;
    if (data.mRawTrace)
        data.mRawTrace->clear();
    else
        data.mRawTrace = new QVector<double>();
    data.mRawTrace->reserve(siz);

    for (quint32 i = 0; i < siz; ++i) {
        double v;
        stream >> v;
        data.mRawTrace->push_back(v);
    }
    // regeneration of this->mFormatedTrace
    if (data.mFormatedTrace)
        data.mFormatedTrace->clear();
    else
        data.mFormatedTrace = new QVector<double>();
    data.updateFormatedTrace();

    return stream;

}

