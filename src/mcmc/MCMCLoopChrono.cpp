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

#include "MCMCLoopChrono.h"

#include "Project.h"
#include "Date.h"
#include "QtUtilities.h"
#include "CalibrationCurve.h"
#include "Generator.h"
#include "AppSettings.h"

#include <QElapsedTimer>
#include <cmath>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QTime>

#define NOTEST //TEST

class Project;

MCMCLoopChrono::MCMCLoopChrono(std::shared_ptr<ModelCurve> model):
    MCMCLoop(model)
{
    if (mModel)
        setMCMCSettings(mModel->mMCMCSettings);

    mCurveSettings.mTimeType = CurveSettings::eModeBayesian;
}

MCMCLoopChrono::~MCMCLoopChrono()
{
}


/**
 * @brief Calibre toutes les dates du modèle associé.
 *
 * Cette méthode parcourt l’ensemble des événements du modèle, collecte
 * les pointeurs vers leurs dates, puis effectue la calibration de chaque
 * date.  Elle émet les signaux Qt `stepChanged` et `stepProgressed` afin
 * d’informer l’interface de l’avancement du processus.
 *
 * @return
 *   - Une chaîne vide (`QString()`) si la calibration s’est déroulée avec succès.
 *   - Un message d’erreur explicite si une condition d’échec est rencontrée
 *     (modèle invalide, calibration manquante, résolution insuffisante, etc.).
 *   - La constante `ABORTED_BY_USER` si l’opération a été interrompue par l’utilisateur.
 *
 * @note
 *   - La fonction est **thread‑safe** tant que le modèle n’est pas modifié
 *     simultanément depuis un autre thread.
 *   - La résolution minimale requise pour une calibration est de 5 points.
 *
 * @throws Aucun.  Tous les cas d’erreur sont signalés via la valeur de retour.
 */
QString MCMCLoopChrono::calibrate()
{
    // -------------------------------------------------------------------------
    // 1️⃣ Vérification du modèle
    // -------------------------------------------------------------------------
    if (!mModel) {
        return tr("Invalid model");
    }

    // -------------------------------------------------------------------------
    // 2️⃣ Récupération des événements (référence const pour éviter copies)
    // -------------------------------------------------------------------------
    const std::vector<std::shared_ptr<Event>>& events = mModel->mEvents;

    // -------------------------------------------------------------------------
    // 3️⃣ Comptage du nombre total de dates afin de réserver la capacité exacte
    // -------------------------------------------------------------------------
    size_t totalDates = 0;
    for (const auto& ev : events) {
        totalDates += ev->mDates.size();
    }

    // -------------------------------------------------------------------------
    // 4️⃣ Collecte des pointeurs vers les dates
    // -------------------------------------------------------------------------
    std::vector<Date*> dates;
    dates.reserve(totalDates);

    for (const auto& ev : events) {
        for (Date& d : ev->mDates) {
            dates.push_back(&d);
        }
    }

    // -------------------------------------------------------------------------
    // 5️⃣ Interruption éventuelle avant le lancement de la calibration
    // -------------------------------------------------------------------------
    if (isInterruptionRequested()) {
        return ABORTED_BY_USER;
    }

    // -------------------------------------------------------------------------
    // 6️⃣ Signal d’initialisation de la progression
    // -------------------------------------------------------------------------
    emit stepChanged(tr("Calibrating..."), 0, static_cast<int>(dates.size()));

    // -------------------------------------------------------------------------
    // 7️⃣ Boucle principale de calibration
    // -------------------------------------------------------------------------
    QString errorMsg;          // contiendra le message d’erreur éventuel
    int progressIdx = 0;       // indice de progression envoyé au signal

    for (Date* datePtr : dates) {
        // Vérification de l’interruption à chaque itération
        if (isInterruptionRequested()) {
            errorMsg = ABORTED_BY_USER;
            break;
        }

        // -----------------------------------------------------------------
        // 7.1️⃣ Vérification de la présence d’une calibration
        // -----------------------------------------------------------------
        if (!datePtr->mCalibration) {
            errorMsg = tr("Invalid Model -> No Calibration on Data %1")
            .arg(datePtr->getQStringName());
            break;
        }

        // -----------------------------------------------------------------
        // 7.2️⃣ Calibration si le vecteur est vide
        // -----------------------------------------------------------------
        if (datePtr->mCalibration->mVector.empty()) {
            datePtr->calibrate(getProject_ptr());
        }

        // -----------------------------------------------------------------
        // 7.3️⃣ Vérification de la résolution (au moins 5 points)
        // -----------------------------------------------------------------
        if (datePtr->mCalibration->mVector.size() < 5) {
            const double newStep = datePtr->mCalibration->mStep / 5.0;

            // Nettoyage complet de la calibration courante
            datePtr->mCalibration->mVector.clear();
            datePtr->mCalibration->mMap.clear();
            datePtr->mCalibration->mRepartition.clear();
            datePtr->mCalibration = nullptr;

            errorMsg = tr("Insufficient resolution for the Event %1\r"
                          "Decrease the step in the study period box to %2")
                           .arg(datePtr->getQStringName(),
                                QString::number(newStep));
            break;
        }

        // -----------------------------------------------------------------
        // 7.4️⃣ Mise à jour de la progression
        // -----------------------------------------------------------------
        emit stepProgressed(progressIdx);
        ++progressIdx;
    }

    // -------------------------------------------------------------------------
    // 8️⃣ Nettoyage et retour
    // -------------------------------------------------------------------------
    dates.clear();   // libération de la mémoire temporaire

    // Si aucune erreur n’a été détectée, on renvoie une chaîne vide.
    return errorMsg;
}


QString MCMCLoopChrono::initialize()
{
    return initialize_time();
}

// Gibbs complet comme V2
bool MCMCLoopChrono::update_v3()
{
    sampler_Gibbs(mModel->mEvents);

    std::for_each(mModel->mPhases.begin(),
                  mModel->mPhases.end(),
                  [this](std::shared_ptr<Phase> p) {
                      p->update_Tau(tminPeriod, tmaxPeriod);
                  });

    std::for_each(mModel->mPhaseConstraints.begin(),
                  mModel->mPhaseConstraints.end(),
                  [](std::shared_ptr<PhaseConstraint> pc) {
                      pc->updateGamma();
                  });

    return true;
}

void MCMCLoopChrono::sampler_Gibbs(std::vector<std::shared_ptr<Event>> &events)
{

    /* --------------------------------------------------------------
     *  A - Update ti Dates
     *  B - Update Theta Events
     *  C.1 - Update Alpha, Beta & Duration Phases
     *  C.2 - Update Tau Phase
     *  C.3 - Update Gamma Phases
     * ---------------------------------------------------------------------- */

    // --------------------------------------------------------------
    //  A - Update ti Dates
    //
    //  B - Update theta Events
    // --------------------------------------------------------------

    for (std::shared_ptr<Event> &event : mModel->mEvents) {
        event->mTheta.mSamplerProposal = SamplerProposal::eEventPrior; // test ici
            //event->updateTheta_v3(tminPeriod, tmaxPeriod);
            //
        const double theta = event->mTheta.value();
        const double S02Theta = event->mS02Theta.value();
        for (auto&& date : event->mDates )   {
            //date.updateDate_v3(event->mTheta.value(), event->mS02Theta.value());

            date.Inversion(theta);

            date.updateDelta_v3(theta, S02Theta);
            // date.updateSigmaShrinkage_K(theta, S02Theta);
            date.mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
            date.updateSigma_Log10(theta, S02Theta);

            date.updateWiggle();
        }

        const double min = event->getThetaMin(tminPeriod);
        const double max = event->getThetaMax(tmaxPeriod);

        if (min > max)
            throw QObject::tr("Error for event : %1 : min = %2 : max = %3").arg(event->getQStringName(), QString::number(min), QString::number(max));

        // -------------------------------------------------------------------------------------------------
        //  Evaluer theta.
        //  Le cas Wiggle est inclus ici car on utilise une formule générale.
        //  On est en "wiggle" si au moins une des mesures a un delta > 0.
        // -------------------------------------------------------------------------------------------------

        double sum_p = 0.0;
        double sum_t = 0.0;

        for (auto&& date: event->mDates) {
            const double variance  = pow(date.mSigmaTi.value(), 2);
            sum_t += (date.mTi.value() + date.mDelta) / variance;
            sum_p += 1.0 / variance;
        }
        const double ti_avg = sum_t / sum_p;
        const double sigma = 1.0 / sqrt(sum_p);

        if (min == max) {
            double try_theta = min;
            event->mTheta.accept_update(try_theta);

        } else {
            switch(event->mTheta.mSamplerProposal)
            {
            case SamplerProposal::eDoubleExp:
            {
                try {
                    double try_theta = Generator::gaussByDoubleExp(ti_avg, sigma, min, max);
                    //  double try_theta = Generator::truncatedNormal(ti_avg, sigma, min, max);
                    event->mTheta.accept_update(try_theta);

                }
                catch(QString error) {
                    throw QObject::tr("Error for event : %1 : %2").arg(event->getQStringName(), error);
                }
                break;
            }

                // Event Prior
            case SamplerProposal::eEventPrior:
            {
                double try_theta = Generator::truncatedNormal(ti_avg, sigma, min, max);
                event->mTheta.accept_update(try_theta);
                break;
            }

            case SamplerProposal::eRWAdaptGauss:
            {
                // MH: The only case where the acceptance rate makes sense, since we use sigma MH :
                double try_theta = Generator::normalDistribution(event->mTheta.value(), event->mTheta.mSigmaMH);

                if (try_theta < min || try_theta > max) {
                    event->mTheta.reject_update();
                    break;
                }
                double diff1 = try_theta - ti_avg;
                double diff2 = event->mTheta.value() - ti_avg;
                double rate = -0.5 * (diff1*diff1 - diff2*diff2) / (sigma*sigma);
                event->mTheta.try_update_log(try_theta, rate);
                break;
            }

            default:
                break;
            }
        }

        //

        if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
            event->updateS02Theta_v338();

        //--------------------- Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
        /* --------------------------------------------------------------
         * C.1 - Update Alpha, Beta & Duration Phases
         * -------------------------------------------------------------- */
        //  Update Phases -set mAlpha and mBeta ; they coud be used by the Event in the other Phase ----------------------------------------
        std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});

    }

    //  Update Phases Tau; they coud be used by the Event in the other Phase ----------------------------------------
    /* --------------------------------------------------------------
     *  C.2 - Update Tau Phases
     * -------------------------------------------------------------- */
    std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_Tau (tminPeriod, tmaxPeriod);});


    /* --------------------------------------------------------------
     *  C.3 - Update Phases constraints
     * -------------------------------------------------------------- */
    std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (std::shared_ptr<PhaseConstraint> pc) {pc->updateGamma();});


}

bool MCMCLoopChrono::update_v3_block_simulated_annealing()
{
    const int iteration =  mLoopChains[ mChainIndex].mTotalIter;

    const int max_expo_T   = mModel->mMCMCSettings.mAnnealTemp;

    bool do_regeneration = (mModel->mMCMCSettings.mAnnealRecurrence > 0
                            && iteration > 0
                            && iteration % mModel->mMCMCSettings.mAnnealRecurrence == 0
                            && mState == State::eAcquisition);

    // ------------------------------------------------------------------
    // 1️⃣  Décision de régénération
    // ------------------------------------------------------------------
    if (do_regeneration) {

        // -------------------------------------------------
        // 1️⃣ Déclaration du vecteur vide (ou avec capacité)
        // -------------------------------------------------
        std::vector<std::shared_ptr<Event>> event_regenerated{};
        event_regenerated.reserve(mModel->mEvents.size());
        // -------------------------------------------------
        // 2️⃣ Remplissage conditionnel
        // -------------------------------------------------
        for (std::size_t j = 0; j < mModel->mEvents.size(); ++j) {
            if (mModel->mEvents[j]->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
                event_regenerated.push_back(mModel->mEvents[j]);   // copie du shared_ptr
            }
        }


        const int dwell_steps_T0 = mModel->mMCMCSettings.mAnnealDwell;

        for (int e = max_expo_T; e >= 0; --e) {
            const double T = std::pow(2, e);

            const int dwell_steps = std::max(1,
                                             static_cast<int>(std::ceil(dwell_steps_T0 / (1.0 + static_cast<double>(e))))
                                             );

            // --------------------------------------------------------------
            // 5️⃣  Relaxation
            // --------------------------------------------------------------

            for (int s = 0; s < dwell_steps; ++s)
                //tempering_339(event_regenerated, T);
                tempering_339_ti_marg(event_regenerated, T);

        }


    }

    // ------------------------------------------------------------------
    // 6️⃣  Mise à jour standard de tous les events
    // ------------------------------------------------------------------
#pragma mark CHOIX DU SAMPLER
    // sampler_339_4v(mModel->mEvents); //fonctionne

    // sampler_339_4vXi(mModel->mEvents);
    // sampler_339_SingleSite(mModel->mEvents); // fonctionne

    //sampler_339_SingleSite_bloc(mModel->mEvents); // defaut // fonctionne
    sampler_339_SingleSite_bloc_delta(mModel->mEvents); // avec shift bloc // Fonctionne bien

    // sampler_339_SingleSite_bloc_2(mModel->mEvents); // fonctionne avec A.1, B.2 peu efficace avec une seule date

    // sampler_339_SingleSite_SliceSampling(mModel->mEvents); //fonctionne , et trop lent en calcul


  //  sampler_339_SingleSite_NonCentered(mModel->mEvents); // en test
    //sampler_339_Couple(mModel->mEvents);

    //sampler_339_3v(mModel->mEvents);
    //sampler_naif(mModel->mEvents);

    std::for_each(mModel->mPhases.begin(),
                  mModel->mPhases.end(),
                  [this](std::shared_ptr<Phase> p) {
                      p->update_Tau(tminPeriod, tmaxPeriod);
                  });

    std::for_each(mModel->mPhaseConstraints.begin(),
                  mModel->mPhaseConstraints.end(),
                  [](std::shared_ptr<PhaseConstraint> pc) {
                      pc->updateGamma();
                  });


    return true;
}

/**
 *
 * https://en.wikipedia.org/wiki/Simulated_annealing

 */

void MCMCLoopChrono::tempering_339(std::vector<std::shared_ptr<Event>> &events, double T)
{
    try{
        for (auto &event : events) {
            try {
                // Évaluation des bornes de theta
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[Event::tempering_339] Error for event : %1 : min = %2 : max = %3")
                    .arg(event->getQStringName(), QString::number(min), QString::number(max));
                }

                // Gestion du cas min == max (Contrainte stricte)
                if (min == max) {
                    event->mTheta.setValue(min);
                    event->mThetaReduced = mModel->reduceTime(min);

                    // 1. Mises à jour individuelles des dates (ti) à theta FIXE
                    for (auto&& date : event->mDates) {
                        date.applyTi(event->mTheta.value());
                    }
                }
                else {
                    // Mises à jour individuelles des dates (ti) à theta FIXE
                    for (auto&& date : event->mDates) {
                        date.applyTi(event->mTheta.value());
                    }

                    // ----------------------------------------
                    // Calcul de θ moyen local et σ
                    // ----------------------------------------
                    double sum_p = 0.0;
                    double sum_t = 0.0;
                    for (auto&& date : event->mDates) {
                        const double var = std::pow(date.mSigmaTi.value(), 2.0);
                        sum_t += (date.mTi.value() + date.mDelta) / var;
                        sum_p += 1.0 / var;
                    }

                    if (sum_p <= 0.0) continue; // Sécurité division par zéro

                    const double ti_avg = sum_t / sum_p;
                    const double sigma  = 1.0 / std::sqrt(sum_p);

                    // Le pas de proposition s'élargit à haute température T
                    const double sigma_proposal = event->mTheta.mSigmaMH * T;
                    const double old_theta = event->mTheta.value();
                    const double prop_theta = Generator::truncatedNormal(old_theta, sigma_proposal, min, max);

                    // Ratio de Hastings des constantes de normalisation pour la loi tronquée centrée sur l'état courant
                    const double num = normalCDF((max - old_theta) / sigma_proposal) - normalCDF((min - old_theta) / sigma_proposal);
                    const double den = normalCDF((max - prop_theta) / sigma_proposal) - normalCDF((min - prop_theta) / sigma_proposal);

                    // Correction : Sécurité numérique sur le domaine de log
                    if (num > 0.0 && den > 0.0) {
                        const double log_support_ratio = std::log(num) - std::log(den);

                        // 4. Variation de la cible conditionnelle sous température T
                        const double log_prior_diff = log_dnorm(prop_theta, ti_avg, sigma) - log_dnorm(old_theta, ti_avg, sigma);

                        // 5. Critère d'acceptation de Metropolis-Hastings (Seul le terme cible est divisé par T)
                        const double log_rate = (log_prior_diff / T) + log_support_ratio;

                        if (MHAcceptanceTest_log(log_rate)) {
                            event->mTheta.setValue(prop_theta);
                            event->mThetaReduced = mModel->reduceTime(prop_theta);
                            // Les dates ti ne sont pas modifiées sur l'étape theta
                        }
                    }
                }

                // 4. Mises à jour des hyperparamètres
                for (auto&& date : event->mDates) {
                    date.applyDelta(event->mTheta.value(), event->mS02Theta.value());
                    date.applySigma(event->mTheta.value(), event->mS02Theta.value());
                    date.applyWiggle();
                }

            } catch (const std::exception &e) {
                qWarning() << "[ " << __func__ << "] Tempering error on event"
                           << event->getQStringName() << ":" << e.what();
            }

            if (AppSettings::mEventModel == EventModelType::EDM2) {
                if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
                    event->applyS02Theta_v3();
            }

            std::for_each(event->mPhases.begin(), event->mPhases.end(),
                          [this](std::shared_ptr<Phase> p) {
                              p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                          });
        }

        // Mises à jour globales
        std::for_each(mModel->mPhases.begin(), mModel->mPhases.end(),
                      [this](std::shared_ptr<Phase> p) {
                          p->update_Tau(tminPeriod, tmaxPeriod);
                      });

        std::for_each(mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });

    } catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;

    } catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error" << e.what();

    } catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range" << e.what();

    } catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();

    } catch(...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";

    }
}

void MCMCLoopChrono::tempering_339_ti_marg(std::vector<std::shared_ptr<Event>> &events, double T)
{
    try{
        for (auto &event : events) {
            try {
                // Évaluation des bornes de theta
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[Event::tempering_339] Error for event : %1 : min = %2 : max = %3")
                    .arg(event->getQStringName(), QString::number(min), QString::number(max));
                }

                // Gestion du cas min == max (Contrainte stricte) : theta connu,
                // rien à marginaliser, on garde le comportement d'origine.
                if (min == max) {
                    event->mTheta.setValue(min);
                    event->mThetaReduced = mModel->reduceTime(min);

                    for (auto&& date : event->mDates) {
                        //date.applyTi(event->mTheta.value());
                        date.applyInversion(event->mTheta.value());
                    }
                }
                else {
                    // ==========================================================
                    // Mise à jour de chaque ti, THETA MARGINALISÉ.
                    // Kernel A.1 : proposition indépendante (calibration/mixing),
                    // acceptée contre la marginale tronquée de theta.
                    // sigma_i et delta_i restent FIXES ici (mis à jour plus bas).
                    // ==========================================================

                    // ----------------------------------------------------------
                    // Statistiques pondérées globales. P_curr = somme des 1/V_i
                    // ne dépend pas des ti : reste CONSTANT sur tout le balayage.
                    // ----------------------------------------------------------
                    long double P_curr  = 0.0L;
                    long double mu_curr = 0.0L;
                    long double S_curr  = 0.0L;

                    for (const auto& date : event->mDates) {
                        const long double sigma_i = static_cast<long double>(date.mSigmaTi.value());
                        const long double y = static_cast<long double>(date.mTi.value()) + static_cast<long double>(date.mDelta);
                        const long double w = 1.0L / (sigma_i * sigma_i);

                        if (P_curr == 0.0L) {
                            P_curr = w; mu_curr = y; S_curr = 0.0L;
                        } else {
                            const long double P_new = P_curr + w;
                            const long double d = y - mu_curr;
                            const long double mu_new = mu_curr + (w / P_new) * d;
                            S_curr += w * d * (y - mu_new);
                            P_curr = P_new;
                            mu_curr = mu_new;
                        }
                    }

                    if (P_curr <= 0.0L) continue; // Sécurité division par zéro

                    // sigma de la marginale de theta : constant sur tout le balayage
                    const long double sigma_avg = 1.0L / std::sqrt(P_curr);

                    for (auto&& date : event->mDates) {

                        const double ti_old0    = date.mTi.value();
                        const double delta_old0 = date.mDelta;
                        const long double sigma_i = static_cast<long double>(date.mSigmaTi.value());
                        const long double w_i      = 1.0L / (sigma_i * sigma_i);
                        const long double y_old    = static_cast<long double>(ti_old0) + delta_old0;

                        // --- Leave-One-Out : retire cette date des stats globales ---
                        const long double P_removed = P_curr - w_i;
                        long double mu_removed = 0.0L;
                        long double S_removed  = 0.0L;

                        if (P_removed > 0.0L) {
                            mu_removed = (P_curr * mu_curr - w_i * y_old) / P_removed;
                            S_removed  = S_curr - w_i * (y_old - mu_curr) * (y_old - mu_removed);
                            if (S_removed < 0.0L && S_removed > -1e-18L) S_removed = 0.0L;
                        } else {
                            mu_removed = y_old;
                        }

                        // --- Proposition indépendante de ti (kernel A.1 : calib/mixing) ---
                        double ti_prop;
                        if (Generator::randomUniform() < date.mMixingLevel) {
                            ti_prop = *date.mCalibration->sample_t();
                        } else {
                            const double tminCalib = date.mCalibration->mTmin;
                            const double tmaxCalib = date.mCalibration->mTmax;
                            const double s = std::max((date.mSettings.mTmax - date.mSettings.mTmin),
                                                      tmaxCalib - tminCalib) / 2.0;
                            ti_prop = Generator::normalDistribution(ti_old0, s);
                        }

                        const double q_fwd = date.fProposalDensity(ti_prop, ti_old0);
                        const double q_rev = date.fProposalDensity(ti_old0, ti_prop);
                        if (q_fwd <= 0.0 || q_rev <= 0.0) {
                            continue; // rejet automatique, ti inchangé
                        }
                        const double log_rate_q_t = std::log(q_rev) - std::log(q_fwd);

                        // --- Vraisemblance calibration ---
                        const double L_old = date.getLikelihood(ti_old0);
                        const double L_new = date.getLikelihood(ti_prop);
                        if (L_new <= 0.0 || L_old <= 0.0) {
                            continue;
                        }
                        const double log_rate_L = std::log(L_new) - std::log(L_old);

                        // --- Recombine avec la nouvelle valeur (P inchangé : seuls mu/S bougent) ---
                        const long double y_new = static_cast<long double>(ti_prop) + delta_old0;

                        long double mu_new, S_new;
                        if (P_removed > 0.0L) {
                            const long double d = y_new - mu_removed;
                            mu_new = mu_removed + (w_i / P_curr) * d;   // P_new == P_curr (poids inchangés)
                            S_new  = S_removed + w_i * d * (y_new - mu_new);
                        } else {
                            mu_new = y_new;
                            S_new  = 0.0L;
                        }
                        if (S_new < 0.0L && S_new > -1e-18L) S_new = 0.0L;

                        // --- Ratio MH : marginale tronquée de theta (T=1 ici, la
                        //     température n'intervient que dans l'étape theta plus bas) ---
                        const long double alpha_old_min = (static_cast<long double>(min) - mu_curr) / sigma_avg;
                        const long double alpha_old_max = (static_cast<long double>(max) - mu_curr) / sigma_avg;
                        const long double alpha_new_min = (static_cast<long double>(min) - mu_new)  / sigma_avg;
                        const long double alpha_new_max = (static_cast<long double>(max) - mu_new)  / sigma_avg;

                        const double log_Z_old = log_diff_cdf(static_cast<double>(alpha_old_min), static_cast<double>(alpha_old_max));
                        const double log_Z_new = log_diff_cdf(static_cast<double>(alpha_new_min), static_cast<double>(alpha_new_max));

                        if (!std::isfinite(log_Z_old) || !std::isfinite(log_Z_new)) {
                            continue; // dégénéré : ti inchangé
                        }

                        // sigma_avg et sigma_i inchangés => pas de jacobien, pas de
                        // ratio sigma_avg_new/sigma_avg_old (les deux valent 1 = log 0)
                        const long double log_prior_marginal_diff = static_cast<long double>(log_Z_new - log_Z_old)
                                                                   - 0.5L * (S_new - S_curr);

                        const long double log_rate_total = log_prior_marginal_diff
                                                           + static_cast<long double>(log_rate_L)
                                                           + static_cast<long double>(log_rate_q_t);

                        if (MHAcceptanceTest_log(static_cast<double>(log_rate_total))) {
                            date.mTi.setValue(ti_prop);
                            mu_curr = mu_new;
                            S_curr  = S_new;
                            // P_curr inchangé
                        }
                    }

                    // ----------------------------------------------------------
                    // theta ~ N(mu_curr, sigma_avg²) tronquée sur [min,max] :
                    // moments exacts, déjà tenus à jour pendant le balayage
                    // ci-dessus (pas besoin de refaire une boucle sum_p/sum_t).
                    // ----------------------------------------------------------
                    const double ti_avg = static_cast<double>(mu_curr);
                    const double sigma  = static_cast<double>(sigma_avg);

                    // Le pas de proposition s'élargit à haute température T
                    const double sigma_proposal = event->mTheta.mSigmaMH * T;
                    const double old_theta = event->mTheta.value();
                    const double prop_theta = Generator::truncatedNormal(old_theta, sigma_proposal, min, max);

                    const double num = normalCDF((max - old_theta) / sigma_proposal) - normalCDF((min - old_theta) / sigma_proposal);
                    const double den = normalCDF((max - prop_theta) / sigma_proposal) - normalCDF((min - prop_theta) / sigma_proposal);

                    if (num > 0.0 && den > 0.0) {
                        const double log_support_ratio = std::log(num) - std::log(den);
                        const double log_prior_diff = log_dnorm(prop_theta, ti_avg, sigma) - log_dnorm(old_theta, ti_avg, sigma);
                        const double log_rate = (log_prior_diff / T) + log_support_ratio;

                        if (MHAcceptanceTest_log(log_rate)) {
                            event->mTheta.setValue(prop_theta);
                            event->mThetaReduced = mModel->reduceTime(prop_theta);
                        }
                    }
                }

                // Mises à jour des hyperparamètres (inchangé)
                for (auto&& date : event->mDates) {
                    date.applyDelta(event->mTheta.value(), event->mS02Theta.value());
                    date.applySigma(event->mTheta.value(), event->mS02Theta.value());// via shrinkage

                    date.applyWiggle();
                }

            } catch (const std::exception &e) {
                qWarning() << "[ " << __func__ << "] Tempering error on event"
                           << event->getQStringName() << ":" << e.what();
            }

            if (AppSettings::mEventModel == EventModelType::EDM2) {
                if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
                    event->applyS02Theta_v3();
            }

            std::for_each(event->mPhases.begin(), event->mPhases.end(),
                          [this](std::shared_ptr<Phase> p) {
                              p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                          });
        }

        std::for_each(mModel->mPhases.begin(), mModel->mPhases.end(),
                      [this](std::shared_ptr<Phase> p) {
                          p->update_Tau(tminPeriod, tmaxPeriod);
                      });

        std::for_each(mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });

    } catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
    } catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error" << e.what();
    } catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range" << e.what();
    } catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
    } catch(...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
    }
}

/**
 * @brief Méthode d'échantillonnage MCMC pour le modèle 339_4v.
 *
 * Cette méthode effectue une mise à jour MCMC adaptative complète des événements,
 * incluant les propositions conjointes de @f$t_i@f$, @f$\delta@f$, et @f$\theta@f$
 * avec des ajustements de vraisemblance et de priori appropriés.
 *
 * Le processus suit les étapes suivantes :
 * 1. Mise à jour de @f$\theta@f$ pour chaque événement (si non fixé)
 * 2. Proposition et évaluation des @f$t_i@f$ avec leur vraisemblance
 * 3. Calcul des ratios de vraisemblance et de Hastings
 * 4. Génération de @f$\delta@f$ selon le type de priori
 * 5. Échantillonnage de @f$\sigma@f$ via une distribution de shrinkage
 * 6. Calcul des statistiques de groupe (moyennes, variances)
 * 7. Évaluation des corrections de troncature pour les prioris
 * 8. Décision Metropolis-Hastings basée sur le ratio global
 * 9. Mise à jour des hyperparamètres (wiggles, EDM2)
 * 10. Mise à jour des phases et contraintes du modèle
 *
 * @param[in,out] events Référence vers le vecteur d'événements à mettre à jour.
 *                       Les paramètres de chaque événement sont modifiés selon les
 *                       résultats de l'échantillonnage MCMC.
 *
 * @note Cette méthode est conçue pour fonctionner dans un contexte multi-thread.
 *       Elle utilise des mécanismes de verrouillage internes pour garantir la
 *       sécurité des accès concurrents aux ressources partagées.
 *
 * @warning En cas d'erreur critique (probabilités nulles, allocations échouées),
 *          la méthode abandonne le traitement de l'événement courant et continue
 *          avec les suivants, sans interrompre l'exécution globale.
 *
 * @throws std::invalid_argument Si les bornes min/max sont invalides.
 * @throws std::runtime_error Si la création de plans FFTW ou d'autres ressources échoue.
 * @throws std::bad_alloc Si l'allocation de mémoire échoue.
 *
 * @see Event::getThetaMin(), Event::getThetaMax(), Date::getLikelihood(),
 *      Date::fProposalDensity(), Generator::truncatedNormal(),
 *      Generator::shrinkageUniforme(), MHAcceptanceTest_log()
 *
 * @since 3.39.4
 * @version 1.0
 */
