#include "MetropolisVariable.h"
#include "StdUtilities.h"
#include "Functions.h"
#if USE_FFT
#include "fftw3.h"
#endif
#include <QDebug>


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
    
    mRawHisto.clear();
    mChainsRawHistos.clear();
    
    mCorrelations.clear();
    
    mHPD.clear();
    
    mChainsResults.clear();
}

float* MetropolisVariable::generateBufferForHisto(const QVector<double>& dataSrc, int numPts, double hFactor)
{
    // Work with double precision here !
    // Otherwise, "denum" can be very large and lead to infinity contribs!
    
    double sigma = dataStd(dataSrc);
    if(sigma == 0)
        return 0;
    
    double h = hFactor * 1.06 * sigma * pow(dataSrc.size(), -1./5.);
    
    double a = vector_min_value(dataSrc) - 4. * h;
    double b = vector_max_value(dataSrc) + 4. * h;
    
    double delta = (b - a) / (numPts - 1);
    double denum = delta * delta * dataSrc.size();
    
    float* input = (float*) fftwf_malloc(numPts * sizeof(float));
    for(int i=0; i<numPts; ++i)
        input[i]= 0.f;
    
    for(int i=0; i<(int)dataSrc.size(); ++i)
    {
        double t = dataSrc[i];
        
        double idx = (t - a) / delta;
        double idx_under = floor(idx);
        double idx_upper = idx_under + 1.;
        
        double contrib_under = (idx_upper - idx) / denum;
        double contrib_upper = (idx - idx_under) / denum;
        
        if(std::isinf(contrib_under) || std::isinf(contrib_upper))
        {
            qDebug() << "FFT input : infinity contrib!";
        }
        if(idx_under < 0 || idx_under >= numPts || idx_upper < 0 || idx_upper > numPts)
        {
            qDebug() << "FFT input : Wrong index";
        }
        if(idx_under < numPts)
            input[(int)idx_under] += (float)contrib_under;
        if(idx_upper < numPts) // This is to handle the case when matching the last point index !
            input[(int)idx_upper] += (float)contrib_upper;
    }
    return input;
}

QMap<double, double> MetropolisVariable::generateRawHisto(const QVector<double>& dataSrc, int fftLen, double tmin, double tmax)
{
    int inputSize = fftLen;
    
    double a = vector_min_value(dataSrc);
    double b = vector_max_value(dataSrc);
    double delta = (b - a) / fftLen;
    
    float* input = generateBufferForHisto(dataSrc, fftLen, 0);
    
    QMap<double, double> result;
    if(input != 0)
    {
        for(int i=0; i<inputSize; ++i)
        {
            double t = a + (double)i * delta;
            if(t >= tmin && t<= tmax)
            {
                result[t] = input[i];
            }
        }
    }
    
    fftwf_free(input);
    
    return result;
}

QMap<double, double> MetropolisVariable::generateHisto(const QVector<double>& dataSrc, int fftLen, double hFactor, double tmin, double tmax)
{
    int inputSize = fftLen;
    int outputSize = 2 * (inputSize / 2 + 1);
    
    double sigma = dataStd(dataSrc);
    double h = hFactor * 1.06 * sigma * pow(dataSrc.size(), -1.f/5.f);
    double a = vector_min_value(dataSrc) - 4.f * h;
    double b = vector_max_value(dataSrc) + 4.f * h;
    double delta = (b - a) / fftLen;
    
    float* input = generateBufferForHisto(dataSrc, fftLen, hFactor);
    float* output = (float*) fftwf_malloc(outputSize * sizeof(float));
    
    QMap<double, double> result;
    if(input != 0)
    {
        // ----- FFT -----
        
        fftwf_plan plan_forward = fftwf_plan_dft_r2c_1d(inputSize, input, (fftwf_complex*)output, FFTW_ESTIMATE);
        fftwf_execute(plan_forward);
        
        for(int i=0; i<outputSize/2; ++i)
        {
            double s = 2.f * M_PI * i / (b-a);
            double factor = exp(-0.5f * s * s * h * h);
            
            output[2*i] *= factor;
            output[2*i + 1] *= factor;
        }
        
        fftwf_plan plan_backward = fftwf_plan_dft_c2r_1d(inputSize, (fftwf_complex*)output, input, FFTW_ESTIMATE);
        fftwf_execute(plan_backward);
        
        // ----- FFT Buffer to result map -----
        
        for(int i=0; i<inputSize; ++i)
        {
            double t = a + (double)i * delta;
            if(t >= tmin && t<= tmax)
            {
                result[t] = input[i];
            }
        }
        
        fftwf_free(input);
        fftwf_free(output);
    }
    
    return result;
}

