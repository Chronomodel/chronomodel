/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "MCMCLoopChronocurve.h"
#include "ModelChronocurve.h"
#include "EventKnown.h"
#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "Date.h"
#include "Project.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "../PluginAbstract.h"
#include "CalibrationCurve.h"

#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QTime>


MCMCLoopChronocurve::MCMCLoopChronocurve(ModelChronocurve* model, Project* project):MCMCLoop(),
mModel(model)
{
    mProject = project;
    if (mModel){
        setMCMCSettings(mModel->mMCMCSettings);
    }
    
    QJsonObject state = project->mState;
    mChronocurveSettings = ChronocurveSettings::fromJson(state.value(STATE_CHRONOCURVE).toObject());
}

MCMCLoopChronocurve::~MCMCLoopChronocurve()
{
    mModel = nullptr;
    mProject = nullptr;
}

#pragma mark MCMC Loop Overloads

/**
 * Idem Chronomodel + prepareEventsY() qui sert à corriger les données d'entrées de Chronocurve.
 * (Calcul de Yx, Yy, Yz et de Sy)
 */
QString MCMCLoopChronocurve::calibrate()
{
    if (mModel)
    {
        QList<Event*>& events = mModel->mEvents;
        events.reserve(mModel->mEvents.size());
        
        //----------------- Calibrate measurements --------------------------------------

        QList<Date*> dates;
        // find number of dates, to optimize memory space
        int nbDates (0);
        for (auto &&e : events)
            nbDates += e->mDates.size();

        dates.reserve(nbDates);
        for (auto &&ev : events) {
            int num_dates = ev->mDates.size();
            for (int j=0; j<num_dates; ++j) {
                Date* date = &ev->mDates[j];
                dates.push_back(date);
            }
        }
        

        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepChanged(tr("Calibrating..."), 0, dates.size());

        int i (0);
        for (auto &&date : dates) {
              if (date->mCalibration) {
                if (date->mCalibration->mCurve.isEmpty())
                    date->calibrate(mModel->mSettings, mProject);
                } else
                    return (tr("Invalid Model -> No Calibration on Data %1").arg(date->mName));


            if (isInterruptionRequested())
                return ABORTED_BY_USER;

            emit stepProgressed(i);
            ++i;

        }
        dates.clear();
        
        prepareEventsY();
        
        return QString();
    }
    return tr("Invalid model");
}

/**
 * Idem Chronomodel + initialisation des variables aléatoires VG (events) et Alpha Lissage (global)
 * TODO : initialisation des résultats g(t), g'(t), g"(t)
 */
void MCMCLoopChronocurve::initVariablesForChain()
{
    // today we have the same acceptBufferLen for every chain
    const int acceptBufferLen =  mChains[0].mNumBatchIter;
    int initReserve (0);
    
    for (const ChainSpecs c: mChains){
       initReserve += ( 1 + (c.mMaxBatchs*c.mNumBatchIter) + c.mNumBurnIter + (c.mNumRunIter/c.mThinningInterval) );
    }
    
    for (Event* event : mModel->mEvents) {
        
        event->mTheta.reset();
        event->mTheta.reserve(initReserve);
        event->mTheta.mLastAccepts.reserve(acceptBufferLen);
        event->mTheta.mLastAcceptsLength = acceptBufferLen;
        
        event->mVG.reset();
        event->mVG.reserve(initReserve);
        event->mVG.mLastAccepts.reserve(acceptBufferLen);
        event->mVG.mLastAcceptsLength = acceptBufferLen;

        // event->mTheta.mAllAccepts.clear(); //don't clean, avalable for cumulate chain

        for (Date& date : event->mDates) {
            date.mTheta.reset();
            date.mTheta.reserve(initReserve);
            date.mTheta.mLastAccepts.reserve(acceptBufferLen);
            date.mTheta.mLastAcceptsLength = acceptBufferLen;

            date.mSigma.reset();
            date.mSigma.reserve(initReserve);
            date.mSigma.mLastAccepts.reserve(acceptBufferLen);
            date.mSigma.mLastAcceptsLength = acceptBufferLen;

            date.mWiggle.reset();
            date.mWiggle.reserve(initReserve);
            date.mWiggle.mLastAccepts.reserve(acceptBufferLen);
            date.mWiggle.mLastAcceptsLength = acceptBufferLen;
        }
    }
    
    mModel->mAlphaLissage.reset();
    mModel->mAlphaLissage.reserve(initReserve);
    mModel->mAlphaLissage.mLastAccepts.reserve(acceptBufferLen);
    mModel->mAlphaLissage.mLastAcceptsLength = acceptBufferLen;
    
    // TODO : init container for g(t) => spectrogram
}

/**
 * Idem Chronomodel + initialisation de VG (events) et Alpha Lissage (global)
 */
