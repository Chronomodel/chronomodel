#ifndef METROPOLISVARIABLE_H
#define METROPOLISVARIABLE_H

#include <QMap>
#include <QVector>
#include <QList>
#include "MCMCLoop.h"
#include "Functions.h"


class MetropolisVariable
{
public:
    MetropolisVariable();
    virtual ~MetropolisVariable();
    
    void memo();
    virtual void reset();
    
    // -----
    //  These functions are time consuming!
    // -----
    
    void generateHistos(const QList<Chain>& chains, int fftLen, float hFactor, float tmin, float tmax);
    void generateCorrelations(const QList<Chain>& chains);
    void generateHPD(int threshold);
    void generateCredibility(const QList<Chain>& chains, int threshold);
    
    // Virtual because MHVariable subclass adds some information
    virtual void generateNumericalResults(const QList<Chain>& chains);

    // -----
    // These functions do not make any calculation
    // -----
    
    const QMap<float, float>& fullHisto() const;
    const QMap<float, float>& fullRawHisto() const;
    const QMap<float, float>& histoForChain(int index) const;
    const QMap<float, float>& rawHistoForChain(int index) const;
    
    // Full trace (burn + adapt + run) as a map
    QMap<float, float> fullTrace(int thinningInterval);
    // Full trace for the chain (burn + adapt + run) as a map
    QMap<float, float> fullTraceForChain(const QList<Chain>& chains, int index);
    
    // Trace for run part as a vector
    QVector<float> fullRunTrace(const QList<Chain>& chains);
    // Trace for run part of the chain as a vector
    QVector<float> runTraceForChain(const QList<Chain>& chains, int index);
    
    QVector<float> correlationForChain(int index);
    
    // -----
    
    virtual QString resultsText() const;
    
    // -----
    
private:
    float* generateBufferForHisto(const QVector<float>& dataSrc, int numPts, float hFactor);
    QMap<float, float> bufferToMap(const float* buffer);
    QMap<float, float> generateRawHisto(const QVector<float>& data, int fftLen, float hFactor, float tmin, float tmax);
    QMap<float, float> generateHisto(const QVector<float>& data, int fftLen, float hFactor, float tmin, float tmax);
    QMap<float, float> generateHistoOld(const QVector<float>& dataSrc, float tmin, float tmax);
    
public:
    float mX;
    QVector<float> mTrace;
    
    QMap<float, float> mHisto;
    QList<QMap<float, float>> mChainsHistos;
    
    QMap<float, float> mRawHisto;
    QList<QMap<float, float>> mChainsRawHistos;
    
    QList<QVector<float>> mCorrelations;
    
    QMap<float, float> mHPD;
    QPair<float, float> mCredibility;
    int mThreshold;
    
    float mExactCredibilityThreshold;
    
    DensityAnalysis mResults;
    QList<DensityAnalysis> mChainsResults;
};

#endif
