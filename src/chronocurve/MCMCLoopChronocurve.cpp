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
    MCMCLoop::mProject = project;
    if (mModel){
        setMCMCSettings(mModel->mMCMCSettings);
    }
}

MCMCLoopChronocurve::~MCMCLoopChronocurve()
{
    mModel = nullptr;
    mProject = nullptr;
}

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
        return QString();

    }
    return tr("Invalid model");
}

void MCMCLoopChronocurve::initVariablesForChain()
{
    // today we have the same acceptBufferLen for every chain
    const int acceptBufferLen =  mChains[0].mNumBatchIter;
    int initReserve (0);

    for (const ChainSpecs c: mChains)
       initReserve += ( 1 + (c.mMaxBatchs*c.mNumBatchIter) + c.mNumBurnIter + (c.mNumRunIter/c.mThinningInterval) );

    for (Event* event : mModel->mEvents) {
        event->mTheta.reset();
        event->mTheta.reserve(initReserve);

        event->mTheta.mLastAccepts.reserve(acceptBufferLen);
        event->mTheta.mLastAcceptsLength = acceptBufferLen;

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

    // TODO : prepare new event variables and final result variables
    // Alpha lissage
    // Variances globale ou individuelle de Y par rapport à g(theta)
}

QString MCMCLoopChronocurve::initMCMC()
{
    return QString();
}

void MCMCLoopChronocurve::update()
{
    // Mise à jour des variables
    // Evaluation de la spline g(t_noeud) => courbe d'étalonnage recherchée
    // => C'est un vecteur sur [t_floor, t_ceil]
}

bool MCMCLoopChronocurve::adapt()
{
    return true;
}

void MCMCLoopChronocurve::finalize()
{
    // Calculer la moyenne des g(t) à partir de toutes les chaines
}
