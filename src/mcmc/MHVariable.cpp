<<<<<<< Updated upstream
/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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
#include "QtUtilities.h"
#include "Generator.h"

#include <QDebug>

#define FixeStr QObject::tr("Fixed value")

#define MHAdaptGaussStr QObject::tr("Adapt. Gaussian random walk")
#define BoxMullerStr QObject::tr("Event Prior")
#define DoubleExpStr QObject::tr("Double-Exponential")

#define MHDatePriorStr QObject::tr("Date Prior")
#define InversionStr QObject::tr("Distribution of Calibrated Date")
#define MHSymGaussAdaptStr QObject::tr("Adapt. Gaussian random walk")

/** Default constructor */
MHVariable::MHVariable():
    mLastAcceptsLength(0),
    mGlobalAcceptationPerCent(.0),
    mHistoryAcceptRateMH(nullptr)
{
  mAllAccepts.clear();
  mHistoryAcceptRateMH = new QList<double>();

}

/** Copy constructor */

MHVariable::MHVariable( const MHVariable& origin):
    MHVariable()

{
    mX = origin.mX;
    mRawTrace->resize(origin.mRawTrace->size());
    std::copy(origin.mRawTrace->begin(), origin.mRawTrace->end(), mRawTrace->begin());

    mFormatedTrace->resize(origin.mFormatedTrace->size());
    std::copy(origin.mFormatedTrace->begin(), origin.mFormatedTrace->end(), mFormatedTrace->begin());

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mFormatedHisto = origin.mFormatedHisto;
    mChainsHistos = origin.mChainsHistos;

    mCorrelations = origin.mCorrelations;

    mFormatedHPD = origin.mFormatedHPD;
    mFormatedCredibility = origin.mFormatedCredibility;

    mExactCredibilityThreshold = origin.mExactCredibilityThreshold;

    mResults = origin.mResults;
    mChainsResults = origin.mChainsResults;

    mfftLenUsed = origin.mBandwidthUsed;
    mBandwidthUsed = origin.mBandwidthUsed;
    mThresholdUsed = origin.mThresholdUsed;

    mtminUsed = origin.mtminUsed;
    mtmaxUsed = origin.mtmaxUsed;

    mAllAccepts.clear();
    mHistoryAcceptRateMH = new QList<double>(origin.mHistoryAcceptRateMH->size());
}

MHVariable::~MHVariable()
{
    delete mHistoryAcceptRateMH;
    mHistoryAcceptRateMH = nullptr;
}

/**
 * @brief MHVariable::tryUpdate
 * @param x : Value proposed and, if applicable, accepted
 * @param rate : Force reject with rate  = -1.
 * @return
 */
bool MHVariable::tryUpdate(const double x, const double rate)
{
    if (mLastAccepts.size() >= mLastAcceptsLength)
        mLastAccepts.removeAt(0);

    bool accepted (false);

    if (rate >= 1.)
        accepted = true;

    else if (rate >= 0){
        const double uniform = Generator::randomUniform();
        accepted = (uniform <= rate);
#ifdef DEBUG
        if (uniform == 0)
            qDebug()<< "[MHVariable::tryUpdate] uniform == 0";
#endif
    }

    if (accepted) {
        mX = x;
    }
    mLastAccepts.append(accepted);

    return accepted;

}

/**
 * @brief MHVariable::adapt
 * @param coef_min value [0; 1], default 0.42
 * @param coef_max value [0; 1], default 0.46
 * @return bool if no adaptation needed
 */
bool MHVariable::adapt (const double coef_min, const double coef_max, const double delta)
{
    bool noAdapted = true;
    const double acceptRate = getCurrentAcceptRate();
    if (acceptRate <= coef_min || acceptRate >= coef_max) {
        noAdapted = false;
        const double sign = (acceptRate <= coef_min) ? -1. : 1.;
        mSigmaMH *= pow(10., sign * delta);
        //qDebug()<<"[MHVariable::adapt] "<<this->getName();
    }
    return noAdapted;
}

void MHVariable::reset()
{
    MetropolisVariable::reset();

    mLastAccepts.clear();
    mAllAccepts.clear();// mAllAccepts.clear(); //don't clean, avalable for cumulate chain

    mLastAccepts.squeeze();
    //mAllAccepts->squeeze();
}

void MHVariable::reserve(const int reserve)
{
    MetropolisVariable::reserve(reserve);
    //mAllAccepts->reserve(reserve);
    mHistoryAcceptRateMH->reserve(reserve);
}

