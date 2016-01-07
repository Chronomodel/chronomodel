#include "MetropolisVariable.h"
#include "StdUtilities.h"
#include "Functions.h"
#include "DateUtils.h"
#if USE_FFT
#include "fftw3.h"
#endif
#include <QDebug>
#include <algorithm>



MetropolisVariable::MetropolisVariable():
mX(0),
mSupport(eR)
{
}

MetropolisVariable::~MetropolisVariable()
{
}

void MetropolisVariable::memo()
{
    mTrace.push_back(mX);
}

void MetropolisVariable::reset()
{
    mTrace.clear();
    
    mHisto.clear();
    mChainsHistos.clear();
    
    mCorrelations.clear();
    
    mHPD.clear();
    
    mChainsResults.clear();
}

MetropolisVariable& MetropolisVariable::copy(MetropolisVariable const& origin)
{
    mX = origin.mX;
    mTrace = origin.mTrace;
    mSupport = origin.mSupport;

    mHisto = origin.mHisto;
    mChainsHistos = origin.mChainsHistos;

    mCorrelations = origin.mCorrelations;

    mHPD = origin.mHPD;
    mCredibility = origin.mCredibility;
    mThreshold = origin.mThreshold;

    mExactCredibilityThreshold = origin.mExactCredibilityThreshold;

    mResults = origin.mResults;
    mChainsResults = origin.mChainsResults;
    mIsDate = origin.mIsDate;

    return *this;
}

MetropolisVariable& MetropolisVariable::operator=( MetropolisVariable const& origin)
{
    copy(origin);
    return *this;
}

/**
 @param[in] dataSrc is the trace, with for example one million data
 @param[in] hFactor corresponds to the bandwidth factor.
 @remarks Produce a density with the area equal to 1. The smoothing is done with Hsilvermann method.
 **/
