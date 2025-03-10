/* ---------------------------------------------------------------------
Copyright or © or Copr. CNRS	2014 - 2025

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

#include "Project.h"

#include "CalibrationCurve.h"
#include "MainWindow.h"
#include "ModelCurve.h"
#include "PluginManager.h"
#include "MCMCSettingsDialog.h"

#include "Event.h"
#include "Bound.h"
#include "EventDialog.h"
#include "EventConstraint.h"
#include "ConstraintDialog.h"

#include "PhaseConstraint.h"
#include "PhaseDialog.h"

#include "DateDialog.h"
#include "Date.h"
#include "PluginAbstract.h"
#include "PluginFormAbstract.h"
#include "TrashDialog.h"

#include "QtUtilities.h"
#include "Generator.h"

#include "MCMCLoopChrono.h"
#include "MCMCLoopCurve.h"
#include "MCMCProgressDialog.h"

#include "SetProjectState.h"
#include "StateEvent.h"
#include "AppSettings.h"

#include <QtWidgets>
#include <QJsonObject>
#include <QFile>
#include <QTimer>

QString res_file_version; // used when loading

Project::Project():
    mName ("Empty Project"),
    mModel(new ModelCurve()),
    mLoop (nullptr),
    mCalibCurves(std::map<std::string, CalibrationCurve>()),
    mState(emptyState()),
    mDesignIsChanged (false),
    mStructureIsChanged (false),
    mItemsIsMoved (false),
    mNoResults (true)
{
    // Create an auto-save timer
    mAutoSaveTimer = new QTimer(this);
    connect(mAutoSaveTimer, &QTimer::timeout, this, &Project::save);

    // Reasons for structure changes
    // Project changes
    mReasonChangeStructure << PROJECT_LOADED_REASON << PROJECT_SETTINGS_UPDATED_REASON << INSERT_PROJECT_REASON;
    mReasonChangeStructure << NEW_EVEN_BY_CSV_DRAG_REASON;

    // dates changes
    mReasonChangeStructure << "Date created" << DATE_MOVE_TO_EVENT_REASON << "Date updated";
    mReasonChangeStructure << "Dates splitted" << "Dates combined" << "Update selected data method";

    // Event changes
    mReasonChangeStructure << "Event constraint deleted" << "Event constraint created" << "Event(s) deleted";
    mReasonChangeStructure << "Event created" << "Bound created" << "Event method updated" ;
    mReasonChangeStructure << "Event(s) restored";
    mReasonChangeStructure << "Event Node updated";
    mReasonChangeStructure << "Update selected event method";

    // Phase changes
    mReasonChangeStructure << "Phase created" << "Phase(s) deleted";
    mReasonChangeStructure << "Phase updated" << "Phase constraint created" << "Phase constraint updated" << "Phase's events updated";
    mReasonChangeStructure << "Phase selected";

    // Curve and MCMC Settings
    mReasonChangeStructure << "Curve Settings updated" << "MCMC Settings updated";

    // Curve paramters
    mReasonChangeStructure << "Event X-Inc updated" << "Event S X-Inc updated";
    mReasonChangeStructure << "Event Y-Dec updated" << "Event S Y updated";
    mReasonChangeStructure << "Event Z-F updated"  << "Event S Z-F updated";
    mReasonChangeStructure << "Event Node updated";
    mReasonChangeStructure.squeeze();

    // Reasons for design changes
    mReasonChangeDesign << "Date name updates" << "Date color updated";
    mReasonChangeDesign << "Event color updated" << "Event name updated";
    mReasonChangeDesign << "Phase color updated" << "Phase name updated";
    mReasonChangeDesign.squeeze();

    // Reasons for position changes
    mReasonChangePosition << "item moved";
    mReasonChangePosition.squeeze();


}

Project::~Project()
{
    mAutoSaveTimer->stop();
    disconnect(mAutoSaveTimer, &QTimer::timeout, this, &Project::save);
    MainWindow::getInstance()->getUndoStack()->clear();

    mState = QJsonObject();
    clear_calibCurves();
    mCalibCurves.clear();

    mModel.reset();
    mLoop = nullptr;

}

void Project::initState(const QString& reason)
{
    (void) reason;

    // Do no call pushProjectState here because we don't want to store this state in the UndoStack
    // This is called when closing a project or openning a new one,
    // so the undoStack has just been cleared and we want to keep it empty at project start!
    mState = emptyState();
}

QJsonObject Project::emptyState()
{
    QJsonObject state;

    state[STATE_APP_VERSION] = qApp->applicationVersion();

    StudyPeriodSettings StudyPeriodSettings;
    QJsonObject settings = StudyPeriodSettings.toJson();
    state[STATE_SETTINGS] = settings;

    MCMCSettings mcmcSettings;
    mcmcSettings.restoreDefault();
    QJsonObject mcmc = mcmcSettings.toJson();

    state[STATE_MCMC] = mcmc;

    state[STATE_EVENTS] = QJsonArray();
    state[STATE_PHASES] = QJsonArray();
    state[STATE_EVENTS_CONSTRAINTS] = QJsonArray();
    state[STATE_PHASES_CONSTRAINTS] = QJsonArray();
    state[STATE_DATES_TRASH] = QJsonArray();
    state[STATE_EVENTS_TRASH] = QJsonArray();

    return state;
}


/**
 * @brief Project::pushProjectState used to store action in Undo-Redo Command
 * @param state
 * @param reason
 * @param notify
 * @param force
 * @return
 */
bool Project::pushProjectState(const QJsonObject &state, const QString &reason, bool notify)
{
    mStructureIsChanged = false;
    mDesignIsChanged = false;
    mItemsIsMoved = false;

    if (reason != NEW_PROJECT_REASON && reason != PROJECT_LOADED_REASON ) {
        qDebug()<<"[Project::pushProjectState] "<< reason << notify;

        if (mReasonChangeStructure.contains(reason))
            mStructureIsChanged = true;

        else if (mReasonChangeDesign.contains(reason))
            mDesignIsChanged = true;

        else if (mReasonChangePosition.contains(reason))
            mItemsIsMoved = true;

        else
            checkStateModification(state, mState);

        SetProjectState* command = new SetProjectState(this, mState, state, reason, false);// notify);
        MainWindow::getInstance()->getUndoStack()->push(command); // connected to SetProjectState::redo] if notify emit mProject->projectStateChanged()
        command = nullptr;
    }

    // Pushes cmd on the stack or merges it with the most recently executed command.
    //In either case, executes cmd by calling its redo() function

    if (mStructureIsChanged) {
        AppSettings::mIsSaved = false;
        emit noResult(); // connected to MainWindows::noResults

    } else if (mDesignIsChanged || mItemsIsMoved ) {
        AppSettings::mIsSaved = false;
    }

    updateState(state, reason, notify); // if notify emit Project::projectStateChanged()
    /*
     * Project::projectStateChanged() is connected to MainWindows::updateProject()
     */

    return true;


}


void Project::sendUpdateState(const QJsonObject &state, const QString &reason, bool notify)
{
    qDebug()<<"[Project::sendUpdateState QGuiApplication::postEvent] "<< reason << notify;

    /* The event must be allocated on the heap since the post event queue will take ownership of the event
     * and delete it once it has been posted.
     * It is not safe to access the event after it has been posted.*/

    QGuiApplication::postEvent(this, new StateEvent(state, reason, notify), Qt::HighEventPriority);//Qt::NormalEventPriority);

}

void Project::checkStateModification(const QJsonObject& stateNew, const QJsonObject& stateOld)
{
    mDesignIsChanged = false;
    mStructureIsChanged = false;
    mItemsIsMoved = false;

    if (stateOld.isEmpty() && !stateNew.isEmpty())  {
        mDesignIsChanged = true;
        mStructureIsChanged = true;
        return;

    } else {
        // Check Study Period modification
        const double tminPeriodNew = stateNew.value(STATE_SETTINGS).toObject().value(STATE_SETTINGS_TMIN).toDouble();
        const double tmaxPeriodNew = stateNew.value(STATE_SETTINGS).toObject().value(STATE_SETTINGS_TMAX).toDouble();
        const double stepPeriodNew = stateNew.value(STATE_SETTINGS).toObject().value(STATE_SETTINGS_STEP).toDouble();

        const double tminPeriodOld = stateOld.value(STATE_SETTINGS).toObject().value(STATE_SETTINGS_TMIN).toDouble();
        const double tmaxPeriodOld = stateOld.value(STATE_SETTINGS).toObject().value(STATE_SETTINGS_TMAX).toDouble();
        const double stepPeriodOld = stateOld.value(STATE_SETTINGS).toObject().value(STATE_SETTINGS_STEP).toDouble();
        if ((tminPeriodNew != tminPeriodOld) || (tmaxPeriodNew != tmaxPeriodOld) || (stepPeriodNew != stepPeriodOld)) {
            mDesignIsChanged = true;
            mStructureIsChanged = true;
            return;
        }
        // Check phase modification
        const QJsonArray phaseNew = stateNew.value(STATE_PHASES).toArray();
        const QJsonArray phaseOld = stateOld.value(STATE_PHASES).toArray();

        if ( phaseNew.size() != phaseOld.size()) {
            mDesignIsChanged = true;
            mStructureIsChanged = true;
            return;
        } else {
            for (int i = 0; i<phaseNew.size(); ++i) {
               // Check DESIGN
               // Check name of Phases
               if (phaseNew.at(i).toObject().value(STATE_NAME) != phaseOld.at(i).toObject().value(STATE_NAME))
                   mDesignIsChanged = true;

               if ( phaseNew.at(i).toObject().value(STATE_ITEM_X) != phaseOld.at(i).toObject().value(STATE_ITEM_X) ||
                    phaseNew.at(i).toObject().value(STATE_ITEM_Y) != phaseOld.at(i).toObject().value(STATE_ITEM_Y) ) {
                   mItemsIsMoved = true;
               }
               // Check color of Phases
               const QColor newPhaseColor = QColor(phaseNew.at(i).toObject().value(STATE_COLOR_RED).toInt(),phaseNew.at(i).toObject().value(STATE_COLOR_GREEN).toInt(), phaseNew.at(i).toObject().value(STATE_COLOR_BLUE ).toInt());
               const QColor oldPhaseColor = QColor(phaseOld.at(i).toObject().value(STATE_COLOR_RED).toInt(),phaseOld.at(i).toObject().value(STATE_COLOR_GREEN).toInt(), phaseOld.at(i).toObject().value(STATE_COLOR_BLUE ).toInt());
               if (newPhaseColor != oldPhaseColor)
                   mDesignIsChanged = true;

               // Check STRUCTURE
               // Check tau parameter
               if ( phaseNew.at(i).toObject().value(STATE_PHASE_TAU_TYPE) != phaseOld.at(i).toObject().value(STATE_PHASE_TAU_TYPE) ||
                   phaseNew.at(i).toObject().value(STATE_PHASE_TAU_FIXED) != phaseOld.at(i).toObject().value(STATE_PHASE_TAU_FIXED) ||
                   phaseNew.at(i).toObject().value(STATE_PHASE_TAU_MIN) != phaseOld.at(i).toObject().value(STATE_PHASE_TAU_MIN) ||
                   phaseNew.at(i).toObject().value(STATE_PHASE_TAU_MAX) != phaseOld.at(i).toObject().value(STATE_PHASE_TAU_MAX) ) {
                   mStructureIsChanged = true;
               }
            }
        }

        // Check phases constraintes modification
        const QJsonArray &phasesConstNew = stateNew.value(STATE_PHASES_CONSTRAINTS).toArray();
        const QJsonArray &phasesConstOld = stateOld.value(STATE_PHASES_CONSTRAINTS).toArray();

        if ( phasesConstNew.size() != phasesConstOld.size()) {
            mStructureIsChanged = true;
            return;

        } else if (phasesConstNew != phasesConstOld) {
            mStructureIsChanged = true;
            return;
        }


         // Check Event and date modification
        const QJsonArray eventsNew = stateNew.value(STATE_EVENTS).toArray();
        const QJsonArray& eventsOld = stateOld.value(STATE_EVENTS).toArray();

        if ( eventsNew.size() != eventsOld.size()) {
            mDesignIsChanged = true;
            mStructureIsChanged = true;
            return;

        } else {
            auto ev_old = eventsOld.begin();
            for (const auto ev_new :eventsNew) {
               const auto& ev_new_obj = ev_new.toObject();
               const auto& ev_old_obj = ev_old->toObject();
                // Check DESIGN
               // Check name of Event
               if (ev_new_obj.value(STATE_NAME) != ev_old_obj.value(STATE_NAME))
                   mDesignIsChanged = true;

               if ( ev_new_obj.value(STATE_ITEM_X) != ev_old_obj.value(STATE_ITEM_X) ||
                    ev_new_obj.value(STATE_ITEM_Y) != ev_old_obj.value(STATE_ITEM_Y) ) {
                   mItemsIsMoved = true;
               }
               // Check color of Event
               QColor newEventsColor = QColor(ev_new_obj.value(STATE_COLOR_RED).toInt(), ev_new_obj.value(STATE_COLOR_GREEN).toInt(), ev_new_obj.value(STATE_COLOR_BLUE ).toInt());
               QColor oldEventsColor = QColor(ev_old_obj.value(STATE_COLOR_RED).toInt(), ev_old_obj.value(STATE_COLOR_GREEN).toInt(), ev_old_obj.value(STATE_COLOR_BLUE ).toInt());
               if (newEventsColor != oldEventsColor)
                   mDesignIsChanged = true;

               // Check EVENTS STRUCTURE
               if ( ev_new_obj.value(STATE_EVENT_TYPE) != ev_old_obj.value(STATE_EVENT_TYPE) ||
                    ev_new_obj.value(STATE_EVENT_POINT_TYPE) != ev_old_obj.value(STATE_EVENT_POINT_TYPE) ||
                    ev_new_obj.value(STATE_EVENT_SAMPLER) != ev_old_obj.value(STATE_EVENT_SAMPLER) ||
                    ev_new_obj.value(STATE_EVENT_PHASE_IDS) != ev_old_obj.value(STATE_EVENT_PHASE_IDS) ||

                   ev_new_obj.value(STATE_EVENT_KNOWN_FIXED) != ev_old_obj.value(STATE_EVENT_KNOWN_FIXED) )

               {
                   mStructureIsChanged = true;
                   return;
               }

               // Check dates inside Event
               const QJsonArray& datesNew = ev_new_obj.value(STATE_EVENT_DATES).toArray();
               const QJsonArray& datesOld = ev_old_obj.value(STATE_EVENT_DATES).toArray();
               if ( datesNew.size() != datesOld.size()) {
                   mStructureIsChanged = true;
                   return;
               } else {
                   auto date_old = datesOld.begin();
                   for (const auto date_new : datesNew) {
                       const auto& date_new_obj = date_new.toObject();
                       const auto& date_old_obj = date_old->toObject();
                        // Check name of Date
                        if (date_new_obj.value(STATE_NAME) != date_old_obj.value(STATE_NAME))
                            mDesignIsChanged = true;

                        // No color in date JSON
                        // Check DATES STRUCTURE
                        if ( date_new_obj.value(STATE_DATE_DATA) != date_old_obj.value(STATE_DATE_DATA) ||
                            date_new_obj.value(STATE_DATE_UUID) != date_old_obj.value(STATE_DATE_UUID) ||
                            date_new_obj.value(STATE_DATE_PLUGIN_ID) != date_old_obj.value(STATE_DATE_PLUGIN_ID) ||
                            date_new_obj.value(STATE_DATE_VALID) != date_old_obj.value(STATE_DATE_VALID) ||
                            date_new_obj.value(STATE_DATE_DELTA_TYPE).toInt() != date_old_obj.value(STATE_DATE_DELTA_TYPE).toInt() ||
                            date_new_obj.value(STATE_DATE_DELTA_FIXED).toDouble() != date_old_obj.value(STATE_DATE_DELTA_FIXED).toDouble() ||
                            date_new_obj.value(STATE_DATE_DELTA_MIN).toDouble() != date_old_obj.value(STATE_DATE_DELTA_MIN).toDouble() ||
                            date_new_obj.value(STATE_DATE_DELTA_MAX).toDouble() != date_old_obj.value(STATE_DATE_DELTA_MAX).toDouble() ||
                            date_new_obj.value(STATE_DATE_DELTA_AVERAGE).toDouble() != date_old_obj.value(STATE_DATE_DELTA_AVERAGE).toDouble() ||
                            date_new_obj.value(STATE_DATE_DELTA_ERROR).toDouble() != date_old_obj.value(STATE_DATE_DELTA_ERROR).toDouble() ||
                            date_new_obj.value(STATE_DATE_SUB_DATES) != date_old_obj.value(STATE_DATE_SUB_DATES) ) {

                            mStructureIsChanged = true;
                            return;
                        }
                        ++date_old;
                     }
                }
               ++ev_old;
            }

        }
        // Check events constraintes modification
        const QJsonArray &eventsConstNew = stateNew.value(STATE_EVENTS_CONSTRAINTS).toArray();
        const QJsonArray &eventsConstOld = stateOld.value(STATE_EVENTS_CONSTRAINTS).toArray();

        if ( eventsConstNew.size() != eventsConstOld.size()) {
            mStructureIsChanged = true;
            return;

        } else if (eventsConstNew != eventsConstOld) {
            mStructureIsChanged = true;
            return;

        }
        // Check Curve parameters modification
        const QJsonObject &curve_New = stateNew.value(STATE_CURVE).toObject();
        const QJsonObject &curve_Old = stateOld.value(STATE_CURVE).toObject();
        if (curve_New != curve_Old) {
           mStructureIsChanged = true;
           return;
        }

        // Check MCMC parameters modification
        const QJsonObject &MCMC_New = stateNew.value(STATE_MCMC).toObject();
        const QJsonObject &MCMC_Old = stateOld.value(STATE_MCMC).toObject();

        if (MCMC_New != MCMC_Old) {
            mStructureIsChanged = true;
            return;
        }
    }

}