MHVariable& MHVariable::copy(MHVariable const& origin)
{
    mX = origin.mX;
    //mRawTrace = origin.mRawTrace;
    mRawTrace->resize(origin.mRawTrace->size());
    std::copy(origin.mRawTrace->begin(), origin.mRawTrace->end(), mRawTrace->begin());

    mFormatedTrace->resize(origin.mFormatedTrace->size());
    std::copy(origin.mFormatedTrace->begin(), origin.mFormatedTrace->end(), mFormatedTrace->begin());

    mSupport = origin.mSupport;
    mFormat = origin.mFormat;

    mFormatedHisto = origin.mFormatedHisto;
    mChainsHistos = origin.mChainsHistos;

    mCorrelations = origin.mCorrelations;

    mFormatedHPD = origin.mFormatedHPD;
    mFormatedCredibility = origin.mFormatedCredibility;

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

    mAllAccepts = origin.mAllAccepts; //->resize(origin.mAllAccepts->size());
    //std::copy(origin.mAllAccepts->begin(), origin.mAllAccepts->end(), mAllAccepts->begin());

    //mAllAccepts = origin.mAllAccepts;

    mGlobalAcceptationPerCent = origin.mGlobalAcceptationPerCent;

    //mHistoryAcceptRateMH = origin.mHistoryAcceptRateMH;

    mHistoryAcceptRateMH->resize(origin.mHistoryAcceptRateMH->size());
    std::copy(origin.mHistoryAcceptRateMH->begin(), origin.mHistoryAcceptRateMH->end(), mHistoryAcceptRateMH->begin());

    mSamplerProposal = origin.mSamplerProposal;

    return *this;
}

MHVariable& MHVariable::operator=( MHVariable const& origin)
{
    copy(origin);
    return *this;
}

double MHVariable::getCurrentAcceptRate() const
{
   // Q_ASSERT(!mLastAccepts.isEmpty());
    if (mLastAccepts.isEmpty())
        return 0.;

    /*const double sum = std::accumulate(mLastAccepts.begin(), mLastAccepts.end(), 0.,[](double s, double a){return s+(a ? 1. : 0.);}) / mLastAccepts.size(); //#include <numeric>
    return std::move(sum) ;
*/

    return std::count_if(mLastAccepts.begin(), mLastAccepts.end(), [](bool i) { return i; })/ (double) mLastAccepts.size();

}

void MHVariable::saveCurrentAcceptRate()
{
    mHistoryAcceptRateMH->push_back(100. * getCurrentAcceptRate());
}

