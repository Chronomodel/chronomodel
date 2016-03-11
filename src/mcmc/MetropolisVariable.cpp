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



MetropolisVariable::MetropolisVariable(QObject *parent):QObject(parent),
mX(0),
mSupport(eR),
mFormat(DateUtils::eNumeric),
mExactCredibilityThreshold(0)
{
    mCredibility = QPair<double,double>();
    mHPD = QMap<double,double>();
    QObject::connect(this, &MetropolisVariable::formatChanged, this, &MetropolisVariable::updateFormatedTrace);
}

MetropolisVariable::~MetropolisVariable()
{
    QObject::disconnect(this, &MetropolisVariable::formatChanged, this, &MetropolisVariable::updateFormatedTrace);
}

void MetropolisVariable::memo()
{
    mRawTrace.push_back(mX);
}

void MetropolisVariable::reset()
{
    mRawTrace.clear();
    mFormatedTrace.clear();
    
    mHisto.clear();
    mChainsHistos.clear();
    
    mCorrelations.clear();
    
    mHPD.clear();
    
    mChainsResults.clear();
}

MetropolisVariable& MetropolisVariable::copy(MetropolisVariable const& origin)
{
    mX = origin.mX;
    mRawTrace = origin.mRawTrace;
    mFormatedTrace = origin.mFormatedTrace;
    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mHisto = origin.mHisto;
    mChainsHistos = origin.mChainsHistos;

    mCorrelations = origin.mCorrelations;

    mHPD = origin.mHPD;
    mCredibility = origin.mCredibility;
    mThreshold = origin.mThreshold;

    mExactCredibilityThreshold = origin.mExactCredibilityThreshold;

    mResults = origin.mResults;
    mChainsResults = origin.mChainsResults;
    //mIsDate = origin.mIsDate;

    return *this;
}

MetropolisVariable& MetropolisVariable::operator=( MetropolisVariable const& origin)
{
    copy(origin);
    return *this;
}