void Project::unselectedAllInState(QJsonObject &state)
{
    QJsonArray phases = state.value(STATE_PHASES).toArray();
    for (QJsonArray::iterator iPhase = phases.begin(); iPhase != phases.end(); ++iPhase) {
        QJsonObject p = iPhase->toObject();
        p[STATE_IS_CURRENT] = false;
        p[STATE_IS_SELECTED] = false;
        *iPhase = p;
    }
    state[STATE_PHASES] = phases;

    QJsonArray events = state.value(STATE_EVENTS).toArray();
    for (QJsonArray::iterator iEvent = events.begin(); iEvent != events.end(); ++iEvent) {
        QJsonObject e =iEvent->toObject();
        e[STATE_IS_CURRENT] = false;
        e[STATE_IS_SELECTED] = false;

        QJsonArray dates = iEvent->toObject().value(STATE_EVENT_DATES).toArray();
        for (QJsonArray::iterator idate = dates.begin(); idate != dates.end(); ++idate){
            QJsonObject d = idate->toObject();
            d[STATE_IS_SELECTED] = false;
            *idate = d;
        }
        e[STATE_EVENT_DATES] = dates;
        *iEvent = e;
    }
    state[STATE_EVENTS] = events;

    qDebug()<<"[Project::unselectedAllInState] end";

}

bool Project::structureIsChanged()
{
    return mStructureIsChanged;
}

bool Project::designIsChanged()
{
    return mDesignIsChanged;
}


// Event handler for events of type "StateEvent".
// Updates the project state by calling updateState() and send a notification (if required).
bool Project::event(QEvent* e)
{
    if (e->type() == QEvent::User) {
        StateEvent* se = static_cast<StateEvent*>(e);
        if (se)
            updateState(se->state(), se->reason(), se->notify());

        qDebug() << "(---) [Project::event]";
        return true;
    } /*else if (e->type() == 1001) {
#ifdef DEBUG
        qDebug() << "(---) Receiving events selection : adapt checked phases";
#endif
        emit selectedEventsChanged();

        return true;
    } */
/*else if (e->type() == 1002) {
#ifdef DEBUG
        //qDebug() << "(---) Receiving phases selection : adapt selected events ?????";
#endif
        emit selectedPhasesChanged();//no used, previously it was used to select the Event corresponding to a selected phase
        return true;
    } */

    else
        return QObject::event(e);

}



// Update the project state directly.
// This is not async! so be careful when calling this from views with notify = true
void Project::updateState(const QJsonObject &state, const QString &reason, bool notify)
{
    qDebug() << " [Project::updateState] ---  reason = " << reason << " notify= " << notify;
    mState = std::move(state);
    if (reason == NEW_PROJECT_REASON)
       showStudyPeriodWarning();

    if (notify) {
        emit projectStateChanged(); // connect to updateMultiCalibration() et MainWindows::updateProject
    }
}


// Project File Management
/**
 * @brief Project::load
 * @param path
 * @param force used for chronomodel_bash, all versions are accepted, but all calibrations are rebuilt
 * @return
 */
bool Project::load(const QString &path, bool force)
{
    bool newerProject = false;
    bool olderProject = false;

    const QString appVersionStr = QApplication::applicationVersion();

    QFileInfo checkFile(path);
    if (!checkFile.exists() || !checkFile.isFile()) {
        QMessageBox message(QMessageBox::Critical,
                            tr("Error loading project file"),
                            tr("The project file could not be loaded.") + "\r" +
                            path  +
                            tr("Could not be find"),
                            QMessageBox::Ok,
                            qApp->activeWindow());
        message.exec();
        return false;
    }
    QFile file(path);

    qDebug() << "[Project::load] Loading project file : " << path;


    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QFileInfo info(path);
        MainWindow::getInstance()->setCurrentPath(info.absolutePath());

        AppSettings::mLastDir = info.absolutePath();
        AppSettings::mLastFile = info.fileName();
        mName = info.fileName();
        QByteArray saveData = file.readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc (QJsonDocument::fromJson(saveData, &error));

        if (error.error !=  QJsonParseError::NoError) {
            QMessageBox message(QMessageBox::Critical,
                                tr("Error loading project file"),
                                tr("The project file could not be loaded.") + "\r" +
                                tr("Error message") + " : " + error.errorString(),
                                QMessageBox::Ok,
                                qApp->activeWindow());
            message.exec();
            return false;
        } else {
            if (mModel->mNumberOfEvents >0)
                mModel->clear_and_shrink();

            QJsonObject loadingState = jsonDoc.object();
            QStringList projectVersionList;
            if (loadingState.contains(STATE_APP_VERSION)) {
                const QString projectVersionStr = loadingState.value(STATE_APP_VERSION).toString();
                projectVersionList = projectVersionStr.split(".");

                const QStringList appVersionList = appVersionStr.split(".");

                if (loadingState.value(STATE_SETTINGS).isNull()) {
                    QJsonObject settings;
                    settings[STATE_SETTINGS_TMIN] = loadingState.value(STATE_SETTINGS_TMIN);
                    settings[STATE_SETTINGS_TMAX] = loadingState.value(STATE_SETTINGS_TMAX);
                    settings[STATE_SETTINGS_STEP] = loadingState.value(STATE_SETTINGS_STEP);
                    settings[STATE_SETTINGS_STEP_FORCED] = loadingState.value(STATE_SETTINGS_STEP_FORCED);
                    loadingState[STATE_SETTINGS] = settings;
                }
                if ( !loadingState.value(STATE_SETTINGS_TMIN).isNull()) {
                    // erase old json version
                    loadingState.remove(STATE_SETTINGS_TMIN);
                    loadingState.remove(STATE_SETTINGS_TMAX);
                    loadingState.remove(STATE_SETTINGS_STEP);
                    loadingState.remove(STATE_SETTINGS_STEP_FORCED);

                }
                // warning box if step_forced
                if (loadingState.value(STATE_SETTINGS).toObject().value(STATE_SETTINGS_STEP_FORCED).toBool() == true) {
                    QMessageBox message(QMessageBox::Information,
                                        tr("Study Period Step Forced"),
                                        tr("This project contains a forced time step.\r\rThis change calculation time"),
                                        QMessageBox::Ok,
                                        qApp->activeWindow());
                    message.exec();
                }
                if (projectVersionList.size() == 3 && appVersionList.size() == 3) {

                    if (projectVersionList[0].toInt() > appVersionList[0].toInt())
                        newerProject = true;

                    else if (projectVersionList[0].toInt() == appVersionList[0].toInt()) { // version
                            if (projectVersionList[1].toInt() > appVersionList[1].toInt())
                                newerProject = true;
                            else if (projectVersionList[1].toInt() < appVersionList[1].toInt())
                                olderProject = true;
                        // Test on build number
                      /*  else if (projectVersionList[1].toInt() == appVersionList[1].toInt()) {
                                if (projectVersionList[2].toInt() > appVersionList[2].toInt())
                                    newerProject = true;
                        }*/
                    }  else {
                        olderProject = true;
                    }

                    if (newerProject) {
                        QMessageBox message(QMessageBox::Warning,
                                            tr("Project version doesn't match"),
                                            "This project has been saved with a newer version of ChronoModel :\r\r- Project version : " + projectVersionStr
                                            + "\r- Current version : " + appVersionStr
                                            + "\r\rSome incompatible data may be missing and you may encounter problems running the model.\r\rLoading the project will update and overwrite the existing file. Do you really want to continue ?",
                                            QMessageBox::Yes | QMessageBox::No,
                                            qApp->activeWindow());

                        if (message.exec() == QMessageBox::No)
                            return false;

                    }

                    if (olderProject && force == false) {
                        QMessageBox message(QMessageBox::Warning,
                                            tr("Project version doesn't match"),
                                            "This project has been saved with an older version of ChronoModel :\r\r- Project version : " + projectVersionStr + "\r- Current version : " + appVersionStr + "\r\rSome incompatible data may be missing and you may encounter problems running the model.\r\rLoading the project will update and overwrite the existing file. Do you really want to continue ?",
                                            QMessageBox::Yes | QMessageBox::No,
                                            qApp->activeWindow());
                        if (message.exec() == QMessageBox::No)
                            return false;
                    }
                }
            }
            file.close();
            //  Ask all plugins if dates are corrects.
            //  If not, it may be an incompatibility between plugins versions (new parameter added for example...)
            //  This function gives a chance to plugins to modify dates saved with old versions in order to use them with the new version.
            qDebug() << "[Project::load]  begin checkDatesCompatibility";
            bool isCorrected;
            loadingState = checkDatesCompatibility(loadingState, isCorrected);

            qDebug() << "[Project::load]  end checkDatesCompatibility";
            //  Check if dates are valid on the current study period
            mState = emptyState();
            mState = checkValidDates(loadingState);

            if (olderProject)
                AppSettings::mIsSaved = false; //force saving later
            else
                AppSettings::mIsSaved = true;
            recenterProject();

            qDebug() << "[Project::load]  unselectedAllInState";
            unselectedAllInState(mState); // modify mState

            // If a version update is to be done :
            mState[STATE_APP_VERSION] = appVersionStr;

            if (AppSettings::mAutoSave) {
                mAutoSaveTimer->setInterval(AppSettings::mAutoSaveDelay * 1000);
                mAutoSaveTimer->start();
            } else
                mAutoSaveTimer->stop();



           // -------------------- look for the calibration file --------------------

            QString caliPath = path + ".cal";
            QFileInfo calfi(caliPath);

            if (calfi.isFile() && !isCorrected && force == false) {
                // Load Calibration Curve
                QFile calFile(caliPath);
                if (calFile.open(QIODevice::ReadOnly) && calFile.exists()) {
                    qDebug() << "[Project::load] Loading model file.cal : " << calFile.fileName() << " size =" << calFile.size();
                    QDataStream in(&calFile);

                    int QDataStreamVersion;
                    in >> QDataStreamVersion;
                    in.setVersion(QDataStreamVersion);
                    if (in.version()!= QDataStream::Qt_6_4) {
                            //setNoResults(true);
                            clear_and_shrink_model();
                            return true;
                    }

                    QString caliVersion;
                    in >> caliVersion;
                    // prepare the future
                    mCalibCurves.clear();
                    if (caliVersion != appVersionStr) {
                        QFileInfo fileInfo(calFile);
                        QString filename(fileInfo.fileName());
                        QString strMessage = tr("%1 has been done with a different version = %2").arg(filename, caliVersion) + " \n (" + tr("current") + " =  "
                                + qApp->applicationVersion() + ") \n"+ tr("Do you really want to load the calibration file: *.chr.cal ?");
                        QString strTitle = tr("Compatibility risk");
                        QMessageBox message(QMessageBox::Question, strTitle, strMessage, QMessageBox::Yes | QMessageBox::No, qApp->activeWindow());
                        if (message.exec() == QMessageBox::No) {
                                setNoResults(true);
                                clear_and_shrink_model();
                                return true;
                        }

                    }
                    // loading cal curve
                    try {
                        mCalibCurves = std::map<std::string, CalibrationCurve>();
                        quint32 siz;
                        in >> siz;
                        for (int i = 0; i < int(siz); ++i) {
                            QString descript;
                            in >> descript;
                            CalibrationCurve cal;
                            in >> cal;
                            mCalibCurves.insert_or_assign(descript.toStdString(), cal);
                        }

                     }
                    catch (const std::exception & e) {
                        QMessageBox message(QMessageBox::Warning,
                                            tr("Error loading project "),
                                            tr("The Calibration file could not be loaded.") + "\r" +
                                            tr("Error %1").arg(e.what()),
                                            QMessageBox::Ok,
                                            qApp->activeWindow());
                        message.exec();
                    }

                }
                calFile.close();
            } else {
                setNoResults(true);
                clear_and_shrink_model();
                return true;
            }

            clear_and_shrink_model();

            /* -------------------- Load results -------------------- */
            /* Changement du fichier *.res dés la version 3.2.2; disparition de la sauvegarde de mFormat dans les MetropolisVariable */
            /* if (projectVersionList[0].toInt() <= 3 && projectVersionList[1].toInt() <= 2 && projectVersionList[2].toInt() < 2)
                return true;
            */

            QString dataPath = path + ".res";

            QFile dataFile;

            dataFile.setFileName(dataPath);

            QFileInfo fi(dataFile);
            dataFile.open(QIODevice::ReadOnly);
            if (fi.isFile()) { // if there is only bounds mCalibCurves.empty()) {
                if (dataFile.exists()) {

                    qDebug() << "[Project::load] Loading model file.res : " << dataPath << " size=" << dataFile.size();

                    try {
                        mModel.reset();
                        mModel = std::make_shared<ModelCurve>(mState);

                        qDebug() << "[Project::load] Create a ModelCurve";

                    }
                    catch (const std::exception & e) {
                        QMessageBox message(QMessageBox::Warning,
                                            tr("Error loading project"),
                                            tr("The project could not be loaded.") + "\r" +
                                            tr("Error : %1").arg(e.what()),
                                            QMessageBox::Ok,
                                            qApp->activeWindow());
                        message.exec();

                        clear_and_shrink_model();
                        return false;
                    }


                    try {
                        mModel->setProject();

                        QFile file(dataPath);
                        if (file.exists() && file.open(QIODevice::ReadOnly)){

                        QDataStream in(&file);

                        mModel->restoreFromFile(&in);

                        mModel->generateCorrelations(mModel->mChains);
                        setNoResults(false);

                     } else {
                         setNoResults(true);
                         clear_and_shrink_model();
                     }

                    } catch (const std::exception & e) {
                        QMessageBox message(QMessageBox::Critical,
                                            tr("Error loading project MCMC results"),
                                            tr("The project MCMC results could not be loaded.") + "\r" +
                                            tr("Error : %1").arg(e.what()),
                                            QMessageBox::Ok,
                                            qApp->activeWindow());
                        setNoResults(true);
                        clear_and_shrink_model();
                        message.exec();
                    }
                } else {
                    qDebug() << "[Project::load] no file.res : "<< dataPath;

                    setNoResults(true);
                    clear_and_shrink_model();
                }
            } else {
                qDebug() << "[Project::load] no file.res : "<< dataPath;
                setNoResults(true);
                clear_and_shrink_model();
            }
            // --------------------

            return true;
        }
    }
    file.close();
    MainWindow::getInstance()->updateWindowTitle();
    return false;
}

