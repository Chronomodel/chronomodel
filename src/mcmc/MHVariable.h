#ifndef MHVARIABLE_H
#define MHVARIABLE_H

#include "MetropolisVariable.h"


class MHVariable: public MetropolisVariable
{
public:
    MHVariable();
    virtual ~MHVariable();
    
    virtual void reset();
    
    bool tryUpdate(const double x, const double rapportToTry);
    
    QVector<float> acceptationForChain(int index, int numChains);
    QVector<float> sigmaMHForChain(int index, int numChains);
    
public:
    double mSigmaMH;
    long mAcceptMHBatch;
    long mAcceptMHTotal;
    
    QVector<float> mHistorySigmaMH;
    QVector<float> mHistoryAcceptRateMH;
};

#endif