void MetropolisVariable::setFormat(const DateUtils::FormatDate fm)
{
    if(fm != mFormat) {
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
    if(!mFormatedTrace.isEmpty()) mFormatedTrace.clear();
    mRawTrace.squeeze(); // just cleaning, must be somewhere else to optimize
    mFormatedTrace.reserve(mRawTrace.size());
    QVector<double>::const_iterator iter = mRawTrace.constBegin();
    if(mFormat == DateUtils::eNumeric) {
        mFormatedTrace = mRawTrace;
    }
    else {
        while(iter!= mRawTrace.constEnd()){
            mFormatedTrace.append(DateUtils::convertToFormat(*iter,mFormat));
            ++iter;
        }
    }
}

/**
 @param[in] dataSrc is the trace, with for example one million data
 @param[in] hFactor corresponds to the bandwidth factor.
 @remarks Produce a density with the area equal to 1. The smoothing is done with Hsilvermann method.
 **/
void MetropolisVariable::generateBufferForHisto(float* input, const QVector<double> &dataSrc, const int numPts, const double a, const double b)
{
    // Work with "double" precision here !
    // Otherwise, "denum" can be very large and lead to infinity contribs!
    
    const double delta = (b - a) / (numPts - 1);

    const double denum = dataSrc.size();
    
    //float* input = (float*) fftwf_malloc(numPts * sizeof(float));
    
    //memset(input, 0.f, numPts);
    for(int i=0; i<numPts; ++i)
        input[i]= 0.f;
    
    QVector<double>::const_iterator iter = dataSrc.cbegin();
    for(; iter != dataSrc.cend(); ++iter)
    {
        const double t = *iter;
        
        const double idx = (t - a) / delta;
        const double idx_under = floor(idx);
        const double idx_upper = idx_under + 1.;
        
        const float contrib_under = (idx_upper - idx) / denum;
        const float contrib_upper = (idx - idx_under) / denum;
        
        if(std::isinf(contrib_under) || std::isinf(contrib_upper))
        {
            qDebug() << "FFT input : infinity contrib!";
        }
        if(idx_under < 0 || idx_under >= numPts || idx_upper < 0 || idx_upper > numPts)
        {
            qDebug() << "FFT input : Wrong index";
        }
        if(idx_under < numPts)
            input[(int)idx_under] += contrib_under;
        if(idx_upper < numPts) // This is to handle the case when matching the last point index !
            input[(int)idx_upper] += contrib_upper;
    }
    //return input;
}

/**
  @param hFactor corresponds to the bandwidth factor
  @param dataSrc is the trace of the raw data
  @brief the FFTW function transform the area such that the area output is the area input multiplied by fftLen. So we have to corret it.
  The result is migth be not with regular step between value.
 **/
QMap<double, double> MetropolisVariable::generateHisto(const QVector<double>& dataSrc, const int fftLen, const double hFactor, const double tmin, const double tmax)
{
    const int inputSize = fftLen;
    const int outputSize = 2 * (inputSize / 2 + 1);

    const double sigma = dataStd(dataSrc);
    QMap<double, double> result;
    if (sigma == 0) {
        qDebug()<<"MetropolisVariable::generateHisto sigma == 0";
        if(dataSrc.size()>0) {
            // if sigma is null and there are several values, it means: this is the same
            // value. It can appear with a bound fixed
            result.insert(dataSrc.at(0)+tmin, 1) ;
            qDebug()<<"MetropolisVariable::generateHisto result = "<< (dataSrc.at(0)+tmin);
        }
        return result;
    }

     const double h = hFactor * sigma * pow(dataSrc.size(), -1./5.);
     const double a = vector_min_value(dataSrc) - 4. * h;
     const double b = vector_max_value(dataSrc) + 4. * h;

     float* input = (float*) fftwf_malloc(fftLen * sizeof(float));
     generateBufferForHisto(input, dataSrc, fftLen, a, b);

     //float* input = generateBufferForHisto(dataSrc, fftLen, a, b);
    float* output = (float*) fftwf_malloc(outputSize * sizeof(float));
    
    if(input != 0) {
        // ----- FFT -----
        // http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
        //https://jperalta.wordpress.com/2006/12/12/using-fftw3/
        fftwf_plan plan_forward = fftwf_plan_dft_r2c_1d(inputSize, input, (fftwf_complex*)output, FFTW_ESTIMATE);
        fftwf_execute(plan_forward);

        for(int i=0; i<outputSize/2; ++i) {
            const double s = 2.f * M_PI * i / (b-a);
            const double factor = expf(-0.5f * s * s * h * h);

            output[2*i] *= factor;
            output[2*i + 1] *= factor;
        }

        fftwf_plan plan_backward = fftwf_plan_dft_c2r_1d(inputSize, (fftwf_complex*)output, input, FFTW_ESTIMATE);
        fftwf_execute(plan_backward);

        // ----- FFT Buffer to result map -----

        double tBegin = a;
        double tEnd = b;
        switch(mSupport)
        {
              case eR :// on R
                  // nothing to do already done by default
              break;
              case eRp : // on R+
                  tBegin = 0;
              break;
              case eRm :// on R-
                  tEnd = 0;
              break;
              case eRpStar : // on R+*
                  tBegin = 0;
              break;
              case eRmStar :// on R-*
                  tEnd = 0;
              break;
              case eBounded : // on [tmin;tmax]
                  tBegin = tmin;
                  tEnd = tmax;
              break;
        }
        const double delta = (b - a) / fftLen;
        for(int i=0; i<inputSize; ++i) {
             const double t = a + (double)i * delta;
             result[t] = input[i];
        }

        result = getMapDataInRange(result,tBegin,tEnd);


        fftwf_free(input);
        fftwf_free(output);
        input = 0;
        output = 0;
        fftwf_destroy_plan(plan_forward);
        fftwf_destroy_plan(plan_backward);

        result = equal_areas(result, 1.); // normalize the output area du to the fftw and the case (t >= tmin && t<= tmax)
    }
    return result; // return a map between a and b with a step delta = (b - a) / fftLen;
}


void MetropolisVariable::generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double hFactor, const double tmin, const double tmax)
{
    const QVector<double> subFullTrace = fullRunTrace(chains);
    mHisto = generateHisto(subFullTrace, fftLen, hFactor, tmin, tmax);

    mChainsHistos.clear();
    for(int i=0; i<chains.size(); ++i)
    {
        const QVector<double> subTrace = runFormatedTraceForChain(chains, i);
        const QMap<double,double> histo = generateHisto(subTrace, fftLen, hFactor, tmin, tmax);
        mChainsHistos.append(histo);
    }
}

void MetropolisVariable::generateHPD(const double threshold)
{
    if(!mHisto.isEmpty())
    {
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
        mThreshold = thresh;
        mHPD = create_HPD(mHisto, thresh);
        
        // No need to have HPD for all chains !
        //mChainsHPD.clear();
        //for(int i=0; i<mChainsHistos.size(); ++i)
        //  mChainsHPD.append(create_HPD(mChainsHistos[i], 1, threshold));
    }
    else
    {
        // This can happen on phase duration, if only one event inside.
        // alpha = beta => duration is always null !
        // We don't display the phase duration but we print the numerical HPD result.
        
        qDebug() << "WARNING : Cannot generate HPD on empty histo in MetropolisVariable::generateHPD";
    }
}

