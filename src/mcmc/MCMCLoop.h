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
