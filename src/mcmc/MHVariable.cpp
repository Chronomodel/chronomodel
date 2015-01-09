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
        mAllAccepts.append(true);
        return true;
    }
    else
    {
        double uniform = Generator::randomUniform();
        if(rapport >= uniform)
        {
            mX = x;
            mLastAccepts.append(true);
            mAllAccepts.append(true);
            return true;
        }
        else
        {
            mLastAccepts.append(false);
            mAllAccepts.append(false);
            return false;
        }
    }
}

void MHVariable::reset()
{
    MetropolisVariable::reset();
    mLastAccepts.clear();
    mHistoryAcceptRateMH.clear();
}

double MHVariable::getCurrentAcceptRate()
{
    double sum = 0.f;
    for(int i=0; i<mLastAccepts.length(); ++i)
        sum += mLastAccepts[i] ? 1.f : 0.f;
    
    //qDebug() << "Last accept on " << sum << " / " << mLastAccepts.length() << " values";
    
    return sum / (double)mLastAccepts.length();
}

void MHVariable::saveCurrentAcceptRate()
{
    double rate = 100.f * getCurrentAcceptRate();
    mHistoryAcceptRateMH.push_back(rate);
    //mHistorySigmaMH.push_back(mSigmaMH);
}

QMap<double, double> MHVariable::acceptationForChain(const QList<Chain>& chains, int index)
{
    QMap<double, double> accept;
    int shift = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        int acceptSize = (chains[i].mNumBurnIter + chains[i].mBatchIndex * chains[i].mNumBatchIter + chains[i].mNumRunIter) / chains[i].mThinningInterval;
        
        if(i == index)
        {
            for(int j=shift; j<shift + acceptSize; ++j)
            {
                int curIndex = j - shift;
                accept[curIndex * chains[i].mThinningInterval] = mHistoryAcceptRateMH[j];
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

void MHVariable::generateGlobalRunAcceptation(const QList<Chain>& chains)
{
    double accepted = 0;
    double acceptsLength = 0;
    int shift = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        int burnAdaptSize = (chains[i].mNumBurnIter + chains[i].mBatchIndex * chains[i].mNumBatchIter) / chains[i].mThinningInterval;
        
        int runSize = chains[i].mNumRunIter / chains[i].mThinningInterval;
        
        shift += burnAdaptSize;
        
        for(int j=shift; j<shift + runSize; ++j)
        {
            int curIndex = j - shift;
            if(mAllAccepts[curIndex])
                ++accepted;
        }
        shift += runSize;
        acceptsLength += runSize;
    }
    mGlobalAcceptation = accepted / acceptsLength;
}

void MHVariable::generateNumericalResults(const QList<Chain>& chains, const ProjectSettings& settings)
{
    MetropolisVariable::generateNumericalResults(chains, settings);
    generateGlobalRunAcceptation(chains);
}

QString MHVariable::resultsText() const
{
    QString result = MetropolisVariable::resultsText();
    if(!mProposal.isEmpty())
        result += "Taux d'acceptation global : " + QString::number(mGlobalAcceptation*100, 'f', 1) + "% ("+mProposal+")\n";
    return result;
}

