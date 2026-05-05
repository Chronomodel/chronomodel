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
        if(event->mTheta.mEmpiricalCDFReady) {
            event->updateThetaPriorCDE(tminPeriod, tmaxPeriod);

        } else {
            event->updateTheta_v3(tminPeriod, tmaxPeriod);
        }


#ifdef S02_BAYESIAN
        if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
            event->updateS02Theta();
#endif
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


bool MCMCLoopChrono::learn_v3_tempering()
{
    int iteration =  mLoopChains[ mChainIndex].mTotalIter;

    //const double u = Generator::randomUniform();
    //constexpr double w_regenerate = 0.3;   // probabilité de régénération de la chaine

    constexpr int    max_expo_T   = 1000;    // nombre d’étapes de température
    //constexpr double w_event = 0.5;   // probabilité de régénération de l'Event

    bool do_regeneration = (iteration % 10 == 0); // (u < w_regenerate)
    // ------------------------------------------------------------------
    // 1️⃣  Décision de régénération
    // ------------------------------------------------------------------
    if (do_regeneration) {
        // --------------------------------------------------------------
        // 2️⃣  Sélection aléatoire des événements à régénérer
        // --------------------------------------------------------------
        std::vector<bool> event_regenerated(mModel->mEvents.size(), true);
       /* for (std::size_t j = 0; j < event_regenerated.size(); ++j) {
            if (mModel->mEvents[j]->mTheta.mSamplerProposal != MHVariable::eFixe) { // On ne bouge pas les bornes
                const double u2 = Generator::randomUniform();
                event_regenerated[j] = true;//static_cast<bool>(u2 < w_event);
            }
        }*/

        // --------------------------------------------------------------
        // 3️⃣  Fonction générique
        // --------------------------------------------------------------

        auto MH_all_temp = [&](double expo_T)
        {
            std::size_t j = 0;
            //double T = std::pow(2, 0.1*expo_T); // a) Exponentiel décroit (α > 1)
            double T = std::pow(2, expo_T);
            //double factor = 1.0 / (1.0 + 0.5 * (max_expo_T - expo_T));
            //double T = factor;
            for (auto &event : mModel->mEvents) {
                try {
                    if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
                        if (event_regenerated[j]) {
                            // --------------------------------------------------------------
                            //  A - Update ti Dates (idem MCMCLoopChrono)
                            // --------------------------------------------------------------
                            try {
                                for (auto&& date : event->mDates) {
                                    date.applyDateProposal_v3(event->mTheta.value(), event->mS02Theta.value(), event->mAShrinkage);

                                    date.updateDelta(event->mTheta.value()); // pas de memo

                                    date.applySigmaShrinkage_K_tempering(event->mTheta.value(), event->mS02Theta.value(), event->mAShrinkage, 1);

                                    date.updateWiggle(); // mise à jour déterministe, pas de tirage date.updateDate(event->mTheta.mX, event->mS02Theta.mX, event->mAShrinkage);

                                }

                            }  catch (...) {
                                qWarning() <<"[MCMCLoopCurve::update_v3_tempering] MH_all_temp-> update Date ???";
                            }

                            const double min = event->getThetaMin(tminPeriod);
                            const double max = event->getThetaMax(tmaxPeriod);
                            if (min >= max)
                                throw QObject::tr("[Event::applyTheta_v6_MH_Tempering] Error for event : %1 : min = %2 : max = %3")
                                    .arg(event->getQStringName(), QString::number(min), QString::number(max));

                            double sum_p = 0.0;
                            double sum_t = 0.0;

                            for (auto&& date: event->mDates) {
                                const double variance  = pow(date.mSigmaTi.mX, 2);
                                sum_t += (date.mTi.mX + date.mDelta) / variance;
                                sum_p += 1.0 / variance;
                            }
                            const double ti_avg = sum_t / sum_p;
                            const double sigma = 1.0 / sqrt(sum_p);

                            //échantillonneur A
                            //double try_theta = Generator::truncatedNormal(event->mTheta.value(), event->mTheta.mSigmaMH * T, min, max);

                            //échantillonneur B

                           //double T_sigma = T;
                           //double try_theta = Generator::truncatedNormal(ti_avg, sigma * T_sigma, min, max);
                           double try_theta = Generator::randomUniform( min, max);

                           // ou
                            //échantillonneur C
                            //double try_theta = Generator::truncatedNormal(ti_avg, sigma, min, max);

                            // On fait le rapport MH avec la conditionnelle, donc on peut choisir l'un ou l'autre des échantillonneurs symétriques A ou B
                            // le A est peut-être plus efficace pour essayer des valeurs loins

                            // Recuit simulé hiérarchique
                            // π(θ) ∝ exp(-H(θ)/T₁) × ∏ᵢ exp(-Hᵢ(θᵢ)/Tᵢ)
                          /* double pi_x = dnorm(event->mTheta.value(), ti_avg, sigma );
                           double pi_y = dnorm(try_theta, ti_avg, sigma );

                           double q_yx = 1;//dnorm(try_theta, ti_avg, sigma * T_sigma);
                           double q_xy = 1;//dnorm(event->mTheta.value(), ti_avg, sigma * T_sigma);

                           double rate = (pi_y*q_xy) / (pi_x*q_yx) ;

                           double Hx = -log(pi_x * q_yx);
                           double Hy = -log(pi_y * q_xy);

                           double rT = exp(-(Hy-Hx) / T);

                           double ln_rT = log(rate)/T;
*/
                            double log_alpha =
                                (log_dnorm(try_theta, ti_avg, sigma)
                                 - log_dnorm(event->mTheta.value(), ti_avg, sigma)) ;

                           /*log_alpha +=
                               (-log_dnorm(try_theta, ti_avg, sigma * sqrt(T))
                                          + log_dnorm(event->mTheta.value(), ti_avg, sigma * sqrt(T)));
*/

                            if (MHAcceptanceTest_log(log_alpha /T)) {
                           //if (MHAcceptanceTest(exp(ln_rT))) {
                           // if (MHAcceptanceTest(rT)) {
                                event->mTheta.setValue(try_theta);
                                event->mThetaReduced = mModel->reduceTime(try_theta);

                            }


                        } else
                            event->applyThetaProposal_v3(tminPeriod, tmaxPeriod);

                    }
                    // On ne bouge pas les bornes
                }
                catch (const std::exception &e) {
                    qWarning() << "[MCMCLoopChrono::update_v3_tempering] Tempering error on event"
                               << event->getQStringName() << ":" << e.what();
                }

                if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                    event->applyS02Theta(1);

                std::for_each(event->mPhases.begin(),
                              event->mPhases.end(),
                              [this](std::shared_ptr<Phase> p) {
                                  p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                              });


                ++j;
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
        };

        // --------------------------------------------------------------
        // 5️⃣  Descente (T décroissant)
        // --------------------------------------------------------------
        for (int i = max_expo_T - 1; i >= 0; --i) {
            MH_all_temp(static_cast<double>(i));
        }


    }
    else {
        // ------------------------------------------------------------------
        // 6️⃣  Pas de régénération → mise à jour standard de tous les events
        // ------------------------------------------------------------------
        for (auto &event : mModel->mEvents) {
            if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {

                const double u = Generator::randomUniform();
                //if( u > 0.5) {
                    event->updateTheta_v3(tminPeriod, tmaxPeriod);

               // } else {
                  //  event->updateTheta_v4(tminPeriod, tmaxPeriod);
              //  }
               // std::cout<< "sampling " << event->mTheta.mName << " " << event->mTheta.value() << std::endl;

                if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                    event->updateS02Theta();

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
    }

    return !do_regeneration; //true;
}

bool MCMCLoopChrono::update_v3_tempering()
{
    int iteration =  mLoopChains[ mChainIndex].mTotalIter;

    //const double u = Generator::randomUniform();
    //constexpr double w_regenerate = 0.3;   // probabilité de régénération de la chaine

    constexpr int max_expo_T   = 100;    // nombre d’étapes de température
    //constexpr double w_event = 0.5;   // probabilité de régénération de l'Event

    bool do_regeneration = (iteration % 500 == 0); // (u < w_regenerate)
    // ------------------------------------------------------------------
    // 1️⃣  Décision de régénération
    // ------------------------------------------------------------------
    if (do_regeneration) {
        // --------------------------------------------------------------
        // 2️⃣  Sélection aléatoire des événements à régénérer
        // --------------------------------------------------------------
        std::vector<bool> event_regenerated(mModel->mEvents.size(), true);
        /* for (std::size_t j = 0; j < event_regenerated.size(); ++j) {
            if (mModel->mEvents[j]->mTheta.mSamplerProposal != MHVariable::eFixe) { // On ne bouge pas les bornes
                const double u2 = Generator::randomUniform();
                event_regenerated[j] = true;//static_cast<bool>(u2 < w_event);
            }
        }*/

        // --------------------------------------------------------------
        // 3️⃣  Fonction générique
        // --------------------------------------------------------------

        auto MH_all_temp = [&](double T)
        {
            std::size_t j = 0;

            //double factor = 1.0 / (1.0 + 0.5 * (max_expo_T - expo_T));
            //double T = factor;
            for (auto &event : mModel->mEvents) {
                try {
                    if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {
                        // Recuit simulé hiérarchique
                        // π(θ) ∝ exp(-H(θ)/T₁) × ∏ᵢ exp(-Hᵢ(θᵢ)/Tᵢ)
                        // Toutes les variables n'ont pas besoin d'avoir la même température de recuit
                        if (event_regenerated[j]) {
                            // --------------------------------------------------------------
                            //  A - Update ti Dates (idem MCMCLoopChrono)
                            // --------------------------------------------------------------
                            try {
                                for (auto&& date : event->mDates) {
                                    date.applyDateProposal_v3(event->mTheta.value(), event->mS02Theta.value(), event->mAShrinkage);
                                }

                            }  catch (...) {
                                qWarning() <<"[MCMCLoopCurve::update_v3_tempering] MH_all_temp-> update Date ???";
                            }

                            const double min = event->getThetaMin(tminPeriod);
                            const double max = event->getThetaMax(tmaxPeriod);
                            if (min >= max)
                                throw QObject::tr("[Event::applyTheta_v6_MH_Tempering] Error for event : %1 : min = %2 : max = %3")
                                    .arg(event->getQStringName(), QString::number(min), QString::number(max));

                            double sum_p = 0.0;
                            double sum_t = 0.0;

                            for (auto&& date: event->mDates) {
                                const double variance  = pow(date.mSigmaTi.mX, 2);
                                sum_t += (date.mTi.mX + date.mDelta) / variance;
                                sum_p += 1.0 / variance;
                            }
                            const double ti_avg = sum_t / sum_p;
                            const double sigma = 1.0 / sqrt(sum_p);

                            //échantillonneur A
                            //double try_theta = Generator::truncatedNormal(event->mTheta.value(), event->mTheta.mSigmaMH * T, min, max);

                            //échantillonneur B avec un pas fonction de T
                            //double T_sigma = 1.;// T;
                            //double try_theta = Generator::truncatedNormal(ti_avg, sigma * T_sigma, min, max);
                            //double q_yx = dnorm(try_theta, ti_avg, sigma * T_sigma);
                            //double q_xy = dnorm(event->mTheta.value(), ti_avg, sigma * T_sigma);
                            //
                            // échantillonneur qui utilise la CDE
                            //const double try_theta = event->mTheta.sampleFromEmpiricalPrior(min, max);

                            // Ratio MH : la proposition n'est pas symétrique → corriger par q(x|x')/q(x'|x)
                            //const double q_yx  = event->mTheta.evalEmpiricalPrior(try_theta);  // q(x'|x)
                            //const double q_xy = event->mTheta.evalEmpiricalPrior(event->mTheta.value()); // q(x|x')

                            //const double rate = (dnorm(theta_try, ti_avg, sigma) / dnorm(event-mTheta.value(), ti_avg, sigma))
                              //                  * (q_backward / q_forward);                   // correction
                            // test MH

                            // ou
                            //échantillonneur C
                            //double try_theta = Generator::truncatedNormal(ti_avg, sigma, min, max);
                            // ou
                            //échantillonneur D
                            double try_theta = Generator::randomUniform(min, max);
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

                            //log_alpha += (log(q_yx) - log(q_xy)) ;

                            if (MHAcceptanceTest_log(log_alpha /T)) {
                            //if (MHAcceptanceTest(exp(ln_rT))) {
                                // if (MHAcceptanceTest(rT)) {
                                event->mTheta.setValue(try_theta);
                                event->mThetaReduced = mModel->reduceTime(try_theta);

                            }


                        } else
                            event->applyThetaProposal_v3(tminPeriod, tmaxPeriod);

                    }
                    // On ne bouge pas les bornes
                }
                catch (const std::exception &e) {
                    qWarning() << "[MCMCLoopChrono::update_v3_tempering] Tempering error on event"
                               << event->getQStringName() << ":" << e.what();
                }

                if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                    event->applyS02Theta(1);

                std::for_each(event->mPhases.begin(),
                              event->mPhases.end(),
                              [this](std::shared_ptr<Phase> p) {
                                  p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                              });


                ++j;
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
        };

        // --------------------------------------------------------------
        // 5️⃣  Descente (T décroissant)
        // --------------------------------------------------------------
        for (int e = max_expo_T ; e >= 0; --e) {
            const double T = std::pow(2, e); // a) Exponentiel décroit (α > 1)
            MH_all_temp(T);
        }


    }

    // ------------------------------------------------------------------
    // 6️⃣  Mise à jour standard de tous les events
    // ------------------------------------------------------------------
    for (auto &event : mModel->mEvents) {
        if (event->mTheta.mSamplerProposal != MHVariable::eFixe) {

            event->updateTheta_v3(tminPeriod, tmaxPeriod);

            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                event->updateS02Theta();

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

// obsolete
// ne permet pas le deplacement le taux MH reste petit quand on est loin du courrant
bool MCMCLoopChrono::update_v3_simulated_tempering_annealing()
{
    const double u = Generator::randomUniform();
    constexpr double w_regenerate = 0.7;   // probabilité de régénération
    constexpr int    max_i        = 1000;    // nombre d’étapes de température
    constexpr int nb_simulation   = 10;

    // ------------------------------------------------------------------
    // 1️⃣  Décision de régénération
    // ------------------------------------------------------------------
    if (u < w_regenerate) {

        // --------------------------------------------------------------
        // 3️⃣  Fonction générique d’update (montée ou descente)
        // --------------------------------------------------------------
        auto update_all = [&](double T)
        {
            for (auto &event : mModel->mEvents) {
                try {
                        event->applyTheta_v6_MH_Tempering(tminPeriod, tmaxPeriod, T);

                }
                catch (const std::exception &e) {
                    qWarning() << "Tempering error on event"
                               << event->getQStringName() << ":" << e.what();
                }

                if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                    event->updateS02Theta();

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
        };

        // --------------------------------------------------------------
        // 4️⃣  Montée (T croissant)
        // --------------------------------------------------------------
       // for (int i = 0; i <= max_i; ++i)
         //   for (int j = 0; j <= nb_simulation; ++j)
           //     update_all(static_cast<double>(i));

        // --------------------------------------------------------------
        // 5️⃣  Descente (T décroissant)
        // --------------------------------------------------------------
        for (int i = max_i - 1; i >= 0; --i)
            for (int j = 0; j <= nb_simulation; ++j)
                update_all(static_cast<double>(i));
    }
    else {
        // ------------------------------------------------------------------
        // 6️⃣  Pas de régénération → mise à jour standard de tous les events
        // ------------------------------------------------------------------
        for (auto &event : mModel->mEvents) {
            event->updateTheta_v3(tminPeriod, tmaxPeriod);

            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                event->updateS02Theta();

            std::for_each(event->mPhases.begin(),
                          event->mPhases.end(),
                          [this](std::shared_ptr<Phase> p) {
                              p->update_AlphaBeta(tminPeriod, tmaxPeriod);
                          });
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
    }

    return !(u < w_regenerate); //true;
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
#ifdef S02_BAYESIAN
            if (event->mS02Theta.mSamplerProposal != MHVariable::eFixe)
                event->updateS02Theta();
#endif
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

#ifdef S02_BAYESIAN
        if ( event->mS02Theta.mSamplerProposal == MHVariable::eMHAdaptGauss)
            noAdapt = event->mS02Theta.adapt(taux_min, taux_max, batchIndex) && noAdapt;
#endif
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
            double memoS02 = sqrt(event->mS02Theta.mX);
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
            double memoS02 = sqrt(event->mS02Theta.mX);
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
    //mModel->generatePosteriorDensities(mChains, 1024, 1);

    // Generate numerical results of :
    // - MHVariables (global acceptation)
    // - MetropolisVariable : analysis of Posterior densities and quartiles from traces.
    // This also should be done in results view...
    //mModel->generateNumericalResults(mChains);

#ifdef DEBUG
    QTime endTime = QTime::currentTime();

    qDebug()<<"Model computed";
    qDebug()<<QString("MCMCLoopChrono::finalize() finish at %1").arg(endTime.toString("hh:mm:ss.zzz")) ;
    qDebug()<<QString("Total time elapsed %1").arg(DHMS(startTime.elapsed()));
#endif
}