void MCMCLoopChrono::sampler_339_4v(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        // ======================================================================
        // 1. Mise à jour de tous les événements
        // ======================================================================
        for (auto &event : events) {
            // ======================================================================
            // 2. Mise à jour de theta uniquement si non fixé
            // ======================================================================
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
                // ======================================================================
                // 3. Évaluation des bornes de theta
                // ======================================================================
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                    .arg(QString::fromLatin1(__func__),
                         event->getQStringName(),
                         QString::number(min),
                         QString::number(max));
                }

#pragma mark Proposal ti
                // ======================================================================
                // 4. Proposition des ti avec leurs vraisemblances L
                // ======================================================================

                std::vector<double> old_ti;
                old_ti.reserve(event->mDates.size());
                std::vector<double> prop_ti;
                prop_ti.reserve(event->mDates.size());

                std::vector<double> old_delta;
                old_delta.reserve(event->mDates.size());
                std::vector<double> prop_delta;
                prop_delta.reserve(event->mDates.size());

                std::vector<double> old_sigma;
                old_sigma.reserve(event->mDates.size());
                std::vector<double> prop_sigma;
                prop_sigma.reserve(event->mDates.size());

                double log_rate_L = 0.0; // Rapport de vraisemblance (divisé par T si Annealing)
                double log_rate_q = 0.0; // Ratio de Hastings des proposals de ti

                bool invalid = false;
                for (auto&& date : event->mDates) {
                    const double ti_old = date.mTi.value();
                    old_ti.push_back(ti_old);

                    const double sigma_old = date.mSigmaTi.value();
                    old_sigma.push_back(sigma_old);

                    double ti_prop;

                    if (Generator::randomUniform() < date.mMixingLevel) {

                        ti_prop = *date.mCalibration->sample_t();
                        //qDebug() << "Calibration ti proposal ti= " << ti_prop;

                    } else {

                        const double tminCalib = date.mCalibration->mTmin;
                        const double tmaxCalib = date.mCalibration->mTmax;

                        const double s = std::max((date.mSettings.mTmax - date.mSettings.mTmin), tmaxCalib - tminCalib) / 2.0 ;
                        ti_prop = Generator::normalDistribution(ti_old, s);

                        //qDebug() << "Calibration ti mixing ti = " << ti_prop;
                    }

                    prop_ti.push_back(ti_prop);

                    // Vraisemblance (divisée par T si Simulated Annealing)

                    const double L_old = date.getLikelihood(ti_old);
                    const double L_new = date.getLikelihood(ti_prop);
                    const double q_fwd = date.fProposalDensity(ti_prop, ti_old);
                    const double q_rev = date.fProposalDensity(ti_old, ti_prop);

                    // Si une proba est nulle, on flag l'invalidité et on stoppe LA BOUCLE DES DATES (break)
                    if (L_new <= 0.0 || L_old <= 0.0 || q_fwd <= 0.0 || q_rev <= 0.0) {
                        invalid = true;
                        break;
                    }

                    log_rate_L += (std::log(L_new) - std::log(L_old));
                    log_rate_q += std::log(q_rev) - std::log(q_fwd);

#pragma mark Update Delta
                    // Proposition de Delta, suivant son Prior, le ratio MH = 1
                    old_delta.push_back(date.mDelta);

                    double delta_prop;

                    switch (date.mDeltaType) {
                    case Date::eDeltaNone: delta_prop = 0.0; break;
                    case Date::eDeltaRange: delta_prop = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax); break;
                    case Date::eDeltaGaussian: delta_prop = Generator::normalDistribution(date.mDeltaAverage, date.mDeltaError); break;
                    case Date::eDeltaFixed: delta_prop = date.mDeltaFixed; break;
                    }
                    prop_delta.push_back(delta_prop);

#pragma mark update sigma
                    const double S02 = event->mS02Theta.value();

                    constexpr double VMin = 1e-12;
                    constexpr double VMax = 1e10;

                    const double V2 = Generator::shrinkageUniforme(S02, VMin, VMax);
                    const double sigma_prop = std::sqrt(V2);

                    prop_sigma.push_back(sigma_prop);
                    // ------

                }

                if (invalid) {
                    for (auto&& date : event->mDates) {
                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                    }
                    event->mTheta.reject_update();
                    continue; // On passe proprement à l'événement SUIVANT dans events, sans crasher ni tout arrêter.
                }

                // ======================================================================
                // 5. Cas limite : min == max
                // ======================================================================
                if (min == max) {
                    qDebug() << "[ " << __func__ << QString("]  ‼️ Warning for event : %1 : min == max = %2")
                    .arg(event->getQStringName(), QString::number(min));

                    // Décision MH basée sur la vraisemblance et le proposal uniquement
                    const double log_rate_total = log_rate_L + log_rate_q;

                    if (event->mTheta.try_update_log(min, log_rate_total)) {
                        for (size_t i = 0; i < event->mDates.size(); ++i) {
                            event->mDates[i].mTi.accept_update(prop_ti[i]);
                            event->mDates[i].mSigmaTi.accept_update(prop_sigma[i]);
                            event->mDates[i].mDelta = prop_delta[i];
                        }
                    } else {
                        for (auto&& date : event->mDates) {
                            date.mTi.reject_update();
                            date.mSigmaTi.reject_update();
                        }
                    }
                }
                // ======================================================================
                // 6. Mise à jour par bloc conjoint : (t_i, sigma_ti, delta, theta)
                // ======================================================================
                else {
                    // ======================================================================
                    // 7. Calculs des moyennes et dispersions
                    // ======================================================================
                    double sum_p_old = 0.0;
                    double sum_t_old = 0.0;

                    double sum_p_prop= 0.0;
                    double sum_t_prop = 0.0;

                    for (size_t i = 0 ; i<event->mDates.size(); i++) {
                        const double var_old = std::pow(old_sigma[i], 2.0);
                        sum_t_old += (old_ti[i] + old_delta[i]) / var_old;
                        sum_p_old += 1.0 / var_old;

                        const double var_prop = std::pow(prop_sigma[i], 2.0);
                        sum_t_prop += (prop_ti[i] + prop_delta[i]) / var_prop;
                        sum_p_prop += 1.0 / var_prop;

                    }

                    const double sigma_avg_old = 1.0 / std::sqrt(sum_p_old);
                    const double ti_avg_old = sum_t_old / sum_p_old;

                    const double sigma_avg_prop = 1.0 / std::sqrt(sum_p_prop);
                    const double ti_avg_prop = sum_t_prop / sum_p_prop;

                    // ======================================================================
                    // 8. Calcul des sommes de carrés S(t)
                    // ======================================================================
                    double S_old = 0.0;
                    double S_prop = 0.0;

                    for (size_t i = 0 ; i<event->mDates.size(); i++) {
                        const double var_old = std::pow(old_sigma[i], 2.0);
                        S_old += std::pow(old_ti[i] + old_delta[i] - ti_avg_old, 2.0) / var_old;

                        const double var_prop = std::pow(prop_sigma[i], 2.0);
                        S_prop += std::pow(prop_ti[i] + prop_delta[i] - ti_avg_prop, 2.0) / var_prop;

                    }

                    // ======================================================================
                    // 9. Calcul logarithmique robuste du ratio de troncature
                    // ======================================================================

                    // Ratio des constantes de troncature

                    const double alpha_old_min = (min - ti_avg_old) / sigma_avg_old;
                    const double alpha_old_max = (max - ti_avg_old) / sigma_avg_old;

                    const double alpha_prop_min = (min - ti_avg_prop) / sigma_avg_prop;
                    const double alpha_prop_max = (max - ti_avg_prop) / sigma_avg_prop;

                    const double log_Z_old  = log_diff_cdf(alpha_old_min,  alpha_old_max);
                    const double log_Z_prop = log_diff_cdf(alpha_prop_min, alpha_prop_max);

                    // Terme de normalisation collectif (sigma_avg), nécessaire dès que sigma est tiré
                    const double log_sigma_avg_ratio = std::log(sigma_avg_prop / sigma_avg_old);

                    // Terme manquant : normalisation individuelle prod_i (2*pi*var_i)^{-1/2}
                    double log_sigma_indiv_ratio = 0.0;
                    for (size_t i = 0; i < event->mDates.size(); ++i) {
                        log_sigma_indiv_ratio += std::log(prop_sigma[i] / old_sigma[i]);
                    }

                    const double log_Z_ratio = (log_Z_prop - log_Z_old) + log_sigma_avg_ratio;

                    // ======================================================================
                    // 10. Log-rate global d'acceptation
                    // ======================================================================
                    const double log_prior_marginal_diff = log_Z_ratio - log_sigma_indiv_ratio - 0.5 * (S_prop - S_old);
                    const double log_rate_total = log_rate_L + log_rate_q + log_prior_marginal_diff;

                    // ======================================================================
                    // 11. Décision Metropolis-Hastings sur la proposition de conjointe
                    // ======================================================================
                    if (MHAcceptanceTest_log(log_rate_total)) {
                        // =====================================================================
                        // SAUT ACCEPTÉ : t_i se déplace vers prop_ti
                        // =====================================================================
                        size_t i = 0;
                        for (auto&& date : event->mDates) {
                            date.mTi.accept_update(prop_ti[i]);
                            date.mSigmaTi.accept_update(prop_sigma[i]);
                            date.mDelta = prop_delta[i++];
                        }

#pragma mark Update Theta
                        // Échantillonnage de Gibbs exact pour theta sachant le NOUVEL état t_prop
                        // (Ce tirage est TOUJOURS valide et ne nécessite pas de test MH)
                        const double new_theta = Generator::truncatedNormal(ti_avg_prop, sigma_avg_prop, min, max);
                        event->mTheta.accept_update(new_theta);

                    } else {
                        // =====================================================================
                        // SAUT REFUSÉ : t_i conserve sa valeur old_ti
                        // =====================================================================
                        for (auto&& date : event->mDates) {
                            date.mTi.reject_update();
                            date.mSigmaTi.reject_update();
                        }
                        event->mTheta.reject_update();
                    }
                }

                // ======================================================================
                // 12. Mise à jour des hyperparamètres, wiggles et EDM2 pour l'événement courant
                // ======================================================================


#pragma mark Update Wiggle
                for (auto&& date : event->mDates) {
                    date.updateWiggle();
                    date.mWiggle.accept_update(date.mWiggle.value());
                }

#pragma mark Update S02Theta
                if (AppSettings::mEventModel == EventModelType::EDM2) {
                    if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
                        event->updateS02Theta_v338();
                }

                // ======================================================================
                // 13. Mise à jour des bornes des phases de l'événement
                // ======================================================================
                std::for_each(event->mPhases.begin(), event->mPhases.end(),
                              [this](std::shared_ptr<Phase> p) {
                                  p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                              });
            }
        }

        // ======================================================================
        // 14. Mises à jour globales des phases et contraintes du modèle
        // ======================================================================
        std::for_each(mModel->mPhases.begin(), mModel->mPhases.end(),
                      [this](std::shared_ptr<Phase> p) {
                          p->update_Tau(tminPeriod, tmaxPeriod);
                      });

        std::for_each(mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });

    } catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
        return;

    } catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error" << e.what();
        return;

    } catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range" << e.what();
        return;

    } catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;

    } catch(...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
        return;
    }
}


void MCMCLoopChrono::sampler_339_4vXi(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        // ======================================================================
        // 1. Mise à jour de tous les événements
        // ======================================================================
        for (auto &event : events) {
            // ======================================================================
            // 2. Mise à jour de theta uniquement si non fixé
            // ======================================================================
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
                // ======================================================================
                // 3. Évaluation des bornes de theta
                // ======================================================================
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                    .arg(QString::fromLatin1(__func__),
                         event->getQStringName(),
                         QString::number(min),
                         QString::number(max));
                }

#pragma mark Proposal ti
                // ======================================================================
                // 4. Proposition des ti avec leurs vraisemblances L
                // ======================================================================

                std::vector<double> old_ti;
                old_ti.reserve(event->mDates.size());
                std::vector<double> prop_ti;
                prop_ti.reserve(event->mDates.size());

                std::vector<double> old_delta;
                old_delta.reserve(event->mDates.size());
                std::vector<double> prop_delta;
                prop_delta.reserve(event->mDates.size());

                std::vector<double> old_sigma;
                old_sigma.reserve(event->mDates.size());
                std::vector<double> prop_sigma;
                prop_sigma.reserve(event->mDates.size());

                /*std::vector<double> old_x;
                old_x.reserve(event->mDates.size());
                std::vector<double> prop_x;
                prop_x.reserve(event->mDates.size());*/

                double log_rate_L = 0.0; // Rapport de vraisemblance (divisé par T si Annealing)
                double log_rate_q = 0.0; // Ratio de Hastings des proposals de ti

                double log_rate_x = 0.0; // Ratio de Hastings des proposals de xi

                bool invalid = false;
                for (auto&& date : event->mDates) {
                    const double ti_old = date.mTi.value();
                    old_ti.push_back(ti_old);

                    const double sigma_old = date.mSigmaTi.value();
                    old_sigma.push_back(sigma_old);

                    double ti_prop;

                    if (Generator::randomUniform() < date.mMixingLevel) {

                        ti_prop = *date.mCalibration->sample_t();
                        //qDebug() << "Calibration ti proposal ti= " << ti_prop;

                    } else {

                        const double tminCalib = date.mCalibration->mTmin;
                        const double tmaxCalib = date.mCalibration->mTmax;

                        const double s = std::max((date.mSettings.mTmax - date.mSettings.mTmin), tmaxCalib - tminCalib) / 2.0 ;
                        ti_prop = Generator::normalDistribution(ti_old, s);

                        //qDebug() << "Calibration ti mixing ti = " << ti_prop;
                    }

                    prop_ti.push_back(ti_prop);

                    // Vraisemblance (divisée par T si Simulated Annealing)

                    const double L_old = date.getLikelihood(ti_old);
                    const double L_new = date.getLikelihood(ti_prop);
                    const double q_fwd = date.fProposalDensity(ti_prop, ti_old);
                    const double q_rev = date.fProposalDensity(ti_old, ti_prop);

                    // Si une proba est nulle, on flag l'invalidité et on stoppe LA BOUCLE DES DATES (break)
                    if (L_new <= 0.0 || L_old <= 0.0 || q_fwd <= 0.0 || q_rev <= 0.0) {
                        invalid = true;
                        break;
                    }

                    log_rate_L += (std::log(L_new) - std::log(L_old));

                    log_rate_q += std::log(q_rev) - std::log(q_fwd);

#pragma mark Update Delta
                    // Proposition de Delta, suivant son Prior, le ratio MH = 1
                    old_delta.push_back(date.mDelta);

                    double delta_prop;

                    switch (date.mDeltaType) {
                    case Date::eDeltaNone: delta_prop = 0.0; break;
                    case Date::eDeltaRange: delta_prop = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax); break;
                    case Date::eDeltaGaussian: delta_prop = Generator::normalDistribution(date.mDeltaAverage, date.mDeltaError); break;
                    case Date::eDeltaFixed: delta_prop = date.mDeltaFixed; break;
                    }
                    prop_delta.push_back(delta_prop);

#pragma mark update sigma
                    /*
                    // --- Dans la boucle des dates  ---
                    const double x_old = date.mXi;
                    old_x.push_back(x_old);

                    const double x_prop = Generator::gammaDistribution(0.5, 2.0); // indépendant, cancel avec x^{-1/2}e^{-x/2}
                   // const double x_prop = Generator::randomUniform(0.01, 100.0); // Plage à ajuster selon votre modèle // test ici
                    prop_x.push_back(x_prop);

                    const double diff_old  = ti_old  + date.mDelta - event->mTheta.value();
                    const double diff_prop = ti_prop + delta_prop  - event->mTheta.value(); // theta pas encore mis à jour ici

                    const double sigma_old_i  = std::sqrt((diff_old  * diff_old ) / (2.0 * x_old));
                    const double sigma_prop_i = std::sqrt((diff_prop * diff_prop) / (2.0 * x_prop));

                    old_sigma.push_back(sigma_old_i);
                    prop_sigma.push_back(sigma_prop_i);

                    const double S02 = event->mS02Theta.value();
                    // code Claude
                   // log_rate_x += (std::log(x_prop) + std::log(std::abs(diff_prop)) - 2.0*std::log(diff_prop*diff_prop + S02*x_prop))
                     //             - (std::log(x_old)  + std::log(std::abs(diff_old))  - 2.0*std::log(diff_old*diff_old   + S02*x_old));

                    // correctin emmy
                    //log_rate_x += (std::log(x_prop) - 0.5 * x_prop + std::log(std::abs(diff_prop)) - 2.0*std::log(diff_prop*diff_prop + S02*x_prop))
                    //              - (std::log(x_old) - 0.5 * x_old + std::log(std::abs(diff_old)) - 2.0*std::log(diff_old*diff_old + S02*x_old));

                    log_rate_x += (std::log(x_prop) - 0.5 * x_prop + std::log(std::abs(diff_prop)) - 2.0*std::log(diff_prop*diff_prop + S02*x_prop))
                                  - (std::log(x_old) - 0.5 * x_old + std::log(std::abs(diff_old)) - 2.0*std::log(diff_old*diff_old + S02*x_old))
                                  + 0.5 * (std::log(x_prop) - std::log(x_old))  // Ratio de Hastings pour Gamma
                                  + 0.5 * (x_prop - x_old);
*/
#pragma mark update sigma
                    // --- Dans la boucle des dates  ---
                    // 1. Suppression de la variable 'x' et de 'diff'
                    const double sigma_old_i = date.mSigmaTi.value();
                    old_sigma.push_back(sigma_old_i);

                    // 2. Proposition de sigma via une marche aléatoire Log-Normale
                    // (Le step size, ici 0.2, peut être ajusté pour cibler un taux d'acceptation optimal)
                    const double z_step = Generator::normalDistribution(0, 2);
                    const double sigma_prop_i = sigma_old_i * std::exp(z_step);
                    prop_sigma.push_back(sigma_prop_i);

                    // 3. Évaluation du Prior sur la variance (Uniform Shrinkage)
                    // P(sigma) proportionnel à S0 / (sigma + S0)^2   (ou S02 selon votre modèle)
                    const double S02 = event->mS02Theta.value();


                    //  Prior sigma :
                    const double log_prior_sigma_prop = std::log(sigma_prop_i) - 2.0 * std::log(sigma_prop_i * sigma_prop_i + S02);
                    const double log_prior_sigma_old  = std::log(sigma_old_i)  - 2.0 * std::log(sigma_old_i * sigma_old_i + S02);

                    // 4. Ratio de Hastings pour la proposition Log-Normale
                    // Asymétrie due au changement de variable d(exp(z))
                    const double log_q_sigma = std::log(sigma_prop_i) - std::log(sigma_old_i);

                    // 5. Mise à jour de log_rate_x (qui devient le log_rate_sigma)
                    log_rate_x += (log_prior_sigma_prop - log_prior_sigma_old) + log_q_sigma;
                }

                if (invalid) {
                    for (auto&& date : event->mDates) {
                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                    }
                    event->mTheta.reject_update();
                    continue; // On passe proprement à l'événement SUIVANT dans events, sans crasher ni tout arrêter.
                }

                // ======================================================================
                // 5. Cas limite : min == max
                // ======================================================================
                if (min == max) {
                    qDebug() << "[ " << __func__ << QString("]  ‼️ Warning for event : %1 : min == max = %2")
                                                        .arg(event->getQStringName(), QString::number(min));

                    // Décision MH basée sur la vraisemblance et le proposal uniquement
                    const double log_rate_total = log_rate_L + log_rate_q + log_rate_x;

                    if (event->mTheta.try_update_log(min, log_rate_total)) {
                        for (size_t i = 0; i < event->mDates.size(); ++i) {
                            //event->mDates[i].mXi = prop_x[i];
                            event->mDates[i].mTi.accept_update(prop_ti[i]);
                            event->mDates[i].mSigmaTi.accept_update(prop_sigma[i]);
                            event->mDates[i].mDelta = prop_delta[i];
                        }
                    } else {
                        for (auto&& date : event->mDates) {
                            date.mTi.reject_update();
                            date.mSigmaTi.reject_update();
                        }
                    }
                }
                // ======================================================================
                // 6. Mise à jour par bloc conjoint : (t_i, sigma_ti, delta, theta)
                // ======================================================================
                else {
                    // ======================================================================
                    // 7. Calculs des moyennes et dispersions
                    // ======================================================================
                    double sum_p_old = 0.0;
                    double sum_t_old = 0.0;

                    double sum_p_prop= 0.0;
                    double sum_t_prop = 0.0;

                    for (size_t i = 0 ; i<event->mDates.size(); i++) {
                        const double var_old = std::pow(old_sigma[i], 2.0);
                        sum_t_old += (old_ti[i] + old_delta[i]) / var_old;
                        sum_p_old += 1.0 / var_old;

                        const double var_prop = std::pow(prop_sigma[i], 2.0);
                        sum_t_prop += (prop_ti[i] + prop_delta[i]) / var_prop;
                        sum_p_prop += 1.0 / var_prop;

                    }

                    const double sigma_avg_old = 1.0 / std::sqrt(sum_p_old);
                    const double ti_avg_old = sum_t_old / sum_p_old;

                    const double sigma_avg_prop = 1.0 / std::sqrt(sum_p_prop);
                    const double ti_avg_prop = sum_t_prop / sum_p_prop;

                    // ======================================================================
                    // 8. Calcul des sommes de carrés S(t)
                    // ======================================================================
                    double S_old = 0.0;
                    double S_prop = 0.0;

                    for (size_t i = 0 ; i<event->mDates.size(); i++) {
                        const double var_old = std::pow(old_sigma[i], 2.0);
                        S_old += std::pow(old_ti[i] + old_delta[i] - ti_avg_old, 2.0) / var_old;

                        const double var_prop = std::pow(prop_sigma[i], 2.0);
                        S_prop += std::pow(prop_ti[i] + prop_delta[i] - ti_avg_prop, 2.0) / var_prop;

                    }

                    // ======================================================================
                    // 9. Calcul logarithmique robuste du ratio de troncature
                    // ======================================================================

                    // Ratio des constantes de troncature

                    const double alpha_old_min = (min - ti_avg_old) / sigma_avg_old;
                    const double alpha_old_max = (max - ti_avg_old) / sigma_avg_old;

                    const double alpha_prop_min = (min - ti_avg_prop) / sigma_avg_prop;
                    const double alpha_prop_max = (max - ti_avg_prop) / sigma_avg_prop;

                    const double log_Z_old  = log_diff_cdf(alpha_old_min,  alpha_old_max);
                    const double log_Z_prop = log_diff_cdf(alpha_prop_min, alpha_prop_max);

                    // Terme de normalisation collectif (sigma_avg), nécessaire dès que sigma est tiré
                    const double log_sigma_avg_ratio = std::log(sigma_avg_prop / sigma_avg_old);


                    const double log_Z_ratio = (log_Z_prop - log_Z_old) + log_sigma_avg_ratio;

                    // ======================================================================
                    // 10. Log-rate global d'acceptation
                    // ======================================================================
                    const double log_prior_marginal_diff = log_Z_ratio  - 0.5 * (S_prop - S_old);

                    const double log_rate_total = log_rate_L + log_rate_q + log_prior_marginal_diff + log_rate_x;
                    // ======================================================================
                    // 11. Décision Metropolis-Hastings sur la proposition de conjointe
                    // ======================================================================
                    if (MHAcceptanceTest_log(log_rate_total)) {
                        // =====================================================================
                        // SAUT ACCEPTÉ : t_i se déplace vers prop_ti
                        // =====================================================================
                        size_t i = 0;
                        for (auto&& date : event->mDates) {
                           // date.mXi = prop_x[i];
                            date.mTi.accept_update(prop_ti[i]);
                            date.mSigmaTi.accept_update(prop_sigma[i]);
                            date.mDelta = prop_delta[i++];
                        }

#pragma mark Update Theta
                        // Échantillonnage de Gibbs exact pour theta sachant le NOUVEL état t_prop
                        // (Ce tirage est TOUJOURS valide et ne nécessite pas de test MH)
                        const double new_theta = Generator::truncatedNormal(ti_avg_prop, sigma_avg_prop, min, max);
                        event->mTheta.accept_update(new_theta);

                    } else {
                        // =====================================================================
                        // SAUT REFUSÉ : t_i conserve sa valeur old_ti
                        // =====================================================================
                        for (auto&& date : event->mDates) {
                            date.mTi.reject_update();
                            date.mSigmaTi.reject_update();
                        }
                        event->mTheta.reject_update();
                    }
                }

                // ======================================================================
                // 12. Mise à jour des hyperparamètres, wiggles et EDM2 pour l'événement courant
                // ======================================================================


#pragma mark Update Wiggle
                for (auto&& date : event->mDates) {
                    date.updateWiggle();
                    date.mWiggle.accept_update(date.mWiggle.value());
                }

#pragma mark Update S02Theta
                if (AppSettings::mEventModel == EventModelType::EDM2) {
                    if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
                        event->updateS02Theta_v338();
                }

                // ======================================================================
                // 13. Mise à jour des bornes des phases de l'événement
                // ======================================================================
                std::for_each(event->mPhases.begin(), event->mPhases.end(),
                              [this](std::shared_ptr<Phase> p) {
                                  p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                              });
            }
        }

        // ======================================================================
        // 14. Mises à jour globales des phases et contraintes du modèle
        // ======================================================================
        std::for_each(mModel->mPhases.begin(), mModel->mPhases.end(),
                      [this](std::shared_ptr<Phase> p) {
                          p->update_Tau(tminPeriod, tmaxPeriod);
                      });

        std::for_each(mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });

    } catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
        return;

    } catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error" << e.what();
        return;

    } catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range" << e.what();
        return;

    } catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;

    } catch(...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
        return;
    }
}

