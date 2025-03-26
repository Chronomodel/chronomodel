/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#ifndef PROJECT_H
#define PROJECT_H

#include "MCMCLoop.h"
#include "ModelCurve.h"
#include "StudyPeriodSettings.h"
#include "CalibrationCurve.h"

#include "QtCore/qtmetamacros.h"

#include <QObject>
#include <QList>
#include <QString>
#include <QJsonObject>


#define PROJECT_LOADED_REASON "Project Loaded"
#define NEW_PROJECT_REASON "New Project"
#define INSERT_PROJECT_REASON "Insert Project"
#define CLOSE_PROJECT_REASON "Close Project"
#define PROJECT_SETTINGS_UPDATED_REASON "Settings updated"
#define MCMC_SETTINGS_UPDATED_REASON "MCMC Settings updated"
#define MCMC_SETTINGS_RESTORE_DEFAULT_REASON "MCMC Settings restore default"
#define MCMC_METHODE_RESET_REASON "MCMC methods reset"
#define CURVE_SETTINGS_UPDATED_REASON "Curve Settings updated"
#define DATE_MOVE_TO_EVENT_REASON "Date moved to event"
#define NEW_EVEN_BY_CSV_DRAG_REASON "New Event by CSV drag"

#define PROJECT_UNDO_REDO_REASON "Undo-Redo action"

class Date;
class Event;
class Phase;
class EventConstraint;
class PhaseConstraint;
class PluginAbstract;


//QString res_file_version;

class Project: public QObject, std::enable_shared_from_this<Project>
{
    Q_OBJECT
public:
    Project();
    virtual ~Project();

    enum ActionOnModel {
        InsertEventsToPhase,
        ExtractEventsFromPhase,
    };

    void initState(const QString& reason);

    // This is the function to call when the project state is to be modified.
    // An undo command is created and a StateEvent is fired asynchronously.
    // The project state will be modified and the view notified (if required)
    bool pushProjectState(const QJsonObject& state, const QString& reason, bool notify);
    void checkStateModification(const QJsonObject& stateNew,const QJsonObject& stateOld);
    bool structureIsChanged();
    bool designIsChanged();
    // Sends a StateEvent asynchronously.
    // Called by "SetProjectState" undo/redo commands.
    void sendUpdateState(const QJsonObject& state, const QString& reason, bool notify);

    // Event handler for events of type "StateEvent".
    // Updates the project state by calling updateState() and send a notification (if required).
    bool event(QEvent* e);

    // Update the project state directly.
    // This is not async! so be careful when calling this from views with notify = true
    void updateState(const QJsonObject& state, const QString& reason, bool notify);


    static QJsonObject emptyState();

    inline const QJsonObject &state() const { return mState;}

    QJsonObject* state_ptr() { return &mState;}

    bool load(const QString &path, bool force = false);
    bool saveAs(const QString &dialogTitle);
    bool askToSave(const QString &saveDialogTitle);
    bool saveProjectToFile();

    bool recenterProject();
    bool insert(const QString &path, QJsonObject &return_state);

    /**
     * @brief setNoResults : set to disable the saving the file *.res
     * @param noResults
     */
    void setNoResults(const bool noResults) { mNoResults = noResults;}
    bool withResults() const {return (!mNoResults && mModel != nullptr);}

    bool setSettings(const StudyPeriodSettings& settings);

    bool studyPeriodIsValid();
    void showStudyPeriodWarning();

    QJsonArray getInvalidDates();

    // ---------------------------
    void restoreMCMCSettings();

    void addEvent(QJsonObject event, const QString &reason);
    int getUnusedEventId(const QJsonArray &events);
    void updateEvent(const QJsonObject &event, const QString &reason);
    void mergeEvents(int eventFromId, int eventToId);
    void deleteSelectedTrashedEvents(const QList<int> &ids);

    Date createDateFromPlugin(PluginAbstract* plugin);
    Date createDateFromData(const QString &pluginName, const QStringList &dataStr);
    void addDate(int eventId, QJsonObject date);
    int getUnusedDateId(const QJsonArray &dates) const;
    void updateDate(int eventId, int dateId);
    void deleteDates(int eventId, const QList<int> &dateIndexes);
    void recycleDates(int eventId);
    void deleteSelectedTrashedDates(const QList<int> &ids);
    QJsonObject checkDatesCompatibility(QJsonObject state, bool &isCorrected);
    QJsonObject checkValidDates(const QJsonObject &state);

    void unselectedAllInState(QJsonObject &state);
    void updateSelectedEventsColor(const QColor &color);
    void updateSelectedEventsMethod(MHVariable::SamplerProposal sp);
    void updateSelectedEventsDataMethod(MHVariable::SamplerProposal sp, const QString &pluginId);
    void updateAllDataInSelectedEvents(const QHash<QString, QVariant> &groupedAction);

    void selectAllEvents();
    bool selectEventsFromSelectedPhases();
    bool selectedEventsWithString(const QString str);

    void updatePhase(const QJsonObject &phaseIn);
    int getUnusedPhaseId(const QJsonArray &phases);
    void mergePhases(int phaseFromId, int phaseToId);
    void updatePhaseEvents(const int phaseId, ActionOnModel action);
    QJsonObject getPhasesWithId(const int id);

    void createEventConstraint(const int idFrom, const int idTo);
    void deleteEventConstraint(int constraintId);
    bool isEventConstraintAllowed(const QJsonObject &eventFrom, const QJsonObject &eventTo);
    void updateEventConstraint(int constraintId);
    int getUnusedEventConstraintId(const QJsonArray &constraints);

    void createPhaseConstraint(int phaseFromId, int phaseToId);
    void deletePhaseConstraint(int constraintId);
    bool isPhaseConstraintAllowed(const QJsonObject &phaseFrom, const QJsonObject &phaseTo);
    void updatePhaseConstraint(const int constraintId);
    int getUnusedPhaseConstraintId(const QJsonArray &constraints);

    void clear_and_shrink_model();
    void clear_model();

    void createEvent(qreal x, qreal y);
    void createEventKnown(qreal x, qreal y);
    void createPhase(qreal x, qreal y, QWidget *parent);

    void extracted();
    void clear_calibCurves();

    bool isCurve() const;

public slots:
    bool save();

    void mcmcSettings();
    void resetMCMC();
    
    void run();
    void runChronomodel();
    void runCurve();

    void setAppSettingsAutoSave();

    void deleteSelectedEvents();
    void recycleEvents();

    void deleteSelectedPhases();

    void combineDates(const int eventId, const QList<int> &dateIds);
    void splitDate(const int eventId, const int dateId);

signals:
    void noResult();
    void projectStateChanged();
    void currentEventChanged(QJsonObject *event);

    void eyedPhasesModified(const QMap<int, bool> &eyedPhases);

    void mcmcStarted();
    void mcmcFinished();

    void projectItemsIsMoved(bool itemsIsMoved);

public:

    QString mName;

    std::shared_ptr<ModelCurve> mModel;
    MCMCLoop* mLoop; //public QThread

    std::map<std::string, CalibrationCurve> mCalibCurves;
    QTimer* mAutoSaveTimer;
    QJsonObject mState;

private :
    // used to define scene modification
    bool mDesignIsChanged;
    bool mStructureIsChanged;
    bool mItemsIsMoved;
    QSet<QString> mReasonChangeStructure;
    QSet<QString> mReasonChangeDesign;
    QSet<QString> mReasonChangePosition;


    bool mNoResults;
};

#endif
