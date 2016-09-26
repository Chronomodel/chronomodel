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


MetropolisVariable::MetropolisVariable(QObject *parent):QObject(parent),
mX(0),
mRawTrace(0),
mFormatedTrace(0),
mSupport(eR),
mFormat(DateUtils::eNumeric),
mExactCredibilityThreshold(0),
mfftLenUsed(-1),
mBandwidthUsed(-1),
mThresholdUsed(-1),
mtminUsed(0),
mtmaxUsed(0)
{
    // will not throw exception,(std::nothrow) in C++ Programmin Language B. Stroustrup Section 19.4.5
   /* mRawTrace = new QVector<float>();
    mFormatedTrace = new QVector<float>();
*/
    mCredibility = QPair<float,float>();
    mHPD = QMap<float,float>();
    QObject::connect(this, &MetropolisVariable::formatChanged, this, &MetropolisVariable::updateFormatedTrace);
}

MetropolisVariable::~MetropolisVariable()
{

    QObject::disconnect(this, &MetropolisVariable::formatChanged, this, &MetropolisVariable::updateFormatedTrace);
}

void MetropolisVariable::memo()
{
    mRawTrace->push_back(mX);
}

void MetropolisVariable::reset()
{
    /*if (!mRawTrace->isEmpty())
        mRawTrace->clear();
    if (!mFormatedTrace->isEmpty())
        mFormatedTrace->clear();*/
    mRawTrace = new QVector<float>();
    mFormatedTrace = new QVector<float>();
    
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
    if (!mRawTrace)
        return;

    mFormatedTrace->clear();
    if (mRawTrace->size()== 0)
       return;

   // mRawTrace.squeeze(); // just cleaning, must be somewhere else to optimize
    mFormatedTrace->reserve(mRawTrace->size());

    if(mFormat == DateUtils::eNumeric)
        mFormatedTrace = mRawTrace;
    else {
        mFormatedTrace->resize(mRawTrace->size());
        std::transform(mRawTrace->cbegin(),mRawTrace->cend(),mFormatedTrace->begin(),[this](const float i){return DateUtils::convertToFormat(i,this->mFormat);});
       /* mFormatedTrace.reserve(mRawTrace.size());
        QVector<double>::const_iterator iter = mRawTrace.constBegin();
        while(iter!= mRawTrace.constEnd()){
            mFormatedTrace.append(DateUtils::convertToFormat(*iter, mFormat));
            ++iter;
        }*/
    }

}

/**
 @param[in] dataSrc is the trace, with for example one million data
 @remarks Produce a density with the area equal to 1. The smoothing is done with Hsilvermann method.
 **/