void MCMCLoopChrono::sampler_339_SingleSite(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        // ======================================================================
        // 1. Mise à jour de tous les événements
        // ======================================================================
        for (auto &event : events) {

            // ======================================================================
            // 2. Mise à jour de theta uniquement si non fixé
            // ======================================================================
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {

                // ==================================================================
                // 3. Évaluation des bornes de theta
                // ==================================================================
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                    .arg(QString::fromLatin1(__func__),
                         event->getQStringName(),
                         QString::number(min),
                         QString::number(max));
                }

#pragma mark Proposal ti, sigma, delta (Single-Site, par date)
                // ==================================================================
                // 4. Initialisation des statistiques pondérées
                //
                //    y_i = t_i + delta_i
                //    w_i = 1 / sigma_i²
                //
                //    P     = sum(w_i)
                //    mu    = sum(w_i y_i) / P
                //    S     = sum(w_i (y_i - mu)²)
                //
                //    IMPORTANT :
                //    on n'utilise PAS :
                //
                //       Q - T²/P
                //
                //    afin d'éviter une forte annulation numérique lorsque
                //    les sigma_i sont petits.
                // ==================================================================
                long double P_curr = 0.0L;
                long double mu_curr = 0.0L;
                long double S_curr = 0.0L;

                for (const auto& date : event->mDates) {

                    const long double sigma = static_cast<long double>(date.mSigmaTi.value());

                    const long double y =
                        static_cast<long double>(date.mTi.value())
                        + static_cast<long double>(date.mDelta);

                    const long double w = 1.0L / (sigma * sigma);

                    if (P_curr == 0.0L) {
                        P_curr  = w;
                        mu_curr = y;
                        S_curr  = 0.0L;
                    }
                    else {
                        // Weighted Welford update
                        const long double P_old = P_curr;
                        const long double mu_old = mu_curr;

                        const long double P_new = P_old + w;
                        const long double d = y - mu_old;
                        const long double mu_new = mu_old + (w / P_new) * d;

                        S_curr +=  w * d * (y - mu_new);
                        P_curr  = P_new;
                        mu_curr = mu_new;
                    }
                }

                // ==================================================================
                // 5. Mise à jour single-site de chaque date
                // ==================================================================
                // ==========================================================================
                // 5. Mise à jour single-site de chaque date : Metropolis-within-Gibbs
                //    Étape A : (ti, delta) | sigma fixe
                //    Étape B : sigma       | (ti, delta) fixés
                // ==========================================================================
#pragma mark mixingKernel

                constexpr double mixingKernel = 0;
                // 0 -> A.1 ti calibré sigma shrinkage ; OK fonctionne ;
                // 1 -> B.2 à controler


                for (size_t i = 0; i < event->mDates.size(); ++i) {

                    auto& date = event->mDates[i];
                    const double ti_old0    = date.mTi.value();

                    const double delta_old0 = date.mDelta;
                    const double sigma_cur  = date.mSigmaTi.value();   // inchangé dans cette étape

                    double ti_prop;
                    double log_rate_L = 0;;
                    double log_rate_q = 0;
                    bool isvalide = true;

#pragma mark --- Étape A : proposition de (ti, delta) | sigma fixe ---

                    // 1. Calcul préalable Leave-One-Out (centre mu_removed des N-1 autres dates)
                    // pour noyaux ti_bar
                    const long double V1_curr    = static_cast<long double>(sigma_cur) * sigma_cur;
                    const long double w_curr     = 1.0L / V1_curr;
                    const long double y_old_curr = static_cast<long double>(ti_old0) + delta_old0;

                    const long double P_removed = P_curr - w_curr;
                    long double mu_removed = 0.0L;

                    if (P_removed > 0.0L) {
                        mu_removed = (P_curr * mu_curr - w_curr * y_old_curr) / P_removed;

                    } else {
                        mu_removed = y_old_curr; // Événement à 1 seule date
                    }

#pragma mark update delta (déplacé ICI : requis par le noyau B.3 avant construction de ti_prop)
                    double delta_prop;
                    switch (date.mDeltaType) {
                    case Date::eDeltaNone:     delta_prop = 0.0; break;
                    case Date::eDeltaRange:    delta_prop = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax); break;
                    case Date::eDeltaGaussian: delta_prop = Generator::normalDistribution(date.mDeltaAverage, date.mDeltaError); break;
                    case Date::eDeltaFixed:    delta_prop = date.mDeltaFixed; break;
                    default: delta_prop = delta_old0; break;
                    }
                    double u_kernel = Generator::randomUniform();
                    /*if (u_kernel < mixingKernel) {
                        // ==============================================================
                        // NOYAU B.1 : Marche Aléatoire Gaussienne Adaptative (sauts locaux)
                        // ==============================================================
                        date.mTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                        const double step_size = date.mTi.mSigmaMH; // Pas adaptatif s_i courant
                        ti_prop = Generator::normalDistribution(ti_old0, step_size);

                        // Transition symétrique N(ti_old0, step_size) -> q_rev == q_fwd
                        log_rate_q = 0.0;

                    }*/

                    if (u_kernel < mixingKernel) {
                        // ==============================================================
                        // NOYAU B.2 : Saut d'Indépendance Non-Centré (NCP Papaspiliopoulos)
                        // Centré sur la moyenne marginalisée (ti_bar)
                        // ==============================================================
#pragma mark NOYAU B.2
                        date.mTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                        // 1. Définition du centre physique (leave-one-out de l'événement)
                        const double ti_bar = static_cast<double>(mu_removed) - delta_old0;

                        // 2. Tirage INDÉPENDANT dans l'espace latent adimensionnel z
                        // s_z gère l'amplitude de l'exploration autour de la moyenne (idéalement proche de 1)
                        const double s_z = date.mTi.mSigmaMH;
                        const double z_prop = Generator::normalDistribution(0.0, s_z);

                        // 3. Transformation déterministe vers l'espace physique (t = mu + sigma * z)
                        ti_prop = ti_bar + sigma_cur * z_prop;

                        // 4. Projection de l'état actuel dans l'espace latent pour évaluer q_rev
                        const double z_old = (ti_old0 - ti_bar) / sigma_cur;

                        // 5. Ratio de Metropolis-Hastings (log(q_rev) - log(q_fwd))
                        // Le Jacobien du changement de variable (1/sigma_cur) s'annule parfaitement
                        // entre le numérateur et le dénominateur.
                        const double d_old  = z_old / s_z;
                        const double d_prop = z_prop / s_z;

                        log_rate_q = 0.5 * (d_prop * d_prop - d_old * d_old);


                    }

                   /* if (u_kernel < mixingKernel) {
                        // ==============================================================
                        // NOYAU B.3 : Saut Non-Centré Centré sur Theta avec Résidu Xi_i
                        // Center = theta - delta_i
                        // Xi_i   = ((t_i + delta_i - theta) / sigma_i)^2
                        // ==============================================================
                        date.mTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                        const double theta_curr = event->mTheta.value();
                        const double delta_curr = date.mDelta;
                        const double sigma_curr = date.mSigmaTi.value();
                        const double s_z        = date.mTi.mSigmaMH;

                        // 1. Projection actuelle dans l'espace latent z_old et calcul de xi_old
                        const double z_old  = (ti_old0 + delta_curr - theta_curr) / sigma_curr;

                        // 2. Tirage indépendant dans l'espace latent adimensionnel
                        const double z_prop  = Generator::normalDistribution(0.0, s_z);

                        // 3. Reconstitution déterministe de ti_prop dans l'espace réel
                        ti_prop = theta_curr - delta_prop + sigma_curr * z_prop;

                        if (i==0) std::cout << " ti_prop = " << ti_prop
                                      << " theta_curr = " << theta_curr
                                      << " sigma_curr *s_z = " << sigma_curr *s_z
                                      << std::endl;

                        // 4. Calcul du ratio de Metropolis-Hastings (log(q_rev / q_fwd))
                        // (xi_prop - xi_old) / (2 * s_z * s_z)
                        const double d_old  = z_old / s_z;
                        const double d_prop = z_prop / s_z;

                        log_rate_q = 0.5 * (d_prop * d_prop - d_old * d_old);
                    }*/

                    /*if (u_kernel < mixingKernel) {
                        // ==============================================================
                        // NOYAU B.4 : Marche Aléatoire Gaussienne Non Centrée sur z_i
                        // Papaspiliopoulos–Roberts–Sköld
                        // ==============================================================
                        date.mTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                        const double theta_curr = event->mTheta.value();
                        const double sigma_curr = date.mSigmaTi.value();

                        // 1. Passage dans l'espace non centré z_i ~ N(0, 1)
                        const double z_old = (ti_old0 + delta_old0 - theta_curr) / sigma_curr;

                        // 2. Saut adaptatif sur la variable z (pas de saut déconnecté de l'échelle de ti)
                        const double step_size_z = date.mTi.mSigmaMH;
                        const double z_prop = Generator::normalDistribution(z_old, step_size_z);

                        // 3. Reconstitution de la proposition ti_prop dans l'espace réel
                        ti_prop = theta_curr - delta_old0 + sigma_curr * z_prop;

                        // La transition sur z est symétrique et le jacobien (sigma_curr) s'annule dans le ratio
                        log_rate_q = 0.0;



                    }*/

                   /* if (u_kernel < mixingKernel) {
                        // ==============================================================
                        // NOYAU B.5 : Marche Aléatoire Centrée sur ti_bar (mu_removed)
                        // ==============================================================
                        date.mTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                        // Centre pour ti (mu_removed est le centre dans l'espace y = ti + delta)
                        const double ti_bar = static_cast<double>(mu_removed) - delta_old0;
                        const double step_size = date.mTi.mSigmaMH * sigma_cur;

                        // Proposition Gaussienne centrée sur le reste du groupe (ti_bar)
                        ti_prop = Generator::normalDistribution(ti_bar, step_size);

                        // Correction du ratio q(t_old) / q(t_prop) pour noyau non centré sur t_old
                        const double d_old  = (ti_old0 - ti_bar) / step_size;
                        const double d_prop = (ti_prop - ti_bar) / step_size;

                        log_rate_q = 0.5 * (d_prop * d_prop - d_old * d_old);
                    }*/
                    else {
                        // ==============================================================
                        // NOYAU A.1 : Proposition globale (Mélange Calibré + N() via mMixingLevel)
                        // ==============================================================

#pragma mark NOYAU A.1
                        date.mTi.mSamplerProposal = SamplerProposal::eLikelihood;

                        if (Generator::randomUniform() < date.mMixingLevel) {
                            ti_prop = *date.mCalibration->sample_t();
                        } else {
                            const double tminCalib = date.mCalibration->mTmin;
                            const double tmaxCalib = date.mCalibration->mTmax;
                            const double s = std::max((date.mSettings.mTmax - date.mSettings.mTmin),
                                                      tmaxCalib - tminCalib) / 2.0;
                            ti_prop = Generator::normalDistribution(ti_old0, s);
                        }

                        const double q_fwd = date.fProposalDensity(ti_prop, ti_old0);
                        const double q_rev = date.fProposalDensity(ti_old0, ti_prop);
                        if ( q_fwd <= 0.0 || q_rev <= 0.0) {
                            isvalide = false;
                            // sigma n'est pas concerné : on continue vers l'étape B
                        } else {
                            log_rate_q = std::log(q_rev) - std::log(q_fwd);
                        }
                    }

                    const double L_old = date.getLikelihood(ti_old0);
                    const double L_new = date.getLikelihood(ti_prop);

                    if (L_new <= 0.0 || L_old <= 0.0 ) {
                        date.mTi.reject_update();
                        isvalide = false;
                        // sigma n'est pas concerné : on continue vers l'étape B
                    }


                    if (isvalide) {
                        log_rate_L = std::log(L_new) - std::log(L_old);

                        const long double V1      = static_cast<long double>(sigma_cur) * sigma_cur;
                        const long double w_A     = 1.0L / V1;
                        const long double y_old   = static_cast<long double>(ti_old0) + delta_old0;

                        const long double y_new = static_cast<long double>(ti_prop) + delta_prop;

                        // Retrait / réinsertion O(1) — même poids des deux côtés
                        const long double P_removed = P_curr - w_A;
                        long double mu_removed = 0.0L, S_removed = 0.0L;
                        if (P_removed > 0.0L) {
                            mu_removed = (P_curr * mu_curr - w_A * y_old) / P_removed;
                            S_removed  = S_curr - w_A * (y_old - mu_curr) * (y_old - mu_removed);
                            if (S_removed < 0.0L && S_removed > -1e-18L) S_removed = 0.0L;
                        }

                        long double P_A = P_removed, mu_A = mu_removed, S_A;
                        if (P_A == 0.0L) {
                            P_A = w_A;
                            mu_A = y_new;
                            S_A = 0.0L;
                        } else {
                            const long double d = y_new - mu_A;
                            P_A  = P_removed + w_A;               // == P_curr, par construction
                            mu_A = mu_removed + (w_A / P_A) * d;
                            S_A  = S_removed + w_A * d * (y_new - mu_A);
                        }
                        if (S_A < 0.0L && S_A > -1e-18L) S_A = 0.0L;

                        long double log_prior_marginal_diff_A = 0.0L;
                        bool degenerate_A = false;

                            if (min == max) {
                                const long double theta_fixed  = static_cast<long double>(min);
                                const long double residual_old = y_old - theta_fixed;
                                const long double residual_new = y_new - theta_fixed;
                                // sigma inchangé -> pas de terme log_sigma_i_ratio
                                log_prior_marginal_diff_A = -0.5L * (residual_new * residual_new
                                                                     - residual_old * residual_old) / V1;
                            } else {
                                const long double sigma_avg = 1.0L / std::sqrt(P_curr);  // == 1/sqrt(P_A)

                                const long double alpha_old_min = (static_cast<long double>(min) - mu_curr) / sigma_avg;
                                const long double alpha_old_max = (static_cast<long double>(max) - mu_curr) / sigma_avg;
                                const long double alpha_new_min = (static_cast<long double>(min) - mu_A)    / sigma_avg;
                                const long double alpha_new_max = (static_cast<long double>(max) - mu_A)    / sigma_avg;

                                const double log_Z_old = log_diff_cdf(static_cast<double>(alpha_old_min), static_cast<double>(alpha_old_max));
                                const double log_Z_new = log_diff_cdf(static_cast<double>(alpha_new_min), static_cast<double>(alpha_new_max));

                                if (!std::isfinite(log_Z_old) || !std::isfinite(log_Z_new)) {
                                    degenerate_A = true;
                                } else {
                                    // sigma_avg inchangé -> pas de terme log_sigma_avg_ratio
                                    log_prior_marginal_diff_A = static_cast<long double>(log_Z_new - log_Z_old)
                                                                - 0.5L * (S_A - S_curr);
                                }
                            }

                            if (degenerate_A) {
                                date.mTi.reject_update();
                            } else {
                                const long double log_rate_A = static_cast<long double>(log_rate_L)
                                                               + static_cast<long double>(log_rate_q)
                                                               + log_prior_marginal_diff_A;
#pragma mark accept ti, delta
                                if (MHAcceptanceTest_log(static_cast<double>(log_rate_A))) {
                                    date.mTi.accept_update(ti_prop);
                                    date.mDelta = delta_prop;
                                    P_curr = P_A;
                                    mu_curr = mu_A;
                                    S_curr = S_A;
                                } else {
                                    date.mTi.reject_update();
                                }
                            }
                        }


#pragma mark --- Étape B : proposition de sigma | (ti, delta) fixés ---
                    {
                        const double ti_cur    = date.mTi.value();      // éventuellement mis à jour par A
                        const double delta_cur = date.mDelta;            // idem
                        const double sigma_old = date.mSigmaTi.value();

                        const long double V1      = static_cast<long double>(sigma_old) * sigma_old;
                        const long double y_fixed = static_cast<long double>(ti_cur) + delta_cur;
                        const long double w_old   = 1.0L / V1;

                        date.mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                        const double log10_V1 = std::log10(static_cast<double>(V1));
                        const double log10_V2 = Generator::normalDistribution(log10_V1, date.mSigmaTi.mSigmaMH);

                        const double logVMin = -100.0, logVMax = 100.0;

                        if (!std::isfinite(log10_V2) || log10_V2 < logVMin || log10_V2 > logVMax) {
                            date.mSigmaTi.reject_update();
                        } else {

                            const long double V2 = std::pow(10.0L, static_cast<long double>(log10_V2));

                            if (!(V2 > 0.0L) || !std::isfinite(V2)) {
                                date.mSigmaTi.reject_update();
                            } else {

                                const long double sigma_prop = std::sqrt(V2);
                                const long double w_prop     = 1.0L / V2;

                                const long double P_removed = P_curr - w_old;
                                long double mu_removed = 0.0L, S_removed = 0.0L;
                                if (P_removed > 0.0L) {
                                    mu_removed = (P_curr * mu_curr - w_old * y_fixed) / P_removed;
                                    S_removed  = S_curr - w_old * (y_fixed - mu_curr) * (y_fixed - mu_removed);
                                    if (S_removed < 0.0L && S_removed > -1e-18L) S_removed = 0.0L;
                                }

                                long double P_B = P_removed, mu_B = mu_removed, S_B = S_removed;
                                if (P_B == 0.0L) {
                                    P_B = w_prop; mu_B = y_fixed; S_B = 0.0L;
                                } else {
                                    const long double d = y_fixed - mu_B;
                                    P_B  = P_removed + w_prop;
                                    mu_B = mu_removed + (w_prop / P_B) * d;
                                    S_B  = S_removed + w_prop * d * (y_fixed - mu_B);
                                }
                                if (S_B < 0.0L && S_B > -1e-18L) S_B = 0.0L;

                                const long double log_sigma_i_ratio = std::log(sigma_prop / static_cast<long double>(sigma_old));

                                long double log_prior_marginal_diff_B = 0.0L;
                                bool degenerate_B = false;

                                if (min == max) {
                                    const long double theta_fixed = static_cast<long double>(min);
                                    const long double residual     = y_fixed - theta_fixed;
                                    log_prior_marginal_diff_B = -0.5L * residual * residual * (1.0L / V2 - 1.0L / V1)
                                                                - log_sigma_i_ratio;
                                } else {
                                    const long double sigma_avg_old = 1.0L / std::sqrt(P_curr);
                                    const long double sigma_avg_B   = 1.0L / std::sqrt(P_B);

                                    const long double alpha_old_min = (static_cast<long double>(min) - mu_curr) / sigma_avg_old;
                                    const long double alpha_old_max = (static_cast<long double>(max) - mu_curr) / sigma_avg_old;
                                    const long double alpha_B_min   = (static_cast<long double>(min) - mu_B)    / sigma_avg_B;
                                    const long double alpha_B_max   = (static_cast<long double>(max) - mu_B)    / sigma_avg_B;

                                    const double log_Z_old = log_diff_cdf(static_cast<double>(alpha_old_min), static_cast<double>(alpha_old_max));
                                    const double log_Z_B   = log_diff_cdf(static_cast<double>(alpha_B_min),   static_cast<double>(alpha_B_max));

                                    if (!std::isfinite(log_Z_old) || !std::isfinite(log_Z_B)) {
                                        degenerate_B = true;
                                    } else {
                                        const long double log_sigma_avg_ratio = std::log(sigma_avg_B / sigma_avg_old);
                                        const long double log_Z_ratio = static_cast<long double>(log_Z_B - log_Z_old) + log_sigma_avg_ratio;
                                        log_prior_marginal_diff_B = log_Z_ratio - log_sigma_i_ratio - 0.5L * (S_B - S_curr);
                                    }
                                }

                                if (degenerate_B) {
                                    date.mSigmaTi.reject_update();
                                } else {
                                    const long double S02_ld = static_cast<long double>(event->mS02Theta.value());
                                    const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1) - std::log(S02_ld + V2));
                                    const long double log_hastings_sigma  = std::log(V2) - std::log(V1);

                                    const long double log_rate_B = log_prior_marginal_diff_B
                                                                   + log_prior_shrinkage
                                                                   + log_hastings_sigma;

                                    if (MHAcceptanceTest_log(static_cast<double>(log_rate_B))) {
                                        date.mSigmaTi.accept_update(static_cast<double>(sigma_prop));
                                        P_curr = P_B; mu_curr = mu_B; S_curr = S_B;
                                    } else {
                                        date.mSigmaTi.reject_update();
                                    }
                                }
                            }
                        }
                    }
                }

#pragma mark Update Theta
                // ==================================================================
                // 6. Mise à jour de theta
                //
                //     theta | dates ~ TruncatedNormal(mu, sigma_theta, min, max)
                //
                //     mu         = T/P = mu_curr
                //     sigma_theta = 1/sqrt(P)
                //
                //     Tirage exact Gibbs.
                // ==================================================================
                if (min == max) {

                    qDebug() << "[ " << __func__ << QString("] Warning for event : %1 : min == max = %2")
                                                        .arg( event->getQStringName(), QString::number(min));

                    event->mTheta.accept_update(min);
                }
                else {

                    const double ti_avg_final =
                        static_cast<double>(mu_curr);

                    const double sigma_avg_final = 1.0 / std::sqrt( static_cast<double>(P_curr));

                    const double new_theta = Generator::truncatedNormal(ti_avg_final, sigma_avg_final, min, max);

                    event->mTheta.accept_update(new_theta);
                }

                // ==================================================================
                // 7. Mise à jour des wiggles
                // ==================================================================
#pragma mark Update Wiggle
                for (auto&& date : event->mDates) {
                    date.updateWiggle();
                    date.mWiggle.accept_update(date.mWiggle.value());
                }

                // ==================================================================
                // 8. Mise à jour de S02Theta
                // ==================================================================
#pragma mark Update S02Theta
                if (AppSettings::mEventModel == EventModelType::EDM2) {

                    if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
                        event->updateS02Theta_v338();
                    }
                }

                // ==================================================================
                // 9. Mise à jour des bornes des phases de l'événement
                // ==================================================================
#pragma mark Update Phases
                std::for_each(
                    event->mPhases.begin(),
                    event->mPhases.end(),
                    [this](std::shared_ptr<Phase> p) {

                        p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                    });
            }
        }

        // ======================================================================
        // 10. Mises à jour globales des phases
        // ======================================================================
        std::for_each(
            mModel->mPhases.begin(),
            mModel->mPhases.end(),
            [this](std::shared_ptr<Phase> p) {

                p->update_Tau(
                    tminPeriod,
                    tmaxPeriod);
            });

        // ======================================================================
        // 11. Mise à jour globale des contraintes de phases
        // ======================================================================
        std::for_each( mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
            [](std::shared_ptr<PhaseConstraint> pc) {
                pc->updateGamma();
            });
    }
    catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
        return;
    }
    catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error " << e.what();
        return;
    }
    catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range " << e.what();
        return;
    }
    catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;
    }
    catch (...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
        return;
    }

}

/**
 * @brief Étape MCMC "Single-Site Block" : met à jour conjointement (t_i, delta_i, sigma_i)
 *        pour chaque date de chaque événement, avec theta marginalisé analytiquement,
 *        puis rééchantillonne theta par tirage direct dans sa loi conditionnelle exacte.
 *
 * @details
 * Pour chaque événement (dont le sampler de theta n'est pas fixé), la fonction parcourt
 * ses dates une à une et propose, pour chacune, un déplacement joint de (t_i, delta_i,
 * sigma_i) via un mélange de deux noyaux de proposition tirés avec probabilité
 * `mixingKernel` (constante compilée en dur) :
 *
 *  - **Noyau A.1** (Likelihood/Calib + Shrinkage) : t_i est proposé soit par tirage dans
 *    la courbe de calibration, soit par une marche gaussienne large ; sigma_i est tiré en
 *    indépendance exacte dans son prior Uniform Shrinkage `Generator::shrinkageUniforme`
 *    (paramétré par `S02Theta`), ce qui annule exactement le terme de shrinkage prior
 *    dans le ratio de Hastings (`log_rate_q_s = -log_prior_shrinkage`).
 *
 *  - **Noyau B.2** (Saut Non-Centré + RW sur log10(V)) : sigma_i est proposé par marche
 *    aléatoire adaptative sur u = log10(V), V = sigma_i². Pour t_i :
 *      - si la date n'est pas seule dans l'événement (Leave-One-Out non nul), t_i est
 *        reparamétré via un ancrage externe fixe t_bar (moyenne pondérée Leave-One-Out des
 *        AUTRES dates) et une variable réduite z = (t_i - t_bar)/sigma_i, elle-même
 *        proposée par marche gaussienne symétrique ; le facteur de Hastings correspond au
 *        jacobien joint du changement de variable (t_i, V) -> (z, u), soit
 *        (sigma_prop/sigma_old)^3 ;
 *      - si la date est seule (N=1), t_bar n'a pas d'ancrage externe valide (le
 *        changement de variable ne serait pas symétrique) : t_i est alors proposé par une
 *        marche gaussienne simple, découplée de sigma_prop, et seul le jacobien "V seul"
 *        s'applique, (sigma_prop/sigma_old)^2.
 *
 * Dans les deux noyaux, delta_i est tiré indépendamment de sa loi a priori propre
 * (aucune, uniforme, gaussienne ou fixe), ce qui annule sa contribution au ratio de
 * Hastings (échantillonnage d'indépendance depuis le prior exact).
 *
 * Le ratio de Metropolis-Hastings combine :
 *  - la vraisemblance de calibration (`getLikelihood`) sur t_i,
 *  - le ratio des densités marginales jointes obtenu en intégrant analytiquement theta
 *    (loi a priori uniforme sur [min, max]) : ce terme dépend de la moyenne et de la
 *    précision pondérées de toutes les dates de l'événement (mu_curr, P_curr), maintenues
 *    à jour de façon incrémentale en O(1) par date via une récursion de type Welford
 *    pondérée, avec une opération Leave-One-Out/Add-Back pour isoler la contribution de
 *    la date en cours de mise à jour,
 *  - le prior de rétrécissement (Uniform Shrinkage) sur sigma_i, paramétré par
 *    `event->mS02Theta`,
 *  - les facteurs de Hastings propres au noyau utilisé (cf. ci-dessus).
 *
 * Un cas dégénéré (min == max, borne de theta ponctuelle) traite directement le résidu
 * gaussien sans passer par la marginalisation/troncature. Un autre cas dégénéré (bornes
 * de troncature non finies dans `log_diff_cdf`) rejette systématiquement la proposition.
 *
 * Après la boucle sur les dates de l'événement :
 *  - theta est rééchantillonné par tirage direct dans sa loi conditionnelle exacte
 *    (gaussienne tronquée sur [min, max], de moyenne/précision données par les statistiques
 *    pondérées courantes mu_curr/P_curr) ;
 *  - les wiggles des dates sont mis à jour (`Date::updateWiggle`) ;
 *  - `S02Theta` est réestimé si le modèle d'événement est EDM2 et que son sampler n'est
 *    pas fixé (`Event::updateS02Theta_v338`) ;
 *  - les bornes alpha/beta des phases associées à l'événement sont mises à jour.
 *
 * Enfin, en dehors de la boucle sur les événements, les Tau des phases et les Gamma des
 * contraintes de phase du modèle sont recalculés globalement.
 *
 * @param[in,out] events Liste des événements du modèle à mettre à jour. Chaque événement
 *        et ses dates (t_i, sigma_i, delta_i) sont modifiés en place ; seuls les
 *        événements dont `mTheta.mSamplerProposal != SamplerProposal::eFixe` sont traités
 *        pour les étapes 2 à 9 (les étapes 10 et 11, globales au modèle, s'appliquent
 *        toujours).
 *
 * @pre Les membres `tminPeriod`, `tmaxPeriod` et `mModel` de `MCMCLoopChrono` doivent être
 *      valides. Chaque `Event` de `events` doit avoir au moins une date, et pour chaque
 *      date `mSigmaTi.value() > 0`.
 *
 * @post Pour chaque événement non fixé : t_i, sigma_i et delta_i de chaque date sont soit
 *       acceptés à leur valeur proposée, soit explicitement rejetés
 *       (`accept_update`/`reject_update`) ; `mTheta` est rééchantillonné ; les wiggles,
 *       `S02Theta` (si EDM2) et les bornes de phase de l'événement sont à jour. Les Tau de
 *       phase et Gamma de contrainte de phase du modèle sont à jour globalement.
 *
 * @note Les statistiques pondérées (P_curr, mu_curr, S_curr) sont calculées en `long
 *       double` pour limiter l'accumulation d'erreurs numériques sur la récursion
 *       Leave-One-Out/Add-Back, répétée à chaque date et à chaque itération MCMC.
 * @note `mixingKernel` est une constante compilée en dur (`constexpr`) qui fixe la
 *       proportion de tirages passant par le noyau B.2 plutôt que A.1 ; ce n'est pas un
 *       paramètre réglable à l'exécution dans cette version.
 * @note Le cas N=1 (date seule dans l'événement) est traité séparément dans le noyau B.2 :
 *       le couplage t_i/sigma via z ne s'applique pas faute d'ancrage externe valide.
 *
 * @warning Toute exception levée pendant le traitement d'un événement (y compris la
 *          `QString` levée en cas de `min > max`) est interceptée par les blocs `catch`
 *          en fin de fonction, journalisée via `qWarning`, et provoque un retour immédiat
 *          de la fonction : les événements non encore traités dans la boucle en cours
 *          sont alors silencieusement ignorés pour cette itération.
 *
 * @see Generator::shrinkageUniforme, Generator::normalDistribution, Generator::truncatedNormal,
 *      Date::getLikelihood, Date::fProposalDensity, Date::updateWiggle,
 *      Event::updateS02Theta_v338, Phase::update_AlphaBeta, Phase::update_Tau,
 *      PhaseConstraint::updateGamma, MHAcceptanceTest_log, log_diff_cdf
 */
void MCMCLoopChrono::sampler_339_SingleSite_bloc(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        // ======================================================================
        // 1. Mise à jour de tous les événements
        // ======================================================================
        for (auto &event : events) {

            // ======================================================================
            // 2. Mise à jour de theta uniquement si non fixé
            // ======================================================================
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {

                // ==================================================================
                // 3. Évaluation des bornes de theta
                // ==================================================================
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                    .arg(QString::fromLatin1(__func__),
                         event->getQStringName(),
                         QString::number(min),
                         QString::number(max));
                }

#pragma mark Proposal ti, sigma, delta (Single-Site Block, par date)
                // ==================================================================
                // 4. Initialisation des statistiques pondérées globales
                // ==================================================================
                long double P_curr = 0.0L;
                long double mu_curr = 0.0L;
                long double S_curr = 0.0L;

                for (const auto& date : event->mDates) {
                    const long double sigma = static_cast<long double>(date.mSigmaTi.value());
                    const long double y = static_cast<long double>(date.mTi.value()) + static_cast<long double>(date.mDelta);
                    const long double w = 1.0L / (sigma * sigma);

                    if (P_curr == 0.0L) {
                        P_curr  = w;
                        mu_curr = y;
                        S_curr  = 0.0L;
                    }
                    else {
                        const long double P_old = P_curr;
                        const long double P_new = P_old + w;
                        const long double d = y - mu_curr;
                        const long double mu_new = mu_curr + (w / P_new) * d;

                        S_curr += w * d * (y - mu_new);
                        P_curr  = P_new;
                        mu_curr = mu_new;
                    }
                }

                // ==========================================================================
                // 5. Mise à jour BLOC (ti, delta, sigma) pour chaque date
                // ==========================================================================
#pragma mark mixingKernel
                constexpr double mixingKernel = 0.8;
                // 0 -> A.1 ti calibré sigma shrinkage ; OK fonctionne ;
                // 1 -> B.2 tout RW: ok fonctionne

                for (size_t i = 0; i < event->mDates.size(); ++i) {

                    auto& date = event->mDates[i];
                    const double ti_old0    = date.mTi.value();
                    const double delta_old0 = date.mDelta;
                    const double sigma_old  = date.mSigmaTi.value();

                    double ti_prop = ti_old0;
                    double sigma_prop = sigma_old;
                    double log_rate_L = 0.0;
                    double log_rate_q_t = 0.0;
                    double log_rate_q_s = 0.0;
                    bool isvalide = true;

                    // ----------------------------------------------------------------------
                    // A. Calcul préalable Leave-One-Out O(1)
                    // (Requis pour le noyau B.2 et l'évaluation rapide de la prior)
                    // ----------------------------------------------------------------------
                    const long double V1_old    = static_cast<long double>(sigma_old) * sigma_old;
                    const long double w_old     = 1.0L / V1_old;
                    const long double y_old     = static_cast<long double>(ti_old0) + delta_old0;

                    const long double P_removed = P_curr - w_old;
                    long double mu_removed = 0.0L;
                    long double S_removed  = 0.0L;

                    if (P_removed > 0.0L) {
                        mu_removed = (P_curr * mu_curr - w_old * y_old) / P_removed;
                        S_removed  = S_curr - w_old * (y_old - mu_curr) * (y_old - mu_removed);
                        if (S_removed < 0.0L && S_removed > -1e-18L) S_removed = 0.0L;
                    } else {
                        mu_removed = y_old;
                    }

                    // ----------------------------------------------------------------------
                    // B. Tirage de delta_prop (indépendant, identique pour les deux noyaux)
                    // ----------------------------------------------------------------------
                    double delta_prop;
                    switch (date.mDeltaType) {
                    case Date::eDeltaNone:     delta_prop = 0.0; break;
                    case Date::eDeltaRange:    delta_prop = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax); break;
                    case Date::eDeltaGaussian: delta_prop = Generator::normalDistribution(date.mDeltaAverage, date.mDeltaError); break;
                    case Date::eDeltaFixed:    delta_prop = date.mDeltaFixed; break;
                    default: delta_prop = delta_old0; break;
                    }

                    // ----------------------------------------------------------------------
                    // C. Bloc de propositions (ti, sigma)
                    // ----------------------------------------------------------------------
                    const double u_kernel = Generator::randomUniform();

                    if (u_kernel < mixingKernel) {
                        // ==============================================================
                        // NOYAU B.2 : Saut Non-Centré (ti) + RW sur log10(V) (sigma)
                        // ==============================================================
#pragma mark NOYAU B.2
                        date.mTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
                        date.mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                        // 1. RW sur log10(V), V = sigma^2
                        const double log10_V1 = std::log10(static_cast<double>(V1_old));
                        const double log10_V2 = Generator::normalDistribution(log10_V1, date.mSigmaTi.mSigmaMH);

                        constexpr double logVMin = -100.0;
                        constexpr double logVMax =  100.0;
                        if (!std::isfinite(log10_V2) || log10_V2 < logVMin || log10_V2 > logVMax) {
                            isvalide = false;
                        } else {
                            const double V2 = std::pow(10.0, log10_V2);
                            sigma_prop = std::sqrt(V2);

                            const double s_z = date.mTi.mSigmaMH;

                            if (P_removed > 0.0L) {
                                // ----------------------------------------------------------------
                                // N >= 2 : ti_bar est un ancrage EXTERNE fixe (Leave-One-Out des
                                // AUTRES dates), indépendant de (ti, sigma) de CETTE date. Le
                                // changement de variable (ti,V)->(z,u) est bien défini, même
                                // ancrage aller/retour => jacobien joint valide.
                                // ----------------------------------------------------------------
                                const double ti_bar = static_cast<double>(mu_removed) - delta_old0;

                                const double z_old  = (ti_old0 - ti_bar) / sigma_old;
                                const double z_prop = Generator::normalDistribution(z_old, s_z);
                                ti_prop = ti_bar + sigma_prop * z_prop;

                                // Marche (z,u) symétrique => Hastings = jacobien joint seul
                                // |J(ti,V)->(z,u)| = 1/(V^{3/2} ln10) => (sigma_prop/sigma_old)^3
                                log_rate_q_t = 0.0;
                                log_rate_q_s = 3.0 * std::log(sigma_prop / sigma_old);


                            } else {
                                // ----------------------------------------------------------------
                                // N == 1 : pas d'ancrage externe. "ti_bar = ti_old0" serait
                                // auto-référent (dépend de l'état lui-même) : ce n'est PAS un
                                // changement de variable valide, l'ancrage bougerait aussi au
                                // retour (ti_bar_retour = ti_prop). Le facteur (sigma_prop/
                                // sigma_old)^3 ne s'applique pas ici (terme résiduel dépendant
                                // de z_prop non compensé -- cf. dérivation).
                                //
                                // Fix : DÉCOUPLER ti de sigma_prop. RW adaptif pur, symétrique,
                                // centré sur ti_old0, échelle fixe s_z (indépendante de sigma) :
                                //   log_rate_q_t = 0 trivialement (RW adaptif symétrique)
                                // sigma garde son propre RW log10(V), mais seul désormais :
                                //   Hastings = (sigma_prop/sigma_old)^2 (jacobien "V seul")
                                // ----------------------------------------------------------------
                                ti_prop = ti_old0 + Generator::normalDistribution(0.0, s_z);

                                log_rate_q_t = 0.0;
                                log_rate_q_s = 2.0 * std::log(sigma_prop / sigma_old);
                               // log_rate_q_s = 2.0 * std::log(sigma_old / sigma_prop);
                            }

                        }
                    }

                    else {
                        // ==============================================================
                        // NOYAU A.1 : Likelihood/Calib (ti) + Shrinkage Uniform (sigma)
                        // ==============================================================
#pragma mark NOYAU A.1
                        date.mTi.mSamplerProposal = SamplerProposal::eLikelihood;
                        date.mSigmaTi.mSamplerProposal = SamplerProposal::ePrior;

                        // 1. Tirage indépendant de V dans le prior shrinkage exact
#pragma mark update sigma
                        const double S02 = event->mS02Theta.value();

                        constexpr double VMin = 1e-12;
                        constexpr double VMax = 1e10;

                        const double V2 = Generator::shrinkageUniforme(S02, VMin, VMax);
                        sigma_prop = std::sqrt(V2);

                        if (sigma_prop <= 0.0) {
                            isvalide = false;
                        } else {
                            // 2. Likelihood sur ti (mélange calib / RW normal)
                            if (Generator::randomUniform() < date.mMixingLevel) {
                                ti_prop = *date.mCalibration->sample_t();
                            } else {
                                const double tminCalib = date.mCalibration->mTmin;
                                const double tmaxCalib = date.mCalibration->mTmax;
                                const double s = std::max((date.mSettings.mTmax - date.mSettings.mTmin),
                                                          tmaxCalib - tminCalib) / 2.0;
                                ti_prop = Generator::normalDistribution(ti_old0, s);
                            }

                            const double q_fwd = date.fProposalDensity(ti_prop, ti_old0);
                            const double q_rev = date.fProposalDensity(ti_old0, ti_prop);

                            if (q_fwd <= 0.0 || q_rev <= 0.0) {
                                isvalide = false;
                            } else {
                                log_rate_q_t = std::log(q_rev) - std::log(q_fwd);

                                // --------------------------------------------------------------
                                // Échantillonneur d'indépendance exact sur V : q(V) = p_V(V).
                                // Rapport MH pour un indépendance sampler :
                                //   [p_V(V2)/p_V(V1)] * [q(V1)/q(V2)]
                                //     = [p_V(V2)/p_V(V1)] * [p_V(V1)/p_V(V2)] = 1
                                // Le prior shrinkage s'annule EXACTEMENT avec la proposition.
                                // Comme log_prior_shrinkage = log(p_V(V2)/p_V(V1)) est ajouté
                                // systématiquement à l'Étape E, on lui oppose ici son opposé
                                // exact pour obtenir une contribution nette nulle.
                                // --------------------------------------------------------------
                                log_rate_q_s = -2.0 * (std::log(S02 + static_cast<double>(V1_old)) - std::log(S02 + V2));
                            }
                        }
                    }

                    // ----------------------------------------------------------------------
                    // D. Vraisemblance Calibration (uniquement pour ti)
                    // ----------------------------------------------------------------------
                    if (isvalide) {
                        const double L_old = date.getLikelihood(ti_old0);
                        const double L_new = date.getLikelihood(ti_prop);

                        if (L_new <= 0.0 || L_old <= 0.0 ) {
                            isvalide = false;
                        } else {
                            log_rate_L = std::log(L_new) - std::log(L_old);
                        }
                    }

                    // ----------------------------------------------------------------------
                    // E. Calcul de la Marginal Prior Conjointe et Acceptation
                    // ----------------------------------------------------------------------
                    if (isvalide) {
                        const long double V2_new = static_cast<long double>(sigma_prop) * sigma_prop;
                        const long double w_new  = 1.0L / V2_new;
                        const long double y_new  = static_cast<long double>(ti_prop) + delta_prop;

                        // Ajout du nouvel état O(1)
                        long double P_new = P_removed, mu_new = mu_removed, S_new = S_removed;
                        if (P_new == 0.0L) {
                            P_new = w_new; mu_new = y_new; S_new = 0.0L;
                        } else {
                            const long double d = y_new - mu_new;
                            P_new  = P_removed + w_new;
                            mu_new = mu_removed + (w_new / P_new) * d;
                            S_new  = S_removed + w_new * d * (y_new - mu_new);
                        }
                        if (S_new < 0.0L && S_new > -1e-18L) S_new = 0.0L;

                        const long double log_sigma_i_ratio = std::log(sigma_prop / static_cast<long double>(sigma_old));
                        long double log_prior_marginal_diff = 0.0L;
                        bool degenerate = false;

                        if (min == max) {
                            const long double theta_fixed  = static_cast<long double>(min);
                            const long double residual_old = y_old - theta_fixed;
                            const long double residual_new = y_new - theta_fixed;

                            log_prior_marginal_diff = -0.5L * (residual_new * residual_new / V2_new
                                                               - residual_old * residual_old / V1_old)
                                                      - log_sigma_i_ratio;
                        } else {
                            const long double sigma_avg_old = 1.0L / std::sqrt(P_curr);
                            const long double sigma_avg_new = 1.0L / std::sqrt(P_new);

                            const long double alpha_old_min = (static_cast<long double>(min) - mu_curr) / sigma_avg_old;
                            const long double alpha_old_max = (static_cast<long double>(max) - mu_curr) / sigma_avg_old;
                            const long double alpha_new_min = (static_cast<long double>(min) - mu_new)  / sigma_avg_new;
                            const long double alpha_new_max = (static_cast<long double>(max) - mu_new)  / sigma_avg_new;

                            const double log_Z_old = log_diff_cdf(static_cast<double>(alpha_old_min), static_cast<double>(alpha_old_max));
                            const double log_Z_new = log_diff_cdf(static_cast<double>(alpha_new_min), static_cast<double>(alpha_new_max));

                            if (!std::isfinite(log_Z_old) || !std::isfinite(log_Z_new)) {
                                degenerate = true;
                            } else {
                                const long double log_sigma_avg_ratio = std::log(sigma_avg_new / sigma_avg_old);
                                log_prior_marginal_diff = static_cast<long double>(log_Z_new - log_Z_old)
                                                          + log_sigma_avg_ratio
                                                          - log_sigma_i_ratio
                                                          - 0.5L * (S_new - S_curr);
                            }
                        }

                        if (degenerate) {
                            date.mTi.reject_update();
                            date.mSigmaTi.reject_update();
                        } else {
                            // Shrinkage de la distribution S02
                            const long double S02_ld = static_cast<long double>(event->mS02Theta.value());
                            const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1_old) - std::log(S02_ld + V2_new));

                            const long double log_rate_total = log_prior_marginal_diff
                                                               + log_prior_shrinkage
                                                               + static_cast<long double>(log_rate_L)
                                                               + static_cast<long double>(log_rate_q_t)
                                                               + static_cast<long double>(log_rate_q_s);

                            if (MHAcceptanceTest_log(static_cast<double>(log_rate_total))) {
                                date.mTi.accept_update(ti_prop);
                                date.mSigmaTi.accept_update(static_cast<double>(sigma_prop));
                                date.mDelta = delta_prop;

                                P_curr = P_new;
                                mu_curr = mu_new;
                                S_curr = S_new;
                            } else {
                                date.mTi.reject_update();
                                date.mSigmaTi.reject_update();
                            }
                        }
                    } else {
                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                    }
                } // Fin de boucle des dates

