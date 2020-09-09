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

#include "Project.h"
#include "MainWindow.h"
#include "Model.h"
#include "PluginManager.h"
#include "MCMCSettingsDialog.h"

#include "Event.h"
#include "EventKnown.h"
#include "EventDialog.h"
#include "EventConstraint.h"
#include "ConstraintDialog.h"

#include "Phase.h"
#include "PhaseConstraint.h"
#include "PhaseDialog.h"

#include "DateDialog.h"
#include "Date.h"
#include "../PluginAbstract.h"
#include "../PluginFormAbstract.h"
#include "TrashDialog.h"
#include "ImportDataView.h"

#include "ModelUtilities.h"
#include "QtUtilities.h"

#include "MCMCLoopMain.h"
#include "MCMCProgressDialog.h"

#include "SetProjectState.h"
#include "StateEvent.h"

#include <iostream>
//#include <exception>
#include <QtWidgets>
#include <QThread>
#include <QJsonObject>
#include <QFile>


Project::Project():
mName(tr("ChronoModel Project")),
//mProjectFileDir(""),
//mProjectFileName(QObject::tr("Untitled")),
mDesignIsChanged(true),
mStructureIsChanged(true),
mItemsIsMoved(true),
mNoResults(true)
{
    mState = emptyState();
    mLastSavedState = mState;

    mAutoSaveTimer = new QTimer(this);
    connect(mAutoSaveTimer, &QTimer::timeout, this, &Project::save);

    if (AppSettings::mAutoSave) {
        mAutoSaveTimer->setInterval(AppSettings::mAutoSaveDelay * 1000);
        mAutoSaveTimer->start();
    } else
        mAutoSaveTimer->stop();

    //
    mModel = new Model();
    mModel->setProject(this);

    mReasonChangeStructure<< PROJECT_LOADED_REASON << PROJECT_SETTINGS_UPDATED_REASON << INSERT_PROJECT_REASON;
    mReasonChangeStructure<<"Event constraint deleted"<<"Event constraint created"<<"Event(s) deleted";
    mReasonChangeStructure<<"Event created"<<"Bound created"<<"Event method updated" ;
    mReasonChangeStructure<<"Update selected event method";//<<"Event selected";

    mReasonChangeStructure<<"Date created"<<"Date moved to event"<<"Date updated"<<"Phase created"<<"Phase(s) deleted";
    mReasonChangeStructure<<"Dates splitted"<< "Dates combined"<<"Update selected data method";
    mReasonChangeStructure<<"Phase updated"<<"Phase constraint created"<<"Phase constraint updated"<<"Phase's events updated";
    mReasonChangeStructure<<"Phase selected";
    mReasonChangeStructure.squeeze();

    mReasonChangeDesign<<"Phase color updated"<<"Phase name updated";
    mReasonChangeDesign<<"Event color updated"<<"Event name updated";
    mReasonChangeDesign<<"Date name updates"<<"Date color updated";
    mReasonChangeDesign.squeeze();

    mReasonChangePosition<<"item moved";
    mReasonChangePosition.squeeze();


}

Project::~Project()
{
    mAutoSaveTimer->stop();
    disconnect(mAutoSaveTimer, &QTimer::timeout, this, &Project::save);
    MainWindow::getInstance()->getUndoStack()->clear();
    mState = QJsonObject();
    mLastSavedState = QJsonObject();
    delete mModel;
    mModel = nullptr;
}

void Project::initState(const QString& reason)
{
    (void) reason;
    const QJsonObject state = emptyState();

    // Do no call pushProjectState here because we don't want to store this state in the UndoStack
    // This is called when closing a project or openning a new one,
    // so the undoStack has just been cleared and we want to keep it empty at project start!
    mState = state;
}

QJsonObject Project::emptyState()
{
    QJsonObject state;

    state[STATE_APP_VERSION] = qApp->applicationVersion();

    ProjectSettings projectSettings;
    QJsonObject settings = projectSettings.toJson();
    state[STATE_SETTINGS] = settings;

    MCMCSettings mcmcSettings;
    mcmcSettings.restoreDefault();
    QJsonObject mcmc = mcmcSettings.toJson();

    state[STATE_MCMC] = mcmc;

    QJsonArray events;
    state[STATE_EVENTS] = events;

    QJsonArray phases;
    state[STATE_PHASES] = phases;

    QJsonArray events_constraints;
    state[STATE_EVENTS_CONSTRAINTS] = events_constraints;

    QJsonArray phases_constraints;
    state[STATE_PHASES_CONSTRAINTS] = phases_constraints;

    QJsonArray dates_trash;
    state[STATE_DATES_TRASH] = dates_trash;

    QJsonArray events_trash;
    state[STATE_EVENTS_TRASH] = events_trash;

    return state;
}

QJsonObject Project::state() const
{
    return mState;
}

QJsonObject* Project::state_ptr()
{
    return &mState;
}
/**
 * @brief Project::pushProjectState used to store action in Undo-Redo Command
 * @param state
 * @param reason
 * @param notify
 * @param force
 * @return
 */
bool Project::pushProjectState(const QJsonObject& state, const QString& reason, bool notify, bool force)
{
    mStructureIsChanged = false;
    mDesignIsChanged = false;
    mItemsIsMoved = false;
    qDebug()<<"Project::pushProjectState "<<reason<<notify<<force;
    if (mReasonChangeStructure.contains(reason))
        mStructureIsChanged = true;

    else  if (mReasonChangeDesign.contains(reason))
        mDesignIsChanged = true;

    else if (mReasonChangePosition.contains(reason))
        mItemsIsMoved = true;

    else
        this->checkStateModification(state, mState);


    if (mStructureIsChanged && (reason != PROJECT_LOADED_REASON) )
        emit projectStructureChanged(true); // connected to MainWindows::noResults

    else if (mDesignIsChanged)
        emit projectDesignChanged(true);


    if (mState != state || force)  {

        SetProjectState* command = new SetProjectState(this, mState, state, reason, notify);
        //command->setText("pushProjectState " + command->actionText());
        if (reason != NEW_PROJECT_REASON && reason != PROJECT_LOADED_REASON)
            MainWindow::getInstance()->getUndoStack()->push(command);

        updateState(state, reason, notify);
        return true;
    }
    return false;
}


void Project::sendUpdateState(const QJsonObject& state, const QString& reason, bool notify)
{
#ifdef DEBUG
    qDebug()<<"Project::sendUpdateState "<<reason<<notify;
    //-----------------
    //-----------------
#endif
    StateEvent* event = new StateEvent(state, reason, notify);
    QCoreApplication::postEvent(this, event, Qt::HighEventPriority);//Qt::NormalEventPriority);
}

