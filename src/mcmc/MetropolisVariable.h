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
    
    void generateHistos(const QList<Chain>& chains, int fftLen, double hFactor, double tmin, double tmax);
    void generateCorrelations(const QList<Chain>& chains);
    void generateHPD(int threshold);
    void generateCredibility(const QList<Chain>& chains, int threshold);
    
    // Virtual because MHVariable subclass adds some information
    virtual void generateNumericalResults(const QList<Chain>& chains);

    // -----
    // These functions do not make any calculation
    // -----
    
    const QMap<double, double>& fullHisto() const;
    const QMap<double, double>& fullRawHisto() const;
    const QMap<double, double>& histoForChain(int index) const;
    const QMap<double, double>& rawHistoForChain(int index) const;
    
    // Full trace (burn + adapt + run) as a map
    QMap<double, double> fullTrace(int thinningInterval);
    // Full trace for the chain (burn + adapt + run) as a map
    QMap<double, double> fullTraceForChain(const QList<Chain>& chains, int index);
    
    // Trace for run part as a vector
    QVector<double> fullRunTrace(const QList<Chain>& chains);
    // Trace for run part of the chain as a vector
    QVector<double> runTraceForChain(const QList<Chain>& chains, int index);
    
    QVector<double> correlationForChain(int index);
    
    // -----
    
    virtual QString resultsText() const;
    
    // -----
    
private:
    float* generateBufferForHisto(const QVector<double>& dataSrc, int numPts, double hFactor);
    QMap<double, double> bufferToMap(const double* buffer);
    QMap<double, double> generateRawHisto(const QVector<double>& data, int fftLen, double tmin, double tmax);
    QMap<double, double> generateHisto(const QVector<double>& data, int fftLen, double hFactor, double tmin, double tmax);
    
public:
    double mX;
    QVector<double> mTrace;
    
    QMap<double, double> mHisto;
    QList<QMap<double, double>> mChainsHistos;
    
    QMap<double, double> mRawHisto;
    QList<QMap<double, double>> mChainsRawHistos;
    
    QList<QVector<double>> mCorrelations;
    
    QMap<double, double> mHPD;
    QPair<double, double> mCredibility;
    int mThreshold;
    
    double mExactCredibilityThreshold;
    
    DensityAnalysis mResults;
    QList<DensityAnalysis> mChainsResults;
};

#endif