#pragma mark Update Theta
                // ==================================================================
                // 6. Mise à jour de theta
                // ==================================================================
                if (min == max) {
                    qDebug() << "[ " << __func__ << QString("] Warning for event : %1 : min == max = %2")
                    .arg( event->getQStringName(), QString::number(min));
                    event->mTheta.accept_update(min);
                }
                else {
#pragma mark useHMC
                    constexpr bool useHMC = false;   // bascule pour A/B test
                    if (useHMC) {
                        constexpr int L_hmc = 20;
                        const double eps_hmc = (M_PI / 2.0) / L_hmc;  // L*eps ~ pi/2

                        const double new_theta = hmcSampleTheta_temporal(
                                    event->mTheta.value(), mu_curr, P_curr, min, max, L_hmc, eps_hmc);

                        event->mTheta.accept_update(new_theta);
                    } else {
                        const double ti_avg_final = static_cast<double>(mu_curr);
                        const double sigma_avg_final = 1.0 / std::sqrt(static_cast<double>(P_curr));
                        const double new_theta = Generator::truncatedNormal(ti_avg_final, sigma_avg_final, min, max);

                        event->mTheta.accept_update(new_theta);
                    }


                }

                // ==================================================================
                // 7. Mise à jour des wiggles
                // ==================================================================
#pragma mark Update Wiggle
                for (auto&& date : event->mDates) {
                    date.updateWiggle();
                }

                // ==================================================================
                // 8. Mise à jour de S02Theta
                // ==================================================================
#pragma mark Update S02Theta
                if (AppSettings::mEventModel == EventModelType::EDM2) {
                    if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
                        event->updateS02Theta_v338();
                    }
                }

                // ==================================================================
                // 9. Mise à jour des bornes des phases de l'événement
                // ==================================================================
#pragma mark Update Phases
                std::for_each(
                    event->mPhases.begin(),
                    event->mPhases.end(),
                    [this](std::shared_ptr<Phase> p) {
                        p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                    });
            }
        }

        // ======================================================================
        // 10. Mises à jour globales des phases
        // ======================================================================
        std::for_each(
            mModel->mPhases.begin(),
            mModel->mPhases.end(),
            [this](std::shared_ptr<Phase> p) {
                p->update_Tau(
                    tminPeriod,
                    tmaxPeriod);
            });

        // ======================================================================
        // 11. Mise à jour globale des contraintes de phases
        // ======================================================================
        std::for_each( mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });
    }
    catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
        return;
    }
    catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error " << e.what();
        return;
    }
    catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range " << e.what();
        return;
    }
    catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;
    }
    catch (...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
        return;
    }
}
void MCMCLoopChrono::sampler_339_SingleSite_bloc_delta(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        // ======================================================================
        // 1. Mise à jour de tous les événements
        // ======================================================================
        for (auto &event : events) {

            // ======================================================================
            // 2. Mise à jour de theta uniquement si non fixé
            // ======================================================================
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {

                // ==================================================================
                // 3. Évaluation des bornes de theta
                // ==================================================================
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                    .arg(QString::fromLatin1(__func__),
                         event->getQStringName(),
                         QString::number(min),
                         QString::number(max));
                }

#pragma mark Proposal ti, sigma, delta (Single-Site Block, par date)
                // ==================================================================
                // 4. Initialisation des statistiques pondérées globales
                // ==================================================================
                long double P_curr = 0.0L;
                long double mu_curr = 0.0L;
                long double S_curr = 0.0L;

                for (const auto& date : event->mDates) {
                    const long double sigma = static_cast<long double>(date.mSigmaTi.value());
                    const long double y = static_cast<long double>(date.mTi.value()) + static_cast<long double>(date.mDelta);
                    const long double w = 1.0L / (sigma * sigma);

                    if (P_curr == 0.0L) {
                        P_curr  = w;
                        mu_curr = y;
                        S_curr  = 0.0L;
                    }
                    else {
                        const long double P_old = P_curr;
                        const long double P_new = P_old + w;
                        const long double d = y - mu_curr;
                        const long double mu_new = mu_curr + (w / P_new) * d;

                        S_curr += w * d * (y - mu_new);
                        P_curr  = P_new;
                        mu_curr = mu_new;
                    }
                }

                // ==========================================================================
                // 5. Mise à jour BLOC (ti, delta, sigma) pour chaque date
                // ==========================================================================
#pragma mark mixingKernel
                constexpr double mixingKernel = 0.8; //Default 0.8
                // 0 -> A.1 ti calibré sigma shrinkage ; OK fonctionne ;
                // 1 -> B.2 tout RW: ok fonctionne

                for (size_t i = 0; i < event->mDates.size(); ++i) {

                    auto& date = event->mDates[i];
                    const double ti_old0    = date.mTi.value();
                    const double delta_old0 = date.mDelta;
                    const double sigma_old  = date.mSigmaTi.value();

                    double ti_prop = ti_old0;
                    double sigma_prop = sigma_old;
                    double log_rate_L = 0.0;
                    double log_rate_q_t = 0.0;
                    double log_rate_q_s = 0.0;
                    bool isvalide = true;

                    // ----------------------------------------------------------------------
                    // A. Calcul préalable Leave-One-Out O(1)
                    // (Requis pour le noyau B.2 et l'évaluation rapide de la prior)
                    // ----------------------------------------------------------------------
                    const long double V1_old    = static_cast<long double>(sigma_old) * sigma_old;
                    const long double w_old     = 1.0L / V1_old;
                    const long double y_old     = static_cast<long double>(ti_old0) + delta_old0;

                    const long double P_removed = P_curr - w_old;
                    long double mu_removed = 0.0L;
                    long double S_removed  = 0.0L;

                    if (P_removed > 0.0L) {
                        mu_removed = (P_curr * mu_curr - w_old * y_old) / P_removed;
                        S_removed  = S_curr - w_old * (y_old - mu_curr) * (y_old - mu_removed);
                        if (S_removed < 0.0L && S_removed > -1e-18L) S_removed = 0.0L;
                    } else {
                        mu_removed = y_old;
                    }

                    // ----------------------------------------------------------------------
                    // B. Tirage de delta_prop (indépendant, identique pour les deux noyaux)
                    // ----------------------------------------------------------------------
                    double delta_prop;
                    switch (date.mDeltaType) {
                    case Date::eDeltaNone:     delta_prop = 0.0; break;
                    case Date::eDeltaRange:    delta_prop = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax); break;
                    case Date::eDeltaGaussian: delta_prop = Generator::normalDistribution(date.mDeltaAverage, date.mDeltaError); break;
                    case Date::eDeltaFixed:    delta_prop = date.mDeltaFixed; break;
                    default: delta_prop = delta_old0; break;
                    }

                    // ----------------------------------------------------------------------
                    // C. Bloc de propositions (ti, sigma)
                    // ----------------------------------------------------------------------
                    const double u_kernel = Generator::randomUniform();

                    if (u_kernel < mixingKernel) {
                        // ==============================================================
                        // NOYAU B.2 : Saut Non-Centré (ti) + RW sur log10(V) (sigma)
                        // ==============================================================
#pragma mark NOYAU B.2
                        date.mTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
                        date.mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                        // 1. RW sur log10(V), V = sigma^2
                        const double log10_V1 = std::log10(static_cast<double>(V1_old));
                        const double log10_V2 = Generator::normalDistribution(log10_V1, date.mSigmaTi.mSigmaMH);

                        constexpr double logVMin = -100.0;
                        constexpr double logVMax =  100.0;
                        if (!std::isfinite(log10_V2) || log10_V2 < logVMin || log10_V2 > logVMax) {
                            isvalide = false;
                        } else {
                            const double V2 = std::pow(10.0, log10_V2);
                            sigma_prop = std::sqrt(V2);

                            const double s_z = date.mTi.mSigmaMH;

                            if (P_removed > 0.0L) {
                                // ----------------------------------------------------------------
                                // N >= 2 : ti_bar est un ancrage EXTERNE fixe (Leave-One-Out des
                                // AUTRES dates), indépendant de (ti, sigma) de CETTE date. Le
                                // changement de variable (ti,V)->(z,u) est bien défini, même
                                // ancrage aller/retour => jacobien joint valide.
                                // ----------------------------------------------------------------
                                const double ti_bar = static_cast<double>(mu_removed) - delta_old0;

                                const double z_old  = (ti_old0 - ti_bar) / sigma_old;
                                const double z_prop = Generator::normalDistribution(z_old, s_z);
                                ti_prop = ti_bar + sigma_prop * z_prop;

                                // Marche (z,u) symétrique => Hastings = jacobien joint seul
                                // |J(ti,V)->(z,u)| = 1/(V^{3/2} ln10) => (sigma_prop/sigma_old)^3
                                log_rate_q_t = 0.0;
                                log_rate_q_s = 3.0 * std::log(sigma_prop / sigma_old);


                            } else {
                                // ----------------------------------------------------------------
                                // N == 1 : pas d'ancrage externe. "ti_bar = ti_old0" serait
                                // auto-référent (dépend de l'état lui-même) : ce n'est PAS un
                                // changement de variable valide, l'ancrage bougerait aussi au
                                // retour (ti_bar_retour = ti_prop). Le facteur (sigma_prop/
                                // sigma_old)^3 ne s'applique pas ici (terme résiduel dépendant
                                // de z_prop non compensé -- cf. dérivation).
                                //
                                // Fix : DÉCOUPLER ti de sigma_prop. RW adaptif pur, symétrique,
                                // centré sur ti_old0, échelle fixe s_z (indépendante de sigma) :
                                //   log_rate_q_t = 0 trivialement (RW adaptif symétrique)
                                // sigma garde son propre RW log10(V), mais seul désormais :
                                //   Hastings = (sigma_prop/sigma_old)^2 (jacobien "V seul")
                                // ----------------------------------------------------------------
                                ti_prop = ti_old0 + Generator::normalDistribution(0.0, s_z);

                                log_rate_q_t = 0.0;
                                log_rate_q_s = 2.0 * std::log(sigma_prop / sigma_old);
                               // log_rate_q_s = 2.0 * std::log(sigma_old / sigma_prop);
                            }

                        }
                    }

                    else {
                        // ==============================================================
                        // NOYAU A.1 : Likelihood/Calib (ti) + Shrinkage Uniform (sigma)
                        // ==============================================================
#pragma mark NOYAU A.1
                        date.mTi.mSamplerProposal = SamplerProposal::eLikelihood;
                        date.mSigmaTi.mSamplerProposal = SamplerProposal::ePrior;

                        // 1. Tirage indépendant de V dans le prior shrinkage exact
#pragma mark update sigma
                        const double S02 = event->mS02Theta.value();

                        constexpr double VMin = 1e-12;
                        constexpr double VMax = 1e10;

                        const double V2 = Generator::shrinkageUniforme(S02, VMin, VMax);
                        sigma_prop = std::sqrt(V2);

                        if (sigma_prop <= 0.0) {
                            isvalide = false;
                        } else {
                            // 2. Likelihood sur ti (mélange calib / RW normal)
                            if (Generator::randomUniform() < date.mMixingLevel) {
                                ti_prop = *date.mCalibration->sample_t();
                            } else {
                                const double tminCalib = date.mCalibration->mTmin;
                                const double tmaxCalib = date.mCalibration->mTmax;
                                const double s = std::max((date.mSettings.mTmax - date.mSettings.mTmin),
                                                          tmaxCalib - tminCalib) / 2.0;
                                ti_prop = Generator::normalDistribution(ti_old0, s);
                            }

                            const double q_fwd = date.fProposalDensity(ti_prop, ti_old0);
                            const double q_rev = date.fProposalDensity(ti_old0, ti_prop);

                            if (q_fwd <= 0.0 || q_rev <= 0.0) {
                                isvalide = false;
                            } else {
                                log_rate_q_t = std::log(q_rev) - std::log(q_fwd);

                                // --------------------------------------------------------------
                                // Échantillonneur d'indépendance exact sur V : q(V) = p_V(V).
                                // Rapport MH pour un indépendance sampler :
                                //   [p_V(V2)/p_V(V1)] * [q(V1)/q(V2)]
                                //     = [p_V(V2)/p_V(V1)] * [p_V(V1)/p_V(V2)] = 1
                                // Le prior shrinkage s'annule EXACTEMENT avec la proposition.
                                // Comme log_prior_shrinkage = log(p_V(V2)/p_V(V1)) est ajouté
                                // systématiquement à l'Étape E, on lui oppose ici son opposé
                                // exact pour obtenir une contribution nette nulle.
                                // --------------------------------------------------------------
                                log_rate_q_s = -2.0 * (std::log(S02 + static_cast<double>(V1_old)) - std::log(S02 + V2));
                            }
                        }
                    }

                    // ----------------------------------------------------------------------
                    // D. Vraisemblance Calibration (uniquement pour ti)
                    // ----------------------------------------------------------------------
                    if (isvalide) {
                        const double L_old = date.getLikelihood(ti_old0);
                        const double L_new = date.getLikelihood(ti_prop);

                        if (L_new <= 0.0 || L_old <= 0.0 ) {
                            isvalide = false;
                        } else {
                            log_rate_L = std::log(L_new) - std::log(L_old);
                        }
                    }

                    // ----------------------------------------------------------------------
                    // E. Calcul de la Marginal Prior Conjointe et Acceptation
                    // ----------------------------------------------------------------------
                    if (isvalide) {
                        const long double V2_new = static_cast<long double>(sigma_prop) * sigma_prop;
                        const long double w_new  = 1.0L / V2_new;
                        const long double y_new  = static_cast<long double>(ti_prop) + delta_prop;

                        // Ajout du nouvel état O(1)
                        long double P_new = P_removed, mu_new = mu_removed, S_new = S_removed;
                        if (P_new == 0.0L) {
                            P_new = w_new; mu_new = y_new; S_new = 0.0L;
                        } else {
                            const long double d = y_new - mu_new;
                            P_new  = P_removed + w_new;
                            mu_new = mu_removed + (w_new / P_new) * d;
                            S_new  = S_removed + w_new * d * (y_new - mu_new);
                        }
                        if (S_new < 0.0L && S_new > -1e-18L) S_new = 0.0L;

                        const long double log_sigma_i_ratio = std::log(sigma_prop / static_cast<long double>(sigma_old));
                        long double log_prior_marginal_diff = 0.0L;
                        bool degenerate = false;

                        if (min == max) {
                            const long double theta_fixed  = static_cast<long double>(min);
                            const long double residual_old = y_old - theta_fixed;
                            const long double residual_new = y_new - theta_fixed;

                            log_prior_marginal_diff = -0.5L * (residual_new * residual_new / V2_new
                                                               - residual_old * residual_old / V1_old)
                                                      - log_sigma_i_ratio;
                        } else {
                            const long double sigma_avg_old = 1.0L / std::sqrt(P_curr);
                            const long double sigma_avg_new = 1.0L / std::sqrt(P_new);

                            const long double alpha_old_min = (static_cast<long double>(min) - mu_curr) / sigma_avg_old;
                            const long double alpha_old_max = (static_cast<long double>(max) - mu_curr) / sigma_avg_old;
                            const long double alpha_new_min = (static_cast<long double>(min) - mu_new)  / sigma_avg_new;
                            const long double alpha_new_max = (static_cast<long double>(max) - mu_new)  / sigma_avg_new;

                            const double log_Z_old = log_diff_cdf(static_cast<double>(alpha_old_min), static_cast<double>(alpha_old_max));
                            const double log_Z_new = log_diff_cdf(static_cast<double>(alpha_new_min), static_cast<double>(alpha_new_max));

                            if (!std::isfinite(log_Z_old) || !std::isfinite(log_Z_new)) {
                                degenerate = true;
                            } else {
                                const long double log_sigma_avg_ratio = std::log(sigma_avg_new / sigma_avg_old);
                                log_prior_marginal_diff = static_cast<long double>(log_Z_new - log_Z_old)
                                                          + log_sigma_avg_ratio
                                                          - log_sigma_i_ratio
                                                          - 0.5L * (S_new - S_curr);
                            }
                        }

                        if (degenerate) {
                            date.mTi.reject_update();
                            date.mSigmaTi.reject_update();
                        } else {
                            // Shrinkage de la distribution S02
                            const long double S02_ld = static_cast<long double>(event->mS02Theta.value());
                            const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1_old) - std::log(S02_ld + V2_new));

                            const long double log_rate_total = log_prior_marginal_diff
                                                               + log_prior_shrinkage
                                                               + static_cast<long double>(log_rate_L)
                                                               + static_cast<long double>(log_rate_q_t)
                                                               + static_cast<long double>(log_rate_q_s);

                            if (MHAcceptanceTest_log(static_cast<double>(log_rate_total))) {
                                date.mTi.accept_update(ti_prop);
                                date.mSigmaTi.accept_update(static_cast<double>(sigma_prop));
                                date.mDelta = delta_prop;

                                P_curr = P_new;
                                mu_curr = mu_new;
                                S_curr = S_new;
                            } else {
                                date.mTi.reject_update();
                                date.mSigmaTi.reject_update();
                            }
                        }
                    } else {
                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                    }
                } // Fin de boucle des dates

                // ==========================================================================
                // 5-bis. NOUVEAU (essai) : décalage rigide conjoint de theta et de tous les
                // ti, calé sur sigma_avg = 1/sqrt(P_curr) - même mécanisme que celui qui a
                // fonctionné sur sampler_339_SingleSite_NonCentered. Cible le cas où les
                // dates d'un événement sont mutuellement très informatives (accord fort,
                // sigma petits) : theta et les ti deviennent alors fortement corrélés
                // (effet "funnel", Neal), et les mises à jour une-date-à-la-fois de
                // l'étape 5 ne suffisent plus à les faire bouger efficacement ensemble.
                //
                // Le décalage rigide ti_prop_i = ti_old_i + Δtheta, theta_prop = theta_old
                // + Δtheta laisse EXACTEMENT invariant l'écart (ti-theta)/sigma_i, donc le
                // prior hiérarchique Normal(ti; theta, sigmaTi²) ne contribue rien au ratio
                // d'acceptation : seules la vraisemblance de calibration (absolue en ti) et
                // la borne dure [min,max] de theta interviennent. sigma_avg est l'écart-type
                // EXACT de la loi conditionnelle courante de theta (même quantité que celle
                // utilisée juste après, à l'étape 6, pour le Gibbs de theta) : c'est la
                // bonne échelle pour une exploration locale de la crête theta/ti - aucun
                // nouveau paramètre adaptatif, recalculé à chaque itération à partir des
                // données courantes. Un mélange d'échelle (kWideJumpProb, kWideJumpFactor),
                // toujours symétrique en Δtheta -> aucun terme de Hastings, ajoute une
                // capacité de saut ponctuel plus large pour franchir un désaccord franc
                // entre deux dates, que sigma_avg seul ne peut pas couvrir.
                // ==========================================================================
#pragma mark Décalage rigide theta + ti
                if (min != max && !event->mDates.empty()) {

                    // constexpr double kWideJumpProb   = 0.30; // proportion de "grands" pas

                    // double kWideJumpFactor = 100.0; // facteur d'inflation du pas
                    // const double sigma_avg = 1.0 / std::sqrt(static_cast<double>(P_curr));
                    // alpha = 2; beta = (alpha-1)/P
                    // const double sigma_avg = std::sqrt(Generator::inverseGammaDistribution(2, 1.0 / static_cast<double>(P_curr) ));

                    // alpha = 3; beta = (alpha-1)/P
                    // const double sigma_avg = std::sqrt(Generator::inverseGammaDistribution(4, 3.0 / static_cast<double>(P_curr) ));


                    // const double stepScale = (Generator::randomUniform() < kWideJumpProb)
                    //                             ? kWideJumpFactor * sigma_avg
                    //                           : sigma_avg;
                    // const double deltaTheta = Generator::normalDistribution(0.0, stepScale);

                    const double theta_old  = event->mTheta.value();

                    event->mTheta.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
                    const double deltaTheta = Generator::normalDistribution(0.0, event->mTheta.mSigmaMH);

                    const double theta_prop = theta_old + deltaTheta;

                    bool shiftValide = (theta_prop >= min && theta_prop <= max);

                    long double log_rate_L_shift = 0.0L;
                    std::vector<double> ti_prop_shift;

                    if (shiftValide) {
                        ti_prop_shift.reserve(event->mDates.size());
                        for (const auto& date : event->mDates) {
                            const double ti_prop_i = date.mTi.value() + deltaTheta;
                            ti_prop_shift.push_back(ti_prop_i);

                            const double L_old = date.getLikelihood(date.mTi.value());
                            const double L_new = date.getLikelihood(ti_prop_i);
                            if (!(L_old > 0.0) || !(L_new > 0.0)) {
                                shiftValide = false;
                                break;
                            }
                            log_rate_L_shift += std::log(static_cast<long double>(L_new))
                                              - std::log(static_cast<long double>(L_old));
                        }
                    }

                    if (shiftValide && MHAcceptanceTest_log(static_cast<double>(log_rate_L_shift))) {
                        event->mTheta.accept_update(theta_prop); // regle sigmaMH pour deltaTheta
                        for (size_t k = 0; k < event->mDates.size(); ++k) {
                            event->mDates[k].mTi.setValue(ti_prop_shift[k]); //mise à jour de Ti sans modifier le taux d'acceptation qui sert pour le RW de ti
                        }
                        // Une translation commune de tous les y_i et de mu laisse P_curr
                        // et S_curr invariants (les écarts (y_i - mu) sont inchangés) :
                        // seul mu_curr doit suivre le décalage.
                        mu_curr += static_cast<long double>(deltaTheta);
                    } else {
                        event->mTheta.reject_update(); // regle sigmaMH pour deltaTheta
                    }
                    // sinon : rejet, P_curr/mu_curr/S_curr restent ceux issus de l'étape 5
                }

#pragma mark Update Theta
                // ==================================================================
                // 6. Mise à jour de theta
                // ==================================================================
                if (min == max) {
                    qDebug() << "[ " << __func__ << QString("] Warning for event : %1 : min == max = %2")
                    .arg( event->getQStringName(), QString::number(min));
                    event->mTheta.setValue(min);
                }
                else {
#pragma mark useHMC
                    constexpr bool useHMC = false;   // bascule pour A/B test
                    if (useHMC) {
                        constexpr int L_hmc = 20;
                        const double eps_hmc = (M_PI / 2.0) / L_hmc;  // L*eps ~ pi/2

                        const double new_theta = hmcSampleTheta_temporal(
                                    event->mTheta.value(), mu_curr, P_curr, min, max, L_hmc, eps_hmc);

                        event->mTheta.setValue(new_theta);
                    } else {
                        const double ti_avg_final = static_cast<double>(mu_curr);
                        const double sigma_avg_final = 1.0 / std::sqrt(static_cast<double>(P_curr));
                        const double new_theta = Generator::truncatedNormal(ti_avg_final, sigma_avg_final, min, max);

                        event->mTheta.setValue(new_theta);
                    }


                }

                // ==================================================================
                // 7. Mise à jour des wiggles
                // ==================================================================
#pragma mark Update Wiggle
                for (auto&& date : event->mDates) {
                    date.updateWiggle();
                    date.mWiggle.accept_update(date.mWiggle.value());
                }

                // ==================================================================
                // 8. Mise à jour de S02Theta
                // ==================================================================
#pragma mark Update S02Theta
                if (AppSettings::mEventModel == EventModelType::EDM2) {
                    if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
                        event->updateS02Theta_v338();
                    }
                }

                // ==================================================================
                // 9. Mise à jour des bornes des phases de l'événement
                // ==================================================================
#pragma mark Update Phases
                std::for_each(
                    event->mPhases.begin(),
                    event->mPhases.end(),
                    [this](std::shared_ptr<Phase> p) {
                        p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                    });
            }
        }

        // ======================================================================
        // 10. Mises à jour globales des phases
        // ======================================================================
        std::for_each(
            mModel->mPhases.begin(),
            mModel->mPhases.end(),
            [this](std::shared_ptr<Phase> p) {
                p->update_Tau(
                    tminPeriod,
                    tmaxPeriod);
            });

        // ======================================================================
        // 11. Mise à jour globale des contraintes de phases
        // ======================================================================
        std::for_each( mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });
    }
    catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
        return;
    }
    catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error " << e.what();
        return;
    }
    catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range " << e.what();
        return;
    }
    catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;
    }
    catch (...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
        return;
    }
}
#pragma mark HMC
// ============================================================================
// HMC pour theta, mode "temporel seul" (h_YWI et h_lambda traités comme
// constants -- eq. 12/13 du document "Modèle MCMC unifié"). Cible :
//   p(theta | t, sigma_t) propto N(theta; mu_curr, 1/P_curr) sur [min,max]
// Sert de brique de base + validation avant d'ajouter le gradient de courbe.
// ============================================================================

static inline double gradU_theta_temporal(double theta, double mu, double P)
{
    return P * (theta - mu);
}

static inline void reflectBounds(double& theta, double& p, double min, double max)
{
    for (int guard = 0; guard < 1000; ++guard) {
        if (theta < min) { theta = 2.0 * min - theta; p = -p; }
        else if (theta > max) { theta = 2.0 * max - theta; p = -p; }
        else break;
    }
}

