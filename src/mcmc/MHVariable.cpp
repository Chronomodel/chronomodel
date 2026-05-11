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
#include "Generator.h"

#include <QDebug>

#define NoneStr QObject::tr("No Proposal")
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
    mLastMHAccepts(),
    mLastMHAcceptsLength(0),
    mMHAcceptcountSinceAquire(),
    mGlobalAcceptationPerCent(0.0),
    mHistoryAcceptRateMH(std::make_shared<std::vector<double>>()),
    mSamplerProposal(eDoubleExp)
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
    mSamplerProposal(eDoubleExp)
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
 * @brief MHVariable::tryUpdate
 * @param x : Value proposed and, if applicable, accepted
 * @param rate : Force reject with rate < 0 or accept with rate = 2.
 * @ref https://fr.wikipedia.org/wiki/Algorithme_de_Metropolis-Hastings
 * @return
 */
bool MHVariable::try_update(const double x, const double rate)
{
    bool accepted = MHAcceptanceTest(rate);

    // --------------------------------------------------------------
    //  1️⃣  Mise à jour de la valeur si accepted
    // --------------------------------------------------------------
    if (accepted) {
        mX = x;
    }
    // --------------------------------------------------------------
    //  2️⃣  Historique
    // --------------------------------------------------------------
    if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
        // La fenêtre est pleine → on enlève l'élément le plus ancien
        mLastMHAccepts.pop_front();
    }

    mLastMHAccepts.push_back(accepted);

    return accepted;

}
/** -----------------------------------------------------------------
*  Implémentation « log‑rate »
*  Identique à test_update_log, mais ici on ne donne pas la valeur courante
* -----------------------------------------------------------------
*/
bool MHVariable::try_update_log(const double x, const double log_rate)
{
    // ------------------------------------------------------------------
    // 1️⃣  Gestion de l’historique pour le taux d'acceptation
    // ------------------------------------------------------------------
    if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
        // La fenêtre est pleine → on enlève l'élément le plus ancien
        mLastMHAccepts.pop_front();
    }
    bool accepted = false;
    // ------------------------------------------------------------------
    // 2️⃣  Cas pathologiques (NaN, -inf, etc.)
    // ------------------------------------------------------------------
    if (std::isnan(log_rate) || log_rate == -INFINITY) {
        // log_rate = -inf ↔ rate = 0  → rejet systématique
        accepted = false;
#ifdef DEBUG
       /* if (std::isnan(log_rate))
            std::cerr << "[MHVariable::try_update_log] log_rate = NaN -> reject : " << mName << '\n';
        else {
            std::cerr << "[MHVariable::try_update_log] log_rate = -inf -> reject : " << mName << '\n';
        }*/
#endif
    }
    // ------------------------------------------------------------------
    // 3️⃣  Acceptation forcée (rate ≥ 1 ↔ log_rate ≥ 0)
    // ------------------------------------------------------------------
    else if (log_rate >= 0.0) {
        // cela couvre le cas spécial rate==2 (log(2) ≈ 0.693) ainsi que tout
        // r > 1.  Le comportement « force accept » est donc conservé.
        accepted = true;
    }
    // ------------------------------------------------------------------
    // 4️⃣  Acceptation probabiliste (0 < rate < 1 ↔ log_rate < 0)
    // ------------------------------------------------------------------
    else {
        // u ~ Uniform(0,1)  →  log(u) ∈ (‑∞,0)
        const double u = Generator::randomUniform();          // 0 < u < 1
        const double log_u = std::log(u);       // toujours négatif
        accepted = (log_u < log_rate);          // équivalent à u < exp(log_rate)
#ifdef DEBUG
        if (u == 0.0)
            std::cerr << "[MHVariable::try_update_log] uniform == 0\n";
#endif
    }
    // ------------------------------------------------------------------
    // 5️⃣  Mise à jour de l’état et de l’historique
    // ------------------------------------------------------------------
    if (accepted) mX = x;
    mLastMHAccepts.push_back(accepted);
    return accepted;
}
/**
 * @brief MHVariable::test_update determines whether to accept a new value for mX based on a given acceptance rate.
 *
 * This function implements a Metropolis-Hastings acceptance criterion. It compares the rate
 * (typically, the ratio of the target distribution at try_value to current_value) against
 * a uniformly distributed random number to decide whether to accept the new value.
 *
 * - If the rate is 1.0 or higher, the new value (try_value) is unconditionally accepted.
 * - If the rate is within [0.0, 1.0), a random number is generated and compared to the rate to decide acceptance.
 * - If the rate is less than 0.0, the new value is rejected outright.
 * - The function maintains a history of the last few acceptance/rejection outcomes in mLastMHAccepts.
 *
 * @param current_value The current value of the variable.
 * @param try_value The proposed new value to be tested.
 * @param rate The acceptance rate, typically the ratio pi(try_value)/pi(current_value).
 * @return bool True if the new value is accepted, false otherwise.
 */
