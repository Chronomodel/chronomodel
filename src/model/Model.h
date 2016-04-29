#ifndef MODEL_H
#define MODEL_H

#include "ProjectSettings.h"
#include "AppSettings.h"
#include "MCMCSettings.h"
#include "Event.h"
#include "Phase.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"

#include <QJsonObject>


class Model:public QObject
{
    Q_OBJECT
public:
    Model();
    virtual ~Model();

    QJsonObject toJson() const;
    
    void generateModelLog();
    QString getModelLog() const;
    
    void generateResultsLog();
    QString getResultsLog() const;
    
    QString getMCMCLog() const;
    
    QList<QStringList> getStats(const QLocale locale, const bool withDateFormat = false);
    QList<QStringList> getPhasesTraces(QLocale locale, const bool withDateFormat = false);
    QList<QStringList> getPhaseTrace(int phaseIdx, const QLocale locale, const bool withDateFormat = false);
    QList<QStringList> getEventsTraces(const QLocale locale, const bool withDateFormat = false);
    
    void updateFormatSettings(const AppSettings *appSet);


    void fromJson( const QJsonObject& json);
    void updateDesignFromJson( const QJsonObject& json);

    bool isValid();
    void clear();

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
    //void generateCredibilityAndHPD(const QList<ChainSpecs>& chains,const double threshold);
    void generateCredibility(const double threshold);
    void generateHPD(const double threshold);
    // Trace and Posterior density needed for this :
    void generateNumericalResults(const QList<ChainSpecs>& chains);
    
    void clearTraces();
    void clearPosteriorDensities();
    void clearCredibilityAndHPD();
    void clearThreshold();
    
public:
    ProjectSettings mSettings;
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
    int mNumberOfEventsInAllPhases;
    int mNumberOfDatesInAllPhases;

public slots:
    void setThreshold(const double threshold);
    void setBandwidth(const double bandwidth);
    void setFFTLength(const double FFTLength);

signals:
    void newCalculus();

private:
    //const QJsonObject * mJson;
     int mFFTLength;
     double mBandwidth;
     double mThreshold;
};

#endif