// Un pas HMC complet (L sous-pas de leapfrog) pour theta, mode temporel seul.
double MCMCLoopChrono::hmcSampleTheta_temporal(double theta0, long double mu, long double P,
                                                double min, double max,
                                                int L, double epsilon)
{
    const double mu_d = static_cast<double>(mu);
    const double P_d  = static_cast<double>(P);
    const double mass = P_d;   // masse = précision cible (dynamique "naturelle")

    double theta = theta0;
    double p = Generator::normalDistribution(0.0, std::sqrt(mass));
    const double p0 = p;

    p -= 0.5 * epsilon * gradU_theta_temporal(theta, mu_d, P_d);
    for (int l = 0; l < L; ++l) {
        theta += epsilon * p / mass;
        reflectBounds(theta, p, min, max);
        if (l != L - 1) {
            p -= epsilon * gradU_theta_temporal(theta, mu_d, P_d);
        }
    }
    p -= 0.5 * epsilon * gradU_theta_temporal(theta, mu_d, P_d);
    p = -p;

    auto U = [&](double th) { const double d = th - mu_d; return 0.5 * P_d * d * d; };
    const double H_old = U(theta0) + (p0 * p0) / (2.0 * mass);
    const double H_new = U(theta)  + (p  * p)  / (2.0 * mass);

    if (MHAcceptanceTest_log(-(H_new - H_old))) {
        return theta;
    }
    return theta0;
}
/**
 * @brief Étape MCMC "Single-Site Block" (variante 2) : met à jour conjointement
 *        (t_i, delta_i, sigma_i) pour chaque date de chaque événement, avec theta
 *        marginalisé analytiquement, puis rééchantillonne theta par tirage direct dans sa
 *        loi conditionnelle exacte.
 *
 * @details
 * Variante de `sampler_339_SingleSite_bloc` dans laquelle **sigma_i est toujours** proposé
 * par marche aléatoire adaptative sur u = log10(V), V = sigma_i², quel que soit le noyau
 * choisi pour t_i. Le mélange de noyaux, tiré avec probabilité `mixingKernel` (constante
 * compilée en dur), ne détermine donc plus que la façon dont **t_i** est proposé :
 *
 *  - **Noyau A.1** (Likelihood/Calib, t_i indépendant de sigma) : t_i est proposé soit par
 *    tirage dans la courbe de calibration, soit par une marche gaussienne large. Le
 *    jacobien du changement de variable V -> u = log10(V) (t_i restant inchangé) donne un
 *    facteur de Hastings (sigma_prop/sigma_old)^2, calculé une fois pour toutes en étape C
 *    avant la sélection du noyau.
 *
 *  - **Noyau B.2** (Saut Non-Centré, t_i couplé à sigma_prop) : t_i est reparamétré via un
 *    centre de proposition t_bar (moyenne pondérée Leave-One-Out des AUTRES dates,
 *    ramenée à l'échelle de t_i via delta courant si N>1 ; sinon t_bar = t_i_old, le noyau
 *    dégénérant en marche gaussienne centrée sur t_i_old couplée à sigma_prop) et une
 *    variable réduite z = (t_i - t_bar)/sigma, elle-même proposée par marche gaussienne
 *    symétrique à partir de z_old = (t_i_old - t_bar)/sigma_old. Le jacobien joint du
 *    changement de variable (t_i, V) -> (z, u) ajoute, par rapport au facteur "sigma seul"
 *    déjà appliqué en étape C, un terme supplémentaire +log(sigma_prop/sigma_old), portant
 *    le facteur de Hastings total à (sigma_prop/sigma_old)^3.
 *
 * Dans les deux noyaux, delta_i est tiré indépendamment de sa loi a priori propre (aucune,
 * uniforme, gaussienne ou fixe), ce qui annule sa contribution au ratio de Hastings.
 * Le prior de rétrécissement (Uniform Shrinkage, paramétré par `event->mS02Theta`) est
 * désormais **systématiquement actif** dans le ratio d'acceptation : contrairement à
 * `sampler_339_SingleSite_bloc`, sigma n'est plus jamais tiré par un échantillonneur
 * d'indépendance exact dans ce prior, donc aucun terme ne vient l'annuler.
 *
 * Le ratio de Metropolis-Hastings combine :
 *  - la vraisemblance de calibration (`getLikelihood`) sur t_i,
 *  - le ratio des densités marginales jointes obtenu en intégrant analytiquement theta
 *    (loi a priori uniforme sur [min, max]) : ce terme dépend de la moyenne et de la
 *    précision pondérées de toutes les dates de l'événement (mu_curr, P_curr), maintenues
 *    à jour de façon incrémentale en O(1) par date via une récursion de type Welford
 *    pondérée, avec une opération Leave-One-Out/Add-Back pour isoler la contribution de la
 *    date en cours de mise à jour,
 *  - le prior de rétrécissement sur sigma_i,
 *  - le(s) facteur(s) de Hastings propres au noyau utilisé et au jacobien de la
 *    reparamétrisation log10(V) (cf. ci-dessus).
 *
 * Un cas dégénéré (min == max, borne de theta ponctuelle) traite directement le résidu
 * gaussien sans passer par la marginalisation/troncature. Un autre cas dégénéré (bornes de
 * troncature non finies dans `log_diff_cdf`) rejette systématiquement la proposition. La
 * proposition de sigma (log10(V)) est elle-même invalidée si elle sort de l'intervalle
 * [logVMin, logVMax] ou produit une valeur non finie, auquel cas le noyau n'est même pas
 * sélectionné (`isvalide` court-circuite l'étape D).
 *
 * Après la boucle sur les dates de l'événement :
 *  - theta est rééchantillonné par tirage direct dans sa loi conditionnelle exacte
 *    (gaussienne tronquée sur [min, max], de moyenne/précision données par les statistiques
 *    pondérées courantes mu_curr/P_curr) ;
 *  - les wiggles des dates sont mis à jour (`Date::updateWiggle`) ;
 *  - `S02Theta` est réestimé si le modèle d'événement est EDM2 et que son sampler n'est pas
 *    fixé (`Event::updateS02Theta_v338`) ;
 *  - les bornes alpha/beta des phases associées à l'événement sont mises à jour.
 *
 * Enfin, en dehors de la boucle sur les événements, les Tau des phases et les Gamma des
 * contraintes de phase du modèle sont recalculés globalement.
 *
 * @param[in,out] events Liste des événements du modèle à mettre à jour. Chaque événement
 *        et ses dates (t_i, sigma_i, delta_i) sont modifiés en place ; seuls les
 *        événements dont `mTheta.mSamplerProposal != SamplerProposal::eFixe` sont traités
 *        pour les étapes 2 à 9 (les étapes 10 et 11, globales au modèle, s'appliquent
 *        toujours).
 *
 * @pre Les membres `tminPeriod`, `tmaxPeriod` et `mModel` de `MCMCLoopChrono` doivent être
 *      valides. Chaque `Event` de `events` doit avoir au moins une date, et pour chaque
 *      date `mSigmaTi.value() > 0`.
 *
 * @post Pour chaque événement non fixé : t_i, sigma_i et delta_i de chaque date sont soit
 *       acceptés à leur valeur proposée, soit explicitement rejetés
 *       (`accept_update`/`reject_update`) ; `mTheta` est rééchantillonné ; les wiggles,
 *       `S02Theta` (si EDM2) et les bornes de phase de l'événement sont à jour. Les Tau de
 *       phase et Gamma de contrainte de phase du modèle sont à jour globalement.
 *       `date.mSigmaTi.mSamplerProposal` est systématiquement réglé à `eRWAdaptGauss`
 *       (contrairement à `sampler_339_SingleSite_bloc`, où il pouvait aussi valoir `ePrior`
 *       sous le noyau A.1).
 *
 * @note Les statistiques pondérées (P_curr, mu_curr, S_curr) sont calculées en `long
 *       double` pour limiter l'accumulation d'erreurs numériques sur la récursion
 *       Leave-One-Out/Add-Back, répétée à chaque date et à chaque itération MCMC.
 * @note `mixingKernel` est une constante compilée en dur (`constexpr`, actuellement 0.5)
 *       qui fixe la proportion de tirages passant par le noyau B.2 plutôt que A.1 ; ce
 *       n'est pas un paramètre réglable à l'exécution dans cette version.
 * @note Contrairement à `sampler_339_SingleSite_bloc`, le cas N=1 n'est pas isolé dans une
 *       branche séparée : t_bar retombe simplement sur t_i_old (P_removed == 0), et le
 *       noyau B.2 reste couplé à sigma_prop via z. Le commentaire du code signale
 *       explicitement que ce couplage est "peu efficace avec une seule date".
 *
 * @warning Toute exception levée pendant le traitement d'un événement (y compris la
 *          `QString` levée en cas de `min > max`) est interceptée par les blocs `catch` en
 *          fin de fonction, journalisée via `qWarning`, et provoque un retour immédiat de
 *          la fonction : les événements non encore traités dans la boucle en cours sont
 *          alors silencieusement ignorés pour cette itération.
 *
 * @see Generator::normalDistribution, Generator::truncatedNormal, Date::getLikelihood,
 *      Date::fProposalDensity, Date::updateWiggle, Event::updateS02Theta_v338,
 *      Phase::update_AlphaBeta, Phase::update_Tau, PhaseConstraint::updateGamma,
 *      MHAcceptanceTest_log, log_diff_cdf, sampler_339_SingleSite_bloc (version précédente,
 *      sigma proposé différemment selon le noyau)
 */
// noyau B.2 ne fonctionne pas avec une seule date, résultat instable!!
void MCMCLoopChrono::sampler_339_SingleSite_bloc_2(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        // ======================================================================
        // 1. Mise à jour de tous les événements
        // ======================================================================
        for (auto &event : events) {

            // ======================================================================
            // 2. Mise à jour de theta uniquement si non fixé
            // ======================================================================
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {

                // ==================================================================
                // 3. Évaluation des bornes de theta
                // ==================================================================
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                    .arg(QString::fromLatin1(__func__),
                         event->getQStringName(),
                         QString::number(min),
                         QString::number(max));
                }

#pragma mark Proposal ti, sigma, delta (Single-Site Block, par date)
                // ==================================================================
                // 4. Initialisation des statistiques pondérées globales
                // ==================================================================
                long double P_curr = 0.0L;
                long double mu_curr = 0.0L;
                long double S_curr = 0.0L;

                for (const auto& date : event->mDates) {
                    const long double sigma = static_cast<long double>(date.mSigmaTi.value());
                    const long double y = static_cast<long double>(date.mTi.value()) + static_cast<long double>(date.mDelta);
                    const long double w = 1.0L / (sigma * sigma);

                    if (P_curr == 0.0L) {
                        P_curr  = w;
                        mu_curr = y;
                        S_curr  = 0.0L;
                    }
                    else {
                        const long double P_old = P_curr;
                        const long double P_new = P_old + w;
                        const long double d = y - mu_curr;
                        const long double mu_new = mu_curr + (w / P_new) * d;

                        S_curr += w * d * (y - mu_new);
                        P_curr  = P_new;
                        mu_curr = mu_new;
                    }
                }

                // ==========================================================================
                // 5. Mise à jour BLOC (ti, delta, sigma) pour chaque date
                //
                // sigmaTi est TOUJOURS mis à jour par RW adaptatif sur log10(V), V=sigma².
                // Le noyau (A.1 / B.2) ne détermine que la façon dont ti est proposé :
                //   - A.1 : ti indépendant de sigma (Likelihood/Calib)
                //   - B.2 : ti couplé à sigma (saut non centré z=(ti-ti_bar)/sigma)
                // ==========================================================================
#pragma mark mixingKernel

                constexpr double mixingKernel = 0.8 ; // Default 0.8
                // 0 -> A.1 OK, fonctionne, traite très bien les combinaisons de gaussienne ;
                // 1 -> B.2 OK, fonctionne (peu efficace avec une seule date)

                for (size_t i = 0; i < event->mDates.size(); ++i) {

                    auto& date = event->mDates[i];
                    const double ti_old0    = date.mTi.value();
                    const double delta_old0 = date.mDelta;
                    const double sigma_old  = date.mSigmaTi.value();

                    double ti_prop = ti_old0;
                    double sigma_prop = sigma_old;
                    double log_rate_L = 0.0;
                    double log_rate_q_t = 0.0;
                    double log_rate_q_s = 0.0;
                    bool isvalide = true;


                    // ----------------------------------------------------------------------
                    // A. Calcul préalable Leave-One-Out O(1)
                    // ----------------------------------------------------------------------
                    const long double V1_old    = static_cast<long double>(sigma_old) * sigma_old;
                    const long double w_old     = 1.0L / V1_old;
                    const long double y_old     = static_cast<long double>(ti_old0) + delta_old0;

                    const long double P_removed = P_curr - w_old;
                    long double mu_removed = 0.0L;
                    long double S_removed  = 0.0L;

                    if (P_removed > 0.0L) {
                        mu_removed = (P_curr * mu_curr - w_old * y_old) / P_removed;
                        S_removed  = S_curr - w_old * (y_old - mu_curr) * (y_old - mu_removed);
                        if (S_removed < 0.0L && S_removed > -1e-18L) S_removed = 0.0L;
                    } else {
                        mu_removed = y_old;
                    }

                    // ----------------------------------------------------------------------
                    // B. Tirage de delta_prop (indépendant, identique pour les deux noyaux)
                    // ----------------------------------------------------------------------
                    double delta_prop;
                    switch (date.mDeltaType) {
                    case Date::eDeltaNone:     delta_prop = 0.0; break;
                    case Date::eDeltaRange:    delta_prop = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax); break;
                    case Date::eDeltaGaussian: delta_prop = Generator::normalDistribution(date.mDeltaAverage, date.mDeltaError); break;
                    case Date::eDeltaFixed:    delta_prop = date.mDeltaFixed; break;
                    default: delta_prop = delta_old0; break;
                    }

                    // ----------------------------------------------------------------------
                    // C. Proposition PARTAGÉE de sigma : RW adaptatif sur log10(V)
                    //    (indépendante du noyau choisi pour ti)
                    // ----------------------------------------------------------------------
#pragma mark update sigma (RW commun aux deux noyaux)

                    date.mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
                    const double log10_V1 = std::log10(static_cast<double>(V1_old));
                    const double log10_V2 = Generator::normalDistribution(log10_V1, date.mSigmaTi.mSigmaMH);

                    constexpr double logVMin = -100.0;
                    constexpr double logVMax =  100.0;
                    if (!std::isfinite(log10_V2) || log10_V2 < logVMin || log10_V2 > logVMax) {
                        isvalide = false;
                    } else {
                        const double V2 = std::pow(10.0, log10_V2);
                        sigma_prop = std::sqrt(V2);
                    }

                    // --------------------------------------------------------------
                    // Jacobien de base du changement de variable V -> u=log10(V)
                    // (seul, ti restant inchangé) : dti dV = V ln10 dti du
                    // Hastings = V_prop / V_old  =>  log = 2 log(sigma_prop/sigma_old)
                    // Valable pour A.1. B.2 y ajoutera +log(sigma_prop/sigma_old)
                    // (couplage ti-sigma via z, cf. dérivation).
                    // --------------------------------------------------------------
                    if (isvalide) {
                        log_rate_q_s = 2.0 * std::log(sigma_prop / sigma_old);
                    }

                    // ----------------------------------------------------------------------
                    // D. Proposition de ti selon le noyau choisi
                    // ----------------------------------------------------------------------
                    const double u_kernel = Generator::randomUniform();

                    if (isvalide && u_kernel < mixingKernel) {
                        // ==============================================================
                        // NOYAU B.2 : Saut Non-Centré sur ti (couplé à sigma_prop)
                        // ==============================================================
#pragma mark NOYAU B.2
                        date.mTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                        // Centre de proposition ti_bar : moyenne Leave-One-Out des AUTRES
                        // dates, ramenée à l'échelle de ti via delta courant (ancre fixe,
                        // indépendante de ti et V). Si N=1 (pas d'autre date), ti_bar=ti_old0
                        // et le noyau dégénère en RW gaussien centré sur ti_old.
                        /*const double ti_bar = (P_removed > 0.0L)
                                                  ? static_cast<double>(mu_removed) - delta_old0
                                                  : ti_old0;

                        const double z_old  = (ti_old0 - ti_bar) / sigma_old;
                        const double s_z    = date.mTi.mSigmaMH;
                        const double z_prop = Generator::normalDistribution(z_old, s_z);
                        ti_prop = ti_bar + sigma_prop * z_prop;

                        // --------------------------------------------------------------
                        // Marche (z,u) symétrique => Hastings réduit au jacobien joint
                        // |J(ti,V)->(z,u)| = 1/(V^{3/2} ln10)
                        // => facteur additionnel +log(sigma_prop/sigma_old) par rapport
                        //    au cas "sigma seul" déjà appliqué ci-dessus.
                        // --------------------------------------------------------------
                        log_rate_q_t = 0.0;
                        log_rate_q_s += std::log(sigma_prop / sigma_old);*/

                        //
                       /* const double theta_curr = event->mTheta.value();
                        const double ti_bar     = theta_curr - delta_old0;

                        const double z_old  = (ti_old0 - ti_bar) / sigma_old;
                        const double s_z    = date.mTi.mSigmaMH;
                        const double z_prop = Generator::normalDistribution(z_old, s_z);
                        ti_prop = ti_bar + sigma_prop * z_prop;

                        log_rate_q_t = 0.0;
                        log_rate_q_s += std::log(sigma_prop / sigma_old);*/

                        if (P_removed > 0.0L) {
                            // Cas A : N > 1 (Événement à dates multiples)
                            // L'ancre ti_bar est la moyenne Leave-One-Out (strictement indépendante de ti).
                            const double ti_bar = static_cast<double>(mu_removed) - delta_old0;

                            const double z_old  = (ti_old0 - ti_bar) / sigma_old;
                            const double s_z    = date.mTi.mSigmaMH;
                            const double z_prop = Generator::normalDistribution(z_old, s_z);
                            ti_prop = ti_bar + sigma_prop * z_prop;

                            // Le saut conjoint sur l'espace latent z est symétrique.
                            log_rate_q_t = 0.0;
                            log_rate_q_s += std::log(sigma_prop / sigma_old);

                        } else {
                            // Cas B : N = 1 (Date unique)
                            // Pas de groupe pour centrer ti. Le couplage avec sigma casse
                            // le bilan détaillé. On utilise un simple Random Walk sur ti.
                            const double s_t = date.mTi.mSigmaMH;
                            ti_prop = Generator::normalDistribution(ti_old0, s_t);

                            // La marche aléatoire sur ti est purement symétrique.
                            log_rate_q_t = 0.0;
                            // log_rate_q_s reste intact. Il ne contient que le Jacobien de sigma
                            // calculé à l'étape C (soit 2 * log(sigma_prop / sigma_old)).
                        }

                    }
                    else if (isvalide) {
                        // ==============================================================
                        // NOYAU A.1 : ti via Likelihood/Calib, indépendant de sigma
                        // ==============================================================
#pragma mark NOYAU A.1
                        date.mTi.mSamplerProposal = SamplerProposal::eLikelihood;

                        if (Generator::randomUniform() < date.mMixingLevel) {
                            ti_prop = *date.mCalibration->sample_t();
                        } else {
                            const double tminCalib = date.mCalibration->mTmin;
                            const double tmaxCalib = date.mCalibration->mTmax;
                            const double s = std::max((date.mSettings.mTmax - date.mSettings.mTmin),
                                                      tmaxCalib - tminCalib) / 2.0;
                            ti_prop = Generator::normalDistribution(ti_old0, s);
                        }

                        const double q_fwd = date.fProposalDensity(ti_prop, ti_old0);
                        const double q_rev = date.fProposalDensity(ti_old0, ti_prop);

                        if (q_fwd <= 0.0 || q_rev <= 0.0) {
                            isvalide = false;
                        } else {
                            log_rate_q_t = std::log(q_rev) - std::log(q_fwd);
                            // log_rate_q_s inchangé : reste le jacobien "sigma seul" (2*log(...))
                            // calculé à l'étape C, ti n'y contribue rien de plus ici.
                        }
                    }

                    // ----------------------------------------------------------------------
                    // E. Vraisemblance Calibration (uniquement pour ti)
                    // ----------------------------------------------------------------------
                    if (isvalide) {
                        const double L_old = date.getLikelihood(ti_old0);
                        const double L_new = date.getLikelihood(ti_prop);

                        if (L_new <= 0.0 || L_old <= 0.0 ) {
                            isvalide = false;
                        } else {
                            log_rate_L = std::log(L_new) - std::log(L_old);
                        }
                    }

                    // ----------------------------------------------------------------------
                    // F. Calcul de la Marginal Prior Conjointe et Acceptation
                    // ----------------------------------------------------------------------
                    if (isvalide) {
                        const long double V2_new = static_cast<long double>(sigma_prop) * sigma_prop;
                        const long double w_new  = 1.0L / V2_new;
                        const long double y_new  = static_cast<long double>(ti_prop) + delta_prop;

                        long double P_new = P_removed, mu_new = mu_removed, S_new = S_removed;
                        if (P_new == 0.0L) {
                            P_new = w_new; mu_new = y_new; S_new = 0.0L;
                        } else {
                            const long double d = y_new - mu_new;
                            P_new  = P_removed + w_new;
                            mu_new = mu_removed + (w_new / P_new) * d;
                            S_new  = S_removed + w_new * d * (y_new - mu_new);
                        }
                        if (S_new < 0.0L && S_new > -1e-18L) S_new = 0.0L;

                        const long double log_sigma_i_ratio = std::log(sigma_prop / static_cast<long double>(sigma_old));
                        long double log_prior_marginal_diff = 0.0L;
                        bool degenerate = false;

                        if (min == max) {
                            const long double theta_fixed  = static_cast<long double>(min);
                            const long double residual_old = y_old - theta_fixed;
                            const long double residual_new = y_new - theta_fixed;

                            log_prior_marginal_diff = -0.5L * (residual_new * residual_new / V2_new
                                                               - residual_old * residual_old / V1_old)
                                                      - log_sigma_i_ratio;
                        } else {
                            const long double sigma_avg_old = 1.0L / std::sqrt(P_curr);
                            const long double sigma_avg_new = 1.0L / std::sqrt(P_new);

                            const long double alpha_old_min = (static_cast<long double>(min) - mu_curr) / sigma_avg_old;
                            const long double alpha_old_max = (static_cast<long double>(max) - mu_curr) / sigma_avg_old;
                            const long double alpha_new_min = (static_cast<long double>(min) - mu_new)  / sigma_avg_new;
                            const long double alpha_new_max = (static_cast<long double>(max) - mu_new)  / sigma_avg_new;

                            const double log_Z_old = log_diff_cdf(static_cast<double>(alpha_old_min), static_cast<double>(alpha_old_max));
                            const double log_Z_new = log_diff_cdf(static_cast<double>(alpha_new_min), static_cast<double>(alpha_new_max));

                            if (!std::isfinite(log_Z_old) || !std::isfinite(log_Z_new)) {
                                degenerate = true;
                            } else {
                                const long double log_sigma_avg_ratio = std::log(sigma_avg_new / sigma_avg_old);
                                log_prior_marginal_diff = static_cast<long double>(log_Z_new - log_Z_old)
                                                          + log_sigma_avg_ratio
                                                          - log_sigma_i_ratio
                                                          - 0.5L * (S_new - S_curr);
                            }
                        }

                        if (degenerate) {
                            date.mTi.reject_update();
                            date.mSigmaTi.reject_update();
                        } else {
                            // Prior shrinkage : TOUJOURS actif désormais (plus d'échantillonneur
                            // d'indépendance à annuler, sigma étant proposé par RW dans les deux noyaux).
                            const long double S02_ld = static_cast<long double>(event->mS02Theta.value());
                            const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1_old) - std::log(S02_ld + V2_new));

                            const long double log_rate_total = log_prior_marginal_diff
                                                               + log_prior_shrinkage
                                                               + static_cast<long double>(log_rate_L)
                                                               + static_cast<long double>(log_rate_q_t)
                                                               + static_cast<long double>(log_rate_q_s);

                            if (MHAcceptanceTest_log(static_cast<double>(log_rate_total))) {
                                date.mTi.accept_update(ti_prop);
                                date.mSigmaTi.accept_update(static_cast<double>(sigma_prop));
                                date.mDelta = delta_prop;

                                P_curr = P_new;
                                mu_curr = mu_new;
                                S_curr = S_new;
                            } else {
                                date.mTi.reject_update();
                                date.mSigmaTi.reject_update();
                            }
                        }
                    } else {
                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                    }
                } // Fin de boucle des dates

#pragma mark Update Theta
                // ==================================================================
                // 6. Mise à jour de theta
                // ==================================================================
                if (min == max) {
                    qDebug() << "[ " << __func__ << QString("] Warning for event : %1 : min == max = %2")
                    .arg( event->getQStringName(), QString::number(min));
                    event->mTheta.accept_update(min);
                }
                else {
#pragma mark useHMC
                    constexpr bool useHMC = false;   // bascule pour A/B test
                    if (useHMC) {
                        constexpr int L_hmc = 20;
                        const double eps_hmc = (M_PI / 2.0) / L_hmc;  // L*eps ~ pi/2

                        const double new_theta = hmcSampleTheta_temporal(
                                    event->mTheta.value(), mu_curr, P_curr, min, max, L_hmc, eps_hmc);

                        event->mTheta.accept_update(new_theta);
                    } else {
                        const double ti_avg_final = static_cast<double>(mu_curr);
                        const double sigma_avg_final = 1.0 / std::sqrt(static_cast<double>(P_curr));
                        const double new_theta = Generator::truncatedNormal(ti_avg_final, sigma_avg_final, min, max);

                        event->mTheta.accept_update(new_theta);
                    }
                }

                // ==================================================================
                // 7. Mise à jour des wiggles
                // ==================================================================
#pragma mark Update Wiggle
                for (auto&& date : event->mDates) {
                    date.updateWiggle();
                }

                // ==================================================================
                // 8. Mise à jour de S02Theta
                // ==================================================================
#pragma mark Update S02Theta
                if (AppSettings::mEventModel == EventModelType::EDM2) {
                    if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
                        event->updateS02Theta_v338();
                    }
                }

                // ==================================================================
                // 9. Mise à jour des bornes des phases de l'événement
                // ==================================================================
#pragma mark Update Phases
                std::for_each(
                    event->mPhases.begin(),
                    event->mPhases.end(),
                    [this](std::shared_ptr<Phase> p) {
                        p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                    });
            }
        }

        // ======================================================================
        // 10. Mises à jour globales des phases
        // ======================================================================
        std::for_each(
            mModel->mPhases.begin(),
            mModel->mPhases.end(),
            [this](std::shared_ptr<Phase> p) {
                p->update_Tau(
                    tminPeriod,
                    tmaxPeriod);
            });

        // ======================================================================
        // 11. Mise à jour globale des contraintes de phases
        // ======================================================================
        std::for_each( mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });
    }
    catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
        return;
    }
    catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error " << e.what();
        return;
    }
    catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range " << e.what();
        return;
    }
    catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;
    }
    catch (...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
        return;
    }
}

void MCMCLoopChrono::sampler_339_3v(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        // ======================================================================
        // 1. Mise à jour de tous les événements
        // ======================================================================
        for (auto &event : events) {
            // ======================================================================
            // 2. Mise à jour de theta uniquement si non fixé
            // ======================================================================
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
                // ======================================================================
                // 3. Évaluation des bornes de theta
                // ======================================================================
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                    .arg(QString::fromLatin1(__func__),
                         event->getQStringName(),
                         QString::number(min),
                         QString::number(max));
                }

#pragma mark Proposal ti
                // ======================================================================
                // 4. Proposition des ti avec leurs vraisemblances L
                // ======================================================================
                std::vector<double> old_ti;
                old_ti.reserve(event->mDates.size());
                std::vector<double> prop_ti;
                prop_ti.reserve(event->mDates.size());

                std::vector<double> old_delta;
                old_delta.reserve(event->mDates.size());
                std::vector<double> prop_delta;
                prop_delta.reserve(event->mDates.size());


                double log_rate_L = 0.0; // Rapport de vraisemblance (divisé par T si Annealing)
                double log_rate_q = 0.0; // Ratio de Hastings des proposals de ti

                bool invalid = false;
                for (auto&& date : event->mDates) {
                    const double ti_old = date.mTi.value();
                    old_ti.push_back(ti_old);

                    const double sigma_old = date.mSigmaTi.value();

                    double ti_prop;

                    if (Generator::randomUniform() < date.mMixingLevel) {

                        ti_prop = *date.mCalibration->sample_t();
                        //qDebug() << "Calibration ti proposal ti= " << ti_prop;

                    } else {

                        const double tminCalib = date.mCalibration->mTmin;
                        const double tmaxCalib = date.mCalibration->mTmax;

                        const double s = std::max((date.mSettings.mTmax - date.mSettings.mTmin), tmaxCalib - tminCalib) / 2.0 ;
                        ti_prop = Generator::normalDistribution(ti_old, s);

                        //qDebug() << "Calibration ti mixing ti = " << ti_prop;
                    }

                    prop_ti.push_back(ti_prop);

                    // Vraisemblance
                    const double L_old = date.getLikelihood(ti_old);
                    const double L_new = date.getLikelihood(ti_prop);
                    const double q_fwd = date.fProposalDensity(ti_prop, ti_old);
                    const double q_rev = date.fProposalDensity(ti_old, ti_prop);

                    // Si une proba est nulle, on flag l'invalidité et on stoppe LA BOUCLE DES DATES (break)
                    if (L_new <= 0.0 || L_old <= 0.0 || q_fwd <= 0.0 || q_rev <= 0.0) {
                        invalid = true;
                        break;
                    }

                    log_rate_L += (std::log(L_new) - std::log(L_old));
                    log_rate_q += std::log(q_rev) - std::log(q_fwd);

#pragma mark Update Delta
                    // Proposition de Delta, suivant son Prior, le ratio MH = 1
                    old_delta.push_back(date.mDelta);

                    double delta_prop;

                    switch (date.mDeltaType) {
                    case Date::eDeltaNone: delta_prop = 0.0; break;
                    case Date::eDeltaRange: delta_prop = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax); break;
                    case Date::eDeltaGaussian: delta_prop = Generator::normalDistribution(date.mDeltaAverage, date.mDeltaError); break;
                    case Date::eDeltaFixed: delta_prop = date.mDeltaFixed; break;
                    }
                    prop_delta.push_back(delta_prop);

                }

                if (invalid) {
                    for (auto&& date : event->mDates) {
                        date.mTi.reject_update();

                    }
                    event->mTheta.reject_update();
                    continue; // On passe proprement à l'événement SUIVANT dans events, sans crasher ni tout arrêter.
                }

                // ======================================================================
                // 5. Cas limite : min == max
                // ======================================================================
                if (min == max) {
                    qDebug() << "[ " << __func__ << QString("]  ‼️ Warning for event : %1 : min == max = %2")
                                                        .arg(event->getQStringName(), QString::number(min));

                    // Décision MH basée sur la vraisemblance et le proposal uniquement
                    const double log_rate_total = log_rate_L + log_rate_q;

                    if (event->mTheta.try_update_log(min, log_rate_total)) {
                        for (size_t i = 0; i < event->mDates.size(); ++i) {
                            event->mDates[i].mTi.accept_update(prop_ti[i]);
                            event->mDates[i].mDelta = prop_delta[i];
                        }
                    } else {
                        for (auto&& date : event->mDates) {
                            date.mTi.reject_update();

                        }
                    }
                }
                // ======================================================================
                // 6. Mise à jour par bloc conjoint : (t_i, sigma_ti, delta, theta)
                // ======================================================================
                else {
                    // ======================================================================
                    // 7. Calculs des moyennes et dispersions
                    // ======================================================================
                    double sum_p_old = 0.0;
                    double sum_t_old = 0.0;

                    double sum_p_prop= 0.0;
                    double sum_t_prop = 0.0;

                    for (size_t i = 0 ; i<event->mDates.size(); i++) {

                        const double sigma_old = event->mDates[i].mSigmaTi.value();
                        const double var_old = std::pow(sigma_old, 2.0);
                        sum_t_old += (old_ti[i] + old_delta[i]) / var_old;
                        sum_p_old += 1.0 / var_old;

                        sum_t_prop += (prop_ti[i] + prop_delta[i]) / var_old;


                    }

                    const double sigma_avg_old = 1.0 / std::sqrt(sum_p_old);
                    const double ti_avg_old = sum_t_old / sum_p_old;

                    const double ti_avg_prop = sum_t_prop / sum_p_old;

                    // ======================================================================
                    // 8. Calcul des sommes de carrés S(t)
                    // ======================================================================
                    double S_old = 0.0;
                    double S_prop = 0.0;

                    for (size_t i = 0 ; i<event->mDates.size(); i++) {

                        const double sigma_old = event->mDates[i].mSigmaTi.value();
                        const double var_old = std::pow(sigma_old, 2.0);
                        S_old += std::pow(old_ti[i] + old_delta[i] - ti_avg_old, 2.0) / var_old;

                        S_prop += std::pow(prop_ti[i] + prop_delta[i] - ti_avg_prop, 2.0) / var_old;

                    }

                    // ======================================================================
                    // 9. Calcul logarithmique robuste du ratio de troncature
                    // ======================================================================

                    // Passage en variables centrée réduites pour gaussienne N(0,1)
                    const double alpha_old_min = (min - ti_avg_old) / sigma_avg_old;
                    const double alpha_old_max = (max - ti_avg_old) / sigma_avg_old;

                    const double alpha_prop_min = (min - ti_avg_prop) / sigma_avg_old;
                    const double alpha_prop_max = (max - ti_avg_prop) / sigma_avg_old;

                    // log_diff_cdf() utilise des variables centrées reduites pour gaussienne N(0,1)
                    const double log_Z_old  = log_diff_cdf(alpha_old_min,  alpha_old_max);
                    const double log_Z_prop = log_diff_cdf(alpha_prop_min, alpha_prop_max);

                    const double log_Z_ratio = (log_Z_prop - log_Z_old);
                    // ======================================================================
                    // 10. Log-rate global d'acceptation
                    // ======================================================================
                    const double log_prior_marginal_diff = log_Z_ratio - 0.5 * (S_prop - S_old);


                    const double log_rate_total = log_rate_L + log_rate_q + log_prior_marginal_diff;

                    // ======================================================================
                    // 11. Décision Metropolis-Hastings sur la proposition de conjointe
                    // ======================================================================
                    if (MHAcceptanceTest_log(log_rate_total)) { //Si log_rate < 0.0 : acceptation probabiliste
                        // =====================================================================
                        // SAUT ACCEPTÉ : t_i se déplace vers prop_ti
                        // =====================================================================
                        size_t i = 0;
                        for (auto&& date : event->mDates) {
                            date.mTi.accept_update(prop_ti[i]);
                            date.mDelta = prop_delta[i++];
                        }

#pragma mark Update Theta
                        // Échantillonnage de Gibbs exact pour theta sachant le NOUVEL état t_prop
                        // (Ce tirage est TOUJOURS valide et ne nécessite pas de test MH)
                        const double new_theta = Generator::truncatedNormal(ti_avg_prop, sigma_avg_old, min, max);
                        event->mTheta.accept_update(new_theta);

                    } else {
                        // =====================================================================
                        // SAUT REFUSÉ : t_i conserve sa valeur old_ti
                        // =====================================================================
                        for (auto&& date : event->mDates) {
                            date.mTi.reject_update();
                        }
                        event->mTheta.reject_update();
                    }
                }

                // ======================================================================
                // 12. Mise à jour des hyperparamètres, wiggles et EDM2 pour l'événement courant
                // ======================================================================

