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
    
    void generateHistos(const QList<Chain>& chains, float tmin, float tmax);
    void generateCorrelations(const QList<Chain>& chains);
    void generateResults(const QList<Chain>& chains, float tmin, float tmax);

    // -----
    
    const QMap<float, float>& fullHisto() const;
    const QMap<float, float>& histoForChain(int index) const;
    
    QVector<float> fullTrace();
    QMap<float, float> traceForChain(const QList<Chain>& chains, int index);
    
    QVector<float> correlationForChain(int index);
    
    QString resultsText(int threshold) const;
    
    // -----
    
    QMap<float, float> generateFullHPD(int threshold) const;
    QMap<float, float> generateHPDForChain(int index, int threshold) const;
    
    // -----
    
private:
    QMap<float, float> generateHisto(const QVector<float>& data, float tmin, float tmax);
    QString getHPDText(int threshold) const;
    
public:
    float mX;
    QVector<float> mTrace;
    
    QList<QMap<float, float>> mHistos;
    QMap<float, float> mHistoFull;
    
    QList<QVector<float>> mCorrelations;
    
    FunctionAnalysis mResults;
    QList<FunctionAnalysis> mChainsResults;
};

#endif
