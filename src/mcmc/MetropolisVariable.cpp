#include "MetropolisVariable.h"
#include "StdUtilities.h"
#include "Functions.h"
#if USE_FFT
#include "fftw3.h"
#endif
#include <QDebug>


MetropolisVariable::MetropolisVariable():
mX(0),
mHistoMode(0),
mHistoMean(0),
mHistoVariance(0)
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

QMap<float, float> MetropolisVariable::generateHisto(const QVector<float>& dataSrc, float tmin, float tmax)
{
    // Use FFT here !
    
    // Définir un pas sur l'axe des t (delta)
    // répartir les valeurs de la trace sur cette grille avec pondération
    // Touver la puissance de 2 entière immédiatemment supérieur au nombre d'années de la plage d'étude [a, b].
    // delta = ((b-a) + 8h) / 2^r => r = ceil(log(b-a + 8h) / log(2))
    // eg : sur [0, 2000], on prendra 2048 points.
    // Le padding doit contenir 8h.
    // Remplir de 0 les points qui dépassent (padding)
    // Lancer la FFTW sur les points.
    // Multiplier tous les points par:
    // exp(-0.5 * s^2 * h^2)
    // s = (2 * Pi * t) / (b-a)
    // t = iteration en cours (dans l'espace des fréquences!!!)
    // h = 1.06 * écart type de la trace * nb d'éléments de la trace ^-1/5 (silvermann)
    // variance trace = traceStd()
    // Reverse FFTW => Histo lissé ! (tronquer le padding!)
    
    // Some checks :
    //qDebug() << "generate histo on trace size : " << dataSrc.size();
    
    
    // Result map
    QMap<float, float> data;
    
#if USE_FFT
    
    // Init data grid
    
    float delta = 1.f;
    for(int t=tmin; t<tmax; t+=delta)
        data[t] = 0;
    
    // Fill data grid
    
    for(int i=0; i<(int)dataSrc.size(); ++i)
    {
        float t = dataSrc[i];
        
        float t_under = floorf(t / delta) * delta;
        float t_upper = t_under + delta;
        
        float denum = delta * delta * dataSrc.size();
        
        float contrib_under = (t_upper - t) / denum;
        float contrib_upper = (t - t_under) / denum;
        
        data[t_under] += contrib_under;
        data[t_upper] += contrib_upper;
    }
    
    // Variables
    
    float numPoints = (tmax - tmin) / delta;
    
    float sigma = dataStd(dataSrc);
    float h = 1.06f * sigma * powf(dataSrc.size(), -1.f/5.f);
    
    float numPointsRequired = tmax - tmin + 8*h;
    float r = ceilf(logf(numPointsRequired) / logf(2.f));
    
    float numPointsFinal = pow(2.f, r);
    
    float deltaPoints = numPointsFinal - numPoints;
    float deltaBefore = ceilf(deltaPoints/2);
    float deltaAfter = floorf(deltaPoints/2);
    
    /*qDebug() << "------------------";
    qDebug() << "Sigma = " << sigma << ", H = " << h;
    qDebug() << "Num Points : " << numPoints;
    qDebug() << "Num Points required (+8h) : " << numPointsRequired;
    qDebug() << "Num Points final : 2^" << r << " = " << numPointsFinal;
    qDebug() << "Before padding : " << data.size();*/
    
    // Fill padding
    
    for(int t=tmin-delta; t >= tmin - delta * deltaBefore; t -= delta)
        data[t] = 0;
    
    for(int t=tmax; t < tmax + delta * deltaAfter; t += delta)
        data[t] = 0;
    
    //qDebug() << "After padding : " << data.size();
    
    // FFT
    
    int inputSize = numPointsFinal;
    int outputSize = 2 * (numPointsFinal / 2 + 1);
    
    float* input = (float*) fftwf_malloc(inputSize * sizeof(float));
	float* output = (float*) fftwf_malloc(outputSize * sizeof(float));
    
    for(int i=0; i<inputSize; ++i)
    {
        float t = tmin - delta * deltaBefore + i*delta;
        if(data.find(t) == data.end())
        {
            qDebug() << "Data not found at : " << t;
        }
        input[i] = data[t]; // * sinf(3.141592f * i / (float)inputSize);
    }
		
    
    fftwf_plan plan_forward = fftwf_plan_dft_r2c_1d(numPointsFinal, input, (fftwf_complex*)output, FFTW_ESTIMATE);
    fftwf_execute(plan_forward);
    
    for(int i=0; i<outputSize/2; ++i)
	{
        float s = 2.f * M_PI * i / outputSize;
        float factor = expf(-0.5 * s * s * h * h);
        
        output[2*i] *= factor;
        output[2*i + 1] *= factor;
	}
    
    fftwf_plan plan_backward = fftwf_plan_dft_c2r_1d(numPointsFinal, (fftwf_complex*)output, input, FFTW_ESTIMATE);
    fftwf_execute(plan_backward);
    
    for(int i=0; i<inputSize; ++i)
    {
        float t = tmin - delta * deltaBefore + i*delta;
        if(data.find(t) == data.end())
        {
            qDebug() << "Data does not correspond : " << t;
        }
		data[t] = input[i];
    }
   
#else
    
    for(int i=0; i<(int)dataSrc.size(); ++i)
    {
        float x = floor(dataSrc[i]);
        if(data.find(x) == data.end())
            data[x] = 1;
        else
            data[x] = data[x] + 1;
    }
    
#endif
    
    //data = normalize_map(data);
    return data;
}

