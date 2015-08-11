#include "MetropolisVariable.h"
#include "StdUtilities.h"
#include "Functions.h"
#if USE_FFT
#include "fftw3.h"
#endif
#include <QDebug>
#include <algorithm>


MetropolisVariable::MetropolisVariable():
mX(0)
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

/**
 @param[in] dataSrc is the trace, with for example one million of date
 @param[in] hFactor corresponds to the bandwidth factor.
 @remarks Produice a density with the area equale to 1. The smoothing is done with Hsilvermann computed inside
 **/
float* MetropolisVariable::generateBufferForHisto(QVector<double>& dataSrc, int numPts, double hFactor)
{
    // Work with double precision here !
    // Otherwise, "denum" can be very large and lead to infinity contribs!
    
    double sigma = dataStd(dataSrc);
    if(sigma == 0)
        return 0;
    
   
    // double areaTot = 0.;
    /* for(int i=0; i<dataSrc.size(); ++i) {
         areaTot += (double)dataSrc[i];
     }
     
     qDebug()<<"MetropolisVariable::generateBufferForHisto areaTot ="<<areaTot<<"size"<<dataSrc.size();
    
   */

    
    
    
    double h = hFactor * 1.06 * sigma * pow(dataSrc.size(), -1./5.);
    
    double a = vector_min_value(dataSrc) - 4. * h;
    double b = vector_max_value(dataSrc) + 4. * h;
    
    double delta = (b - a) / (numPts - 1);
    //double denum = delta * delta * dataSrc.size();
    double denum = dataSrc.size();
    
    float* input = (float*) fftwf_malloc(numPts * sizeof(float));
    
    //memset(input, 0.f, numPts);
    for(int i=0; i<numPts; ++i)
        input[i]= 0.f;
    
    QVector<double>::const_iterator iter = dataSrc.begin();
    for(; iter != dataSrc.end(); ++iter)
    //for(int i=0; i<dataSrc.size(); ++i)
    {
        //double t = dataSrc[i];
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
    // just a check
    /*areaTot = 0.;
    for(int i=0; i<numPts; ++i) {
        areaTot += input[i];
    }
    
    qDebug()<<"MetropolisVariable::generateBufferForHisto areaTot final ="<<areaTot;
   */
    return input;
}

/**
  @param hFactor corresponds to the bandwidth factor
 @param dataSrc is the trace of the raw data
 @remarks the FFTW function transform the area such that the area output is the area input multiplied by fftLen. So we have to corret it.
 **/
QMap<double, double> MetropolisVariable::generateHisto(QVector<double>& dataSrc, int fftLen, double hFactor, double tmin, double tmax)
{
    int inputSize = fftLen;
    int outputSize = 2 * (inputSize / 2 + 1);
    
    double sigma = dataStd(dataSrc);
    QMap<double, double> result;
    if (sigma==0) return result;

    double h = hFactor * 1.06 * sigma * pow(dataSrc.size(), -1.f/5.f);
    double a = vector_min_value(dataSrc) - 4.f * h;
    double b = vector_max_value(dataSrc) + 4.f * h;
    double delta = (b - a) / fftLen;
    
    float* input = generateBufferForHisto(dataSrc, fftLen, hFactor);
    float* output = (float*) fftwf_malloc(outputSize * sizeof(float));
    /*
    double areaTot = 0.;
    for(int i=0; i<inputSize; ++i) {
        areaTot += input[i];
    }
    
        qDebug()<<"MetropolisVariable::generateHisto areaTot ="<<areaTot<<" a="<<a<<" b="<<b;
     qDebug()<<areaTot;
     */
    if(input != 0) {
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
        /*
        areaTot =0.;
        for(int i=0; i<inputSize; ++i) {
            areaTot += input[i];
        }
        
        //qDebug()<<"MetropolisVariable::generateHisto areaTot ="<<areaTot<<" a="<<a<<" b="<<b;
        //qDebug()<<areaTot/inputSize;
        */
        for(int i=0; i<inputSize; ++i)  {
            double t = a + (double)i * delta;
            if(t >= tmin && t<= tmax) {
                result[t] = input[i];
            }
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
    if (mChainsHistos.isEmpty() ) {
        for(int i=0; i<chains.size(); ++i) {
            QVector<double> subTrace = runTraceForChain(chains, i);
            mChainsHistos.append(generateHisto(subTrace, fftLen, hFactor, tmin, tmax));
        }
    }
}

void MetropolisVariable::generateHPD(double threshold)
{
    if(!mHisto.isEmpty())
    {
        
        //threshold = qMin(threshold, 100);
        //threshold = qMax(threshold, 0);
        threshold =  (threshold > 100 ? threshold = 100.0 : threshold);
        threshold = (threshold < 0 ? threshold = 0.0 : threshold);
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
        
        // No need to have HPD for al chains !
        //mChainsHPD.clear();
        //for(int i=0; i<mChainsHistos.size(); ++i)
        //  mChainsHPD.append(create_HPD(mChainsHistos[i], threshold));
    }
}

void MetropolisVariable::generateCorrelations(const QList<Chain>& chains)
{
    int hmax = 100;
    
    for(int c=0; c<chains.size(); ++c)
    {
        // Retourne la trace de la partie "acquisition" de la chaine :
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
            trace.reserve(traceSize);
            
            QVector<double>::const_iterator iter = mTrace.begin() + shift;
            for(;iter != mTrace.begin() + shift + traceSize; ++iter){
                trace.append(*iter);
            }
            
            /*for(unsigned int j=shift; j<shift + traceSize; ++j)
                trace.append(mTrace[j]);*/
            
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
        
        unsigned long runSize = traceSize - burnAdaptSize;
        trace.reserve(trace.size() + runSize);
        
        QVector<double>::const_iterator iter = mTrace.begin() + shift + burnAdaptSize;
        for(;iter != mTrace.begin() + shift + traceSize; ++iter){
            trace.append(*iter);
        }
        
        /*for(int j=(int)(shift + burnAdaptSize); j<(int)(shift + traceSize); ++j)
            trace.append(mTrace[j]); // j must be integer type while mTrace is QVector*/
        
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
        qDebug() << "mtrace vide";
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
                unsigned long runSize = traceSize - burnAdaptSize;
                trace.reserve(runSize);
                
                QVector<double>::const_iterator iter = mTrace.begin() + shift + burnAdaptSize;
                for(;iter != mTrace.begin() + shift + traceSize; ++iter){
                    trace.append(*iter);
                }
                
                /*for(int j=(int)(shift + burnAdaptSize); (j<(int)(shift + traceSize)) && (j<mTrace.length()); ++j)
                    trace.append(mTrace[j]);*/
                
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


QString MetropolisVariable::resultsText(const QString& noResultMessage, const QString& unit, FormatFunc formatFunc) const
{
    if(mHisto.isEmpty())
        return noResultMessage;
    
    QString result = densityAnalysisToString(mResults);
    
    int precision = 1;
    
    result += "HPD Region (" + QString::number(mThreshold, 'f', 1) + "%) : " + getHPDText(mHPD, mThreshold, unit, formatFunc) + "<br>";
    if (formatFunc) {
        result += "Credibility Interval (" + QString::number(mExactCredibilityThreshold * 100.f, 'f', 1) + "%) : [" + formatFunc(mCredibility.first) + ", " + formatFunc(mCredibility.second) + "] " + unit;
    }
    else {
        result += "Credibility Interval (" + QString::number(mExactCredibilityThreshold * 100.f, 'f', 1) + "%) : [" + QString::number(mCredibility.first, 'f', precision) + ", " + QString::number(mCredibility.second, 'f', precision) + "]";
    }
    
    return result;
}

void MetropolisVariable::saveToStream(QDataStream* out) // ajout PhD
{
     *out << this->mChainsHistos;
     //out << this->mChainsResults;
     *out << this->mCorrelations;
     *out << this->mCredibility;
     *out << this->mExactCredibilityThreshold;
     *out << this->mHisto;
     *out << this->mHPD;
     //out << this->mResults;
     *out << this->mThreshold;
     *out << this->mTrace;
     *out << this->mX;
}
void MetropolisVariable::loadFromStream(QDataStream *in) // ajout PhD
{
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
    *in >> this->mX;
}
