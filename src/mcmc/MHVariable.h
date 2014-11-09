#ifndef MHVARIABLE_H
#define MHVARIABLE_H

#include "MetropolisVariable.h"


class MHVariable: public MetropolisVariable
{
public:
    MHVariable();
    virtual ~MHVariable();
    
    virtual void reset();
    
    float getCurrentAcceptRate();
    void saveCurrentAcceptRate();
    
    bool tryUpdate(const double x, const double rapportToTry);
    
    QMap<float, float> acceptationForChain(const QList<Chain>& chains, int index);
    QVector<float> sigmaMHForChain(int index, int numChains);
    
public:
    double mSigmaMH;
    
    QVector<bool> mLastAccepts;
    int mLastAcceptsLength;
    
    QVector<float> mHistorySigmaMH;
    QVector<float> mHistoryAcceptRateMH;
};

#endif