#pragma mark Update Wiggle
                for (auto&& date : event->mDates) {
                    //date.updateSigmaShrinkage_K(event->mTheta.value(), event->mS02Theta.value());
                    date.updateSigma_Log10(event->mTheta.value(), event->mS02Theta.value());
                    date.updateWiggle();
                }

#pragma mark Update S02Theta
                if (AppSettings::mEventModel == EventModelType::EDM2) {
                    if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
                        event->updateS02Theta_v338();
                }

                // ======================================================================
                // 13. Mise à jour des bornes des phases de l'événement
                // ======================================================================
                std::for_each(event->mPhases.begin(), event->mPhases.end(),
                              [this](std::shared_ptr<Phase> p) {
                                  p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                              });
            }
        }

        // ======================================================================
        // 14. Mises à jour globales des phases et contraintes du modèle
        // ======================================================================
        std::for_each(mModel->mPhases.begin(), mModel->mPhases.end(),
                      [this](std::shared_ptr<Phase> p) {
                          p->update_Tau(tminPeriod, tmaxPeriod);
                      });

        std::for_each(mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });

    } catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
        return;

    } catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error" << e.what();
        return;

    } catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range" << e.what();
        return;

    } catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;

    } catch(...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
        return;
    }
}

void MCMCLoopChrono::sampler_339_Couple(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        // ======================================================================
        // 1. Mise à jour de tous les événements
        // ======================================================================
        for (auto &event : events) {

            // ======================================================================
            // 2. Mise à jour de theta uniquement si non fixé
            // ======================================================================
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {

                // ==================================================================
                // 3. Évaluation des bornes de theta
                // ==================================================================
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                    .arg(QString::fromLatin1(__func__),
                         event->getQStringName(),
                         QString::number(min),
                         QString::number(max));
                }

#pragma mark Proposal ti, sigma, delta (Single-Site, par date)


                // ==================================================================
                // 5. Mise à jour single-site de chaque date
                // ==================================================================
                for (size_t i = 0; i < event->mDates.size(); ++i) {

                    auto& date = event->mDates[i];

                    const double ti_old = date.mTi.value();
                    const double sigma_old = date.mSigmaTi.value();
                    const double delta_old = date.mDelta;

                    const long double V1 = static_cast<long double>(sigma_old) * static_cast<long double>(sigma_old);

                    const long double w_old = 1.0L / V1;

                    const long double y_old = static_cast<long double>(ti_old) + static_cast<long double>(delta_old);

#pragma mark ti
                    // ==================================================================
                    // 5.1 Proposition de ti
                    // ==================================================================
                    double ti_prop = ti_old;

                    if (Generator::randomUniform() < date.mMixingLevel) {

                        ti_prop =* date.mCalibration->sample_t();
                    }
                    else {

                        const double tminCalib = date.mCalibration->mTmin;
                        const double tmaxCalib = date.mCalibration->mTmax;
                        const double s = std::max( (date.mSettings.mTmax - date.mSettings.mTmin),
                                                  tmaxCalib - tminCalib)
                                         / 2.0;

                        ti_prop = Generator::normalDistribution(ti_old, s);
                    }

                    const double L_old = date.getLikelihood(ti_old);
                    const double L_new = date.getLikelihood(ti_prop);
                    const double q_fwd = date.fProposalDensity( ti_prop, ti_old);
                    const double q_rev = date.fProposalDensity(ti_old, ti_prop);

                    if (L_new <= 0.0 ||
                        L_old <= 0.0 ||
                        q_fwd <= 0.0 ||
                        q_rev <= 0.0) {

                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                        continue;
                    }

                    // Rapport local pour la proposition de ti
                    const double log_rate_L = std::log(L_new) - std::log(L_old);
                    const double log_rate_q = std::log(q_rev) - std::log(q_fwd);

#pragma mark Update Delta
                    // ==================================================================
                    // 5.2 Proposition de delta
                    // ==================================================================
                    double delta_prop = delta_old;

                    switch (date.mDeltaType) {

                    case Date::eDeltaNone:
                        delta_prop = 0.0;
                        break;

                    case Date::eDeltaRange:
                        delta_prop = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax);
                        break;

                    case Date::eDeltaGaussian:
                        delta_prop = Generator::normalDistribution( date.mDeltaAverage, date.mDeltaError);
                        break;

                    case Date::eDeltaFixed:
                        delta_prop = date.mDeltaFixed;
                        break;

                    default:
                        delta_prop = delta_old;
                        break;
                    }

#pragma mark Update sigma
                    // ==================================================================
                    // 5.3 Proposition de sigma
                    // ==================================================================
                    const double S02 = event->mS02Theta.value();

                    date.mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                    //const double log10_V1 = std::log10(V1);

                    // Cas A : sigma proposal inv-gamma alpha=1 , beta = (ti -theta)^2
                    // const double u = Generator::randomUniform();
                    // const double diff = ti_prop + delta_prop - event->mTheta.value();
                   // const double V2 = -0.5 * diff * diff / std::log(u);

                    // Cas B : uniforme autour de diff

                    // const double diff = ti_prop + delta_prop - event->mTheta.value();
                    // constexpr double k = 0.5;
                    // const double V2 = Generator::randomUniform(diff*diff*k, (1+k) * diff*diff);

                    // const double log10_V2_double = std::log10(V2);

                    // cas C:
                    //const double diff = ti_prop + delta_prop - event->mTheta.value();
                    //const double log10_V1 = std::log10(diff*diff);
                    //const double log10_V2_double = Generator::normalDistribution(log10_V1, date.mSigmaTi.mSigmaMH);

                    //const long double V2 = std::pow(  10.0L, static_cast<long double>(log10_V2_double));

                    // cas D: Echantillonneur gaussien adaptatif

                     const double log10_V1 = std::log10(V1);
                     const double log10_V2_double = Generator::normalDistribution(log10_V1, date.mSigmaTi.mSigmaMH);

                     const long double V2 = std::pow(  10.0L, static_cast<long double>(log10_V2_double));

                    // cas E: Echantillonneur Shrinkage(S02)
                    // const long double V2 = Generator::shrinkageUniforme(S02);
                    // const double log10_V2_double = std::log10(V2);

                    // cas F: Echantillonneur Shrinkage(diff^2)
                    // const double diff = ti_prop + delta_prop - event->mTheta.value();

                    // const long double V2 = Generator::shrinkageUniforme(diff*diff);
                    // const double log10_V2_double = std::log10(V2);
                    // ---


                   // std::cout << date.name() << " sigmaTi=" << date.mSigmaTi.value() << " date.mSigmaTi.mSigmaMH=" << date.mSigmaTi.mSigmaMH << std::endl;
                    const double logVMin = -100.0;
                    const double logVMax =  100.0;

                    // Proposition en dehors du support autorisé :
                    // on rejette simplement cette date.
                    if (!std::isfinite(log10_V2_double) ||
                        log10_V2_double < logVMin ||
                        log10_V2_double > logVMax) {

                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                        continue;
                    }


                    if (!(V2 > 0.0L) ||
                        !std::isfinite(V2)) {

                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                        continue;
                    }

                    const long double sigma_prop = std::sqrt(V2);


                    if (min == max) {
                        // ==============================================================
                        // theta fixé exactement
                        // ==============================================================

                        const long double theta_fixed = static_cast<long double>(min);

                       /* const long double residual_old = y_old - theta_fixed;

                        const long double residual_prop = y_prop - theta_fixed;

                        const long double log_like_diff = -0.5L
                                                              * (residual_prop * residual_prop / V2
                                                                 - residual_old * residual_old / V1 )
                                                          - log_sigma_i_ratio;

                        log_prior_marginal_diff = log_like_diff;*/
                    }
                    else {

                        // ==============================================================
                        // theta marginalisé analytiquement
                        // ==============================================================

                       /* const long double alpha_old_min = (static_cast<long double>(min) - ti_avg_old) / sigma_avg_old;

                        const long double alpha_old_max = (static_cast<long double>(max) - ti_avg_old) / sigma_avg_old;

                        const long double alpha_prop_min = (static_cast<long double>(min) - ti_avg_prop) / sigma_avg_prop;

                        const long double alpha_prop_max = (static_cast<long double>(max) - ti_avg_prop) / sigma_avg_prop;

                        const double log_Z_old = log_diff_cdf(static_cast<double>(alpha_old_min), static_cast<double>(alpha_old_max));
                        const double log_Z_prop = log_diff_cdf(static_cast<double>(alpha_prop_min), static_cast<double>(alpha_prop_max));

                        // Une probabilité de troncature est strictement
                        // positive pour un intervalle de largeur non nulle.
                        // Si le calcul numérique retourne -inf/NaN,
                        // on rejette la proposition.
                        if (!std::isfinite(log_Z_old) ||
                            !std::isfinite(log_Z_prop)) {

                            date.mTi.reject_update();
                            date.mSigmaTi.reject_update();
                            continue;
                        }*/

                        /*const long double log_sigma_avg_ratio = std::log(sigma_avg_prop / sigma_avg_old);

                        const long double log_Z_ratio = static_cast<long double>(log_Z_prop)
                                                        - static_cast<long double>(log_Z_old)
                                                        + log_sigma_avg_ratio;

                        log_prior_marginal_diff = log_Z_ratio
                                                  - log_sigma_i_ratio
                                                  - 0.5L * (S_prop - S_old);*/
                    }

                    // ==================================================================
                    // 5.9 Priors de sigma
                    // ==================================================================

                    // Prior p(sigma) ∝ 1/sigma
                    // const long double log_prior_sigma = 0.5L * (std::log(V1) - std::log(V2));

                    // Prior shrinkage :
                    //
                    //     p(V) ∝ 1 / (S02 + V)^2
                    //
                    const long double S02_ld = static_cast<long double>(S02);

                    // cas A :
                    // const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1) - std::log(S02_ld + V2));
                    // const long double Hastings_sigma = std::pow(sigma_prop/sigma_old, 3)
                    //                                  * std::pow( (ti_old - event->mTheta.value())
                    //                                              /(ti_prop- event->mTheta.value()), 2 );
                    //
                    // const long double log_hastings_sigma = std::log(Hastings_sigma);

                    // CAS B :
                    // const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1) - std::log(S02_ld + V2));
                    // const long double Hastings_sigma = std::pow(
                    //    (ti_prop + delta_prop - event->mTheta.value())
                    //        /(ti_old + delta_old - event->mTheta.value())
                    //    , 2 );
                    // const long double log_hastings_sigma = std::log(Hastings_sigma);

                    // CAS C :
                    // const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1) - std::log(S02_ld + V2));
                    //const double diff_prop = ti_prop + delta_prop - event->mTheta.value();
                    //const double diff_old = ti_old + delta_old - event->mTheta.value();

                    //const long double log_hastings_sigma = log_dnorm(sigma_prop*sigma_prop, diff_prop*diff_prop, date.mSigmaTi.mSigmaMH )
                    //                                       -log_dnorm(sigma_old*sigma_old, diff_old*diff_old, date.mSigmaTi.mSigmaMH );

                    // CAS D : Adaptatif Gaussien
                     const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1) - std::log(S02_ld + V2));
                     long double log_hastings_sigma = 0.;
                        // plus jacobien
                     log_hastings_sigma =+ std::log(V2)-std::log(V1);

                    // CAS E : Shrinkage uniforme(S02)
                    // const long double log_prior_shrinkage = 0.;
                    // long double log_hastings_sigma = 0.;

                    // CAS F : Shrinkage uniforme(diff^2)
                    // const double diff_prop = ti_prop + delta_prop - event->mTheta.value();
                    // const double diff_old = ti_old + delta_old - event->mTheta.value();

                    // const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1) - std::log(S02_ld + V2));

                    // const long double log_hastings_sigma = 2.0L * (std::log(diff_old*diff_old + V1) - std::log(diff_prop*diff_prop + V2));


                    // model Event
                    double log_E_old = log_dnorm(ti_old+delta_old, event->mTheta.value(), sigma_old );
                    double log_E_prop = log_dnorm(ti_prop + delta_prop, event->mTheta.value(), sigma_prop );


                    // ==================================================================
                    // 5.11 Ratio global pour sigma
                    // ==================================================================

                    const long double log_rate_sigma = log_prior_shrinkage
                                                       + log_E_prop - log_E_old
                                                       + log_hastings_sigma;
                    // ==================================================================
                    // 5.12 Log-rate global d'acceptation
                    // ==================================================================
                    const long double log_rate_total = static_cast<long double>(log_rate_L)
                                                       + static_cast<long double>(log_rate_q)
                                                       + log_rate_sigma;

                    if (MHAcceptanceTest_log(static_cast<double>(log_rate_total))) {

                        // ==============================================================
                        // Acceptation
                        // ==============================================================

                        date.mTi.accept_update(ti_prop);

                        date.mSigmaTi.accept_update(static_cast<double>(sigma_prop));

                        date.mDelta = delta_prop;

                    }
                    else {

                        // ==============================================================
                        // Rejet
                        // ==============================================================

                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                    }
                }

#pragma mark Update Theta
                // ==================================================================
                // 4. Initialisation des statistiques pondérées
                //
                //    y_i = t_i + delta_i
                //    w_i = 1 / sigma_i²
                //
                //    P     = sum(w_i)
                //    mu    = sum(w_i y_i) / P
                //    S     = sum(w_i (y_i - mu)²)
                //
                //    IMPORTANT :
                //    on n'utilise PAS :
                //
                //       Q - T²/P
                //
                //    afin d'éviter une forte annulation numérique lorsque
                //    les sigma_i sont petits.
                // ==================================================================
                long double P_curr = 0.0L;
                long double mu_curr = 0.0L;
                long double S_curr = 0.0L;

                for (const auto& date : event->mDates) {

                    const long double sigma = static_cast<long double>(date.mSigmaTi.value());

                    const long double y =
                        static_cast<long double>(date.mTi.value())
                        + static_cast<long double>(date.mDelta);

                    const long double w = 1.0L / (sigma * sigma);

                    if (P_curr == 0.0L) {
                        P_curr  = w;
                        mu_curr = y;
                        S_curr  = 0.0L;
                    }
                    else {
                        // Weighted Welford update
                        const long double P_old = P_curr;
                        const long double mu_old = mu_curr;

                        const long double P_new =
                            P_old + w;

                        const long double d =
                            y - mu_old;

                        const long double mu_new =
                            mu_old + (w / P_new) * d;

                        S_curr +=
                            w * d * (y - mu_new);

                        P_curr  = P_new;
                        mu_curr = mu_new;
                    }
                }
                // ==================================================================
                // 6. Mise à jour de theta
                //
                //     theta | dates ~ TruncatedNormal(mu, sigma_theta, min, max)
                //
                //     mu         = T/P = mu_curr
                //     sigma_theta = 1/sqrt(P)
                //
                //     Tirage exact Gibbs.
                // ==================================================================
                if (min == max) {

                    qDebug() << "[ " << __func__ << QString("] Warning for event : %1 : min == max = %2")
                    .arg( event->getQStringName(), QString::number(min));

                    event->mTheta.accept_update(min);
                }
                else {

                    const double ti_avg_final =
                        static_cast<double>(mu_curr);

                    const double sigma_avg_final = 1.0 / std::sqrt( static_cast<double>(P_curr));

                    const double new_theta = Generator::truncatedNormal(ti_avg_final, sigma_avg_final, min, max);

                    event->mTheta.accept_update(new_theta);
                }

                // ==================================================================
                // 7. Mise à jour des wiggles
                // ==================================================================
#pragma mark Update Wiggle
                for (auto&& date : event->mDates) {
                    date.updateWiggle();
                }

                // ==================================================================
                // 8. Mise à jour de S02Theta
                // ==================================================================
#pragma mark Update S02Theta
                if (AppSettings::mEventModel == EventModelType::EDM2) {

                    if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
                        event->updateS02Theta_v338();
                    }
                }

                // ==================================================================
                // 9. Mise à jour des bornes des phases de l'événement
                // ==================================================================
#pragma mark Update Phases
                std::for_each(
                    event->mPhases.begin(),
                    event->mPhases.end(),
                    [this](std::shared_ptr<Phase> p) {

                        p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                    });
            }
        }

        // ======================================================================
        // 10. Mises à jour globales des phases
        // ======================================================================
        std::for_each(
            mModel->mPhases.begin(),
            mModel->mPhases.end(),
            [this](std::shared_ptr<Phase> p) {

                p->update_Tau(
                    tminPeriod,
                    tmaxPeriod);
            });

        // ======================================================================
        // 11. Mise à jour globale des contraintes de phases
        // ======================================================================
        std::for_each( mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });
    }
    catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
        return;
    }
    catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error " << e.what();
        return;
    }
    catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range " << e.what();
        return;
    }
    catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;
    }
    catch (...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
        return;
    }

}

/**
 * @brief Uncollapsed joint Metropolis-Hastings block sampler for (t_i, sigma_i, theta).
 *
 * @details Unlike sampler_339_4v, this sampler does NOT analytically
 * marginalize theta out of the acceptance ratio (no "collapsed"/Rao-Blackwellized step).
 * Instead it performs a single joint Metropolis-Hastings test on the full block
 * (t_1..t_n, sigma_1..sigma_n, theta), built by sequential composition of proposals:
 *
 *   1) t_i'     ~ mixture( calibrated density , N(t_i, s) )   [random walk / calib mix]
 *   2) sigma_i' ~ Shrink(s0^2) truncated on [VMin, VMax]      [== prior -> cancels]
 *   3) theta'   ~ N( ti_avg(t', sigma'), sigma_theta(t', sigma') )   [NOT truncated]
 *
 * The full joint log-posterior (measurement likelihood + structural Gaussian
 * t_i|theta,sigma_i, per eq. (1)-(3) of Lanos & Philippe, 2015) is evaluated at both
 * the old and proposed states via evaluate_log_posterior(), and combined with the
 * Hastings correction for the t_i and theta proposals to form log_rate_total.
 *
 * @note On the sigma_i prior cancellation: sigma_i is proposed directly from its own
 * prior (Shrink(s0^2), same VMin/VMax/S02 used for both the proposal and the implicit
 * prior term), so the prior/proposal ratio is exactly 1 and is intentionally omitted
 * from both evaluate_log_posterior() and log_rate_q. Re-expressing the prior in terms
 * of sigma instead of variance V2 = sigma^2 would introduce a Jacobian factor
 * (2*sigma), but since the SAME function plays both the proposal and the reference-prior
 * role, that factor would appear identically in numerator and denominator and cancel;
 * it is therefore correctly left out rather than computed and cancelled explicitly.
 *
 * @note On the theta reverse proposal density (q_rev_theta): because t_i and sigma_i
 * are drawn independently of the current state (proposal == prior, so their density does
 * not depend on where the chain currently is), the reverse move's t,sigma draws have the
 * same density regardless of direction. The conditional theta proposal, however, DOES
 * depend on whichever (t, sigma) values it is conditioned on. Forward: theta' is centered
 * on ti_avg computed from the newly drawn (t_prop, sigma_prop). Reverse: returning to
 * old_theta requires evaluating the theta-proposal density conditioned on (t_old,
 * sigma_old) — i.e. dnorm(old_theta, ti_avg_old, sigma_theta_old) — NOT on (t_prop,
 * sigma_prop). This asymmetry is what makes the block proposal genuinely non-symmetric
 * and is the main point to get right in this scheme.
 *
 * @note Efficiency vs. sampler_339_4v : this sampler is intentionally
 * "naive" in two respects — (1) theta is proposed from an UNtruncated normal and
 * rejected outright whenever it falls outside [min, max], wasting proposals that the
 * truncated-normal Gibbs draw in the collapsed samplers never wastes; (2) theta is not
 * analytically marginalized out of the acceptance ratio, so the full per-date
 * log-likelihood must be recomputed for both the old and proposed states on every
 * iteration rather than reusing a closed-form marginal. Mathematically valid, but
 * expected to mix more slowly and cost more per iteration than the collapsed versions.
 *
 * @param events The list of Events to update. For each event whose theta is not fixed,
 *               proposes and jointly accepts/rejects (t_i, sigma_i, theta), then updates
 *               delta (Gibbs), wiggle, S02Theta (EDM2) and phase bounds as usual.
 *
 * @see Lanos, P. and Philippe, A. (2015). Event model: a robust Bayesian tool for
 *      chronological modeling, eq. (1)-(4), for the target hierarchical model.
 * @see MCMCLoopChrono::sampler_339_2v, MCMCLoopChrono::sampler_339_3v for the collapsed
 *      (Rao-Blackwellized) alternative that marginalizes theta analytically.
 */

void MCMCLoopChrono::sampler_naif(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        for (auto &event : events) {
            if (event->mTheta.mSamplerProposal == SamplerProposal::eFixe) {
                continue;
            }

            // ======================================================================
            // 1. Évaluation des bornes de theta
            // ======================================================================
            const double min = event->getThetaMin(tminPeriod);
            const double max = event->getThetaMax(tmaxPeriod);

            if (min > max) {
                throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                .arg(QString::fromLatin1(__func__),
                     event->getQStringName(),
                     QString::number(min),
                     QString::number(max));
            }

            // ======================================================================
            // 2. Sauvegarde de l'état INITIAL (OLD)
            // ======================================================================
            const double old_theta = event->mTheta.value();

            std::vector<double> old_ti;
            std::vector<double> old_sigmaTi;
            old_ti.reserve(event->mDates.size());
            old_sigmaTi.reserve(event->mDates.size());

            for (auto&& date : event->mDates) {
                old_ti.push_back(date.mTi.value());
                old_sigmaTi.push_back(date.mSigmaTi.value());
            }

            // ======================================================================
            // 3. PROPOSAL : Tirage des ti et sigmaTi + Calcul du ratio q
            // ======================================================================
            std::vector<double> prop_ti;
            std::vector<double> prop_sigmaTi;
            prop_ti.reserve(event->mDates.size());
            prop_sigmaTi.reserve(event->mDates.size());

            double log_rate_q = 0.0;
            bool invalid = false;

            size_t idx = 0;
            const double S02 = event->mS02Theta.value();

            for (auto&& date : event->mDates) {
                const double ti_old = old_ti[idx];
                double ti_prop;

                // Tirage de ti (Vraisemblance / Normal)
                if (Generator::randomUniform() < date.mMixingLevel) {
                    const double tminCalib = date.mCalibration->mTmin;
                    const double u = Generator::randomUniform();
                    const double idxCalib = interpolate_index(u, date.mCalibration->mRepartition);
                    ti_prop = tminCalib + idxCalib * date.mCalibration->mStep;
                } else {
                    const double s = (date.mSettings.mTmax - date.mSettings.mTmin) / 2.0;
                    ti_prop = Generator::normalDistribution(ti_old, s);
                }

                // ======================================================================
                // 9. Mise à jour des sigmas
                // ======================================================================
#pragma mark Update Sigma
                constexpr double VMin = 1e-12;
                constexpr double VMax = 1e10;
                const double V2 = Generator::shrinkageUniforme(S02, VMin, VMax);

                double sigma_prop = std::sqrt(V2);


                const double L_old = date.getLikelihood(ti_old);
                const double L_new = date.getLikelihood(ti_prop);

                const double q_fwd_t = date.fProposalDensity(ti_prop, ti_old);
                const double q_rev_t = date.fProposalDensity(ti_old, ti_prop);

                // inutile ici, car on utilise le Prior de Sigma
                //const double q_fwd_sigma = date.mSigmaTi.fProposalDensity(sigmaNew, sigma0);
                //const double q_rev_sigma = date.mSigmaTi.fProposalDensity(sigma0, sigmaNew);

                //if (L_new <= 0.0 || L_old <= 0.0 || q_fwd_t <= 0.0 || q_rev_t <= 0.0 || q_fwd_sigma <= 0.0 || q_rev_sigma <= 0.0) {
                if (L_new <= 0.0 || L_old <= 0.0 || q_fwd_t <= 0.0 || q_rev_t <= 0.0) {
                    invalid = true;
                    break;
                }

                prop_ti.push_back(ti_prop);
                prop_sigmaTi.push_back(sigma_prop);

                log_rate_q += (std::log(q_rev_t) - std::log(q_fwd_t));// + (std::log(q_rev_sigma) - std::log(q_fwd_sigma));
                idx++;
            }

            if (invalid) {
                for (auto&& d : event->mDates) {
                    d.mTi.reject_update();
                    d.mSigmaTi.reject_update();
                }
                event->mTheta.reject_update();
                continue;
            }

            // ======================================================================
            // 4. PROPOSAL : Tirage de theta par Loi Normale standard (SANS Troncature)
            // ======================================================================
            double sum_p_prop = 0.0;
            double sum_t_prop = 0.0;
            idx = 0;
            for (auto&& date : event->mDates) {
                const double var = std::pow(prop_sigmaTi[idx], 2.0);
                sum_t_prop += (prop_ti[idx] + date.mDelta) / var;
                sum_p_prop += 1.0 / var;
                idx++;
            }

            const double sigma_theta_prop = 1.0 / std::sqrt(sum_p_prop);
            const double ti_avg_prop = sum_t_prop / sum_p_prop;

            // Proposition de theta via la loi normale sans tronquer
            const double prop_theta = Generator::normalDistribution(ti_avg_prop, sigma_theta_prop);

            // Rejet immédiat si prop_theta est hors du support [min, max]
            if (prop_theta < min || prop_theta > max) {
                for (auto&& d : event->mDates) {
                    d.mTi.reject_update();
                    d.mSigmaTi.reject_update();
                }
                event->mTheta.reject_update();
                continue;
            }

            // Densités de proposition gaussiennes standards (sans terme de normalisation CDF)
            const double q_fwd_theta = dnorm(prop_theta, ti_avg_prop, sigma_theta_prop);

            // Densité reverse
            double sum_p_old = 0.0;
            double sum_t_old = 0.0;
            idx = 0;
            for (auto&& date : event->mDates) {
                const double var = std::pow(old_sigmaTi[idx], 2.0);
                sum_t_old += (old_ti[idx] + date.mDelta) / var;
                sum_p_old += 1.0 / var;
                idx++;
            }
            const double sigma_theta_old = 1.0 / std::sqrt(sum_p_old);
            const double ti_avg_old = sum_t_old / sum_p_old;

            const double q_rev_theta = dnorm(old_theta, ti_avg_old, sigma_theta_old);

            log_rate_q += std::log(q_rev_theta) - std::log(q_fwd_theta);

            // Lambda helper : Evaluation du Log-Posterior Conjoint p(t, theta, sigma)
            auto evaluate_log_posterior = [&](const std::vector<double>& ti_vec,
                                              const std::vector<double>& sigma_vec,
                                              double theta_val) -> double
            {
                if (theta_val < min || theta_val > max) {
                    return -std::numeric_limits<double>::infinity();
                }

                double log_post = 0.0;
                size_t i = 0;
                for (auto&& date : event->mDates) {
                    const double ti = ti_vec[i];
                    const double sig = sigma_vec[i];
                    const double var = sig * sig;

                    // 1. Vraisemblance de la date C14 / calibration : L(ti)
                    log_post += std::log(date.getLikelihood(ti));

                    // 2. Vraisemblance conditionnelle du modèle structural : ti ~ N(theta - delta, sig^2)
                    const double diff = ti + date.mDelta - theta_val;
                    log_post += -std::log(sig) - 0.5 * (diff * diff) / var;

                    // 3. Prior sur sigma : S'annule car proposal == prior !
                    // Aucune ligne supplémentaire nécessaire pour sigma_i.

                    // Si on veut quand même mettre le calcul, quan on utilise un autre proposal
                    // Sur la variance V2 = sig * sig :
                    //const double s02 = event->mS02Theta.value();
                    //const double log_prior_sigma = std::log(s02) - 2.0 * std::log(s02 + var);

                    // Si exprimé par rapport à sig (avec Jacobien 2*sig) :
                    // log_prior_sigma = std::log(2.0 * sig) + std::log(s02) - 2.0 * std::log(s02 + var);

                    i++;
                }

                return log_post;
            };
            // ======================================================================
            // 5. Évaluation des Conditionnelles Totales (OLD vs PROP)
            // ======================================================================
            const double log_post_old  = evaluate_log_posterior(old_ti, old_sigmaTi, old_theta);
            const double log_post_prop = evaluate_log_posterior(prop_ti, prop_sigmaTi, prop_theta);

            const double log_rate_total = (log_post_prop - log_post_old) + log_rate_q;

            // ======================================================================
            // 6. Décision Metropolis-Hastings et Mise à jour
            // ======================================================================
            if (MHAcceptanceTest_log(log_rate_total)) {
                size_t i = 0;
                for (auto&& date : event->mDates) {
                    date.mTi.accept_update(prop_ti[i]);
                    date.mSigmaTi.accept_update(prop_sigmaTi[i]);
                    i++;
                }
                event->mTheta.accept_update(prop_theta);
            } else {
                for (auto&& date : event->mDates) {
                    date.mTi.reject_update();
                    date.mSigmaTi.reject_update();
                }
                event->mTheta.reject_update();
            }

            // ======================================================================
            // 7. Mises à jour secondaires
            // ======================================================================
            for (auto&& date : event->mDates) {
                date.updateDelta(event->mTheta.value(), event->mS02Theta.value()); // Gibbs
                date.updateWiggle();
            }

            if (AppSettings::mEventModel == EventModelType::EDM2) {
                if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
                    event->updateS02Theta_v338();
                }
            }

            std::for_each(event->mPhases.begin(), event->mPhases.end(),
                          [this](std::shared_ptr<Phase> p) {
                              p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                          });
        }

        std::for_each(mModel->mPhases.begin(), mModel->mPhases.end(),
                      [this](std::shared_ptr<Phase> p) {
                          p->update_Tau(tminPeriod, tmaxPeriod);
                      });

        std::for_each(mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });

    } catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;
    }
}