void MetropolisVariable::generateFullHisto(const QList<Chain>& chains, float tmin, float tmax)
{
    // -----------------------
    //  The full histo is constructed using the values saved during the "Run" phase of all chains.
    //  The step of "Thinning interval" has been taken into account when saving.
    //  (only necessary values are saved)
    // -----------------------
    
    int curTotIter = 0;
    QVector<float> subTrace;
    
    for(int i=0; i<chains.size(); ++i)
    {
        int traceStartIter = curTotIter + chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter);
        
        for(int j=traceStartIter; j<traceStartIter + (int)(chains[i].mNumRunIter / chains[i].mThinningInterval); ++j)
            subTrace.append(mTrace[j]);
        
        curTotIter += chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter) + (chains[i].mNumRunIter / chains[i].mThinningInterval);
    }
    mHistoFull = generateHisto(subTrace, tmin, tmax);
}

void MetropolisVariable::generateHistos(const QList<Chain>& chains, float tmin, float tmax)
{
    mHistos.clear();
    
    // -----------------------
    //  Histos are generated based on the trace's values saved during the "Run" phase
    //  with a step of "thinning interval" between them.
    //  We have to find those values in mTrace.
    //  Note : all chains dont have the same length in the trace
    //  because the adapt phase may have stop earlier for some chains.
    // -----------------------
    
    int curTotIter = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        int traceStartIter = curTotIter + chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter);
        
        QVector<float> subTrace;
        for(int j=traceStartIter; j<traceStartIter + (int)(chains[i].mNumRunIter / chains[i].mThinningInterval); ++j)
            subTrace.append(mTrace[j]);
        
        QMap<float, float> histo = generateHisto(subTrace, tmin, tmax);
        mHistos.push_back(histo);
        
        curTotIter += chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter) + (chains[i].mNumRunIter / chains[i].mThinningInterval);
    }
}

QMap<float, float>& MetropolisVariable::fullHisto()
{
    return mHistoFull;
}

QMap<float, float>& MetropolisVariable::histoForChain(int index)
{
    return mHistos[index];
}

QMap<float, float> MetropolisVariable::generateFullHPD(int threshold)
{
    QMap<float, float>& histo = fullHisto();
    QMap<float, float> hpd = create_HPD(histo, 1, threshold);
    return hpd;
}

QMap<float, float> MetropolisVariable::generateHPDForChain(int index, int threshold)
{
    QMap<float, float>& histo = histoForChain(index);
    QMap<float, float> hpd = create_HPD(histo, 1, threshold);
    return hpd;
}

QVector<float> MetropolisVariable::fullTrace()
{
    return mTrace;
}

QMap<float, float> MetropolisVariable::traceForChain(const QList<Chain>& chains, int index)
{
    QMap<float, float> trace;
    int shift = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        int traceSize = chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter) + (chains[i].mNumRunIter / chains[i].mThinningInterval);
        
        if(i == index)
        {
            for(int j=shift; j<shift + traceSize; ++j)
            {
                int burnAdaptLen = chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter);
                int curIndex = j - shift;
                
                if(curIndex < burnAdaptLen)
                    trace[curIndex] = mTrace[j];
                else
                {
                    int runIndex = burnAdaptLen + (curIndex - burnAdaptLen) * chains[i].mThinningInterval;
                    trace[runIndex] = mTrace[j];
                }
            }
            break;
        }
        else
        {
            shift += traceSize;
        }
    }
    //qDebug() << "Trace for chain " << index << " : " << trace.size();
    return trace;
}