void Project::checkStateModification(const QJsonObject& stateNew,const QJsonObject& stateOld)
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
        const double tminPeriodNew = stateNew.value(STATE_SETTINGS_TMIN).toDouble();
        const double tmaxPeriodNew = stateNew.value(STATE_SETTINGS_TMAX).toDouble();
        const double stepPeriodNew = stateNew.value(STATE_SETTINGS_STEP).toDouble();

        const double tminPeriodOld = stateOld.value(STATE_SETTINGS_TMIN).toDouble();
        const double tmaxPeriodOld = stateOld.value(STATE_SETTINGS_TMAX).toDouble();
        const double stepPeriodOld = stateOld.value(STATE_SETTINGS_STEP).toDouble();
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
               const QColor newPhaseColor = QColor(phaseNew.at(i).toObject().value(STATE_COLOR_RED).toInt(),phaseNew.at(i).toObject().value(STATE_COLOR_GREEN).toInt(),phaseNew.at(i).toObject().value(STATE_COLOR_BLUE ).toInt());
               const QColor oldPhaseColor = QColor(phaseOld.at(i).toObject().value(STATE_COLOR_RED).toInt(),phaseOld.at(i).toObject().value(STATE_COLOR_GREEN).toInt(),phaseOld.at(i).toObject().value(STATE_COLOR_BLUE ).toInt());
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
        const QJsonArray phasesConstNew = stateNew.value(STATE_PHASES_CONSTRAINTS).toArray();
        const QJsonArray phasesConstOld = stateOld.value(STATE_PHASES_CONSTRAINTS).toArray();

        if ( phasesConstNew.size()!=phasesConstOld.size()) {
            mStructureIsChanged = true;
            return;
        } else if (phasesConstNew != phasesConstOld) {
                mStructureIsChanged = true;
                return;
        }


         // Check Event and date modification
        const QJsonArray eventsNew = stateNew.value(STATE_EVENTS).toArray();
        const QJsonArray eventsOld = stateOld.value(STATE_EVENTS).toArray();
        if ( eventsNew.size()!=eventsOld.size()) {
            mDesignIsChanged = true;
            mStructureIsChanged = true;
            return;
        } else {
            for (int i = 0; i<eventsNew.size(); ++i){
                // Check DESIGN
               // Check name of Event
               if (eventsNew.at(i).toObject().value(STATE_NAME) != eventsOld.at(i).toObject().value(STATE_NAME))
                   mDesignIsChanged = true;

               if ( eventsNew.at(i).toObject().value(STATE_ITEM_X) != eventsOld.at(i).toObject().value(STATE_ITEM_X) ||
                    eventsNew.at(i).toObject().value(STATE_ITEM_Y) != eventsOld.at(i).toObject().value(STATE_ITEM_Y) ) {
                   mItemsIsMoved = true;
               }
               // Check color of Event
               QColor newEventsColor=QColor(eventsNew.at(i).toObject().value(STATE_COLOR_RED).toInt(),eventsNew.at(i).toObject().value(STATE_COLOR_GREEN).toInt(),eventsNew.at(i).toObject().value(STATE_COLOR_BLUE ).toInt());
               QColor oldEventsColor=QColor(eventsOld.at(i).toObject().value(STATE_COLOR_RED).toInt(),eventsOld.at(i).toObject().value(STATE_COLOR_GREEN).toInt(),eventsOld.at(i).toObject().value(STATE_COLOR_BLUE ).toInt());
               if (newEventsColor != oldEventsColor)
                   mDesignIsChanged = true;

               // Check EVENTS STRUCTURE
               if ( eventsNew.at(i).toObject().value(STATE_EVENT_TYPE) != eventsOld.at(i).toObject().value(STATE_EVENT_TYPE) ||
                   eventsNew.at(i).toObject().value(STATE_EVENT_METHOD) != eventsOld.at(i).toObject().value(STATE_EVENT_METHOD) ||
                   eventsNew.at(i).toObject().value(STATE_EVENT_PHASE_IDS) != eventsOld.at(i).toObject().value(STATE_EVENT_PHASE_IDS) ||

                   eventsNew.at(i).toObject().value(STATE_EVENT_KNOWN_FIXED) != eventsOld.at(i).toObject().value(STATE_EVENT_KNOWN_FIXED) )

               {
                   mStructureIsChanged = true;
                   return;
               }

               // Check dates inside Event
               QJsonArray datesNew = eventsNew.at(i).toObject().value(STATE_EVENT_DATES).toArray();
               QJsonArray datesOld = eventsOld.at(i).toObject().value(STATE_EVENT_DATES).toArray();
               if ( datesNew.size() != datesOld.size()) {
                   mStructureIsChanged = true;
                   return;
               } else {
                   for (int j = 0; j<datesNew.size(); ++j) {
                        // Check name of Date
                        if (datesNew.at(j).toObject().value(STATE_NAME) != datesOld.at(j).toObject().value(STATE_NAME))
                            mDesignIsChanged = true;

                        // No color in date JSON
                        // Check DATES STRUCTURE
                        if ( datesNew.at(j).toObject().value(STATE_DATE_DATA) != datesOld.at(j).toObject().value(STATE_DATE_DATA) ||
                            datesNew.at(j).toObject().value(STATE_DATE_PLUGIN_ID) != datesOld.at(j).toObject().value(STATE_DATE_PLUGIN_ID) ||
                            datesNew.at(j).toObject().value(STATE_DATE_VALID) != datesOld.at(j).toObject().value(STATE_DATE_VALID) ||
                            datesNew.at(j).toObject().value(STATE_DATE_DELTA_TYPE).toInt() != datesOld.at(j).toObject().value(STATE_DATE_DELTA_TYPE).toInt() ||
                            datesNew.at(j).toObject().value(STATE_DATE_DELTA_FIXED).toDouble() != datesOld.at(j).toObject().value(STATE_DATE_DELTA_FIXED).toDouble() ||
                            datesNew.at(j).toObject().value(STATE_DATE_DELTA_MIN).toDouble() != datesOld.at(j).toObject().value(STATE_DATE_DELTA_MIN).toDouble() ||
                            datesNew.at(j).toObject().value(STATE_DATE_DELTA_MAX).toDouble() != datesOld.at(j).toObject().value(STATE_DATE_DELTA_MAX).toDouble() ||
                            datesNew.at(j).toObject().value(STATE_DATE_DELTA_AVERAGE).toDouble() != datesOld.at(j).toObject().value(STATE_DATE_DELTA_AVERAGE).toDouble() ||
                            datesNew.at(j).toObject().value(STATE_DATE_DELTA_ERROR).toDouble() != datesOld.at(j).toObject().value(STATE_DATE_DELTA_ERROR).toDouble() ||
                            datesNew.at(j).toObject().value(STATE_DATE_SUB_DATES) != datesOld.at(j).toObject().value(STATE_DATE_SUB_DATES) ) {

                            mStructureIsChanged = true;
                            return;
                        }
                     }
                }

            }

        }
        // Check events  constraintes modification
        const QJsonArray eventsConstNew = stateNew.value(STATE_EVENTS_CONSTRAINTS).toArray();
        const QJsonArray eventsConstOld = stateOld.value(STATE_EVENTS_CONSTRAINTS).toArray();

        if ( eventsConstNew.size() != eventsConstOld.size()) {
            mStructureIsChanged = true;
            return;
        } else if (eventsConstNew != eventsConstOld) {
                mStructureIsChanged = true;
                return;
        }
    }

}

void Project::unselectedAllInState()
{
    QJsonArray phases = mState.value(STATE_PHASES).toArray();
    for (QJsonArray::iterator iPhase = phases.begin(); iPhase != phases.end(); ++iPhase) {
        QJsonObject p = iPhase->toObject();
        p[STATE_IS_CURRENT] = false;
        p[STATE_IS_SELECTED] = false;
        *iPhase = p;
    }
    mState[STATE_PHASES] = phases;

    QJsonArray events = mState.value(STATE_EVENTS).toArray();
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
    mState[STATE_EVENTS] = events;
qDebug()<<"Project::unselectedAllInState end";

}

bool Project::structureIsChanged()
{
 return mStructureIsChanged;
}

bool Project::designIsChanged()
{
    return mDesignIsChanged;
}