void MetropolisVariable::generateCredibility(const QList<ChainSpecs> &chains, double threshold)
{
    if(!mHisto.isEmpty())
    {
        mThreshold = threshold;
        mCredibility = credibilityForTrace(fullRunTrace(chains), threshold, mExactCredibilityThreshold);
    }
}

void MetropolisVariable::generateCorrelations(const QList<ChainSpecs>& chains)
{
    const int hmax = 40;
    mCorrelations.clear();

    mCorrelations.reserve(chains.size());

    for(int c=0; c<chains.size(); ++c)
    {
        // Return the acquisition part of the trace
        const QVector<double> trace = runRawTraceForChain(chains, c);
        if(trace.size()<hmax) continue;
        QVector<double> results;
        results.reserve(hmax);

        const int n = trace.size();
        
        const double s = sum(trace);
        const double m = s / (double)n;
        const double s2 = sum2Shifted(trace, -m);
        
        // Correlation pour cette chaine

        for(int h=0; h<hmax; ++h)
        {
            double sH = 0;
            for(QVector<double>::const_iterator iter = trace.cbegin(); iter != trace.cbegin() + (n-h); ++iter){
                sH += (*iter - m) * (*(iter + h) - m);
            }
            
            const double result = sH / s2;
            results.append(result);
        }
        // Correlation ajoutée à la liste (une courbe de corrélation par chaine)
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
    for(int i = 0; i<mChainsHistos.size(); ++i)
    {
        DensityAnalysis result;
        result.analysis = analyseFunction(mChainsHistos.at(i));
        result.quartiles = quartilesForTrace(runFormatedTraceForChain(chains, i));
        mChainsResults.append(result);
    }
}

#pragma mark getters (no calculs)
QMap<double, double>& MetropolisVariable::fullHisto()
{
    return mHisto;
}

QMap<double, double>& MetropolisVariable::histoForChain(int index)
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
QVector<double> MetropolisVariable::fullTraceForChain(const QList<ChainSpecs>& chains, int index)
{
    QVector<double> trace(0);
    int shift = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        unsigned long traceSize = chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter) + chains.at(i).mNumRunIter / chains.at(i).mThinningInterval;
        //qDebug()<<traceSize << chains.at(i).mTotalIter;
        if(i == index)
        {
            trace=mFormatedTrace.mid(shift , traceSize);
            
            break;
        }
        shift += traceSize;
    }
    return trace;
}

QVector<double> MetropolisVariable::fullRunTrace(const QList<ChainSpecs>& chains)
{
    QVector<double> trace(0);
    int shift = 0;
    for(int i=0; i<chains.size(); ++i)
    {
        const ChainSpecs& chain = chains.at(i);
        
        unsigned long burnAdaptSize = chain.mNumBurnIter + (chain.mBatchIndex * chain.mNumBatchIter);
        unsigned long traceSize = burnAdaptSize + chain.mNumRunIter / chain.mThinningInterval;
        
        trace=mFormatedTrace.mid(shift + burnAdaptSize, traceSize - burnAdaptSize);
        
        shift += traceSize;
    }
    return trace;
}
/**
 * @brief MetropolisVariable::runTraceForChain
 * @param chains
 * @param index the number of the Trace to extract
 * @return a QVector containing juste the acquisition Trace for one chaine n° index
 */
QVector<double> MetropolisVariable::runRawTraceForChain(const QList<ChainSpecs> &chains, int index)
{
    QVector<double> trace(0);
    if (mRawTrace.empty()) {
        qDebug() << "in MetropolisVariable::runRawTraceForChain -> mRawTrace empty";
        return trace ;
    }
    else
    {
        int shift = 0;
        for(int i=0; i<chains.size(); ++i) {
            const ChainSpecs& chain = chains.at(i);
            
            unsigned int burnAdaptSize = int (chain.mNumBurnIter + chain.mBatchIndex * chain.mNumBatchIter);
            unsigned int traceSize = int (burnAdaptSize + chain.mNumRunIter / chain.mThinningInterval);
            
            if(i == index) {
                trace=mRawTrace.mid(shift + burnAdaptSize, traceSize - burnAdaptSize);
                break;
            }
            shift += traceSize;
        }
        return trace;
    }
}

