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

#define MHAdaptGaussStr QObject::tr("Proposal : Adapt. Gaussian random walk")
// Only for Event
#define BoxMullerStr QObject::tr("Proposal : Gaussian (Event Prior)")
#define DoubleExpStr QObject::tr("Proposal : Double-Exponential")
// Only for Date
#define MHDatePriorStr QObject::tr("Proposal : Gaussian (Date Prior)")
#define InversionStr QObject::tr("Proposal : Distribution of Calibrated Date")
//#define MHSymGaussAdaptStr QObject::tr("Adapt. Gaussian random walk") // Obsolete

/** Default constructor */
MHVariable::MHVariable():
    MetropolisVariable(),
    mSigmaMH(0),
    mLastAccepts(),
    mLastAcceptsLength(0),
    mNbValuesAccepted(),
    mGlobalAcceptationPerCent(0.),
    //mHistoryAcceptRateMH(new std::vector<double>()),
    mHistoryAcceptRateMH(std::make_shared<std::vector<double>>()),
    mSamplerProposal(eDoubleExp)
{
}

/** Copy constructor */
MHVariable::MHVariable(const MHVariable& origin):
    MetropolisVariable(origin),
    mSigmaMH(origin.mSigmaMH),
    mLastAccepts(origin.mLastAccepts),
    mLastAcceptsLength(origin.mLastAcceptsLength),
    mNbValuesAccepted(origin.mNbValuesAccepted),
    mGlobalAcceptationPerCent(origin.mGlobalAcceptationPerCent),
    mSamplerProposal(origin.mSamplerProposal)
{

    //mHistoryAcceptRateMH = std::shared_ptr<std::vector<double>>(origin.mHistoryAcceptRateMH);
    //*mHistoryAcceptRateMH = *origin.mHistoryAcceptRateMH;
    mHistoryAcceptRateMH = std::make_shared<std::vector<double>>(*origin.mHistoryAcceptRateMH);
#ifdef DEBUG
    if (mHistoryAcceptRateMH->empty()&& !origin.mHistoryAcceptRateMH->empty()) {
        qDebug()<<"[MHVariable::MHVariable]" << _name;
    }
#endif

}

/** move constructor */
MHVariable::MHVariable(MHVariable&& other) noexcept
{
    MetropolisVariable(std::move(other));
    mSigmaMH = std::move(other.mSigmaMH);
    mLastAccepts = std::move(other.mLastAccepts);
    mLastAcceptsLength = std::move(other.mLastAcceptsLength);
    mNbValuesAccepted = std::move(other.mNbValuesAccepted);
    mGlobalAcceptationPerCent = std::move(other.mGlobalAcceptationPerCent);
    mSamplerProposal = std::move(other.mSamplerProposal);
    //mHistoryAcceptRateMH.swap(other.mHistoryAcceptRateMH);
    mHistoryAcceptRateMH = std::move(other.mHistoryAcceptRateMH);
#ifdef DEBUG
    if (mHistoryAcceptRateMH->empty()&& !other.mHistoryAcceptRateMH->empty()) {
        qDebug()<<"[MHVariable::MHVariable]" <<_name;
    }
#endif


}

MHVariable::MHVariable(const MetropolisVariable& origin):
    MetropolisVariable(origin),
    mSigmaMH(0),
    mLastAccepts(),
    mLastAcceptsLength(0),
    mNbValuesAccepted(),
    mGlobalAcceptationPerCent(0.),
    //mHistoryAcceptRateMH(new std::vector<double>()),
    mHistoryAcceptRateMH(std::make_shared<std::vector<double>>()),
    mSamplerProposal(eDoubleExp)
{
#ifdef DEBUG
    if (mHistoryAcceptRateMH->empty()) {
        qDebug()<<"[MHVariable::MHVariable]" << _name;
    }
#endif
}

