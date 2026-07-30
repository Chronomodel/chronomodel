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

#include "ModelUtilities.h"
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

QString MCMCLoopChrono::calibrate()
{
    if (mModel) {
        std::vector<std::shared_ptr<Event>> &events = mModel->mEvents;
        //events.reserve(mModel->mEvents.size());
        //----------------- Calibrate measurements --------------------------------------

        QList<Date*> dates;
        // find number of dates, to optimize memory space
        /*int nbDates = 0;
        for (auto&& e : events)
            nbDates += e->mDates.size();

        dates.reserve(nbDates);*/

        for (std::shared_ptr<Event>& ev : events) {
            unsigned long num_dates = ev->mDates.size();
            for (unsigned long j = 0; j<num_dates; ++j) {
                Date* date = &ev->mDates[j];
                dates.push_back(date);
            }
        }


        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepChanged(tr("Calibrating..."), 0, (int)dates.size());

        int i = 0;
        for (auto&& date : dates) {
              if (date->mCalibration) {
                if (date->mCalibration->mVector.empty())
                    date->calibrate(getProject_ptr());

                if (date->mCalibration->mVector.size() < 5) {
                    const double new_step = date->mCalibration->mStep/5.;
                    date->mCalibration->mVector.clear();
                    date->mCalibration->mMap.clear();
                    date->mCalibration->mRepartition.clear();
                    date->mCalibration = nullptr;

                    const QString mes = tr("Insufficient resolution for the Event %1 \r Decrease the step in the study period box to %2").arg(date->getQStringName(), QString::number(new_step));
                    return (mes);

                }

              } else
                  return (tr("Invalid Model -> No Calibration on Data %1").arg(date->getQStringName()));


            if (isInterruptionRequested())
                return ABORTED_BY_USER;

            emit stepProgressed(i);
            ++i;

        }
        dates.clear();
        return QString();

    }
    return tr("Invalid model");
}


QString MCMCLoopChrono::initialize()
{
    return initialize_time();
}

bool MCMCLoopChrono::update_v3()
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

            event->updateTheta_v3(tminPeriod, tmaxPeriod);

            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
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


    return true;

}


/**
 *
 * https://en.wikipedia.org/wiki/Simulated_annealing

 */

