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

QMap<float, float> MetropolisVariable::generateHistoOld(const QVector<float>& dataSrc, float tmin, float tmax)
{
    // Use FFT here !
    
    // Définir un pas sur l'axe des t (delta)
    // répartir les valeurs de la trace sur cette grille avec pondération
    // Touver la puissance de 2 entière immédiatemment supérieur au nombre d'années de la plage d'étude [a, b].
    // delta = ((b-a) + 8h) / 2^r => r = ceil(logf(b-a + 8h) / logf(2))
    // eg : sur [0, 2000], on prendra 2048 points.
    // Le padding doit contenir 8h.
    // Remplir de 0 les points qui dépassent (padding)
    // Lancer la FFTW sur les points.
    // Multiplier tous les points par:
    // expf(-0.5 * s^2 * h^2)
    // s = (2 * Pi * t) / (b-a)
    // t = iteration en cours (dans l'espace des fréquences!!!)
    // h = 1.06 * écart type de la trace * nb d'éléments de la trace ^-1/5 (silvermann)
    // variance trace = traceStd()
    // Reverse FFTW => Histo lissé ! (tronquer le padding!)
    
    // Some checks :
    //qDebug() << "generate histo on trace size : " << dataSrc.size();
    
    
    // Result map
    QMap<float, float> data;
    
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
    
    //return data;
  
#if USE_FFT
    
    // Variables
    
    float numPoints = (tmax - tmin) / delta;
    
    float sigma = dataStd(dataSrc);
    float h = 1.06f * sigma * powf(dataSrc.size(), -1.f/5.f);
    
    float numPointsRequired = tmax - tmin + 8*h;
    float r = ceilf(logf(numPointsRequired) / logf(2.f));
    
    float numPointsFinal = powf(2.f, r);
    
    float deltaPoints = numPointsFinal - numPoints;
    float deltaBefore = ceilf(deltaPoints/2);
    float deltaAfter = floorf(deltaPoints/2);
    
    /*qDebug() << "sigma = " << sigma;
    qDebug() << "h = " << h;
    qDebug() << "r = " << r;
    qDebug() << "numPointsRequired = " << numPointsRequired;
    qDebug() << "numPointsFinal = " << numPointsFinal;*/
    
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
    
    //qDebug() << "FFT malloc num points : " << inputSize;
    float* input = (float*) fftwf_malloc(inputSize * sizeof(float));
	float* output = (float*) fftwf_malloc(outputSize * sizeof(float));
    
    for(int i=0; i<inputSize; ++i)
    {
        float t = tmin - delta * deltaBefore + i*delta;
        if(data.find(t) == data.end())
        {
            qDebug() << "FFT Error : Data not found at : " << t;
        }
        input[i] = data[t];
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
    
    // Remove padding
    
    for(int t=tmin-delta; t >= tmin - delta * deltaBefore; t -= delta)
        data.remove(t);
    
    for(int t=tmax; t < tmax + delta * deltaAfter; t += delta)
        data.remove(t);
    
#endif

    return data;
}

QMap<float, float> MetropolisVariable::generateHisto(const QVector<float>& dataSrc, float tmin, float tmax)
{
    Q_UNUSED(tmin);
    Q_UNUSED(tmax);
    
    // Use FFT here !
    
    QMap<int, float> data;
    QMap<float, float> result;
    
    float sigma = dataStd(dataSrc);
    float h = 1.06f * sigma * powf(dataSrc.size(), -1.f/5.f);
    
    float a = vector_min_value(dataSrc) - 4.f * h;
    float b = vector_max_value(dataSrc) + 4.f * h;
    float r = 10.f;
    float numPts = powf(2.f, r);
    float delta = (b - a) / numPts;
    
    /*qDebug() << "--------------------------";
    qDebug() << "trace std dev : " << sigma;
    qDebug() << "h : " << h;
    qDebug() << "a : " << a;
    qDebug() << "b : " << b;
    qDebug() << "r : " << r;
    qDebug() << "numPts : " << numPts;
    qDebug() << "delta : " << delta;*/
    
    float prec = 1000.f;
    
    int ai = (int) roundf(a * prec);
    int bi = (int) roundf(b * prec);
    int deltai = (int) roundf(delta * prec);
    
    /*qDebug() << "------ Int equivalent ------------";
    qDebug() << "ai : " << ai;
    qDebug() << "bi : " << bi;
    qDebug() << "deltai : " << deltai;
    qDebug() << "------------------";*/
    
    for(int ti=ai; ti<=bi; ti+=deltai)
    {
        data[ti] = 0.f;
        //qDebug() << "Preparing Grid at : " << ti;
    }
    
    float denum = delta * delta * dataSrc.size();
    
    float final_contrib = 0.f;
    for(int i=0; i<(int)dataSrc.size(); ++i)
    {
        float t = dataSrc[i];
        int ti = (int) roundf(t * prec); ;
        
        int ti_under = ai + (int)floorf((float)(ti - ai) / (float)deltai) * deltai;
        int ti_upper = ti_under + deltai;
        
        float t_under = (float)ti_under / prec;
        float t_upper = (float)ti_upper / prec;
        
        float contrib_under = ((t_upper - t) / (t_upper - t_under)) / denum;
        float contrib_upper = ((t - t_under) / (t_upper - t_under)) / denum;
        
        final_contrib += (contrib_under + contrib_upper);
        
        if(data.find(ti_under) == data.end() || data.find(ti_upper) == data.end())
        {
            qDebug() << "Error : Filling grid between points!";
        }
        
        data[ti_under] += contrib_under;
        data[ti_upper] += contrib_upper;
    }
    //qDebug() << "Final contrib : " << final_contrib * delta * delta;
    
    int index = 0;
    QMapIterator<int, float> it(data);
    while(it.hasNext())
    {
        it.next();
        float t = a + (float)index * delta;
        result[t] = it.value();
        ++index;
    }
    
    //return result;
    
#if USE_FFT
    
    int inputSize = numPts;
    int outputSize = 2 * (numPts / 2 + 1);
    
    //qDebug() << "FFT malloc num points : " << inputSize;
    float* input = (float*) fftwf_malloc(inputSize * sizeof(float));
    float* output = (float*) fftwf_malloc(outputSize * sizeof(float));
    
    // Copy all data to FFT buffer;
    int index2 = 0;
    QMapIterator<int, float> iter(data);
    while(iter.hasNext())
    {
        iter.next();
        input[index2] = iter.value();
        //qDebug() << "input = " << input[i];
        ++index2;
    }
    
    fftwf_plan plan_forward = fftwf_plan_dft_r2c_1d(numPts, input, (fftwf_complex*)output, FFTW_ESTIMATE);
    fftwf_execute(plan_forward);
    
    for(int i=0; i<outputSize/2; ++i)
    {
        float s = 2.f * M_PI * i / (b-a);
        float factor = expf(-0.5f * s * s * h * h);
        
        output[2*i] *= factor;
        output[2*i + 1] *= factor;
    }
    
    fftwf_plan plan_backward = fftwf_plan_dft_c2r_1d(numPts, (fftwf_complex*)output, input, FFTW_ESTIMATE);
    fftwf_execute(plan_backward);
    
    //qDebug() << "--------------------------";
    
    for(int i=0; i<inputSize; ++i)
    {
        float t = a + (float)i * delta;
        //qDebug() << "Final grid t = " << t << ", v = " << input[i];
        result[t] = input[i];
    }
    
#endif
    
    return result;
}

void MetropolisVariable::generateHistos(const QList<Chain>& chains, float tmin, float tmax)
{
    mChainsHistos.clear();
    
    QVector<float> subFullTrace = fullRunTrace(chains);
    mHisto = generateHisto(subFullTrace, tmin, tmax);
    
    for(int i=0; i<chains.size(); ++i)
    {
        QVector<float> subTrace = runTraceForChain(chains, i);
        mChainsHistos.append(generateHisto(subTrace, tmin, tmax));
    }
}

void MetropolisVariable::generateHPD(int threshold)
{
    if(!mHisto.isEmpty())
    {
        mThreshold = threshold;
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
        QVector<float> trace = runTraceForChain(chains, c);
        
        // Correlation pour cette chaine
        QVector<float> results;
        
        float n = trace.size();
        
        float sum = 0;
        for(float i=0; i<n; ++i)
            sum += trace[i];
        float m = sum / n;
        
        float sum2 = 0;
        for(float i=0; i<n; ++i)
            sum2 += (trace[i] - m) * (trace[i] - m);
        
        for(float h=0; h<hmax; ++h)
        {
            float sumH = 0;
            for(float i=0; i<n-h; ++i)
                sumH += (trace[i] - m) * (trace[i + h] - m);
            
            float result = sumH / sum2;
            results.append(result);
        }
        // Correlation ajoutée à la liste (une courbe de corrélation par chaine)
        mCorrelations.append(results);
    }
}

void MetropolisVariable::generateResults(const QList<Chain>& chains, float tmin, float tmax)
{
    // Results for chain concatenation
    
    mResults.analysis = analyseFunction(mHisto);
    mResults.quartiles = quartilesForTrace(fullRunTrace(chains));
    
    // Results for individual chains
    
    if(mChainsHistos.size() == 0)
        generateHistos(chains, tmin, tmax);
    
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
const QMap<float, float>& MetropolisVariable::fullHisto() const
{
    return mHisto;
}

const QMap<float, float>& MetropolisVariable::histoForChain(int index) const
{
    return mChainsHistos[index];
}

QMap<float, float> MetropolisVariable::fullTrace(int thinningInterval)
{
    QMap<float, float> trace;
    for(int i=0; i<mTrace.size(); ++i)
    {
        trace[i * thinningInterval] = mTrace[i];
    }
    return trace;
}

QMap<float, float> MetropolisVariable::fullTraceForChain(const QList<Chain>& chains, int index)
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

QVector<float> MetropolisVariable::fullRunTrace(const QList<Chain>& chains)
{
    QVector<float> trace;
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

QVector<float> MetropolisVariable::runTraceForChain(const QList<Chain>& chains, int index)
{
    QVector<float> trace;
    int shift = 0;
    for(int i=0; i<chains.size(); ++i)
    {
        int burnAdaptSize = (chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter)) / chains[i].mThinningInterval;
        int traceSize = burnAdaptSize + chains[i].mNumRunIter / chains[i].mThinningInterval;
        
        if(i == index)
        {
            for(int j=shift + burnAdaptSize; j<shift + traceSize; ++j)
                trace.append(mTrace[j]);
            break;
        }
        shift += traceSize;
    }
    return trace;
}

QVector<float> MetropolisVariable::correlationForChain(int index)
{
    if(index < mCorrelations.size())
        return mCorrelations[index];
    return QVector<float>();
}



QString MetropolisVariable::resultsText() const
{
    QString result = densityAnalysisToString(mResults);
    int precision = 0;
    
    result += "HPD Intervals (" + QString::number(mThreshold) + "%) : " + getHPDText(mHPD) + "\n";
    result += "Credibility Interval (" + QString::number(mExactCredibilityThreshold * 100.f, 'f', 2) + "%) : [" + QString::number(mCredibility.first, 'f', precision) + ", " + QString::number(mCredibility.second, 'f', precision) + "]\n";
    
    return result;
}