bool Project::event(QEvent* e)
{
    if (e->type() == QEvent::User) {
        StateEvent* se = static_cast<StateEvent*>(e);
        if (se)
            updateState(se->state(), se->reason(), se->notify());

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

void Project::updateState(const QJsonObject& state, const QString& reason, bool notify)
{
    qDebug() << " ---  Receiving : " << reason;
    mState = state;
    if (notify) {

         if (reason == NEW_PROJECT_REASON)
            showStudyPeriodWarning();

        emit projectStateChanged();
    }
}

void Project::sendEventsSelectionChanged()
{
#ifdef DEBUG
    //qDebug() << "(+++) Sending events selection : use marked events";
#endif
    QEvent* e = new QEvent(QEvent::Type (1001));
    QCoreApplication::postEvent(this, e, Qt::NormalEventPriority);
}

// Project File Management

bool Project::load(const QString& path)
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

    qDebug() << "in Project::load Loading project file : " << path;

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
            if (mModel)
                mModel->clear();

            QJsonObject loadingState = jsonDoc.object();

            if (loadingState.contains(STATE_APP_VERSION)) {
                QString projectVersionStr = loadingState.value(STATE_APP_VERSION).toString();
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

                    if (olderProject) {
                        QMessageBox message(QMessageBox::Warning,
                                            tr("Project version doesn't match"),
                                            "This project has been saved with a older version of ChronoModel :\r\r- Project version : " + projectVersionStr + "\r- Current version : " + appVersionStr + "\r\rSome incompatible data may be missing and you may encounter problems running the model.\r\rLoading the project will update and overwrite the existing file. Do you really want to continue ?",
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
           qDebug() << "in Project::load  begin checkDatesCompatibility";
           bool isCorrected;
           loadingState = checkDatesCompatibility(loadingState, isCorrected);

           qDebug() << "in Project::load  end checkDatesCompatibility";
            //  Check if dates are valid on the current study period
            mState = checkValidDates(loadingState);

            recenterProject();
            // When openning a project, it is maked as saved : mState == mLastSavedState
            mLastSavedState = mState;

            qDebug() << "in Project::load  unselectedAllInState";
            unselectedAllInState(); // modify mState

            // If a version update is to be done :
            QJsonObject state = mState;
            state[STATE_APP_VERSION] = qApp->applicationVersion();

            // -------------------- look for the calibration file --------------------
          //  if (newerProject || olderProject)
           //     return true;

            QString caliPath = path + ".cal";
            QFileInfo calfi(caliPath);

            if (calfi.isFile() && !isCorrected) {
                // load Calibration Curve
                QFile calFile(caliPath);
                if (calFile.open(QIODevice::ReadOnly)) {

                    if (calFile.exists()) {
                        qDebug() << "Project::load Loading model file.cal : " << calFile.fileName() << " size =" << calFile.size();
                        QDataStream in(&calFile);

                        int QDataStreamVersion;
                        in >> QDataStreamVersion;
                        in.setVersion(QDataStreamVersion);
                        if (in.version()!= QDataStream::Qt_5_5)
                                return false;

                        QString caliVersion;
                        in >> caliVersion;
                        // prepare the future
                         mCalibCurves.clear();
                        if (caliVersion != qApp->applicationVersion()) {
                            QFileInfo fileInfo(calFile);
                            QString filename(fileInfo.fileName());
                            QString strMessage = tr("%1 has been done with a different version = %2").arg(filename, caliVersion) + " \n (" + tr("current") + " =  "
                                    + qApp->applicationVersion() + ") \n"+ tr("Do you really want to load the calibration file: *.chr.cal ?");
                            QString strTitle = tr("Compatibility risk");
                            QMessageBox message(QMessageBox::Question, strTitle, strMessage, QMessageBox::Yes | QMessageBox::No, qApp->activeWindow());
                            if (message.exec() == QMessageBox::Yes) {
                                // loading cal curve
                                try {
                                    mCalibCurves = QMap<QString, CalibrationCurve>();
                                    quint32 siz;
                                    in >> siz;
                                    for (int i = 0; i < int(siz); ++i) {
                                        QString descript;
                                        in >> descript;
                                        CalibrationCurve cal;
                                        in >> cal;
                                        mCalibCurves.insert(descript,cal);
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
                                  // return true;
                                }

                            } /*else {
                               // return true;
                            }*/

                        } else {
                            // loading cal curve
                            mCalibCurves = QMap<QString, CalibrationCurve>();
                            quint32 siz;
                            in >> siz;
                            for (int i = 0; i < int(siz); ++i) {
                                QString descript;
                                in >> descript;
                                CalibrationCurve cal;
                                in >> cal;
                                mCalibCurves.insert(descript,cal);
                            }
                        }

                     }
                 }
                calFile.close();
            } else {
                setNoResults(true);
                clearModel();
                return true;
            }

            clearModel();

            /* -------------------- Load results -------------------- */
            QString dataPath = path + ".res";

            QFile dataFile;

            dataFile.setFileName(dataPath);

            QFileInfo fi(dataFile);
            dataFile.open(QIODevice::ReadOnly);
            if (fi.isFile() && !mCalibCurves.isEmpty()) {
                if (dataFile.exists()) {

                    qDebug() << "Project::load Loading model file.res : " << dataPath << " size=" << dataFile.size();

                    try {
                        mModel->fromJson(mState);
                     }
                    catch (const std::exception & e) {
                        QMessageBox message(QMessageBox::Warning,
                                            tr("Error loading project"),
                                            tr("The project could not be loaded.") + "\r" +
                                            tr("Error : %1").arg(e.what()),
                                            QMessageBox::Ok,
                                            qApp->activeWindow());
                        message.exec();

                        clearModel();
                    }


                    try {
                        mModel->setProject(this);
                        mModel->restoreFromFile(dataPath);
                        setNoResults(false);
                        emit mcmcFinished(mModel);

                    } catch (const std::exception & e) {
                        QMessageBox message(QMessageBox::Critical,
                                            tr("Error loading project MCMC results"),
                                            tr("The project MCMC results could not be loaded.") + "\r" +
                                            tr("Error : %1").arg(e.what()),
                                            QMessageBox::Ok,
                                            qApp->activeWindow());
                        setNoResults(true);
                        message.exec();
                    }
                } else {
                    setNoResults(true);
                    clearModel();
                }
            } else {
                setNoResults(true);
                clearModel();
            }
            // --------------------

            return true;
        }
    }
    return false;
}

bool Project::save()
{
    QFileInfo info(AppSettings::mLastDir + "/" + AppSettings::mLastFile);
    return info.exists() ? saveProjectToFile() : saveAs(tr("Save current project as..."));
}


bool Project::insert(const QString& path)
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

    qDebug() << "in Project::load Loading project file : " << path;

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
                                            "This project has been saved with a older version of ChronoModel :\r\r- Project version : " + projectVersionStr + "\r- Current version : " + appVersionStr + "\r\rSome incompatible data may be missing and you may encounter problems running the model.\r\rLoading the project will update and overwrite the existing file. Do you really want to continue ?",
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
           qDebug() << "in Project::load  begin checkDatesCompatibility";
           bool isCorrected;
           loadedState = checkDatesCompatibility(loadedState, isCorrected);

            qDebug() << "in Project::load  end checkDatesCompatibility";
            //  Check if dates are valid on the current study period
            loadedState = checkValidDates(loadedState);


            // 1 - find min-max position and min max index in the mState yet in memory
           double minXEvent (HUGE_VAL), maxXEvent(- HUGE_VAL), minYEvent(HUGE_VAL), maxYEvent(- HUGE_VAL);
           double minXPhase(HUGE_VAL), maxXPhase(- HUGE_VAL), minYPhase(HUGE_VAL), maxYPhase(- HUGE_VAL);
           int  maxIDEvent(0), maxIDPhase(0), maxIDEventConstraint(0), maxIDPhaseConstraint(0);

           const QJsonArray phases = mState.value(STATE_PHASES).toArray();

           if (phases.isEmpty()) {
               minXPhase = 0;
               maxXPhase = 0;
               minYPhase = 0;
               maxYPhase = 0;

           } else {
               for (auto phaseJSON : phases) {
                   QJsonObject phase = phaseJSON.toObject();
                   minXPhase =std::min(minXPhase, phase[STATE_ITEM_X].toDouble());
                   maxXPhase =std::max(maxXPhase, phase[STATE_ITEM_X].toDouble());

                   minYPhase =std::min(minYPhase, phase[STATE_ITEM_Y].toDouble());
                   maxYPhase =std::max(maxYPhase, phase[STATE_ITEM_Y].toDouble());
                   maxIDPhase =std::max(maxIDPhase, phase[STATE_ID].toInt());
               }
               maxIDPhase += 1;

               const QJsonArray phaseConstraints = mState.value(STATE_PHASES_CONSTRAINTS).toArray();
               for (auto phaseConsJSON : phaseConstraints) {
                   QJsonObject phaseCons = phaseConsJSON.toObject();
                   maxIDPhaseConstraint =std::max(maxIDPhaseConstraint, phaseCons[STATE_ID].toInt());
               }
               maxIDPhaseConstraint += 1;

           }

           const QJsonArray events = mState.value(STATE_EVENTS).toArray();
           if (events.isEmpty()) {
               minXEvent = 0;
               maxXEvent = 0;
               minYEvent = 0;
               maxYEvent = 0;

           } else {
               for (auto eventJSON : events) {
                   QJsonObject event = eventJSON.toObject();
                   minXEvent =std::min(minXEvent, event[STATE_ITEM_X].toDouble());
                   maxXEvent =std::max(maxXEvent, event[STATE_ITEM_X].toDouble());

                   minYEvent =std::min(minYEvent, event[STATE_ITEM_Y].toDouble());
                   maxYEvent =std::max(maxYEvent, event[STATE_ITEM_Y].toDouble());
                   maxIDEvent =std::max(maxIDEvent, event[STATE_ID].toInt());
               }
               maxIDEvent += 1;

               const QJsonArray eventConstraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();
               for (auto eventConsJSON : eventConstraints) {
                   QJsonObject eventCons = eventConsJSON.toObject();
                   maxIDEventConstraint =std::max(maxIDEventConstraint, eventCons[STATE_ID].toInt());
               }
               maxIDEventConstraint += 1;
            }

           // 2- find min X and  min -max Y in the imported project
           double minXEventNew (HUGE_VAL);
           double minYEventNew (HUGE_VAL), maxYEventNew (-HUGE_VAL);
           const QJsonArray eventsNew = loadedState.value(STATE_EVENTS).toArray();
           if (eventsNew.isEmpty()) {
               minXEventNew = 0;

               minYEventNew = 0;
               maxYEventNew = 0;

           } else {
               for (auto eventJSON : eventsNew) {
                   QJsonObject event = eventJSON.toObject();
                   minXEventNew =std::min(minXEventNew, event[STATE_ITEM_X].toDouble());

                   minYEventNew =std::min(minYEventNew, event[STATE_ITEM_Y].toDouble());
                   maxYEventNew =std::max(maxYEventNew, event[STATE_ITEM_Y].toDouble());
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
               for (auto phaseJSON : phasesNew) {
                   QJsonObject phase = phaseJSON.toObject();
                   minXPhaseNew =std::min(minXPhaseNew, phase[STATE_ITEM_X].toDouble());

                   minYPhaseNew =std::min(minYPhaseNew, phase[STATE_ITEM_Y].toDouble());
                   maxYPhaseNew =std::max(maxYPhaseNew, phase[STATE_ITEM_Y].toDouble());

               }
            }
          // 3 - Shift the new loadedState

           QJsonObject newState = loadedState;
           QJsonArray newPhases = newState.value(STATE_PHASES).toArray();
           for (auto phaseJSON : newPhases) {
               QJsonObject phase = phaseJSON.toObject();
               // set on the right+ + mItemWidth(150.) in AbstractItem.h
               phase[STATE_ITEM_X] =  phase[STATE_ITEM_X].toDouble() + maxXPhase - minXPhaseNew + 200;
               // center on Y
               phase[STATE_ITEM_Y] = phase[STATE_ITEM_Y].toDouble() - (maxYPhaseNew + minYPhaseNew)/2.;
               phase[STATE_ID] = phase[STATE_ID].toInt() + maxIDPhase;

               phaseJSON = phase;
           }
           newState[STATE_PHASES] = newPhases;

           QJsonArray newPhaseConstraints = newState.value(STATE_PHASES_CONSTRAINTS).toArray();
           for (auto phaseConsJSON : newPhaseConstraints) {
               QJsonObject phaseCons = phaseConsJSON.toObject();
               phaseCons[STATE_ID] = phaseCons[STATE_ID].toInt() + maxIDPhaseConstraint;
               phaseCons[STATE_CONSTRAINT_FWD_ID] = phaseCons[STATE_CONSTRAINT_FWD_ID].toInt() + maxIDPhase;
               phaseCons[STATE_CONSTRAINT_BWD_ID] = phaseCons[STATE_CONSTRAINT_BWD_ID].toInt() + maxIDPhase;

               phaseConsJSON = phaseCons;
           }
           newState[STATE_PHASES_CONSTRAINTS] = newPhaseConstraints;

           QJsonArray newEvents = newState.value(STATE_EVENTS).toArray();
           for (auto eventJSON : newEvents) {
              QJsonObject event = eventJSON.toObject();
              // set on the right + mItemWidth(150.) in AbstractItem.h
              event[STATE_ITEM_X] = event[STATE_ITEM_X].toDouble() + maxXEvent - minXEventNew + 200;
              // center on Y
              event[STATE_ITEM_Y] = event[STATE_ITEM_Y].toDouble() - (maxYEventNew + minYEventNew)/2. ;
              event[STATE_ID] = event[STATE_ID].toInt() + maxIDEvent;

              QList<int> mPhasesIds  = stringListToIntList(event.value(STATE_EVENT_PHASE_IDS).toString(), ",");
              for ( int i(0); i<mPhasesIds.size(); ++i)
                  mPhasesIds[i] += maxIDPhase;

              event[STATE_EVENT_PHASE_IDS] = intListToString( mPhasesIds, ",");

              eventJSON = event;
           }
           newState[STATE_EVENTS] = newEvents;

           QJsonArray newEventConstraints = newState.value(STATE_EVENTS_CONSTRAINTS).toArray();
           for (auto eventConsJSON : newEventConstraints) {
               QJsonObject eventCons = eventConsJSON.toObject();
               eventCons[STATE_ID] = eventCons[STATE_ID].toInt() + maxIDEventConstraint;

               eventCons[STATE_CONSTRAINT_FWD_ID] = eventCons[STATE_CONSTRAINT_FWD_ID].toInt() + maxIDEvent;
               eventCons[STATE_CONSTRAINT_BWD_ID] = eventCons[STATE_CONSTRAINT_BWD_ID].toInt() + maxIDEvent;

              eventConsJSON = eventCons;
           }

           newState[STATE_EVENTS_CONSTRAINTS] = newEventConstraints;

           // 4 - Adding the new loadedState after mState
           QJsonObject stateNext = mState;

           QJsonArray nextPhases = mState.value(STATE_PHASES).toArray();
           for (auto phaseJSON : newPhases)
               nextPhases.append(phaseJSON.toObject());
           stateNext[STATE_PHASES] = nextPhases;

           QJsonArray nextPhaseConstraints = mState.value(STATE_PHASES_CONSTRAINTS).toArray();
           for (auto phaseConsJSON : newPhaseConstraints)
               nextPhaseConstraints.append(phaseConsJSON.toObject());
           stateNext[STATE_PHASES_CONSTRAINTS] = nextPhaseConstraints;

           QJsonArray nextEvents = mState.value(STATE_EVENTS).toArray();
           for (auto eventJSON : newEvents)
               nextEvents.append(eventJSON.toObject());
           stateNext[STATE_EVENTS] = nextEvents;

           QJsonArray nextEventConstraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();
           for (auto eventConsJSON : newEventConstraints)
               nextEventConstraints.append(eventConsJSON.toObject());
           stateNext[STATE_EVENTS_CONSTRAINTS] = nextEventConstraints;

           clearModel();
           pushProjectState(stateNext, INSERT_PROJECT_REASON, true, true);

           qDebug() << "in Project::insert  unselectedAllInState";
           unselectedAllInState(); // modify mState
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

        // We need to reset mLastSavedState because it corresponds
        // to the last saved state in the previous file.
        mLastSavedState = QJsonObject();

        return saveProjectToFile();
    }
    return false;
}

bool Project::askToSave(const QString& saveDialogTitle)
{
    // Check if modifs have been made
    (void) saveDialogTitle;
    /*if( mState == mLastSavedState)
        return true;
    */
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
     if (mLastSavedState != mState) {
        QString path = AppSettings::mLastDir + "/" + AppSettings::mLastFile;
        QFile file(path);
        if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
#ifdef DEBUG
            qDebug() << "Project::saveProjectToFile() Project saved to : " << path;
#endif
            // reset version number
            mState[STATE_APP_VERSION] = QApplication::applicationVersion();

            mLastSavedState = mState;

            QJsonDocument jsonDoc(mState);
            file.write(jsonDoc.toJson(QJsonDocument::Indented));
            file.resize(file.pos());
            file.close();
        } else
            return false;

    } else {
#ifdef DEBUG
        //qDebug() << "Nothing new to save in project model";
#endif
    }

    QFile file(AppSettings::mLastDir + "/" + AppSettings::mLastFile + ".cal");
    if (file.open(QIODevice::WriteOnly)) {

        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_5_5);
        out << out.version();

        out << qApp->applicationVersion();
        // save Calibration Curve
        out << quint32 (mCalibCurves.size());
        for (QMap<QString, CalibrationCurve>::const_iterator it = mCalibCurves.cbegin() ; it != mCalibCurves.cend(); ++it) {
            out << it.key();
            out << it.value();
        }
        file.close();
     }

    QFileInfo checkFile(AppSettings::mLastDir + "/" + AppSettings::mLastFile + ".res");
    if (checkFile.exists() && checkFile.isFile())
        QFile(AppSettings::mLastDir + "/" + AppSettings::mLastFile + ".res").remove();

    if (!mNoResults && !mModel->mChains.isEmpty()) {
        qDebug() << "Project::saveProjectToFile() Saving project results in "<<AppSettings::mLastDir + "/" + AppSettings::mLastFile + ".res";
        mModel->setProject(this);
        mModel->saveToFile(AppSettings::mLastDir + "/" + AppSettings::mLastFile + ".res");
    }
    return true;
}

