/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "MHVariable.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "Generator.h"

#include <QDebug>

/** Default constructor */
MHVariable::MHVariable():
mLastAcceptsLength(0),
mGlobalAcceptation(0),
mHistoryAcceptRateMH(nullptr)
{
  /*mAllAccepts = new (std::nothrow) QVector<bool>();
  mHistoryAcceptRateMH = new (std::nothrow) QVector<double>();*/
  mAllAccepts = new QVector<bool>();
  mHistoryAcceptRateMH = new QVector<double>();

}

/** Copy constructor */

MHVariable::MHVariable( const MHVariable& origin)
{
   // MetropolisVariable(origin);

    mX = origin.mX;
    mRawTrace = new QVector<double>(origin.mRawTrace->size());
    std::copy(origin.mRawTrace->begin(),origin.mRawTrace->end(),mRawTrace->begin());

    mFormatedTrace = new QVector<double>(origin.mFormatedTrace->size());
    std::copy(origin.mFormatedTrace->begin(),origin.mFormatedTrace->end(),mFormatedTrace->begin());

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mHisto = origin.mHisto;
    mChainsHistos = origin.mChainsHistos;

    mCorrelations = origin.mCorrelations;

    mHPD = origin.mHPD;
    mCredibility = origin.mCredibility;

    mExactCredibilityThreshold = origin.mExactCredibilityThreshold;

    mResults = origin.mResults;
    mChainsResults = origin.mChainsResults;

    mfftLenUsed = origin.mBandwidthUsed;
    mBandwidthUsed = origin.mBandwidthUsed;
    mThresholdUsed = origin.mThresholdUsed;

    mtminUsed = origin.mtminUsed;
    mtmaxUsed = origin.mtmaxUsed;

    mAllAccepts = new QVector<bool>(origin.mAllAccepts->size());
    mHistoryAcceptRateMH = new QVector<double>(origin.mHistoryAcceptRateMH->size());
}

MHVariable::~MHVariable()
{
    mAllAccepts->~QVector();
    mHistoryAcceptRateMH->~QVector();

    // mRawTrace and mFormatedTrace are destroye by the MetropolisVariable destructor
   // mRawTrace->~QVector();// = nullptr;;
   // mFormatedTrace->~QVector();// = nullptr;;
}

/**
 * @brief MHVariable::tryUpdate It's a puch function where mLastAcceptsLength = mChains[0].mNumBatchIter;
 * @param x
 * @param rapport
 * @return
 */
bool MHVariable::tryUpdate(const double x, const double rapport)
{
    if (mLastAccepts.length() >= mLastAcceptsLength)
        mLastAccepts.removeAt(0);

    bool accepted (false);

    if (rapport >= 1.)
        accepted = true;

    else {
        const double uniform = Generator::randomUniform();
        accepted = (rapport >= uniform);
    }

    if (accepted)
        mX = x;

    mLastAccepts.append(accepted);
    mAllAccepts->append(accepted);
    return accepted;

}

void MHVariable::reset()
{
    MetropolisVariable::reset();

    mLastAccepts.clear();
    mAllAccepts->clear();// mAllAccepts.clear(); //don't clean, avalable for cumulate chain

    mLastAccepts.squeeze();
    mAllAccepts->squeeze();
}

void MHVariable::reserve(const int reserve)
{
    MetropolisVariable::reserve(reserve);
    mAllAccepts->reserve(reserve);
    mHistoryAcceptRateMH->reserve(reserve);
}

