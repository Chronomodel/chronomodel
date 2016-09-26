#include "MHVariable.h"
#include "StdUtilities.h"
#include "Generator.h"
#include <QDebug>


MHVariable::MHVariable():
mLastAcceptsLength(0),
mAllAccepts(0),
mGlobalAcceptation(0),
mHistoryAcceptRateMH(0)
{
 /* mAllAccepts = new (std::nothrow) QVector<bool>();
  mHistoryAcceptRateMH = new (std::nothrow) QVector<float>();
*/
}

MHVariable::~MHVariable()
{

}

bool MHVariable::tryUpdate(const double x, const double rapport)
{
   if (mLastAccepts.length() >= mLastAcceptsLength)
        mLastAccepts.removeAt(0);
    
    bool accepted = false;
    
    if (rapport >= 1)
        accepted = true;

    else {
        const double uniform = Generator::randomUniform();
        accepted = (rapport >= uniform);
    }
    
    if(accepted)
        mX = x;
     
    mLastAccepts.append(accepted);
    mAllAccepts->append(accepted);

    return accepted;

     
}

void MHVariable::reset()
{
    MetropolisVariable::reset();
    mLastAccepts.clear();
  //  mAllAccepts.clear();// mAllAccepts.clear(); //don't clean, avalable for cumulate chain

    mAllAccepts = new (std::nothrow) QVector<bool>();
    mHistoryAcceptRateMH = new (std::nothrow) QVector<float>();

  /*  if (!mHistoryAcceptRateMH->isEmpty())
        mHistoryAcceptRateMH->clear();*/

}

void MHVariable::reserve(const int reserve)
{
    MetropolisVariable::reserve(reserve);
    mAllAccepts->reserve(reserve);
    mHistoryAcceptRateMH->reserve(reserve);
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

float MHVariable::getCurrentAcceptRate()
{
    float sum = 0.f;

    sum = std::accumulate(mLastAccepts.begin(), mLastAccepts.end(), sum,[](float s, bool a){return s+(a?1.:0.);}); //#include <numeric>
    
    return sum / (float)mLastAccepts.size();
}

void MHVariable::saveCurrentAcceptRate()
{
    const float rate = 100.f * getCurrentAcceptRate();
    mHistoryAcceptRateMH->push_back(rate);
}

QVector<float> MHVariable::acceptationForChain(const QList<ChainSpecs> &chains, int index)
{
    QVector<float> accept(0);
    int shift = 0;
    const int reserveSize = (int) ceil(chains.at(index).mNumBurnIter + (chains.at(index).mBatchIndex * chains.at(index).mNumBatchIter) + chains.at(index).mNumRunIter / chains.at(index).mThinningInterval);

    accept.reserve(reserveSize);

    for(int i=0; i<chains.size(); ++i) {
        int chainSize = chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter) + chains.at(i).mNumRunIter / chains.at(i).mThinningInterval;
        

        if(i == index) {
            // could be done with
            //accept.resize(chainSize
            //std::copy(from_vector.begin(), from_vector.end(), to_vector.begin());

            for(int j=0; j<chainSize; ++j) {
                accept.append((float)mHistoryAcceptRateMH->at(shift + j));
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
    float accepted = 0;
    float acceptsLength = 0;
    int shift = 0;

    for(int i=0; i<chains.size(); ++i) {
        int burnAdaptSize = chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter);
        int runSize = chains.at(i).mNumRunIter;
        shift += burnAdaptSize;
        for (int j=shift; (j<shift + runSize) && (j<mAllAccepts->size()); ++j) {
            if (mAllAccepts->at(j))
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

QDataStream &operator<<( QDataStream &stream, const MHVariable &data )
{
    stream << dynamic_cast<const MetropolisVariable&>(data);

    /* owned by MHVariable*/
    stream << *(data.mAllAccepts);
    stream << *(data.mHistoryAcceptRateMH);

     //*out << this->mProposal; // it's a QString, already set
     stream << data.mSigmaMH;

    return stream;
}

QDataStream &operator>>( QDataStream &stream, MHVariable &data )
{
    /* herited from MetropolisVariable*/
    stream >> dynamic_cast<MetropolisVariable&>(data);
    if (data.mAllAccepts)
        data.mAllAccepts->clear();
    else
        data.mAllAccepts = new QVector<bool>();
    stream >> *(data.mAllAccepts);

    if (data.mHistoryAcceptRateMH)
        data.mHistoryAcceptRateMH->clear();
    else
        data.mHistoryAcceptRateMH = new QVector<float>();
    stream >> *(data.mHistoryAcceptRateMH);

    stream >> data.mSigmaMH;

    return stream;

}