void MetropolisVariable::generateBufferForHisto(float* input, const QVector<float> &dataSrc, const int numPts, const float a, const float b)
{
    // Work with "double" precision here !
    // Otherwise, "denum" can be very large and lead to infinity contribs!
    
    const float delta = (b - a) / (numPts - 1);

    const float denum = dataSrc.size();
    
    //float* input = (float*) fftwf_malloc(numPts * sizeof(float));
    
    //memset(input, 0.f, numPts);
    for(int i=0; i<numPts; ++i)
        input[i]= 0.f;
    
    QVector<float>::const_iterator iter = dataSrc.cbegin();
    for(; iter != dataSrc.cend(); ++iter) {
        const float t = *iter;
        
        const float idx = (t - a) / delta;
        const float idx_under = floor(idx);
        const float idx_upper = idx_under + 1.;
        
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
  @param[in] bandwidth corresponds to the bandwidth factor
  @param[in] dataSrc is the trace of the raw data
  @brief the FFTW function transform the area such that the area output is the area input multiplied by fftLen. So we have to corret it.
  The result is migth be not with regular step between value.
 **/
QMap<float, float> MetropolisVariable::generateHisto(const QVector<float>& dataSrc, const int fftLen, const float bandwidth, const float tmin, const float tmax)
{
    mfftLenUsed = fftLen;
    mBandwidthUsed = bandwidth;
    mtmaxUsed = tmax;
    mtminUsed = tmin;

    const int inputSize = fftLen;
    const int outputSize = 2 * (inputSize / 2 + 1);

    const float sigma = dataStd(dataSrc);
    QMap<float, float> result;



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
    // DEBUG
   /*QVector<double> histo = vector_to_histo(dataSrc,tmin,tmax,fftLen);
    const double step = (tmax-tmin)/fftLen;
    result = vector_to_map(histo, tmin, tmax, step);
    return result;
    */// /// DEBUG

     const float h = bandwidth * sigma * pow(dataSrc.size(), -1.f/5.f);
     const float a = vector_min_value(dataSrc) - 4.f * h;
     const float b = vector_max_value(dataSrc) + 4.f * h;

     float* input = (float*) fftwf_malloc(fftLen * sizeof(float));
     generateBufferForHisto(input, dataSrc, fftLen, a, b);

     float* output = (float*) fftwf_malloc(outputSize * sizeof(float));
    
    if(input != 0) {
        // ----- FFT -----
        // http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
        //https://jperalta.wordpress.com/2006/12/12/using-fftw3/
        fftwf_plan plan_forward = fftwf_plan_dft_r2c_1d(inputSize, input, (fftwf_complex*)output, FFTW_ESTIMATE);
        fftwf_execute(plan_forward);

        for(int i=0; i<outputSize/2; ++i) {
            const float s = 2.f * M_PI * i / (b-a);
            const float factor = expf(-0.5f * s * s * h * h);

            output[2*i] *= factor;
            output[2*i + 1] *= factor;
        }

        fftwf_plan plan_backward = fftwf_plan_dft_c2r_1d(inputSize, (fftwf_complex*)output, input, FFTW_ESTIMATE);
        fftwf_execute(plan_backward);

        // ----- FFT Buffer to result map -----

        float tBegin = a;
        float tEnd = b;
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
        const float delta = (b - a) / fftLen;

        for(int i=0; i<inputSize; ++i) {
             const float t = a + (float)i * delta;
             result[t] = input[i];
        }

        result = getMapDataInRange(result,tBegin,tEnd);


        fftwf_free(input);
        fftwf_free(output);
        input = 0;
        output = 0;
        fftwf_destroy_plan(plan_forward);
        fftwf_destroy_plan(plan_backward);

        result = equal_areas(result, 1.f); // normalize the output area du to the fftw and the case (t >= tmin && t<= tmax)
    }
    return result; // return a map between a and b with a step delta = (b - a) / fftLen;
}


void MetropolisVariable::generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const float bandwidth, const float tmin, const float tmax)
{
    const QVector<float> subFullTrace (fullRunTrace(chains));
    mHisto = generateHisto(subFullTrace, fftLen, bandwidth, tmin, tmax);

    mChainsHistos.clear();
    for (int i=0; i<chains.size(); ++i) {
        const QVector<float> subTrace ( runFormatedTraceForChain(chains, i));
        if (!subTrace.isEmpty()) {
            const QMap<float,float> histo (generateHisto(subTrace, fftLen, bandwidth, tmin, tmax) );
            mChainsHistos.append(histo);
        }
    }
}
void MetropolisVariable::memoHistoParameter(const int fftLen, const float bandwidth, const float tmin, const float tmax)
{
    mfftLenUsed = fftLen;
    mBandwidthUsed = bandwidth;
    mtminUsed = tmin;
    mtmaxUsed = tmax;
}

bool MetropolisVariable::HistoWithParameter(const int fftLen, const float bandwidth, const float tmin, const float tmax)
{
   return ((mfftLenUsed == fftLen) &&  (mBandwidthUsed == bandwidth) &&   (mtminUsed == tmin) &&  (mtmaxUsed == tmax) ? true: false);
}

void MetropolisVariable::generateHPD(const float threshold)
{
    if(!mHisto.isEmpty())
    {
        const float thresh = qBound(0.0f, threshold, 100.0f);
        if (thresh == 100.f) {
            mHPD = mHisto;
            return;
        }
        //threshold = (threshold < 0 ? threshold = 0.0 : threshold);
        if (thresh == 0.f) {
            mHPD.clear();
            return;
        }
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
        mHPD = QMap<float,float>();

        qDebug() << "WARNING : Cannot generate HPD on empty histo in MetropolisVariable::generateHPD";
    }
}

void MetropolisVariable::generateCredibility(const QList<ChainSpecs> &chains, float threshold)
{
    if (!mHisto.isEmpty())
        mCredibility = credibilityForTrace(fullRunTrace(chains), threshold, mExactCredibilityThreshold,"Compute credibility for "+getName());
    else
        mCredibility = QPair<float,float>();

}

void MetropolisVariable::generateCorrelations(const QList<ChainSpecs>& chains)
{
    const int hmax = 40;
    if(!mCorrelations.isEmpty())
        mCorrelations.clear();
    mCorrelations.reserve(chains.size());


    for (int c=0; c<chains.size(); ++c) {
        // Return the acquisition part of the trace
        const QVector<float> trace (runRawTraceForChain(chains, c));
        if(trace.size()<hmax)
            continue;
        QVector<float> results;
        results.reserve(hmax);

        const int n = trace.size();
        
        const float s = sum(trace);
        const float m = s / (float)n;
        const float s2 = sum2Shifted(trace, -m);
        
        // Correlation pour cette chaine

        for (int h=0; h<hmax; ++h) {
            float sH = 0;
            for (QVector<float>::const_iterator iter = trace.cbegin(); iter != trace.cbegin() + (n-h); ++iter) {
                sH += (*iter - m) * (*(iter + h) - m);
            }
            
            const float result = sH / s2;
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
    for (int i = 0; i<mChainsHistos.size(); ++i) {
        DensityAnalysis result;
        result.analysis = analyseFunction(mChainsHistos.at(i));
        result.quartiles = quartilesForTrace(runFormatedTraceForChain(chains, i));
        mChainsResults.append(result);
    }
}

#pragma mark getters (no calculs)
QMap<float, float>& MetropolisVariable::fullHisto()
{
    return mHisto;
}

QMap<float, float>& MetropolisVariable::histoForChain(const int index)
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
QVector<float> MetropolisVariable::fullTraceForChain(const QList<ChainSpecs>& chains, const int index)
{
   // const int reserveSize = (int) ceil( chains.at(index).mNumBurnIter + (chains.at(index).mBatchIndex * chains.at(index).mNumBatchIter) + (chains.at(index).mNumRunIter /chains.at(index).mThinningInterval ) );
   // trace.reserve(reserveSize);

    //QVector<float> trace(reserveSize);
    std::vector<float> trace;
    int shift = 0;
    
    for (int i=0; i<chains.size(); ++i) {
        const int traceSize = chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter) + (int) (chains.at(i).mNumRunIter / chains.at(i).mThinningInterval);
        trace.resize(traceSize);
        if (i == index) {
            std::copy(mFormatedTrace->begin()+shift, mFormatedTrace->begin()+shift+traceSize, trace.begin());
            //trace = mFormatedTrace.mid(shift , traceSize);
            break;
        }
        shift += traceSize;
    }
    return QVector<float>::fromStdVector(trace);
}

QVector<float> MetropolisVariable::fullRunTrace(const QList<ChainSpecs>& chains)
{

    // calcul reserve space
    int reserveSize = 0;

    for (const ChainSpecs& chain:chains)
        reserveSize += (int) ceil(chain.mNumRunIter / chain.mThinningInterval);

   // trace.resize(reserveSize);
   // trace.reserve(reserveSize);
    QVector<float> trace(reserveSize);

    int shift = 0;
    int shiftTrace = 0;
    for (int i = 0; i<chains.size(); ++i) {
        const ChainSpecs& chain = chains.at(i);
        
        const int burnAdaptSize = (int) (chain.mNumBurnIter + (chain.mBatchIndex * chain.mNumBatchIter));
        const int runTraceSize = (int)(chain.mNumRunIter / chain.mThinningInterval);
        const int firstRunPosition = shift + burnAdaptSize;
        std::copy(mRawTrace->begin()+ firstRunPosition, mRawTrace->begin() + firstRunPosition + runTraceSize, trace.begin()+ shiftTrace);

        //trace += mFormatedTrace.mid(firstRunPosition, runTraceSize);
        
       // shift += burnAdaptSize + runTraceSize ;
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
QVector<float> MetropolisVariable::runRawTraceForChain(const QList<ChainSpecs> &chains, const int index)
{

    if (mRawTrace->isEmpty()) {
        qDebug() << "in MetropolisVariable::runRawTraceForChain -> mRawTrace empty";
        return QVector<float>() ;
        
    } else {

        int shift = 0;
        //const int reserveSize = (int) ceil(chains.at(index).mNumRunIter /chains.at(index).mThinningInterval );
        //QVector<float> trace;//(reserveSize);
std::vector<float> trace;

        for (int i=0; i<chains.size(); ++i) {
            const ChainSpecs& chain = chains.at(i);
            
            const int burnAdaptSize = int (chain.mNumBurnIter + chain.mBatchIndex * chain.mNumBatchIter);
            const int traceSize = (int)(chain.mNumRunIter / chain.mThinningInterval);

            if (i == index) {
                // code in case of trace is std::vector
                trace.resize(traceSize);
                std::copy(mRawTrace->begin()+shift+ burnAdaptSize, mRawTrace->begin()+shift+ burnAdaptSize +traceSize, trace.begin());
                // code in case of trace is QVector
                //trace = mRawTrace.mid(shift + burnAdaptSize, traceSize );

                break;
            }
            shift += burnAdaptSize + traceSize;
        }
        //return trace;
        return QVector<float>::fromStdVector(trace);
    }
}

QVector<float> MetropolisVariable::runFormatedTraceForChain(const QList<ChainSpecs> &chains, const int index)
{
   // QVector<float> trace(0);
    std::vector<float> trace;
    if (mFormatedTrace->empty()) {
        qDebug() << "in MetropolisVariable::runFormatedTraceForChain -> mFormatedTrace empty";
        return QVector<float>(0);//trace ;
    }  else  {
        //const int reserveSize = (int) ceil(chains.at(index).mNumRunIter /chains.at(index).mThinningInterval );
        //trace.resize(reserveSize);

        int shift = 0;
        for (int i=0; i<chains.size(); ++i)  {
            const ChainSpecs& chain = chains.at(i);

            const int burnAdaptSize = (int) (chain.mNumBurnIter + chain.mBatchIndex * chain.mNumBatchIter);
            const int traceSize = (int) (chain.mNumRunIter / chain.mThinningInterval);

            if (i == index) {
                trace.resize(traceSize);
                std::copy(mRawTrace->begin()+shift+ burnAdaptSize, mRawTrace->begin()+shift + burnAdaptSize +traceSize, trace.begin());
                //trace = mFormatedTrace.mid(shift + burnAdaptSize, traceSize);
                break;
            }
            shift += traceSize + burnAdaptSize ;
        }
        return QVector<float>::fromStdVector(trace);
    }
}


QVector<float> MetropolisVariable::correlationForChain(const int index)
{
    if (index < mCorrelations.size())
        return mCorrelations.at(index);
    return QVector<float>();
}


QString MetropolisVariable::resultsString(const QString& nl, const QString& noResultMessage, const QString& unit, FormatFunc formatFunc) const
{
    if (mHisto.isEmpty())
        return noResultMessage;
    
    const QLocale locale;
    QString result = densityAnalysisToString(mResults, nl) + nl;
    
    if (!mHPD.isEmpty())
        result += "HPD Region (" + locale.toString(mThresholdUsed, 'f', 1) + "%) : " + getHPDText(mHPD, mThresholdUsed, unit, formatFunc) + nl;

    
    if (mCredibility != QPair<float,float>()) {
        if (formatFunc)
            result += "Credibility Interval (" + locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1) + "%) : [" + formatFunc(mCredibility.first) + ", " + formatFunc(mCredibility.second) + "] " + unit;
        else
            result += "Credibility Interval (" + locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1) + "%) : [" + DateUtils::dateToString(mCredibility.first) + ", " + DateUtils::dateToString(mCredibility.second) + "]";

   }
   return result;
}

QStringList MetropolisVariable::getResultsList(const QLocale locale, const bool withDateFormat)
{
    QStringList list;
    if (withDateFormat) {

        list << locale.toString(mResults.analysis.mode);
        list << locale.toString(mResults.analysis.mean);
        list << DateUtils::dateToString(mResults.analysis.stddev);
        list << locale.toString(mResults.quartiles.Q1);
        list << locale.toString(mResults.quartiles.Q2);
        list << locale.toString(mResults.quartiles.Q3);
        list << locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1);
        list << locale.toString(mCredibility.first);
        list << locale.toString(mCredibility.second);

        const QList<QPair<float, QPair<float, float> > > intervals = intervalsForHpd(mHPD, mThresholdUsed);
        QStringList results;
        for (int i=0; i<intervals.size(); ++i) {
            list << locale.toString(intervals.at(i).first, 'f', 1);
            list << locale.toString(intervals.at(i).second.first);
            list << locale.toString(intervals.at(i).second.second);
        }

    } else {
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.analysis.mode));
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.analysis.mean));
        list << DateUtils::dateToString(mResults.analysis.stddev);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.quartiles.Q1));
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.quartiles.Q2));
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mResults.quartiles.Q3));
        list << locale.toString(mExactCredibilityThreshold * 100.f, 'f', 1);
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mCredibility.first));
        list << locale.toString(DateUtils::convertFromAppSettingsFormat(mCredibility.second));

        const QList<QPair<float, QPair<float, float> > > intervals = intervalsForHpd(mHPD, mThresholdUsed);
        QStringList results;
        for (int i=0; i<intervals.size(); ++i)  {
            list << locale.toString(intervals.at(i).first, 'f', 1);
            list << locale.toString(DateUtils::convertFromAppSettingsFormat(intervals.at(i).second.first));
            list << locale.toString(DateUtils::convertFromAppSettingsFormat(intervals.at(i).second.second));
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
    };

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
    };

   // stream << data.mRawTrace;
    stream << (quint32) data.mRawTrace->size();
    for(QVector<float>::const_iterator v = data.mRawTrace->cbegin(); v != data.mRawTrace->cend(); ++v)
        stream << *v;

    qDebug()<<"&operator<<( QDataStream &stream, const MetropolisVariable &data )"<<data.mRawTrace->size();

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
   };

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
   };

    quint32 siz;
    stream >> siz;
    if (data.mRawTrace)
        data.mRawTrace->clear();
    else
        data.mRawTrace = new QVector<float>();
    data.mRawTrace->reserve(siz);

    for(quint32 i = 0; i < siz; ++i) {
        float v;
        stream >> v;
        data.mRawTrace->push_back(v);
    }
    // regeneration of this->mFormatedTrace
    if (data.mFormatedTrace)
        data.mFormatedTrace->clear();
    else
        data.mFormatedTrace = new QVector<float>();
    data.updateFormatedTrace();

    return stream;

}

