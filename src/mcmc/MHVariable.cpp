/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2026

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

#include <QDebug>

#define NoneStr QObject::tr("No Proposal")
#define FixeStr QObject::tr("Fixed value")
#define PriorStr QObject::tr("Proposal: Prior")
#define AdaptiveGaussianStr QObject::tr("Proposal: Adaptive Gaussian random walk")

// Only for Event
//#define EventPriorStr QObject::tr("Proposal : Gaussian (Event Prior)")
#define EventPriorStr QObject::tr("Proposal: Event Prior")
#define DoubleExpStr QObject::tr("Proposal: Double-Exponential")
// Only for Date
//#define DatePriorStr QObject::tr("Proposal: Gaussian (Date Prior)")
#define DatePriorStr QObject::tr("Proposal: Date Prior")
//#define LikelihoodStr QObject::tr("Proposal: Distribution of Calibrated Date")
#define LikelihoodStr QObject::tr("Proposal: Date likelihood")

/** Default constructor */
MHVariable::MHVariable():
    MetropolisVariable(),
    mSigmaMH(0),
    mLastMHAccepts(),
    mLastMHAcceptsLength(0),
    mMHAcceptcountSinceAquire(),
    mGlobalAcceptationPerCent(0.0),
    mHistoryAcceptRateMH(std::make_shared<std::vector<double>>()),
    mSamplerProposal(SamplerProposal::eNone)
{
}

/** Copy constructor */
MHVariable::MHVariable(const MHVariable& origin):
    MetropolisVariable(origin),
    mSigmaMH(origin.mSigmaMH),
    mLastMHAccepts(origin.mLastMHAccepts),
    mLastMHAcceptsLength(origin.mLastMHAcceptsLength),
    mMHAcceptcountSinceAquire(origin.mMHAcceptcountSinceAquire),
    mGlobalAcceptationPerCent(origin.mGlobalAcceptationPerCent),
    mSamplerProposal(origin.mSamplerProposal)
{
    mHistoryAcceptRateMH = std::make_shared<std::vector<double>>(*origin.mHistoryAcceptRateMH);
#ifdef DEBUG
    if (mHistoryAcceptRateMH->empty()&& !origin.mHistoryAcceptRateMH->empty()) {
        qDebug()<<"[MHVariable::MHVariable]" << QString::fromStdString(mName);
    }
#endif

}

/** move constructor */
MHVariable::MHVariable(MHVariable&& other) noexcept
{
    MetropolisVariable(std::move(other));
    mSigmaMH = std::move(other.mSigmaMH);
    mLastMHAccepts = std::move(other.mLastMHAccepts);
    mLastMHAcceptsLength = std::move(other.mLastMHAcceptsLength);
    mMHAcceptcountSinceAquire = std::move(other.mMHAcceptcountSinceAquire);
    mGlobalAcceptationPerCent = std::move(other.mGlobalAcceptationPerCent);
    mSamplerProposal = std::move(other.mSamplerProposal);
    mHistoryAcceptRateMH = std::move(other.mHistoryAcceptRateMH);
#ifdef DEBUG
    if (mHistoryAcceptRateMH->empty()&& !other.mHistoryAcceptRateMH->empty()) {
        qDebug()<<"[MHVariable::MHVariable]" <<QString::fromStdString(mName);
    }
#endif


}

MHVariable::MHVariable(const MetropolisVariable& origin):
    MetropolisVariable(origin),
    mSigmaMH(0),
    mLastMHAccepts(),
    mLastMHAcceptsLength(0),
    mMHAcceptcountSinceAquire(),
    mGlobalAcceptationPerCent(0.0),
    mHistoryAcceptRateMH(std::make_shared<std::vector<double>>()),
    mSamplerProposal(SamplerProposal::eNone)
{
#ifdef DEBUG
    if (mHistoryAcceptRateMH->empty()) {
        qDebug()<<"[MHVariable::MHVariable]" << QString::fromStdString(mName);
    }
#endif
}