MHVariable& MHVariable::copy(MHVariable const& origin)
{
    mX = origin.mX;
    mRawTrace->resize(origin.mRawTrace->size());
    std::copy(origin.mRawTrace->begin(),origin.mRawTrace->end(),mRawTrace->begin());

    mFormatedTrace->resize(origin.mFormatedTrace->size());
    std::copy(origin.mFormatedTrace->begin(),origin.mFormatedTrace->end(),mFormatedTrace->begin());

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mHisto = origin.mHisto;
    mChainsHistos = origin.mChainsHistos;

    mCorrelations = origin.mCorrelations;

    mHPD = origin.mHPD;
    mCredibility = origin.mCredibility;

    mExactCredibilityThreshold = origin.mExactCredibilityThreshold;

    mResults = origin.mResults;
    mChainsResults = origin.mChainsResults;

    mfftLenUsed = origin.mBandwidthUsed;
    mBandwidthUsed = origin.mBandwidthUsed;
    mThresholdUsed = origin.mThresholdUsed;

    mtminUsed = origin.mtminUsed;
    mtmaxUsed = origin.mtmaxUsed;

    mSigmaMH = origin.mSigmaMH;
    mLastAccepts = origin.mLastAccepts;
    mLastAcceptsLength = origin.mLastAcceptsLength;

    mAllAccepts->resize(origin.mAllAccepts->size());
    std::copy(origin.mAllAccepts->begin(),origin.mAllAccepts->end(),mAllAccepts->begin());

    mGlobalAcceptation = origin.mGlobalAcceptation;

    mHistoryAcceptRateMH->resize(origin.mHistoryAcceptRateMH->size());
    std::copy(origin.mHistoryAcceptRateMH->begin(),origin.mHistoryAcceptRateMH->end(),mHistoryAcceptRateMH->begin());

    mProposal = origin.mProposal;

    return *this;
}

MHVariable& MHVariable::operator=( MHVariable const& origin)
{
    copy(origin);
    return *this;
}

double MHVariable::getCurrentAcceptRate() const
{
    Q_ASSERT(!mLastAccepts.isEmpty());

    double sum (0.);

    sum = std::accumulate(mLastAccepts.begin(), mLastAccepts.end(), sum,[](double s, double a){return s+(a ? 1. : 0.);}); //#include <numeric>

    sum = sum / (double)mLastAccepts.size();

    return sum ;

}

void MHVariable::saveCurrentAcceptRate()
{
    const double rate = 100. * getCurrentAcceptRate();
    mHistoryAcceptRateMH->push_back(rate);
}

QVector<double> MHVariable::acceptationForChain(const QList<ChainSpecs> &chains, int index)
{
    QVector<double> accept(0);
    int shift (0);
    const int reserveSize = (int) ceil(chains.at(index).mNumBurnIter + (chains.at(index).mBatchIndex * chains.at(index).mNumBatchIter) + chains.at(index).mNumRunIter / chains.at(index).mThinningInterval);

    accept.reserve(reserveSize);

    for (int i=0; i<chains.size(); ++i) {
        // We add 1 for the init
        const int chainSize = 1 +chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter) + chains.at(i).mNumRunIter / chains.at(i).mThinningInterval;

        if (i == index) {
            // could be done with
            //accept.resize(chainSize
            //std::copy(from_vector.begin(), from_vector.end(), to_vector.begin());

            for (int j=0; j<chainSize; ++j)
                accept.append(mHistoryAcceptRateMH->at(shift + j));

            break;
        }
        else
            shift += chainSize;
    }
    return accept;
}

void MHVariable::generateGlobalRunAcceptation(const QList<ChainSpecs> &chains)
{
    double accepted (0.);
    double acceptsLength (0.);
    int shift (0);

    for (auto&& chain : chains) {
        int burnAdaptSize = chain.mNumBurnIter + (chain.mBatchIndex * chain.mNumBatchIter);
        int runSize = chain.mNumRunIter;
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

QString MHVariable::resultsString(const QString& nl, const QString& noResultMessage, const QString& unit, DateConversion formatFunc, const bool forCSV) const
{
    QString result = MetropolisVariable::resultsString(nl, noResultMessage, unit, formatFunc, forCSV);
    if (!mProposal.isEmpty()) {
        if (forCSV)
             result += nl + tr("Acceptance rate (all acquire iterations) : %1 % (%2)").arg(stringForCSV(mGlobalAcceptation*100.), mProposal);

        else
             result += nl + tr("Acceptance rate (all acquire iterations) : %1 % (%2)").arg(stringForLocal(mGlobalAcceptation*100.), mProposal);
    }
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
        data.mHistoryAcceptRateMH = new QVector<double>();
    stream >> *(data.mHistoryAcceptRateMH);

    stream >> data.mSigmaMH;

    return stream;

}
