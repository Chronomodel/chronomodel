#include "MHVariable.h"
#include "StdUtilities.h"
#include "Generator.h"
#include <QDebug>


MHVariable::MHVariable():mLastAcceptsLength(0){}
MHVariable::~MHVariable(){}

bool MHVariable::tryUpdate(const double x, const double rapport)
{
    if(rapport == 0)
        return false;
    
    if(mLastAccepts.length() >= mLastAcceptsLength)
        mLastAccepts.removeAt(0);
    
    if(rapport >= 1)
    {
        mX = x;
        mLastAccepts.append(true);
        return true;
    }
    else
    {
        double uniform = Generator::randomUniform();
        if(rapport >= uniform)
        {
            mX = x;
            mLastAccepts.append(true);
            return true;
        }
        else
        {
            mLastAccepts.append(false);
            return false;
        }
    }
}

void MHVariable::reset()
{
    MetropolisVariable::reset();
    mLastAccepts.clear();
    mHistorySigmaMH.clear();
    mHistoryAcceptRateMH.clear();
}

float MHVariable::getCurrentAcceptRate()
{
    float sum = 0.f;
    for(int i=0; i<mLastAccepts.length(); ++i)
        sum += mLastAccepts[i] ? 1.f : 0.f;
    return sum / (float)mLastAccepts.length();
}

void MHVariable::saveCurrentAcceptRate()
{
    float rate = 100.f * getCurrentAcceptRate();
    mHistoryAcceptRateMH.push_back(rate);
    mHistorySigmaMH.push_back(mSigmaMH);
}

QMap<float, float> MHVariable::acceptationForChain(const QList<Chain>& chains, int index)
{
    QMap<float, float> accept;
    int shift = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        int acceptSize = chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter) + (chains[i].mNumRunIter / chains[i].mThinningInterval);
        
        if(i == index)
        {
            for(int j=shift; j<shift + acceptSize; ++j)
            {
                int burnAdaptLen = chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter);
                int curIndex = j - shift;
                
                if(curIndex < burnAdaptLen)
                    accept[curIndex] = mHistoryAcceptRateMH[j];
                else
                {
                    int runIndex = burnAdaptLen + (curIndex - burnAdaptLen) * chains[i].mThinningInterval;
                    accept[runIndex] = mHistoryAcceptRateMH[j];
                }
            }
            break;
        }
        else
        {
            shift += acceptSize;
        }
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