MHVariable::~MHVariable()
{
    //qDebug() << "[MHVariable::~MHVariable] ";//<< (mName.isNull()? " Deleted Name": mName);

}










/**
 * @brief MHVariable::adapt
 * @param coef_min value [0; 1], default 0.42
 * @param coef_max value [0; 1], default 0.46
 * @return bool if no adaptation needed
 */
/*bool MHVariable::adapt (const double coef_min, const double coef_max, const double delta)
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
}*/

/*bool MHVariable::adapt(double coef_min, double coef_max,
                       double delta, double sigma_min, double sigma_max)
{
    bool stillAdapted = true;                     // true → pas besoin d’ajustement
    const double acceptRate = getCurrentAcceptRate();   // taux d’acceptation sur la fenêtre courante

    if (acceptRate <= coef_min || acceptRate >= coef_max) {
        // on doit changer l’échelle
        const double sign = (acceptRate <= coef_min) ? -1.0 : 1.0;
        // mise à jour multiplicative sur le log10
        mSigmaMH *= std::pow(10.0, sign * delta);

        // on impose les bornes (containment)
        if (mSigmaMH < sigma_min) mSigmaMH = sigma_min;
        if (mSigmaMH > sigma_max) mSigmaMH = sigma_max;

        stillAdapted = false;                    // adaptation a eu lieu
    }
    return stillAdapted;
}*/

// ---------------------------------------------------------------
// 2.  Méthode d'adaptation d'une variable MH (mise à jour du sigma)
// ---------------------------------------------------------------
/**
 * @brief Adaptation du paramètre de proposition Metropolis–Hastings
 *        par approximation stochastique de type Robbins–Monro.
 *
 * Cette fonction ajuste dynamiquement l'écart-type de la loi de proposition
 * (`mSigmaMH`) afin de maintenir le taux d'acceptation de Metropolis–Hastings
 * dans un intervalle cible donné.
 *
 * L'adaptation repose sur un schéma de Robbins–Monro avec pas décroissant :
 * \f[
 *   \gamma_t = \frac{c}{(t + t_0)^\kappa}
 * \f]
 * et une mise à jour multiplicative sur l'échelle logarithmique de \f$\sigma\f$ :
 * \f[
 *   \log(\sigma_{t+1}) = \log(\sigma_t) \pm \gamma_t
 * \f]
 *
 * L'algorithme inclut :
 * - une fenêtre d'acceptation cible [coef_min, coef_max],
 * - une condition de compacité (containment) garantissant
 *   l'ergodicité de la chaîne adaptative,
 * - une décroissance du pas d'apprentissage assurant
 *   la diminution de l'adaptation au cours du temps.
 *
 * Cette approche est conforme aux cadres théoriques de l'Adaptive MCMC
 * (Haario et al., Andrieu & Moulines).
 *
 * @param coef_min Taux d'acceptation minimal acceptable.
 * @param coef_max Taux d'acceptation maximal acceptable.
 * @param batchIndex Indice de batch courant (temps discret de l'adaptation).
 * @param sigma_min Borne inférieure autorisée pour \f$\sigma\f$.
 * @param sigma_max Borne supérieure autorisée pour \f$\sigma\f$.
 * @param c Constante de pas de Robbins–Monro (amplitude de l'adaptation).
 * @param kappa Exposant de décroissance du pas (\f$0.5 < \kappa \le 1\f$).
 * @param t0 Décalage temporel pour stabiliser les premières itérations.
 *
 * @return true si aucune adaptation n'a été nécessaire (taux dans l'intervalle cible),
 *         false si le paramètre de proposition a été ajusté.
 */

