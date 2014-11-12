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

    return data;
}

void MetropolisVariable::generateHistos(const QList<Chain>& chains, float tmin, float tmax)
{
    // -----------------------
    //  Histos are generated based on the trace's values saved during the "Run" phase
    //  with a step of "thinning interval" between them.
    //  We have to find those values in mTrace.
    //  Note : all chains dont have the same length in the trace
    //  because the adapt phase may have stop earlier for some chains.
    // -----------------------
    
    mHistos.clear();
    
    int curTotIter = 0;
    QVector<float> subFullTrace; // All chains trace but only in run phase
    
    for(int i=0; i<chains.size(); ++i)
    {
        int traceStartIter = curTotIter;
        traceStartIter += chains[i].mNumBurnIter / chains[i].mThinningInterval;
        traceStartIter += (chains[i].mBatchIndex * chains[i].mNumBatchIter) / chains[i].mThinningInterval;
        
        QVector<float> subTrace; // Current chain trace in run phase
        for(int j=traceStartIter; j<traceStartIter + (int)(chains[i].mNumRunIter / chains[i].mThinningInterval); ++j)
        {
            subTrace.append(mTrace[j]);
            subFullTrace.append(mTrace[j]);
        }
        
        QMap<float, float> histo = generateHisto(subTrace, tmin, tmax);
        mHistos.push_back(histo);
        
        curTotIter += chains[i].mNumBurnIter / chains[i].mThinningInterval;
        curTotIter += (chains[i].mBatchIndex * chains[i].mNumBatchIter) / chains[i].mThinningInterval;
        curTotIter += chains[i].mNumRunIter / chains[i].mThinningInterval;
    }
    
    mHistoFull = generateHisto(subFullTrace, tmin, tmax);
}

const QMap<float, float>& MetropolisVariable::fullHisto() const
{
    return mHistoFull;
}

const QMap<float, float>& MetropolisVariable::histoForChain(int index) const
{
    return mHistos[index];
}

QMap<float, float> MetropolisVariable::generateFullHPD(int threshold) const
{
    const QMap<float, float>& histo = fullHisto();
    QMap<float, float> hpd = create_HPD(histo, 1, threshold);
    return hpd;
}

QMap<float, float> MetropolisVariable::generateHPDForChain(int index, int threshold) const
{
    const QMap<float, float>& histo = histoForChain(index);
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
        int traceSize = (chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter) + chains[i].mNumRunIter) / chains[i].mThinningInterval;
        
        if(i == index)
        {
            for(int j=shift; j<shift + traceSize; ++j)
            {
                int curIndex = j - shift;
                trace[curIndex] = mTrace[j];
                
                // Code utilisé quand on ne sauvait pas les 3 phases avec le même pas :
                /*int burnAdaptLen = chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter);
                
                if(curIndex < burnAdaptLen)
                    trace[curIndex] = mTrace[j];
                else
                {
                    int runIndex = burnAdaptLen + (curIndex - burnAdaptLen) * chains[i].mThinningInterval;
                    trace[runIndex] = mTrace[j];
                }*/
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

void MetropolisVariable::generateCorrelations(const QList<Chain>& chains)
{
    int hmax = 100;
    
    for(int c=0; c<chains.size(); ++c)
    {
        QMap<float, float> traceMap = traceForChain(chains, c);
        QList<float> trace = traceMap.values();
        QVector<float> results;
        float n = trace.size();
        
        for(float h=0; h<hmax; ++h)
        {
            float sum0 = 0;
            float sum1 = 0;
            float sum2 = 0;
            for(float i=0; i<n-h; ++i)
            {
                sum0 += trace[i] * trace[i];
                sum1 += trace[i] * trace[i + h];
                sum2 += trace[i];
            }
            float s0 = sum0 / n;
            float s = sum1 / (n-h);
            float m = sum2 / n;
            float result = (s -m*m) / (s0 -m*m);
            
            results.append(result);
        }
        mCorrelations.append(results);
    }
}

QVector<float> MetropolisVariable::correlationForChain(int index)
{
    return mCorrelations[index];
}

void MetropolisVariable::generateResults(const QList<Chain>& chains, float tmin, float tmax)
{
    if(mHistos.size() == 0)
        generateHistos(chains, tmin, tmax);
    
    for(int i = 0; i<mHistos.size(); ++i)
    {
        FunctionAnalysis results = analyseFunction(mHistos[i]);
        mChainsResults.append(results);
    }
    mResults = analyseFunction(mHistoFull);
}

QString MetropolisVariable::resultsText(int threshold) const
{
    QString result;
    
    //result += "CHAINS CONCATENATION\n";
    result += "POSTERIOR DISTRIBUTION (Chains Concatenation)\n";
    result += "------------------------------------------------\n";
    result += "Max : " + QString::number(mResults.max, 'f', 2) + "   ";
    result += "Mode : " + QString::number(mResults.mode, 'f', 2) + "   ";
    result += "Mean : " + QString::number(mResults.mean, 'f', 2) + "   ";
    result += "Variance : " + QString::number(mResults.variance, 'f', 2) + "\n";
    result += "HPD Intervals : " + getHPDText(threshold) + "\n";
    result += "------------------------------------------------\n";
    
    /*for(int i = 0; i<mChainsResults.size(); ++i)
    {
        result += "CHAIN " + QString::number(i) + "\n";
        result += "Max : " + QString::number(mChainsResults[i].max) + "\n";
        result += "Mode : " + QString::number(mChainsResults[i].mode) + "\n";
        result += "Mean : " + QString::number(mChainsResults[i].mean) + "\n";
        result += "Variance : " + QString::number(mChainsResults[i].variance) + "\n";
        result += "-------------------------------\n";
    }*/
    return result;
}

QString MetropolisVariable::getHPDText(int threshold) const
{
    QMap<float, float> hpd = generateFullHPD(threshold);
    QMapIterator<float, float> it(hpd);
    QList<QPair<float, float>> intervals;
    
    bool inInterval = false;
    QPair<float, float> curInterval;
    
    while(it.hasNext())
    {
        it.next();
        if(it.value() != 0 && !inInterval)
        {
            inInterval = true;
            curInterval.first = it.key();
        }
        else if(it.value() == 0 && inInterval)
        {
            inInterval = false;
            curInterval.second = it.key();
            intervals.append(curInterval);
        }
    }
    
    QStringList results;
    for(int i=0; i<intervals.size(); ++i)
    {
        results << "[" + QString::number(intervals[i].first) + ", " + QString::number(intervals[i].second) + "]";
    }
    QString result = results.join(" ");
    return result;
}