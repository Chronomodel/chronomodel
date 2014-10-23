#ifndef PROJECT_H
#define PROJECT_H

#include "Settings.h"
#include "ProjectSettings.h"
#include "MCMCSettings.h"
#include "Model.h"

#include <QObject>
#include <QList>
#include <QString>
#include <QJsonObject>

#define STATE_SETTINGS "settings"
#define STATE_MCMC "mcmc"
#define STATE_EVENTS "events"
#define STATE_PHASES "phases"
#define STATE_EVENTS_CONSTRAINTS "events_constraints"
#define STATE_PHASES_CONSTRAINTS "phases_constraints"
#define STATE_DATES_TRASH "dates_trash"
#define STATE_EVENTS_TRASH "events_trash"

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
    void pushProjectState(const QJsonObject& state, const QString& reason, bool notify, bool force = false);
    
    // Sends a StateEvent asynchronously.
    // Called by "SetProjectState" undo/redo commands.
    void sendUpdateState(const QJsonObject& state, const QString& reason, bool notify);
    
    // Event handler for events of type "StateEvent".
    // Updates the project state by calling updateState() and send a notification (if required).
    bool event(QEvent* e);
    
    // Update the project state directly.
    // This is not async! so be careful when calling this from views with notify = true
    void updateState(const QJsonObject& state, const QString& reason, bool notify);
    
    QJsonObject emptyState() const;
    QJsonObject state() const;
    
    
    bool load(const QString& path);
    bool saveAs();
    bool askToSave();
    bool saveProjectToFile();
    
    void setSettings(const ProjectSettings& settings);
    void setAppSettings(const Settings& settings);
    
    void addEvent(QJsonObject event, const QString& reason);
    int getUnusedEventId(const QJsonArray& events);
    void updateEvent(const QJsonObject& event, const QString& reason);
    
    Date createDateFromPlugin(PluginAbstract* plugin);
    Date createDateFromData(const QString& pluginName, const QStringList& dataStr);
    void addDate(int eventId, QJsonObject date);
    int getUnusedDateId(const QJsonArray& dates);
    void updateDate(int eventId, int dateId);
    void deleteDates(int eventId, const QList<int>& dateIndexes);
    void recycleDates(int eventId);
    
public slots:
    bool save();
    
    void mcmcSettings();
    void run();
    void exportAsText();
    
    void createEvent();
    void createEventKnown();
    void deleteSelectedEvents();
    void recycleEvents();
    
signals:
    void projectStateChanged();
    void currentEventChanged(const QJsonObject& event);
    void currentDateChanged(const QJsonObject& date);
    void selectedPhasesChanged(const QList<int>& phasesIds);
    
    void mcmcFinished(const Model& model);
    
public:
    QJsonObject mState;
    QJsonObject mLastSavedState;
    
    QString mName;
    QString mProjectFileDir;
    QString mProjectFileName;
    
    QList<Model> mModels;
    
    QTimer* mAutoSaveTimer;
};

#endif