MHVariable::~MHVariable()
{
    //qDebug() << "[MHVariable::~MHVariable] ";//<< (mName.isNull()? " Deleted Name": mName);

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
        mLastAccepts.erase(mLastAccepts.begin());//removeAt(0);

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
    mLastAccepts.push_back(accepted);

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

void MHVariable::clear()
{
    MetropolisVariable::clear();
    if (mHistoryAcceptRateMH) {
        mHistoryAcceptRateMH->clear();
        //qDebug()<<"[MHVariable::clear] mHistoryAcceptRateMH"<< _name;
    }

    mLastAccepts.clear();

    mNbValuesAccepted.clear();

}

void MHVariable::shrink_to_fit() noexcept
{
    MetropolisVariable::shrink_to_fit();
    if (mHistoryAcceptRateMH) {
        mHistoryAcceptRateMH->shrink_to_fit();
    }
    mLastAccepts.shrink_to_fit();
    mNbValuesAccepted.shrink_to_fit();

}

void MHVariable::clear_and_shrink() noexcept
{
    MetropolisVariable::clear_and_shrink();
    if (mHistoryAcceptRateMH) {
        mHistoryAcceptRateMH->clear();
        //qDebug()<<"[MHVariable::clear_and_shrink] mHistoryAcceptRateMH"<< _name;
        mHistoryAcceptRateMH->shrink_to_fit();
    }

    mLastAccepts.clear();
    mLastAccepts.shrink_to_fit();

    mNbValuesAccepted.clear();
    mNbValuesAccepted.shrink_to_fit();

}

void MHVariable::remove_smoothed_densities()
{
    MetropolisVariable::remove_smoothed_densities();
}

void MHVariable::reserve(const size_t reserve)
{
    MetropolisVariable::reserve(reserve);
    mNbValuesAccepted.reserve(reserve);

}



MHVariable& MHVariable::operator=(const MHVariable& origin)
{
    MetropolisVariable::operator=(origin);
    
    mSigmaMH = origin.mSigmaMH;
    mLastAccepts = origin.mLastAccepts;
    mLastAcceptsLength = origin.mLastAcceptsLength;

    mNbValuesAccepted = origin.mNbValuesAccepted;

    mGlobalAcceptationPerCent = origin.mGlobalAcceptationPerCent;

    mHistoryAcceptRateMH = std::shared_ptr<std::vector<double>>(origin.mHistoryAcceptRateMH);
#ifdef DEBUG
    if (mHistoryAcceptRateMH->empty() && !origin.mHistoryAcceptRateMH->empty()) {
        qDebug()<<"[MHVariable::MHVariable:: operator =]" << _name;
    }
#endif
      
    mSamplerProposal = origin.mSamplerProposal;
    return *this;
}

double MHVariable::getCurrentAcceptRate() const
{
   if (mLastAccepts.empty())
        return 0.;

    return std::count_if(mLastAccepts.begin(), mLastAccepts.end(), [](bool i) { return i; })/ (double) mLastAccepts.size();

}

void MHVariable::saveCurrentAcceptRate()
{
    mHistoryAcceptRateMH->push_back(100. * getCurrentAcceptRate());
   // qDebug()<<"[MHVariable::saveCurrentAcceptRate] "<<_name <<" size=" << mHistoryAcceptRateMH->size();
}

std::vector<double> MHVariable::acceptationForChain(const std::vector<ChainSpecs> &chains, size_t index)
{
    std::vector<double> accept(0);
    size_t shift = 0;

    for (size_t i = 0; i < chains.size(); ++i) {
        // We add 1 for the init
        const size_t chainSize = 1 +chains.at(i).mIterPerBurn + (chains.at(i).mBatchIndex * chains.at(i).mIterPerBatch) + chains.at(i).mRealyAccepted;

        if (i == index) {
            // could be done with
            //accept.resize(chainSize
            //std::copy(from_vector.begin(), from_vector.end(), to_vector.begin());
            if (mHistoryAcceptRateMH->size() < shift+chainSize) {
                qDebug()<< "[MHVariable::acceptationForChain] variable : "<< _name << "No mHistoryAcceptRateMH";
                return accept;
            }

            for (size_t j = 0; j < chainSize; ++j)
                accept.push_back(mHistoryAcceptRateMH->at(shift + j));

            break;
        }
        else
            shift += chainSize;
    }
    return accept;
}



void MHVariable::generateGlobalRunAcceptation(const std::vector<ChainSpecs> &chains)
{
    int aquisition = 0.;

    mGlobalAcceptationPerCent = 0;
    for (size_t i = 0 ; i<chains.size(); i++) {
        aquisition += chains.at(i).mAquisitionIterIndex/chains.at(i).mThinningInterval;
        mGlobalAcceptationPerCent += mNbValuesAccepted.at(i);
    }

    mGlobalAcceptationPerCent = mGlobalAcceptationPerCent / aquisition * 100.;

}


void MHVariable::generateNumericalResults(const std::vector<ChainSpecs> &chains)
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
        return QObject::tr("Fixed value : %1 %2").arg(stringForLocal(mFormatedTrace->at(0)), unit); // for VG mX is Variance and we need Std gi
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

/**
 write stream
 */
QDataStream &operator<<( QDataStream& stream, const MHVariable& data )
{
    stream << dynamic_cast<const MetropolisVariable&>(data);

    /* owned by MHVariable*/
    //stream << data.mNbValuesAccepted;
    save_container(stream, data.mNbValuesAccepted);
    save_container(stream, *data.mHistoryAcceptRateMH);
    
    //stream << data.mLastAccepts;
    save_container(stream, data.mLastAccepts);

    stream << data.mSigmaMH;
    stream << data.mSamplerProposal;

    return stream;
}

/**
 read stream
 */
void MHVariable::load_stream_v328(QDataStream& stream)
{
    /* herited from MetropolisVariable*/
    MetropolisVariable::load_stream_v328(stream);

    mNbValuesAccepted.clear();
    //stream >> data.mNbValuesAccepted;
    load_container(stream, mNbValuesAccepted);

    //data.mHistoryAcceptRateMH = load_std_vector_ptr(stream);
    reload_shared_ptr(mHistoryAcceptRateMH, stream);

    if (!mLastAccepts.empty())
        mLastAccepts.clear();

    //stream >> data.mLastAccepts;
    load_container(stream, mLastAccepts);

    stream >> mSigmaMH;
    stream >> mSamplerProposal;

}

void MHVariable::load_stream_v327(QDataStream& stream)
{
    /* herited from MetropolisVariable*/
    MetropolisVariable::load_stream_v328(stream);


    mNbValuesAccepted.clear();

    QList<long long > tmpAll;
    stream >> tmpAll;
    mNbValuesAccepted = std::vector<long long>(tmpAll.begin(), tmpAll.end());

    //data.mHistoryAcceptRateMH = load_std_vector_ptr(stream);
    reload_shared_ptr(mHistoryAcceptRateMH, stream); // à revoir

    if (!mLastAccepts.empty())
        mLastAccepts.clear();

    //stream >> data.mLastAccepts;
    QList<bool> tmpbool;
    stream >> tmpbool;
    mLastAccepts = std::vector<bool>(tmpbool.begin(), tmpbool.end());

    stream >> mSigmaMH;
    stream >> mSamplerProposal;

}

QDataStream &operator>>(QDataStream& stream, MHVariable& data )
{
    /* herited from MetropolisVariable*/
    MetropolisVariable metro_data;
    stream >> metro_data;
    
    const MHVariable tmp_data (metro_data);
    data = tmp_data;
    
    data.mNbValuesAccepted.clear();
    //stream >> data.mNbValuesAccepted;
    load_container(stream, data.mNbValuesAccepted);

    //data.mHistoryAcceptRateMH = load_std_vector_ptr(stream);
    reload_shared_ptr(data.mHistoryAcceptRateMH, stream);
    
    if (!data.mLastAccepts.empty())
        data.mLastAccepts.clear();

    //stream >> data.mLastAccepts;
    load_container(stream, data.mLastAccepts);

    stream >> data.mSigmaMH;
    stream >> data.mSamplerProposal;

    return stream;

}

