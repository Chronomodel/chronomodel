#include "MHVariable.h"
#include "StdUtilities.h"
#include "Generator.h"
#include <QDebug>


MHVariable::MHVariable():mAcceptMHBatch(0),mAcceptMHTotal(0){}
MHVariable::~MHVariable(){}

bool MHVariable::tryUpdate(const double x, const double rapport)
{
    if(rapport == 0)
        return false;
        
    if(rapport >= 1)
    {
        mX = x;
        ++mAcceptMHBatch;
        ++mAcceptMHTotal;
        return true;
    }
    else
    {
        double uniform = Generator::randomUniform();
        if(rapport >= uniform)
        {
            mX = x;
            ++mAcceptMHBatch;
            ++mAcceptMHTotal;
            return true;
        }
        else
            return false;
    }
}

void MHVariable::reset()
{
    MetropolisVariable::reset();
    mAcceptMHBatch = 0;
    mAcceptMHTotal = 0;
    mHistorySigmaMH.clear();
    mHistoryAcceptRateMH.clear();
}


QVector<float> MHVariable::acceptationForChain(int index, int numChains)
{
    //return mHistoryAcceptRateMH;
    
    int itersPerChain = mHistoryAcceptRateMH.size() / numChains;
    int start = index * itersPerChain;
    
    QVector<float> accept;
    for(int i=start; i<start + itersPerChain; ++i)
    {
        accept.push_back(mHistoryAcceptRateMH[i]);
    }
    return accept;
}

QVector<float> MHVariable::sigmaMHForChain(int index, int numChains)
{
    int itersPerChain = mHistorySigmaMH.size() / numChains;
    int start = index * itersPerChain;
    
    QVector<float> sigmaMH;
    for(int i=start; i<start + itersPerChain; ++i)
    {
        sigmaMH.push_back(mHistorySigmaMH[i]);
    }
    return sigmaMH;
}
