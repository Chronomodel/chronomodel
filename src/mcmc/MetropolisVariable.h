#ifndef METROPOLISVARIABLE_H
#define METROPOLISVARIABLE_H

#include <QMap>
#include <QVector>
#include <QList>
#include "MCMCLoop.h"


class MetropolisVariable
{
public:
    MetropolisVariable();
    virtual ~MetropolisVariable();
    
    void memo();
    virtual void reset();
    
    void generateHPD(const float classe, const float threshold);
    
    
    void generateFullHisto(const QList<Chain>& chains, float tmin, float tmax);
    void generateHistos(const QList<Chain>& chains, float tmin, float tmax);

    QMap<float, float>& fullHisto();
    QMap<float, float>& histoForChain(int index);
    
    QMap<float, float> generateFullHPD(int threshold);
    QMap<float, float> generateHPDForChain(int index, int threshold);
    
    QVector<float> fullTrace();
    QMap<float, float> traceForChain(const QList<Chain>& chains, int index);
    
private:
    QMap<float, float> generateHisto(const QVector<float>& data, float tmin, float tmax);
    
public:
    float mX;
    QVector<float> mTrace;
    
    QList<QMap<float, float>> mHistos;
    QMap<float, float> mHistoFull;
    
    float mHistoMode;
    float mHistoMean;
    float mHistoVariance;
};

#endif
