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


class Model
{
public:
    Model();
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
    virtual ~Model();

    //static fromJson(const QJsonObject& json);


    void fromJson( const QJsonObject& json);
    
  /*  void setJson( const QJsonObject & json);
    const QJsonObject & getJson();
  */
        
    
    bool isValid();
    void clear();

    void saveToFile(const QString& fileName);
    void restoreFromFile(const QString& fileName);
    
    // Only trace needed for this :
    void generateCorrelations(const QList<ChainSpecs>& chains);
    // Computed from trace using FFT :
    void generatePosteriorDensities(const QList<ChainSpecs>& chains, int fftLen, double hFactor);
    // Trace and Posterior density needed for this :
    void generateCredibilityAndHPD(const QList<ChainSpecs>& chains,const double threshold);
    // Trace and Posterior density needed for this :
    void generateNumericalResults(const QList<ChainSpecs>& chains);
    
    void clearPosteriorDensities();
    void clearCredibilityAndHPD();
    
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
private:
    //const QJsonObject * mJson;
};

#endif