bool MCMCLoopChrono::update_v4()
{

    /* --------------------------------------------------------------
     *  A - Update ti Dates
     *  B - Update Theta Events
     *  C.1 - Update Alpha, Beta & Duration Phases
     *  C.2 - Update Tau Phase
     *  C.3 - Update Gamma Phases
     * ---------------------------------------------------------------------- */

    // --------------------------------------------------------------
    //  B - Update theta Events
    // --------------------------------------------------------------
    for (auto&& event : mModel->mEvents) {
        // --------------------------------------------------------------
        //  A - Update ti Dates
        // --------------------------------------------------------------
        if (event->mType == Event::eDefault) {
            event->updateTheta(tminPeriod, tmaxPeriod);
            if (AppSettings::mEventModel == EventModelType::EDM2 ) {
                if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe)
                    event->updateS02Theta_v338();
            }
        } else //if (event->mType == Event::eBound)
                event->updateTheta(tminPeriod, tmaxPeriod);



       //--------------------- Update Phases -set mAlpha and mBeta they coud be used by the Event in the other Phase ----------------------------------------
        /* --------------------------------------------------------------
         * C.1 - Update Alpha, Beta & Duration Phases
         * -------------------------------------------------------------- */
        //  Update Phases -set mAlpha and mBeta ; they coud be used by the Event in the other Phase ----------------------------------------
        std::for_each(PAR event->mPhases.begin(), event->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_AlphaBeta (tminPeriod, tmaxPeriod);});

    }

    //  Update Phases Tau; they coud be used by the Event in the other Phase ----------------------------------------
    /* --------------------------------------------------------------
     *  C.2 - Update Tau Phases
     * -------------------------------------------------------------- */
    std::for_each(PAR mModel->mPhases.begin(), mModel->mPhases.end(), [this] (std::shared_ptr<Phase> p) {p->update_Tau (tminPeriod, tmaxPeriod);});


    /* --------------------------------------------------------------
     *  C.3 - Update Phases constraints
     * -------------------------------------------------------------- */
    std::for_each(PAR mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(), [] (std::shared_ptr<PhaseConstraint> pc) {pc->updateGamma();});


    return true;

}


bool MCMCLoopChrono::adapt(const int batchIndex)
{

    // Pour un Random‑Walk Metropolis en une dimension, la théorie (Roberts et al., 1997) montre que le taux optimal est ≈ 0.44
    constexpr double taux_min = 0.42;
    constexpr double taux_max = 0.46;

    //
    /* En haute dimension (> 5), le taux optimal se rapproche de 0.23.
        Si vous avez des vecteurs de grande dimension, il serait judicieux de réduire la fenêtre (ex. 0.20‑0.30).
    */
    constexpr double taux_min_2 = 0.30;
    constexpr double taux_max_2 = 0.40;
/*
 * d (paramètres proposés ensemble)	taux optimal
1	≈ 0.44
2	≈ 0.35
3	≈ 0.32
∞	0.234
 */
    bool noAdapt = true;

    //--------------------- Adapt -----------------------------------------


    for (const auto& event : mModel->mEvents) {
        if (event->mDates.size() > 1) {
            for (auto& date : event->mDates) {
                //--------------------- Adapt Sigma MH de t_i -----------------------------------------
                if (date.mTi.mSamplerProposal == SamplerProposal::eRWAdaptGauss)
                    noAdapt &= date.mTi.adapt(taux_min_2, taux_max_2, batchIndex);

                //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
                if (date.mSigmaTi.mSamplerProposal == SamplerProposal::eRWAdaptGauss)
                    noAdapt &= date.mSigmaTi.adapt(taux_min_2, taux_max_2, batchIndex);

            }
        } else {
            for (auto& date : event->mDates) {
                //--------------------- Adapt Sigma MH de t_i -----------------------------------------
                if (date.mTi.mSamplerProposal == SamplerProposal::eRWAdaptGauss)
                    noAdapt &= date.mTi.adapt(taux_min, taux_max, batchIndex);

                //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
                if (date.mSigmaTi.mSamplerProposal == SamplerProposal::eRWAdaptGauss)
                    noAdapt &= date.mSigmaTi.adapt(taux_min, taux_max, batchIndex);

            }
        }

        //--------------------- Adapt Sigma MH de Theta Event -----------------------------------------
        if ((event->mType != Event::eBound) && ( event->mTheta.mSamplerProposal == SamplerProposal::eRWAdaptGauss) )
            noAdapt = event->mTheta.adapt(taux_min, taux_max, batchIndex) && noAdapt;

        if (AppSettings::mEventModel == EventModelType::EDM2 ) {
            if ( event->mS02Theta.mSamplerProposal == SamplerProposal::eRWAdaptGauss)
                noAdapt = event->mS02Theta.adapt(taux_min, taux_max, batchIndex) && noAdapt;
        }
    }


    return noAdapt;
}

void MCMCLoopChrono::acquire()
{
    for (auto& event : mModel->mEvents) {
        //--------------------- Memo Events -----------------------------------------
        if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
            event->mTheta.acquire();
            for (auto&& date : event->mDates ) {
                //--------------------- Memo Dates -----------------------------------------
                date.mTi.acquire();
                date.mSigmaTi.acquire();
                date.mWiggle.acquire();
            }

        }

        if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
            double memoS02 = sqrt(event->mS02Theta.value());
            event->mS02Theta.acquire(&memoS02);

        }

    }

    //--------------------- Memo Phases -----------------------------------------
    for (auto& ph : mModel->mPhases)
        ph->acquire();

}


void MCMCLoopChrono::recordBurnAdapt()
{
    for (auto& event : mModel->mEvents) {
        //--------------------- Memo Events -----------------------------------------
        if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {
            event->mTheta.recordBurnAdapt();
            for (auto&& date : event->mDates )   {
                //--------------------- Memo Dates -----------------------------------------
                date.mTi.recordBurnAdapt();
                date.mSigmaTi.recordBurnAdapt();
                date.mWiggle.recordBurnAdapt();
            }
        }
        if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
            double memoS02 = sqrt(event->mS02Theta.value());
            event->mS02Theta.recordBurnAdapt(&memoS02);

        }
    }

    //--------------------- Memo Phases -----------------------------------------
    for (auto& ph : mModel->mPhases)
        ph->recordBurnAdapt();

}

// Ne sert que pourl'affichage
void MCMCLoopChrono::recordMH()
{
    for (auto& event : mModel->mEvents) {
        //--------------------- Memo Events -----------------------------------------
        if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {

            event->mTheta.saveCurrentAcceptRate();
            for (auto&& date : event->mDates )   {
                //--------------------- Memo Dates -----------------------------------------
                date.mTi.saveCurrentAcceptRate();

                date.mSigmaTi.saveCurrentAcceptRate();
            }

            if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
                event->mS02Theta.saveCurrentAcceptRate();
            }

        }

    }

}



void MCMCLoopChrono::finalize()
{
#ifdef DEBUG
    qDebug()<<QString("[MCMCLoopChrono::finalize]");
    QElapsedTimer startTime;
    startTime.start();
#endif

    // This is not a copy of all data!
    // Chains only contain description of what happened in the chain (numIter, numBatch adapt, ...)
    // Real data are inside mModel members (mEvents, mPhases, ...)
    mModel->mChains = mLoopChains;

    // This is called here because it is calculated only once and will never change afterwards
    // This is very slow : it is for this reason that the results display may be long to appear at the end of MCMC calculation.

    emit setMessage(tr("Computing posterior distributions and numerical results"));
    mModel->generateCorrelations(mModel->mChains);

    mModel->initDensities();

    // This should not be done here because it uses resultsView parameters
    // ResultView will trigger it again when loading the model

    // Generate numerical results of :
    // - MHVariables (global acceptation)
    // - MetropolisVariable : analysis of Posterior densities and quartiles from traces.
    // This also should be done in results view...
    //mModel->generateNumericalResults(mChains);

#ifdef DEBUG
    QTime endTime = QTime::currentTime();

    qDebug()<<"[MCMCLoopChrono::finalize] Model computed";
    qDebug()<<QString("[MCMCLoopChrono::finalize] finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug()<<QString("Total time elapsed %1").arg(DHMS(startTime.elapsed()));
#endif
}


#pragma mark Slice Sampling
// ============================================================================
// sampler_339_SingleSite_SliceSampling
//
// Copie complète et autonome de sampler_339_SingleSite_bloc_2, mais avec un
// SEUL noyau pour ti : slice sampling exact (sigma_old fixé pendant le
// tirage), suivi d'un test M-H séparé pour sigma seul (ti fixé à sa nouvelle
// valeur des deux côtés). Plus de mixingKernel, plus de branches A.1/B.2 :
// c'est voulu, pour isoler et valider ce seul noyau avant de l'intégrer comme
// option dans le sampler bloc principal.
//
// À ajouter dans MCMCLoopChrono.h :
//   void sampler_339_SingleSite_SliceSampling(std::vector<std::shared_ptr<Event>> &events);
//
// À ajouter dans votre énum SamplerProposal si absent :
//   eSliceSampling
// ============================================================================


// ----------------------------------------------------------------------------
// Slice sampling 1D (Neal, 2003) : "stepping out" puis "shrinkage".
// logTarget(t) renvoie le log-densité (à une constante additive près).
// x0 = état courant, w = largeur initiale de tranche, lower/upper = bornes dures.
// ----------------------------------------------------------------------------
namespace {

double sliceSample1D(const std::function<double(double)>& logTarget,
                      double x0,
                      double w,
                      double lower,
                      double upper,
                      int maxStepsOut = 50,
                      int maxShrink   = 100)
{
    const double logY = logTarget(x0) + std::log(Generator::randomUniform()); // niveau (log) de la tranche

    double L = x0 - w * Generator::randomUniform();
    double R = L + w;
    L = std::max(L, lower);
    R = std::min(R, upper);

    int stepsLeft = maxStepsOut;
    while (L > lower && logTarget(L) > logY && stepsLeft-- > 0) {
        L -= w;
        L = std::max(L, lower);
    }
    stepsLeft = maxStepsOut;
    while (R < upper && logTarget(R) > logY && stepsLeft-- > 0) {
        R += w;
        R = std::min(R, upper);
    }

    for (int i = 0; i < maxShrink; ++i) {
        const double xProp = L + Generator::randomUniform() * (R - L);
        if (logTarget(xProp) > logY)
            return xProp; // dans la tranche : tirage EXACT, aucun test M-H requis

        if (xProp < x0) L = xProp;
        else            R = xProp;
    }

    return x0; // repli de sécurité (cible pathologique / bornes trop serrées)
}

} // namespace anonyme


void MCMCLoopChrono::sampler_339_SingleSite_SliceSampling(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        // ======================================================================
        // 1. Mise à jour de tous les événements
        // ======================================================================
        for (auto &event : events) {

            // ======================================================================
            // 2. Mise à jour de theta uniquement si non fixé
            // ======================================================================
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {

                // ==================================================================
                // 3. Évaluation des bornes de theta
                // ==================================================================
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                    .arg(QString::fromLatin1(__func__),
                         event->getQStringName(),
                         QString::number(min),
                         QString::number(max));
                }

                // ==================================================================
                // 4. Initialisation des statistiques pondérées globales
                // ==================================================================
                long double P_curr = 0.0L;
                long double mu_curr = 0.0L;
                long double S_curr = 0.0L;

                for (const auto& date : event->mDates) {
                    const long double sigma = static_cast<long double>(date.mSigmaTi.value());
                    const long double y = static_cast<long double>(date.mTi.value()) + static_cast<long double>(date.mDelta);
                    const long double w = 1.0L / (sigma * sigma);

                    if (P_curr == 0.0L) {
                        P_curr  = w;
                        mu_curr = y;
                        S_curr  = 0.0L;
                    }
                    else {
                        const long double P_old = P_curr;
                        const long double P_new = P_old + w;
                        const long double d = y - mu_curr;
                        const long double mu_new = mu_curr + (w / P_new) * d;

                        S_curr += w * d * (y - mu_new);
                        P_curr  = P_new;
                        mu_curr = mu_new;
                    }
                }

                // ==========================================================================
                // 5. Mise à jour (ti, delta, sigma) pour chaque date : SLICE SAMPLING SEUL
                // ==========================================================================
                for (size_t i = 0; i < event->mDates.size(); ++i) {

                    auto& date = event->mDates[i];
                    const double ti_old0    = date.mTi.value();
                    const double delta_old0 = date.mDelta;
                    const double sigma_old  = date.mSigmaTi.value();

                    double sigma_prop  = sigma_old;
                    double log_rate_q_s = 0.0;
                    bool isvalide = true;

                    // ----------------------------------------------------------------------
                    // A. Calcul préalable Leave-One-Out O(1)
                    // ----------------------------------------------------------------------
                    const long double V1_old    = static_cast<long double>(sigma_old) * sigma_old;
                    const long double w_old     = 1.0L / V1_old;
                    const long double y_old     = static_cast<long double>(ti_old0) + delta_old0;

                    const long double P_removed = P_curr - w_old;
                    long double mu_removed = 0.0L;
                    long double S_removed  = 0.0L;

                    if (P_removed > 0.0L) {
                        mu_removed = (P_curr * mu_curr - w_old * y_old) / P_removed;
                        S_removed  = S_curr - w_old * (y_old - mu_curr) * (y_old - mu_removed);
                        if (S_removed < 0.0L && S_removed > -1e-18L) S_removed = 0.0L;
                    } else {
                        mu_removed = y_old;
                    }

                    // ----------------------------------------------------------------------
                    // B. Tirage de delta_prop (indépendant)
                    // ----------------------------------------------------------------------
                    double delta_prop;
                    switch (date.mDeltaType) {
                    case Date::eDeltaNone:     delta_prop = 0.0; break;
                    case Date::eDeltaRange:    delta_prop = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax); break;
                    case Date::eDeltaGaussian: delta_prop = Generator::normalDistribution(date.mDeltaAverage, date.mDeltaError); break;
                    case Date::eDeltaFixed:    delta_prop = date.mDeltaFixed; break;
                    default: delta_prop = delta_old0; break;
                    }

                    // ----------------------------------------------------------------------
                    // C. Proposition de sigma : RW adaptatif sur log10(V)
                    // ----------------------------------------------------------------------
                    date.mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
                    const double log10_V1 = std::log10(static_cast<double>(V1_old));
                    const double log10_V2 = Generator::normalDistribution(log10_V1, date.mSigmaTi.mSigmaMH);

                    constexpr double logVMin = -100.0;
                    constexpr double logVMax =  100.0;
                    if (!std::isfinite(log10_V2) || log10_V2 < logVMin || log10_V2 > logVMax) {
                        isvalide = false;
                    } else {
                        const double V2 = std::pow(10.0, log10_V2);
                        sigma_prop = std::sqrt(V2);
                    }

                    if (isvalide) {
                        // Jacobien "sigma seul" du changement de variable V -> u = log10(V)
                        log_rate_q_s = 2.0 * std::log(sigma_prop / sigma_old);
                    } else {
                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                        continue; // date suivante
                    }

                    // ======================================================================
                    // D. NOYAU SLICE : tirage exact de ti à sigma_old fixé
                    // ======================================================================
                    date.mTi.mSamplerProposal = SamplerProposal::eSliceSampling;

                    const double tLower = date.mCalibration ? date.mCalibration->mTmin
                                                             : -std::numeric_limits<double>::infinity();
                    const double tUpper = date.mCalibration ? date.mCalibration->mTmax
                                                             :  std::numeric_limits<double>::infinity();

                    // Densité cible de ti, à sigma = sigma_old FIXÉ (dérivée de la formule
                    // du prior marginal après intégration de theta, avec V2_new -> V1_old
                    // partout ; le terme de shrinkage S02 ne dépend que de sigma, il
                    // disparaît car constant en t).
                    auto logTargetTi = [&](double t) -> double {
                        const double L = date.getLikelihood(t);
                        if (!(L > 0.0))
                            return -std::numeric_limits<double>::infinity();
                        const long double logL = std::log(static_cast<long double>(L));

                        const long double y_t = static_cast<long double>(t) + delta_prop;
                        long double logPriorMarginal;

                        if (min == max) {
                            const long double theta_fixed = static_cast<long double>(min);
                            const long double residual = y_t - theta_fixed;
                            logPriorMarginal = -0.5L * residual * residual / V1_old;
                        } else {
                            const long double P_t = P_removed + w_old; // constant en t
                            long double mu_t, S_t;
                            if (P_removed > 0.0L) {
                                const long double d = y_t - mu_removed;
                                mu_t = mu_removed + (w_old / P_t) * d;
                                S_t  = S_removed + w_old * d * (y_t - mu_t);
                            } else {
                                mu_t = y_t;
                                S_t  = 0.0L;
                            }
                            const long double sigma_avg = 1.0L / std::sqrt(P_t); // constant en t

                            const long double alpha_min = (static_cast<long double>(min) - mu_t) / sigma_avg;
                            const long double alpha_max = (static_cast<long double>(max) - mu_t) / sigma_avg;
                            const double logZ = log_diff_cdf(static_cast<double>(alpha_min), static_cast<double>(alpha_max));
                            if (!std::isfinite(logZ))
                                return -std::numeric_limits<double>::infinity();

                            logPriorMarginal = static_cast<long double>(logZ) - 0.5L * S_t;
                        }

                        return static_cast<double>(logL + logPriorMarginal);
                    };

                    const double ti_prop = sliceSample1D(logTargetTi, ti_old0, date.mTi.mSigmaMH, tLower, tUpper);

                    // Tirage exact : pas de test M-H pour ti.
                    date.mTi.accept_update(ti_prop);
                    date.mDelta = delta_prop;

                    // --- Accumulateurs après le tirage de ti, sigma encore = sigma_old ---
                    const long double y_afterTi = static_cast<long double>(ti_prop) + delta_prop;
                    long double P_afterTi, mu_afterTi, S_afterTi;
                    if (P_removed > 0.0L) {
                        const long double d = y_afterTi - mu_removed;
                        P_afterTi  = P_removed + w_old;
                        mu_afterTi = mu_removed + (w_old / P_afterTi) * d;
                        S_afterTi  = S_removed + w_old * d * (y_afterTi - mu_afterTi);
                    } else {
                        P_afterTi  = w_old;
                        mu_afterTi = y_afterTi;
                        S_afterTi  = 0.0L;
                    }

                    // ======================================================================
                    // E. Test M-H séparé pour sigma seul (ti fixé à ti_prop des deux côtés)
                    // ======================================================================
                    const long double V2_new = static_cast<long double>(sigma_prop) * sigma_prop;
                    const long double w_new  = 1.0L / V2_new;

                    long double P_sig, mu_sig, S_sig;
                    if (P_removed > 0.0L) {
                        const long double d = y_afterTi - mu_removed;
                        P_sig  = P_removed + w_new;
                        mu_sig = mu_removed + (w_new / P_sig) * d;
                        S_sig  = S_removed + w_new * d * (y_afterTi - mu_sig);
                    } else {
                        P_sig  = w_new;
                        mu_sig = y_afterTi;
                        S_sig  = 0.0L;
                    }
                    if (S_sig < 0.0L && S_sig > -1e-18L) S_sig = 0.0L;

                    const long double log_sigma_i_ratio = std::log(sigma_prop / static_cast<long double>(sigma_old));
                    long double log_prior_marginal_diff_sigma = 0.0L;
                    bool degenerate = false;

                    if (min == max) {
                        const long double theta_fixed = static_cast<long double>(min);
                        const long double residual     = y_afterTi - theta_fixed; // ti identique des deux côtés
                        log_prior_marginal_diff_sigma = -0.5L * residual * residual * (1.0L / V2_new - 1.0L / V1_old)
                                                         - log_sigma_i_ratio;
                    } else {
                        const long double sigma_avg_old = 1.0L / std::sqrt(P_afterTi);
                        const long double sigma_avg_new = 1.0L / std::sqrt(P_sig);

                        const long double alpha_old_min = (static_cast<long double>(min) - mu_afterTi) / sigma_avg_old;
                        const long double alpha_old_max = (static_cast<long double>(max) - mu_afterTi) / sigma_avg_old;
                        const long double alpha_new_min = (static_cast<long double>(min) - mu_sig)      / sigma_avg_new;
                        const long double alpha_new_max = (static_cast<long double>(max) - mu_sig)      / sigma_avg_new;

                        const double log_Z_old = log_diff_cdf(static_cast<double>(alpha_old_min), static_cast<double>(alpha_old_max));
                        const double log_Z_new = log_diff_cdf(static_cast<double>(alpha_new_min), static_cast<double>(alpha_new_max));

                        if (!std::isfinite(log_Z_old) || !std::isfinite(log_Z_new)) {
                            degenerate = true;
                        } else {
                            const long double log_sigma_avg_ratio = std::log(sigma_avg_new / sigma_avg_old);
                            log_prior_marginal_diff_sigma = static_cast<long double>(log_Z_new - log_Z_old)
                                                            + log_sigma_avg_ratio
                                                            - log_sigma_i_ratio
                                                            - 0.5L * (S_sig - S_afterTi);
                        }
                    }

                    if (degenerate) {
                        date.mSigmaTi.reject_update();
                        P_curr = P_afterTi; mu_curr = mu_afterTi; S_curr = S_afterTi;
                    } else {
                        const long double S02_ld = static_cast<long double>(event->mS02Theta.value());
                        const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1_old) - std::log(S02_ld + V2_new));

                        const long double log_rate_total_sigma = log_prior_marginal_diff_sigma
                                                                 + log_prior_shrinkage
                                                                 + static_cast<long double>(log_rate_q_s);

                        if (MHAcceptanceTest_log(static_cast<double>(log_rate_total_sigma))) {
                            date.mSigmaTi.accept_update(static_cast<double>(sigma_prop));
                            P_curr = P_sig; mu_curr = mu_sig; S_curr = S_sig;
                        } else {
                            date.mSigmaTi.reject_update();
                            P_curr = P_afterTi; mu_curr = mu_afterTi; S_curr = S_afterTi;
                        }
                    }

                } // Fin de boucle des dates

                // ==================================================================
                // 6. Mise à jour de theta (inchangé)
                // ==================================================================
                if (min == max) {
                    qDebug() << "[ " << __func__ << QString("] Warning for event : %1 : min == max = %2")
                    .arg( event->getQStringName(), QString::number(min));
                    event->mTheta.accept_update(min);
                }
                else {
                    constexpr bool useHMC = false;   // bascule pour A/B test
                    if (useHMC) {
                        constexpr int L_hmc = 20;
                        const double eps_hmc = (M_PI / 2.0) / L_hmc;

                        const double new_theta = hmcSampleTheta_temporal(
                                    event->mTheta.value(), mu_curr, P_curr, min, max, L_hmc, eps_hmc);

                        event->mTheta.accept_update(new_theta);
                    } else {
                        const double ti_avg_final = static_cast<double>(mu_curr);
                        const double sigma_avg_final = 1.0 / std::sqrt(static_cast<double>(P_curr));
                        const double new_theta = Generator::truncatedNormal(ti_avg_final, sigma_avg_final, min, max);

                        event->mTheta.accept_update(new_theta);
                    }
                }

                // ==================================================================
                // 7. Mise à jour des wiggles
                // ==================================================================
                for (auto&& date : event->mDates) {
                    date.updateWiggle();
                }

                // ==================================================================
                // 8. Mise à jour de S02Theta
                // ==================================================================
                if (AppSettings::mEventModel == EventModelType::EDM2) {
                    if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
                        event->updateS02Theta_v338();
                    }
                }

                // ==================================================================
                // 9. Mise à jour des bornes des phases de l'événement
                // ==================================================================
                std::for_each(
                    event->mPhases.begin(),
                    event->mPhases.end(),
                    [this](std::shared_ptr<Phase> p) {
                        p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                    });
            }
        }

        // ======================================================================
        // 10. Mises à jour globales des phases
        // ======================================================================
        std::for_each(
            mModel->mPhases.begin(),
            mModel->mPhases.end(),
            [this](std::shared_ptr<Phase> p) {
                p->update_Tau(
                    tminPeriod,
                    tmaxPeriod);
            });

        // ======================================================================
        // 11. Mise à jour globale des contraintes de phases
        // ======================================================================
        std::for_each( mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });
    }
    catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
        return;
    }
    catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error " << e.what();
        return;
    }
    catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range " << e.what();
        return;
    }
    catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;
    }
    catch (...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
        return;
    }
}

// ============================================================================
// sampler_339_SingleSite_NonCentered
//
// Basé sur sampler_339_SingleSite_bloc_2, avec DEUX changements par rapport à
// l'original :
//
// 1) Le noyau A.1 (étape 5) est inchangé. Le noyau B.2 est REMPLACÉ : au lieu
//    de perturber aléatoirement z=(ti-theta)/sigma en même temps que sigma
//    (deux sources de bruit qui se diluent, et un mSigmaMH de plus sujet au
//    même effondrement adaptatif que les autres), le nouveau B.2 tient z
//    EXACTEMENT fixe et ne fait bouger ti QUE via sigma_prop (déjà tiré à
//    l'étape C) : ti_prop = theta_courant + sigma_prop * z_fixed, avec
//    theta_courant = event->mTheta.value() (PAS une moyenne marginale des
//    autres dates : toujours disponible, y compris pour un événement à une
//    seule date, cas où l'ancienne version dégénérait en marche aléatoire
//    découplée de sigma - donc sans effet sur le funnel). C'est un rescale
//    non centré exact, qui cible le funnel (ti_i, sigmaTi_i) PAR DATE : quand
//    une date est très informative, sigmaTi_i est poussé vers 0 par le prior
//    de shrinkage, et ti_i doit alors rester collé à theta de plus en plus
//    précisément - géométrie en entonnoir classique (Neal), ici à l'échelle
//    d'une seule date plutôt qu'au niveau de l'événement entier. L'étape C
//    (proposition de sigma) reçoit aussi un mélange d'échelle, pour la même
//    raison que pour deltaTheta ci-dessous.
//
// 2) AJOUT de l'étape 5-bis : un mouvement de décalage rigide conjoint de
//    theta et de TOUS les ti de l'événement. Ce mouvement cible le cas où les
//    dates sont mutuellement très informatives (accord fort, sigma petits) :
//    dans la paramétrisation centrée, theta et les ti deviennent alors
//    fortement corrélés (même effet "funnel", mais au niveau de l'événement
//    entier) et les mises à jour une-date-à-la-fois de l'étape 5 ne
//    suffisent plus à les faire bouger efficacement ensemble.
//
// Le décalage rigide ti_prop_i = ti_old_i + Δtheta, theta_prop = theta_old +
// Δtheta laisse EXACTEMENT invariant l'écart z_i = (ti-theta)/sigma_i, donc le
// prior hiérarchique Normal(ti; theta, sigmaTi²) ne contribue rien au ratio
// d'acceptation : seules la vraisemblance de calibration (absolue en ti) et
// la borne dure [min,max] de theta interviennent.
//
// À valider sur le même protocole que les noyaux précédents : d'abord le cas
// qui a révélé le problème (N dates identiques centrées), en comparant le
// traceplot/ESS de theta avec et sans l'étape 5-bis ; puis vérifier que ce
// mouvement ne dégrade rien sur des dates qui NE sont PAS d'accord (il devrait
// y être simplement rarement accepté, sans nuire) ; puis le cas de deux dates
// décalées avec des variances différentes (deux chaînes qui se figent chacune
// sur une date différente = échec de mixing, pas de la vraie multimodalité
// puisque le modèle est gaussien-gaussien donc log-concave).
//
// À ajouter dans MCMCLoopChrono.h :
//   void sampler_339_SingleSite_NonCentered(std::vector<std::shared_ptr<Event>> &events);
//
// Pas de nouveau champ adaptatif requis sur mTheta : le pas de Δtheta est
// calé sur sigma_avg = 1/sqrt(P_curr), l'écart-type EXACT de la loi
// conditionnelle courante de theta (même quantité que celle utilisée à
// l'étape 6 pour le Gibbs de theta), recalculé à chaque itération à partir
// des données courantes - donc pas d'adaptation par accumulation
// acceptations/rejets, et pas de risque d'effondrement du pas vers zéro par
// rétroaction (un mSigmaMH adaptatif classique peut s'auto-piéger : proche
// d'une des deux dates en conflit, les grands pas vers l'autre sont presque
// toujours rejetés, donc l'adaptation réduit le pas, ce qui aggrave le
// piège). Un mélange d'échelle (kWideJumpProb, kWideJumpFactor, voir
// l'étape 5-bis) ajoute une capacité de saut ponctuel plus large, utile
// quand deux dates en désaccord imposent un compromis éloigné de sigma_avg
// seul (sigma_avg ne dépend que de la précision du couplage hiérarchique
// sigmaTi, pas de la largeur des vraisemblances de calibration ni de
// l'écart entre les dates) - à régler/valider empiriquement sur vos cas
// de test.
//
// 3) AJOUT (v2) : Δtheta peut aussi être dérivé d'un tirage d'indépendance sur
// une date pilote tirée au hasard dans l'événement (proba kPilotProb), au lieu
// du saut aveugle N(0,sigma_avg) seul. Utile quand une ou deux dates de
// l'événement ont une vraisemblance de calibration très multimodale (plateau
// de calibration, ~30 modes) : le prior hiérarchique met son veto à un saut
// A.1 vers un mode éloigné tant que theta ne l'a pas déjà rejoint (problème de
// l'oeuf et la poule), et un saut aveugle N(0,sigma_avg) a très peu de chances
// de tomber pile dans un pic étroit. Dériver Δtheta du tirage de la date
// pilote (même mélange calibration/RW que A.1) vise directement un mode
// plausible et entraîne theta + toutes les autres dates avec lui - toujours
// une translation rigide pure, donc le couplage hiérarchique s'annule pour
// toutes les dates comme avant ; seule la proposition n'étant plus symétrique,
// le terme de Hastings de la date pilote (identique à celui de A.1) s'ajoute
// au ratio. Voir l'étape 5-bis pour le détail.
// ============================================================================