bool MCMCLoopChrono::update_v338_simulated_annealing()
{
    const int iteration =  mLoopChains[ mChainIndex].mTotalIter;

    const int max_expo_T   = mModel->mMCMCSettings.mAnnealTemp;

    bool do_regeneration = (iteration> 0 && iteration % mModel->mMCMCSettings.mAnnealRecurrence == 0);
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
            if (mModel->mEvents[j]->mTheta.mSamplerProposal != MHVariable::eFixe) {
                event_regenerated.push_back(mModel->mEvents[j]);   // copie du shared_ptr
            }
        }

        // --------------------------------------------------------------
        // 3️⃣  Fonction générique
        // --------------------------------------------------------------

        auto MH_all_temp = [&](double T) -> double
        {
            int n_accepted = 0;
            int n_total    = 0;

            //std::size_t j = 0;
            for (auto &event : event_regenerated) {
                try {
                    //if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
                    // Recuit simulé hiérarchique
                    // π(θ) ∝ exp(-H(θ)/T₁) × ∏ᵢ exp(-Hᵢ(θᵢ)/Tᵢ)
                    // Toutes les variables n'ont pas besoin d'avoir la même température de recuit
                    //if (event_regenerated[j]) {
                    // --------------------------------------------------------------
                    //  A - Update ti Dates (idem MCMCLoopChrono)
                    // --------------------------------------------------------------
                    try {
                        for (auto&& date : event->mDates) {
                            date.applyDate(event->mTheta.value(), event->mS02Theta.value());
                        }

                    }  catch (...) {
                        qWarning() <<"[MCMCLoopCurve::update_v338_simulated_annealing] MH_all_temp-> update Date ???";
                    }

                    const double min = event->getThetaMin(tminPeriod);
                    const double max = event->getThetaMax(tmaxPeriod);
                    if (min >= max)
                        throw QObject::tr("[Event::update_v338_simulated_annealing] Error for event : %1 : min = %2 : max = %3")
                            .arg(event->getQStringName(), QString::number(min), QString::number(max));

                    double sum_p = 0.0;
                    double sum_t = 0.0;

                    for (auto&& date: event->mDates) {
                        const double variance  = pow(date.mSigmaTi.value(), 2);
                        sum_t += (date.mTi.value() + date.mDelta) / variance;
                        sum_p += 1.0 / variance;
                    }
                    const double ti_avg = sum_t / sum_p;
                    const double sigma = 1.0 / sqrt(sum_p);

                    //échantillonneur A
                    //double try_theta = Generator::truncatedNormal(event->mTheta.value(), event->mTheta.mSigmaMH * T, min, max);

                    // échantillonneur B avec un pas fonction de T
                    double sigma_proposal = sigma * T;
                    double try_theta = Generator::truncatedNormal(ti_avg, sigma_proposal, min, max);
                    // Ratio MH : la proposition n'est pas symétrique → corriger par q(x|x')/q(x'|x)
                    //double q_yx = dnorm(try_theta, ti_avg, sigma * T);
                    //double q_xy = dnorm(event->mTheta.value(), ti_avg, sigma * T);

                    double ln_q_yx = log_dnorm(try_theta, ti_avg, sigma_proposal);
                    double ln_q_xy = log_dnorm(event->mTheta.value(), ti_avg, sigma_proposal);


                    //const double rate = (dnorm(theta_try, ti_avg, sigma) / dnorm(event-mTheta.value(), ti_avg, sigma))
                    //                  * (q_backward / q_forward);                   // correction
                    // test MH

                    // ou
                    //échantillonneur C
                    //double try_theta = Generator::truncatedNormal(ti_avg, sigma, min, max);
                    // ou

                    //échantillonneur D -- d'origine
                    // double try_theta = Generator::randomUniform(min, max);


                    //constexpr double q_yx = 1;
                    //constexpr double q_xy = 1;

                    // On fait le rapport MH avec la conditionnelle, donc on peut choisir l'un ou l'autre des échantillonneurs symétriques A ou B
                    // le D est peut-être plus efficace pour essayer des valeurs loins


                    /*double pi_x = dnorm(event->mTheta.value(), ti_avg, sigma );
                            double pi_y = dnorm(try_theta, ti_avg, sigma );

                            double rate = (pi_y*q_xy) / (pi_x*q_yx) ;

                            double Hx = -log(pi_x * q_yx);
                            double Hy = -log(pi_y * q_xy);

                            double rT = exp(-(Hy-Hx) / T);

                            double ln_rT = log(rate)/T;*/

                    double log_alpha =
                        (log_dnorm(try_theta, ti_avg, sigma)
                         - log_dnorm(event->mTheta.value(), ti_avg, sigma)) ;

                    log_alpha /= T;

                    // Dans un recuit simulé correctement formulé, c'est seulement la cible (l'énergie H(θ)) qu'on tempère par 1/T,
                    //   pas la correction de proposition asymétrique
                    log_alpha += (ln_q_yx - ln_q_xy) ; // SI Echantillonneur différent de Uniform D,  //

                    if (MHAcceptanceTest_log(log_alpha)) {
                        event->mTheta.setValue(try_theta);
                        event->mThetaReduced = mModel->reduceTime(try_theta);
                        // accepté
                        ++n_accepted;

                    }

                    ++n_total;

                    //} else
                    //   event->applyThetaProposal_v3(tminPeriod, tmaxPeriod);

                    //}
                    // On ne bouge pas les bornes
                }
                catch (const std::exception &e) {
                    qWarning() << "[MCMCLoopChrono::update_v4_simulated_annealing] Tempering error on event"
                               << event->getQStringName() << ":" << e.what();
                }

                if (AppSettings::mEventModel == EventModelType::EDM2 ) {
                    if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                        event->applyS02Theta();
                }


                std::for_each(event->mPhases.begin(),
                              event->mPhases.end(),
                              [this](std::shared_ptr<Phase> p) {
                                  p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                              });


                // ++j;
            }

            // Mise à jour globale des phases
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

            // Retourne un taux entre 0.0 et 1.0
            return (n_total > 0) ? static_cast<double>(n_accepted) / n_total : 1.0;
        };

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
                MH_all_temp(T);

        }
    }

    // ------------------------------------------------------------------
    // 6️⃣  Mise à jour standard de tous les events
    // ------------------------------------------------------------------
    for (auto &event : mModel->mEvents) {
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {

            event->updateTheta_v3(tminPeriod, tmaxPeriod);

            if (AppSettings::mEventModel == EventModelType::EDM2 ) {
                if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                    event->updateS02Theta_v338();
            }
            std::for_each(event->mPhases.begin(),
                          event->mPhases.end(),
                          [this](std::shared_ptr<Phase> p) {
                              p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                          });
        }
    }

    // Mise à jour globale des phases (Tau + contraintes)
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