QVector<double> MetropolisVariable::runFormatedTraceForChain(const QList<ChainSpecs> &chains, int index)
{
    QVector<double> trace(0);
    if (mFormatedTrace.empty()) {
        qDebug() << "in MetropolisVariable::runFormatedTraceForChain -> mFormatedTrace empty";
        return trace ;
    }
    else
    {
        int shift = 0;
        for(int i=0; i<chains.size(); ++i)  {
            const ChainSpecs& chain = chains.at(i);

            unsigned int burnAdaptSize = int (chain.mNumBurnIter + chain.mBatchIndex * chain.mNumBatchIter);
            unsigned int traceSize = int (burnAdaptSize + chain.mNumRunIter / chain.mThinningInterval);

            if(i == index) {
                trace=mFormatedTrace.mid(shift + burnAdaptSize-1, traceSize - burnAdaptSize);
                break;
            }
            shift += traceSize;
        }
        return trace;
    }
}


QVector<double> MetropolisVariable::correlationForChain(const int index)
{
    if(index < mCorrelations.size())
        return mCorrelations.at(index);
    return QVector<double>();
}


QString MetropolisVariable::resultsString(const QString& nl, const QString& noResultMessage, const QString& unit, FormatFunc formatFunc) const
{
    if(mHisto.isEmpty())
        return noResultMessage;
    
    const QLocale locale;
    QString result = densityAnalysisToString(mResults, nl) + nl;
    if(!mHPD.isEmpty()) {
        result += "HPD Region (" + locale.toString(mThreshold, 'f', 1) + "%) : " + getHPDText(mHPD, mThreshold, unit, formatFunc) + nl;
    }
    if(mCredibility != QPair<double,double>()) {
        if (formatFunc) {
            result += "Credibility Interval (" + locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1) + "%) : [" + formatFunc(mCredibility.first) + ", " + formatFunc(mCredibility.second) + "] " + unit;
        }
        else {
            result += "Credibility Interval (" + locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1) + "%) : [" + DateUtils::dateToString(mCredibility.first) + ", " + DateUtils::dateToString(mCredibility.second) + "]";
        }
   }
   return result;
}

QStringList MetropolisVariable::getResultsList(const QLocale locale, const bool withDateFormat)
{
    QStringList list;
    if(withDateFormat) {

        list << locale.toString(mResults.analysis.mode);
        list << locale.toString(mResults.analysis.mean);
        list << DateUtils::dateToString(mResults.analysis.stddev);
        list << locale.toString(mResults.quartiles.Q1);
        list << locale.toString(mResults.quartiles.Q2);
        list << locale.toString(mResults.quartiles.Q3);
        list << locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1);
        list << locale.toString(mCredibility.first);
        list << locale.toString(mCredibility.second);

        const QList<QPair<double, QPair<double, double> > > intervals = intervalsForHpd(mHPD, mThreshold);
        QStringList results;
        for(int i=0; i<intervals.size(); ++i)
        {
            list << locale.toString(intervals.at(i).first, 'f', 1);
            list << locale.toString(intervals.at(i).second.first);
            list << locale.toString(intervals.at(i).second.second);
        }

    }
    else {
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.analysis.mode));
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.analysis.mean));
        list << DateUtils::dateToString(mResults.analysis.stddev);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.quartiles.Q1));
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.quartiles.Q2));
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.quartiles.Q3));
        list << locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mCredibility.first));
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mCredibility.second));

        const QList<QPair<double, QPair<double, double> > > intervals = intervalsForHpd(mHPD, mThreshold);
        QStringList results;
        for(int i=0; i<intervals.size(); ++i)
        {
            list << locale.toString(intervals.at(i).first, 'f', 1);
            list << locale.toString(DateUtils::convertFromAppSettingsFormat(intervals.at(i).second.first));
            list << locale.toString(DateUtils::convertFromAppSettingsFormat(intervals.at(i).second.second));
        }
    }



    return list;
}

void MetropolisVariable::saveToStream(QDataStream* out) // ajout PhD
{
    /* *out << this->mChainsHistos;
     //out << this->mChainsResults;
     *out << this->mCorrelations;
     *out << this->mCredibility;
     *out << this->mExactCredibilityThreshold;
     *out << this->mHisto;
     *out << this->mHPD;
     //out << this->mResults;
     *out << this->mThreshold;
     *out << this->mTrace;
     *out << this->mX;*/
}
void MetropolisVariable::loadFromStream(QDataStream *in) // ajout PhD
{/*
    *in >> this->mChainsHistos;
    //in >> this->mChainsResults;
    *in >> this->mCorrelations;
    *in >> this->mCredibility;
    *in >> this->mExactCredibilityThreshold;
    *in >> this->mHisto;
    *in >> this->mHPD;
    //in >> this->mResults;
    *in >> this->mThreshold;
    *in >> this->mTrace;
    *in >> this->mX; */
}
