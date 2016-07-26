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
    
    for(int i=0; i<mLastAccepts.size(); ++i)
        sum += mLastAccepts.at(i) ? 1.f : 0.f;

    
    return sum / (double)mLastAccepts.size();
}

void MHVariable::saveCurrentAcceptRate()
{
    const double rate = 100.f * getCurrentAcceptRate();
    mHistoryAcceptRateMH.push_back(rate);
}

QVector<double> MHVariable::acceptationForChain(const QList<ChainSpecs> &chains, int index)
{
    QVector<double> accept(0);
    int shift = 0;
    const int reserveSize = (int) ceil(chains.at(index).mNumBurnIter + (chains.at(index).mBatchIndex * chains.at(index).mNumBatchIter) + chains.at(index).mNumRunIter / chains.at(index).mThinningInterval);

    accept.reserve(reserveSize);

    for(int i=0; i<chains.size(); ++i) {
        int chainSize = chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter) + chains.at(i).mNumRunIter / chains.at(i).mThinningInterval;
        
        if(i == index) {
            for(int j=0; j<chainSize; ++j) {
                accept.append((double)mHistoryAcceptRateMH.at(shift + j));
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
    int shift = 0;

    for(int i=0; i<chains.size(); ++i) {
        int burnAdaptSize = chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter);
        int runSize = chains.at(i).mNumRunIter;
        shift += burnAdaptSize;
        for (int j=shift; (j<shift + runSize) && (j<mAllAccepts.size()); ++j) {
            if (mAllAccepts.at(j))
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
    stream << data.mAllAccepts;
    stream << data.mHistoryAcceptRateMH;

     //*out << this->mProposal; // it's a QString, already set
     stream << data.mSigmaMH;

    return stream;
}

QDataStream &operator>>( QDataStream &stream, MHVariable &data )
{
    /* herited from MetropolisVariable*/
    stream >> dynamic_cast<MetropolisVariable&>(data);

    stream >> data.mAllAccepts;
    stream >> data.mHistoryAcceptRateMH;

    stream >> data.mSigmaMH;

    return stream;

}

/* Obsolete function
 * */
void MHVariable::saveToStream(QDataStream &out)
{
     /* herited from MetropolisVariable*/
    
    this->MetropolisVariable::saveToStream(out);

    /* owned by MHVariable*/
    //*out << this->mAllAccepts;
    out << quint32(mAllAccepts.size());
    for ( const bool b:mAllAccepts)
        out << (b? quint8(1): quint8(0));

    out << qreal(mGlobalAcceptation);
   // *out << mHistoryAcceptRateMH;
    out << quint32(mHistoryAcceptRateMH.size());
    for ( const float d : mHistoryAcceptRateMH )
        out << qreal(d);
   // *out << this->mLastAccepts;
    
     out << quint32(mLastAcceptsLength);
     //*out << this->mProposal; // it's a QString, already set
     out << qreal(mSigmaMH);

}

void MHVariable::saveToStreamOfQByteArray(QDataStream *out)
{
     /* herited from MetropolisVariable*/

    this->MetropolisVariable::saveToStreamOfQByteArray(out);

    /* owned by MHVariable*/
    //*out << this->mAllAccepts;
    *out << QByteArray::number(mAllAccepts.size());
    for ( const bool b:mAllAccepts)
        *out << QByteArray::number((short)(b? 1: 0));

    *out << QByteArray::number(mGlobalAcceptation);
   // *out << mHistoryAcceptRateMH;
    *out << QByteArray::number(mHistoryAcceptRateMH.size());
  //  for ( float& d : mHistoryAcceptRateMH )
    //    *out << QByteArray::number(d);
    for (int i=0; i<mHistoryAcceptRateMH.size(); ++i)
        *out << QByteArray::number(mHistoryAcceptRateMH.at(i));
   // *out << this->mLastAccepts;

     *out << QByteArray::number((int)mLastAcceptsLength);
     //*out << this->mProposal; // it's a QString, already set
     *out << QByteArray::number(mSigmaMH);

}

void MHVariable::loadFromStream(QDataStream &in)
{
    /* herited from MetropolisVariable*/
    this->MetropolisVariable::loadFromStream(in);
    //*in >> this->mAllAccepts;
    quint32 reserveTmp;
    in >> reserveTmp;
    mAllAccepts.resize((int)reserveTmp);
    for ( bool& b : mAllAccepts) {
        quint8 bTmp;
        in >> bTmp;
        b = (bTmp==1);
    }
    qreal tmp;
    in >> tmp;
    mGlobalAcceptation = (double) tmp;

    //*in >> mHistoryAcceptRateMH;
    in >> reserveTmp;
    mHistoryAcceptRateMH.resize((int)reserveTmp);
    for (float& rate : mHistoryAcceptRateMH) {
           in >> tmp;
           rate = (float) tmp;
    }

   // *in >> this->mLastAccepts;
    quint32 utmp;
    in >> utmp;
    mLastAcceptsLength = (int) utmp;
   // *in >> this->mProposal; // it's a QString, already set
    in >> tmp;
    mSigmaMH = tmp;
}

void MHVariable::loadFromStreamOfQByteArray(QDataStream *in)
{
    /* herited from MetropolisVariable*/
    this->MetropolisVariable::loadFromStreamOfQByteArray(in);
    //*in >> this->mAllAccepts;
    QByteArray tmpArray;
    *in >> tmpArray;
    mAllAccepts.resize(tmpArray.toInt());
    for ( bool& b : mAllAccepts) {
        QByteArray bTmp;
        *in >> bTmp;
        b = (bTmp.toShort()==1);
    }

    *in >> tmpArray;
    mGlobalAcceptation = tmpArray.toDouble();

    //*in >> mHistoryAcceptRateMH;
    *in >> tmpArray;
    mHistoryAcceptRateMH.resize(tmpArray.toInt());
    for (float& rate : mHistoryAcceptRateMH) {
           *in >> tmpArray;
           rate = tmpArray.toFloat();
    }

   // *in >> this->mLastAccepts;
    *in >> tmpArray;
    mLastAcceptsLength = tmpArray.toInt();
   // *in >> this->mProposal; // it's a QString, already set
    *in >> tmpArray;
    mSigmaMH = tmpArray.toDouble();
}
