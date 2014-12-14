#ifndef PROJECT_H
#define PROJECT_H

#include "AppSettings.h"
#include "ProjectSettings.h"
#include "MCMCSettings.h"
#include "MCMCLoopMain.h"
#include "Model.h"
#include "StateKeys.h"

#include <QObject>
#include <QList>
#include <QString>
#include <QJsonObject>

#define PROJECT_LOADED_REASON "project loaded"

class Date;
class Date;
class Event;
class EventKnown;
class Phase;
class EventConstraint;
class PhaseConstraint;
class PluginAbstract;


class Project: public QObject
{
    Q_OBJECT
public:
    Project();
    virtual ~Project();
    
    void initState();
    
    // This is the function to call when the project state is to be modified.
    // An undo command is created and a StateEvent is fired asynchronously.
    // The project state will be modified and the view notified (if required)
    bool pushProjectState(const QJsonObject& state, const QString& reason, bool notify, bool force = false);
    
    // Sends a StateEvent asynchronously.
    // Called by "SetProjectState" undo/redo commands.
    void sendUpdateState(const QJsonObject& state, const QString& reason, bool notify);
    
    // Event handler for events of type "StateEvent".
    // Updates the project state by calling updateState() and send a notification (if required).
    bool event(QEvent* e);
    
    // Update the project state directly.
    // This is not async! so be careful when calling this from views with notify = true
    void updateState(const QJsonObject& state, const QString& reason, bool notify);
    
    // Special events for selection... too bad!
    void sendEventsSelectionChanged();
    void sendPhasesSelectionChanged();
    
    QJsonObject emptyState() const;
    QJsonObject state() const;
    
    
    bool load(const QString& path);
    bool saveAs();
    bool askToSave();
    bool saveProjectToFile();
    
    bool setSettings(const ProjectSettings& settings);
    void setAppSettings(const AppSettings& settings);
    
    bool areStudyBoundsValid();
    
    void addEvent(QJsonObject event, const QString& reason);
    int getUnusedEventId(const QJsonArray& events);
    void updateEvent(const QJsonObject& event, const QString& reason);
    void mergeEvents(int eventFromId, int eventToId);
    
    Date createDateFromPlugin(PluginAbstract* plugin);
    Date createDateFromData(const QString& pluginName, const QStringList& dataStr);
    void addDate(int eventId, QJsonObject date);
    int getUnusedDateId(const QJsonArray& dates);
    void updateDate(int eventId, int dateId);
    void deleteDates(int eventId, const QList<int>& dateIndexes);
    void recycleDates(int eventId);
    
    void updatePhase(const QJsonObject& phaseIn);
    int getUnusedPhaseId(const QJsonArray& phases);
    void mergePhases(int phaseFromId, int phaseToId);
    void updatePhaseEvents(int phaseId, Qt::CheckState state);
    //void updatePhaseEyed(int phaseId, bool eyed);
    
    void createEventConstraint(int eventFromId, int eventToId);
    void deleteEventConstraint(int constraintId);
    bool isEventConstraintAllowed(const QJsonObject& eventFrom, const QJsonObject& eventTo);
    void updateEventConstraint(int constraintId);
    int getUnusedEventConstraintId(const QJsonArray& constraints);
    
    void createPhaseConstraint(int phaseFromId, int phaseToId);
    void deletePhaseConstraint(int constraintId);
    bool isPhaseConstraintAllowed(const QJsonObject& phaseFrom, const QJsonObject& phaseTo);
    void updatePhaseConstraint(int constraintId);
    int getUnusedPhaseConstraintId(const QJsonArray& constraints);
    
public slots:
    bool save();
    
    void mcmcSettings();
    void run();
    void exportAsText();
    
    void createEvent();
    void createEventKnown();
    void deleteSelectedEvents();
    void recycleEvents();
    
    void createPhase();
    void deleteSelectedPhases();
    
signals:
    void projectStateChanged();
    void currentEventChanged(const QJsonObject& event);
    void currentDateChanged(const QJsonObject& date);
    void currentPhaseChanged(const QJsonObject& phase);
    void selectedEventsChanged();
    void selectedPhasesChanged();
    void eyedPhasesModified(const QMap<int, bool>& eyedPhases);
    
    void mcmcStarted();
    void mcmcFinished(MCMCLoopMain& loop);
    
public:
    QJsonObject mState;
    QJsonObject mLastSavedState;
    
    QString mName;
    QString mProjectFileDir;
    QString mProjectFileName;
    
    Model* mModel;
    
    QTimer* mAutoSaveTimer;
};

#endif