bool MHVariable::adapt(double coef_min, double coef_max,
                       size_t batchIndex,
                       double sigma_min, double sigma_max,
                       double c, double kappa, double t0 )
{
    // le pas d'apprentissage qui décroit (Robbins‑Monro)
    const double gamma_t = c / std::pow(static_cast<double>(batchIndex) + t0, kappa);

    const double acceptRate = getCurrentAcceptRate(); // fenêtre glissante

    bool stillAdapted = true; // true → aucune adaptation nécessaire

    if (acceptRate <= coef_min || acceptRate >= coef_max) {
        const double sign = (acceptRate <= coef_min) ? -1.0 : 1.0;
        // mise à jour multiplicative sur le log10
        mSigmaMH *= std::pow(10.0, sign * gamma_t);

        /** 4️⃣ Containment ✔️ (condition clé en Adaptive MCMC)
        C’est la condition de compacité (Andrieu & Moulines, 2006)
        indispensable pour garantir l’ergodicité de la chaîne adaptative.
        */
#ifdef DEBUG_no
        std::cout << " mSigmaMH=" << mSigmaMH << std::endl;
        if (mSigmaMH < sigma_min) {
            // On dépasse la borne inférieure → on la corrige
            qWarning() << "[" << __func__ << "]"
                       << "mSigmaMH (" << mSigmaMH << ") < sigma_min ("
                       << sigma_min << "); clamping to sigma_min.";
            mSigmaMH = sigma_min;
        }
        if (mSigmaMH > sigma_max) {
            // On dépasse la borne supérieure → on la corrige
            qWarning() << "[" << __func__ << "]"
                       << "mSigmaMH (" << mSigmaMH << ") > sigma_max ("
                       << sigma_max << "); clamping to sigma_max.";
            mSigmaMH = sigma_max;
        }
#else
        if (mSigmaMH < sigma_min) mSigmaMH = sigma_min;
        if (mSigmaMH > sigma_max) mSigmaMH = sigma_max;
#endif
        stillAdapted = false; // on a effectivement adapté
    }
    return stillAdapted;
}

bool MHVariable::adapt_Robbins_Monro(size_t batchIndex,
                       double targetAcceptRate,
                       double sigma_min, double sigma_max,
                       double c, double kappa, double t0)
{
    // Pas de Robbins-Monro décroissant
    const double gamma_t = c / std::pow(static_cast<double>(batchIndex) + t0, kappa);
    const double acceptRate = getCurrentAcceptRate();

    // Correction continue sur l'échelle log10
    const double log10_sigma = std::log10(mSigmaMH) + gamma_t * (acceptRate - targetAcceptRate);
    mSigmaMH = std::pow(10.0, log10_sigma);

    // Containment (bornage)
    bool clamped = false;
    if (mSigmaMH < sigma_min) { mSigmaMH = sigma_min; clamped = true; }
    if (mSigmaMH > sigma_max) { mSigmaMH = sigma_max; clamped = true; }

    // Indispensable : réinitialisation des compteurs pour le batch suivant
    //resetBatchAcceptanceCounters();

    return !clamped;
}

void MHVariable::clear()
{
    MetropolisVariable::clear();
    if (mHistoryAcceptRateMH) {
        mHistoryAcceptRateMH->clear();
    }

    mLastMHAccepts.clear();

}

void MHVariable::shrink_to_fit() noexcept
{
    MetropolisVariable::shrink_to_fit();
    if (mHistoryAcceptRateMH) {
        mHistoryAcceptRateMH->shrink_to_fit();
    }
    mLastMHAccepts.shrink_to_fit();

}

void MHVariable::clear_and_shrink() noexcept
{
    MetropolisVariable::clear_and_shrink();
    if (mHistoryAcceptRateMH) {
        mHistoryAcceptRateMH->clear();
        mHistoryAcceptRateMH->shrink_to_fit();
    }

    mLastMHAccepts.clear();
    mLastMHAccepts.shrink_to_fit();

    //mAllMHAccepts.clear();


}

void MHVariable::remove_smoothed_densities()
{
    MetropolisVariable::remove_smoothed_densities();
}

