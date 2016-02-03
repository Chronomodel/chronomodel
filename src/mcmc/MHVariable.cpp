#include "MHVariable.h"
#include "StdUtilities.h"
#include "Generator.h"
#include <QDebug>


MHVariable::MHVariable():
mLastAcceptsLength(0)
{

}

MHVariable::~MHVariable()
{

}

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

MHVariable& MHVariable::copy(MHVariable const& origin)
{
    MetropolisVariable::copy(origin);
    mSigmaMH = origin.mSigmaMH;
    mLastAccepts = origin.mLastAccepts;
    mLastAcceptsLength = origin.mLastAcceptsLength;
    mAllAccepts = origin.mAllAccepts;
    mGlobalAcceptation = origin.mGlobalAcceptation;
    mHistoryAcceptRateMH = origin.mHistoryAcceptRateMH;
    mProposal = origin.mProposal;

    return *this;
}

MHVariable& MHVariable::operator=( MHVariable const& origin)
{
    copy(origin);
    return *this;
}

double MHVariable::getCurrentAcceptRate()
{
    double sum = 0.f;
    
    for(int i=0; i<mLastAccepts.size(); ++i) {
        sum += mLastAccepts.at(i) ? 1.f : 0.f;
    }
    
    return sum / (double)mLastAccepts.size();
}

void MHVariable::saveCurrentAcceptRate()
{
    double rate = 100.f * getCurrentAcceptRate();
    mHistoryAcceptRateMH.push_back(rate);
}

QVector<double> MHVariable::acceptationForChain(const QList<ChainSpecs> &chains, int index)
{
    QVector<double> accept;
     int shift = 0;
    
    for(int i=0; i<chains.size(); ++i)
    {
        int chainSize = chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter) + chains.at(i).mNumRunIter / chains.at(i).mThinningInterval;
        
        if(i == index)
        {
            for(int j=0; j<chainSize; ++j) {
                accept.append(mHistoryAcceptRateMH.at(shift + j));
            }
            break;
        }
        else
            shift += chainSize;
    }
    return accept;
}

void MHVariable::generateGlobalRunAcceptation(const QList<ChainSpecs> &chains)
{
    double accepted = 0;
    double acceptsLength = 0;
    unsigned long shift = 0;

    for(int i=0; i<chains.size(); ++i)
    {
        unsigned long burnAdaptSize = chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter);
        unsigned long runSize = chains.at(i).mNumRunIter;
        shift += burnAdaptSize;
        for(unsigned long j=shift; j<shift + runSize; ++j)
        {
            if(mAllAccepts.at(j))
                ++accepted;
        }
        shift += runSize;
        acceptsLength += runSize;
    }



    mGlobalAcceptation = accepted / acceptsLength;
}

void MHVariable::generateNumericalResults(const QList<ChainSpecs> &chains)
{
    MetropolisVariable::generateNumericalResults(chains);
    generateGlobalRunAcceptation(chains);
}

QString MHVariable::resultsString(const QString& nl, const QString& noResultMessage, const QString& unit, FormatFunc formatFunc) const
{
    QString result = MetropolisVariable::resultsString(nl, noResultMessage, unit, formatFunc);
    if(!mProposal.isEmpty())
        result += nl + "Acceptation rate (all acquire iterations) : " + QString::number(mGlobalAcceptation*100, 'f', 1) + "% ("+mProposal+")";
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