bool Project::recenterProject()
{
    double minXEvent (HUGE_VAL), maxXEvent(- HUGE_VAL), minYEvent(HUGE_VAL), maxYEvent(- HUGE_VAL);
    double minXPhase(HUGE_VAL), maxXPhase(- HUGE_VAL), minYPhase(HUGE_VAL), maxYPhase(- HUGE_VAL);

    const QJsonArray phases = mState.value(STATE_PHASES).toArray();
    for (auto phaseJSON : phases) {
        QJsonObject phase = phaseJSON.toObject();
        minXPhase =std::min(minXPhase, phase[STATE_ITEM_X].toDouble());
        maxXPhase =std::max(maxXPhase, phase[STATE_ITEM_X].toDouble());

        minYPhase =std::min(minYPhase, phase[STATE_ITEM_Y].toDouble());
        maxYPhase =std::max(maxYPhase, phase[STATE_ITEM_Y].toDouble());
    }

    const QJsonArray events = mState.value(STATE_EVENTS).toArray();
    for (auto eventJSON : events) {
        QJsonObject event = eventJSON.toObject();
        minXEvent =std::min(minXEvent, event[STATE_ITEM_X].toDouble());
        maxXEvent =std::max(maxXEvent, event[STATE_ITEM_X].toDouble());

        minYEvent =std::min(minYEvent, event[STATE_ITEM_Y].toDouble());
        maxYEvent =std::max(maxYEvent, event[STATE_ITEM_Y].toDouble());
    }


    QJsonObject newState = mState;
    QJsonArray newPhases = newState.value(STATE_PHASES).toArray();
    for (auto phaseJSON : newPhases) {
        QJsonObject phase = phaseJSON.toObject();
        phase[STATE_ITEM_X] =  phase[STATE_ITEM_X].toDouble() - (maxXPhase + minXPhase)/2.;
        phase[STATE_ITEM_Y] = phase[STATE_ITEM_Y].toDouble() - (maxYPhase + minYPhase)/2.;
        phaseJSON = phase;
    }
    newState[STATE_PHASES] = newPhases;

    QJsonArray newEvents = newState.value(STATE_EVENTS).toArray();
    for (auto eventJSON : newEvents) {
       QJsonObject event = eventJSON.toObject();
       event[STATE_ITEM_X] = event[STATE_ITEM_X].toDouble() - (maxXEvent + minXEvent)/2. ;
       event[STATE_ITEM_Y] = event[STATE_ITEM_Y].toDouble() - (maxYEvent + minYEvent)/2. ;
       eventJSON = event;
    }
    newState[STATE_EVENTS] = newEvents;
    mState = newState;

    return true;
}