void MetropolisVariable::generateHistos(const QList<Chain>& chains, int fftLen, double hFactor, double tmin, double tmax)
{
    mChainsHistos.clear();
    
    QVector<double> subFullTrace = fullRunTrace(chains);
    mHisto = generateHisto(subFullTrace, fftLen, hFactor, tmin, tmax);
    mRawHisto = generateRawHisto(subFullTrace, fftLen, tmin, tmax);
    
    for(int i=0; i<chains.size(); ++i)
    {
        QVector<double> subTrace = runTraceForChain(chains, i);
        mChainsHistos.append(generateHisto(subTrace, fftLen, hFactor, tmin, tmax));
        mChainsRawHistos.append(generateRawHisto(subTrace, fftLen, tmin, tmax));
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
        
        // Correlation pour cette chaine
        QVector<double> results;
        
        double n = trace.size();
        
        double sum = 0;
        for(double i=0; i<n; ++i)
            sum += trace[i];
        double m = sum / n;
        
        double sum2 = 0;
        for(double i=0; i<n; ++i)
            sum2 += (trace[i] - m) * (trace[i] - m);
        
        for(double h=0; h<hmax; ++h)
        {
            double sumH = 0;
            for(double i=0; i<n-h; ++i)
                sumH += (trace[i] - m) * (trace[i + h] - m);
            
            double result = sumH / sum2;
            results.append(result);
        }
        // Correlation ajoutée à la liste (une courbe de corrélation par chaine)
        mCorrelations.append(results);
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
        QVector<double> trace =runTraceForChain(chains, i);
        result.quartiles = quartilesForTrace(trace);
        mChainsResults.append(result);
    }
}

#pragma mark getters (no calculs)
const QMap<double, double>& MetropolisVariable::fullHisto() const
{
    return mHisto;
}

const QMap<double, double>& MetropolisVariable::fullRawHisto() const
{
    return mRawHisto;
}

const QMap<double, double>& MetropolisVariable::histoForChain(int index) const
{
    return mChainsHistos[index];
}

const QMap<double, double>& MetropolisVariable::rawHistoForChain(int index) const
{
    return mChainsRawHistos[index];
}
/**
 * @brief MetropolisVariable::fullTraceForChain
 * @param chains QList of the Chain in the Model
 * @param index
 * @return The complet trace (Burning, adaptation, acquire) corresponding to chain n°index
 */
QVector<double> MetropolisVariable::fullTraceForChain(const QList<Chain>& chains, int index)
{
    QVector<double> trace;
    int shift = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        unsigned long traceSize = chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter) + chains[i].mNumRunIter / chains[i].mThinningInterval;
        
        if(i == index)
        {
            for(unsigned int j=shift; j<shift + traceSize; ++j)
                trace.append(mTrace[j]);
            break;
        }
        shift += traceSize;
    }
    return trace;
}

QVector<double> MetropolisVariable::fullRunTrace(const QList<Chain>& chains)
{
    QVector<double> trace;
    int shift = 0;
    for(int i=0; i<chains.size(); ++i)
    {
        unsigned long burnAdaptSize = chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter);
        unsigned long traceSize = burnAdaptSize + chains[i].mNumRunIter / chains[i].mThinningInterval;
        
        for(int j=(int)(shift + burnAdaptSize); j<(int)(shift + traceSize); ++j)
            trace.append(mTrace[j]); // j must be integer type while mTrace is QVector
        
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
    QVector<double> trace;
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
        unsigned int burnAdaptSize = int (chains[i].mNumBurnIter + chains[i].mBatchIndex * chains[i].mNumBatchIter);
        unsigned int traceSize = int (burnAdaptSize + chains[i].mNumRunIter / chains[i].mThinningInterval);
        
        if(i == index)
        {
            //for(unsigned long j=shift + burnAdaptSize; j<shift + traceSize; ++j) // if mTrace is std::vector
            for(int j=(int)(shift + burnAdaptSize); (j<(int)(shift + traceSize)) && (j<mTrace.length()); ++j)
                trace.append(mTrace[j]);
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



QString MetropolisVariable::resultsText(const QString& noResultMessage) const
{
    if(mHisto.isEmpty())
        return noResultMessage;
    
    QString result = densityAnalysisToString(mResults);
    
    int precision = 0;
    
    result += "HPD Region (" + QString::number(mThreshold, 'f', 1) + "%) : " + getHPDText(mHPD, mThreshold) + "<br>";
    result += "Credibility Interval (" + QString::number(mExactCredibilityThreshold * 100.f, 'f', 1) + "%) : [" + QString::number(mCredibility.first, 'f', precision) + ", " + QString::number(mCredibility.second, 'f', precision) + "]";
    
    return result;
}

void MetropolisVariable::saveToStream(QDataStream* out) // ajout PhD
{
     *out << this->mChainsHistos;
     *out << this->mChainsRawHistos;
     //out << this->mChainsResults;
     *out << this->mCorrelations;
     *out << this->mCredibility;
     *out << this->mExactCredibilityThreshold;
     *out << this->mHisto;
     *out << this->mHPD;
     *out << this->mRawHisto;
     //out << this->mResults;
     *out << this->mThreshold;
     *out << this->mTrace;
     *out << this->mX;
}
void MetropolisVariable::loadFromStream(QDataStream *in) // ajout PhD
{
    *in >> this->mChainsHistos;
    *in >> this->mChainsRawHistos;
    //in >> this->mChainsResults;
    *in >> this->mCorrelations;
    *in >> this->mCredibility;
    *in >> this->mExactCredibilityThreshold;
    *in >> this->mHisto;
    *in >> this->mHPD;
    *in >> this->mRawHisto;
    //in >> this->mResults;
    *in >> this->mThreshold;
    *in >> this->mTrace;
    *in >> this->mX;
}
