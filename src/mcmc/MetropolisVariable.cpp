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

void MetropolisVariable::generateHPD(int threshold)
{
    if(!mHisto.isEmpty())
    {
        mThreshold = threshold;
        threshold = qMin(threshold, 100);
        threshold = qMax(threshold, 0);
        mHPD = create_HPD(mHisto, 1, threshold);
        
        // No need to have HPD for al chains !
        //mChainsHPD.clear();
        //for(int i=0; i<mChainsHistos.size(); ++i)
          //  mChainsHPD.append(create_HPD(mChainsHistos[i], 1, threshold));
    }
    else
    {
        qDebug() << "ERROR : Cannot generate HPD on empty histo";
    }
}

void MetropolisVariable::generateCredibility(const QList<Chain>& chains, int threshold)
{
    if(!mHisto.isEmpty())
    {
        mThreshold = threshold;
        mCredibility = credibilityForTrace(fullRunTrace(chains), threshold, mExactCredibilityThreshold);
        
        // No need to have HPD for al chains !
        //mChainsHPD.clear();
        //for(int i=0; i<mChainsHistos.size(); ++i)
        //  mChainsHPD.append(create_HPD(mChainsHistos[i], 1, threshold));
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

void MetropolisVariable::generateNumericalResults(const QList<Chain>& chains, const ProjectSettings& settings)
{
    // Results for chain concatenation
    mResults.analysis = analyseFunction(mHisto);
    mResults.quartiles = quartilesForTrace(fullRunTrace(chains), settings.mStep);
    
    // Results for individual chains
    mChainsResults.clear();
    for(int i = 0; i<mChainsHistos.size(); ++i)
    {
        DensityAnalysis result;
        result.analysis = analyseFunction(mChainsHistos[i]);
        result.quartiles = quartilesForTrace(runTraceForChain(chains, i), settings.mStep);
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

QMap<double, double> MetropolisVariable::fullTrace(int thinningInterval)
{
    QMap<double, double> trace;
    for(int i=0; i<mTrace.size(); ++i)
    {
        trace[i * thinningInterval] = mTrace[i];
    }
    return trace;
}

QMap<double, double> MetropolisVariable::fullTraceForChain(const QList<Chain>& chains, int index)
{
    QMap<double, double> trace;
    int shift = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        int traceSize = (chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter) + chains[i].mNumRunIter) / chains[i].mThinningInterval;
        
        if(i == index)
        {
            for(int j=shift; j<shift + traceSize; ++j)
            {
                int curIndex = j - shift;
                trace[curIndex * chains[i].mThinningInterval] = mTrace[j];
            }
            break;
        }
        else
        {
            shift += traceSize;
        }
    }
    return trace;
}

QVector<double> MetropolisVariable::fullRunTrace(const QList<Chain>& chains)
{
    QVector<double> trace;
    int shift = 0;
    for(int i=0; i<chains.size(); ++i)
    {
        int burnAdaptSize = (chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter)) / chains[i].mThinningInterval;
        int traceSize = burnAdaptSize + chains[i].mNumRunIter / chains[i].mThinningInterval;
        
        for(int j=shift + burnAdaptSize; j<shift + traceSize; ++j)
            trace.append(mTrace[j]);
        
        shift += traceSize;
    }
    return trace;
}

QVector<double> MetropolisVariable::runTraceForChain(const QList<Chain>& chains, int index)
{
    QVector<double> trace;
    int shift = 0;
    for(int i=0; i<chains.size(); ++i)
    {
        int burnAdaptSize = (chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter)) / chains[i].mThinningInterval;
        int traceSize = burnAdaptSize + chains[i].mNumRunIter / chains[i].mThinningInterval;
        
        if(i == index)
        {
            for(int j=shift + burnAdaptSize; j<shift + traceSize; ++j)
            {
                trace.append(mTrace[j]);
            }
            break;
        }
        shift += traceSize;
    }
    return trace;
}

QVector<double> MetropolisVariable::correlationForChain(int index)
{
    if(index < mCorrelations.size())
        return mCorrelations[index];
    return QVector<double>();
}



QString MetropolisVariable::resultsText() const
{
    QString result = densityAnalysisToString(mResults);
    int precision = 0;
    
    result += "HPD Intervals (" + QString::number(mThreshold) + "%) : " + getHPDText(mHPD) + "\n";
    result += "Credibility Interval (" + QString::number(mExactCredibilityThreshold * 100.f, 'f', 2) + "%) : [" + QString::number(mCredibility.first, 'f', precision) + ", " + QString::number(mCredibility.second, 'f', precision) + "]\n";
    
    return result;
}

