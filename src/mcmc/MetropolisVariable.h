#ifndef METROPOLISVARIABLE_H
#define METROPOLISVARIABLE_H

#include <QMap>
#include <QVector>
#include <QList>
#include "MCMCLoop.h"
#include "Functions.h"


struct Quartiles{
    float Q1;
    float Q2;
    float Q3;
};

struct MetropolisResult
{
    Quartiles quartiles;
    float max = 0.;
    float mode = 0.;
    float mean = 0.;
    float stddev = 0.;
};

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
    
    void generateHistos(const QList<Chain>& chains, float tmin, float tmax);
    void generateCorrelations(const QList<Chain>& chains);
    void generateHPD(int threshold);
    void generateCredibility(const QList<Chain>& chains, int threshold);
    void generateResults(const QList<Chain>& chains, float tmin, float tmax);

    // -----
    // These functions do not make any calculation
    // -----
    
    const QMap<float, float>& fullHisto() const;
    const QMap<float, float>& histoForChain(int index) const;
    
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
    
    QString resultsText() const;
    
    static Quartiles quartilesForTrace(const QVector<float>& trace);
    static QPair<float, float> credibilityForTrace(const QVector<float>& trace, int threshold);
    static QString getHPDText(const QMap<float, float>& hpd);
    static QList<QPair<float, float>> intervalsForHpd(const QMap<float, float>& hpd);
    
    // -----
    
private:
    QMap<float, float> generateHisto(const QVector<float>& data, float tmin, float tmax);
    
public:
    float mX;
    QVector<float> mTrace;
    
    QMap<float, float> mHisto;
    QList<QMap<float, float>> mChainsHistos;
    
    QList<QVector<float>> mCorrelations;
    
    QMap<float, float> mHPD;
    QPair<float, float> mCredibility;
    int mThreshold;
    //QList<QMap<float, float>> mChainsHPD;
    
    MetropolisResult mResults;
    QList<MetropolisResult> mChainsResults;
};

#endif