bool Project::save()
{
    if (this->mState == emptyState()) {
        return true;

    } else {
        QFileInfo info(AppSettings::mLastDir + "/" + AppSettings::mLastFile);
        return info.exists() ? saveProjectToFile() : saveAs(tr("Save current project as..."));
    }
}


bool Project::insert(const QString &path, QJsonObject &return_state)
{
    bool newerProject = false;
    bool olderProject = false;

    QFileInfo checkFile(path);
    if (!checkFile.exists() || !checkFile.isFile()) {
        QMessageBox message(QMessageBox::Critical,
                            tr("Error loading project file"),
                            tr("The project file could not be loaded.") + "\r" +
                            path  +
                            tr("Could not be find"),
                            QMessageBox::Ok,
                            qApp->activeWindow());
        message.exec();
        return false;
    }
    QFile file(path);

    qDebug() << "[Project::insert] Insert project file : " << path;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QFileInfo info(path);
        MainWindow::getInstance()->setCurrentPath(info.absolutePath());

        QByteArray saveData = file.readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc (QJsonDocument::fromJson(saveData, &error));

        if (error.error !=  QJsonParseError::NoError) {
            QMessageBox message(QMessageBox::Critical,
                                tr("Error loading project file"),
                                tr("The project file could not be loaded.") + "\r" +
                                tr("Error message") + " : " + error.errorString(),
                                QMessageBox::Ok,
                                qApp->activeWindow());
            message.exec();
            return false;

        } else {
            if (mModel)
                mModel->clear();

            QJsonObject loadedState = jsonDoc.object();

            if (loadedState.contains(STATE_APP_VERSION)) {
                QString projectVersionStr = loadedState.value(STATE_APP_VERSION).toString();
                QStringList projectVersionList = projectVersionStr.split(".");

                QString appVersionStr = QApplication::applicationVersion();
                QStringList appVersionList = appVersionStr.split(".");


                if (projectVersionList.size() == 3 && appVersionList.size() == 3) {

                    if (projectVersionList[0].toInt() > appVersionList[0].toInt())
                        newerProject = true;

                    else if (projectVersionList[0].toInt() == appVersionList[0].toInt()) { // version
                            if (projectVersionList[1].toInt() > appVersionList[1].toInt())
                                newerProject = true;
                            else if (projectVersionList[1].toInt() < appVersionList[1].toInt())
                                olderProject = true;

                    }  else {
                        olderProject = true;
                    }

                    if (newerProject) {
                        QMessageBox message(QMessageBox::Warning,
                                            tr("Project version doesn't match"),
                                            "This project has been saved with a newer version of ChronoModel :\r\r- Project version : " + projectVersionStr
                                            + "\r- Current version : " + appVersionStr
                                            + "\r\rSome incompatible data may be missing and you may encounter problems running the model.\r\rLoading the project will update and overwrite the existing file. Do you really want to continue ?",
                                            QMessageBox::Yes | QMessageBox::No,
                                            qApp->activeWindow());

                        if (message.exec() == QMessageBox::No)
                            return false;

                    }

                    if (olderProject) {
                        QMessageBox message(QMessageBox::Warning,
                                            tr("Project version doesn't match"),
                                            "This project has been saved with an older version of ChronoModel :\r\r- Project version : " + projectVersionStr + "\r- Current version : " + appVersionStr + "\r\rSome incompatible data may be missing and you may encounter problems running the model.\r\rLoading the project will update and overwrite the existing file. Do you really want to continue ?",
                                            QMessageBox::Yes | QMessageBox::No,
                                            qApp->activeWindow());
                        if (message.exec() == QMessageBox::No)
                            return false;
                    }
                }
            }
            file.close();
            //  Ask all plugins if dates are corrects.
            //  If not, it may be an incompatibility between plugins versions (new parameter added for example...)
            //  This function gives a chance to plugins to modify dates saved with old versions in order to use them with the new version.
           qDebug() << "[Project::insert]  begin checkDatesCompatibility";
           bool isCorrected;
           loadedState = checkDatesCompatibility(loadedState, isCorrected);

            qDebug() << "[Project::insert]  end checkDatesCompatibility";
            //  Check if dates are valid on the current study period
            loadedState = checkValidDates(loadedState);


            // 1 - find min-max position and min max index in the mState yet in memory
           double minXEvent (HUGE_VAL), maxXEvent(- HUGE_VAL), minYEvent(HUGE_VAL), maxYEvent(- HUGE_VAL);
           double minXPhase(HUGE_VAL), maxXPhase(- HUGE_VAL), minYPhase(HUGE_VAL), maxYPhase(- HUGE_VAL);
           int  maxIDEvent(0), maxIDPhase(0), maxIDEventConstraint(0), maxIDPhaseConstraint(0);

           const QJsonArray &phases = mState.value(STATE_PHASES).toArray();

           if (phases.isEmpty()) {
               maxXPhase = 0;

           } else {
               for (auto&& phaseJSON : phases) {
                   QJsonObject phase = phaseJSON.toObject();
                   minXPhase = std::min(minXPhase, phase[STATE_ITEM_X].toDouble());
                   maxXPhase = std::max(maxXPhase, phase[STATE_ITEM_X].toDouble());

                   minYPhase = std::min(minYPhase, phase[STATE_ITEM_Y].toDouble());
                   maxYPhase = std::max(maxYPhase, phase[STATE_ITEM_Y].toDouble());
                   maxIDPhase = std::max(maxIDPhase, phase[STATE_ID].toInt());
               }
               maxIDPhase += 1;

               const QJsonArray phaseConstraints = mState.value(STATE_PHASES_CONSTRAINTS).toArray();
               for (auto&& phaseConsJSON : phaseConstraints) {
                   QJsonObject phaseCons = phaseConsJSON.toObject();
                   maxIDPhaseConstraint = std::max(maxIDPhaseConstraint, phaseCons[STATE_ID].toInt());
               }
               maxIDPhaseConstraint += 1;

           }

           const QJsonArray &events = mState.value(STATE_EVENTS).toArray();
           if (events.isEmpty()) {
               maxXEvent = 0;

           } else {
               for (auto&& eventJSON : events) {
                   const QJsonObject &event = eventJSON.toObject();
                   minXEvent = std::min(minXEvent, event.value(STATE_ITEM_X).toDouble());
                   maxXEvent = std::max(maxXEvent, event.value(STATE_ITEM_X).toDouble());

                   minYEvent = std::min(minYEvent, event.value(STATE_ITEM_Y).toDouble());
                   maxYEvent = std::max(maxYEvent, event.value(STATE_ITEM_Y).toDouble());
                   maxIDEvent = std::max(maxIDEvent, event.value(STATE_ID).toInt());
               }
               maxIDEvent += 1;

               const QJsonArray &eventConstraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();
               for (auto&& eventConsJSON : eventConstraints) {
                   const QJsonObject &eventCons = eventConsJSON.toObject();
                   maxIDEventConstraint = std::max(maxIDEventConstraint, eventCons.value(STATE_ID).toInt());
               }
               maxIDEventConstraint += 1;
            }

           // 2- Find min X and  min -max Y in the imported project
           double minXEventNew (HUGE_VAL);
           double minYEventNew (HUGE_VAL), maxYEventNew (-HUGE_VAL);
           const QJsonArray &eventsNew = loadedState.value(STATE_EVENTS).toArray();
           if (eventsNew.isEmpty()) {
               minXEventNew = 0;

               minYEventNew = 0;
               maxYEventNew = 0;

           } else {
               for (auto&& eventJSON : eventsNew) {
                   const QJsonObject &event = eventJSON.toObject();
                   minXEventNew = std::min(minXEventNew, event.value(STATE_ITEM_X).toDouble());

                   minYEventNew = std::min(minYEventNew, event.value(STATE_ITEM_Y).toDouble());
                   maxYEventNew = std::max(maxYEventNew, event.value(STATE_ITEM_Y).toDouble());
               }
          }

           double minXPhaseNew (HUGE_VAL);
           double minYPhaseNew (HUGE_VAL), maxYPhaseNew (-HUGE_VAL);
           const QJsonArray phasesNew = loadedState.value(STATE_PHASES).toArray();
           if (phasesNew.isEmpty()) {
               minXPhaseNew = 0;

               minYPhaseNew = 0;
               maxYPhaseNew = 0;

           } else {
               for (auto&& phaseJSON : phasesNew) {
                   const QJsonObject &phase = phaseJSON.toObject();
                   minXPhaseNew = std::min(minXPhaseNew, phase.value(STATE_ITEM_X).toDouble());

                   minYPhaseNew = std::min(minYPhaseNew, phase.value(STATE_ITEM_Y).toDouble());
                   maxYPhaseNew = std::max(maxYPhaseNew, phase.value(STATE_ITEM_Y).toDouble());

               }
            }
          // 3 - Shift the new loadedState

           QJsonObject new_state = loadedState;

           QJsonArray newPhases = new_state.value(STATE_PHASES).toArray();
           for (auto&& phaseJSON : newPhases) {
               QJsonObject phase = phaseJSON.toObject();
               // set on the right+ + mItemWidth(150.) in AbstractItem.h
               phase[STATE_ITEM_X] =  phase.value(STATE_ITEM_X).toDouble() + maxXPhase - minXPhaseNew + 200;
               // center on Y
               phase[STATE_ITEM_Y] = phase.value(STATE_ITEM_Y).toDouble() - (maxYPhaseNew + minYPhaseNew)/2.;
               phase[STATE_ID] = phase.value(STATE_ID).toInt() + maxIDPhase;

               phaseJSON = phase;
           }
           new_state[STATE_PHASES] = newPhases;

           QJsonArray newPhaseConstraints = new_state.value(STATE_PHASES_CONSTRAINTS).toArray();
           for (auto&& phaseConsJSON : newPhaseConstraints) {
               QJsonObject phaseCons = phaseConsJSON.toObject();
               phaseCons[STATE_ID] = phaseCons[STATE_ID].toInt() + maxIDPhaseConstraint;
               phaseCons[STATE_CONSTRAINT_FWD_ID] = phaseCons.value(STATE_CONSTRAINT_FWD_ID).toInt() + maxIDPhase;
               phaseCons[STATE_CONSTRAINT_BWD_ID] = phaseCons.value(STATE_CONSTRAINT_BWD_ID).toInt() + maxIDPhase;

               phaseConsJSON = phaseCons;
           }
           new_state[STATE_PHASES_CONSTRAINTS] = newPhaseConstraints;

           QJsonArray newEvents = new_state.value(STATE_EVENTS).toArray();
           for (auto&& eventJSON : newEvents) {
              QJsonObject event = eventJSON.toObject();
              // set on the right + mItemWidth(150.) in AbstractItem.h
              event[STATE_ITEM_X] = event.value(STATE_ITEM_X).toDouble() + maxXEvent - minXEventNew + 200;
              // center on Y
              event[STATE_ITEM_Y] = event.value(STATE_ITEM_Y).toDouble() - (maxYEventNew + minYEventNew)/2. ;
              event[STATE_ID] = event.value(STATE_ID).toInt() + maxIDEvent;

              std::vector<int> mPhasesIds  = QStringToStdVectorInt(event.value(STATE_EVENT_PHASE_IDS).toString(), ",");
              for (size_t i = 0; i<mPhasesIds.size(); ++i)
                  mPhasesIds[i] += maxIDPhase;

              event[STATE_EVENT_PHASE_IDS] = StdVectorIntToQString( mPhasesIds, ",");

              eventJSON = event;
           }
           new_state[STATE_EVENTS] = newEvents;

           QJsonArray newEventConstraints = new_state.value(STATE_EVENTS_CONSTRAINTS).toArray();
           for (auto&& eventConsJSON : newEventConstraints) {
               QJsonObject eventCons = eventConsJSON.toObject();
               eventCons[STATE_ID] = eventCons.value(STATE_ID).toInt() + maxIDEventConstraint;

               eventCons[STATE_CONSTRAINT_FWD_ID] = eventCons.value(STATE_CONSTRAINT_FWD_ID).toInt() + maxIDEvent;
               eventCons[STATE_CONSTRAINT_BWD_ID] = eventCons.value(STATE_CONSTRAINT_BWD_ID).toInt() + maxIDEvent;

              eventConsJSON = eventCons;
           }

           new_state[STATE_EVENTS_CONSTRAINTS] = newEventConstraints;

           // 4 - Adding the new loadedState after mState
           return_state = mState;

           QJsonArray nextPhases = mState.value(STATE_PHASES).toArray();
           for (auto&& phaseJSON : newPhases)
               nextPhases.append(phaseJSON.toObject());
           return_state[STATE_PHASES] = nextPhases;

           QJsonArray nextPhaseConstraints = mState.value(STATE_PHASES_CONSTRAINTS).toArray();
           for (auto&& phaseConsJSON : newPhaseConstraints)
               nextPhaseConstraints.append(phaseConsJSON.toObject());
           return_state[STATE_PHASES_CONSTRAINTS] = nextPhaseConstraints;

           QJsonArray nextEvents = mState.value(STATE_EVENTS).toArray();
           for (auto&& eventJSON : newEvents)
               nextEvents.append(eventJSON.toObject());

           return_state[STATE_EVENTS] = nextEvents;

           QJsonArray nextEventConstraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();
           for (auto&& eventConsJSON : newEventConstraints)
               nextEventConstraints.append(eventConsJSON.toObject());
           return_state[STATE_EVENTS_CONSTRAINTS] = nextEventConstraints;

           unselectedAllInState(return_state); // modify mState

           clear_model();

           qDebug() << "[Project::insert]  unselectedAllInState";
           return true;
        }
    }
    return false;
}


/**
 * @brief Project::saveAs On native box of MacOS the title is not show
 * @param dialogTitle
 * @return
 */

bool Project::saveAs(const QString& dialogTitle)
{
    QString path = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                           dialogTitle,
                                           MainWindow::getInstance()->getCurrentPath(),
                                           tr("Chronomodel Project (*.chr)"));


    if (!path.isEmpty()) {
        QFileInfo info(path);
        MainWindow::getInstance()->setCurrentPath(info.absolutePath());

        AppSettings::mLastDir = info.absolutePath();
        AppSettings::mLastFile = info.fileName();
        mName = AppSettings::mLastFile;
        // We need to reset mLastSavedState because it corresponds
        // to the last saved state in the previous file.
        //mLastSavedState = QJsonObject();
        AppSettings::mIsSaved = false;
        return saveProjectToFile();
    }
    return false;
}

