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

#ifndef MCMCLOOP_H
#define MCMCLOOP_H

#include "CurveSettings.h"
#include "MCMCSettings.h"
#include "ModelCurve.h"

#include <QThread>
#include <memory>
#include <vector>

#define ABORTED_BY_USER "Aborted by user"

#if PARALLEL
#include <execution>
#define PAR std::execution::par,
#else
#define PAR
#endif

/**
 * @class MCMCLoop
 * @brief Classe principale pour l'exécution des simulations MCMC
 *
 * Cette classe gère l'ensemble du processus MCMC incluant :
 * - Calibration des données
 * - Initialisation SMC pour la sélection des graines
 * - Exécution des chaînes MCMC (initialisation, burn-in, adaptation, acquisition)
 * - Calcul des scores SMC pour l'évaluation des configurations
 */
class MCMCLoop : public QThread
{
    Q_OBJECT
public:

    QString mAbortedReason;
    std::shared_ptr<ModelCurve> mModel;

    enum class State : int
    {
        eCalibrating = 0,   // avant toute chose
        eSMC         = 1,   // simulation SMC
        eInit        = 2,   // initialisation du MCMC
        eBurning     = 3,   // burn‑in
        eAdapting    = 4,   // adaptation du pas d’acceptation
        eAcquisition = 5,   // collecte des échantillons
        eFinalize    = 6    // calcul des stats
    };


    explicit MCMCLoop (std::shared_ptr<ModelCurve> model);
    virtual ~MCMCLoop();

    void setMCMCSettings(const MCMCSettings& settings);
    const std::vector<ChainSpecs> &chains() const;
    void run();

signals:
    void stepChanged(QString title, int min, int max);
    void stepProgressed(int value);
    void setMessage(QString message);

protected:

    // Variable for update function
    double tminPeriod;
    double tmaxPeriod;
    CurveSettings mCurveSettings;

    virtual QString calibrate() = 0;
    virtual QString initialize() = 0;

    QString initialize_time();
    double SMC_score();
    double log_SMC_score(double Tmin, double Tmax, double Xmin, double Xmax);
    virtual bool update() = 0;
    virtual bool learn() {return update();};

    //virtual void memo() = 0; // obsolete décomposer en recordBurnAdapt et acquire, pour la partie aquisition
    virtual void recordBurnAdapt() = 0;
    virtual void acquire() = 0;

    virtual void recordMH() = 0;

    //virtual void memo_accept(const unsigned int i_chain) = 0;
    virtual void finalize() = 0;
    virtual bool adapt(const int batchIndex) = 0;

    std::vector<ChainSpecs> mLoopChains;
    size_t mChainIndex;
    State mState;

};


class AnnealAwareEstimator
{
public:
    void setSubSteps(int n) { mSubSteps = n; }

    void addSample(qint64 dtNs, bool wasRegen, bool annealingEnabled)
    {
        constexpr double alpha = 0.1;

        if (!wasRegen) {
            mEmaNormal = (mEmaNormal < 0.0) ? (double)dtNs
                                            : alpha * dtNs + (1.0 - alpha) * mEmaNormal;

            if (annealingEnabled && !mSubStepSeededFromReal)   // 👈 garde-fou
                mEmaSubStep = mEmaNormal;
        }
        else if (mSubSteps > 0) {
            const double base = (mEmaNormal >= 0.0) ? mEmaNormal : 0.0;
            const double perSub = std::max(0.0, (dtNs - base)) / mSubSteps;

            if (!mSubStepSeededFromReal) {
                // première vraie mesure : on remplace l'estimation a priori
                mEmaSubStep = perSub;
                mSubStepSeededFromReal = true;
            } else {
                mEmaSubStep = alpha * perSub + (1.0 - alpha) * mEmaSubStep;
            }
        }
    }

    qint64 estimateRemainingNs(qint64 t, qint64 N, qint64 R, bool regenApplies) const
    {
        if (mEmaNormal < 0.0) return 0;

        const qint64 nRegen  = regenApplies ? countUpcomingRegen(std::max<qint64>(t, 1), N, R) : 0;
        const qint64 nNormal = N - nRegen;

        const double subCost   = (mEmaSubStep >= 0.0) ? mEmaSubStep : 0.0;
        const double regenCost = mEmaNormal + mSubSteps * subCost;

        return (qint64)(nNormal * mEmaNormal + nRegen * regenCost);
    }

private:
    static qint64 countUpcomingRegen(qint64 t, qint64 N, qint64 R)
    {
        if (R <= 0 || N <= 0) return 0;
        return (t + N - 1) / R - (t - 1) / R;
    }

    int    mSubSteps  = 0;
    double mEmaNormal = -1.0;
    double mEmaSubStep = -1.0;
    bool   mSubStepSeededFromReal = false;   // 👈 bascule dès la 1ʳᵉ vraie mesure
};

inline int computeAnnealSubSteps(int max_expo_T, int dwell_steps_T0)
{
    int total = 0;
    for (int e = max_expo_T; e >= 0; --e) {
        const int dwell = std::max(1,
                                   (int)std::ceil(dwell_steps_T0 / (1.0 + e)));
        total += dwell;
    }
    return total;
}

inline qint64 countUpcomingRegen(qint64 t, qint64 N, qint64 R)
{
    // multiples de R dans [t, t+N-1]
    if (R <= 0) return 0;
    return (t + N - 1) / R - (t - 1) / R;
}


qint64 estimateGlobalRemainingNs(const std::vector<ChainSpecs>& chains,   // ou le type réel de mLoopChains
                                 int currentChainIndex,
                                 MCMCLoop::State state,
                                 const AnnealAwareEstimator& est,
                                 qint64 R);

#endif