bool MHVariable::test_update(const double current_value, const double try_value, const double rate)
{

    bool accepted = MHAcceptanceTest(rate);
    // --------------------------------------------------------------
    //  1️⃣  Mise à jour de la valeur
    // --------------------------------------------------------------
    mX = accepted ? try_value : current_value;

    // --------------------------------------------------------------
    //  2️⃣  Historique
    // --------------------------------------------------------------
    if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
        // La fenêtre est pleine → on enlève l'élément le plus ancien
        mLastMHAccepts.pop_front();
    }

    mLastMHAccepts.push_back(accepted);

    return accepted;
}

/*======================================================================
 *  Implémentation principale – travaille en log‑espace
 *  Identique à try_update_log, mais ici on donne la valeur courante
 *====================================================================*/

bool MHVariable::test_update_log(double current_value,
                                 double try_value,
                                 double log_rate)
{
    bool accepted = MHAcceptanceTest_log(log_rate);
    // --------------------------------------------------------------
    //  1️⃣  Mise à jour de la valeur
    // --------------------------------------------------------------
    mX = accepted ? try_value : current_value;

    // --------------------------------------------------------------
    //  2️⃣  Historique
    // --------------------------------------------------------------
    if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
        // La fenêtre est pleine → on enlève l'élément le plus ancien
        mLastMHAccepts.pop_front();
    }

    mLastMHAccepts.push_back(accepted);

    return accepted;
}

/**
 * @brief MHVariable::accept_update force setting mX with the value of x.
 * And append a true value to mLastAccept
 * @param x
 */
void MHVariable::accept_update(const double x)
{
    // --------------------------------------------------------------
    //  1️⃣  Mise à jour de la valeur
    // --------------------------------------------------------------
    mX = x;

    // --------------------------------------------------------------
    //  2️⃣  Historique
    // --------------------------------------------------------------
    if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
        // La fenêtre est pleine → on enlève l'élément le plus ancien
        mLastMHAccepts.pop_front();
    }

    mLastMHAccepts.push_back(true);

}

/**
 * @brief MHVariable::reject_update no update of mX, but append a false value to mLastAccept
 */
void MHVariable::reject_update()
{
    // --------------------------------------------------------------
    //  1️⃣ Historique
    // --------------------------------------------------------------
    if (mLastMHAccepts.size() == mLastMHAcceptsLength) {
        // La fenêtre est pleine → on enlève l'élément le plus ancien
        mLastMHAccepts.pop_front();
    }

    mLastMHAccepts.push_back(false);

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
        if (mSigmaMH < sigma_min) mSigmaMH = sigma_min;
        if (mSigmaMH > sigma_max) mSigmaMH = sigma_max;

        stillAdapted = false; // on a effectivement adapté
    }
    return stillAdapted;
}

void MHVariable::clear()
{
    MetropolisVariable::clear();
    if (mHistoryAcceptRateMH) {
        mHistoryAcceptRateMH->clear();
    }
    //mMHAcceptcountSinceAquire.clear();

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

double MHVariable::getCurrentAcceptRate() const
{
    if (mLastMHAccepts.empty())
        return 0.0;

    std::size_t trueCount = std::count(mLastMHAccepts.begin(),
                                       mLastMHAccepts.end(),
                                       true);
    return static_cast<double>(trueCount) / mLastMHAccepts.size();


}

void MHVariable::saveCurrentAcceptRate()
{
    mHistoryAcceptRateMH->push_back(100. * getCurrentAcceptRate());
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

// Peu être fait une fois à la sortie des iterations
void MHVariable::generateTraceNumericalResults(const std::vector<ChainSpecs>& chains)
{
    MetropolisVariable::generateTraceNumericalResults(chains);
    generateGlobalRunAcceptation(chains);
}


QString MHVariable::resultsString(const QString &noResultMessage, const QString &unit) const
{
    if (mSamplerProposal != MHVariable::eFixe) {
        const QString result = MetropolisVariable::resultsString(noResultMessage, unit);
        const QString globalTxt = stringForLocal(mGlobalAcceptationPerCent);

        return result + "<br>" + QObject::tr("Acceptance rate (all acquire iterations) : %1 % (%2)").arg(globalTxt, getSamplerProposalText(mSamplerProposal));

    } else {
        return QObject::tr("Fixed value : %1 %2").arg(stringForLocal(mFormatedBurnAdaptTrace->at(0)), unit); // for VG mX is Variance and we need Std gi
    }

}


QString MHVariable::getSamplerProposalText(const MHVariable::SamplerProposal sp)
{
    switch (sp) {
    case MHVariable::eNone:
        return NoneStr;
        break;
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

    else if (text == NoneStr)
        return MHVariable::eNone;
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
    /* herited from MetropolisVariable*/
    MetropolisVariable::load_stream_v338(stream);

    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_stream_v330]  erreur de flux ; stream.status()=" << stream.status();
        // throw std::runtime_error("Error reading from stream");
        // return;
    }

    qint64 l;
    stream >> l;
    mLastMHAcceptsLength = l;

    load_container_nullable(stream, mHistoryAcceptRateMH);

    load_container(stream, mLastMHAccepts);

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

    load_container_nullable(stream, data.mHistoryAcceptRateMH);

    load_container(stream, data.mLastMHAccepts);

    stream >> data.mSigmaMH;
    stream >> data.mSamplerProposal;

    return stream;

}