void MCMCLoopChrono::sampler_339_SingleSite_NonCentered(std::vector<std::shared_ptr<Event>> &events)
{
    try {
        // ======================================================================
        // 1. Mise à jour de tous les événements
        // ======================================================================
        for (auto &event : events) {

            // ======================================================================
            // 2. Mise à jour de theta uniquement si non fixé
            // ======================================================================
            if (event->mTheta.mSamplerProposal != SamplerProposal::eFixe) {

                // ==================================================================
                // 3. Évaluation des bornes de theta
                // ==================================================================
                const double min = event->getThetaMin(tminPeriod);
                const double max = event->getThetaMax(tmaxPeriod);

                if (min > max) {
                    throw QObject::tr("[%1] Error for event : %2 : min = %3 > max = %4")
                    .arg(QString::fromLatin1(__func__),
                         event->getQStringName(),
                         QString::number(min),
                         QString::number(max));
                }

                // ==================================================================
                // 4. Initialisation des statistiques pondérées globales
                // ==================================================================
                long double P_curr = 0.0L;
                long double mu_curr = 0.0L;
                long double S_curr = 0.0L;

                for (const auto& date : event->mDates) {
                    const long double sigma = static_cast<long double>(date.mSigmaTi.value());
                    const long double y = static_cast<long double>(date.mTi.value()) + static_cast<long double>(date.mDelta);
                    const long double w = 1.0L / (sigma * sigma);

                    if (P_curr == 0.0L) {
                        P_curr  = w;
                        mu_curr = y;
                        S_curr  = 0.0L;
                    }
                    else {
                        const long double P_old = P_curr;
                        const long double P_new = P_old + w;
                        const long double d = y - mu_curr;
                        const long double mu_new = mu_curr + (w / P_new) * d;

                        S_curr += w * d * (y - mu_new);
                        P_curr  = P_new;
                        mu_curr = mu_new;
                    }
                }

                // ==========================================================================
                // 5. Mise à jour BLOC (ti, delta, sigma) pour chaque date
                //    A.1 inchangé ; B.2 REMPLACÉ par le rescale non centré exact (voir
                //    l'en-tête du fichier et le commentaire à l'étape D ci-dessous).
                // ==========================================================================
                constexpr double mixingKernel = 0.5;
                // 0 -> A.1 uniquement ; 1 -> nouveau B.2 uniquement

                for (size_t i = 0; i < event->mDates.size(); ++i) {

                    auto& date = event->mDates[i];
                    const double ti_old0    = date.mTi.value();
                    const double delta_old0 = date.mDelta;
                    const double sigma_old  = date.mSigmaTi.value();

                    double ti_prop = ti_old0;
                    double sigma_prop = sigma_old;
                    double log_rate_L = 0.0;
                    double log_rate_q_t = 0.0;
                    double log_rate_q_s = 0.0;
                    bool isvalide = true;

                    // ----------------------------------------------------------------------
                    // A. Calcul préalable Leave-One-Out O(1)
                    // ----------------------------------------------------------------------
                    const long double V1_old    = static_cast<long double>(sigma_old) * sigma_old;
                    const long double w_old     = 1.0L / V1_old;
                    const long double y_old     = static_cast<long double>(ti_old0) + delta_old0;

                    const long double P_removed = P_curr - w_old;
                    long double mu_removed = 0.0L;
                    long double S_removed  = 0.0L;

                    if (P_removed > 0.0L) {
                        mu_removed = (P_curr * mu_curr - w_old * y_old) / P_removed;
                        S_removed  = S_curr - w_old * (y_old - mu_curr) * (y_old - mu_removed);
                        if (S_removed < 0.0L && S_removed > -1e-18L) S_removed = 0.0L;
                    } else {
                        mu_removed = y_old;
                    }

                    // ----------------------------------------------------------------------
                    // B. Tirage de delta_prop (indépendant, identique pour les deux noyaux)
                    // ----------------------------------------------------------------------
                    double delta_prop;
                    switch (date.mDeltaType) {
                    case Date::eDeltaNone:     delta_prop = 0.0; break;
                    case Date::eDeltaRange:    delta_prop = Generator::randomUniform(date.mDeltaMin, date.mDeltaMax); break;
                    case Date::eDeltaGaussian: delta_prop = Generator::normalDistribution(date.mDeltaAverage, date.mDeltaError); break;
                    case Date::eDeltaFixed:    delta_prop = date.mDeltaFixed; break;
                    default: delta_prop = delta_old0; break;
                    }

                    // ----------------------------------------------------------------------
                    // C. Proposition PARTAGÉE de sigma : RW adaptatif sur log10(V), avec
                    //    mélange d'échelle (même principe que pour deltaTheta) : le pas
                    //    adaptatif seul s'auto-piège quand la date est très informative
                    //    (funnel (ti,sigma) : les grands pas sont presque toujours
                    //    rejetés près de la solution, donc l'adaptation réduit le pas,
                    //    ce qui aggrave le blocage). Une faible proportion de pas
                    //    beaucoup plus larges, non filtrés par l'historique d'acceptation,
                    //    permet d'en sortir.
                    // ----------------------------------------------------------------------
                    date.mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
                    const double log10_V1 = std::log10(static_cast<double>(V1_old));

                    constexpr double kWideJumpProbSigma   = 0.10; // proportion de "grands" pas
                    constexpr double kWideJumpFactorSigma = 10.0; // facteur d'inflation (échelle log10(V))
                    const double sigmaStepMH = (Generator::randomUniform() < kWideJumpProbSigma)
                                                    ? kWideJumpFactorSigma * date.mSigmaTi.mSigmaMH
                                                    : date.mSigmaTi.mSigmaMH;

                    const double log10_V2 = Generator::normalDistribution(log10_V1, sigmaStepMH);

                    constexpr double logVMin = -100.0;
                    constexpr double logVMax =  100.0;
                    if (!std::isfinite(log10_V2) || log10_V2 < logVMin || log10_V2 > logVMax) {
                        isvalide = false;
                    } else {
                        const double V2 = std::pow(10.0, log10_V2);
                        sigma_prop = std::sqrt(V2);
                    }

                    if (isvalide) {
                        log_rate_q_s = 2.0 * std::log(sigma_prop / sigma_old);
                    }

                    // ----------------------------------------------------------------------
                    // D. Proposition de ti selon le noyau choisi
                    // ----------------------------------------------------------------------
                    const double u_kernel = Generator::randomUniform();

                    if (isvalide && u_kernel < mixingKernel) {
                        // ==============================================================
                        // NOYAU B.2 REMPLACÉ : rescale non centré EXACT (z fixé)
                        // ==============================================================
                        // L'ancien B.2 perturbait z ALÉATOIREMENT (z_prop = N(z_old,s_z))
                        // en plus de faire bouger sigma_prop : deux sources de bruit qui
                        // diluent la corrélation qu'on cherche justement à exploiter, et
                        // un paramètre de pas (mTi.mSigmaMH) de plus à régler/à voir
                        // s'effondrer comme les autres. Ici z est laissé EXACTEMENT fixe :
                        // seul sigma_prop (déjà tiré à l'étape C, avec son propre mélange
                        // d'échelle) fait bouger ti, via ti = c + sigma_prop * z.
                        //
                        // Centre c = theta_courant (event->mTheta.value()), PAS ti_bar
                        // (la moyenne marginale des autres dates) : ti_bar n'existe pas
                        // pour un événement à une seule date (P_removed=0), ce qui faisait
                        // retomber ce noyau sur une marche aléatoire simple, découplée de
                        // sigma - donc SANS AUCUN effet sur le funnel (ti,sigmaTi) dans ce
                        // cas, justement celui qui posait problème. theta_courant est
                        // toujours disponible (y compris theta fixé, min=max), et
                        // l'argument du Jacobien (t = c + sigma·z, c constante pendant le
                        // balayage des dates) reste valide pour N'IMPORTE QUELLE constante
                        // c - pas seulement ti_bar. Comme z = (ti-theta)/sigma est inchangé,
                        // le terme de couplage hiérarchique Normal(ti; theta, sigma_i²)
                        // reste rigoureusement le même avant/après, pour TOUT N (y compris
                        // N=1) : c'est le même rescale non centré que l'analogue en échelle
                        // du décalage rigide de theta (étape 5-bis), qui casse le funnel
                        // (ti_i, sigmaTi_i) au lieu du funnel (theta, {ti}).
                        date.mTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

                        const double theta_courant = event->mTheta.value(); // fixe pendant tout le balayage des dates (mis à jour seulement à l'étape 6)
                        const double z_fixed = (ti_old0 - theta_courant) / sigma_old;

                        ti_prop = theta_courant + sigma_prop * z_fixed;

                        log_rate_q_t = 0.0;
                        // Jacobien du changement de variable (ti,V) -> (z,V) à z fixé :
                        // terme log(sigma_prop/sigma_old), EN PLUS du 2*log(sigma_prop/
                        // sigma_old) déjà accumulé à l'étape C (Jacobien du RW en
                        // log10(V) seul). Sans ce terme le ratio serait faux.
                        log_rate_q_s += std::log(sigma_prop / sigma_old);
                    }
                    else if (isvalide) {
                        // ==============================================================
                        // NOYAU A.1 : ti via Likelihood/Calib, indépendant de sigma
                        // ==============================================================
                        date.mTi.mSamplerProposal = SamplerProposal::eLikelihood;

                        if (Generator::randomUniform() < date.mMixingLevel) {
                            ti_prop = *date.mCalibration->sample_t();
                        } else {
                            const double tminCalib = date.mCalibration->mTmin;
                            const double tmaxCalib = date.mCalibration->mTmax;
                            const double s = std::max((date.mSettings.mTmax - date.mSettings.mTmin),
                                                      tmaxCalib - tminCalib) / 2.0;
                            ti_prop = Generator::normalDistribution(ti_old0, s);
                        }

                        const double q_fwd = date.fProposalDensity(ti_prop, ti_old0);
                        const double q_rev = date.fProposalDensity(ti_old0, ti_prop);

                        if (q_fwd <= 0.0 || q_rev <= 0.0) {
                            isvalide = false;
                        } else {
                            log_rate_q_t = std::log(q_rev) - std::log(q_fwd);
                        }
                    }

                    // ----------------------------------------------------------------------
                    // E. Vraisemblance Calibration (uniquement pour ti)
                    // ----------------------------------------------------------------------
                    if (isvalide) {
                        const double L_old = date.getLikelihood(ti_old0);
                        const double L_new = date.getLikelihood(ti_prop);

                        if (L_new <= 0.0 || L_old <= 0.0 ) {
                            isvalide = false;
                        } else {
                            log_rate_L = std::log(L_new) - std::log(L_old);
                        }
                    }

                    // ----------------------------------------------------------------------
                    // F. Calcul de la Marginal Prior Conjointe et Acceptation
                    // ----------------------------------------------------------------------
                    if (isvalide) {
                        const long double V2_new = static_cast<long double>(sigma_prop) * sigma_prop;
                        const long double w_new  = 1.0L / V2_new;
                        const long double y_new  = static_cast<long double>(ti_prop) + delta_prop;

                        long double P_new = P_removed, mu_new = mu_removed, S_new = S_removed;
                        if (P_new == 0.0L) {
                            P_new = w_new; mu_new = y_new; S_new = 0.0L;
                        } else {
                            const long double d = y_new - mu_new;
                            P_new  = P_removed + w_new;
                            mu_new = mu_removed + (w_new / P_new) * d;
                            S_new  = S_removed + w_new * d * (y_new - mu_new);
                        }
                        if (S_new < 0.0L && S_new > -1e-18L) S_new = 0.0L;

                        const long double log_sigma_i_ratio = std::log(sigma_prop / static_cast<long double>(sigma_old));
                        long double log_prior_marginal_diff = 0.0L;
                        bool degenerate = false;

                        if (min == max) {
                            const long double theta_fixed  = static_cast<long double>(min);
                            const long double residual_old = y_old - theta_fixed;
                            const long double residual_new = y_new - theta_fixed;

                            log_prior_marginal_diff = -0.5L * (residual_new * residual_new / V2_new
                                                               - residual_old * residual_old / V1_old)
                                                      - log_sigma_i_ratio;
                        } else {
                            const long double sigma_avg_old = 1.0L / std::sqrt(P_curr);
                            const long double sigma_avg_new = 1.0L / std::sqrt(P_new);

                            const long double alpha_old_min = (static_cast<long double>(min) - mu_curr) / sigma_avg_old;
                            const long double alpha_old_max = (static_cast<long double>(max) - mu_curr) / sigma_avg_old;
                            const long double alpha_new_min = (static_cast<long double>(min) - mu_new)  / sigma_avg_new;
                            const long double alpha_new_max = (static_cast<long double>(max) - mu_new)  / sigma_avg_new;

                            const double log_Z_old = log_diff_cdf(static_cast<double>(alpha_old_min), static_cast<double>(alpha_old_max));
                            const double log_Z_new = log_diff_cdf(static_cast<double>(alpha_new_min), static_cast<double>(alpha_new_max));

                            if (!std::isfinite(log_Z_old) || !std::isfinite(log_Z_new)) {
                                degenerate = true;
                            } else {
                                const long double log_sigma_avg_ratio = std::log(sigma_avg_new / sigma_avg_old);
                                log_prior_marginal_diff = static_cast<long double>(log_Z_new - log_Z_old)
                                                          + log_sigma_avg_ratio
                                                          - log_sigma_i_ratio
                                                          - 0.5L * (S_new - S_curr);
                            }
                        }

                        if (degenerate) {
                            date.mTi.reject_update();
                            date.mSigmaTi.reject_update();
                        } else {
                            const long double S02_ld = static_cast<long double>(event->mS02Theta.value());
                            const long double log_prior_shrinkage = 2.0L * (std::log(S02_ld + V1_old) - std::log(S02_ld + V2_new));

                            const long double log_rate_total = log_prior_marginal_diff
                                                               + log_prior_shrinkage
                                                               + static_cast<long double>(log_rate_L)
                                                               + static_cast<long double>(log_rate_q_t)
                                                               + static_cast<long double>(log_rate_q_s);

                            if (MHAcceptanceTest_log(static_cast<double>(log_rate_total))) {
                                date.mTi.accept_update(ti_prop);
                                date.mSigmaTi.accept_update(static_cast<double>(sigma_prop));
                                date.mDelta = delta_prop;

                                P_curr = P_new;
                                mu_curr = mu_new;
                                S_curr = S_new;
                            } else {
                                date.mTi.reject_update();
                                date.mSigmaTi.reject_update();
                            }
                        }
                    } else {
                        date.mTi.reject_update();
                        date.mSigmaTi.reject_update();
                    }
                } // Fin de boucle des dates

                // ==========================================================================
                // 5-bis. NOUVEAU : décalage rigide conjoint de theta et de tous les ti
                // ==========================================================================
#pragma mark Reparam - Décalage rigide theta + ti

                if (min != max && !event->mDates.empty()) {

                    // --- Génération de Δtheta : mélange de deux mécanismes ---
                    //
                    // (a) Saut local/large "aveugle" (inchangé) : calé sur sigma_avg
                    // courant (1/sqrt(P_curr), même quantité que le Gibbs de theta à
                    // l'étape 6), avec mélange d'échelle pour occasionnellement franchir
                    // un écart plus grand. Toujours symétrique en Δtheta -> aucun terme
                    // de Hastings. Efficace pour la crête theta/ti quand les dates
                    // s'accordent (sigma_avg petit = bonne échelle locale), ou pour
                    // franchir un désaccord franc entre deux dates (composante large).
                    //
                    // (b) Saut dirigé par une date pilote tirée au hasard dans
                    // l'événement (NOUVEAU) : Δtheta est dérivé d'un tirage
                    // d'indépendance sur cette seule date, EXACTEMENT le même mélange
                    // calibration/RW que le noyau A.1 (mMixingLevel). Comme
                    // theta_prop = theta_old + Δtheta et ti_prop,k = ti_old,k + Δtheta
                    // par construction, l'écart (ti_k - theta) reste invariant pour la
                    // date pilote elle-même, tout comme pour les autres dates : c'est
                    // toujours une translation rigide pure du système entier, le
                    // couplage hiérarchique s'annule donc exactement pour TOUTES les
                    // dates, comme en (a). Seule différence : la proposition n'étant
                    // plus symétrique, il faut le terme de Hastings de la date pilote
                    // (identique à celui déjà calculé par A.1, cf. fProposalDensity).
                    // Utile quand une ou deux dates de l'événement ont une vraisemblance
                    // de calibration très multimodale (plateau de calibration, ~30
                    // modes) : un saut aveugle N(0,sigma_avg) a très peu de chances de
                    // tomber dans un pic étroit, alors qu'un tirage direct dans la
                    // calibration de la date pilote vise un mode réellement plausible,
                    // et entraîne theta et toutes les autres dates avec lui.
                    // (c) NOUVEAU : quand (b) est tiré, sigmaTi de la date pilote propose
                    // aussi une nouvelle valeur, pour "suivre" le saut au lieu de rester
                    // figé à une échelle calée sur l'ANCIENNE position. Sans ça, après un
                    // grand saut vers un mode éloigné (courbure locale de calibration
                    // différente), sigmaTi de la pilote peut se retrouver mal adapté à la
                    // nouvelle position et mettre très longtemps à converger via les
                    // seules étapes A.1/B.2 du sweep suivant (ce qui se lit comme un
                    // blocage de sigmaTi). Le résidu r=y_pilote-theta étant EXACTEMENT
                    // invariant sous le décalage de position (theta et ti_pilote bougent
                    // du même Δtheta), seul le changement de sigma_pilote modifie le
                    // terme de couplage hiérarchique pour cette date : on peut donc
                    // calculer son ratio directement (même forme que le cas dégénéré
                    // theta-fixe de l'étape F), sans marginalisation sur theta puisque
                    // theta est ici une variable explicite du mouvement.
                    constexpr double kPilotProb      = 0.15; // proportion de sauts dirigés
                    constexpr double kWideJumpProb   = 0.10; // proportion de "grands" pas (sauts aveugles)
                    constexpr double kWideJumpFactor = 30.0; // facteur d'inflation du pas (sauts aveugles)

                    const double theta_old = event->mTheta.value();
                    double deltaTheta = 0.0;
                    double log_rate_q_shift = 0.0;
                    double log_rate_sigma_pilot = 0.0;
                    bool pilotValide = true;
                    bool pilotSigmaMoved = false;
                    size_t kPilotIdx = 0;
                    double sigma_prop_pilot = 0.0;
                    long double P_after_shift = P_curr, mu_after_shift = mu_curr, S_after_shift = S_curr;

                    if (Generator::randomUniform() < kPilotProb) {
                        kPilotIdx = std::min(event->mDates.size() - 1,
                                              static_cast<size_t>(Generator::randomUniform() * event->mDates.size()));
                        auto& pilotDate = event->mDates[kPilotIdx];
                        const double ti_old_pilot    = pilotDate.mTi.value();
                        const double sigma_old_pilot = pilotDate.mSigmaTi.value();
                        const double delta_pilot     = pilotDate.mDelta;

                        double ti_prop_pilot;
                        if (Generator::randomUniform() < pilotDate.mMixingLevel) {
                            ti_prop_pilot = *pilotDate.mCalibration->sample_t();
                        } else {
                            const double tminCalib = pilotDate.mCalibration->mTmin;
                            const double tmaxCalib = pilotDate.mCalibration->mTmax;
                            const double s = std::max((pilotDate.mSettings.mTmax - pilotDate.mSettings.mTmin),
                                                      tmaxCalib - tminCalib) / 2.0;
                            ti_prop_pilot = Generator::normalDistribution(ti_old_pilot, s);
                        }

                        const double q_fwd = pilotDate.fProposalDensity(ti_prop_pilot, ti_old_pilot);
                        const double q_rev = pilotDate.fProposalDensity(ti_old_pilot, ti_prop_pilot);
                        if (q_fwd <= 0.0 || q_rev <= 0.0) {
                            pilotValide = false;
                        } else {
                            log_rate_q_shift = std::log(q_rev) - std::log(q_fwd);
                            deltaTheta = ti_prop_pilot - ti_old_pilot;

                            // --- (c) sigmaTi de la pilote suit le saut ---
                            const long double V1_pilot = static_cast<long double>(sigma_old_pilot) * sigma_old_pilot;
                            const long double w_old_pilot = 1.0L / V1_pilot;
                            const double log10_V1_pilot = std::log10(static_cast<double>(V1_pilot));
                            const double log10_V2_pilot = Generator::normalDistribution(log10_V1_pilot, pilotDate.mSigmaTi.mSigmaMH);

                            constexpr double logVMin = -100.0;
                            constexpr double logVMax =  100.0;
                            if (!std::isfinite(log10_V2_pilot) || log10_V2_pilot < logVMin || log10_V2_pilot > logVMax) {
                                pilotValide = false;
                            } else {
                                const long double V2_pilot = std::pow(10.0L, static_cast<long double>(log10_V2_pilot));
                                sigma_prop_pilot = static_cast<double>(std::sqrt(V2_pilot));

                                // Jacobien du RW en log10(V) (identique à l'étape C)
                                log_rate_q_shift += 2.0 * std::log(sigma_prop_pilot / sigma_old_pilot);

                                // Résidu invariant sous le décalage de position
                                const long double r = static_cast<long double>(ti_old_pilot) + static_cast<long double>(delta_pilot)
                                                     - static_cast<long double>(theta_old);
                                const long double log_sigma_pilot_ratio = std::log(static_cast<long double>(sigma_prop_pilot)
                                                                                    / static_cast<long double>(sigma_old_pilot));

                                const long double log_rate_target_pilot = -0.5L * r * r * (1.0L / V2_pilot - 1.0L / V1_pilot)
                                                                          - log_sigma_pilot_ratio;

                                const long double S02_ld = static_cast<long double>(event->mS02Theta.value());
                                const long double log_prior_shrinkage_pilot = 2.0L * (std::log(S02_ld + V1_pilot) - std::log(S02_ld + V2_pilot));

                                log_rate_sigma_pilot = static_cast<double>(log_rate_target_pilot + log_prior_shrinkage_pilot);
                                pilotSigmaMoved = true;

                                // --- Candidats (P,mu,S) après décalage de position (inchangés,
                                // cf. (a)/(b)) PUIS changement du poids de la pilote (retrait/
                                // réinjection O(1)) : nécessaire pour que l'étape 6 (Gibbs de
                                // theta) utilise ensuite la bonne précision si ce mouvement est
                                // accepté. Ne sert PAS au ratio d'acceptation ci-dessus (theta
                                // est explicite dans ce mouvement, pas marginalisé).
                                const long double y_pilot_new = static_cast<long double>(ti_prop_pilot) + static_cast<long double>(delta_pilot);
                                const long double mu_shifted  = mu_curr + static_cast<long double>(deltaTheta);
                                const long double P_wo_pilot  = P_curr - w_old_pilot;

                                long double mu_wo_pilot, S_wo_pilot;
                                if (P_wo_pilot > 0.0L) {
                                    mu_wo_pilot = (P_curr * mu_shifted - w_old_pilot * y_pilot_new) / P_wo_pilot;
                                    S_wo_pilot  = S_curr - w_old_pilot * (y_pilot_new - mu_shifted) * (y_pilot_new - mu_wo_pilot);
                                    if (S_wo_pilot < 0.0L && S_wo_pilot > -1e-18L) S_wo_pilot = 0.0L;
                                } else {
                                    mu_wo_pilot = y_pilot_new;
                                    S_wo_pilot  = 0.0L;
                                }

                                const long double w_new_pilot = 1.0L / V2_pilot;
                                if (P_wo_pilot == 0.0L) {
                                    P_after_shift  = w_new_pilot;
                                    mu_after_shift = y_pilot_new;
                                    S_after_shift  = 0.0L;
                                } else {
                                    const long double d = y_pilot_new - mu_wo_pilot;
                                    P_after_shift  = P_wo_pilot + w_new_pilot;
                                    mu_after_shift = mu_wo_pilot + (w_new_pilot / P_after_shift) * d;
                                    S_after_shift  = S_wo_pilot + w_new_pilot * d * (y_pilot_new - mu_after_shift);
                                    if (S_after_shift < 0.0L && S_after_shift > -1e-18L) S_after_shift = 0.0L;
                                }
                            }
                        }
                    } else {
                        const double sigma_avg = 1.0 / std::sqrt(static_cast<double>(P_curr));
                        const double stepScale = (Generator::randomUniform() < kWideJumpProb)
                                                      ? kWideJumpFactor * sigma_avg
                                                      : sigma_avg;
                        deltaTheta = Generator::normalDistribution(0.0, stepScale);
                    }

                    const double theta_prop = theta_old + deltaTheta;

                    bool shiftValide = pilotValide && (theta_prop >= min && theta_prop <= max);

                    long double log_rate_L_shift = 0.0L;
                    std::vector<double> ti_prop_shift;

                    if (shiftValide) {
                        ti_prop_shift.reserve(event->mDates.size());
                        for (const auto& date : event->mDates) {
                            const double ti_prop_i = date.mTi.value() + deltaTheta;
                            ti_prop_shift.push_back(ti_prop_i);

                            const double L_old = date.getLikelihood(date.mTi.value());
                            const double L_new = date.getLikelihood(ti_prop_i);
                            if (!(L_old > 0.0) || !(L_new > 0.0)) {
                                shiftValide = false;
                                break;
                            }
                            log_rate_L_shift += std::log(static_cast<long double>(L_new))
                                              - std::log(static_cast<long double>(L_old));
                        }
                    }

                    const double log_rate_shift_total = static_cast<double>(log_rate_L_shift) + log_rate_q_shift + log_rate_sigma_pilot;

                    if (shiftValide && MHAcceptanceTest_log(log_rate_shift_total)) {
                        event->mTheta.accept_update(theta_prop);
                        for (size_t k = 0; k < event->mDates.size(); ++k) {
                            event->mDates[k].mTi.accept_update(ti_prop_shift[k]);
                        }
                        if (pilotSigmaMoved) {
                            event->mDates[kPilotIdx].mSigmaTi.accept_update(sigma_prop_pilot);
                            // Le poids de la pilote a changé : (P_curr,mu_curr,S_curr) ne sont
                            // plus simplement "translatés", ils doivent refléter le nouveau
                            // poids (cf. calcul des candidats ci-dessus).
                            P_curr  = P_after_shift;
                            mu_curr = mu_after_shift;
                            S_curr  = S_after_shift;
                        } else {
                            // Translation commune pure de tous les y_i et de mu : P_curr et
                            // S_curr restent invariants (les écarts (y_i - mu) sont inchangés),
                            // seul mu_curr doit suivre le décalage.
                            mu_curr += static_cast<long double>(deltaTheta);
                        }
                    } else if (pilotSigmaMoved) {
                        event->mDates[kPilotIdx].mSigmaTi.reject_update();
                    }
                    // sinon : rejet, P_curr/mu_curr/S_curr restent ceux issus de l'étape 5
                }

#pragma mark Update Theta
                // ==================================================================
                // 6. Mise à jour de theta (Gibbs exact, inchangé) - complémentaire au
                //    décalage rigide ci-dessus : affine theta localement une fois le
                //    bloc éventuellement recentré par l'étape 5-bis
                // ==================================================================
                if (min == max) {
                    qDebug() << "[ " << __func__ << QString("] Warning for event : %1 : min == max = %2")
                    .arg( event->getQStringName(), QString::number(min));
                    event->mTheta.accept_update(min);
                }
                else {
                    constexpr bool useHMC = false;
                    if (useHMC) {
                        constexpr int L_hmc = 20;
                        const double eps_hmc = (M_PI / 2.0) / L_hmc;

                        const double new_theta = hmcSampleTheta_temporal(
                                    event->mTheta.value(), mu_curr, P_curr, min, max, L_hmc, eps_hmc);

                        event->mTheta.accept_update(new_theta);
                    } else {
                        const double ti_avg_final = static_cast<double>(mu_curr);
                        const double sigma_avg_final = 1.0 / std::sqrt(static_cast<double>(P_curr));
                        const double new_theta = Generator::truncatedNormal(ti_avg_final, sigma_avg_final, min, max);

                        event->mTheta.accept_update(new_theta);
                    }
                }

                // ==================================================================
                // 7. Mise à jour des wiggles
                // ==================================================================
#pragma mark Update Wiggle
                for (auto&& date : event->mDates) {
                    date.updateWiggle();
                }

                // ==================================================================
                // 8. Mise à jour de S02Theta
                // ==================================================================
#pragma mark Update S02Theta
                if (AppSettings::mEventModel == EventModelType::EDM2) {
                    if (event->mS02Theta.mSamplerProposal != SamplerProposal::eFixe) {
                        event->updateS02Theta_v338();
                    }
                }

                // ==================================================================
                // 9. Mise à jour des bornes des phases de l'événement
                // ==================================================================
#pragma mark Update Phases
                std::for_each(
                    event->mPhases.begin(),
                    event->mPhases.end(),
                    [this](std::shared_ptr<Phase> p) {
                        p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                    });
            }
        }

        // ======================================================================
        // 10. Mises à jour globales des phases
        // ======================================================================
        std::for_each(
            mModel->mPhases.begin(),
            mModel->mPhases.end(),
            [this](std::shared_ptr<Phase> p) {
                p->update_Tau(
                    tminPeriod,
                    tmaxPeriod);
            });

        // ======================================================================
        // 11. Mise à jour globale des contraintes de phases
        // ======================================================================
        std::for_each( mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                      [](std::shared_ptr<PhaseConstraint> pc) {
                          pc->updateGamma();
                      });
    }
    catch (const char* e) {
        qWarning() << "[" << __func__ << "] char " << e;
        return;
    }
    catch (const std::length_error& e) {
        qWarning() << "[" << __func__ << "] length_error " << e.what();
        return;
    }
    catch (const std::out_of_range& e) {
        qWarning() << "[" << __func__ << "] out_of_range " << e.what();
        return;
    }
    catch (const std::exception& e) {
        qWarning() << "[" << __func__ << "] " << e.what();
        return;
    }
    catch (...) {
        qWarning() << "[" << __func__ << "] Caught Exception!";
        return;
    }
}