bool MCMCLoopChrono::update_v4_simulated_annealing()
{
    const int iteration =  mLoopChains[ mChainIndex].mTotalIter;

    const int max_expo_T   = mModel->mMCMCSettings.mAnnealTemp;

    bool do_regeneration = (iteration> 0 && iteration % mModel->mMCMCSettings.mAnnealRecurrence == 0);
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
            if (mModel->mEvents[j]->mTheta.mSamplerProposal != MHVariable::eFixe) {
                event_regenerated.push_back(mModel->mEvents[j]);   // copie du shared_ptr
            }
        }

        // --------------------------------------------------------------
        // 3️⃣  Fonction générique
        // --------------------------------------------------------------

        auto MH_all_temp = [&](double T) -> double
        {
            int n_accepted = 0;
            int n_total    = 0;

            //std::size_t j = 0;
            for (auto &event : event_regenerated) {
                try {
                    //if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
                    // Recuit simulé hiérarchique
                    // π(θ) ∝ exp(-H(θ)/T₁) × ∏ᵢ exp(-Hᵢ(θᵢ)/Tᵢ)
                    // Toutes les variables n'ont pas besoin d'avoir la même température de recuit
                    //if (event_regenerated[j]) {
                    // --------------------------------------------------------------
                    //  A - Update ti Dates (idem MCMCLoopChrono)
                    // --------------------------------------------------------------


                    event->applyTheta(tminPeriod, tmaxPeriod, T);


                    ++n_total;

                    // On ne bouge pas les bornes
                }
                catch (const std::exception &e) {
                    qWarning() << "[MCMCLoopChrono::update_v4_simulated_annealing] Tempering error on event"
                               << event->getQStringName() << ":" << e.what();
                }

                if (AppSettings::mEventModel == EventModelType::EDM2 ) {
                    if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                        event->applyS02Theta();
                }


                std::for_each(event->mPhases.begin(),
                              event->mPhases.end(),
                              [this](std::shared_ptr<Phase> p) {
                                  p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                              });


            }

            // Mise à jour globale des phases
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

            // Retourne un taux entre 0.0 et 1.0
            return (n_total > 0) ? static_cast<double>(n_accepted) / n_total : 1.0;
        };

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
                MH_all_temp(T);

        }
    }

    // ------------------------------------------------------------------
    // 6️⃣  Mise à jour standard de tous les events
    // ------------------------------------------------------------------
    for (auto &event : mModel->mEvents) {
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {

            event->updateTheta_v4(tminPeriod, tmaxPeriod);

            if (AppSettings::mEventModel == EventModelType::EDM2 ) {
                if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                    event->updateS02Theta_v4();
            }
            std::for_each(event->mPhases.begin(),
                          event->mPhases.end(),
                          [this](std::shared_ptr<Phase> p) {
                              p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                          });
        }
    }

    // Mise à jour globale des phases (Tau + contraintes)
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

bool MCMCLoopChrono::update_v3_block_simulated_annealing()
{
    const int iteration =  mLoopChains[ mChainIndex].mTotalIter;

    const int max_expo_T   = mModel->mMCMCSettings.mAnnealTemp;

    bool do_regeneration = (iteration > 0
                            && iteration % mModel->mMCMCSettings.mAnnealRecurrence == 0
                            && mState == eAquisition);

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
            if (mModel->mEvents[j]->mTheta.mSamplerProposal != MHVariable::eFixe) {
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

                tempering_339(event_regenerated, T);

        }


    }

    // ------------------------------------------------------------------
    // 6️⃣  Mise à jour standard de tous les events
    // ------------------------------------------------------------------

    sampler_339(mModel->mEvents);


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


