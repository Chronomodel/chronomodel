#include "MHVariable.h"
#include "StdUtilities.h"
#include "Generator.h"
#include <QDebug>


MHVariable::MHVariable():mLastAcceptsLength(0){}
MHVariable::~MHVariable(){}

bool MHVariable::tryUpdate(const double x, const double rapport)
{
    if(mLastAccepts.length() >= mLastAcceptsLength)
        mLastAccepts.removeAt(0);
    
    bool accepted = false;
    
    if(rapport >= 1)
    {
        accepted = true;
    }
    else
    {
        double uniform = Generator::randomUniform();
        accepted = (rapport >= uniform);
    }
    
    if(accepted)
        mX = x;
    
    mLastAccepts.append(accepted);
    mAllAccepts.append(accepted);
    return accepted;
}

void MHVariable::reset()
{
    MetropolisVariable::reset();
    mLastAccepts.clear();
    mAllAccepts.clear();
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
}

QVector<double> MHVariable::acceptationForChain(const QList<Chain>& chains, int index)
{
    QVector<double> accept;
    int shift = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        int chainSize = chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter) + chains[i].mNumRunIter / chains[i].mThinningInterval;
        
        if(i == index)
        {
            for(int j=0; j<chainSize; ++j)
                accept.append(mHistoryAcceptRateMH[shift + j]);
            break;
        }
        else
            shift += chainSize;
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
        int burnAdaptSize = chains[i].mNumBurnIter + (chains[i].mBatchIndex * chains[i].mNumBatchIter);
        int runSize = chains[i].mNumRunIter;
        
        shift += burnAdaptSize;
        
        for(int j=shift; j<shift + runSize; ++j)
        {
            if(mAllAccepts[j])
                ++accepted;
        }
        shift += runSize;
        acceptsLength += runSize;
    }
    mGlobalAcceptation = accepted / acceptsLength;
}

void MHVariable::generateNumericalResults(const QList<Chain>& chains)
{
    MetropolisVariable::generateNumericalResults(chains);
    generateGlobalRunAcceptation(chains);
}

QString MHVariable::resultsText(const QString& noResultMessage) const
{
    QString result = MetropolisVariable::resultsText(noResultMessage);
    if(!mProposal.isEmpty())
        result += "<br>Acceptation rate (all acquire iterations) : " + QString::number(mGlobalAcceptation*100, 'f', 1) + "% ("+mProposal+")";
    return result;
}

void MHVariable::saveToStream(QDataStream *out) // ajout PhD
{
     /* herited from MetropolisVariable*/
    *out << this->mChainsHistos;
     *out << this->mChainsRawHistos;
     //out << this->mChainsResults;
     *out << this->mCorrelations;
     *out << this->mCredibility;
     *out << this->mExactCredibilityThreshold;
     *out << this->mHisto;
     *out << this->mHPD;
     *out << this->mRawHisto;
     //out << this->mResults;
     *out << this->mThreshold;
     *out << this->mTrace;
     *out << this->mX;

     /* owned by MHVariable*/
     *out << this->mAllAccepts;
     *out << this->mGlobalAcceptation;
     *out << this->mHistoryAcceptRateMH;
     *out << this->mLastAccepts;
     *out << this->mLastAcceptsLength;
     *out << this->mProposal;
     *out << this->mSigmaMH;

}
void MHVariable::loadFromStream(QDataStream *in) // ajout PhD
{
    /* herited from MetropolisVariable*/
    *in >> this->mChainsHistos;
    *in >> this->mChainsRawHistos;
    //*in >> this->mChainsResults;
    *in >> this->mCorrelations;
    *in >> this->mCredibility;
    *in >> this->mExactCredibilityThreshold;
    *in >> this->mHisto;
    *in >> this->mHPD;
    *in >> this->mRawHisto;
    //*in >> this->mResults;
    *in >> this->mThreshold;
    *in >> this->mTrace;
    *in >> this->mX;

     /* owned by MHVariable*/
    *in >> this->mAllAccepts;
    *in >> this->mGlobalAcceptation;
    *in >> this->mHistoryAcceptRateMH;
    *in >> this->mLastAccepts;
    *in >> this->mLastAcceptsLength;
    *in >> this->mProposal;
    *in >> this->mSigmaMH;
}
