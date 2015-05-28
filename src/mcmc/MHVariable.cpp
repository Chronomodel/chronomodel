#include "MHVariable.h"
#include "StdUtilities.h"
#include "Generator.h"
#include <QDebug>


MHVariable::MHVariable():mLastAcceptsLength(0){}
MHVariable::~MHVariable(){}

bool MHVariable::tryUpdate(const double x, const double rapport)
{
   /* if(mLastAccepts.length() >= mLastAcceptsLength)
        mLastAccepts.removeAt(0);*/
    if(mLastAccepts.size() >= mLastAcceptsLength)
     mLastAccepts.erase(mLastAccepts.begin(), mLastAccepts.end());
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
    
    /*mLastAccepts.append(accepted);
    mAllAccepts.append(accepted);*/
    mLastAccepts.push_back(accepted); //PhD
    mAllAccepts.push_back(accepted);
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
   /* for(int i=0; i<mLastAccepts.length(); ++i)
        sum += mLastAccepts[i] ? 1.f : 0.f;
    
    //qDebug() << "Last accept on " << sum << " / " << mLastAccepts.length() << " values";
    
    return sum / (double)mLastAccepts.length();*/
    
    for(int i=0; i<mLastAccepts.size(); ++i)
        sum += mLastAccepts[i] ? 1.f : 0.f;
    
    //qDebug() << "Last accept on " << sum << " / " << mLastAccepts.length() << " values";
    
    return sum / (double)mLastAccepts.size();
}

void MHVariable::saveCurrentAcceptRate()
{
    double rate = 100.f * getCurrentAcceptRate();
    mHistoryAcceptRateMH.push_back(rate);
}

QVector<double> MHVariable::acceptationForChain(const QList<Chain>& chains, int index)
{
    QVector<double> accept;
    unsigned long long shift = 0;
    
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
    
    this->MetropolisVariable::saveToStream(out);
     /* owned by MHVariable*/
    *out << this->mAllAccepts;
    
    //*out << QVector<bool>::fromStdVector(this->mAllAccepts);
    
     *out << this->mGlobalAcceptation;
    *out << this->mHistoryAcceptRateMH;
    *out << this->mLastAccepts;
    
      /**out << QVector<float>::fromStdVector(this->mHistoryAcceptRateMH);
     *out << QVector<bool>::fromStdVector(this->mLastAccepts);*/
     *out << this->mLastAcceptsLength;
     *out << this->mProposal;
     *out << this->mSigmaMH;

}
void MHVariable::loadFromStream(QDataStream *in) // ajout PhD
{
    
    
    /* herited from MetropolisVariable*/
   
    
    this->MetropolisVariable::loadFromStream(in);
     /* owned by MHVariable*/
    QVector<bool> vectorOfBool;
    QVector<float> vectorOfFoat;
    
     /* *in >> vectorOfBool;
    this->mAllAccepts=vectorOfBool.toStdVector();*/
    
    *in >> this->mAllAccepts;
    *in >> this->mGlobalAcceptation;
    
    /* *in >> vectorOfFoat;
    this->mHistoryAcceptRateMH=vectorOfFoat.toStdVector();*/
    *in >> this->mHistoryAcceptRateMH;
    
    /* *in >> vectorOfBool;
    this->mLastAccepts=vectorOfBool.toStdVector();*/
    *in >> this->mLastAccepts;
    
    *in >> this->mLastAcceptsLength;
    *in >> this->mProposal;
    *in >> this->mSigmaMH;
}