void MCMCLoopChrono::tempering_339(std::vector<std::shared_ptr<Event>> &events, double T)
{
    for (auto &event : events) {
        try {


            // Evaluation des bornes de theta
            const double min = event->getThetaMin(tminPeriod);
            const double max = event->getThetaMax(tmaxPeriod);

            if (min > max) {
                throw QObject::tr("[Event::tempering_339] Error for event : %1 : min = %2 : max = %3")
                .arg(event->getQStringName(), QString::number(min), QString::number(max));
            }

            // Gestion du cas min == max
            if (min == max) {
                event->mTheta.setValue(min);
                event->mThetaReduced = mModel->reduceTime(min);

                // 1. Noyau 1 : Mises à jour individuelles des dates (ti) à theta FIXE
                // Il ne faut pas mémorisé la date, le recuit est un état transitoire
                for (auto&& date : event->mDates) {
                    date.applyTi(event->mTheta.value());
                }
            }
            else {
                // 1. Noyau 1 : Mises à jour individuelles des dates (ti) à theta FIXE
                // Il ne faut pas mémorisé la date, le recuit est un état transitoire
                for (auto&& date : event->mDates) {
                    date.applyTi(event->mTheta.value());
                }
                // ----------------------------------------
                // Calcul de θ moyen local et σ
                // ----------------------------------------
                double sum_p = 0.0;
                double sum_t = 0.0;
                for (auto&& date : event->mDates) {
                    const double var = pow(date.mSigmaTi.value(), 2.);
                    sum_t += (date.mTi.value() + date.mDelta) / var;
                    sum_p += 1.0 / var;
                }

                const double ti_avg = sum_t / sum_p;
                const double sigma  = 1.0 / std::sqrt(sum_p);

                // Le pas de proposition s'élargit à haute température T
                double sigma_proposal = event->mTheta.mSigmaMH * T;
                double old_theta = event->mTheta.value();
                double prop_theta = Generator::truncatedNormal(old_theta, sigma_proposal, min, max);


                // Ratio de Hastings pour la loi tronquée
                double num = normalCDF((max - event->mTheta.value()) / sigma_proposal) - normalCDF((min - event->mTheta.value()) / sigma_proposal);
                double den = normalCDF((max - prop_theta) / sigma_proposal) - normalCDF((min - prop_theta) / sigma_proposal);
                double log_support_ratio = std::log(num) - std::log(den);

                // 4. Variation du Prior a posteriori de theta (ex: 0.0 si uniforme)
                double log_prior_diff = log_dnorm(prop_theta, ti_avg, sigma) - log_dnorm(old_theta, ti_avg, sigma);

                // 5. Critère d'acceptation de Metropolis-Hastings
                // ATTENTION : Seul le Prior/Vraisemblance est divisé par T, PAS le ratio entier de Hastings !
                double log_rate = (log_prior_diff / T) + log_support_ratio;


                // Calcul du ti_avg et sigma local
                /*             double sum_p = 0.0;
                    double sum_t = 0.0;
                    for (auto&& date : event->mDates) {
                        const double var = std::pow(date.mSigmaTi.value(), 2.0);
                        sum_t += (date.mTi.value() + date.mDelta) / var;
                        sum_p += 1.0 / var;
                    }

                    const double ti_avg = sum_t / sum_p;
                    const double sigma_ti_avg = 1.0 / std::sqrt(sum_p);
                    const double old_theta = event->mTheta.value();


                    // -------------------------------------------------------------------------
                    // MOUVEMENT : SAUT GLOBAL DE THETA (ti FIXES) -> Invariance de la Vraisemblance
                    // -------------------------------------------------------------------------
                    double sigma_prop = sigma_ti_avg * T;
                    double prop_theta = Generator::truncatedNormal(ti_avg, sigma_prop, min, max);

                    // Hastings ratio pour la proposition ti_avg sans translation de ti
                    double log_gaussian_ratio = -2.0 * (prop_theta - old_theta) * (ti_avg - old_theta) / (sigma_prop * sigma_prop);

                    double Z_forward = normalCDF((max - ti_avg) / sigma_prop) - normalCDF((min - ti_avg) / sigma_prop);
                    double Z_reverse = normalCDF((max - prop_theta) / sigma_prop) - normalCDF((min - prop_theta) / sigma_prop);
                    double log_truncation_ratio = std::log(Z_forward) - std::log(Z_reverse);

                    double log_support_ratio = log_gaussian_ratio + log_truncation_ratio;

                    // Vraisemblance inchangée car ti ne bougent pas (log_rate_L = 0)
                    // Seul le Prior hiérarchique ou la température s'applique
                    double log_alpha = log_support_ratio / T;
*/
                if (MHAcceptanceTest_log(log_rate)) {
                    event->mTheta.setValue(prop_theta);
                    event->mThetaReduced = mModel->reduceTime(prop_theta);
                    // On NE TOUCHE PAS aux dates ti ici !
                }



            }
            // 4. Mises à jour des hyperparamètres
            for (auto&& date : event->mDates) {
                date.applyDelta(event->mTheta.value(), event->mS02Theta.value());
                date.applySigma(event->mTheta.value(), event->mS02Theta.value());
                date.applyWiggle();
            }

        } catch (const std::exception &e) {
            qWarning() << "[MCMCLoopChrono::tempering_339] Tempering error on event"
                       << event->getQStringName() << ":" << e.what();
        }

        if (AppSettings::mEventModel == EventModelType::EDM2) {
            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
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
}

//Metropolis-Hastings par blocs)
void MCMCLoopChrono::sampler_339(std::vector<std::shared_ptr<Event>> &events)
{
    // ------------------------------------------------------------------
    // Mise à jour MCMC adaptative standard de tous les événements
    // ------------------------------------------------------------------
    for (auto &event : events) {

        // Initialement Cas de Event seul, sans contrainte
        // Finalement, se comporte bien en cas de contrainte
        // Echantillonnage multi-paramètre
        // On propose directement les ti dans leurs vraisemblances et les theta dans l'a priori Event
        // On teste surtout le ration des troncatures
       /* if (event->mTheta.mSamplerProposal != MHVariable::eFixe
            && event->mConstraintsBwd.empty()
            && event->mConstraintsFwd.empty() && true) {*/
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {

            // Évaluation des bornes de theta
            const double min = event->getThetaMin(tminPeriod);
            const double max = event->getThetaMax(tmaxPeriod);

            if (min > max) {
                throw QObject::tr("[MCMCLoopChrono::sampler_339] Error for event : %1 : min = %2 : max = %3")
                .arg(event->getQStringName(), QString::number(min), QString::number(max));
            }

            // 1. Proposition des ti avec leurs vraissemblance L


            std::vector<double> old_ti;
            old_ti.reserve(event->mDates.size());
            std::vector<double> prop_ti;
            prop_ti.reserve(event->mDates.size());

            double log_rate_L = 0.0; // Rapport de vraisemblance divisé par T
            double log_rate_q = 0.0; // Ratio de Hastings des proposals de ti

            for (auto&& date : event->mDates) {
                const double t0 = date.mTi.value();
                old_ti.push_back(t0);
                double tiNew;

                if (Generator::randomUniform() < date.mMixingLevel) {
                    const double tminCalib = date.mCalibration->mTmin;
                    const double u = Generator::randomUniform();
                    const double idx = interpolate_index(u, date.mCalibration->mRepartition);
                    tiNew = tminCalib + idx * date.mCalibration->mStep;
                } else {
                    const double s = (date.mSettings.mTmax - date.mSettings.mTmin) / 2.0;
                    tiNew = Generator::normalDistribution(t0, s);
                }

                prop_ti.push_back(tiNew);

                // Vraisemblance (divisée par T si Simulated Annealing)
                const double L_old = date.getLikelihood(t0);
                const double L_new = date.getLikelihood(tiNew);
                if (L_new <= 0.0 || L_old <= 0.0) return; // Sécurité hors support

                log_rate_L += (std::log(L_new) - std::log(L_old));// / T;

                // Proposal (Non divisé par T)
                // Correct : fProposalDensity(cible, départ)
                const double q_fwd = date.fProposalDensity(tiNew, t0); // q(tiNew | t0)
                const double q_rev = date.fProposalDensity(t0, tiNew); // q(t0 | tiNew)
                log_rate_q += std::log(q_rev) - std::log(q_fwd);
            }


            // 2. Proposition de theta avec le Prior
            double prop_theta;
            // Si min = max, on force l'acceptation de theta et des ti
            if (min == max) {
                qDebug() << QObject::tr("[MCMCLoopChrono::sampler_339] Warning for event : %1 : min == max = %2")
                .arg(event->getQStringName(), QString::number(min));

                event->mTheta.accept_update(min);
                size_t i = 0;
                for (auto&& date : event->mDates) {
                    date.mTi.accept_update(prop_ti[i++]);
                }
            }
            else {
                // -------------------------------------------------------------------------
                // Moyennes, dispersions S(t) et constantes de troncature Z
                // -------------------------------------------------------------------------
                // ----------------------------------------
                // Calcul des ti moyen et σ
                // ----------------------------------------

                double sum_p = 0.0;
                double sum_t_old = 0.0;
                double sum_t_prop = 0.0;
                size_t idx = 0;

                for (auto&& date : event->mDates) {
                    const double var = std::pow(date.mSigmaTi.value(), 2.0);
                    sum_t_old += (old_ti[idx] + date.mDelta) / var;
                    sum_t_prop += (prop_ti[idx] + date.mDelta) / var;
                    sum_p += 1.0 / var;
                    idx++;
                }

                const double sigma  = 1.0 / std::sqrt(sum_p);
                const double ti_avg_old = sum_t_old / sum_p;
                const double ti_avg_prop = sum_t_prop / sum_p;

                // Calcul de S(t_old) et S(t_prop)
                double S_old = 0.0;
                double S_prop = 0.0;
                idx = 0;

                for (auto&& date : event->mDates) {
                    const double var = std::pow(date.mSigmaTi.value(), 2.0);
                    S_old += std::pow(old_ti[idx] + date.mDelta - ti_avg_old, 2.0) / var;
                    S_prop += std::pow(prop_ti[idx] + date.mDelta - ti_avg_prop, 2.0) / var;
                    idx++;
                }

                // Ratio des constantes de troncature
                const double Z_old = normalCDF((max - ti_avg_old) / sigma) - normalCDF((min - ti_avg_old) / sigma);
                const double Z_prop = normalCDF((max - ti_avg_prop) / sigma) - normalCDF((min - ti_avg_prop) / sigma);
                const double log_Z_ratio = std::log(Z_prop) - std::log(Z_old);

                // -------------------------------------------------------------------------
                // Log-rate global d'acceptation
                // -------------------------------------------------------------------------
                const double log_prior_marginal_diff = log_Z_ratio - 0.5 * (S_prop - S_old);
                const double log_rate_total = log_rate_L + log_rate_q + log_prior_marginal_diff;

                // Échantillonnage de prop_theta à partir du nouvel état prop_ti
                prop_theta = Generator::truncatedNormal(ti_avg_prop, sigma, min, max);

                // Décision Metropolis-Hastings conjointe
                if (event->mTheta.try_update_log(prop_theta, log_rate_total)) {
                    size_t i = 0;
                    for (auto&& date : event->mDates) {
                        date.mTi.accept_update(prop_ti[i++]);
                    }
                } else {
                    for (auto&& date : event->mDates) {
                        date.mTi.reject_update();
                    }
                }
            }
            /*else if (event->mDates.size() == 1) {
                auto& date = event->mDates[0];
                const double t0 = date.mTi.value();
                const double sigma = date.mSigmaTi.value();
                const double delta = date.mDelta;

                //date.mMixingLevel = 1; // test

                // 1. Échantillonnage de tiNew
                double tiNew;
                if (Generator::randomUniform() < date.mMixingLevel) {
                    const double tminCalib = date.mCalibration->mTmin;
                    const double u = Generator::randomUniform();
                    const double idx = interpolate_index(u, date.mCalibration->mRepartition);
                    tiNew = tminCalib + idx * date.mCalibration->mStep;

                } else {
                    const double s = (date.mSettings.mTmax - date.mSettings.mTmin) / 2.0;
                    tiNew = Generator::normalDistribution(t0, s);
                }

                // 2. Évaluation des vraisemblances
                const double L_old = date.getLikelihood(t0);
                const double L_new = date.getLikelihood(tiNew);
                if (L_new <= 0.0 || L_old <= 0.0) return; // Sécurité hors support

                // Vraisemblance + Ratio de Hastings de ti
                const double log_rate_L = (std::log(L_new) - std::log(L_old)); // / T;
                // Correct : fProposalDensity(cible, départ)
                const double q_fwd = date.fProposalDensity(tiNew, t0); // q(tiNew | t0)
                const double q_rev = date.fProposalDensity(t0, tiNew); // q(t0 | tiNew)

                const double log_rate_q = std::log(q_rev) - std::log(q_fwd);

                // 3. Constantes de troncature Z (S(t) est nul car N = 1)
                const double ti_avg_old = t0 + delta;
                const double ti_avg_prop = tiNew + delta;

                const double Z_old = normalCDF((max - ti_avg_old) / sigma) - normalCDF((min - ti_avg_old) / sigma);
                const double Z_prop = normalCDF((max - ti_avg_prop) / sigma) - normalCDF((min - ti_avg_prop) / sigma);
                const double log_Z_ratio = std::log(Z_prop) - std::log(Z_old);

                // 4. Log-rate global d'acceptation
                const double log_rate_total = log_rate_L + log_rate_q + log_Z_ratio;

                // 5. Proposition de prop_theta selon p(theta | tiNew)
                const double prop_theta = Generator::truncatedNormal(ti_avg_prop, sigma, min, max);

                // 6. Mise à jour conjointe
                if (event->mTheta.try_update_log(prop_theta, log_rate_total)) {
                    date.mTi.accept_update(tiNew);
                } else {
                    date.mTi.reject_update();
                }
            }
            else {
                // ----------------------------------------
                // Calcul de σ
                // ----------------------------------------

                double sum_p = 0.0;
                for (auto&& date : event->mDates) {
                    const double var = std::pow(date.mSigmaTi.value(), 2.0);

                    sum_p += 1.0 / var;

                }
                const double sigma  = 1.0 / std::sqrt(sum_p);

                // 1. Tirage direct de prop_ti selon la vraisemblance L(ti)
                // ici il manque la partie mixing et troncature Z
                double prop_ti = sample_in_repartition(event->mMixingCalibrations, min, max);

                // 2. Tirage direct de theta selon son prior p(theta | prop_ti)
                double prop_theta = Generator::truncatedNormal(prop_ti, sigma, min, max);

                event->mTheta.setValue(prop_theta);
                event->mDates[0].mTi.setValue(prop_ti);


            }*/
    //std::cout << "sampling mix " << prop_ti_avg << std::endl;







            // 3. Mise à jour des hyperparamètres, wiggles et EDM2 pour l'événement courant
            for (auto&& date : event->mDates) {
                date.updateDelta(event->mTheta.value(), event->mS02Theta.value());
                date.updateSigma(event->mTheta.value(), event->mS02Theta.value());
                date.updateWiggle();
            }

            if (AppSettings::mEventModel == EventModelType::EDM2) {
                if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                    event->updateS02Theta_v338();
            }

            // Mise à jour des bornes des phases de l'événement
            std::for_each(event->mPhases.begin(), event->mPhases.end(),
                          [this](std::shared_ptr<Phase> p) {
                              p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                          });
        }


        // échantillonnage par move block
        //else if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
        /*else if (false) {
            // 1. Noyau 1 : Mise à jour individuelle des dates ti
            // Le taux d'acceptation est modifie ici , si besoin
            for (auto&& date : event->mDates) {
                date.updateTi(event->mTheta.value());
            }

            // Évaluation des bornes de theta
            const double min = event->getThetaMin(tminPeriod);
            const double max = event->getThetaMax(tmaxPeriod);

            if (min > max) {
                throw QObject::tr("[MCMCLoopChrono::sampler_339] Error for event : %1 : min = %2 : max = %3")
                .arg(event->getQStringName(), QString::number(min), QString::number(max));
            }

            bool accepted = false;
            double epsilon = 0.0;

            // 2. Noyau 2 : Proposition et test Metropolis-Hastings pour theta
            if (min == max) {
                // FIX BUG : Cas du bloc figé (On applique la translation sans stopper l'exécution de la boucle)
                epsilon = min - event->mTheta.value();
                event->mTheta.accept_update(min);
                for (auto&& date : event->mDates) {
                    date.mTi.setValue(date.mTi.value() + epsilon);
                }
            }
            else {
                // Proposition de translation de bloc pour theta
                double prop_theta = Generator::truncatedNormal(event->mTheta.value(), event->mTheta.mSigmaMH, min, max);

                // Calcul du ratio de Hastings pour la loi tronquée
                double num = normalCDF((max - event->mTheta.value()) / event->mTheta.mSigmaMH) - normalCDF((min - event->mTheta.value()) / event->mTheta.mSigmaMH);
                double den = normalCDF((max - prop_theta) / event->mTheta.mSigmaMH) - normalCDF((min - prop_theta) / event->mTheta.mSigmaMH);
                double log_support_ratio = std::log(num) - std::log(den);

                epsilon = prop_theta - event->mTheta.value();

                // Évaluation Cible : Log-Vraisemblance avec sécurité numérique
                double log_rate_L = 0.0;
                bool valid_proposal = true;

                for (size_t i = 0; i < event->mDates.size(); ++i) {
                    double prop_ti = event->mDates[i].mTi.value();
                    double ti_star = prop_ti + epsilon;

                    double L_new = event->mDates[i].getLikelihood(ti_star);
                    double L_old = event->mDates[i].getLikelihood(prop_ti);

                    // FIX SÉCURITÉ : Anti-log(0) / NaN
                    if (L_new <= 0.0 || L_old <= 0.0) {
                        valid_proposal = false;
                        break;
                    }

                    log_rate_L += std::log(L_new) - std::log(L_old);
                }

                // Décision Metropolis-Hastings avec enregistrement des statistiques adaptatives
                if (valid_proposal) {
                    accepted = event->mTheta.try_update_log(prop_theta, log_rate_L + log_support_ratio);
                } else {
                    // Transmet -inf pour acter le REJET dans les compteurs d'adaptation de mTheta
                    accepted = event->mTheta.try_update_log(prop_theta, -std::numeric_limits<double>::infinity());
                }

                // Application de la translation physique aux dates si la proposition est acceptée
                if (accepted) {
                    for (auto&& date : event->mDates) {
                        date.mTi.setValue(date.mTi.value() + epsilon);
                    }
                }
            }

            // 3. Mise à jour des hyperparamètres, wiggles et EDM2 pour l'événement courant
            for (auto&& date : event->mDates) {
                date.updateDelta(event->mTheta.value(), event->mS02Theta.value());
                date.updateSigma(event->mTheta.value(), event->mS02Theta.value());
                date.updateWiggle();
            }

            if (AppSettings::mEventModel == EventModelType::EDM2) {
                if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                    event->updateS02Theta_v338();
            }

            // Mise à jour des bornes des phases de l'événement
            std::for_each(event->mPhases.begin(), event->mPhases.end(),
                          [this](std::shared_ptr<Phase> p) {
                              p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                          });
        }*/
    }

    // 4. Mises à jour globales des phases et contraintes du modèle
    std::for_each(mModel->mPhases.begin(), mModel->mPhases.end(),
                  [this](std::shared_ptr<Phase> p) {
                      p->update_Tau(tminPeriod, tmaxPeriod);
                  });

    std::for_each(mModel->mPhaseConstraints.begin(), mModel->mPhaseConstraints.end(),
                  [](std::shared_ptr<PhaseConstraint> pc) {
                      pc->updateGamma();
                  });
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
                if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
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


/*
bool MCMCLoopChrono::adapt(const int batchIndex) //original code
{

    // Pour un Random‑Walk Metropolis en une dimension, la théorie (Roberts et al., 1997) montre que le taux optimal est ≈ 0.44
    const double taux_min = 0.42;
    const double taux_max = 0.46;

    //
    // En haute dimension (> 5), le taux optimal se rapproche de 0.23.
    //   Si vous avez des vecteurs de grande dimension, il serait judicieux de réduire la fenêtre (ex. 0.20‑0.30).
    //
    //const double taux_min = 0.20;
    //const double taux_max = 0.30;

    bool noAdapt = true;

    //--------------------- Adapt -----------------------------------------

    //const double delta = (batchIndex < 10000) ? 0.01 : (1.0 / sqrt(batchIndex));

    const double delta = 0.5 * std::pow(static_cast<double>(batchIndex+1), -0.5);

    for (const auto& event : mModel->mEvents) {
       for (auto& date : event->mDates) {

            //--------------------- Adapt Sigma MH de t_i -----------------------------------------
            if (date.mTi.mSamplerProposal == MHVariable::eMHAdaptGauss)
                noAdapt = date.mTi.adapt(taux_min, taux_max, delta) && noAdapt;

            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            if (date.mSigmaTi.mSamplerProposal == MHVariable::eMHAdaptGauss)
                noAdapt = date.mSigmaTi.adapt(taux_min, taux_max, delta) && noAdapt;

        }

        //--------------------- Adapt Sigma MH de Theta Event -----------------------------------------
       if ((event->mType != Event::eBound) && ( event->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss) )
           noAdapt = event->mTheta.adapt(taux_min, taux_max, delta) && noAdapt;

#ifdef S02_BAYESIAN
       if ( event->mS02Theta.mSamplerProposal == MHVariable::eMHAdaptGauss)
            noAdapt = event->mS02Theta.adapt(taux_min, taux_max, delta) && noAdapt;
#endif
    }


    return noAdapt;
}
*/

bool MCMCLoopChrono::adapt(const int batchIndex)
{

    // Pour un Random‑Walk Metropolis en une dimension, la théorie (Roberts et al., 1997) montre que le taux optimal est ≈ 0.44
    const double taux_min = 0.42;
    const double taux_max = 0.46;

    //
    /* En haute dimension (> 5), le taux optimal se rapproche de 0.23.
        Si vous avez des vecteurs de grande dimension, il serait judicieux de réduire la fenêtre (ex. 0.20‑0.30).
    */
    //const double taux_min = 0.20;
    //const double taux_max = 0.30;

    bool noAdapt = true;

    //--------------------- Adapt -----------------------------------------


    for (const auto& event : mModel->mEvents) {
        for (auto& date : event->mDates) {

            //--------------------- Adapt Sigma MH de t_i -----------------------------------------
            if (date.mTi.mSamplerProposal == MHVariable::eMHAdaptGauss)
                noAdapt = date.mTi.adapt(taux_min, taux_max, batchIndex) && noAdapt;

            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------
            if (date.mSigmaTi.mSamplerProposal == MHVariable::eMHAdaptGauss)
                noAdapt = date.mSigmaTi.adapt(taux_min, taux_max, batchIndex) && noAdapt;

        }

        //--------------------- Adapt Sigma MH de Theta Event -----------------------------------------
        if ((event->mType != Event::eBound) && ( event->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss) )
            noAdapt = event->mTheta.adapt(taux_min, taux_max, batchIndex) && noAdapt;

        if (AppSettings::mEventModel == EventModelType::EDM2 ) {
            if ( event->mS02Theta.mSamplerProposal == MHVariable::eMHAdaptGauss)
                noAdapt = event->mS02Theta.adapt(taux_min, taux_max, batchIndex) && noAdapt;
        }
    }


    return noAdapt;
}


/*void MCMCLoopChrono::memo()
{
    for (auto& event : mModel->mEvents) {
        //--------------------- Memo Events -----------------------------------------
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
            event->mTheta.memo();
            event->mTheta.saveCurrentAcceptRate();
            for (auto&& date : event->mDates )   {
                //--------------------- Memo Dates -----------------------------------------
                date.mTi.memo();
                date.mSigmaTi.memo();
                date.mWiggle.memo();

                date.mTi.saveCurrentAcceptRate();
                date.mSigmaTi.saveCurrentAcceptRate();
            }

            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
                double memoS02 = sqrt(event->mS02Theta.mX);
                event->mS02Theta.memo(&memoS02);
                event->mS02Theta.saveCurrentAcceptRate();
            }

        }


    }

    //--------------------- Memo Phases -----------------------------------------
    for (auto& ph : mModel->mPhases)
            ph->memoAll();

}*/


void MCMCLoopChrono::acquire()
{
    for (auto& event : mModel->mEvents) {
        //--------------------- Memo Events -----------------------------------------
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
            event->mTheta.acquire();
            for (auto&& date : event->mDates ) {
                //--------------------- Memo Dates -----------------------------------------
                date.mTi.acquire();
                date.mSigmaTi.acquire();
                date.mWiggle.acquire();
            }

        }

        if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
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
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
            event->mTheta.recordBurnAdapt();
            for (auto&& date : event->mDates )   {
                //--------------------- Memo Dates -----------------------------------------
                date.mTi.recordBurnAdapt();
                date.mSigmaTi.recordBurnAdapt();
                date.mWiggle.recordBurnAdapt();
            }
        }
        if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
            double memoS02 = sqrt(event->mS02Theta.value());
            event->mS02Theta.recordBurnAdapt(&memoS02);

        }
    }

    //--------------------- Memo Phases -----------------------------------------
    for (auto& ph : mModel->mPhases)
        ph->recordBurnAdapt();

}

void MCMCLoopChrono::recordMH()
{
    for (auto& event : mModel->mEvents) {
        //--------------------- Memo Events -----------------------------------------
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {

            event->mTheta.saveCurrentAcceptRate();
            for (auto&& date : event->mDates )   {
                //--------------------- Memo Dates -----------------------------------------
                date.mTi.saveCurrentAcceptRate();
                date.mSigmaTi.saveCurrentAcceptRate();
            }

            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
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