void MHVariable::reserve(const size_t reserve)
{
    MetropolisVariable::reserve(reserve);
    //mAllMHAccepts.reserve(reserve);

}


MHVariable& MHVariable::operator=(const MHVariable& origin)
{
    MetropolisVariable::operator=(origin);
    
    mSigmaMH = origin.mSigmaMH;
    mLastMHAccepts = origin.mLastMHAccepts;
    mLastMHAcceptsLength = origin.mLastMHAcceptsLength;

    mMHAcceptcountSinceAquire = origin.mMHAcceptcountSinceAquire;

    mGlobalAcceptationPerCent = origin.mGlobalAcceptationPerCent;

    mHistoryAcceptRateMH = std::shared_ptr<std::vector<double>>(origin.mHistoryAcceptRateMH);
#ifdef DEBUG
    if (mHistoryAcceptRateMH->empty() && !origin.mHistoryAcceptRateMH->empty()) {
        qDebug()<<"[MHVariable::MHVariable:: operator =]" << QString::fromStdString(mName);
    }
#endif
      
    mSamplerProposal = origin.mSamplerProposal;
    return *this;
}





std::vector<double> MHVariable::acceptationForChain(const std::vector<ChainSpecs> &chains, size_t index)
{
    std::vector<double> accept(0);
    size_t shift = 0;

    for (size_t i = 0; i < chains.size(); ++i) {
        // We add 1 for the init
        const size_t chainSize = 1 + chains[i].mIterPerBurn + (chains[i].mBatchIndex * chains[i].mIterPerBatch) + chains[i].mRealyAccepted;

        if (i == index) {
            // could be done with
            //accept.resize(chainSize
            //std::copy(from_vector.begin(), from_vector.end(), to_vector.begin());
            if (mHistoryAcceptRateMH->size() < shift+chainSize) {
                qDebug()<< "[MHVariable::acceptationForChain] variable : "<< QString::fromStdString(mName) << " No mHistoryAcceptRateMH";
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
    double aquisition = 0;

    mGlobalAcceptationPerCent = 0;
    for (size_t i = 0 ; i<chains.size(); i++) {
        aquisition += chains[i].mAquisitionIterIndex / chains[i].mThinningInterval;
     }

    mGlobalAcceptationPerCent = mMHAcceptcountSinceAquire / aquisition * 100.;
}


/*void MHVariable::generateNumericalResults(const std::vector<ChainSpecs> &chains)
{
    MetropolisVariable::generateNumericalResults(chains);
    generateGlobalRunAcceptation(chains);
}*/

// doit être fait à chaque modification du lissage
void MHVariable::generateDensityNumericalResults(const std::vector<ChainSpecs>& chains)
{
    MetropolisVariable::generateDensityNumericalResults(chains);
}

// Peut être fait une fois à la sortie des iterations
void MHVariable::generateTraceNumericalResults(const std::vector<ChainSpecs>& chains)
{
    MetropolisVariable::generateTraceNumericalResults(chains);
    generateGlobalRunAcceptation(chains);
}


QString MHVariable::resultsString(const QString &noResultMessage, const QString &unit) const
{
    if (mSamplerProposal != SamplerProposal::eFixe) {
        const QString result = MetropolisVariable::resultsString(noResultMessage, unit);
        const QString globalTxt = stringForLocal(mGlobalAcceptationPerCent);

        return result + "<br>" + QObject::tr("Acceptance rate (all acquire iterations) : %1 % (%2)").arg(globalTxt, getSamplerProposalText(mSamplerProposal));

    } else {
        return QObject::tr("Fixed value : %1 %2").arg(stringForLocal(mX), unit); // for VG mX is Variance and we need Std gi
    }

}


QString MHVariable::getSamplerProposalText(const SamplerProposal sp)
{
    switch (sp) {
    case SamplerProposal::eNone:
        return NoneStr;
        break;
    case SamplerProposal::eFixe:
        return FixeStr;
        break;
    case SamplerProposal::ePrior:
        return PriorStr;
        break;
    // Event
    case SamplerProposal::eRWAdaptGauss:
        return AdaptiveGaussianStr;
        break;

    case SamplerProposal::eEventPrior:
        return EventPriorStr;
        break;

    case SamplerProposal::eDoubleExp:
        return DoubleExpStr;
        break;

    // Data
    case SamplerProposal::eLikelihood:
        return LikelihoodStr;
        break;
    case SamplerProposal::eDatePrior:
        return DatePriorStr;
        break;

    default:
        return QObject::tr("Unknown");
        break;

    }
}

SamplerProposal MHVariable::getSamplerProposalFromText(const QString& text)
{
    if (text == AdaptiveGaussianStr)
        return SamplerProposal::eRWAdaptGauss;

    else if (text == EventPriorStr)
        return SamplerProposal::eEventPrior;

    else if (text == DoubleExpStr)
        return SamplerProposal::eDoubleExp;

    else if (text == LikelihoodStr)
        return SamplerProposal::eLikelihood;

    else if (text == DatePriorStr)
        return SamplerProposal::eDatePrior;

    else if (text == FixeStr)
        return SamplerProposal::eFixe;

    else if (text == NoneStr)
        return SamplerProposal::eNone;
    else if (text == PriorStr)
        return SamplerProposal::ePrior;
    else {
        // ouch... what to do ???
        return SamplerProposal::eRWAdaptGauss;
    }
}

/**
 write stream
 */
QDataStream &operator<<( QDataStream& stream, const MHVariable& data )
{
    stream << dynamic_cast<const MetropolisVariable&>(data);

    /* owned by MHVariable*/

    stream << static_cast<qint64>(data.mLastMHAcceptsLength);

    //save_container(stream, data.mNbValuesAccepted);
    save_container_nullable(stream, data.mHistoryAcceptRateMH);
    
    //stream << data.mLastMHAccepts;
    save_container(stream, data.mLastMHAccepts);

    stream << data.mSigmaMH;
    stream << data.mSamplerProposal;

    return stream;
}

/**
 read stream
 */

void MHVariable::load_stream_v327(QDataStream& stream)
{
    /* herited from MetropolisVariable*/
    MetropolisVariable::load_stream_v328(stream);

    //mNbValuesAccepted.clear();

    //load_container(stream, mNbValuesAccepted);
    load_container(stream, mHistoryAcceptRateMH);

    if (!mLastMHAccepts.empty())
        mLastMHAccepts.clear();

    load_container(stream, mLastMHAccepts);

    stream >> mSigmaMH;
    stream >> mSamplerProposal;

}

void MHVariable::load_stream_v328(QDataStream& stream)
{
    /* herited from MetropolisVariable*/
    MetropolisVariable::load_stream_v328(stream);

    //mNbValuesAccepted.clear();

    //load_container(stream, mNbValuesAccepted);

    load_container_nullable(stream, mHistoryAcceptRateMH);

    if (!mLastMHAccepts.empty())
        mLastMHAccepts.clear();

    load_container(stream, mLastMHAccepts);

    stream >> mSigmaMH;
    stream >> mSamplerProposal;

}

void MHVariable::load_stream_v330(QDataStream& stream)
{
    /* herited from MetropolisVariable*/
    MetropolisVariable::load_stream_v330(stream);

    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_stream_v330]  erreur de flux ; stream.status()=" << stream.status();
        // throw std::runtime_error("Error reading from stream");
        // return;
    }

    qint64 l;
    stream >> l;
    mLastMHAcceptsLength = l;

    std::vector <long long> mNbValuesAccepted; // not used since v337
    load_container(stream, mNbValuesAccepted);

    load_container_nullable(stream, mHistoryAcceptRateMH);

    load_container(stream, mLastMHAccepts);

    stream >> mSigmaMH;
    stream >> mSamplerProposal;

}

void MHVariable::load_stream_v337(QDataStream& stream)
{
    /* herited from MetropolisVariable*/
    MetropolisVariable::load_stream_v337(stream);

    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_stream_v330]  erreur de flux ; stream.status()=" << stream.status();
        // throw std::runtime_error("Error reading from stream");
        // return;
    }

    qint64 l;
    stream >> l;
    mLastMHAcceptsLength = l;

    //load_container(stream, mNbValuesAccepted);

    load_container_nullable(stream, mHistoryAcceptRateMH);

    load_container(stream, mLastMHAccepts);

    stream >> mSigmaMH;
    stream >> mSamplerProposal;

}

