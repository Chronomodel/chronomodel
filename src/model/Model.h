#ifndef MODEL_H
#define MODEL_H

#include "ProjectSettings.h"
#include "AppSettings.h"
#include "MCMCSettings.h"
//#include "Project.h"

#include "MHVariable.h"
#include "Event.h"
#include "Phase.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"


#include <QJsonObject>

class Project;


class Model:public QObject
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
    
    void updateFormatSettings(const AppSettings *appSet);

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

    void initThreshold(const double threshold);
    double getThreshold() const ;
    void initDensities(const int fftLength, const double bandwidth, const double threshold);
    void updateDensities(const int fftLength, const double bandwidth, const double threshold);

    // Computed from trace using FFT :
    void generatePosteriorDensities(const QList<ChainSpecs>& chains, int fftLen, double bandwidth);
    // Trace and Posterior density needed for this :

    void generateCredibility(const double threshold);
    void generateHPD(const double threshold);
    // Trace and Posterior density needed for this :
    void generateNumericalResults(const QList<ChainSpecs>& chains);
    
    void generateTempo();
   // void generateIntensity();

    void clearTraces();
    void clearPosteriorDensities();
    void clearCredibilityAndHPD();
    void clearThreshold();
    
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