QList<double> MHVariable::acceptationForChain(const QList<ChainSpecs> &chains, int index)
{
    QList<double> accept(0);
    int shift = 0;
    const int reserveSize = (int) ceil(chains.at(index).mIterPerBurn + (chains.at(index).mBatchIndex * chains.at(index).mIterPerBatch) + chains.at(index).mRealyAccepted);
    accept.reserve(reserveSize);

    for (int i = 0; i < chains.size(); ++i) {
        // We add 1 for the init
        const int chainSize = 1 +chains.at(i).mIterPerBurn + (chains.at(i).mBatchIndex * chains.at(i).mIterPerBatch) + chains.at(i).mRealyAccepted;

        if (i == index) {
            // could be done with
            //accept.resize(chainSize
            //std::copy(from_vector.begin(), from_vector.end(), to_vector.begin());

            for (int j = 0; j < chainSize; ++j)
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
    int aquisition = 0.;//std::accumulate(chains.begin(), chains.end(), 0, [](ChainSpecs chain){return  chain.mIterPerAquisition;});

    mGlobalAcceptationPerCent = 0;
    for (qsizetype i = 0 ; i<chains.size(); i++) {
        aquisition += chains.at(i).mAquisitionIterIndex/chains.at(i).mThinningInterval;
        mGlobalAcceptationPerCent += mAllAccepts.at(i);
    }

    mGlobalAcceptationPerCent = mGlobalAcceptationPerCent / aquisition * 100.;

}


void MHVariable::generateNumericalResults(const QList<ChainSpecs> &chains)
{
    MetropolisVariable::generateNumericalResults(chains);
    generateGlobalRunAcceptation(chains);
}

QString MHVariable::resultsString(const QString &noResultMessage, const QString &unit) const
{
    if (mSamplerProposal != MHVariable::eFixe) {
        const QString result = MetropolisVariable::resultsString(noResultMessage, unit);
        const QString globalTxt = stringForLocal(mGlobalAcceptationPerCent);

        return result + "<br>" + QObject::tr("Acceptance rate (all acquire iterations) : %1 % (%2)").arg(globalTxt, getSamplerProposalText(mSamplerProposal));

    } else {
        return QObject::tr("Fixed value : %1").arg(stringForLocal(mFormatedTrace->at(0))); // for VG mX is Variance and we need Std gi
    }

}


QString MHVariable::getSamplerProposalText(const MHVariable::SamplerProposal sp)
{
    switch (sp) {
    case MHVariable::eFixe:
        return FixeStr;
        break;
    // Event
    case MHVariable::eMHAdaptGauss:
        return MHAdaptGaussStr;
        break;

    case MHVariable::eBoxMuller:
        return BoxMullerStr;
        break;

    case MHVariable::eDoubleExp:
        return DoubleExpStr;
        break;

    // Data
    case MHVariable::eInversion:
        return InversionStr;
        break;
    case MHVariable::eMHPrior:
        return MHDatePriorStr;
        break;

    default:
        return QObject::tr("Unknown");
        break;

    }
}

MHVariable::SamplerProposal MHVariable::getSamplerProposalFromText(const QString& text)
{
    if (text == MHAdaptGaussStr)
        return MHVariable::eMHAdaptGauss;

    else if (text == BoxMullerStr)
        return MHVariable::eBoxMuller;

    else if (text == DoubleExpStr)
        return MHVariable::eDoubleExp;

    else if (text == InversionStr)
        return MHVariable::eInversion;

    else if (text == MHDatePriorStr)
        return MHVariable::eMHPrior;

    else if (text == FixeStr)
        return MHVariable::eFixe;
    else {
        // ouch... what to do ???
        return MHVariable::eMHAdaptGauss;
    }
}


QDataStream &operator<<( QDataStream &stream, const MHVariable &data )
{
    stream << dynamic_cast<const MetropolisVariable&>(data);

    /* owned by MHVariable*/
    stream << data.mAllAccepts;
    stream << *(data.mHistoryAcceptRateMH);
    stream << data.mLastAccepts;

    stream << data.mSigmaMH;
    stream << data.mSamplerProposal;

    return stream;
}

QDataStream &operator>>( QDataStream &stream, MHVariable &data )
{
    /* herited from MetropolisVariable*/
    stream >> dynamic_cast<MetropolisVariable&>(data);

    data.mAllAccepts.clear();
    stream >> data.mAllAccepts;

    if (data.mHistoryAcceptRateMH)
        data.mHistoryAcceptRateMH->clear();
    else
        data.mHistoryAcceptRateMH = new QList<double>();
    stream >> *(data.mHistoryAcceptRateMH);

    if (!data.mLastAccepts.isEmpty())
        data.mLastAccepts.clear();

    stream >> data.mLastAccepts;

    stream >> data.mSigmaMH;
    stream >> data.mSamplerProposal;

    return stream;

}
=======
﻿#include "MHVariable.h"
#include "StdUtilities.h"
#include "Generator.h"
#include <QDebug>


MHVariable::MHVariable():
mLastAcceptsLength(0),
//mAllAccepts(nullptr),
mGlobalAcceptation(0),
mHistoryAcceptRateMH(nullptr)
{
  /*mAllAccepts = new (std::nothrow) QVector<bool>();
  mHistoryAcceptRateMH = new (std::nothrow) QVector<double>();*/
  mAllAccepts = new QVector<bool>();
  mHistoryAcceptRateMH = new QVector<double>();

}

MHVariable::~MHVariable()
{
   mAllAccepts->clear();
   mHistoryAcceptRateMH->clear();
}

bool MHVariable::tryUpdate(const double x, const double rapport)
{
    if (mLastAccepts.length() >= mLastAcceptsLength)
        mLastAccepts.removeAt(0);

    bool accepted = false;
    
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

   // mLastAccepts.clear();
  //  mAllAccepts.clear();// mAllAccepts.clear(); //don't clean, avalable for cumulate chain

  //  if(!mAllAccepts)
  //      mAllAccepts = new (std::nothrow) QVector<bool>();
   /* mHistoryAcceptRateMH = new (std::nothrow) QVector<float>();

    if (!mHistoryAcceptRateMH->isEmpty())
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

double MHVariable::getCurrentAcceptRate()
{
    double sum (0.);

    sum = std::accumulate(mLastAccepts.begin(), mLastAccepts.end(), sum,[](double s, double a){return s+(a ? 1.f : 0.f);}); //#include <numeric>

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
    int shift = 0;
    const int reserveSize = (int) ceil(chains.at(index).mNumBurnIter + (chains.at(index).mBatchIndex * chains.at(index).mNumBatchIter) + chains.at(index).mNumRunIter / chains.at(index).mThinningInterval);

    accept.reserve(reserveSize);

    for(int i=0; i<chains.size(); ++i) {
        int chainSize = chains.at(i).mNumBurnIter + (chains.at(i).mBatchIndex * chains.at(i).mNumBatchIter) + chains.at(i).mNumRunIter / chains.at(i).mThinningInterval;
        

        if(i == index) {
            // could be done with
            //accept.resize(chainSize
            //std::copy(from_vector.begin(), from_vector.end(), to_vector.begin());

            for(int j=0; j<chainSize; ++j)
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
    double accepted = 0.;
    double acceptsLength = 0.;
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
        data.mHistoryAcceptRateMH = new QVector<double>();
    stream >> *(data.mHistoryAcceptRateMH);

    stream >> data.mSigmaMH;

    return stream;

}


>>>>>>> Stashed changes