QString MCMCLoopChronocurve::initMCMC()
{
    QList<Event*>& events(mModel->mEvents);
    const double tmin = mModel->mSettings.mTmin;
    const double tmax = mModel->mSettings.mTmax;

    if (isInterruptionRequested())
        return ABORTED_BY_USER;

    // ---------------------- Reset Events ---------------------------
    for (Event* ev : events){
        ev->mInitialized = false;
    }

    // ----------------------------------------------------------------
    //  Init theta event, ti, ...
    // ----------------------------------------------------------------
    QVector<Event*> unsortedEvents = ModelUtilities::unsortEvents(events);
    
    emit stepChanged(tr("Initializing Events..."), 0, unsortedEvents.size());
    
    for (auto e : unsortedEvents)
    {
        mModel->initNodeEvents();
        QString circularEventName = "";
        QList<Event*> startEvents = QList<Event*>();

        const bool ok (e->getThetaMaxPossible (e, circularEventName, startEvents));
        qDebug() << " MCMCLoopMain::InitMCMC check constraint" << e->mName << ok;
        if (!ok) {
            mAbortedReason = QString(tr("Warning : Find Circular Constraint Path %1  %2 ")).arg (e->mName, circularEventName);
            return mAbortedReason;
        }
    }

    for (int i (0); i<unsortedEvents.size(); ++i) {
        if (unsortedEvents.at(i)->mType == Event::eDefault) {

            qDebug() << "in initMCMC(): ---------------------------------";
            mModel->initNodeEvents(); // ?? FAIT AU-DESSUS ?
            QString circularEventName = "";
            QList< Event*> startEvents = QList<Event*>();
            const double min (unsortedEvents.at(i)->getThetaMinRecursive (tmin));
            
            // ?? Comment circularEventName peut-il être pas vide ?
            if (!circularEventName.isEmpty()) {
                mAbortedReason = QString(tr("Warning : Find Circular constraint with %1  bad path  %2 ")).arg(unsortedEvents.at(i)->mName, circularEventName);
                return mAbortedReason;
            }

            //qDebug() << "in initMCMC(): Event initialised min : " << unsortedEvents[i]->mName << " : "<<" min"<<min<<tmin;
            mModel->initNodeEvents();
            const double max ( unsortedEvents.at(i)->getThetaMaxRecursive(tmax) );
#ifdef DEBUG
            if (min >= max)
                qDebug() << tr("-----Error Init for event : %1 : min = %2 : max = %3-------").arg(unsortedEvents.at(i)->mName, QString::number(min), QString::number(max));
#endif
            
            // ----------------------------------------------------------------
            // Chronocurve init Theta event :
            // ----------------------------------------------------------------
            if(mChronocurveSettings.mTimeType == ChronocurveSettings::eModeFixed)
            {
                // Dans le cas theta fixe (pas de Bayésien),
                // On initialise les theta event à la valeur médiane de la calibration de leur première date.
                // La valeur médiane est retrouvée grâce à Q2
                
                const Date& firstDate = unsortedEvents.at(i)->mDates[0];
                
                Quartiles quartiles = quartilesForRepartition(
                    firstDate.mCalibration->mRepartition,
                    firstDate.mCalibration->mTmin,
                    firstDate.mCalibration->mStep
                );
                
                unsortedEvents.at(i)->mTheta.mX = quartiles.Q2;
            }
            else
            {
                unsortedEvents.at(i)->mTheta.mX = Generator::randomUniform(min, max);
            }
            
            unsortedEvents.at(i)->mInitialized = true;

            // ----------------------------------------------------------------

            double s02_sum (0.);
            for (int j=0; j<unsortedEvents.at(i)->mDates.size(); ++j) {
                Date& date = unsortedEvents.at(i)->mDates[j];

                // 1 - Init ti
                double sigma;
                if (!date.mCalibration->mRepartition.isEmpty()) {
                    const double idx = vector_interpolate_idx_for_value(Generator::randomUniform(), date.mCalibration->mRepartition);
                    date.mTheta.mX = date.mCalibration->mTmin + idx *date.mCalibration->mStep;
                  //  qDebug()<<"MCMCLoopMain::Init"<<date.mName <<" mThe.mx="<<QString::number(date.mTheta.mX, 'g', 15);

                    FunctionAnalysis data = analyseFunction(vector_to_map(date.mCalibration->mCurve, tmin, tmax, date.mCalibration->mStep));
                    sigma = double (data.stddev);
                }
                else { // in the case of mRepartion curve is null, we must init ti outside the study period
                       // For instance we use a gaussian random sampling
                    sigma = mModel->mSettings.mTmax - mModel->mSettings.mTmin;
                    const double u = Generator::gaussByBoxMuller(0., sigma);
                    if (u<0)
                        date.mTheta.mX = mModel->mSettings.mTmin + u;
                    else
                        date.mTheta.mX = mModel->mSettings.mTmax + u;

                    if (date.mMethod == Date::eInversion) {
                        qDebug()<<"Automatic sampling method exchange eInversion to eMHSymetric for"<< date.mName;
                        date.mMethod = Date::eMHSymetric;
                        date.autoSetTiSampler(true);
                    }

                }
                // 2 - Init Delta Wiggle matching and Clear mLastAccepts array
                date.initDelta(unsortedEvents.at(i));
                date.mWiggle.memo();
                date.mWiggle.mLastAccepts.push_back(true);
                date.mWiggle.saveCurrentAcceptRate();

                // 3 - Init sigma MH adaptatif of each Data ti
                date.mTheta.mSigmaMH = sigma;

                // 4 - Clear mLastAccepts array and set this init at 100%
                date.mTheta.mLastAccepts.clear();
                date.mTheta.mLastAccepts.push_back(true);

                // 5 - Memo
                date.mTheta.memo();

                date.mTheta.saveCurrentAcceptRate();

                // intermediary calculus for the harmonic average
                s02_sum += 1. / (sigma * sigma);
            }

            // 4 - Init S02 of each Event
            unsortedEvents.at(i)->mS02 = unsortedEvents.at(i)->mDates.size() / s02_sum;// /100;

            // 5 - Init sigma MH adaptatif of each Event with sqrt(S02)
            unsortedEvents.at(i)->mTheta.mSigmaMH = sqrt(unsortedEvents.at(i)->mS02);
            unsortedEvents.at(i)->mAShrinkage = 1.;

            // 6- Clear mLastAccepts array
            unsortedEvents.at(i)->mTheta.mLastAccepts.clear();
            unsortedEvents.at(i)->mTheta.mLastAccepts.push_back(true);
            
            // 7 - Memo
            unsortedEvents.at(i)->mTheta.memo();
            unsortedEvents.at(i)->mTheta.saveCurrentAcceptRate();
            
            // ----------------------------------------------------------------
            // Chronocurve init VG :
            // ----------------------------------------------------------------
            Event* event = unsortedEvents.at(i);
            if(mChronocurveSettings.mVarianceType == ChronocurveSettings::eModeFixed){
                event->mVG.mX = mChronocurveSettings.mVarianceFixed;
            }else{
                event->mVG.mX = 1.;
            }
            
            event->mVG.mSigmaMH = 1.;
            event->mVG.mLastAccepts.clear();
            event->mVG.mLastAccepts.push_back(true);
            event->mVG.memo();
            event->mVG.saveCurrentAcceptRate();
            
            // ----------------------------------------------------------------
            //  Les W des events ne dépendant que de leur VG
            //  Lors de l'update, on a besoin de W pour les calculs de mise à jour de theta, VG et Alpha lissage
            //  On sera donc amenés à remettre le W à jour à chaque modification de VG
            //  On le calcul ici lors de l'initialisation pour avoir sa valeur de départ
            // ----------------------------------------------------------------
            event->updateW();
        }

        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepProgressed(i);
    }

    // ----------------------------------------------------------------
    //  Init sigma i and its sigma MH
    // ----------------------------------------------------------------
    QString log;
    emit stepChanged(tr("Initializing Variances..."), 0, events.size());

    for (int i=0; i<events.size(); ++i) {
        for (int j=0; j<events.at(i)->mDates.size(); ++j) {
            Date& date = events.at(i)->mDates[j];

            // date.mSigma.mX = sqrt(shrinkageUniform(events[i]->mS02)); // modif the 2015/05/19 with PhL
            date.mSigma.mX = std::abs(date.mTheta.mX - (events.at(i)->mTheta.mX - date.mDelta));

            if (date.mSigma.mX<=1E-6) {
               date.mSigma.mX = 1E-6; // Add control the 2015/06/15 with PhL
               log += line(date.mName + textBold("Sigma indiv. <=1E-6 set to 1E-6"));
            }
            date.mSigma.mSigmaMH = 1.;

            date.mSigma.mLastAccepts.clear();
            date.mSigma.mLastAccepts.push_back(true);

            date.mSigma.memo();
            date.mSigma.saveCurrentAcceptRate();

        }
        if (isInterruptionRequested())
            return ABORTED_BY_USER;

        emit stepProgressed(i);
    }
    
    // ----------------------------------------------------------------
    //  Init alpha lissage
    // ----------------------------------------------------------------
    if(mChronocurveSettings.mCoeffLissageType == ChronocurveSettings::eModeFixed){
        mModel->mAlphaLissage.mX = mChronocurveSettings.mAlphaLissage;
    }else{
        mModel->mAlphaLissage.mX = 1e-6;
    }
    
    mModel->mAlphaLissage.mSigmaMH = 1.;
    mModel->mAlphaLissage.mLastAccepts.clear();
    mModel->mAlphaLissage.mLastAccepts.push_back(true);
    mModel->mAlphaLissage.memo();
    mModel->mAlphaLissage.saveCurrentAcceptRate();
    

    // --------------------------- Log Init ---------------------
    log += "<hr>";
    log += textBold("Events Initialization (with their data)");

    for (int i=0; i<events.size(); ++i) {
        
        const Event* event = events[i];
        
        log += "<hr><br>";
        
        log += line(textBlue(tr("Event ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(events.size()), event->mName)));
        log += line(textBlue(tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(event->mTheta.mX), DateUtils::getAppSettingsFormatStr())));
        log += line(textBlue(tr(" - Sigma_MH on Theta : %1").arg(stringForLocal(event->mTheta.mSigmaMH))));
        log += line(textBlue(tr(" - S02 : %1").arg(stringForLocal(event->mS02))));
        
        int j (0);
        for (const Date & date : event->mDates) {
            ++j;
            log += "<br>";
            
            log += line(textBlack(tr("Data ( %1 / %2 ) : %3").arg(QString::number(j), QString::number(event->mDates.size()), date.mName)));
            log += line(textBlack(tr(" - ti : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(date.mTheta.mX), DateUtils::getAppSettingsFormatStr())));
            if (date.mMethod == Date::eMHSymGaussAdapt)
                log += line(textBlack(tr(" - Sigma_MH on ti : %1").arg(stringForLocal(date.mTheta.mSigmaMH))));

            log += line(textBlack(tr(" - Sigma_i : %1").arg(stringForLocal(date.mSigma.mX))));
            log += line(textBlack(tr(" - Sigma_MH on Sigma_i : %1").arg(stringForLocal(date.mSigma.mSigmaMH))));
            if (date.mDeltaType != Date::eDeltaNone)
                log += line(textBlack(tr(" - Delta_i : %1").arg(stringForLocal(date.mDelta))));

        }
    }

    mInitLog += "<hr>";
    mInitLog += textBold(tr("INIT CHAIN %1").arg(QString::number(mChainIndex+1)));
    mInitLog += "<hr>";
    mInitLog += log;

    return QString();
}

/**
 * Idem Chronomodel pour les Dates
 * Pour les events, theta a une nouvelle composante et est donc entièrement revu
 * Les events doivent aussi mettre VG à jour (sur le même principe que theta)
 * Idem pour le alpha lissage global
 * Ces 3 mises à jour font intervenir le calcul matriciel (cravate, spline, etc...)
 */
void MCMCLoopChronocurve::update()
{
    const double t_max (mModel->mSettings.mTmax);
    const double t_min (mModel->mSettings.mTmin);

    ChainSpecs& chain = mChains[mChainIndex];
    const bool doMemo = (mState == eBurning) || (mState == eAdapting) || (chain.mTotalIter % chain.mThinningInterval == 0);
    
    // --------------------------------------------------------------
    //  Update Dates (idem chronomodel)
    // --------------------------------------------------------------
    for(Event* event : mModel->mEvents)
    {
        for(Date& date : event->mDates)
        {
            date.updateDelta(event);
            date.updateTheta(event);
            date.updateSigma(event);
            date.updateWiggle();

            if (doMemo) {
                date.mTheta.memo();
                date.mSigma.memo();
                date.mWiggle.memo();

                date.mTheta.saveCurrentAcceptRate();
                date.mSigma.saveCurrentAcceptRate();
            }
        }
    }
    
    // --------------------------------------------------------------
    //  Pour les calculs qui suivent, on travaille en temps réduit.
    //  (nécessaire pour les calculs spline)
    // --------------------------------------------------------------
    // TODO : convertir tous les theta event entre 0 et 1
    
    // --------------------------------------------------------------
    //  Update theta Events
    // --------------------------------------------------------------
    if(mChronocurveSettings.mTimeType == ChronocurveSettings::eModeBayesian)
    {
        orderEventsByTheta();
        saveEventsTheta();
        spreadEventsTheta();
        
        double h_current = h_YWI_AY() * h_alpha() * h_theta();
        
        restoreEventsTheta();
        
        for(Event* event : mModel->mEvents)
        {
            // ----------------------------------------------------------------------
            //  Dans Chronomodel, on appelle simplement : event->updateTheta(t_min,t_max);
            //  Dans Chronocurve, on la formule de mise à jour a une composante supplémentaire.
            //  Pour mettre theta à jour, on doit accéder aux thetas des autres events.
            //  => on effectue donc la mise à jour directement ici, sans passer par une fonction
            //  de la classe event (qui n'a pas accès aux autres events)
            // ----------------------------------------------------------------------
            const double min = event->getThetaMin(t_min);
            const double max = event->getThetaMax(t_max);

            if (min >= max){
                throw QObject::tr("Error for event theta : %1 : min = %2 : max = %3").arg(event->mName, QString::number(min), QString::number(max));
            }
            
            // On stocke l'ancienne valeur :
            double value_current = event->mTheta.mX;
            
            // On tire une nouvelle valeur :
            const double value_new = Generator::gaussByBoxMuller(event->mTheta.mX, event->mTheta.mSigmaMH);
            
            double rapport = 0;
            
            if (value_new >= min && value_new <= max)
            {
                // On force la mise à jour de la nouvelle valeur pour calculer h_new
                event->mTheta.mX = value_new;
                
                orderEventsByTheta(); // on réordonne les thetas
                saveEventsTheta(); // On les sauvegarde
                spreadEventsTheta(); // On les espace si nécessaire
                
                double h_new = h_YWI_AY() * h_alpha() * h_theta();
                
                restoreEventsTheta(); // On supprime les décalages introduits pour les calculs de h_new
                
                // Calcul du rapport :
                rapport = (h_current == 0) ? 1 : (h_new / h_current);
                
                // On reprend l'ancienne valeur, qui sera éventuellement mise à jour dans ce qui suit (Metropolis Hastings)
                event->mTheta.mX = value_current;
                orderEventsByTheta();
                
                // Pour l'itération suivante :
                h_current = h_new;
            }
            
            event->mTheta.tryUpdate(value_new, rapport);
            
            // on réordonne les theta car celui courant a peut-être changé
            orderEventsByTheta();
            
            if(doMemo){
               event->mTheta.memo();
               event->mTheta.saveCurrentAcceptRate();
            }
        }
    }
    // Pas bayésien : on sauvegarde la valeur constante dans la trace
    else
    {
        for(Event* event : mModel->mEvents)
        {
            event->mTheta.tryUpdate(event->mTheta.mX, 1);
            if(doMemo){
                event->mTheta.memo();
                event->mTheta.saveCurrentAcceptRate();
            }
        }
    }

    // --------------------------------------------------------------
    //  Remarque : à ce stade, tous les theta events sont à jour et ordonnés.
    //  On va à présent mettre à jour tous les VG, puis Alpha lissage.
    //  Pour cela, nous devons espacer les thetas pour permettre les calculs.
    //  Nous le faisons donc ici, et restaurerons les vrais thetas à la fin.
    // --------------------------------------------------------------
    orderEventsByTheta();
    saveEventsTheta();
    spreadEventsTheta();
    
    // --------------------------------------------------------------
    //  Update VG Events
    // --------------------------------------------------------------
    if(mChronocurveSettings.mVarianceType == ChronocurveSettings::eModeBayesian)
    {
        double h_current = h_YWI_AY() * h_alpha() * h_VG();
        
        for(Event* event : mModel->mEvents)
        {
            const double min = log10(1e-10);
            const double max = log10(1e+20);
            
            if (min >= max){
                throw QObject::tr("Error for event VG : %1 : min = %2 : max = %3").arg(event->mName, QString::number(min), QString::number(max));
            }
            
            // On stocke l'ancienne valeur :
            double value_current = event->mVG.mX;
            // On tire une nouvelle valeur :
            const double value_new_log = Generator::gaussByBoxMuller(log10(event->mVG.mX), event->mVG.mSigmaMH);
            const double value_new = pow(10, value_new_log);
            
            double rapport = 0;
            
            if (value_new_log >= min && value_new_log <= max)
            {
                // On force la mise à jour de la nouvelle valeur pour calculer h_new
                // A chaque fois qu'on modifie VG, W change !
                event->mVG.mX = value_new;
                event->updateW();
                
                // Calcul du rapport :
                double h_new = h_YWI_AY() * h_alpha() * h_VG();
                rapport = (h_current == 0) ? 1 : ((h_new * value_new) / (h_current * value_current));
                
                // On remet l'ancienne valeur, qui sera éventuellement mise à jour dans ce qui suit (Metropolis Hastings)
                // A chaque fois qu'on modifie VG, W change !
                event->mVG.mX = value_current;
                event->updateW();
                
                // Pour l'itération suivante :
                h_current = h_new;
            }
            
            event->mVG.tryUpdate(value_new, rapport);
            
            // A chaque fois qu'on modifie VG, W change !
            event->updateW();
            
            if(doMemo){
               event->mVG.memo();
               event->mVG.saveCurrentAcceptRate();
            }
        }
    }
    // Pas bayésien : on sauvegarde la valeur constante dans la trace
    else
    {
        for(Event* event : mModel->mEvents)
        {
            event->mVG.tryUpdate(event->mVG.mX, 1);
            if(doMemo){
                event->mVG.memo();
                event->mVG.saveCurrentAcceptRate();
            }
        }
    }
    
    // --------------------------------------------------------------
    //  Update Alpha
    // --------------------------------------------------------------
    if(mChronocurveSettings.mCoeffLissageType == ChronocurveSettings::eModeBayesian)
    {
        const double min = log10(1e-20);
        const double max = log10(1e+5);
        
        if (min >= max){
            throw QObject::tr("Error for Alpha lissage : min = %1 : max = %2").arg(QString::number(min), QString::number(max));
        }
        
        // On stocke l'ancienne valeur :
        double value_current = mModel->mAlphaLissage.mX;
        // On tire une nouvelle valeur :
        const double value_new_log = Generator::gaussByBoxMuller(log10(mModel->mAlphaLissage.mX), mModel->mAlphaLissage.mSigmaMH);
        const double value_new = pow(10, value_new_log);
        
        double rapport = 0;
        
        if (value_new_log >= min && value_new_log <= max)
        {
            double h_current = h_YWI_AY() * h_alpha();
            
            // On force la mise à jour de la nouvelle valeur pour calculer h_new
            mModel->mAlphaLissage.mX = value_new;
            
            // Calcul du rapport :
            double h_new = h_YWI_AY() * h_alpha();
            rapport = (h_current == 0) ? 1 : ((h_new * value_new) / (h_current * value_current));
            
            // On remet l'ancienne valeur, qui sera éventuellement mise à jour dans ce qui suit (Metropolis Hastings)
            mModel->mAlphaLissage.mX = value_current;
        }
        
        mModel->mAlphaLissage.tryUpdate(value_new, rapport);
        
        if(doMemo){
           mModel->mAlphaLissage.memo();
           mModel->mAlphaLissage.saveCurrentAcceptRate();
        }
    }
    // Pas bayésien : on sauvegarde la valeur constante dans la trace
    else
    {
        mModel->mAlphaLissage.tryUpdate(mModel->mAlphaLissage.mX, 1);
        if(doMemo)
        {
            mModel->mAlphaLissage.memo();
            mModel->mAlphaLissage.saveCurrentAcceptRate();
        }
    }
    
    // --------------------------------------------------------------
    //  Restauration des valeurs des theta (non espacés pour le calcul)
    // --------------------------------------------------------------
    restoreEventsTheta();
    
    // --------------------------------------------------------------
    //  Les calculs spline sont terminés
    //  On repasse les theta event en temps année
    // --------------------------------------------------------------
    // TODO : convertir tous les theta event entre tmin et tmax
    
    // --------------------------------------------------------------
    //  Calcul de la spline g, g' et gerr à l'année sur plage d'étude
    // --------------------------------------------------------------
    
    /*Do_cravate(tab_crav,nb_noeuds);
    Calcul_spline(tab_crav,alpha,true,Vec_spline);
    Calcul_splines_Enveloppe(Tab_crav,alpha,Vec_spline,Vec_spline_inf,Vec_spline_sup);*/
}

bool MCMCLoopChronocurve::adapt()
{
    ChainSpecs& chain = mChains[mChainIndex];

    const double taux_min = 41.;           // taux_min minimal rate of acceptation=42
    const double taux_max = 47.;           // taux_max maximal rate of acceptation=46

    bool allOK = true;

    //--------------------- Adapt -----------------------------------------

    double delta = (chain.mBatchIndex < 10000) ? 0.01 : (1. / sqrt(chain.mBatchIndex));

    for (auto && event : mModel->mEvents ) {

       for (auto && date : event->mDates) {

            //--------------------- Adapt Sigma MH de Theta i -----------------------------------------

            if (date.mMethod == Date::eMHSymGaussAdapt) {
                const double taux = 100. * date.mTheta.getCurrentAcceptRate();
                if (taux <= taux_min || taux >= taux_max) {
                    allOK = false;
                    const double sign = (taux <= taux_min) ? -1. : 1.;
                    date.mTheta.mSigmaMH *= pow(10., sign * delta);
                }
            }

            //--------------------- Adapt Sigma MH de Sigma i -----------------------------------------

            const double taux = 100. * date.mSigma.getCurrentAcceptRate();
            if (taux <= taux_min || taux >= taux_max) {
                allOK = false;
                const double sign = (taux <= taux_min) ? -1. : 1.;
                date.mSigma.mSigmaMH *= pow(10., sign * delta);
            }
        }

        //--------------------- Adapt Sigma MH de Theta f -----------------------------------------

        double taux = 100. * event->mTheta.getCurrentAcceptRate();
        if (taux <= taux_min || taux >= taux_max) {
            allOK = false;
            const double sign = (taux <= taux_min) ? -1. : 1.;
            event->mTheta.mSigmaMH *= pow(10., sign * delta);
        }
        
        //--------------------- Adapt Sigma MH de VG -----------------------------------------

        taux = 100. * event->mVG.getCurrentAcceptRate();
        if (taux <= taux_min || taux >= taux_max) {
            allOK = false;
            const double sign = (taux <= taux_min) ? -1. : 1.;
            event->mVG.mSigmaMH *= pow(10., sign * delta);
        }
    }
    
    //--------------------- Adapt Sigma MH de Alpha Lissage -----------------------------------------

    const double taux = 100. * mModel->mAlphaLissage.getCurrentAcceptRate();
    if (taux <= taux_min || taux >= taux_max) {
        allOK = false;
        const double sign = (taux <= taux_min) ? -1. : 1.;
        mModel->mAlphaLissage.mSigmaMH *= pow(10., sign * delta);
    }
    
    return allOK;
}

void MCMCLoopChronocurve::finalize()
{
    // Calculer la moyenne des g(t) à partir de toutes les chaines
}

#pragma mark Related to : calibrate

void MCMCLoopChronocurve::prepareEventsY()
{
    for(Event* event : mModel->mEvents){
        prepareEventY(event);
    }
}

void MCMCLoopChronocurve::prepareEventY(Event* event)
{
    double y1 = event->mYInc;
    double y2 = event->mYDec;
    double y3 = event->mYInt;
    double s1 = event->mSInc;
    double s3 = event->mSInt;
    
    if(mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeUnivarie)
    {
        if(mChronocurveSettings.mVariableType == ChronocurveSettings::eVariableTypeInclinaison)
        {
            event->mYx = y1;
            event->mSy = s1;
        }
        else if(mChronocurveSettings.mVariableType == ChronocurveSettings::eVariableTypeDeclinaison)
        {
            event->mYx = y2;
            event->mSy = s1 / cos(y1 * M_PI / 180.);
        }
        else
        {
            event->mYx = y3;
            event->mSy = s3;
        }
    }
    else if(mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique)
    {
        event->mYx = y1;
        event->mYy = y2;
        event->mSy = s1;
    }
    else if(mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel)
    {
        event->mYx = y3 * cos(y1 * M_PI / 180.) * cos(y2 * M_PI / 180.);
        event->mYy = y3 * cos(y1 * M_PI / 180.) * sin(y2 * M_PI / 180.);
        event->mYz = y3 * sin(y1 * M_PI / 180.);
        event->mSy = pow((1/3) * (s3 * s3 + 2 * y3 * y3 / (s1 * s1)), 0.5);
    }
}

#pragma mark Related to : update : calcul de h_new

/**
 * Calcul de h_YWI_AY pour toutes les composantes de Y event (suivant la configuration univarié, spérique ou vectoriel)
 */
double MCMCLoopChronocurve::h_YWI_AY()
{
    // calcul matRQ, etc...
    
    for(Event* event : mModel->mEvents){
        event->mY = event->mYx;
    }
    
    double h = h_YWI_AY_composante();
    
    if(mChronocurveSettings.mProcessType != ChronocurveSettings::eProcessTypeUnivarie)
    {
        for(Event* event : mModel->mEvents){
            event->mY = event->mYy;
        }
        h *= h_YWI_AY_composante();
    }
    if(mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel)
    {
        for(Event* event : mModel->mEvents){
            event->mY = event->mYz;
        }
        h *= h_YWI_AY_composante();
    }
    return h;
}

/**
 * Calcul de h_YWI_AY pour la composante courante des Y des events (Y peut valoir Yx, Yy ou Yz)
 * Ceci est nécessaire dans les cas sphérique et vectoriel.
 */
double MCMCLoopChronocurve::h_YWI_AY_composante()
{
    if(mModel->mAlphaLissage.mX == 0){
        return 1;
    }
    
    calcul_spline(true);
    
    return 1;
    
#if 0
    
    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y}
    double YWY = 0;
    double YWAY = 0;
    
    int nb_noeuds = mModel->mEvents.size();
    
    for(int i=0; i<nb_noeuds; ++i)
    {
        Event* e = mModel->mEvents[i];
        
        // TODO
        Wi = Tab_crav[i].Wi;
        YWY += Wi * sqr(Tab_crav[i].Yi);
        YWAY += tab_crav[i].Yi * Wi * Vec_spline.g[i];
    }
    
    double h_exp = -0.5 * (YWY-YWAY);
    
    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------

    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ
    
    Decomposition_Cholesky(Mat_QtQ,Nb_noeuds,5,1,Mat_Lq,Mat_Dq);

    double det_1_2 = 1;
    for(int i=2; i<=nb_noeuds-1; ++i){
        det_1_2 *= Mat_Dq[i,i] / Mat_D[i,i];
    }
    
    // calcul à un facteur (2*PI) puissance -(n-2) près
    return exp(0.5 * (Nb_noeuds-2) * ln(mModel->mAlphaLissage.mX) + h_exp) * sqrt(det_1_2);
#endif
}

double MCMCLoopChronocurve::h_alpha()
{
    return 1;
    
#if 0
    // La trace de la matrice produit W_1.K est égal à la somme des valeurs propores si W_1.K est symétrique,
    // ce qui implique que W_1 doit être une constante
    // d'où on remplace W_1 par la matrice W_1m moyenne des (W_1)ii
    
    int nb_noeuds = mModel->mEvents.size();
    init_vecteur(Diag_W_1m, nb_noeuds);
    
    double W_1m = 0;
    for(int i=0; i<nb_noeuds; ++i){
        W_1m += Diag_W_1[i];
    }
    W_1m /= nb_noeuds;
    for(int i=0; i<nb_noeuds; ++i){
        Diag_W_1m[i] = W_1m;
    }

    // calcul des termes diagonaux de W_1.K
    Decomposition_Cholesky(Mat_R,nb_noeuds,3,1,Mat_Lc,Mat_Dc);
    inverse_Mat_sym(Mat_Lc,Mat_Dc,nb_noeuds,1,Mat_R_1,5);
    Multi_mat_par_mat(Mat_Q,Mat_R_1,Nb_noeuds,3,3,tmp);
    // si alpha local, Mat_QT correspond à Mat_ST
    Multi_mat_par_mat(tmp,Mat_QT,Nb_noeuds,3,3,Mat_K);
    Multi_diag_par_Mat(Diag_W_1m,Mat_K,Nb_noeuds,1,Mat_W_1K);

    double vm = 0;
    for(int i=0; i<nb_noeuds; ++i){
        vm += Mat_W_1K[i,i];
    }
    
    double c = (nb_noeuds-2) / vm;

    // initialisation de l'exposant mu du prior "shrinkage" sur alpha : fixe
    // en posant mu=2, on la moyenne a priori sur alpha finie = (nb_noeuds-2)/somme(Mat_W_1K[i,i]) ;
    // et la variance a priori sur alpha est infinie
    // si on veut un shrinkage avec espérance et variance finies, alors mu > 2
    double mu = 2.;
    
    // prior "shrinkage"
    return (mu/c) * power(c/(c + mModel->mAlphaLissage.mX),(mu+1));
#endif
}

double MCMCLoopChronocurve::h_VG()
{
    // Densité a priori sur variance de type "shrinkage" avec paramètre S02
    // bool_shrinkage_uniforme:=true;

    double shrink_VG;
    
    if(mChronocurveSettings.mVarianceType == ChronocurveSettings::eModeFixed){
        shrink_VG = mChronocurveSettings.mVarianceFixed;
    }else{
        
        int nb_noeuds = mModel->mEvents.size();
        
        if(mChronocurveSettings.mUseVarianceIndividual){
            
            shrink_VG = 1;
            for(int i=0; i<nb_noeuds; ++i){
                Event* e = mModel->mEvents[i];
                double S02 = e->mSInc * e->mSInc;
                shrink_VG *= (S02 / pow(S02 + e->mVG.mX,2));
            }
            
        }else{
            
            // S02 : moyenne harmonique des erreurs sur Y
            double som_inv_S02 = 0;
            
            for(int i=0; i<nb_noeuds; ++i){
                Event* e = mModel->mEvents[i];
                som_inv_S02 += ( 1 / (e->mSInc * e->mSInc));
            }
            double S02 = nb_noeuds/som_inv_S02;
            // Shrinkage avec a = 1
            
            shrink_VG = S02 / pow(S02 + mModel->mEvents[0]->mVG.mX, 2);
        }
    }
    
    return shrink_VG;
}

double MCMCLoopChronocurve::h_theta()
{
    double sum = 0;
    int nb_noeuds = mModel->mEvents.size();
    
    for(int i=0; i<nb_noeuds; ++i)
    {
        Event* e = mModel->mEvents[i];
        
        double pi = 0;
        double ti_moy = 0;
        
        for(int j=0; j<e->mDates.size(); ++j)
        {
            Date& date = e->mDates[j];
            pi += 1 / pow(date.mSigma.mX, 2);
            ti_moy += (date.mTheta.mX + date.mDelta) / pow(date.mSigma.mX, 2);
        }
        ti_moy /= pi;
        
        sum += pi * pow(e->mTheta.mX - ti_moy, 2);
    }
    
    return exp(-0.5 * sum);
}

#pragma mark Related to : do_cravate

/**
 * La création de la matrice diagonale des erreurs est nécessaire à chaque mise à jour de :
 * - Theta event : qui peut engendrer un nouvel ordonnancement des events (definitionNoeuds)
 * - VG event : qui intervient directement dans le calcul de W
 */
std::vector<double> MCMCLoopChronocurve::createDiagW1()
{
    std::vector<double> diag_W_1;
    
    for(Event* event : mModel->mEvents){
        diag_W_1.push_back(event->mW1);
    }
    
    return diag_W_1;
}

/**
 * Cette fonction est l'ancien do_cravate() qui portait mal son nom car nous ne faisons pas de cravates !
 * Une cravate correspondait à des theta tellement proches qu'on "fusionnait" les events correspondants pour les calculs.
 * Or, reorderEventsByTheta a pour rôle d'introduire un espace minimal entre chaque theta pour justement ne pas avoir de cravate.
 * Cette fonction ne fait donc que retourner le résultat de definitionNoeuds dans un format pratique pour la suite des calculs
*/
void MCMCLoopChronocurve::orderEventsByTheta()
{
    // On manipule directement la liste des évènements
    QList<Event*>& result = mModel->mEvents;
    
    //std::sort(result.begin(), result.end(), sortEventsByTheta);
    std::sort(result.begin(), result.end(), [](const Event* a, const Event* b) {
        return (a->mTheta.mX < b->mTheta.mX);
    });
}

void MCMCLoopChronocurve::saveEventsTheta()
{
    mThetasMemo.clear();
    for(Event* e : mModel->mEvents){
        mThetasMemo.insert(std::pair<int, double>(e->mId, e->mTheta.mX));
    }
}

void MCMCLoopChronocurve::restoreEventsTheta()
{
    for(Event* e : mModel->mEvents){
        e->mTheta.mX = mThetasMemo.at(e->mId);
    }
}

void MCMCLoopChronocurve::spreadEventsTheta(double minStep)
{
    // On manipule directement la liste des évènements
    QList<Event*>& result = mModel->mEvents;
    
    // Espacement possible ?
    double count = result.size();
    double firstValue = result[0]->mTheta.mX;
    double lastValue = result[count - 1]->mTheta.mX;
    if((lastValue - firstValue) < (count - 1) * minStep){
        throw tr("Not enought span between events theta");
    }
    
    // Il faut au moins 3 points
    if(count < 3){
        throw tr("3 events minimum required");
    }

    // 0 veut dire qu'on n'a pas détecté d'égalité :
    unsigned long startIndex = 0;
    unsigned long endIndex = 0;
    
    for(unsigned long i=1; i<count; ++i)
    {
        double value = result[i]->mTheta.mX;
        double lastValue = result[i - 1]->mTheta.mX;
        
        // Si l'écart n'est pas suffisant entre la valeur courante et la précedente,
        // alors on mémorise l'index précédent comme le début d'une égalité
        // (à condition de ne pas être déjà dans une égalité)
        if((value - lastValue < minStep) && (startIndex == 0))
        {
            // La valeur à l'index 0 ne pourra pas être déplacée vers la gauche !
            // S'il y a égalité dès le départ, on considère qu'elle commence à l'index 1.
            startIndex = (i == 1) ? 1 : (i-1);
        }
        
        //qDebug() << "i = " << i << " | value = " << value << " | lastValue = " << lastValue << " | startIndex = " << startIndex;
        
        // Si on est à la fin du tableau et dans un cas d'égalité,
        // alors on s'assure d'avoir suffisamment d'espace disponible
        // en incluant autant de points précédents que nécessaire dans l'égalité.
        if((i == count - 1) && (startIndex != 0))
        {
            endIndex = i-1;
            for(unsigned long j=startIndex; j>=1; j--)
            {
                double delta = value - result[j-1]->mTheta.mX;
                double deltaMin = minStep * (i - j + 1);
                if(delta >= deltaMin){
                    startIndex = j;
                    qDebug() << "=> Egalité finale | startIndex = " << startIndex << " | endIndex = " << endIndex;
                    break;
                }
            }
        }
        
        // Si l'écart entre la valeur courante et la précédente est suffisant
        // ET que l'on était dans un cas d'égalité (pour les valeurs précédentes),
        // alors il se peut qu'on ait la place de les espacer.
        if((value - lastValue >= minStep) && (startIndex != 0))
        {
            double startValue = result[startIndex-1]->mTheta.mX;
            double delta = (value - startValue);
            double deltaMin = minStep * (i - startIndex + 1);
            
            //qDebug() << "=> Vérification de l'espace disponible | delta = " << delta << " | deltaMin = " << deltaMin;
            
            if(delta >= deltaMin)
            {
                endIndex = i-1;
            }
        }
        
        if(endIndex != 0)
        {
            //qDebug() << "=> On espace les valeurs entre les bornes " << result[startIndex - 1]->mTheta.mX << " et " << result[i]->mTheta.mX;
            
            // On a la place d'espacer les valeurs !
            // - La borne inférieure ne peut pas bouger (en startIndex-1)
            // - La borne supérieure ne peut pas bouger (en endIndex)
            // => On espace les valeurs intermédiaires (de startIndex à endIndex-1) du minimum nécessaire
            double startSpread = result[endIndex] - result[startIndex];
            for(unsigned long j=startIndex; j<=endIndex; j++)
            {
                if((result[j]->mTheta.mX - result[j-1]->mTheta.mX) < minStep){
                    result[j]->mTheta.mX = result[j-1]->mTheta.mX + minStep;
                }
            }
            // En espaçant les valeurs vers la droite, on a "décentré" l'égalité.
            // => On redécale tous les points de l'égalité vers la gauche pour les recentrer :
            double endSpread = result[endIndex]->mTheta.mX - result[startIndex]->mTheta.mX;
            double shiftBack = (endSpread - startSpread) / 2;
            
            // => On doit prendre garde à ne pas trop se rappocher le la borne de gauche :
            if((result[startIndex]->mTheta.mX - shiftBack) - result[startIndex-1]->mTheta.mX < minStep){
                shiftBack = result[startIndex]->mTheta.mX - (result[startIndex-1]->mTheta.mX + minStep);
            }
            
            // On doit décaler suffisamment vers la gauche pour ne pas être trop près de la borne de droite :
            if(result[endIndex + 1]->mTheta.mX - (result[endIndex]->mTheta.mX - shiftBack) < minStep){
                shiftBack = result[endIndex]->mTheta.mX - (result[endIndex + 1]->mTheta.mX - minStep);
            }
            for(unsigned long j=startIndex; j<=endIndex; j++)
            {
                result[j]->mTheta.mX -= shiftBack;
            }
            
            // On marque la fin de l'égalité
            startIndex = 0;
            endIndex = 0;
        }
    }
}

#pragma mark Related to : calcul_spline

void MCMCLoopChronocurve::calcul_spline(bool newTime)
{
    // Si nouvelle configuration de temps ti et/ou de wi
    // (Ceci n'est nécessaire que pour la première composante dans les cas sphérique et vectoriel)
    if(newTime)
    {
        //std::vector<double> vec_h = calculVecH(records);
        //Calcul_Mat_RQ(Vec_h,Diag_W_1,nb_noeuds,Mat_R,Mat_Q,Mat_QT,Mat_QTW_1Q,Mat_QTQ,false);
    }
}

std::vector<double> MCMCLoopChronocurve::calculVecH(const std::vector<Record>& records)
{
    std::vector<double> result;
    for(unsigned long i=0; i<records.size()-1; ++i)
    {
        result.push_back(records[i + 1].ti - records[i].ti);
    }
    return result;
}