/* --------------------------------------------------------------------
     Project Settings
 --------------------------------------------------------------------
        Settings   */
bool Project::setSettings(const ProjectSettings& settings)
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
        return pushProjectState(stateNext, PROJECT_SETTINGS_UPDATED_REASON, true, true);
    }
}

void Project::setAppSettings()
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
    MCMCSettingsDialog dialog(qApp->activeWindow());
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
 * @brief Project::resetMCMC Restore default samplinf methods on each ti and theta
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
            event[STATE_EVENT_METHOD] = int (Event::eDoubleExp);

            QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
            for (int j = 0; j < dates.size(); ++j) {
                QJsonObject date = dates.at(j).toObject();

                try {
                    Date d;
                    d.fromJson(date);
                    if (!d.isNull()) {
                        date[STATE_DATE_METHOD] = int (d.mPlugin->getDataMethod());
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
    int id = -1;
    bool idIsFree = false;
    while (!idIsFree) {
        ++id;
        idIsFree = true;
        for (int i = 0; i<events.size(); ++i) {
            QJsonObject event = events.at(i).toObject();
            if (event.value(STATE_ID).toInt() == id)
                idIsFree = false;
        }
    }
    return id;
}

void Project::createEvent(qreal x, qreal y)
{
    if (studyPeriodIsValid()) {
        EventDialog* dialog = new EventDialog(qApp->activeWindow(), tr("New Event"));
        if (dialog->exec() == QDialog::Accepted) {
            Event event = Event();
            event.mName = dialog->getName();
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
            EventKnown eventKnown = EventKnown();
            eventKnown.mName = dialog->getName();
            eventKnown.mColor= dialog->getColor();
            QJsonObject eventJSON (eventKnown.toJson());
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

    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    QJsonArray events_constraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();
    QJsonArray events_trash = mState.value(STATE_EVENTS_TRASH).toArray();

    for (int i = events.size()-1; i >= 0; --i) {
        QJsonObject event = events.at(i).toObject();

        if (event.value(STATE_IS_SELECTED).toBool()) {
            const int event_id = event.value(STATE_ID).toInt();
            qDebug()<<"Project::deleteSelectedEvents : "<<event.value(STATE_NAME);
            for (int j = events_constraints.size()-1; j >= 0; --j) {
                QJsonObject constraint = events_constraints.at(j).toObject();
                const int bwd_id = constraint.value(STATE_CONSTRAINT_BWD_ID).toInt();
                const int fwd_id = constraint.value(STATE_CONSTRAINT_FWD_ID).toInt();

                if (bwd_id == event_id || fwd_id == event_id)
                    events_constraints.removeAt(j);

            }
            events.removeAt(i);
            events_trash.append(event);
        }
    }
    stateNext[STATE_EVENTS] = events;
    stateNext[STATE_EVENTS_CONSTRAINTS] = events_constraints;
    stateNext[STATE_EVENTS_TRASH] = events_trash;

    pushProjectState(stateNext, "Event(s) deleted", true);

    // send to clear the propertiesView
    const QJsonObject itemEmpty ;
    emit currentEventChanged(itemEmpty); // connect to EventPropertiesView::setEvent


    clearModel();
    MainWindow::getInstance() -> setResultsEnabled(false);
    MainWindow::getInstance() -> setLogEnabled(false);
}

void Project::deleteSelectedTrashedEvents(const QList<int>& ids)
{
    QJsonObject stateNext = mState;

    QJsonArray events_trash = mState.value(STATE_EVENTS_TRASH).toArray();

    for (int i = events_trash.size()-1; i >= 0; --i) {
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
        qDebug() << indexes;

        QJsonObject stateNext = mState;
        QJsonArray events = mState.value(STATE_EVENTS).toArray();
        QJsonArray events_trash = mState.value(STATE_EVENTS_TRASH).toArray();

        QJsonObject settingsJson = stateNext[STATE_SETTINGS].toObject();
        ProjectSettings settings = ProjectSettings::fromJson(settingsJson);

        for (int i = indexes.size()-1; i >= 0; --i) {
            QJsonObject event = events_trash.takeAt(indexes[i]).toObject();
            event[STATE_ID] = getUnusedEventId(events);

            QJsonArray dates= event[STATE_EVENT_DATES].toArray();
            for (int j = dates.size()-1; j >= 0; --j) {
                QJsonObject date = dates.takeAt(j).toObject();
                // Validate the date before adding it to the correct event and pushing the state
                PluginAbstract* plugin = PluginManager::getPluginFromId(date[STATE_DATE_PLUGIN_ID].toString());
                bool valid = plugin->isDateValid(date[STATE_DATE_DATA].toObject(), settings);
                date[STATE_DATE_VALID] = valid;

                date[STATE_ID] = getUnusedDateId(dates);
                dates.append(date);
            }
            event[STATE_EVENT_DATES] = dates;
            events.append(event);

        }
        stateNext[STATE_EVENTS] = events;
        stateNext[STATE_EVENTS_TRASH] = events_trash;

        pushProjectState(stateNext, "Event(s) restored", true);
    }
}

void Project::updateEvent(const QJsonObject& event, const QString& reason)
{
    QJsonObject stateNext = mState;
    QJsonArray events = mState.value(STATE_EVENTS).toArray();

    for (QJsonArray::iterator i = events.begin(); i != events.end(); ++i) {
        if ( i->toObject().value(STATE_ID).toInt() == event.value(STATE_ID).toInt()) {
            *i = event;
            break;
        }
    }
    stateNext[STATE_EVENTS] = events;
    pushProjectState(stateNext, reason, true);
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
    int fromIndex = -1;
    int toIndex = -1;

    for (int i = events.size()-1; i >= 0; --i) {
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
    for (int i=0; i<datesFrom.size(); ++i) {
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
    for (int i = constraints.size()-1; i >= 0; --i) {
        QJsonObject c = constraints.at(i).toObject();
        int fromId = c.value(STATE_CONSTRAINT_BWD_ID).toInt();
        int toId = c.value(STATE_CONSTRAINT_FWD_ID).toInt();
        if (eventFromId == fromId || eventFromId == toId)
            constraints.removeAt(i);

    }
    stateNext[STATE_EVENTS_CONSTRAINTS] = constraints;

    //qDebug() << events;

    pushProjectState(stateNext, "Events merged", true);
}

// Grouped actions on events
void Project::selectedEventsFromSelectedPhases()
{

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
}

void Project::updateSelectedEventsColor(const QColor& color)
{

    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    for (int i = 0; i < events.size(); ++i) {
        QJsonObject evt = events.at(i).toObject();
        if (evt.value(STATE_IS_SELECTED).toBool()) {
            evt[STATE_COLOR_RED] = color.red();
            evt[STATE_COLOR_GREEN] = color.green();
            evt[STATE_COLOR_BLUE] = color.blue();
            events[i] = evt;
        }
    }

    QJsonObject stateNext = mState;
    stateNext[STATE_EVENTS] = events;
    pushProjectState(stateNext, "Update selected events color", true);
}

void Project::updateSelectedEventsMethod(Event::Method method)
{
    QJsonObject stateNext = mState;
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    for (int i = 0; i<events.size(); ++i) {
        QJsonObject evt = events.at(i).toObject();
        if (evt.value(STATE_IS_SELECTED).toBool()) {
            evt[STATE_EVENT_METHOD] = method;
            events[i] = evt;
        }
    }
    stateNext[STATE_EVENTS] = events;
    pushProjectState(stateNext, "Update selected events method", true);
}

void Project::updateSelectedEventsDataMethod(Date::DataMethod method, const QString& pluginId)
{
    QJsonObject stateNext = mState;
    QJsonArray events = mState.value(STATE_EVENTS).toArray();
    for (int i = 0; i<events.size(); ++i) {
        QJsonObject evt = events[i].toObject();
        if (evt.value(STATE_IS_SELECTED).toBool()) {
            QJsonArray dates = evt[STATE_EVENT_DATES].toArray();
            for (int j = 0; j<dates.size(); ++j) {
                QJsonObject date = dates[j].toObject();
                if (date[STATE_DATE_PLUGIN_ID].toString() == pluginId) {
                    date[STATE_DATE_METHOD] = method;
                    dates[j] = date;
                }
            }
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
int Project::getUnusedDateId(const QJsonArray& dates) const
{
    int id = -1;
    bool idIsFree = false;
    while (!idIsFree) {
        ++id;
        idIsFree = true;
        for (int i = 0; i < dates.size(); ++i) {
            QJsonObject date = dates.at(i).toObject();
            if (date.value(STATE_ID).toInt() == id)
                idIsFree = false;
        }
    }
    return id;
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

                date.mName = dialog.getName();
                date.mMethod = dialog.getMethod();
                date.mDeltaType = dialog.getDeltaType();
                date.mDeltaFixed = dialog.getDeltaFixed();
                date.mDeltaMin = dialog.getDeltaMin();
                date.mDeltaMax = dialog.getDeltaMax();
                date.mDeltaAverage = dialog.getDeltaAverage();
                date.mDeltaError = dialog.getDeltaError();
            } else {
                QMessageBox message(QMessageBox::Critical,
                                    tr("Invalid data"),
                                    form->mError,
                                    QMessageBox::Ok,
                                    qApp->activeWindow());
                message.exec();
            }
        }
    }
    return date;
}

void Project::addDate(int eventId, QJsonObject date)
{
    QJsonObject stateNext = mState;

    // Validate the date before adding it to the correct event and pushing the state
    QJsonObject settingsJson = stateNext[STATE_SETTINGS].toObject();
    ProjectSettings settings = ProjectSettings::fromJson(settingsJson);
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
    QJsonArray phases = state.value(STATE_PHASES).toArray();
    for (int i = 0; i<events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
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

            if (date[STATE_DATE_PLUGIN_ID].toString() == "typo_ref." || date[STATE_DATE_PLUGIN_ID].toString() == "typo") {
                date[STATE_DATE_PLUGIN_ID] = QString("unif"); // since version 2.0.14
                isCorrected = true;
            }

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
                subdates[k] = subdate;
            }
            date[STATE_DATE_SUB_DATES] = subdates;
            // -----------------------------------------------------------

            dates[j] = date;
            event[STATE_EVENT_DATES] = dates;
            events[i] = event;
            state[STATE_EVENTS] = events;
        }
    }
    // conversion since version 1.4 test
    bool phaseConversion = false;
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

    return state;
}

/**
 * @brief Project::updateDate : Function to change the date parameter inside an Event
 * Open the dialogBox
 * @param eventId
 * @param dateIndex
 */
void Project::updateDate(int eventId, int dateIndex)
{
    QJsonObject state = mState;

    QJsonObject settingsJson = state.value(STATE_SETTINGS).toObject();
    ProjectSettings settings = ProjectSettings::fromJson(settingsJson);

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
                        date[STATE_DATE_METHOD] = dialog.getMethod();

                        date[STATE_DATE_DELTA_TYPE] = dialog.getDeltaType();
                        date[STATE_DATE_DELTA_FIXED] = dialog.getDeltaFixed();
                        date[STATE_DATE_DELTA_MIN] = dialog.getDeltaMin();
                        date[STATE_DATE_DELTA_MAX] = dialog.getDeltaMax();
                        date[STATE_DATE_DELTA_AVERAGE] = dialog.getDeltaAverage();
                        date[STATE_DATE_DELTA_ERROR] = dialog.getDeltaError();

                        PluginAbstract* plugin = PluginManager::getPluginFromId(date.value(STATE_DATE_PLUGIN_ID).toString());
                        bool valid = plugin->isDateValid(date.value(STATE_DATE_DATA).toObject(), settings);
                        date[STATE_DATE_VALID] = valid;

                        dates[dateIndex] = date;
                        event[STATE_EVENT_DATES] = dates;
                        events[i] = event;
                        state[STATE_EVENTS] = events;

                        pushProjectState(state, "Date from dialog", true);

                    } else {
                        QMessageBox message(QMessageBox::Critical,
                                            tr("Invalid data"),
                                            form->mError,
                                            QMessageBox::Ok,
                                            qApp->activeWindow());
                        message.exec();
                    }
                }
            }
            break;
        }
    }
}

