#ifndef MODEL_H
#define MODEL_H

#include "ProjectSettings.h"
#include "MCMCSettings.h"
#include "Event.h"
#include "Phase.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"

#include <QObject>
#include <QJsonObject>


class Model: public QObject
{
    Q_OBJECT
public:
    Model();
    virtual ~Model();

    static Model* fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    bool isValid();
    
    void saveToFile(const QString& path);
    void restoreFromFile(const QString& path);
    
    // Only trace needed for this :
    void generateCorrelations(const QList<Chain>& chains);
    // Computed from trace using FFT :
    void generatePosteriorDensities(const QList<Chain>& chains, int fftLen, double hFactor);
    // Trace and Posterior density needed for this :
    void generateCredibilityAndHPD(const QList<Chain>& chains, int threshold);
    // Trace and Posterior density needed for this :
    void generateNumericalResults(const QList<Chain>& chains);
    
    
    QString modelLog() const;
    
public:
    ProjectSettings mSettings;
    MCMCSettings mMCMCSettings;
    
    QList<Event*> mEvents;
    QList<Phase*> mPhases;
    QList<EventConstraint*> mEventConstraints;
    QList<PhaseConstraint*> mPhaseConstraints;
    
    QList<Chain> mChains;
    
    QString mMCMCLog;
};

#endif