void MHVariable::load_stream_v338(QDataStream& stream)
{
    // -------------------------------------------------
    // 1️⃣   Chargement de la partie de la classe de base
    // -------------------------------------------------
    MetropolisVariable::load_stream_v338(stream);

    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_stream_v338] erreur après MetropolisVariable::load_stream_v338"
                 << stream.status() << stream.device()->errorString();
        throw std::runtime_error("[MHVariable::load_stream_v338] Error reading base class data");
    }
    // -------------------------------------------------
    // 2️⃣  Lecture de mLastMHAcceptsLength (qint64)
    // -------------------------------------------------
    qint64 l = 0;
    stream >> l;
    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_stream_v338] erreur lecture mLastMHAcceptsLength"
                 << stream.status() << stream.device()->errorString();
        throw std::runtime_error("[MHVariable::load_stream_v338] Error reading mLastMHAcceptsLength");
    }
    if (l < 0) {
        qDebug() << "[QtUtilities::load_stream_v338] valeur négative pour mLastMHAcceptsLength:" << l;
        throw std::runtime_error("[MHVariable::load_stream_v338] Invalid mLastMHAcceptsLength (negative)");
    }
    mLastMHAcceptsLength = l;

    // -------------------------------------------------
    // 3️⃣  Chargement du conteneur nullable
    // -------------------------------------------------
    load_container_nullable(stream, mHistoryAcceptRateMH);
    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_stream_v338] erreur lecture mHistoryAcceptRateMH"
                 << stream.status() << stream.device()->errorString();
        throw std::runtime_error("[MHVariable::load_stream_v338] Error reading mHistoryAcceptRateMH");
    }

    // -------------------------------------------------
    // 4️⃣  Chargement du vecteur d'acceptations
    // -------------------------------------------------
    load_container(stream, mLastMHAccepts);
    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_stream_v338] erreur lecture mLastMHAccepts"
                 << stream.status() << stream.device()->errorString();
        throw std::runtime_error("[MHVariable::load_stream_v338] Error reading mLastMHAccepts");
    }
    // -------------------------------------------------
    // 5️⃣  Lecture de mSigmaMH
    // -------------------------------------------------
    stream >> mSigmaMH;
    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_stream_v338] erreur lecture mSigmaMH"
                 << stream.status() << stream.device()->errorString();
        throw std::runtime_error("[MHVariable::load_stream_v338] Error reading mSigmaMH");
    }
    // -------------------------------------------------
    // 6️⃣  Lecture de mSamplerProposal
    // -------------------------------------------------
    stream >> mSamplerProposal;
    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_stream_v338] erreur lecture mSamplerProposal"
                 << stream.status() << stream.device()->errorString();
        throw std::runtime_error("[MHVariable::load_stream_v338] Error reading mSamplerProposal");
    }

}


QDataStream &operator>>(QDataStream& stream, MHVariable& data )
{
    /* herited from MetropolisVariable*/
    MetropolisVariable metro_data;
    stream >> metro_data;
    
    const MHVariable tmp_data (metro_data);
    data = tmp_data;

    load_container_nullable(stream, data.mHistoryAcceptRateMH);

    load_container(stream, data.mLastMHAccepts);

    stream >> data.mSigmaMH;
    stream >> data.mSamplerProposal;

    return stream;

}