void Project::deleteDates(int eventId, const QList<int>& dateIndexes)
{
    QJsonObject state = mState;

    QJsonArray events = state.value(STATE_EVENTS).toArray();
    for (int i = 0; i < events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
        if (event.value(STATE_ID).toInt() == eventId) {
            QJsonArray dates_trash = state.value(STATE_DATES_TRASH).toArray();
            QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();

            for (int j = dates.size()-1; j >= 0; --j) {
                if (dateIndexes.contains(j)) {
                    QJsonObject date = dates.takeAt(j).toObject();
                    dates_trash.append(date);
                }
            }
            event[STATE_EVENT_DATES] = dates;
            events[i] = event;
            state[STATE_EVENTS] = events;
            state[STATE_DATES_TRASH] = dates_trash;

            pushProjectState(state, QString::number(dateIndexes.size()) + " date(s) deleted", true);

            break;
        }
    }
    clearModel();
    MainWindow::getInstance() -> setResultsEnabled(false);
    MainWindow::getInstance() -> setLogEnabled(false);
}

void Project::deleteSelectedTrashedDates(const QList<int>& ids)
{
    QJsonObject stateNext = mState;

    QJsonArray dates_trash = mState.value(STATE_DATES_TRASH).toArray();

    for (int i = dates_trash.size()-1; i >= 0; --i) {
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
        qDebug() << indexes;

        QJsonObject stateNext = mState;
        QJsonArray events = mState.value(STATE_EVENTS).toArray();
        QJsonArray dates_trash = mState.value(STATE_DATES_TRASH).toArray();

        for (int i = 0; i < events.size(); ++i) {
            QJsonObject event = events.at(i).toObject();
            if (event.value(STATE_ID).toInt() == eventId){
                QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
                for (int i = indexes.size()-1; i >= 0; --i) {
                    QJsonObject date = dates_trash.takeAt(indexes.at(i)).toObject();
                    // Validate the date before adding it to the correct event and pushing the state
                    QJsonObject settingsJson = stateNext[STATE_SETTINGS].toObject();
                    ProjectSettings settings = ProjectSettings::fromJson(settingsJson);
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
    ProjectSettings settings = ProjectSettings::fromJson(settingsJson);

    QJsonArray events = state.value(STATE_EVENTS).toArray();
    for (int i=0; i<events.size(); ++i) {
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
    ProjectSettings settings = ProjectSettings::fromJson(settingsJson);
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
                        for (int j=dates.size()-1; j>=0; --j) {
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
            
            for (int j(0); j<dates.size(); ++j)
                QJsonObject date = dates[j].toObject();
           
            events[i] = event;
            emit currentEventChanged(event);
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
    ProjectSettings settings = ProjectSettings::fromJson(settingsJson);
    QJsonArray events = mState.value(STATE_EVENTS).toArray();

    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events.at(i).toObject();
        if (event.value(STATE_ID).toInt() == eventId) {
            QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
            for (int j=0; j<dates.size(); ++j) {
                QJsonObject date = dates.at(j).toObject();
                if (date.value(STATE_ID).toInt() == dateId) {
                    
                    // We have found the date to split !
                    QJsonArray subdates = date.value(STATE_DATE_SUB_DATES).toArray();
                    PluginAbstract* plugin = PluginManager::getPluginFromId(date.value(STATE_DATE_PLUGIN_ID).toString());

                    for (int k=0; k<subdates.size(); ++k) {
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

    for (int i=0; i<events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
        if ((event.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) &&  // Not a bound
           (event.value(STATE_IS_SELECTED).toBool())) // Event is selected
        {
            QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
            for (int j=0; j<dates.size(); ++j) {
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

void Project::createPhase(qreal x, qreal y)
{
    if (studyPeriodIsValid()) {
        PhaseDialog* dialog = new PhaseDialog(qApp->activeWindow());
        int r (dialog->exec());
        if (r == int(QDialog::Accepted) ) {
            if (dialog->isValid()) {
                QJsonObject phase = dialog->getPhase();
                QJsonObject stateNext = mState;
                QJsonArray phases = stateNext.value(STATE_PHASES).toArray();

                phase[STATE_ID] = getUnusedPhaseId(phases);

                //QJsonObject eventJSON (event.toJson());
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
            delete dialog;
        }
    }
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

    for (int i=phases.size()-1; i>=0; --i) {
        const QJsonObject phase = phases.at(i).toObject();
        if (phase.value(STATE_IS_SELECTED).toBool()) {
            const int phase_id = phase.value(STATE_ID).toInt();
            for (int j=phases_constraints.size()-1; j>=0; --j) {
                QJsonObject constraint = phases_constraints.at(j).toObject();
                const int bwd_id = constraint.value(STATE_CONSTRAINT_BWD_ID).toInt();
                const int fwd_id = constraint.value(STATE_CONSTRAINT_FWD_ID).toInt();

                if (bwd_id == phase_id || fwd_id == phase_id)
                    phases_constraints.removeAt(j);

            }
            for (int j=0; j<events.size(); ++j) {
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

    clearModel();
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
        for (int i=0; i<phases.size(); ++i) {
            const QJsonObject phase = phases.at(i).toObject();
            if (phase.value(STATE_ID).toInt() == id)
                idIsFree = false;
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
    for (int j=phases_constraints.size()-1; j>=0; --j) {
        QJsonObject constraint = phases_constraints.at(j).toObject();
        const int bwd_id = constraint.value(STATE_CONSTRAINT_BWD_ID).toInt();
        const int fwd_id = constraint.value(STATE_CONSTRAINT_FWD_ID).toInt();
        if ( (bwd_id == phaseFromId) || (fwd_id == phaseFromId))
            phases_constraints.removeAt(j);

    }

    // Change phase id in events
    for (int j=0; j<events.size(); ++j) {
        QJsonObject event = events.at(j).toObject();
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
    for (int i=phases.size()-1; i>=0; --i) {
        const QJsonObject p = phases.at(i).toObject();
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
    const QJsonArray phases = mState.value(STATE_PHASES).toArray();
    foreach (const QJsonValue pha, phases) {
        if (pha.toObject().value(STATE_ID) == id)
            return pha.toObject();
    }
    return QJsonObject();
}

// Events constraints
bool Project::isEventConstraintAllowed(const QJsonObject& eventFrom, const QJsonObject& eventTo)
{
    const QJsonArray constraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();

    const int eventFromId = eventFrom.value(STATE_ID).toInt();
    const int eventToId = eventTo.value(STATE_ID).toInt();
    bool ConstraintAllowed = true;
    for (int i=0; i<constraints.size(); ++i) {
        const QJsonObject constraint = constraints.at(i).toObject();
        if (constraint.value(STATE_CONSTRAINT_BWD_ID) == eventFromId && constraint.value(STATE_CONSTRAINT_FWD_ID) == eventToId) {
            ConstraintAllowed = false;
            qDebug() << "Project::isEventConstraintAllowed: not Allowed " ;
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


    qDebug()<<"Project::createEventConstraint "<<eventFrom.value(STATE_NAME)<<eventTo.value(STATE_NAME);
    EventConstraint c;
    c.mId = getUnusedEventConstraintId(constraints);
    c.mFromId = eventFrom.value(STATE_ID).toInt();
    c.mToId = eventTo.value(STATE_ID).toInt();
    QJsonObject constraint = c.toJson();
    constraints.append(constraint);
    state[STATE_EVENTS_CONSTRAINTS] = constraints;

    pushProjectState(state, "Event constraint created", true, false);// notify=true, force=false

}

void Project::deleteEventConstraint(int constraintId)
{
    QJsonObject state = mState;
    QJsonArray constraints = mState.value(STATE_EVENTS_CONSTRAINTS).toArray();

    for (int i=0; i<constraints.size(); ++i) {
        QJsonObject c = constraints.at(i).toObject();
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
        QJsonObject c = constraints.at(i).toObject();

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
        for (int i=0; i<constraints.size(); ++i) {
            QJsonObject constraint = constraints.at(i).toObject();
            if (constraint.value(STATE_ID).toInt() == id)
                idIsFree = false;
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

        pushProjectState(state, "Phase constraint created", true, false);

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
                qDebug() << constraint;
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

int Project::getUnusedPhaseConstraintId(const QJsonArray& constraints)
{
    int id = -1;
    bool idIsFree = false;
    while (!idIsFree) {
        ++id;
        idIsFree = true;
        for (int i=0; i<constraints.size(); ++i) {
            QJsonObject constraint = constraints.at(i).toObject();
            if (constraint.value(STATE_ID).toInt() == id)
                idIsFree = false;
        }
    }
    return id;
}


// -------
// Export
void Project::exportAsText()
{
    /*QString currentDir = MainWindow::getInstance()->getCurrentPath();

    QString path = QFileDialog::getExistingDirectory(qApp->activeWindow(), tr("Location"), currentDir);
    if(!path.isEmpty())
    {
        MainWindow::getInstance()->setCurrentPath(path);

        QString folderBaseName = mName.simplified().toLower().replace(" ", "_");
        QString folderName = folderBaseName;
        int index = 1;
        while(QFileInfo::exists(path + "/" + folderName))
        {
            folderName = folderBaseName + " (" + QString::number(index) + ")";
            ++index;
        }
        QDir dir(path);
        if(dir.mkdir(folderName))
        {
            dir = QDir(path + "/" + folderName);

            QString descFileName = "description.txt";
            QFile descFile(dir.absoluteFilePath(descFileName));
            if(descFile.open(QIODevice::ReadWrite | QIODevice::Text))
            {
                QTextStream stream(&descFile);

                stream << "Project name : " << mName << endl;

                stream << endl << "****************************************" << endl;
                stream << "EVENTS" << endl;
                stream << "****************************************" << endl;

                for(int i=0; i<mEvents.size(); ++i)
                {
                    Event* event = mEvents[i];
                    stream << endl << "++++++++++++++++++++++++++++++++++++++++" << endl;
                    stream << "EVENT : " << endl;
                    stream << "- Name : " << event->mName << endl;
                    stream << "- Method : " << ModelUtilities::getEventMethodText(event->mMethod) << endl;
                    stream << "- Num data : " << QString::number(event->mDates.size()) << endl;
                    stream << "- Num phases : " << QString::number(event->mPhases.size()) << endl;
                    stream << "++++++++++++++++++++++++++++++++++++++++" << endl;

                    for(int j=0; j<event->mDates.size(); ++j)
                    {
                        Date* date = (Date*) event->mDates[j];
                        stream << "DATA : " << endl;
                        stream << "- Name : " << date->mName << endl;
                        stream << "- Type : " << date->mPlugin->getName() << endl;
                        stream << "---------------" << endl;
                    }
                }

                stream << endl << "****************************************" << endl;
                stream << "PHASES" << endl;
                stream << "****************************************" << endl;

                for(int i=0; i<mPhases.size(); ++i)
                {
                    Phase* phase = mPhases[i];
                    stream << endl << "++++++++++++++++++++++++++++++++++++++++" << endl;
                    stream << "PHASE : " << endl;
                    stream << "- Name : " << phase->mName << endl;
                    stream << "- Num events : " << QString::number(phase->mEvents.size()) << endl;
                    stream << "++++++++++++++++++++++++++++++++++++++++" << endl;
                }
            }
        }
    }*/

    /*QMessageBox message(QMessageBox::Critical, tr("TODO"), tr("create readable textual representation of XML project file"), QMessageBox::Ok, qApp->activeWindow(), Qt::Sheet);
    message.exec();*/
}

// --------------------------------------------------------------------
//     Project Run
// --------------------------------------------------------------------

void Project::run()
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
        QAbstractButton *IStop = messageBox.addButton(tr("Stop, check the data"), QMessageBox::NoRole);
       // messageBox.addButton(tr("I agree to continue"), QMessageBox::YesRole);

        messageBox.exec();
        if (messageBox.clickedButton() == IStop)
          return;


    }

    // Save the project before running MCMC :

    if (AppSettings::mAutoSave)
        save();
    else
       askToSave(tr("Save current project as..."));


    // This is the occasion to clean EVERYTHING using the previous model before deleting it!
    // e.g. : clean the result view with any graphs, ...
    emit mcmcStarted();

    clearModel();

    mModel->fromJson(mState);
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
        MCMCLoopMain loop(mModel, this);
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
                mModel->mLogMCMC = loop.getChainsLog() + loop.getInitLog();
                emit mcmcFinished(mModel);
            } else {
                if (loop.mAbortedReason != ABORTED_BY_USER) {
                    QMessageBox message(QMessageBox::Warning,
                                        tr("Error"),
                                        loop.mAbortedReason,
                                        QMessageBox::Ok,
                                        qApp->activeWindow());
                    message.exec();
                }
                clearModel();
            }
        }
        // Dialog is never "rejected", so this should never happen :
        else {
            qDebug() << "ERROR : MCMCProgressDialog::rejected : Should NEVER happen !";
            clearModel();
        }
    }
}


void Project::clearModel()
{
    if (mModel)
        mModel->clear();

     emit noResult();
}