float* MetropolisVariable::generateBufferForHisto(QVector<double>& dataSrc, int numPts, double a, double b)
{
    // Work with "double" precision here !
    // Otherwise, "denum" can be very large and lead to infinity contribs!
    
    double delta = (b - a) / (numPts - 1);

    double denum = dataSrc.size();
    
    float* input = (float*) fftwf_malloc(numPts * sizeof(float));
    
    //memset(input, 0.f, numPts);
    for(int i=0; i<numPts; ++i)
        input[i]= 0.f;
    
    QVector<double>::const_iterator iter = dataSrc.begin();
    for(; iter != dataSrc.end(); ++iter)
    {
        double t = *iter;
        
        double idx = (t - a) / delta;
        double idx_under = floor(idx);
        double idx_upper = idx_under + 1.;
        
        float contrib_under = (idx_upper - idx) / denum;
        float contrib_upper = (idx - idx_under) / denum;
        
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
    return input;
}

/**
  @param hFactor corresponds to the bandwidth factor
 @param dataSrc is the trace of the raw data
 @brief the FFTW function transform the area such that the area output is the area input multiplied by fftLen. So we have to corret it.
 The result is migth be not with regular step between value.
 **/
QMap<double, double> MetropolisVariable::generateHisto(QVector<double>& dataSrc, int fftLen, double hFactor, double tmin, double tmax)
{
    int inputSize = fftLen;
    int outputSize = 2 * (inputSize / 2 + 1);

    double sigma = dataStd(dataSrc);
    QMap<double, double> result;
    if (sigma==0) {
        qDebug()<<"MetropolisVariable::generateHisto sigma=0";
        return result;
    }

     double h = hFactor * 1.06 * sigma * pow(dataSrc.size(), -1./5.);
     double a = vector_min_value(dataSrc) - 4. * h;
     double b = vector_max_value(dataSrc) + 4. * h;
    /*
     switch(mSupport)
     {
          case eR :// on R
              // nothing to do already done by default
          break;
          case eRp : // on R+
              a = 0;
          break;
          case eRm :// on R-
              b = 0;
          break;
          case eRpStar : // on R+*
              a = 0;
          break;
          case eRmStar :// on R-*
              b = 0;
          break;
          case eBounded : // on [tmin;ytmax]
              a = tmin;//qMax(tmin,a);
              b = tmax;//qMin(tmax,b);
          break;
     }
   */
    float* input = generateBufferForHisto(dataSrc, fftLen, a, b);
    float* output = (float*) fftwf_malloc(outputSize * sizeof(float));
    
    if(input != 0)
    {
        // ----- FFT -----
        
        fftwf_plan plan_forward = fftwf_plan_dft_r2c_1d(inputSize, input, (fftwf_complex*)output, FFTW_ESTIMATE);
        fftwf_execute(plan_forward);
        
        for(int i=0; i<outputSize/2; ++i) {
            double s = 2.f * M_PI * i / (b-a);
            double factor = expf(-0.5f * s * s * h * h);
            
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
          case eBounded : // on [tmin;ytmax]
              tBegin = tmin;
              tEnd = tmax;
          break;
     }


        double delta = (b - a) / fftLen;
        double tBeforeBegin;
        double vBeforeBegin;
        bool pointBeforeBegin =false;
        double tAfterEnd;
        double vAfterEnd;
        bool pointAfterEnd =false;
        for(int i=0; i<inputSize; ++i)
        {
            double t = a + (double)i * delta;
            if((t >= tBegin) && (t <= tEnd)) {
                result[t] = input[i];
             }
            else if(t<tBegin){
               pointBeforeBegin =true;
               tBeforeBegin = t;
               vBeforeBegin = input[i];
            }
            else if( t>tEnd && !pointAfterEnd ){
                pointAfterEnd =true;
                tAfterEnd = t;
                vAfterEnd = input[i];
            }
        }
        // Correct the QMap, with addition of value on the extremum tmin and tmax
        if(pointBeforeBegin && result.constFind(tBegin) == result.cend()){
            result[tBegin] = interpolate( tBegin, tBeforeBegin, result.firstKey(), vBeforeBegin, result.first() );
        }
        if(pointAfterEnd && result.constFind(tEnd) == result.cend()){
            result[tEnd] = interpolate( tEnd, result.lastKey(), tAfterEnd, result.last(), vAfterEnd );
        }

        fftwf_free(input);
        fftwf_free(output);
        
        result = equal_areas(result, 1.); // normalize the output area du to the fftw and the case (t >= tmin && t<= tmax)
    }
    return result; // return a map between a and b with a step delta = (b - a) / fftLen;
}


void MetropolisVariable::generateHistos(const QList<Chain>& chains, int fftLen, double hFactor, double tmin, double tmax)
{
    QVector<double> subFullTrace = fullRunTrace(chains);
    mHisto = generateHisto(subFullTrace, fftLen, hFactor, tmin, tmax);
 
    mChainsHistos.clear();
    for(int i=0; i<chains.size(); ++i)
    {
        QVector<double> subTrace = runTraceForChain(chains, i);
        mChainsHistos.append(generateHisto(subTrace, fftLen, hFactor, tmin, tmax));
    }
}

void MetropolisVariable::generateHPD(double threshold)
{
    if(!mHisto.isEmpty())
    {
        threshold = (threshold > 100 ? threshold = 100.0 : threshold);
        if (threshold==100.) {
            mHPD = mHisto;
            return;
        }
        threshold = (threshold < 0 ? threshold = 0.0 : threshold);
        if (threshold==0.) {
            mHPD.clear();
            return;
        }
        mThreshold = threshold;
        mHPD = create_HPD(mHisto, threshold);
        
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

void MetropolisVariable::generateCredibility(const QList<Chain>& chains, double threshold)
{
    if(!mHisto.isEmpty())
    {
        mThreshold = threshold;
        mCredibility = credibilityForTrace(fullRunTrace(chains), threshold, mExactCredibilityThreshold);
    }
}

void MetropolisVariable::generateCorrelations(const QList<Chain>& chains)
{
    int hmax = 40;
    
    for(int c=0; c<chains.size(); ++c)
    {
        // Return the acquisition part of the trace
        QVector<double> trace = runTraceForChain(chains, c);
        
        int n = trace.size();
        
        double s = sum(trace);
        double m = s / (double)n;
        double s2 = sum2Shifted(trace, -m);
        
        // Correlation pour cette chaine
        QVector<double> results;
        for(int h=0; h<hmax; ++h)
        {
            double sH = 0;
            for(QVector<double>::const_iterator iter = trace.begin(); iter != trace.begin() + (n-h); ++iter){
                sH += (*iter - m) * (*(iter + h) - m);
            }
            
            double result = sH / s2;
            results.append(result);
        }
        // Correlation ajoutée à la liste (une courbe de corrélation par chaine)
        mCorrelations.append(results);
        
        
        
        
        // Old version, about 2.24 times slower :
        
        /*double sum = 0.;
        for(int i=0; i<n; ++i)
            sum += trace[i];
        double m = sum / (double)n;
        
        double sum2 = 0;
        for(int i=0; i<n; ++i){
            double value = trace[i] - m;
            sum2 += pow(value, 2);
        }
        
        // Correlation pour cette chaine
        QVector<double> results;
        for(int h=0; h<hmax; ++h)
        {
            double sumH = 0;
            for(int i=0; i<n-h; ++i)
                sumH += (trace[i] - m) * (trace[i + h] - m);
            
            double result = sumH / sum2;
            results.append(result);
        }
        // Correlation ajoutée à la liste (une courbe de corrélation par chaine)
        mCorrelations.append(results);*/
    }
}

void MetropolisVariable::generateNumericalResults(const QList<Chain>& chains)
{
    // Results for chain concatenation
    mResults.analysis = analyseFunction(mHisto);
    mResults.quartiles = quartilesForTrace(fullRunTrace(chains));
    
    // Results for individual chains
    mChainsResults.clear();
    for(int i = 0; i<mChainsHistos.size(); ++i)
    {
        DensityAnalysis result;
        result.analysis = analyseFunction(mChainsHistos[i]);
        result.quartiles = quartilesForTrace(runTraceForChain(chains, i));
        mChainsResults.append(result);
    }
}

#pragma mark getters (no calculs)
const QMap<double, double>& MetropolisVariable::fullHisto() const
{
    return mHisto;
}

const QMap<double, double>& MetropolisVariable::histoForChain(int index) const
{
    Q_ASSERT(index < mChainsHistos.size());
    return mChainsHistos[index];
}

/**
 * @brief MetropolisVariable::fullTraceForChain
 * @param chains QList of the Chain in the Model
 * @param index
 * @return The complet trace (Burning, adaptation, acquire) corresponding to chain n°index
 */
QVector<double> MetropolisVariable::fullTraceForChain(const QList<Chain>& chains, int index)
{
    QVector<double> trace(0);
    int shift = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        unsigned long traceSize = chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter) + chains[i].mNumRunIter / chains[i].mThinningInterval;
        
        if(i == index)
        {
            /*trace.reserve(traceSize);
            
            QVector<double>::const_iterator iter = mTrace.begin() + shift;
            for(;iter != mTrace.begin() + shift + traceSize; ++iter){
                trace.append(*iter);
            }*/
            
            trace=mTrace.mid(shift , traceSize);
            
            break;
        }
        shift += traceSize;
    }
    return trace;
}

QVector<double> MetropolisVariable::fullRunTrace(const QList<Chain>& chains)
{
    QVector<double> trace(0);
    int shift = 0;
    for(int i=0; i<chains.size(); ++i)
    {
        const Chain& chain = chains[i];
        
        unsigned long burnAdaptSize = chain.mNumBurnIter + (chain.mBatchIndex * chain.mNumBatchIter);
        unsigned long traceSize = burnAdaptSize + chain.mNumRunIter / chain.mThinningInterval;
        
        //unsigned long runSize = traceSize - burnAdaptSize;

        trace=mTrace.mid(shift + burnAdaptSize, traceSize - burnAdaptSize);
        
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
QVector<double> MetropolisVariable::runTraceForChain(const QList<Chain>& chains, int index)
{
    QVector<double> trace(0);
    if (mTrace.empty()) {
        qDebug() << "mTrace empty";
        return trace ;
    }
    else
    {
        //QVector<double> trace;
        int shift = 0;
        for(int i=0; i<chains.size(); ++i) // if chains[i] is std::vector
            //    for(int i=0; i<chains.length(); ++i) // if chains[i] is QVector
        {
            const Chain& chain = chains[i];
            
            unsigned int burnAdaptSize = int (chain.mNumBurnIter + chain.mBatchIndex * chain.mNumBatchIter);
            unsigned int traceSize = int (burnAdaptSize + chain.mNumRunIter / chain.mThinningInterval);
            
            if(i == index)
            {
                //unsigned long runSize = traceSize - burnAdaptSize;
                //trace.reserve(runSize);
                
                /*QVector<double>::const_iterator iter = mTrace.begin() + shift + burnAdaptSize;
                for(;iter != mTrace.begin() + shift + traceSize; ++iter){
                    trace.append(*iter);
                }*/
                trace=mTrace.mid(shift + burnAdaptSize, traceSize - burnAdaptSize);
                
                
                break;
            }
            shift += traceSize;
        }
        return trace;
    }
}

QVector<double> MetropolisVariable::correlationForChain(int index)
{
    if(index < mCorrelations.size())
        return mCorrelations[index];
    return QVector<double>();
}


QString MetropolisVariable::resultsString(const QString& nl, const QString& noResultMessage, const QString& unit, FormatFunc formatFunc) const
{
    if(mHisto.isEmpty())
        return noResultMessage;
    
    QLocale locale;
    QString result = densityAnalysisToString(mResults, nl) + nl;
    
    result += "HPD Region (" + locale.toString(mThreshold, 'f', 1) + "%) : " + getHPDText(mHPD, mThreshold, unit, formatFunc) + nl;
    if (formatFunc) {
        result += "Credibility Interval (" + locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1) + "%) : [" + formatFunc(mCredibility.first) + ", " + formatFunc(mCredibility.second) + "] " + unit;
    }
    else {
        result += "Credibility Interval (" + locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1) + "%) : [" + DateUtils::dateToString(mCredibility.first) + ", " + DateUtils::dateToString(mCredibility.second) + "]";
    }
    
    return result;
}

QStringList MetropolisVariable::getResultsList(const QLocale locale)
{
    QStringList list;
    list << locale.toString(DateUtils::convertToAppSettingsFormat(mResults.analysis.mode));
    list << locale.toString(DateUtils::convertToAppSettingsFormat(mResults.analysis.mean));
    list << DateUtils::dateToString(mResults.analysis.stddev);
    list << locale.toString(DateUtils::convertToAppSettingsFormat(mResults.quartiles.Q1));
    list << locale.toString(DateUtils::convertToAppSettingsFormat(mResults.quartiles.Q2));
    list << locale.toString(DateUtils::convertToAppSettingsFormat(mResults.quartiles.Q3));
    list << locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1);
    list << locale.toString(DateUtils::convertToAppSettingsFormat(mCredibility.first));
    list << locale.toString(DateUtils::convertToAppSettingsFormat(mCredibility.second));
    
    QList<QPair<double, QPair<double, double> > > intervals = intervalsForHpd(mHPD, mThreshold);
    QStringList results;
    for(int i=0; i<intervals.size(); ++i)
    {
        list << locale.toString(intervals[i].first, 'f', 1);
        list << locale.toString(DateUtils::convertToAppSettingsFormat(intervals[i].second.first));
        list << locale.toString(DateUtils::convertToAppSettingsFormat(intervals[i].second.second));
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
