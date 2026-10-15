/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#ifndef MCMCLOOP_H
#define MCMCLOOP_H

#include "CurveSettings.h"
#include "MCMCSettings.h"
#include "ModelCurve.h"

#include <QThread>
#include <QElapsedTimer>
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


/**
 * @brief AnnealAwareEstimator
 * Estimation du temps restant du MCMC.
 *  - Le coût d'une itération est mesuré en temps réel écoulé entre deux tick(),
 *    donc TOUT est compté : update(), record*, acquire(), adapt(), emit, processEvents...
 *  - Moyenne cumulée par phase (Burn-in, Adaptation, Acquisition) au lieu d'une EMA
 *    très bruitée ; une phase pas encore observée prend le coût de la plus proche connue.
 *  - Les itérations avec régénération (recuit) sont mesurées à part : coût d'un sous-pas.
 *  - Le temps d'initialisation des chaînes déjà faites sert à estimer celui des suivantes.
 */
class AnnealAwareEstimator
{
public:
    enum Phase { eBurn = 0, eAdapt = 1, eAcq = 2 };

    void setSubSteps(int n) { mSubSteps = n; }

    /** À appeler juste avant la boucle d'une phase : le temps hors boucle n'est pas compté. */
    void beginPhase()
    {
        if (!mWall.isValid())
            mWall.start();
        mLast = mWall.nsecsElapsed();
    }

    /** À appeler à la FIN de chaque itération (après emit / processEvents). */
    void tick(Phase p, bool wasRegen)
    {
        if (!mWall.isValid()) {
            beginPhase();
            return;
        }
        const qint64 now = mWall.nsecsElapsed();
        const qint64 dt  = now - mLast;
        mLast = now;

        if (!wasRegen || mSubSteps <= 0) {
            mSum[p] += dt;
            ++mCount[p];

        } else {
            const double base   = std::max(0.0, cost(p));
            const double perSub = std::max(0.0, double(dt) - base) / mSubSteps;
            mSumSub += perSub;
            ++mCountSub;
        }
    }

    /** Durée mesurée d'une initialisation de chaîne (ns). */
    void addInit(qint64 ns) { mSumInit += ns; ++mCountInit; }

    /** Coût moyen d'une itération normale de la phase p (ns), -1 si rien n'est encore mesuré. */
    double cost(Phase p) const
    {
        if (mCount[p] > 0)
            return double(mSum[p]) / double(mCount[p]);

        // phase pas encore observée : coût de la phase connue la plus proche
        static constexpr Phase order[3][3] = { {eBurn, eAdapt, eAcq},
                                               {eAdapt, eBurn, eAcq},
                                               {eAcq, eAdapt, eBurn} };
        for (Phase q : order[p])
            if (mCount[q] > 0)
                return double(mSum[q]) / double(mCount[q]);
        return -1.0;
    }

    /** Coût d'un sous-pas de recuit ; même fonction qu'une itération normale avant la 1re mesure. */
    double subStepCost(Phase p) const
    {
        return (mCountSub > 0) ? mSumSub / double(mCountSub) : cost(p);
    }

    qint64 initCost() const { return (mCountInit > 0) ? mSumInit / mCountInit : 0; }

    bool isReady() const { return mCount[eBurn] + mCount[eAdapt] + mCount[eAcq] > 0; }

    /**
     * @param t  index d'itération (mTotalIter) au début des N itérations restantes
     * @param N  nombre d'itérations restantes dans la phase
     * @param R  période de régénération (recuit), 0 si pas de recuit
     */
    qint64 estimateRemainingNs(Phase p, qint64 t, qint64 N, qint64 R, bool regenApplies) const
    {
        const double c = cost(p);
        if (c < 0.0 || N <= 0)
            return 0;

        const qint64 nRegen  = regenApplies ? countUpcomingRegen(std::max<qint64>(t, 1), N, R) : 0;
        const qint64 nNormal = N - nRegen;
        const double regenCost = c + mSubSteps * subStepCost(p);

        return qint64(nNormal * c + nRegen * regenCost);
    }

private:
    static qint64 countUpcomingRegen(qint64 t, qint64 N, qint64 R)
    {
        if (R <= 0 || N <= 0) return 0;
        return (t + N - 1) / R - (t - 1) / R;
    }

    QElapsedTimer mWall;
    qint64 mLast = 0;

    int    mSubSteps = 0;

    qint64 mSum[3]   = {0, 0, 0};
    qint64 mCount[3] = {0, 0, 0};

    double mSumSub   = 0.0;
    qint64 mCountSub = 0;

    qint64 mSumInit   = 0;
    qint64 mCountInit = 0;
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
