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

#ifndef MODEL_H
#define MODEL_H

#include "ProjectSettings.h"
#include "AppSettings.h"
#include "MCMCSettings.h"
#include "MHVariable.h"
#include "Event.h"
#include "Phase.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"

#include <QJsonObject>

class Project;


class Model: public QObject
{
    Q_OBJECT
public:
    Model();
    virtual ~Model();

    void generateModelLog();
    QString getModelLog() const;

    void generateResultsLog();
    QString getResultsLog() const;

    QString getMCMCLog() const;

    QList<QStringList> getStats(const QLocale locale, const int precision, const bool withDateFormat = false);
    QList<QStringList> getPhasesTraces(QLocale locale, const bool withDateFormat = false);
    QList<QStringList> getPhaseTrace(int phaseIdx, const QLocale locale, const bool withDateFormat = false);
    QList<QStringList> getEventsTraces(const QLocale locale, const bool withDateFormat = false);

    void updateFormatSettings();

    QJsonObject toJson() const;
    void fromJson( const QJsonObject& json);
    void setProject(Project *project);

    void updateDesignFromJson( const QJsonObject& json);

    bool isValid();
    void clear();

    void initNodeEvents(); // use in MCMCLoopMain::initMCMC()

    void saveToFile(const QString& fileName);
    void restoreFromFile(const QString& fileName);

    // Only trace needed for this :
    void generateCorrelations(const QList<ChainSpecs>& chains);

    double getThreshold() const;
    double getBandwidth() const;
    int getFFTLength() const;
    
    void setThresholdToAllModel();
    void initDensities();
    void updateDensities();

    // Computed from trace using FFT :
    void generatePosteriorDensities(const QList<ChainSpecs>& chains, int fftLen, double bandwidth);
    // Trace and Posterior density needed for this :

    void generateCredibility(const double threshold);
    void generateHPD(const double threshold);
    // Trace and Posterior density needed for this :
    void generateNumericalResults(const QList<ChainSpecs>& chains);

    void generateTempo();

    void clearTraces();
    void clearPosteriorDensities();
    void clearCredibilityAndHPD();
    void clearThreshold();
    
    bool hasSelectedEvents();
    bool hasSelectedPhases();

public:
    ProjectSettings mSettings;
    Project *mProject;
    MCMCSettings mMCMCSettings;

    QList<Event*> mEvents;
    QList<Phase*> mPhases;
    QList<EventConstraint*> mEventConstraints;
    QList<PhaseConstraint*> mPhaseConstraints;

    QList<ChainSpecs> mChains;

    QString mLogModel;
    QString mLogMCMC;
    QString mLogResults;

    int mNumberOfPhases;
    int mNumberOfEvents;
    int mNumberOfDates;

    // Members used in the next-previous sheet system
    // they count all the Events and the Dates availables to display
    // We could have the same Event and Date in several phases,
    // so mNumberOfEventsInAllPhases is not egual to mNumberOfEvents
    //int mNumberOfEventsInAllPhases;
    //int mNumberOfDatesInAllPhases;

public slots:
    void setThreshold(const double threshold);
    void setBandwidth(const double bandwidth);
    void setFFTLength(const int FFTLength);

signals:
    void newCalculus();

private:
     int mFFTLength;
     double mBandwidth;
     double mThreshold;
};

#endif