bool Project::askToSave(const QString& saveDialogTitle)
{
    // Check if modifs have been made
    (void) saveDialogTitle;

    // We have some modifications : ask to save :
    int result = QMessageBox::question(QApplication::activeWindow(),
                                       QApplication::applicationName(),
                                       tr("Do you want to save the current project ?"),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (result == QMessageBox::Yes) {
        // return true if saving is done correcty
        return save();

    } else if (result == QMessageBox::No) {
        // the user doesn't want to save : return true to continue
        return true;

    } else if (result == QMessageBox::Cancel) {
        // the user canceled : return false to cancel any further operations
        return false;
    }
    return false;
}

bool Project::saveProjectToFile()
{

    QString path = AppSettings::mLastDir + "/" + AppSettings::mLastFile;
    QFile file_chr(path);

    QFile file_cal(AppSettings::mLastDir + "/" + AppSettings::mLastFile + ".cal");
    QFile file_res(AppSettings::mLastDir + "/" + AppSettings::mLastFile + ".res");

    /*
     * Save in case of no backup or different version.
     */
    if (AppSettings::mIsSaved == false || mState[STATE_APP_VERSION] != QApplication::applicationVersion()) {
        // Creation of a copy of the last successful result
        /*   if (mNoResults && file_res.exists() && file_cal.exists()) {

            file_chr.copy(path + "_bak");

            file_cal.copy(path + ".cal_bak");

            file_res.copy(path + ".res_bak");
        }
        */

        if (file_chr.open(QIODevice::ReadWrite | QIODevice::Text)) {

            qDebug() << "[Project::saveProjectToFile] Project saved to : " << path;
#if DEBUG
            if (mState["events"].toArray().isEmpty())
                qDebug() << "[Project::saveProjectToFile] empty Project saved ???? ";
#endif
            // reset version number
            mState[STATE_APP_VERSION] = QApplication::applicationVersion();

            QJsonDocument jsonDoc(mState);
            QByteArray textDoc = jsonDoc.toJson(QJsonDocument::Indented);
            file_chr.write(textDoc);

            file_chr.resize(file_chr.pos());
            file_chr.close();

            AppSettings::mIsSaved = true;

        } else {
            AppSettings::mIsSaved = false;
            return false;
        }

    }
#ifdef DEBUG
     else {
        //qDebug() << "Nothing new to save in project model";
        return true;
    }
#endif
    if (file_cal.open(QIODevice::WriteOnly)) {

        QDataStream out(&file_cal);
        out.setVersion(QDataStream::Qt_6_4);
        out << out.version();

        out << QApplication::applicationVersion();
        // save Calibration Curve
        out << quint32 (mCalibCurves.size());
        for (auto it = mCalibCurves.cbegin() ; it != mCalibCurves.cend(); ++it) {
            out << QString::fromStdString(it->first);
            out << it->second;
        }
        file_cal.close();

        AppSettings::mIsSaved = true;
    }

    QFileInfo checkFile(file_res.fileName());
    if (checkFile.exists() && checkFile.isFile())
        file_res.remove();

    if (!mNoResults && !mModel->mEvents.empty()) {

        qDebug() << "[Project::saveProjectToFile] Saving project results in "<< AppSettings::mLastDir + "/" + AppSettings::mLastFile + ".res";

        mModel->setProject();

        // -----------------------------------------------------
        //  Create file
        // -----------------------------------------------------
        //QFileInfo info(fileName);
        //QFile file(info.path() + info.baseName() + ".~res"); // when we could do a compressed file
        //QFile file(info.path() + info.baseName() + ".res");

        if (file_res.open(QIODevice::WriteOnly)) {
            QDataStream out(&file_res);
            mModel->saveToFile(&out);
            file_res.close();

            AppSettings::mIsSaved = true;
        }

    }
    MainWindow::getInstance()->updateWindowTitle();
    return true;
}

bool Project::recenterProject()
{
    double minXEvent (HUGE_VAL), maxXEvent(- HUGE_VAL), minYEvent(HUGE_VAL), maxYEvent(- HUGE_VAL);
    double minXPhase(HUGE_VAL), maxXPhase(- HUGE_VAL), minYPhase(HUGE_VAL), maxYPhase(- HUGE_VAL);

    const QJsonArray phases = mState.value(STATE_PHASES).toArray();
    for (const auto phaseJSON : phases) {
        const QJsonObject& phase = phaseJSON.toObject();
        minXPhase =std::min(minXPhase, phase[STATE_ITEM_X].toDouble());
        maxXPhase =std::max(maxXPhase, phase[STATE_ITEM_X].toDouble());

        minYPhase =std::min(minYPhase, phase[STATE_ITEM_Y].toDouble());
        maxYPhase =std::max(maxYPhase, phase[STATE_ITEM_Y].toDouble());
    }

    const QJsonArray events = mState.value(STATE_EVENTS).toArray();
    for (const auto eventJSON : events) {
        const QJsonObject& event = eventJSON.toObject();
        minXEvent =std::min(minXEvent, event[STATE_ITEM_X].toDouble());
        maxXEvent =std::max(maxXEvent, event[STATE_ITEM_X].toDouble());

        minYEvent =std::min(minYEvent, event[STATE_ITEM_Y].toDouble());
        maxYEvent =std::max(maxYEvent, event[STATE_ITEM_Y].toDouble());
    }


    QJsonObject newState = mState;
    QJsonArray newPhases = newState.value(STATE_PHASES).toArray();

    double shift_X_phase = (maxXPhase + minXPhase)/2.;
    double shift_Y_phase = (maxYPhase + minYPhase)/2.;

    const qreal delta (180); //= AbstractItem::mItemWidth
    shift_X_phase = ceil(shift_X_phase/delta) * delta;
    shift_Y_phase = ceil(shift_Y_phase/delta) * delta;

    for (auto&& phaseJSON : newPhases) {
        QJsonObject phase = phaseJSON.toObject();
        phase[STATE_ITEM_X] = phase[STATE_ITEM_X].toDouble() - shift_X_phase;
        phase[STATE_ITEM_Y] = phase[STATE_ITEM_Y].toDouble() - shift_Y_phase;
        phaseJSON = phase;
    }
    newState[STATE_PHASES] = newPhases;

    double shift_X_event = (maxXEvent + minXEvent)/2.;
    double shift_Y_event = (maxYEvent + minYEvent)/2.;

    shift_X_event = ceil(shift_X_event/delta) * delta;
    shift_Y_event = ceil(shift_Y_event/delta) * delta;

    QJsonArray newEvents = newState.value(STATE_EVENTS).toArray();
    for (auto&& eventJSON : newEvents) {
       QJsonObject event = eventJSON.toObject();
       event[STATE_ITEM_X] = event[STATE_ITEM_X].toDouble() - shift_X_event ;
       event[STATE_ITEM_Y] = event[STATE_ITEM_Y].toDouble() - shift_Y_event ;
       eventJSON = event;
    }
    newState[STATE_EVENTS] = newEvents;
    mState = newState;

    return true;
}

# pragma mark  Project Settings

bool Project::setSettings(const StudyPeriodSettings& settings)
{
    if (settings.mTmin >= settings.mTmax) {
        QMessageBox message(QMessageBox::Critical, tr("Inconsistent values"), tr("Start Date must be lower than End Date !"), QMessageBox::Ok, qApp->activeWindow());
        message.exec();
        return false;
    }

    else {
        QJsonObject stateNext = mState;

        // Set the new srudy period in the new state
        stateNext[STATE_SETTINGS] = settings.toJson();

        // Check if dates are still valid on the new study period
        stateNext = checkValidDates(stateNext);

        //  Push the new state having a new study period with dates' "valid flag" updated!
        return pushProjectState(stateNext, PROJECT_SETTINGS_UPDATED_REASON, true);
    }
}

void Project::setAppSettingsAutoSave()
{
    mAutoSaveTimer->setInterval(AppSettings::mAutoSaveDelay * 1000);

    if(mAutoSaveTimer->isActive() && !AppSettings::mAutoSave)
        mAutoSaveTimer->stop();

    else if(!mAutoSaveTimer->isActive() && AppSettings::mAutoSave)
        mAutoSaveTimer->start();
}

/**
 * @brief Project::restoreMCMCSettings not used
 */
void Project::restoreMCMCSettings()
{
    MCMCSettings settings;
    settings.restoreDefault();

    QJsonObject stateNext = mState;
    stateNext[STATE_MCMC] = settings.toJson();
    pushProjectState(stateNext, MCMC_SETTINGS_RESTORE_DEFAULT_REASON, true);

}

void Project::mcmcSettings()
{
    MCMCSettingsDialog dialog(qApp->activeWindow(), AppSettings::mShowHelp);
    MCMCSettings settings = MCMCSettings::fromJson(mState.value(STATE_MCMC).toObject());

    dialog.setSettings(settings);
    dialog.setModal(true);

    if (dialog.exec() == QDialog::Accepted) {
        MCMCSettings settings = dialog.getSettings();

        QJsonObject stateNext = mState;
        stateNext[STATE_MCMC] = settings.toJson();
        pushProjectState(stateNext, MCMC_SETTINGS_UPDATED_REASON, true);
    }
}

/**
 * @brief Project::resetMCMC Restore default sampling methods on each ti and theta
 */
void Project::resetMCMC()
{
    QMessageBox message(QMessageBox::Warning,
                        tr("Reset MCMC methods"),
                        tr("All event's and data's MCMC methods will be reset to their default value.\rDo you really want to do this ?"),
                        QMessageBox::Yes | QMessageBox::No,
                        qApp->activeWindow());
    if (message.exec() == QMessageBox::Yes) {
        QJsonObject stateNext = mState;
        QJsonArray events = mState.value(STATE_EVENTS).toArray();

        for (int i = 0; i < events.size(); ++i) {
            QJsonObject event = events.at(i).toObject();
            event[STATE_EVENT_SAMPLER] = int (MHVariable::eDoubleExp);

            QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
            for (int j = 0; j < dates.size(); ++j) {
                QJsonObject date = dates.at(j).toObject();

                try {
                    Date d;
                    d.fromJson(date);
                    if (!d.isNull()) {
                        date[STATE_DATE_SAMPLER] = int (d.mPlugin->getDataMethod());
                        dates[j] = date;
                    }
                } catch(QString error) {
                    QMessageBox message(QMessageBox::Critical,
                                        qApp->applicationName() + " " + qApp->applicationVersion(),
                                        tr("Error : %1").arg(error),
                                        QMessageBox::Ok,
                                        qApp->activeWindow());
                    message.exec();
                }
            }
            event[STATE_EVENT_DATES] = dates;
            events[i] = event;
        }
        stateNext[STATE_EVENTS] = events;
        stateNext[STATE_MCMC_MIXING] = MCMC_MIXING_DEFAULT;
        pushProjectState(stateNext, MCMC_METHODE_RESET_REASON, true);
    }
}

bool Project::studyPeriodIsValid()
{
    const QJsonObject settings = mState.value(STATE_SETTINGS).toObject();
    const int tmin = settings.value(STATE_SETTINGS_TMIN).toInt();
    const int tmax = settings.value(STATE_SETTINGS_TMAX).toInt();
    if (tmin >= tmax)
        showStudyPeriodWarning();

    return (tmin < tmax);
}

void Project::showStudyPeriodWarning()
{
    // Display help message
    QMessageBox message(QMessageBox::Information,
                        tr("Study period definition required"),
                        tr("To start your new project, you first have to define a study period."),
                        QMessageBox::Ok,
                        qApp->activeWindow());
    message.exec();
}


// --------------------------------------------------------------------
//     Events
// --------------------------------------------------------------------

int Project::getUnusedEventId(const QJsonArray& events)
{
    auto as_ID = [](int id) {
        return [id](QJsonValueConstRef ev) { return ev.toObject().value(STATE_ID).toInt() == id; };
    };

    int id = -1;
    bool idIsFree = false;
    while (!idIsFree) {
        ++id;
        idIsFree = !std::ranges::any_of(events, as_ID(id));
    }
    return id;
}

void Project::createEvent(qreal x, qreal y)
{
    if (studyPeriodIsValid()) {
        EventDialog* dialog = new EventDialog(qApp->activeWindow(), tr("New Event"));
        if (dialog->exec() == QDialog::Accepted) {
            Event event = Event();
            event.setName(dialog->getName());

            event.mColor= dialog->getColor();
            QJsonObject eventJSON (event.toJson());
            eventJSON[STATE_ITEM_X] = x;
            eventJSON[STATE_ITEM_Y] = y;
            addEvent(eventJSON, tr("Event created"));

        }
        delete dialog;
    }
}

void Project::createEventKnown(qreal x, qreal y)
{
    if (studyPeriodIsValid()) {
        EventDialog* dialog = new EventDialog(qApp->activeWindow(), tr("New Bound"));
        if (dialog->exec() == QDialog::Accepted) {
            Bound bound = Bound();
            bound.setName(dialog->getName());

            bound.mColor= dialog->getColor();
            QJsonObject eventJSON (bound.toJson());
            eventJSON[STATE_ITEM_X] = x;
            eventJSON[STATE_ITEM_Y] = y;
            addEvent(eventJSON, tr("Bound created"));
        }
        delete dialog;
    }
}

void Project::addEvent(QJsonObject event, const QString& reason)
{
    QJsonObject stateNext = mState;
    QJsonArray events = stateNext.value(STATE_EVENTS).toArray();

    event[STATE_ID] = getUnusedEventId(events);
    events.append(event);
    stateNext[STATE_EVENTS] = events;

    pushProjectState(stateNext, reason, true);
}


void Project::deleteSelectedEvents()
{
    QJsonObject stateNext = mState;

    QJsonArray new_events_list ;
    QJsonArray new_events_constraints ;

    QList<int> id_events_remove;

    QJsonArray events = mState.value(STATE_EVENTS).toArray() ;
    QJsonArray events_constraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();
    QJsonArray events_trash = mState.value(STATE_EVENTS_TRASH).toArray();

    // Create new Events list
    for (int i = 0 ; i < events.size(); ++i) {
        if (!events.at(i).toObject().value(STATE_IS_SELECTED).toBool()) {
            new_events_list.append(events.at(i).toObject());

        } else {
            QJsonObject eventsToRemove = events.at(i).toObject();
           // eventsToRemove[STATE_EVENT_DATES] = events.at(i).toObject().value(STATE_EVENT_DATES).toArray();
            events_trash.append(eventsToRemove);
            id_events_remove.append(eventsToRemove.value(STATE_ID).toInt());

        }
    }
    // Create new events constraint
    for (int i = 0; i < events_constraints.size(); ++i) {
        QJsonObject constraint = events_constraints.at(i).toObject();
        const int bwd_id = constraint.value(STATE_CONSTRAINT_BWD_ID).toInt();
        const int fwd_id = constraint.value(STATE_CONSTRAINT_FWD_ID).toInt();

        if ( !id_events_remove.contains(bwd_id) && !id_events_remove.contains(fwd_id) )
            new_events_constraints.append(constraint);
    }

    stateNext[STATE_EVENTS] = new_events_list;
    stateNext[STATE_EVENTS_CONSTRAINTS] = new_events_constraints;
    stateNext[STATE_EVENTS_TRASH] = events_trash;

    pushProjectState(stateNext, "Event(s) deleted", true);

    // send to clear the propertiesView
    emit currentEventChanged(nullptr);

    clear_model();
    MainWindow::getInstance() -> setResultsEnabled(false);
    MainWindow::getInstance() -> setLogEnabled(false);
}

void Project::deleteSelectedTrashedEvents(const QList<int>& ids)
{
    QJsonObject stateNext = mState;

    QJsonArray events_trash = mState.value(STATE_EVENTS_TRASH).toArray();

    for (auto i = events_trash.size()-1; i >= 0; --i) {
        QJsonObject event = events_trash[i].toObject();
        int id = event.value(STATE_ID).toInt();

        if (ids.contains(id))
            events_trash.removeAt(i);

    }
    stateNext[STATE_EVENTS_TRASH] = events_trash;

    pushProjectState(stateNext, "Trashed event(s) deleted", true);
}

void Project::recycleEvents()
{
    TrashDialog dialog(TrashDialog::eEvent, qApp->activeWindow());
    if (dialog.exec() == QDialog::Accepted) {
        QList<int> indexes = dialog.getSelectedIndexes();

        QJsonObject stateNext = mState;
        QJsonArray events = mState.value(STATE_EVENTS).toArray();
        QJsonArray events_trash = mState.value(STATE_EVENTS_TRASH).toArray();

        QJsonArray new_events_trash;

        QJsonObject settingsJson = stateNext.value(STATE_SETTINGS).toObject();
        StudyPeriodSettings settings = StudyPeriodSettings::fromJson(settingsJson);

        for (int i = 0; i < indexes.size(); ++i) {
            QJsonObject event = events_trash.at(indexes[i]).toObject();
            const int id = getUnusedEventId(events);
            event[STATE_ID] = id;
            Event::Type type = (Event::Type) event.value(STATE_EVENT_TYPE).toInt();

            /* if the event is default type, we have to validate the dates
             * if the event is know type, juste insert the json
             */
            if (type == Event::Type::eDefault) {
                QJsonArray dates = event[STATE_EVENT_DATES].toArray();
                QJsonArray new_dates;
                for (int j = 0; j < dates.size(); ++j) {
                    QJsonObject new_date = dates.at(j).toObject();
                    // Validate the date before adding it to the correct event and pushing the state
                    PluginAbstract* plugin = PluginManager::getPluginFromId(new_date[STATE_DATE_PLUGIN_ID].toString());
                    bool valid = plugin->isDateValid(new_date[STATE_DATE_DATA].toObject(), settings);
                    new_date[STATE_DATE_VALID] = valid;

                    new_date[STATE_ID] = getUnusedDateId(dates);
                    // Add UUID since version 2.1.3
                    if (new_date.find(STATE_DATE_UUID) == new_date.end()) {
                        new_date[STATE_DATE_UUID] = QString::fromStdString(Generator::UUID());
                    }
                    new_dates.append(new_date);
                }
                event[STATE_EVENT_DATES] = new_dates;

            }

            events.append(event);

        }

        // create new trash
        for (int i = 0; i < events_trash.size(); ++i) {
            if (!indexes.contains(i))
                new_events_trash.append(events_trash[i].toObject());
        }

        stateNext[STATE_EVENTS] = std::move(events);
        stateNext[STATE_EVENTS_TRASH] = std::move(new_events_trash);

        pushProjectState(stateNext, "Event(s) restored", true);
    }
}

void Project::updateEvent(const QJsonObject& event, const QString& reason)
{
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    bool found = false;

    for (QJsonValueRef eventRef : events) {
        QJsonObject eventObj = eventRef.toObject();
        if (eventObj.value(STATE_ID).toInt() == event.value(STATE_ID).toInt()) {
            eventRef = event;
            found = true;
            break;
        }
    }

    if (found) {
        QJsonObject stateNext = mState;
        stateNext[STATE_EVENTS] = events;
        pushProjectState(stateNext, reason, true);
    }
}

/**
 * @brief Project::mergeEvents Offer to insert all the dates of an event inside an other event
 * @param eventFromId
 * @param eventToId
 */
void Project::mergeEvents(int eventFromId, int eventToId)
{
    QJsonObject stateNext = mState;
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    QJsonObject eventFrom;
    QJsonObject eventTo;
    qsizetype fromIndex = -1;
    qsizetype toIndex = -1;

    for (qsizetype i = events.size()-1; i >= 0; --i) {
        QJsonObject evt = events.at(i).toObject();
        int id = evt.value(STATE_ID).toInt();
        if (id == eventFromId) {
            eventFrom = evt;
            fromIndex = i;
        } else if (id == eventToId) {
            eventTo = evt;
            toIndex = i;
        }
    }
    QJsonArray datesFrom = eventFrom.value(STATE_EVENT_DATES).toArray();
    QJsonArray datesTo = eventTo.value(STATE_EVENT_DATES).toArray();
    for (qsizetype i=0; i<datesFrom.size(); ++i) {
        QJsonObject dateFrom = datesFrom.at(i).toObject();
        dateFrom[STATE_ID] = getUnusedDateId(datesTo);
        datesTo.append(dateFrom);
    }
    eventTo[STATE_EVENT_DATES] = datesTo;

    events[toIndex] = eventTo;
    events.removeAt(fromIndex);
    stateNext[STATE_EVENTS] = events;

    // Delete constraints around the disappearing event
    QJsonArray constraints = stateNext.value(STATE_EVENTS_CONSTRAINTS).toArray();
    for (qsizetype i = constraints.size()-1; i >= 0; --i) {
        QJsonObject c = constraints.at(i).toObject();
        int fromId = c.value(STATE_CONSTRAINT_BWD_ID).toInt();
        int toId = c.value(STATE_CONSTRAINT_FWD_ID).toInt();
        if (eventFromId == fromId || eventFromId == toId)
            constraints.removeAt(i);

    }
    stateNext[STATE_EVENTS_CONSTRAINTS] = constraints;

    pushProjectState(stateNext, "Events merged", true);
}

// Grouped actions on events

/*void Project::selectAllEvents()
{
    const QJsonArray events = mState.value(STATE_EVENTS).toArray();
    QJsonArray newEvents = QJsonArray();
    for (int i = 0; i < events.size(); ++i) {
         QJsonObject evt = events.at(i).toObject();
         evt[STATE_IS_SELECTED] = true;
         newEvents.append(evt);

     }
    // create new state to push
    QJsonObject stateNext = mState;
    stateNext[STATE_EVENTS] = newEvents;
    pushProjectState(stateNext, "Select All Events", true);
}*/

void Project::selectAllEvents()
{
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    bool modified = false;

    for (QJsonValueRef eventRef : events) {
        QJsonObject eventObj = eventRef.toObject();
        if (!eventObj.contains(STATE_IS_SELECTED) || !eventObj[STATE_IS_SELECTED].toBool()) {
            eventObj[STATE_IS_SELECTED] = true;
            eventRef = eventObj;
            modified = true;
        }
    }

    if (modified) {
        QJsonObject stateNext = mState;
        stateNext[STATE_EVENTS] = events;
        pushProjectState(stateNext, "Select All Events", true);
    }
}


bool Project::selectEventsFromSelectedPhases()
{
    bool res = false;
    const QJsonArray events = mState.value(STATE_EVENTS).toArray();
    QJsonArray newEvents = QJsonArray();
    for (int i = 0; i < events.size(); ++i) {
         QJsonObject evt = events.at(i).toObject();
         const QString idsStr = evt.value(STATE_EVENT_PHASE_IDS).toString();
         const QStringList ids = idsStr.split(",");
         bool willBeSelected = false;
         foreach (const QString id, ids) {
             // if id=="" id.toInt return 0 or it is not the number of the phase, there is no phase
             if (id != "") {
                 const QJsonObject pha = getPhasesWithId(id.toInt());
                 if (pha.value(STATE_IS_SELECTED) == true) {
                     willBeSelected = true;
                     res = true;
                     break;
                 }
             }
         }
         evt[STATE_IS_SELECTED] = willBeSelected;
         newEvents.append(evt);

     }
    // create new state to push
    QJsonObject stateNext = mState;
    stateNext[STATE_EVENTS] = newEvents;
    pushProjectState(stateNext, "Select events in selected phases", true);
    return res;
}

bool Project::selectedEventsWithString(const QString str)
{
    bool res = false;
    const QJsonArray events = mState.value(STATE_EVENTS).toArray();
    QJsonArray newEvents = QJsonArray();
    for (auto e : events) {
        QJsonObject evt = e.toObject();
        evt[STATE_IS_SELECTED] = e.toObject().value(STATE_NAME).toString().contains(str);
        if (e.toObject().value(STATE_NAME).toString().contains(str))
            res = true;
        newEvents.append(evt);
     }
    // create new state to push
    QJsonObject stateNext = mState;
    stateNext[STATE_EVENTS] = newEvents;
    pushProjectState(stateNext, "Select events with string", true);
    return res;
}

void Project::updateSelectedEventsColor(const QColor& color)
{
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    bool modified = false;

    for (QJsonValueRef eventRef : events) {
        QJsonObject eventObj = eventRef.toObject();
        if (eventObj.value(STATE_IS_SELECTED).toBool()) {
            eventObj[STATE_COLOR_RED] = color.red();
            eventObj[STATE_COLOR_GREEN] = color.green();
            eventObj[STATE_COLOR_BLUE] = color.blue();
            eventRef = eventObj;
            modified = true;
        }
    }

    if (modified) {
        QJsonObject stateNext = mState;
        stateNext[STATE_EVENTS] = events;
        pushProjectState(stateNext, "Update selected events color", true);
    }
}


/*void Project::updateSelectedEventsMethod(MHVariable::SamplerProposal sp)
{
    QJsonObject stateNext = mState;
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    for (int i = 0; i<events.size(); ++i) {
        QJsonObject evt = events.at(i).toObject();
        if (evt.value(STATE_IS_SELECTED).toBool()) {
            evt[STATE_EVENT_SAMPLER] = sp;
            events[i] = evt;
        }
    }
    stateNext[STATE_EVENTS] = events;
    pushProjectState(stateNext, "Update selected events method", true);
}*/
void Project::updateSelectedEventsMethod(MHVariable::SamplerProposal sp)
{
    // Create a copy of the current state to modify.
    QJsonObject stateNext = mState;

    // Retrieve the array of events from the current state.
    QJsonArray events = stateNext.value(STATE_EVENTS).toArray();

    // Iterate through the events and update the sampler for selected events.
    for (const QJsonValue& value : events) {
        QJsonObject event = value.toObject();

        // If the event is selected, update its sampler method.
        if (event.value(STATE_IS_SELECTED).toBool()) {
            event[STATE_EVENT_SAMPLER] = sp;
        }
    }

    // Update the events array in the state object.
    stateNext[STATE_EVENTS] = events;

    // Push the updated state to the project with a description.
    pushProjectState(stateNext, "Update selected events method", true);
}

void Project::updateSelectedEventsDataMethod(MHVariable::SamplerProposal sp, const QString& pluginId)
{
    QJsonObject stateNext = mState;
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    for (int i = 0; i<events.size(); ++i) {
        QJsonObject evt = events[i].toObject();

        if (evt.value(STATE_IS_SELECTED).toBool()) {
            QJsonArray dates = evt[STATE_EVENT_DATES].toArray();

            // Iterate through the dates to update the sampler for the specified plugin.
            for (const QJsonValue& dateValue : dates) {
                QJsonObject date = dateValue.toObject();

                // Update the sampler if the plugin ID matches.
                if (date[STATE_DATE_PLUGIN_ID].toString() == pluginId) {
                    date[STATE_DATE_SAMPLER] = sp;
                }
            }
           /* for (int j = 0; j<dates.size(); ++j) {
                QJsonObject date = dates[j].toObject();
                if (date[STATE_DATE_PLUGIN_ID].toString() == pluginId) {
                    date[STATE_DATE_SAMPLER] = sp;
                    dates[j] = date;
                }
            }*/
            evt[STATE_EVENT_DATES] = dates;
            events[i] = evt;
        }
    }

    stateNext[STATE_EVENTS] = events;
    pushProjectState(stateNext, "Update selected data method", true);
}

// --------------------------------------------------------------------
//     Dates
// --------------------------------------------------------------------
/** @brief getUnusedDateId find a valid index in a project
 */
/*int Project::getUnusedDateId(const QJsonArray& dates) const
{
    int id = -1;
    bool idIsFree = false;
    while (!idIsFree) {
        ++id;
        idIsFree = true;
        //for (int i = 0; i < dates.size(); ++i) {
        //    QJsonObject date = dates.at(i).toObject();
        for (const auto date : dates) {
            if (date.toObject().value(STATE_ID).toInt() == id) {
                idIsFree = false;
                break;
            }
        }
    }
    return id;
}*/
int Project::getUnusedDateId(const QJsonArray& dates) const
{
    int id = 0;
    while (true) {
        bool idIsFree = true;
        for (const QJsonValue& dateValue : dates) {
            QJsonObject dateObj = dateValue.toObject();
            if (dateObj.value(STATE_ID).toInt() == id) {
                idIsFree = false;
                break;
            }
        }
        if (idIsFree) {
            return id;
        }
        ++id;
    }
}


Date Project::createDateFromPlugin(PluginAbstract* plugin)
{
    Date date;
    if (plugin) {
        DateDialog dialog(qApp->activeWindow());
        PluginFormAbstract* form = plugin->getForm();
        dialog.setForm(form);
        dialog.setDataMethod(plugin->getDataMethod());

        if (dialog.exec() == QDialog::Accepted) {
            if (form->isValid()) {
                date.mPlugin = plugin;
                date.mData = form->getData();

                date.setName(dialog.getName());
                date.mTi.mSamplerProposal = dialog.getMethod();
                date.mDeltaType = dialog.getDeltaType();
                date.mDeltaFixed = dialog.getDeltaFixed();
                date.mDeltaMin = dialog.getDeltaMin();
                date.mDeltaMax = dialog.getDeltaMax();
                date.mDeltaAverage = dialog.getDeltaAverage();
                date.mDeltaError = dialog.getDeltaError();
                date.mUUID = Generator::UUID(); // Add UUID since version 2.1.3
            } else {
                QMessageBox message(QMessageBox::Critical,
                                    tr("Invalid data"),
                                    form->mError,
                                    QMessageBox::Ok,
                                    qApp->activeWindow());
                message.exec();
            }
        }
        form = nullptr;
    }
    return date;
}

void Project::addDate(int eventId, QJsonObject date)
{
    QJsonObject stateNext = mState;

    // Validate the date before adding it to the correct event and pushing the state
    QJsonObject settingsJson = stateNext[STATE_SETTINGS].toObject();
    StudyPeriodSettings settings = StudyPeriodSettings::fromJson(settingsJson);
    PluginAbstract* plugin = PluginManager::getPluginFromId(date[STATE_DATE_PLUGIN_ID].toString());
    bool valid = plugin->isDateValid(date[STATE_DATE_DATA].toObject(), settings);
    date[STATE_DATE_VALID] = valid;

    // Add the date
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    for (int i = 0; i < events.size(); ++i) {
        QJsonObject evt = events[i].toObject();
        if (evt.value(STATE_ID).toInt() == eventId) {
            date[STATE_ID] = getUnusedDateId(evt[STATE_EVENT_DATES].toArray());
            QJsonArray dates = evt[STATE_EVENT_DATES].toArray();
            dates.append(date);

            evt[STATE_EVENT_DATES] = dates;
            events[i] = evt;
            stateNext[STATE_EVENTS] = events;
            pushProjectState(stateNext, "Date created", true);
            break;
        }
    }
}

QJsonObject Project::checkDatesCompatibility(QJsonObject state, bool& isCorrected)
{
    isCorrected = false;
    QJsonArray events = state.value(STATE_EVENTS).toArray();

    QJsonObject curveSetJSon;
    CurveSettings cs;

    if (state.find("chronocurve") != state.end()) {
        state.remove("chronocurve");
    }

    if (state.find(STATE_CURVE) != state.end()) {
        curveSetJSon = state.value(STATE_CURVE).toObject();
        cs = CurveSettings::fromJson(curveSetJSon);

    } else {
        cs = CurveSettings();
        cs.mProcessType = CurveSettings::eProcess_None;
        state[STATE_CURVE] = cs.toJson();
    }


    for (int i = 0; i<events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();

        // Since v3 , 2021-04-30, ,the key "method" disapered and it's change with "sampler" for EVENT and DATA.
        /*
         *    enum Method{
         * eFixe = -1,  //<  use with Type==eBound
         * eDoubleExp = 0, //<  The default method
         * eBoxMuller = 1,
         * eMHAdaptGauss = 2,
         */
        if (event.find(STATE_EVENT_METHOD) != event.end()) {
            switch (event.value(STATE_EVENT_METHOD).toInt()) {
            case -1 :
                event[STATE_EVENT_SAMPLER] = MHVariable::eFixe;
                break;
            case 0 :
                event[STATE_EVENT_SAMPLER] = MHVariable::eDoubleExp;
                break;
            case 1 :
                event[STATE_EVENT_SAMPLER] = MHVariable::eBoxMuller;
                break;
            case 2:
                event[STATE_EVENT_SAMPLER] = MHVariable::eMHAdaptGauss;
                break;

            }
            event.remove(STATE_EVENT_METHOD);
        }

        // Since v 3.1.4
        if (event.find("YInc") != event.end()) {
            event[STATE_EVENT_X_INC_DEPTH] = event.value("YInc").toDouble();
            event.remove("YInc");
        }
        if (event.find("SInc") != event.end()) {
            event[STATE_EVENT_SX_ALPHA95_SDEPTH] = event.value("SInc").toDouble();
            event.remove("SInc");
        }
        if (event.find("YDec") != event.end()) {
            event[STATE_EVENT_Y_DEC] = event.value("YDec").toDouble();
            event[STATE_EVENT_SY] = 0.0;
            event.remove("YDec");
        }
        if (event.find("YInt") != event.end()) {
            if ( cs.mProcessType == CurveSettings::eProcess_Univariate ||
                 cs.mProcessType == CurveSettings::eProcess_Depth )
                 event[STATE_EVENT_X_INC_DEPTH] = event.value("YInt").toDouble();
            else
                event[STATE_EVENT_Z_F] = event.value("YInt").toDouble();

            event.remove("YInt");
        }
        if (event.find("SInt") != event.end()) {
            if ( cs.mProcessType == CurveSettings::eProcess_Univariate ||
                 cs.mProcessType == CurveSettings::eProcess_Depth )
                 event[STATE_EVENT_SX_ALPHA95_SDEPTH] = event.value("SInt").toDouble();
            else
                event[STATE_EVENT_SZ_SF] = event.value("SInt").toDouble();

            event.remove("SInt");

        }
        // since v3.2.1
        if (event.find(STATE_EVENT_POINT_TYPE) == event.end()) {
            event[STATE_EVENT_POINT_TYPE] = Event::PointType::ePoint;
        }

        QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
        for (int j = 0; j < dates.size(); ++j) {
            QJsonObject date = dates.at(j).toObject();

            // -----------------------------------------------------------
            //  Check the date compatibility with the plugin version.
            //  Here, we can control if all date fields are present, and add them if not.
            // -----------------------------------------------------------
            if (date.find(STATE_DATE_SUB_DATES) == date.end()) {
                date[STATE_DATE_SUB_DATES] = QJsonArray();
                isCorrected = true;
            }

            if (date.find(STATE_DATE_VALID) == date.end()) {
                date[STATE_DATE_VALID] = true;
                isCorrected = true;
            }
            // Add UUID since version 2.1.3
            if (date.find(STATE_DATE_UUID) == date.end()) {
                date[STATE_DATE_UUID] = QString::fromStdString(Generator::UUID());
            }

            if (date[STATE_DATE_PLUGIN_ID].toString() == "typo_ref." || date[STATE_DATE_PLUGIN_ID].toString() == "typo") {
                date[STATE_DATE_PLUGIN_ID] = QString("unif"); // since version 2.0.14
                isCorrected = true;
            }


            if (date.find(STATE_DATE_METHOD) != date.end()) { // since version 3.0
                switch (date.value(STATE_DATE_METHOD).toInt()) {
                case 0 :
                    date[STATE_DATE_SAMPLER] = MHVariable::eMHPrior;
                    break;
                case 1 :
                    date[STATE_DATE_SAMPLER] = MHVariable::eInversion;
                    break;
                case 2:
                    date[STATE_DATE_SAMPLER] = MHVariable::eMHAdaptGauss;
                    break;
                default: // old version is eMHSymGaussAdapt = 5
                    date[STATE_DATE_SAMPLER] = MHVariable::eMHAdaptGauss;
                    break;

                }
                date.remove(STATE_DATE_METHOD);
            }

            if (date.value(STATE_DATE_SAMPLER).toInt()>4)
                date[STATE_DATE_SAMPLER] = MHVariable::eMHAdaptGauss;
            // etc...

            // -----------------------------------------------------------
            //  Check the date compatibility with the plugin version
            //  Only the STATE_DATE_DATA is to be checked
            // -----------------------------------------------------------
            QString pluginId = date[STATE_DATE_PLUGIN_ID].toString();
            PluginAbstract* plugin = PluginManager::getPluginFromId(pluginId);
            date[STATE_DATE_DATA] = plugin->checkValuesCompatibility(date[STATE_DATE_DATA].toObject());

            // -----------------------------------------------------------
            //  Check subdates compatibility with the plugin version
            // -----------------------------------------------------------
            QJsonArray subdates = date[STATE_DATE_SUB_DATES].toArray();
            for (int k = 0; k < subdates.size(); ++k) {
                QJsonObject subdate = subdates[k].toObject();
                subdate[STATE_DATE_DATA] = plugin->checkValuesCompatibility(subdate[STATE_DATE_DATA].toObject());
                // Add UUID since version 2.1.3
                if (subdate.find(STATE_DATE_UUID) == subdate.end()) {
                    subdate[STATE_DATE_UUID] = QString::fromStdString(Generator::UUID());
                }

                if (subdate.find(STATE_DATE_METHOD) == date.end()) { // since version 3.0
                    switch (subdate.value(STATE_DATE_METHOD).toInt()) {
                    case 0 :
                        subdate[STATE_DATE_SAMPLER] = MHVariable::eMHPrior;
                        break;
                    case 1 :
                        subdate[STATE_DATE_SAMPLER] = MHVariable::eInversion;
                        break;
                    case 2:
                        subdate[STATE_DATE_SAMPLER] = MHVariable::eMHAdaptGauss;
                        break;
                    default: // old version is eMHSymGaussAdapt = 5
                        date[STATE_DATE_SAMPLER] = MHVariable::eMHAdaptGauss;
                        break;
                    }

                    subdate.remove(STATE_DATE_METHOD);
                }
                
                subdates[k] = subdate;
            }
            date[STATE_DATE_SUB_DATES] = subdates;
            // -----------------------------------------------------------

            dates[j] = date;
            event[STATE_EVENT_DATES] = dates;

        }
        events[i] = event;
        state[STATE_EVENTS] = events;
    }
    // conversion since version 1.4 test, obsolete since V3, never used
  /*  bool phaseConversion = false;
    for (int i = 0; i < phases.size(); i++) {
       QJsonObject phase = phases.at(i).toObject();
       if ( phase[STATE_PHASE_TAU_TYPE].toInt() == Phase::eTauRange) {
           phase[STATE_PHASE_TAU_TYPE] = Phase::eTauFixed;
           phase[STATE_PHASE_TAU_FIXED] = phase[STATE_PHASE_TAU_MAX];
           phases[i] = phase;
           phaseConversion = true;
           isCorrected = true;
       }
    }
    if (phaseConversion)
        state[STATE_PHASES] = phases;
    */
    return state;
}

/**
 * @brief Project::updateDate : Updates the data of a date included in mState event in json.
 * Open the dialogBox
 * @param eventId
 * @param dateIndex
 */
void Project::updateDate(int eventId, int dateIndex)
{

    const QJsonObject& settingsJson = mState.value(STATE_SETTINGS).toObject();
    StudyPeriodSettings settings = StudyPeriodSettings::fromJson(settingsJson);
    QJsonArray events = mState.value(STATE_EVENTS).toArray();

    for (int i = 0; i < events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
        if (event.value(STATE_ID).toInt() == eventId) {
            QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
            if (dateIndex < dates.size()) {
                QJsonObject date = dates[dateIndex].toObject();

                QString pluginId = date.value(STATE_DATE_PLUGIN_ID).toString();
                PluginAbstract* plugin = PluginManager::getPluginFromId(pluginId);

                DateDialog dialog(qApp->activeWindow());
                PluginFormAbstract* form = plugin->getForm();
                dialog.setForm(form);
                dialog.setDate(date);

                if (dialog.exec() == QDialog::Accepted) {
                    if (form->isValid()) {
                        // this reason is not inside the list of mReasonChangeStructure
                        // or mReasonChangeDesign or mReasonChangePosition,
                        // we force to use the fonction checkStateModification() within pushProjectState()

                        date[STATE_DATE_DATA] = form->getData();

                        date[STATE_NAME] = dialog.getName();
                        date[STATE_DATE_SAMPLER] = dialog.getMethod();

                        date[STATE_DATE_DELTA_TYPE] = dialog.getDeltaType();
                        date[STATE_DATE_DELTA_FIXED] = dialog.getDeltaFixed();
                        date[STATE_DATE_DELTA_MIN] = dialog.getDeltaMin();
                        date[STATE_DATE_DELTA_MAX] = dialog.getDeltaMax();
                        date[STATE_DATE_DELTA_AVERAGE] = dialog.getDeltaAverage();
                        date[STATE_DATE_DELTA_ERROR] = dialog.getDeltaError();

                        PluginAbstract* plugin = PluginManager::getPluginFromId(date.value(STATE_DATE_PLUGIN_ID).toString());
                        const bool valid = plugin->isDateValid(date.value(STATE_DATE_DATA).toObject(), settings);
                        plugin = nullptr;
                        date[STATE_DATE_VALID] = valid;

                        if (dates[dateIndex].toObject() != date) {
                            QJsonObject state = mState;

                            dates[dateIndex] = date;
                            event[STATE_EVENT_DATES] = dates;
                            events[i] = event;
                            state[STATE_EVENTS] = events;

                            pushProjectState(state, "Date from dialog", true);
                        }

                    } else {
                        QMessageBox message(QMessageBox::Critical,
                                            tr("Invalid data"),
                                            form->mError,
                                            QMessageBox::Ok,
                                            qApp->activeWindow());
                        message.exec();
                    }
                }
                plugin = nullptr;
                form = nullptr;
            }
            break;
        }
    }
}

void Project::deleteDates(int eventId, const QList<int>& dateIndexes)
{
    QJsonArray events = mState.value(STATE_EVENTS).toArray();

    for (int i = 0; i < events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
        if (event.value(STATE_ID).toInt() == eventId) {
            QJsonArray dates_trash = mState.value(STATE_DATES_TRASH).toArray();
            QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();

            for (auto j = dates.size()-1; j >= 0; --j) {
                if (dateIndexes.contains(j)) {
                    QJsonObject date = dates.takeAt(j).toObject();
                    dates_trash.append(date);
                }
            }
            event[STATE_EVENT_DATES] = dates;
            events[i] = event;

            QJsonObject state = mState;
            state[STATE_EVENTS] = events;
            state[STATE_DATES_TRASH] = dates_trash;

            pushProjectState(state, QString::number(dateIndexes.size()) + " date(s) deleted", true);

            break;
        }
    }
    clear_model();
    MainWindow::getInstance() -> setResultsEnabled(false);
    MainWindow::getInstance() -> setLogEnabled(false);
}

void Project::deleteSelectedTrashedDates(const QList<int> &ids)
{
    QJsonObject stateNext = mState;

    QJsonArray dates_trash = mState.value(STATE_DATES_TRASH).toArray();

    for (auto i = dates_trash.size()-1; i >= 0; --i) {
        QJsonObject date = dates_trash.at(i).toObject();
        int id = date.value(STATE_ID).toInt();

        if (ids.contains(id))
            dates_trash.removeAt(i);

    }
    stateNext[STATE_DATES_TRASH] = dates_trash;

    pushProjectState(stateNext, "Trashed data deleted", true);
}

void Project::recycleDates(int eventId)
{
    TrashDialog dialog(TrashDialog::eDate, qApp->activeWindow());
    if (dialog.exec() == QDialog::Accepted) {
        QList<int> indexes = dialog.getSelectedIndexes();

        QJsonObject stateNext = mState;
        QJsonArray events = mState.value(STATE_EVENTS).toArray();
        QJsonArray dates_trash = mState.value(STATE_DATES_TRASH).toArray();

        for (int i = 0; i < events.size(); ++i) {
            QJsonObject event = events.at(i).toObject();
            if (event.value(STATE_ID).toInt() == eventId){
                QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
                for (auto i = indexes.size()-1; i >= 0; --i) {
                    QJsonObject date = dates_trash.takeAt(indexes.at(i)).toObject();
                    // Validate the date before adding it to the correct event and pushing the state
                    QJsonObject settingsJson = stateNext[STATE_SETTINGS].toObject();
                    StudyPeriodSettings settings = StudyPeriodSettings::fromJson(settingsJson);
                    PluginAbstract* plugin = PluginManager::getPluginFromId(date[STATE_DATE_PLUGIN_ID].toString());
                    bool valid = plugin->isDateValid(date[STATE_DATE_DATA].toObject(), settings);
                    date[STATE_DATE_VALID] = valid;


                    date[STATE_ID] = getUnusedDateId(dates);
                    dates.append(date);
                }
                event[STATE_EVENT_DATES] = dates;
                events[i] = event;
                stateNext[STATE_EVENTS] = events;
                stateNext[STATE_DATES_TRASH] = dates_trash;

                pushProjectState(stateNext, QString::number(indexes.size()) + " date(s) restored", true);

                break;
            }
        }
    }
}

QJsonObject Project::checkValidDates(const QJsonObject& stateToCheck)
{
    QJsonObject state = stateToCheck;

    QJsonObject settingsJson = state.value(STATE_SETTINGS).toObject();
    StudyPeriodSettings settings = StudyPeriodSettings::fromJson(settingsJson);

    QJsonArray events = state.value(STATE_EVENTS).toArray();
    for (int i = 0; i < events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
        QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
        for (int j=0; j<dates.size(); ++j) {
            QJsonObject date = dates.at(j).toObject();

            PluginAbstract* plugin = PluginManager::getPluginFromId(date.value(STATE_DATE_PLUGIN_ID).toString());
            if (date.value(STATE_DATE_ORIGIN).toInt() == Date::eSingleDate) {
                date[STATE_DATE_VALID] = plugin->isDateValid(date.value(STATE_DATE_DATA).toObject(), settings);
                
            } else if (date.value(STATE_DATE_ORIGIN).toInt() == Date::eCombination) {
                 date[STATE_DATE_VALID] = plugin->isCombineValid(date.value(STATE_DATE_DATA).toObject(), settings);
                
            } else
                date[STATE_DATE_VALID] = false;

            dates[j] = date;
        }
        event[STATE_EVENT_DATES] = dates;
        events[i] = event;
    }
    state[STATE_EVENTS] = events;
    return state;
}

QJsonArray Project::getInvalidDates(){
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    QJsonArray invalidDates;
    for (int i=0; i<events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
        QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
        for (int j=0; j<dates.size(); ++j){
            QJsonObject date = dates.at(j).toObject();
            if(!date[STATE_DATE_VALID].toBool()){
                date["event_name"] = event.value(STATE_NAME);
                invalidDates.push_back(date);
            }
        }
    }
    return invalidDates;
}

void Project::combineDates(const int eventId, const QList<int>& dateIds)
{
    QJsonObject stateNext = mState;
    QJsonObject settingsJson = stateNext.value(STATE_SETTINGS).toObject();
    StudyPeriodSettings settings = StudyPeriodSettings::fromJson(settingsJson);
    QJsonArray events = mState.value(STATE_EVENTS).toArray();

    for (int i=0; i<events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
        if (event.value(STATE_ID).toInt() == eventId) {
            // All event dates :
            QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();

            // event dates to be merged :
            QJsonArray datesToMerge;
            for (int j=0; j<dates.size(); ++j) {
                QJsonObject date = dates[j].toObject();
                if (dateIds.contains(date.value(STATE_ID).toInt()))
                    datesToMerge.push_back(date);

            }

            // merge !
            if (datesToMerge.size() > 0) {
                PluginAbstract* plugin = PluginManager::getPluginFromId(datesToMerge[0].toObject().value(STATE_DATE_PLUGIN_ID).toString());
                if (plugin) {

                    // add the new combination
                    QJsonObject mergedDate = plugin->mergeDates(datesToMerge);
                    if (mergedDate.find("error") != mergedDate.end()) {
                        QMessageBox message(QMessageBox::Critical,
                                            tr("Cannot combine"),
                                            mergedDate["error"].toString(),
                                            QMessageBox::Ok,
                                            qApp->activeWindow());
                        message.exec();
                    } else {
                        // remove merged dates
                        for (qsizetype j=dates.size()-1; j>=0; --j) {
                            QJsonObject date = dates[j].toObject();
                            if (dateIds.contains(date.value(STATE_ID).toInt())) {
                                dates.removeAt(j);
                            }
                        }

                        // Validate the date before adding it to the correct event and pushing the state
                        const bool valid = plugin->isCombineValid(mergedDate, settings);
                        mergedDate[STATE_DATE_VALID] = valid;
                       // mergedDate[STATE_DATE_ORIGIN] = Date::eCombination; // done by plugin->mergeDate
                        mergedDate[STATE_ID] = getUnusedDateId(dates);
                        dates.push_back(mergedDate);
                        
                    }
                }
            }

            event[STATE_EVENT_DATES] = dates;

            events[i] = event;
            stateNext[STATE_EVENTS] = events;
            
            pushProjectState(stateNext, "Dates combined", true);

            break;
        }
    }
}

void Project::splitDate(const int eventId, const int dateId)
{
    QJsonObject stateNext = mState;
    QJsonObject settingsJson = stateNext.value(STATE_SETTINGS).toObject();
    StudyPeriodSettings settings = StudyPeriodSettings::fromJson(settingsJson);
    QJsonArray events = mState.value(STATE_EVENTS).toArray();

    for (int i=0; i<events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
        if (event.value(STATE_ID).toInt() == eventId) {
            QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
            for (int j=0; j<dates.size(); ++j) {
                QJsonObject date = dates.at(j).toObject();
                if (date.value(STATE_ID).toInt() == dateId) {
                    
                    // We have found the date to split !
                    QJsonArray subdates = date.value(STATE_DATE_SUB_DATES).toArray();
                    PluginAbstract* plugin = PluginManager::getPluginFromId(date.value(STATE_DATE_PLUGIN_ID).toString());

                    for (int k = 0; k<subdates.size(); ++k) {
                        QJsonObject sd = subdates.at(k).toObject();
                        bool valid = plugin->isDateValid(sd[STATE_DATE_DATA].toObject(), settings);
                        sd[STATE_DATE_VALID] = valid;
                        sd[STATE_DATE_ORIGIN] = Date::eSingleDate;
                        sd[STATE_ID] =getUnusedDateId(dates);
                        dates.push_back(sd);
                    }
                    dates.removeAt(j);
                    break;
                }
            }
            event[STATE_EVENT_DATES] = dates;
        }
        events[i] = event;
    }
    stateNext[STATE_EVENTS] = events;
    pushProjectState(stateNext, "Dates splitted", true);
}

// Grouped actions on dates
void Project::updateAllDataInSelectedEvents(const QHash<QString, QVariant>& groupedAction)
{
    QJsonObject stateNext = mState;
    QJsonArray events = mState.value(STATE_EVENTS).toArray();

    for (int i = 0; i < events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
        if ((event.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) &&  // Not a bound
           (event.value(STATE_IS_SELECTED).toBool())) // Event is selected
        {
            QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
            for (int j = 0; j<dates.size(); ++j) {
                QJsonObject date = dates.at(j).toObject();
                if (date.value(STATE_DATE_PLUGIN_ID).toString() == groupedAction.value("pluginId").toString()) {
                    QJsonObject data = date.value(STATE_DATE_DATA).toObject();
                    data[groupedAction["valueKey"].toString()] = groupedAction.value("value").toString();
                    date[STATE_DATE_DATA] = data;
                    dates[j] = date;
                }
            }
            event[STATE_EVENT_DATES] = dates;
            events[i] = event;
        }
    }
    stateNext[STATE_EVENTS] = events;

    pushProjectState(stateNext, "Grouped action applied : " + groupedAction.value("title").toString(), true);
}

void Project::createPhase(qreal x, qreal y, QWidget* parent)
{
    if (studyPeriodIsValid()) {
        PhaseDialog* dialog = new PhaseDialog(parent);//qApp->activeWindow());
        const Phase& p = Phase();
        dialog->setPhase(p.toJson());

        if (dialog->exec() == QDialog::Accepted ) {
            if (dialog->isValid()) {
                QJsonObject phase = dialog->getPhase();
                QJsonObject stateNext = mState;
                QJsonArray phases = stateNext.value(STATE_PHASES).toArray();

                phase[STATE_ID] = getUnusedPhaseId(phases);

                phase[STATE_ITEM_X] = x;
                phase[STATE_ITEM_Y] = y;

                phases.append(phase);
                stateNext[STATE_PHASES] = phases;

                pushProjectState(stateNext, "Phase created", true);

            } else {
                QMessageBox message(QMessageBox::Critical,
                                    tr("Invalid value"),
                                    dialog->mError,
                                    QMessageBox::Ok,
                                    qApp->activeWindow());
                message.exec();
                return;
            }
            
        }
        delete dialog;
    }
}

void Project::clear_calibCurves()
{
    for (auto &c: mCalibCurves) {
        c.second.mMap.clear();
        c.second.mRepartition.clear();
        c.second.mRepartition.shrink_to_fit();
        c.second.mVector.clear();
        c.second.mVector.shrink_to_fit();
    }
    mCalibCurves.clear();
}

void Project::updatePhase(const QJsonObject& phaseIn)
{
    PhaseDialog* dialog = new PhaseDialog(qApp->activeWindow());
    dialog->setPhase(phaseIn);
    if (dialog->exec() == QDialog::Accepted) {
        if (dialog->isValid()) {
            QJsonObject phase = dialog->getPhase();

            QJsonObject stateNext = mState;
            QJsonArray phases = stateNext.value(STATE_PHASES).toArray();
            QString reason;
            for (int i=0; i<phases.size(); ++i) {
                QJsonObject p = phases.at(i).toObject();
                if (p.value(STATE_ID).toInt() == phase.value(STATE_ID).toInt()) {
                    // check modification type to set mReasonChangeStructure or mReasonChangeDesign in pushProjectState
                    // if only mReasonChangeDesign we don't need to redo calcul to see the result
                    if (p.value(STATE_NAME).toString() != phase.value(STATE_NAME).toString())
                        reason = "Phase name updated";

                    else if (p.value(STATE_COLOR_BLUE).toInt() != phase.value(STATE_COLOR_BLUE).toInt()
                              || p.value(STATE_COLOR_GREEN).toInt() != phase.value(STATE_COLOR_GREEN).toInt()
                              || p.value(STATE_COLOR_RED).toInt() != phase.value(STATE_COLOR_RED).toInt() )
                        reason = "Phase color updated";

                    // set mReasonChangeStructure

                    if (p.value(STATE_PHASE_TAU_TYPE).toInt() != phase.value(STATE_PHASE_TAU_TYPE).toInt())
                        reason = "Phase updated";

                    else if (p.value(STATE_PHASE_TAU_FIXED).toDouble() != phase.value(STATE_PHASE_TAU_FIXED).toDouble())
                        reason = "Phase updated";


                    phases[i] = phase;

                    break;
                }
            }
            stateNext[STATE_PHASES] = phases;
            pushProjectState(stateNext, reason, true);

        } else {
            QMessageBox message(QMessageBox::Critical,
                                tr("Invalid value"),
                                dialog->mError,
                                QMessageBox::Ok,
                                qApp->activeWindow());
            message.exec();
            return;
        }

    }
    delete dialog;
}

void Project::deleteSelectedPhases()
{
    QJsonObject stateNext = mState;

    QJsonArray phases = mState.value(STATE_PHASES).toArray();
    QJsonArray phases_constraints = mState.value(STATE_PHASES_CONSTRAINTS).toArray();
    QJsonArray events = mState.value(STATE_EVENTS).toArray();

    for (auto i=phases.size()-1; i>=0; --i) {
        const QJsonObject phase = phases.at(i).toObject();
        if (phase.value(STATE_IS_SELECTED).toBool()) {
            const int phase_id = phase.value(STATE_ID).toInt();
            for (qsizetype j=phases_constraints.size()-1; j>=0; --j) {
                QJsonObject constraint = phases_constraints.at(j).toObject();
                const int bwd_id = constraint.value(STATE_CONSTRAINT_BWD_ID).toInt();
                const int fwd_id = constraint.value(STATE_CONSTRAINT_FWD_ID).toInt();

                if (bwd_id == phase_id || fwd_id == phase_id)
                    phases_constraints.removeAt(j);

            }
            for (qsizetype j=0; j<events.size(); ++j) {
                QJsonObject event = events.at(j).toObject();
                QString idsStr = event.value(STATE_EVENT_PHASE_IDS).toString();
                QStringList ids = idsStr.split(",");
                ids.removeAll(QString::number(phase_id));
                event[STATE_EVENT_PHASE_IDS] = ids.join(",");
                events[j] = event;
            }
            phases.removeAt(i);
        }
    }
    stateNext[STATE_PHASES] = phases;
    stateNext[STATE_PHASES_CONSTRAINTS] = phases_constraints;
    stateNext[STATE_EVENTS] = events;

    pushProjectState(stateNext, "Phase(s) deleted", true);

    clear_model();
    MainWindow::getInstance() -> setResultsEnabled(false);
    MainWindow::getInstance() -> setLogEnabled(false);
}

int Project::getUnusedPhaseId(const QJsonArray& phases)
{
    int id = -1;
    bool idIsFree = false;
    while (!idIsFree) {
        ++id;
        idIsFree = true;
        for (qsizetype i = 0; i<phases.size(); ++i) {
            const QJsonObject phase = phases.at(i).toObject();
            if (phase.value(STATE_ID).toInt() == id) {
                idIsFree = false;
                break;
            }
        }
    }
    return id;
}

void Project::mergePhases(int phaseFromId, int phaseToId)
{
    QJsonObject stateNext = mState;
    QJsonArray phases = mState.value(STATE_PHASES).toArray();
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    QJsonArray phases_constraints = mState.value(STATE_PHASES_CONSTRAINTS).toArray();

    // Delete constraints around the disappearing phase
    for (qsizetype j=phases_constraints.size()-1; j>=0; --j) {
        const QJsonObject &constraint = phases_constraints.at(j).toObject();
        const int bwd_id = constraint.value(STATE_CONSTRAINT_BWD_ID).toInt();
        const int fwd_id = constraint.value(STATE_CONSTRAINT_FWD_ID).toInt();
        if ( (bwd_id == phaseFromId) || (fwd_id == phaseFromId))
            phases_constraints.removeAt(j);

    }

    // Change phase id in events
    for (qsizetype j=0; j<events.size(); ++j) {
        const QJsonObject &event = events.at(j).toObject();
        QString idsStr = event.value(STATE_EVENT_PHASE_IDS).toString();
        QStringList ids = idsStr.split(",");
        if (ids.contains(QString::number(phaseFromId))) {
            for (int k=0; k<ids.size(); ++k) {
                if(ids[k] == QString::number(phaseFromId))
                    ids[k] = QString::number(phaseToId);
            }
        }
        event[STATE_EVENT_PHASE_IDS] = ids.join(",");
        events[j] = event;
    }

    // remove disappearing phase
    for (qsizetype i=phases.size()-1; i>=0; --i) {
        const QJsonObject &p = phases.at(i).toObject();
        const int id = p.value(STATE_ID).toInt();
        if (id == phaseFromId) {
            phases.removeAt(i);
            break;
        }
    }

    stateNext[STATE_PHASES] = phases;
    stateNext[STATE_EVENTS] = events;
    stateNext[STATE_PHASES_CONSTRAINTS] = phases_constraints;

    pushProjectState(stateNext, "Phases merged", true);
}



 void Project::updatePhaseEvents(const int phaseId, ActionOnModel action)
{
    QJsonObject stateNext = mState;
    QJsonArray events = mState.value(STATE_EVENTS).toArray();

    for (int i=0; i<events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();

        if (event.value(STATE_IS_SELECTED).toBool()) {
            QString phaseIdsStr = event.value(STATE_EVENT_PHASE_IDS).toString();
            QStringList phaseIds = phaseIdsStr.isEmpty() ? QStringList() : phaseIdsStr.split(",");

            phaseIds.removeAll(QString::number(phaseId));
            if (action == InsertEventsToPhase)
                phaseIds.append(QString::number(phaseId));

            event[STATE_EVENT_PHASE_IDS] = phaseIds.join(",");
            events[i] = event;
        }
    }
    stateNext[STATE_EVENTS] = events;
    pushProjectState(stateNext, "Phase's events updated", true);
}

QJsonObject Project::getPhasesWithId(const int id)
{
    const QJsonArray& phases = mState.value(STATE_PHASES).toArray();
    for (const QJsonValue pha : phases) {
        if (pha.toObject().value(STATE_ID) == id)
            return pha.toObject();
    }
    return QJsonObject();
}

// Events constraints
bool Project::isEventConstraintAllowed(const QJsonObject& eventFrom, const QJsonObject& eventTo)
{
    const QJsonArray &constraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();

    const int eventFromId = eventFrom.value(STATE_ID).toInt();
    const int eventToId = eventTo.value(STATE_ID).toInt();
    bool ConstraintAllowed = true;

    for (const auto cts : constraints) {
        const QJsonObject& constraint = cts.toObject();
        if (constraint.value(STATE_CONSTRAINT_BWD_ID) == eventFromId && constraint.value(STATE_CONSTRAINT_FWD_ID) == eventToId) {
            ConstraintAllowed = false;
            qDebug() << "[Project::isEventConstraintAllowed] not Allowed " ;
            return ConstraintAllowed;

        } else if (constraint.value(STATE_CONSTRAINT_BWD_ID) == eventToId && constraint.value(STATE_CONSTRAINT_FWD_ID) == eventFromId) {
            ConstraintAllowed = false;
            qDebug() << "[Project::isEventConstraintAllowed] not Allowed " ;
            return ConstraintAllowed;
        }
    }
    return ConstraintAllowed;

}

void Project::createEventConstraint(const int eventFromId, const int eventToId)
{
    QJsonObject state = mState;

    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    QJsonArray constraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();

    QJsonObject eventFrom = QJsonObject();
    QJsonObject eventTo = QJsonObject();

    for (int i=0; i<events.size(); ++i)  {
        QJsonObject event = events.at(i).toObject();
        if (event.value(STATE_ID).toInt() == eventFromId)
            eventFrom = event;

        else if (event.value(STATE_ID).toInt() == eventToId)
            eventTo = event;
    }


    qDebug()<<"[Project::createEventConstraint] "<<eventFrom.value(STATE_NAME)<<eventTo.value(STATE_NAME);
    EventConstraint c;
    c.mId = getUnusedEventConstraintId(constraints);
    c.mFromId = eventFrom.value(STATE_ID).toInt();
    c.mToId = eventTo.value(STATE_ID).toInt();
    QJsonObject constraint = c.toJson();
    constraints.append(constraint);
    state[STATE_EVENTS_CONSTRAINTS] = constraints;

    pushProjectState(state, "Event constraint created", true);// notify=true, force=false

}

void Project::deleteEventConstraint(int constraintId)
{
    QJsonObject state = mState;
    QJsonArray constraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();

    for (int i=0; i<constraints.size(); ++i) {
        const QJsonObject &c = constraints.at(i).toObject();
        if (c.value(STATE_ID).toInt() == constraintId) {
            constraints.removeAt(i);
            break;
        }
    }
    state[STATE_EVENTS_CONSTRAINTS] = constraints;
    pushProjectState(state, "Event constraint deleted", true);
}

void Project::updateEventConstraint(int constraintId)
{
    QJsonObject stateNext = mState;
    QJsonArray constraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();
    QJsonObject constraint;
    int index = -1;
    for (int i=0; i<constraints.size(); ++i) {
        const QJsonObject &c = constraints.at(i).toObject();

        if (c.value(STATE_ID).toInt() == constraintId) {
            constraint = c;
            index = i;
        }
    }
    if (index != -1) {
        ConstraintDialog* dialog = new ConstraintDialog(qApp->activeWindow(), ConstraintDialog::eEvent);
        dialog->setConstraint(constraint);
        if (dialog->exec() == QDialog::Accepted) {
            if (dialog->deleteRequested())
                constraints.removeAt(index);
            else {
                constraint = dialog->constraint();
                constraints[index] = constraint;
            }
            stateNext[STATE_EVENTS_CONSTRAINTS] = constraints;
            pushProjectState(stateNext, "Event constraint updated", true);
        }
        delete dialog;
    }
}

int Project::getUnusedEventConstraintId(const QJsonArray& constraints)
{
    int id = -1;
    bool idIsFree = false;
    while (!idIsFree) {
        ++id;
        idIsFree = true;
        for (const auto constraint : constraints) {
            if (constraint.toObject().value(STATE_ID).toInt() == id) {
                idIsFree = false;
                break;
            }
        }
    }
    return id;
}

// Phases constraints
bool Project::isPhaseConstraintAllowed(const QJsonObject& phaseFrom, const QJsonObject& phaseTo)
{
    if(!phaseFrom.isEmpty() && !phaseTo.isEmpty())
        return true;

    return false;
}

void Project::createPhaseConstraint(int phaseFromId, int phaseToId)
{
    QJsonObject state = mState;

    QJsonArray phases = mState.value(STATE_PHASES).toArray();
    QJsonArray constraints = mState.value(STATE_PHASES_CONSTRAINTS).toArray();

    QJsonObject phaseFrom = QJsonObject();
    QJsonObject phaseTo = QJsonObject();

    for (int i=0; i<phases.size(); ++i)  {
        QJsonObject phase = phases.at(i).toObject();
        if (phase.value(STATE_ID).toInt() == phaseFromId)
            phaseFrom = phase;

        else if (phase.value(STATE_ID).toInt() == phaseToId)
            phaseTo = phase;
    }

    if (isPhaseConstraintAllowed(phaseFrom, phaseTo)) {
        PhaseConstraint c;
        c.mId = getUnusedPhaseConstraintId(constraints);
        c.mFromId = phaseFrom.value(STATE_ID).toInt();
        c.mToId = phaseTo.value(STATE_ID).toInt();
        QJsonObject constraint = c.toJson();
        constraints.append(constraint);
        state[STATE_PHASES_CONSTRAINTS] = constraints;

        pushProjectState(state, "Phase constraint created", true);

    }
}

void Project::deletePhaseConstraint(int constraintId)
{
    QJsonObject state = mState;
    QJsonArray constraints = mState.value(STATE_PHASES_CONSTRAINTS).toArray();

    for (int i=0; i<constraints.size(); ++i) {
        QJsonObject c = constraints.at(i).toObject();
        if (c.value(STATE_ID).toInt() == constraintId) {
            constraints.removeAt(i);
            break;
        }
    }
    state[STATE_PHASES_CONSTRAINTS] = constraints;
    pushProjectState(state, "Phase constraint deleted", true);
}

void Project::updatePhaseConstraint(const int constraintId)
{
    QJsonObject state = mState;
    QJsonArray constraints = mState.value(STATE_PHASES_CONSTRAINTS).toArray();
    QJsonObject constraint;
    int index = -1;
    for (int i=0; i<constraints.size(); ++i) {
        QJsonObject c = constraints.at(i).toObject();
        if (c.value(STATE_ID).toInt() == constraintId) {
            constraint = c;
            index = i;
        }
    }
    if (index != -1) {
        ConstraintDialog dialog(qApp->activeWindow(), ConstraintDialog::ePhase);
        dialog.setConstraint(constraint);
        if (dialog.exec() == QDialog::Accepted) {
            if (dialog.deleteRequested())
                constraints.removeAt(index);
            else {
                constraint = dialog.constraint();
                //qDebug() << constraint;
                if (constraint.value(STATE_CONSTRAINT_GAMMA_TYPE).toInt() == PhaseConstraint::eGammaFixed &&
                   constraint.value(STATE_CONSTRAINT_GAMMA_FIXED).toDouble() == 0.)
                {
                    QMessageBox message(QMessageBox::Critical,
                                        tr("Invalid value"),
                                        tr("The fixed value must be positive!"),
                                        QMessageBox::Ok,
                                        qApp->activeWindow());
                    message.exec();
                    return;
                }
                else if (constraint.value(STATE_CONSTRAINT_GAMMA_TYPE).toInt() == PhaseConstraint::eGammaRange &&
                        constraint.value(STATE_CONSTRAINT_GAMMA_MIN).toDouble() >= constraint.value(STATE_CONSTRAINT_GAMMA_MAX).toDouble())
                {
                    QMessageBox message(QMessageBox::Critical,
                                        tr("Invalid values"),
                                        tr("Min must be lower than max!"),
                                        QMessageBox::Ok,
                                        qApp->activeWindow());
                    message.exec();
                    return;
                }

                constraints[index] = constraint;
            }
            state[STATE_PHASES_CONSTRAINTS] = constraints;
            pushProjectState(state, "Phase constraint updated", true);
        }
    }
}

int Project::getUnusedPhaseConstraintId(const QJsonArray &constraints)
{
    int id = -1;
    bool idIsFree = false;
    while (!idIsFree) {
        ++id;
        idIsFree = true;
        //for (int i = 0; i<constraints.size(); ++i) {
        //    QJsonObject constraint = constraints.at(i).toObject();
        for (const auto constraint : constraints) {
            if (constraint.toObject().value(STATE_ID).toInt() == id) {
                idIsFree = false;
                break;
            }
        }
    }
    return id;
}




// --------------------------------------------------------------------
//     Project Run
// --------------------------------------------------------------------
void Project::run()
{
    if (isCurve()) {
        runCurve();

    } else {
        runChronomodel();
    }
}

void Project::runChronomodel()
{
    // Check if project contains invalid dates, e.g. with no computable calibration curve
    const QJsonArray invalidDates = getInvalidDates();
    if (invalidDates.size() > 0) {

        QMessageBox messageBox;
        messageBox.setMinimumWidth(10 * AppSettings::widthUnit());
        messageBox.setIcon(QMessageBox::Warning);
        //http://doc.qt.io/qt-5/qmessagebox.html#setWindowTitle
        //Sets the title of the message box to title. On OS X, the window title is ignored (as required by the OS X Guidelines).

        messageBox.setWindowTitle(tr("Risk on computation"));
        messageBox.setText(tr("The model contains at least one date whose calibration is not digitally computable. \r\rDo you really want to continue ?"));
        QAbstractButton *stopButton = messageBox.addButton(tr("Stop, check the data"), QMessageBox::NoRole);
        messageBox.addButton(tr("I agree to continue"), QMessageBox::YesRole);

        messageBox.exec();
        if (messageBox.clickedButton() == stopButton)
          return;
    }

    // Save the project before running MCMC :

    if (AppSettings::mAutoSave)
        save();
    else
       askToSave(tr("Save current project as..."));


    // This is the occasion to clean EVERYTHING using the previous model before deleting it!
    // e.g. : clean the result view with any graphs, ...

    if (mModel) {
        mModel->clearTraces();
        mModel->clear();
        mModel->shrink_to_fit();
        mModel.reset();
        mModel = std::make_shared<ModelCurve>(mState);

    } else
        mModel = std::make_shared<ModelCurve>(mState);

    bool modelOk = false;
    try {
        // Check if model structure is valid
        modelOk = mModel->isValid();
    } catch(QString error) {
        QMessageBox message(QMessageBox::Warning,
                            tr("Your model is not valid"),
                            error,
                            QMessageBox::Ok,
                            qApp->activeWindow());
        message.exec();
    }
    if (modelOk) {
        emit mcmcStarted();


        MCMCLoopChrono loop(mModel);

        MCMCProgressDialog dialog(&loop, qApp->activeWindow(), Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint);

        /* --------------------------------------------------------------------
          The dialog startMCMC() method starts the loop and the dialog.
          When the loop emits "finished", the dialog will be "accepted"
          This "finished" signal can be the results of :
          - MCMC ran all the way to the end => mAbortedReason is empty and "run" returns.
          - User clicked "Cancel" :
              - an interruption request is sent to the loop
              - The loop catches the interruption request with "isInterruptionRequested"
              - the loop "run" function returns after setting mAbortedReason = ABORTED_BY_USER
          - An error occured :
              - The loop sets mAbortedReason to the correct error message
              - The run function returns
          => THE DIALOG IS NEVER REJECTED ! (Escape key also disabled to prevent default behavior)
         -------------------------------------------------------------------- */

        if (dialog.startMCMC() == QDialog::Accepted) {

            if (loop.mAbortedReason.isEmpty()) {
                //Memo of the init variable state to show in Log view
                //mModel->mLogInit = loop.getChainsLog() + loop.getInitLog();
                AppSettings::mIsSaved = false;
                emit mcmcFinished();


            } else {
                if (loop.mAbortedReason != ABORTED_BY_USER) {
                    QMessageBox message(QMessageBox::Warning,
                                        tr("Error"),
                                        loop.mAbortedReason,
                                        QMessageBox::Ok,
                                        qApp->activeWindow());
                    message.exec();
                }
                clear_model();
            }
        }
        // Dialog is never "rejected", so this should never happen :
        else {
            qDebug() << "[Project::runChronomodel] ERROR : MCMCProgressDialog::rejected : Should NEVER happen !";
            clear_model();
        }
    }
}


void Project::clear_and_shrink_model()
{
    if (mModel && mModel->mNumberOfEvents > 0) {
        mModel->clear_and_shrink();
    }

    emit noResult();
}

void Project::clear_model()
{
    if (mModel && mModel->mNumberOfEvents > 0) {
        mModel->clear();

    }

    emit noResult();
}

bool Project::isCurve() const {
    const QJsonObject &CurveSettings = mState.value(STATE_CURVE).toObject();
    return (!CurveSettings.isEmpty() && CurveSettings.value(STATE_CURVE_PROCESS_TYPE).toInt() != CurveSettings::eProcess_None);

}

void Project::runCurve()
{
    // ------------------------------------------------------------------------------------------
    //  Check if project contains invalid dates, e.g. with no computable calibration curve
    // ------------------------------------------------------------------------------------------
    const QJsonArray invalidDates = getInvalidDates();
    if (invalidDates.size() > 0) {
        QMessageBox messageBox;
        messageBox.setMinimumWidth(10 * AppSettings::widthUnit());
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.setWindowTitle(tr("Your data seems to be invalid"));
        messageBox.setText(tr("The model contains at least one date whose calibration is not digitally computable. \r\rDo you really want to continue ?"));      
        QAbstractButton *stopButton = messageBox.addButton(tr("Cancel"), QMessageBox::NoRole);
        messageBox.addButton(tr("I agree to continue"), QMessageBox::YesRole);
        messageBox.exec();
        
        if (messageBox.clickedButton() == stopButton) {
            return;
        }
    }

    // ------------------------------------------------------------------------------------------
    // Save the project before running MCMC :
    // ------------------------------------------------------------------------------------------
    if (AppSettings::mAutoSave)
        save();
    else
       askToSave(tr("Save current project as..."));


    // ------------------------------------------------------------------------------------------
    //  This signal can be used to clean the previous calculations (result view, ...)
    // ------------------------------------------------------------------------------------------
    emit mcmcStarted();

    // ------------------------------------------------------------------------------------------
    //  Clear current model and recreate et Curve Model
    //  using the project state
    // ------------------------------------------------------------------------------------------
    //clearModel();
    mModel.reset(new ModelCurve(mState));

    // ------------------------------------------------------------------------------------------
    //  Check if the model is valid
    // ------------------------------------------------------------------------------------------
    bool modelOk = false;
    try {
        modelOk = mModel->isValid();
    } catch (QString error) {
        QMessageBox message (QMessageBox::Warning,
            tr("Your model is not valid"),
            error,
            QMessageBox::Ok,
            qApp->activeWindow());
        message.exec();
    }
    if (!modelOk) {
        return;
    }
    
    // ------------------------------------------------------------------------------------------
    //  Start MCMC for Curve
    // ------------------------------------------------------------------------------------------

    MCMCLoopCurve loop (mModel);
    MCMCProgressDialog dialog (&loop, qApp->activeWindow(), Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint);

    /* --------------------------------------------------------------------
      The dialog startMCMC() method starts the loop and the dialog.
      When the loop emits "finished", the dialog will be "accepted"
      This "finished" signal can be the results of :
      - MCMC ran all the way to the end => mAbortedReason is empty and "run" returns.
      - User clicked "Cancel" :
          - an interruption request is sent to the loop
          - The loop catches the interruption request with "isInterruptionRequested"
          - the loop "run" function returns after setting mAbortedReason = ABORTED_BY_USER
      - An error occured :
          - The loop sets mAbortedReason to the correct error message
          - The run function returns
      => THE DIALOG IS NEVER REJECTED ! (Escape key also disabled to prevent default behavior)
     -------------------------------------------------------------------- */
    if (dialog.startMCMC() == QDialog::Accepted) {
        if (loop.mAbortedReason.isEmpty()) {
            //Memo of the init variable state to show in Log view
            //mModel->mLogInit = loop.getChainsLog() + loop.getInitLog();
            AppSettings::mIsSaved = false;
            dialog.setFinishedState();
            emit mcmcFinished();

        } else {
            if (loop.mAbortedReason != ABORTED_BY_USER) {
                QMessageBox message(QMessageBox::Warning,
                    tr("Error"),
                    loop.mAbortedReason,
                    QMessageBox::Ok,
                    qApp->activeWindow());
                message.exec();
            }
            clear_model();
        }
    }
    // Dialog is never "rejected", so this should never happen :
    else {
        qDebug() << "[Project::runCurve] ERROR : MCMCProgressDialog::rejected : Should NEVER happen !";
        clear_model();
    }
}
