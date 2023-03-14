/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

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

#include "ModelView.h"
#include "EventsScene.h"
#include "PhasesScene.h"
#include "PhaseItem.h"
#include "CurveSettingsView.h"
#include "Event.h"
#include "Painting.h"
#include "Button.h"
#include "Label.h"
#include "ImportDataView.h"
#include "EventPropertiesView.h"
#include "PluginAbstract.h"
#include "SceneGlobalView.h"
#include "ScrollCompressor.h"
#include "CalibrationView.h"
#include "MultiCalibrationView.h"
#include "CalibrationCurve.h"
#include "AppSettings.h"

#include "HelpWidget.h"
#include "MainWindow.h"
#include "Project.h"
#include "QtUtilities.h"
#include "StudyPeriodDialog.h"

#include <QtWidgets>
#include <QtSvg>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QRectF>

#include <assert.h>

ModelView::ModelView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
    mEventsScene(nullptr),
    mCurSearchIdx(0),
    mPhasesScene(nullptr),
    mCurrentRightWidget(nullptr),
    mTmin(0.),
    mTmax(2000.),
    mProject(nullptr),
    mSplitProp(0.6),
    mHandlerW ( int (0.25 *AppSettings::widthUnit())),
    mIsSplitting(false),
    mCalibVisible(false),
    mCurveSettingsVisible(false)
{
    setMouseTracking(true);
    //setFont(AppSettings::font());

    mTopRect = QRect(0, 0, width(), int (0.5 * AppSettings::heigthUnit()));
    mTopWrapper = new QWidget(this);
    mTopWrapper->setGeometry(mTopRect);

    mHandlerRect = QRect(int((width()-mHandlerW)*mSplitProp), mTopRect.height(), mHandlerW, height() - mTopRect.height());

    mLeftRect = QRect(0, mTopRect.height(), width() - mHandlerRect.x() + 1, height() - mTopRect.height());
    mLeftHiddenRect = mLeftRect.adjusted(0, 0, mLeftRect.width(), 0);

    mLeftWrapper = new QWidget(this);
    mLeftWrapper->setGeometry(mLeftRect);

    mRightRect = QRect(mLeftRect.width() + mHandlerRect.width(), mTopRect.height(), width() - mLeftRect.width() - mHandlerRect.width() +1, height() - mTopRect.height());
    mRightHiddenRect = mRightRect.adjusted(mRightRect.width(), 0, 0, 0);

    mRightWrapper = new QWidget(this);
    mRightWrapper->setGeometry(mRightRect);

    // ---- Header Top Bar with Study period --------------
    // ----------- on mTopWrapper ------------------

    //mButModifyPeriod = new Button(tr("STUDY PERIOD") , mTopWrapper);
    mButModifyPeriod = new QPushButton(tr("STUDY PERIOD") , mTopWrapper);

  // mButModifyPeriod->setStyleSheet("QPushButton:active {background-color: rgb(230, 230, 230); border: none;}  "); // bug QT 5.15
    //ButCurve = new SwitchWidget(this);
    mButCurve = new Button(tr("Curve Building :"), mTopWrapper);
    mButCurve->setToolTip(tr("Define curve parameters"));
    mButCurve->setFlatHorizontal();
    mButCurve->setCheckable(true);
    mButCurve->setIconOnly(false);

    adaptStudyPeriodButton(mTmin, mTmax);
    //connect(mButModifyPeriod,  static_cast<void (QPushButton::*)(bool)>(&Button::clicked), this, &ModelView::modifyPeriod);
    // connect(mButModifyPeriod,  &Button::clicked, this, &ModelView::modifyPeriod);
    connect(mButModifyPeriod,  &QPushButton::clicked, this, &ModelView::modifyPeriod);

   // connect(mButCurve, static_cast<void (Button::*)(bool)>(&Button::toggled), this, &ModelView::showCurveSettings);
    connect(mButCurve, &Button::toggled, this, &ModelView::showCurveSettings);

    mLeftPanelTitle = new Label(tr("Events Scene"), mTopWrapper);
    mLeftPanelTitle->setLight();
    mLeftPanelTitle->setBackground(Painting::borderDark);

    mRightPanelTitle = new Label(tr("Phases Scene"), mTopWrapper);
    mRightPanelTitle->setLight();
    mRightPanelTitle->setBackground(Painting::borderDark);

    const QFontMetrics fm(font());
    const int radarW = 4 * AppSettings::widthUnit();
    const int radarH = 4 * AppSettings::heigthUnit();
    const int searchH = round(1.3 * fm.height());

   // ---- Windows on the left hand Event scene --------------
    mEventsView = new QGraphicsView(mLeftWrapper);
    mEventsScene = new EventsScene(mEventsView);
    mEventsView->setScene(mEventsScene);
    mEventsView->setAcceptDrops(true);
    mEventsView->setInteractive(true);
    mEventsView->setDragMode(QGraphicsView::RubberBandDrag);

    mEventsOverview = new SceneGlobalView(mEventsScene, mEventsView, mLeftWrapper);
    mEventsOverview->setVisible(false);

    mEventsSearchEdit = new QLineEdit(mLeftWrapper);
    mEventsSearchEdit->setVisible(false);
   // mEventsSearchEdit->setStyleSheet("QLineEdit {background-color: rgb(100, 100, 100); color: white;}");
    mEventsSearchEdit->setPlaceholderText(tr("Search Event or Data..."));


    mEventsSearchEdit->setGeometry(mEventsView->x() + 5, 5, radarW, searchH);
    mEventsOverview->setGeometry(mEventsView->x() + 5, mEventsSearchEdit->y() + mEventsSearchEdit->height(), radarW, radarH);

    mButNewEvent = new Button(tr("New Event"), mLeftWrapper);
    mButNewEvent->setToolTip(tr("Create a new Event"));
    mButNewEvent->setIcon(QIcon(":new_event.png"));
    mButNewEvent->setFlatVertical();

    mButNewEventKnown = new Button(tr("New Bound"), mLeftWrapper);
    mButNewEventKnown->setToolTip(tr("Create a new Bound"));
    mButNewEventKnown->setIcon(QIcon(":new_bound.png"));
    mButNewEventKnown->setFlatVertical();

    mButDeleteEvent = new Button(tr("Delete"), mLeftWrapper);
    mButDeleteEvent->setToolTip(tr("Delete selected Event(s) or Bound(s)"));
    mButDeleteEvent->setIcon(QIcon(":delete.png"));
    mButDeleteEvent->setFlatVertical();

    mButRecycleEvent = new Button(tr("Restore"), mLeftWrapper);
    mButRecycleEvent->setToolTip(tr("Restore deleted item (Event or Bound)"));
    mButRecycleEvent->setIcon(QIcon(":restore.png"));
    mButRecycleEvent->setFlatVertical();

    mButExportEvents = new Button(tr("Save Image"), mLeftWrapper);
    mButExportEvents->setToolTip(tr("Save image of the Events scene as a file"));
    mButExportEvents->setIcon(QIcon(":picture_save.png"));
    mButExportEvents->setFlatVertical();

    mButEventsGlobalView = new Button(tr("Overview"), mLeftWrapper);
    mButEventsGlobalView->setToolTip(tr("Overview on the global Events scene"));
    mButEventsGlobalView->setIcon(QIcon(":eye_w.png"));
    mButEventsGlobalView->setCheckable(true);
    mButEventsGlobalView->setFlatVertical();

    mButEventsGrid = new Button(tr("Grid"), mLeftWrapper);
    mButEventsGrid->setToolTip(tr("Drawing a grid under the events scene to organize"));
    mButEventsGrid->setIcon(QIcon(":grid2.png"));
    mButEventsGrid->setCheckable(true);
    mButEventsGrid->setFlatVertical();

    mEventsGlobalZoom = new ScrollCompressor(mLeftWrapper);
    mEventsGlobalZoom->setProp(0.5);
    mEventsGlobalZoom->showText(tr("Zoom"), true);
    mEventsGlobalZoom->setVertical(true);

    connect(mButEventsGrid, &Button::toggled, mEventsScene, &EventsScene::showGrid);

    mButProperties = new Button(tr("Properties"), mLeftWrapper);
    mButProperties->setToolTip(tr("Show the propeties of the first selected Event"));
    mButProperties->setCheckable(true);
    mButProperties->setIcon(QIcon(":settings_w.png"));
    mButProperties->setChecked(false);
    mButProperties->setDisabled(true);
    mButProperties->setFlatVertical();

    mButMultiCalib = new Button(tr("MultiCalib"), mLeftWrapper);
    mButMultiCalib->setToolTip(tr("Show the calibrated curves of data within selected Events"));
    mButMultiCalib->setCheckable(true);
    mButMultiCalib->setIcon(QIcon(":multiCalib_w.png"));
    mButMultiCalib->setChecked(false);
    mButMultiCalib->setDisabled(true);
    mButMultiCalib->setFlatVertical();

    mButImport = new Button(tr("Data"), mLeftWrapper);
    mButImport->setToolTip(tr("Show the data importation panel from file"));
    mButImport->setCheckable(true);
    mButImport->setIcon(QIcon(":csv_import.png"));
    mButImport->setFlatVertical();

    connect(mButImport, static_cast<void (Button::*)(bool)>(&Button::toggled), this, &ModelView::showImport);

    // -------- Windows Data Importation --------------------

    mImportDataView = new ImportDataView(mRightWrapper);
    connect(mEventsScene, &EventsScene::csvDataLineDropAccepted, mImportDataView, &ImportDataView::removeCsvRows);
    connect(mEventsScene, &EventsScene::csvDataLineDropRejected, mImportDataView, &ImportDataView::errorCsvRows);

    // -------- Curve settings ---------------------------
    mCurveSettingsView = new CurveSettingsView(mRightWrapper);
    connect(mCurveSettingsView, &CurveSettingsView::newProcess, this, &ModelView::updateCurveButton);
    // -------- Windows Phase scene ---------------------------

    mPhasesView = new QGraphicsView(mRightWrapper);
    mPhasesScene = new PhasesScene(mPhasesView);
    mPhasesView->setScene(mPhasesScene);
    mPhasesView->setInteractive(true);
    mPhasesView->setDragMode(QGraphicsView::RubberBandDrag);

    // this signal has already been connected inside the EventsView constructor, to make sure events are marked as selected.
    // Thus, the following connection can make use in updateCheckedPhases of these marks.
    // -> no more used, changed when set inser and extract button in the PhaseItem
    //connect(mEventsScene, &EventsScene::selectionChanged, mPhasesScene, &PhasesScene::updateCheckedPhases);

    mPhasesGlobalView = new SceneGlobalView(mPhasesScene, mPhasesView, mRightWrapper);
    mPhasesGlobalView->setVisible(false);
    mPhasesGlobalView->setGeometry(5, 5, radarW, radarH);

    mButNewPhase = new Button(tr("New Phase"), mRightWrapper);
    mButNewPhase->setToolTip(tr("Create a new Phase"));
    mButNewPhase->setIcon(QIcon(":new_event.png"));
    mButNewPhase->setFlatVertical();

    mButDeletePhase = new Button(tr("Delete Phase"), mRightWrapper);
    mButDeletePhase->setToolTip(tr("Delete selected Phase(s)"));
    mButDeletePhase->setIcon(QIcon(":delete.png"));
    mButDeletePhase->setFlatVertical();

    mButExportPhases = new Button(tr("Save Image"), mRightWrapper);
    mButExportPhases->setToolTip(tr("Save image of the Phases scene as a file"));
    mButExportPhases->setIcon(QIcon(":picture_save.png"));
    mButExportPhases->setFlatVertical();

    mButPhasesGlobaliew = new Button(tr("Overview"), mRightWrapper);
    mButPhasesGlobaliew->setToolTip(tr("Overview on the global Phases scene"));
    mButPhasesGlobaliew->setIcon(QIcon(":eye_w.png"));
    mButPhasesGlobaliew->setCheckable(true);
    mButPhasesGlobaliew->setFlatVertical();

    mButPhasesGrid = new Button(tr("Grid"), mRightWrapper);
    mButEventsGrid->setToolTip(tr("Drawing a grid under the Phases scene to organize"));
    mButPhasesGrid->setIcon(QIcon(":grid2.png"));
    mButPhasesGrid->setCheckable(true);
    mButPhasesGrid->setFlatVertical();

    mPhasesGlobalZoom = new ScrollCompressor(mRightWrapper);
    mPhasesGlobalZoom->setProp(0.5);
    mPhasesGlobalZoom->showText(tr("Zoom"), true);

    connect(mPhasesGlobalZoom, &ScrollCompressor::valueChanged, this, &ModelView::updatePhasesZoom);
    connect(mButExportPhases, &Button::clicked, this, &ModelView::exportPhasesScene);

    connect(mButPhasesGlobaliew, &Button::toggled, mPhasesGlobalView, &SceneGlobalView::setVisible);
    connect(mButPhasesGrid, &Button::toggled, mPhasesScene, &PhasesScene::showGrid);


    // -------- Windows Event propreties -----------------------

    mEventPropertiesView = new EventPropertiesView(mRightWrapper);
    mEventPropertiesView->hide();

    // ------------- Windows calibration ---------------------

    mCalibrationView = new CalibrationView(mLeftWrapper);

     // ------------- Windows Multi-calibration ---------------------
     mMultiCalibrationView = new MultiCalibrationView(mRightWrapper);
     mMultiCalibrationView->hide();
    // -------- Animation -------------------------------

    mAnimationHide = new QPropertyAnimation();
    mAnimationHide->setPropertyName("geometry");
    mAnimationHide->setDuration(300);
    mAnimationHide->setTargetObject(mEventPropertiesView);
    mAnimationHide->setEasingCurve(QEasingCurve::OutCubic);//(QEasingCurve::Linear);

    mAnimationShow = new QPropertyAnimation();
    mAnimationShow->setPropertyName("geometry");
    mAnimationShow->setDuration(300);
    mAnimationShow->setEasingCurve(QEasingCurve::OutCubic);//(QEasingCurve::Linear);

    mAnimationCalib = new QPropertyAnimation();
    mAnimationCalib->setPropertyName("geometry");
    mAnimationCalib->setDuration(400);
    mAnimationCalib->setEasingCurve(QEasingCurve::OutCubic);

    // ---- update and paint with the appSettingsFont

    //toggleCurve(false);
    applyAppSettings();
}

ModelView::~ModelView()
{

}

void ModelView::setProject(Project* project)
{
    assert(project != nullptr);

    const bool projectExist = (mProject ? true : false);
    mProject = project;
    mPhasesScene->setProject(mProject);
    mEventsScene->setProject(mProject);
    mCurveSettingsView->setProject(mProject);

    if (mProject && !projectExist) {
        connectScenes();

    } else if (projectExist && !mProject) {
        disconnectScenes();
    }

    // if there is no phase, we must show all events
    const QJsonArray phases = mProject->state().value(STATE_PHASES).toArray();
    if (phases.size() == 0 )
        mEventsScene->setShowAllThumbs(true);

    showCalibration(false);

    const ProjectSettings settings = ProjectSettings::fromJson(mProject->mState[STATE_SETTINGS].toObject());

    mTmin = settings.mTmin;
    mTmax = settings.mTmax;

    setSettingsValid(settings.mTmin < settings.mTmax);

    //Unselect all Item in all scene
    mProject->unselectedAllInState();

    //mEventsScene->createSceneFromState();
   // mPhasesScene->createSceneFromState();

    mPhasesScene->updateSceneFromState();

    applyAppSettings(); // do phase->update()
}

void ModelView::connectScenes()
{
    connect(mButNewEvent, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &ModelView::createEventInPlace);
    connect(mButNewEventKnown, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &ModelView::createEventKnownInPlace);
   // connect(mButDeleteEvent, &Button::clicked, mProject, &Project::deleteSelectedEvents);
    connect(mButDeleteEvent, static_cast<void (Button::*)(bool)> (&Button::clicked), mEventsScene, &EventsScene::deleteSelectedItems);

    connect(mButRecycleEvent, static_cast<void (Button::*)(bool)> (&Button::clicked), mProject, &Project::recycleEvents);
    connect(mButEventsGlobalView, &Button::toggled, mEventsOverview, &SceneGlobalView::setVisible);
    connect(mButEventsGlobalView, &Button::toggled, mEventsSearchEdit, &QLineEdit::setVisible);
    connect(mEventsSearchEdit, &QLineEdit::returnPressed, this, &ModelView::searchEvent);
    connect(mEventsGlobalZoom, &ScrollCompressor::valueChanged, this, &ModelView::updateEventsZoom);
    connect(mButExportEvents, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &ModelView::exportEventsScene);

    connect(mButProperties, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &ModelView::showProperties);

    //connect(mButNewPhase,  static_cast<void (Button::*)(bool)> (&Button::clicked), mProject, &Project::createPhase);
    connect(mButNewPhase,  static_cast<void (Button::*)(bool)> (&Button::clicked), this, &ModelView::createPhaseInPlace);
    connect(mButDeletePhase,  static_cast<void (Button::*)(bool)> (&Button::clicked), mPhasesScene, &PhasesScene::deleteSelectedItems);
    connect(mButPhasesGlobaliew, &Button::toggled, mPhasesGlobalView, &SceneGlobalView::setVisible);

     connect(mEventsScene, &EventsScene::noSelection, this, &ModelView::noEventSelected);
    connect(mEventsScene, &EventsScene::eventsAreSelected, this, &ModelView::eventsAreSelected);

    connect(mEventsScene, &EventsScene::eventDoubleClicked, this, &ModelView::togglePropeties);

    // When one or several phases are selected, we hightLigth the data inside the Events included in the phases
    connect(mPhasesScene, &PhasesScene::noSelection, mEventsScene, &EventsScene::noHide);
    connect(mPhasesScene, &PhasesScene::phasesAreSelected, mEventsScene, &EventsScene::phasesSelected);

    // Project::currentEventChanged come from Project::deleteSelectedEvents() and EventsScene::updateSceneFromState()
    connect(mProject, &Project::currentEventChanged, mEventPropertiesView, &EventPropertiesView::setEvent);

    // Properties View
    connect(mEventPropertiesView, &EventPropertiesView::combineDatesRequested, mProject, &Project::combineDates);
    connect(mEventPropertiesView, &EventPropertiesView::splitDateRequested, mProject, &Project::splitDate);
    connect(mEventPropertiesView, &EventPropertiesView::updateCalibRequested, this, &ModelView::updateCalibration);
    connect(mEventPropertiesView, &EventPropertiesView::showCalibRequested, this, &ModelView::showCalibration);

    connect(mButMultiCalib,   static_cast<void (Button::*)(bool)> (&Button::clicked), this, &ModelView::showMultiCalib);
    mMultiCalibrationView->setProject(mProject);
    connect(mProject, &Project::projectStateChanged, this, &ModelView::updateMultiCalibration);

    connect(mButCurve, &Button::toggled, MainWindow::getInstance(), &MainWindow::toggleCurve);
}

void ModelView::disconnectScenes()
{
    disconnect(mButNewEvent, &Button::clicked, this, &ModelView::createEventInPlace);
    disconnect(mButNewEventKnown, &Button::clicked, this, &ModelView::createEventKnownInPlace);
    disconnect(mButDeleteEvent,  static_cast<void (Button::*)(bool)> (&Button::clicked), mEventsScene, &EventsScene::deleteSelectedItems);

    disconnect(mButRecycleEvent, &Button::clicked, mProject, &Project::recycleEvents);
    disconnect(mButEventsGlobalView, &Button::toggled, mEventsOverview, &SceneGlobalView::setVisible);
    disconnect(mButEventsGlobalView, &Button::toggled, mEventsSearchEdit, &QLineEdit::setVisible);
    disconnect(mEventsSearchEdit, &QLineEdit::returnPressed, this, &ModelView::searchEvent);
    disconnect(mEventsGlobalZoom, &ScrollCompressor::valueChanged, this, &ModelView::updateEventsZoom);
    disconnect(mButExportEvents, &Button::clicked, this, &ModelView::exportEventsScene);
    disconnect(mButProperties, &Button::clicked, this, &ModelView::showProperties);

    disconnect(mButMultiCalib,  &Button::clicked, this, &ModelView::showMultiCalib);
    mMultiCalibrationView->setProject(nullptr);

    //disconnect(mButNewPhase, &Button::clicked, mProject, &Project::createPhase);
    disconnect(mButNewPhase, &Button::clicked, this, &ModelView::createPhaseInPlace);
    disconnect(mButDeletePhase, &Button::clicked, mPhasesScene, &PhasesScene::deleteSelectedItems);
    disconnect(mButPhasesGlobaliew, &Button::toggled, mPhasesGlobalView, &SceneGlobalView::setVisible);

    // when there is no Event selected we must show all data inside phases
    disconnect(mEventsScene, &EventsScene::noSelection, this, &ModelView::noEventSelected);
    disconnect(mEventsScene, &EventsScene::eventsAreSelected, mPhasesScene, &PhasesScene::eventsSelected);
    disconnect(mEventsScene, &EventsScene::eventDoubleClicked, this, &ModelView::togglePropeties);
    // When one or several phases are selected, we hightLigth the data inside the Event includes in the phases
    disconnect(mPhasesScene, &PhasesScene::noSelection, mEventsScene, &EventsScene::noHide);
    disconnect(mPhasesScene, &PhasesScene::phasesAreSelected, mEventsScene, &EventsScene::phasesSelected);

    disconnect(mProject, &Project::currentEventChanged, mEventPropertiesView, &EventPropertiesView::setEvent);
    // Properties View
    disconnect(mEventPropertiesView, &EventPropertiesView::combineDatesRequested, mProject, &Project::combineDates);
    disconnect(mEventPropertiesView, &EventPropertiesView::splitDateRequested, mProject, &Project::splitDate);
    disconnect(mEventPropertiesView, &EventPropertiesView::updateCalibRequested, this, &ModelView::updateCalibration);
    disconnect(mEventPropertiesView, &EventPropertiesView::showCalibRequested, this, &ModelView::showCalibration);

    disconnect(mButCurve, &Button::toggled, MainWindow::getInstance(), &MainWindow::toggleCurve);

}

Project* ModelView::getProject() const
{
   return  mProject;
}

void ModelView::resetInterface()
{
    mButProperties->setEnabled(false);
    mButProperties->setChecked(false);

    mButMultiCalib->setEnabled(false);
    mButMultiCalib->setChecked(false);

    mButDeleteEvent->setEnabled(false);
    mButDeleteEvent->setChecked(false);

    mButNewEvent->setEnabled(true);
    mButNewEventKnown->setEnabled(true);
    mButImport->setEnabled(true);

  //  noEventSelected();
    disconnectScenes();
    mProject = nullptr;
    mEventsScene->clean();
    mPhasesScene->clean();
    mCalibrationView->setDate(QJsonObject());

    mMultiCalibrationView->setProject(mProject);
    mMultiCalibrationView->setEventsList(QList<Event*> ());

    mEventPropertiesView->setEvent(QJsonObject());
   // hideProperties();
    updateLayout();
}

void ModelView::adaptStudyPeriodButton(const double& min, const double& max)
{
        /* Addapt mButModifyPeriod size and position */
    // same variable in updateLayout()
    //const int topButtonHeight =  int ( 1.3 * fontMetrics().height());//  1 * AppSettings::heigthUnit());
    const QString studyStr = tr("STUDY PERIOD") + QString(" [ %1 ; %2 ] BC/AD").arg(locale().toString(min), locale().toString(max));;
    mButModifyPeriod->setText(studyStr);
    //mButModifyPeriod->setIconOnly(false);
    //mButModifyPeriod ->setGeometry((mTopWrapper->width() - fontMetrics().boundingRect(mButModifyPeriod->text()).width()) /2 - 2*mMargin, (mTopWrapper->height() - topButtonHeight)/2, fontMetrics().boundingRect(mButModifyPeriod->text()).width() + 4*mMargin, topButtonHeight );

}

void ModelView::updateProject()
{
    QJsonObject state = mProject->state();
    const ProjectSettings settings = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());

    mTmin = settings.mTmin;
    mTmax = settings.mTmax;

    adaptStudyPeriodButton(settings.mTmin, settings.mTmax);
    updateCurveButton();

    setSettingsValid(settings.mTmin < settings.mTmax);

    mEventsScene->updateSceneFromState();

    mPhasesScene->updateSceneFromState();

    // Les sélections dans les scènes doivent être mises à jour après que
    // LES 2 SCENES aient été updatées
    // false : ne pas envoyer de notification pour updater l'état du projet,
    // puisque c'est justement ce que l'on fait ici!
     // DONE BY UPDATEPROJECT ????
/*
    const QJsonObject& event = mEventPropertiesView->getEvent();
    if (!event.isEmpty()) {
        const QJsonArray events = state.value(STATE_EVENTS).toArray();
        for (int i = 0; i < events.size(); ++i) {
            const QJsonObject evt = events.at(i).toObject();
            if (evt.value(STATE_ID).toInt() == event.value(STATE_ID).toInt()) {
                if (evt != event)
                    mEventPropertiesView->setEvent(evt);
            }
        }
    }
    */
    updateLayout();
}

bool ModelView::findCalibrateMissing()
{
    bool calibMissing = false;
    QJsonObject state = mProject->state();

    QJsonArray Qevents = state.value(STATE_EVENTS).toArray();

    /* If the Events Scene isEmpty (i.e. when the project is created)
    * There is no date to calibrate
    */
    if (!Qevents.isEmpty()) {
        QVector<Event> events;
        for (auto&& Qev: Qevents)
            events.append(Event::fromJson(Qev.toObject()));

        QProgressDialog *progress = new QProgressDialog("Calibration curve missing","Wait" , 1, 10);//, qApp->activeWindow(), Qt::Window);
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(nullptr);
        progress->setMinimumDuration(4);
        progress->setMinimum(0);

        progress->setMinimumWidth(int (progress->fontMetrics().horizontalAdvance(progress->labelText()) * 1.5));

        int position = 0;
        for (auto& ev : events)
            position += ev.mDates.size();
        progress->setMaximum(position);

        position = 0;
        // look for missing calibration

        for (auto&& ev : events) {
            for (auto&& date : ev.mDates) {
                // look if the refCurve is still in the Plugin Ref. Curves list
                // because the mProject->mCalibCurves is still existing but maybe not the curve in the list of the plugin
                if (date.mCalibration) {
                    const QStringList refsNames = date.getPlugin()->getRefsNames();
                    const QString dateRefName = date.getPlugin()->getDateRefCurveName(&date);

                    if (!dateRefName.isEmpty() && !refsNames.contains(dateRefName) ) {
                        calibMissing = true;
                        continue;
                    }
                } else {
                    calibMissing = true;
                    continue;
                }
                // look inside mProject->mCalibCurves, if there is a missing calibration
                // to try to rebuild it after
                const QString toFind (date.mUUID);
                QMap<QString, CalibrationCurve>::iterator it = mProject->mCalibCurves.find(toFind);
                if ( it == mProject->mCalibCurves.end()) {
                    calibMissing = true;
                    continue;
                }
                ++position;
                progress->setValue(position);
            }
            if (calibMissing) {
                progress->setValue(progress->maximum());
                continue;
            }
       }

    }
    return calibMissing;
}

void ModelView::calibrateAll(ProjectSettings newS)
{
    QJsonObject state = mProject->state();

    QJsonArray Qevents = state.value(STATE_EVENTS).toArray();

    /* If the Events Scene isEmpty (i.e. when the project is created)
    * There is no date to calibrate
    */
    if (!Qevents.isEmpty()) {
        mProject->mCalibCurves.clear();
        QVector<Event> events;
        for (auto&& Qev: Qevents)
            events.append(Event::fromJson(Qev.toObject()));

        //QProgressDialog *progress = new QProgressDialog("Calibration curve generation -----2","Wait" , 1, 10, qApp->activeWindow(), Qt::Widget);
        QProgressDialog *progress = new QProgressDialog("Calibration in progress...","Wait" , 1, 10);
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(nullptr);
        progress->setMinimumDuration(4);
        progress->setMinimum(0);
        //progress->setMinimumWidth(7 * AppSettings::widthUnit());
        progress->setMinimumWidth(int (progress->fontMetrics().horizontalAdvance(progress->labelText()) * 1.5));

        int position(0);
        for (auto& ev : events)
            position += ev.mDates.size();

        progress->setMaximum(position);

        position = 0;
        // rebuild all calibration
        for (auto&& ev : events) {
                 const QJsonArray listDates = ev.toJson().value(STATE_EVENT_DATES).toArray();

                for (auto&& date : listDates) {
                    Date d (date.toObject());
                    d.autoSetTiSampler(true);
                    // date.mCalibration->mCurve.clear();
                    d.calibrate(newS, mProject, true);
                    //d.calibrate(mProject);
                    ++position;
                    progress->setValue(position);
                }
          }
         mProject -> setSettings(newS); //do pushProjectState
    }


}

void ModelView::modifyPeriod()
{
    QJsonObject state = mProject->state();
    ProjectSettings s = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());

    StudyPeriodDialog dialog(qApp->activeWindow());
    dialog.setSettings(s);

    if (dialog.exec() == QDialog::Accepted) {
        ProjectSettings newS = dialog.getSettings();
        if (s != newS) {
          ModelView::calibrateAll(newS);

          Scale xScale;
          xScale.findOptimal(newS.mTmin, newS.mTmax, 7);
          xScale.tip = 4;
          mMultiCalibrationView->initScale(xScale);
          mCalibrationView->initScale(xScale);

          mProject -> setSettings(newS); //do pushProjectState //done in ModelView::calibrateAll(newS);??
          MainWindow::getInstance() -> setResultsEnabled(false);
          MainWindow::getInstance() -> setLogEnabled(false);
        }

    }

}

void ModelView::updateCurveButton()
{
    QJsonObject state = mProject->state();

    const CurveSettings cs = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());
    mButCurve->setText(tr("Curve Building : ") + cs.processText());
}

/**
 * @brief ModelView::setSettingsValid
 * Important : just disable "Run" and "Results" and "Log" when typing in tmin and tmax edit boxes
 * Run will be enabled again when validating all dates in the next valid study period
 * @param valid
 * @todo controle management with project->pushProjectState()
 */
void ModelView::setSettingsValid(bool valid)
{
    if (!valid) {
        MainWindow::getInstance()->setRunEnabled(false);
        MainWindow::getInstance()->setResultsEnabled(false);
        MainWindow::getInstance()->setLogEnabled(false);
    }
}

void ModelView::showHelp(bool show)
{
    mEventsScene->showHelp(show);
}

void ModelView::searchEvent()
{
    QString search = mEventsSearchEdit->text();

    if (search.isEmpty())
        return;

    // Search text has changed : regenerate corresponding events list
    if (search != mLastSearch) {
        mLastSearch = search;
        mSearchIds.clear();

        QJsonObject state = mProject->state();
        QJsonArray events = state.value(STATE_EVENTS).toArray();

        for (auto&& evJSON : events) {
            const QJsonObject event = evJSON.toObject();
            const int eventId = event.value(STATE_ID).toInt();
            const QString name = event.value(STATE_NAME).toString();

            if (name.contains(search, Qt::CaseInsensitive))
                mSearchIds.push_back(eventId);

            else {
                const QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();

                 for (auto&& datJSON : dates) {
                     const QJsonObject data = datJSON.toObject();
                     const QString dataName = data.value(STATE_NAME).toString();

                     if (dataName.contains(search, Qt::CaseInsensitive)) {
                         mSearchIds.push_back(eventId);
                         continue;
                     }
                 }
             }

        }

        mCurSearchIdx = 0;
    }
    else if (mSearchIds.size() > 0) {
        mCurSearchIdx += 1;
        if (mCurSearchIdx >= mSearchIds.size())
            mCurSearchIdx = 0;
    }

    if (mCurSearchIdx < mSearchIds.size())
        mEventsScene->centerOnEvent(mSearchIds[mCurSearchIdx]);


}

void ModelView::createEventInPlace()
{
    QRectF viewRect = mEventsView->rect();
    QPointF visiblePos = mEventsView->mapToScene( int (viewRect.x()), int (viewRect.y()));

    mProject->createEvent(visiblePos.x() + viewRect.width()/2., visiblePos.y() + viewRect.height()/2.);
}

void ModelView::createEventKnownInPlace()
{
    QRectF viewRect = mEventsView->rect();
    QPointF visiblePos = mEventsView->mapToScene( int (viewRect.x()), int (viewRect.y()));

    mProject->createEventKnown(visiblePos.x() + viewRect.width()/2., visiblePos.y() + viewRect.height()/2.);
}


void ModelView::createPhaseInPlace()
{
    QRectF viewRect = mPhasesView->rect();
    QPointF visiblePos = mPhasesView->mapToScene( int (viewRect.x()), int (viewRect.y()));

    mProject->createPhase(visiblePos.x() + viewRect.width()/2., visiblePos.y() + viewRect.height()/2.);
}
/**
 * @brief ModelView::showProperties is done to work with ModelView::showMultiCalib()
 * Both manage button to keep only one of them checked or any of them
 */
void ModelView::showProperties()
{
    // Why now ?!
  //  updateLayout();
    mAnimationHide->setStartValue(mRightRect);
    mAnimationHide->setEndValue(mRightHiddenRect);

    if (mButProperties->isChecked() && mButProperties->isEnabled()) {
       if (mButMultiCalib->isChecked()) {
           // hide mMultiCalibrationView

           mAnimationHide->setTargetObject(mMultiCalibrationView);
           mAnimationHide->start();

           mButMultiCalib->setChecked(false);

       } else  if (mCurveSettingsVisible) {
           // hide mMultiCalibrationView
           mAnimationHide->setTargetObject(mCurveSettingsView);
           mAnimationHide->start();

           mButCurve->setChecked(false);

       } else  if (mButImport->isChecked()) {
           // hide mMultiCalibrationView
           mAnimationHide->setTargetObject(mImportDataView);
           mAnimationHide->start();

           mButImport->setChecked(false);

       } else  {
           // hide mPhasesView

           mAnimationHide->setTargetObject(mPhasesView);
           mAnimationHide->start();
           mPhasesView->hide();
           mPhasesGlobalView->hide();

           mButNewPhase      ->hide();
           mButDeletePhase   ->hide();
           mButExportPhases  ->hide();
           mButPhasesGlobaliew->hide();
           mButPhasesGrid    ->hide();
           mPhasesGlobalZoom ->hide();
       }



        // show Properties View
        mEventPropertiesView->updateEvent();
        mEventPropertiesView->show();

        mCalibrationView->repaint();

        mAnimationCalib->setTargetObject(mCalibrationView);

        mAnimationShow->setStartValue(mRightHiddenRect);
        mAnimationShow->setEndValue(mRightRect);
       // mEventPropertiesView->raise();
        mAnimationShow->setTargetObject(mEventPropertiesView);
        mAnimationShow->start();

    } else {
       mAnimationCalib->setTargetObject(nullptr);
      // delete mCalibrationView;

        mAnimationHide->setTargetObject(mEventPropertiesView);
        mAnimationHide->start();

        mPhasesView->show();
        mPhasesGlobalView->setVisible(mButPhasesGlobaliew->isChecked());

        mButNewPhase      ->show();
        mButDeletePhase   ->show();
        mButExportPhases  ->show();
        mButPhasesGlobaliew->show();
        mButPhasesGrid    ->show();
        mPhasesGlobalZoom ->show();

        mAnimationShow->setTargetObject(mPhasesView);
        mAnimationShow->start();


     }

    //updateLayout();
}

void ModelView::hideProperties()
{
   if (mCalibrationView && (mCalibrationView->geometry() == mLeftRect) ) {
       mAnimationCalib->setTargetObject(mCalibrationView);
       mAnimationCalib->setStartValue(mLeftRect);
       mAnimationCalib->setEndValue(mLeftHiddenRect);
       mAnimationCalib->start();
    }
   if (mEventPropertiesView && (mEventPropertiesView->geometry() == mRightRect) ) {
       mAnimationHide->setTargetObject(mEventPropertiesView);
       mAnimationHide->setStartValue(mRightRect);
       mAnimationHide->setEndValue(mRightHiddenRect);
       mAnimationHide->start();
  }

}

/**
 * @brief ModelView::showMultiCalib is done to work with ModelView::showProperties()
 * Both manage button to keep only one of them checked or any of them
 */
void ModelView::showMultiCalib()
{
    //updateLayout();
    const bool showAnim = mEventsScene->getItemsList().size() < 21;
    if (showAnim) {
        mAnimationHide->setStartValue(mRightRect);
        mAnimationHide->setEndValue(mRightHiddenRect);
    }
    if (mButMultiCalib->isChecked() && mButMultiCalib->isEnabled()) {
        mPhasesGlobalZoom->hide();
        if (mButProperties->isChecked()) {
            // hide mEventPropertiesView
            if (showAnim) {
                mAnimationHide->setTargetObject(mEventPropertiesView);
                mAnimationHide->start();
            }
            mButProperties->setChecked(false);

        }  else  if (mButCurve->isChecked()) {
            // hide mCurveSettingsView
            if (showAnim) {
                mAnimationHide->setTargetObject(mCurveSettingsView);
                mAnimationHide->start();
            }

            mButCurve->setChecked(false);

        } else  if (mButImport->isChecked()) {
            // hide mCurveSettingsView
            if (showAnim) {
                mAnimationHide->setTargetObject(mImportDataView);
                mAnimationHide->start();
            }

            mButImport->setChecked(false);

        }
        mMultiCalibrationView->updateGraphList();
        mMultiCalibrationView->setVisible(true);
        if (showAnim) {
            mAnimationShow->setStartValue(mRightHiddenRect);
            mAnimationShow->setEndValue(mRightRect);

            // mMultiCalibrationView->raise();

            mAnimationShow->setTargetObject(mMultiCalibrationView);
            mAnimationShow->start();
        }
     } else {
        if (showAnim) {
            mAnimationHide->setTargetObject(mMultiCalibrationView);
            mAnimationHide->start();
        }
        if (mButPhasesGlobaliew->isChecked())
            mPhasesGlobalView->show();
    }
    updateLayout();
}

void ModelView::updateMultiCalibration()
{
    if (mButMultiCalib->isChecked())
        mMultiCalibrationView->updateMultiCalib();

    if (mButProperties->isChecked())
        mEventPropertiesView->updateEvent();

}

void ModelView::showImport()
{
   updateLayout();
   if (mButImport->isChecked()) {
       if (mProject->studyPeriodIsValid()) {
            mButProperties  -> setChecked(false);
            mButCurve  -> setChecked(false);

            mAnimationShow->setStartValue(mRightHiddenRect);
            mAnimationShow->setEndValue(mRightRect);
            mImportDataView->raise();
            mAnimationShow->setTargetObject(mImportDataView);
            mAnimationShow->start();
        }

   } else {
       if (!mButProperties->isChecked()) {
            mAnimationHide->setStartValue(mRightRect);
            mAnimationHide->setEndValue(mRightHiddenRect);
            mAnimationHide->setTargetObject(mImportDataView);
            mAnimationHide->start();

        } else
           mImportDataView->lower();
   }

}

/**
 * @brief ModelView::eventsAreSelected It's a bridge to stop the signal from Events'Scene::eventsAreSelected
 * if there is a panel with the phases scene
 */
void ModelView::eventsAreSelected()
{
    qDebug()<<"ModelView::eventsAreSelected()";
    if (!mButProperties->isEnabled()) {
        mButNewEventKnown->setDisabled(true);
        mButNewEvent->setDisabled(true);

        mButExportEvents->setDisabled(true);
        mButImport->setDisabled(true);

        mButProperties->setDisabled(false);
        mButMultiCalib->setDisabled(false);
        mButDeleteEvent->setDisabled(false);

     }

    updateLayout();

    if (!mButMultiCalib->isChecked() && !mButProperties->isChecked())
        mPhasesScene->eventsSelected();
}

void ModelView::togglePropeties()
{
    qDebug()<<"ModelView::togglePropeties()";
    //eventsAreSelected();

    mButNewEventKnown->setDisabled(true);
    mButNewEvent->setDisabled(true);

    mButExportEvents->setDisabled(true);
    mButImport->setDisabled(true);

    mButProperties->setDisabled(false);
    mButMultiCalib->setDisabled(false);
    mButDeleteEvent->setDisabled(false);

    mButProperties->setChecked(true);
    mButProperties->update();
      //  updateLayout();
    showProperties();

}

void ModelView::noEventSelected()
{
    qDebug()<<"ModelView::noEventSelected()";
    mButNewEventKnown->setDisabled(false);
    mButNewEvent->setDisabled(false);

    mButExportEvents->setDisabled(false);
    mButImport->setDisabled(false);

    /* useless modify managment
    * // we disable AutoEsclusive to be able to uncheck both Properties and MultiCalib
    * mButProperties->setAutoExclusive(false);
    * mButMultiCalib->setAutoExclusive(false);
    */

    mButProperties->setDisabled(true);
    mButProperties->setChecked(false);

    mButMultiCalib->setDisabled(true);
    mButMultiCalib->setChecked(false);

    mButDeleteEvent->setDisabled(true);

    mCalibrationView->hide();

    updateLayout();

    mPhasesScene->noHide();
}

void ModelView::showPhases()
{
    if (mProject->studyPeriodIsValid()) {
        showCalibration(false);

        mButImport->setChecked(false);
        slideRightPanel();

    } else
        mButImport->setChecked(false);

}

void ModelView::slideRightPanel()
{
    mAnimationShow->setStartValue(mRightHiddenRect);
    mAnimationShow->setEndValue(mRightRect);

    mAnimationHide->setStartValue(mRightRect);
    mAnimationHide->setEndValue(mRightHiddenRect);

    QWidget* target = nullptr;
    if (mButImport->isChecked())
        target = mImportDataView;

    else if (mButProperties->isChecked())
        target = mEventPropertiesView;

    if ( target && mCurrentRightWidget && (target != mCurrentRightWidget) ) {
        mCurrentRightWidget = target;
        target->raise();
        mAnimationShow->setTargetObject(target);
        mAnimationHide->start();
    }
    target = nullptr;
}

void ModelView::prepareNextSlide()
{
    QWidget* target = nullptr;
    if (mButImport->isChecked())
        target = mImportDataView;

    else if (mButProperties->isChecked())
        target = mEventPropertiesView;

    if (target)
        mAnimationHide->setTargetObject(target);

    target = nullptr;
}

// Painting
void ModelView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(mTopRect, Painting::borderDark);
    p.fillRect(mHandlerRect, Painting::borderDark);
}

// Layout
void ModelView::applyAppSettings()
{
    mMargin = int (0.3 * AppSettings::widthUnit());
    mToolbarH = int (2 * AppSettings::heigthUnit());
    mButtonWidth = int (1.7 * AppSettings::widthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mButtonHeigth = int (1.7 * AppSettings::heigthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mHandlerW = int (0.25 * AppSettings::widthUnit());

    // ------

   for (auto&& item : mEventsScene->getItemsList()) {
        EventItem* itm = static_cast<EventItem*>(item);
        if (Event::Type (itm->mData.value(STATE_EVENT_TYPE).toInt()) == Event::eDefault ) {
            itm->redrawEvent();
        }
        itm->update();

    }

    // ------
    mEventPropertiesView->applyAppSettings();

    for (auto&& item : mPhasesScene->getItemsList()) {
        static_cast<PhaseItem*>(item)->redrawPhase();
        static_cast<PhaseItem*>(item)->update();
    }

    // ------
    mCalibrationView->applyAppSettings();
    mMultiCalibrationView->applyAppSettings();

    adaptStudyPeriodButton(mTmin, mTmax);


    updateLayout();
}

void ModelView::resizeEvent(QResizeEvent* e)
{
    (void) e;
    updateLayout();
}

void ModelView::updateLayout()
{
    const QFontMetrics fm(font());
    
    mTopRect = QRect(0, 0, width(), 40);
    mTopWrapper->setGeometry(mTopRect);
    
    int margin = AppSettings::widthUnit();
    
    // Label left
    QString leftTitle = tr("Events Scene");
    if (mButProperties->isChecked()  && mEventPropertiesView->isCalibChecked() && !mButMultiCalib->isChecked()){
        leftTitle = tr("Calibrated Data View");
    }
    mLeftPanelTitle->setText(leftTitle);
    mLeftPanelTitle->setGeometry(margin, 0, fm.horizontalAdvance(leftTitle) + 10, mTopRect.height());
        
    // Label right
    updateRightPanelTitle();
    int labw = fm.horizontalAdvance(mRightPanelTitle->text()) + 10;
    mRightPanelTitle->setGeometry(std::max(0, width() - labw - margin), 0, labw, mTopRect.height());

    // Center buttons
    mButModifyPeriod->setGeometry(width()/2 - 251, 2, 300, mTopRect.height() - 4);
    mButCurve->setGeometry(mButModifyPeriod->x() + mButModifyPeriod->width() + 2, 5, 200, mTopRect.height() - 10);


    //-------------- Top Flag

    // coordinates in ModelView

    mHandlerRect = QRect(int((width()-mHandlerW)*mSplitProp), mTopRect.height(), mHandlerW, height() - mTopRect.height());
    mLeftWrapper->setGeometry(QRect(0, mTopRect.height(), mHandlerRect.x() , height() - mTopRect.height()));

    mRightWrapper->setGeometry(QRect(mHandlerRect.x() + mHandlerW, mTopRect.height(), std::max(0, width() - mHandlerRect.x() - mHandlerW +1), height() - mTopRect.height()));

     // coordinates in mLeftWrapper
    mLeftRect = QRect(0, 0, mLeftWrapper->width(), mLeftWrapper->height());
    mLeftHiddenRect = mLeftRect.adjusted(0, 0, -mLeftWrapper->width(), 0);

    // coordinates in mRightWrapper
    mRightRect = QRect(0, 0, mRightWrapper->width(), mRightWrapper->height());
    mRightHiddenRect = mRightRect.adjusted(mRightWrapper->width(), 0, 0, 0);

    mEventPropertiesView->setGeometry(mButProperties->isChecked() ? mRightRect : mRightHiddenRect);

    mMultiCalibrationView->setGeometry(mButMultiCalib->isChecked() ? mRightRect : mRightHiddenRect);

    mImportDataView->setGeometry(mButImport->isChecked() ? mRightRect : mRightHiddenRect);

    mCurveSettingsView->setGeometry(mCurveSettingsVisible ? mRightRect : mRightHiddenRect);

    // ----------

    const int radarW = 4 * AppSettings::widthUnit();
    const int radarH = 4 * AppSettings::heigthUnit();
    const int searchH = round(1.3 * fm.height());
    if (mButProperties->isChecked() && mEventPropertiesView->isCalibChecked())
        mEventsView ->setGeometry(0, 0, 0, 0);
    else
        mEventsView ->setGeometry(mLeftRect.adjusted(mButtonWidth -1, -1, +1, +1));

    mEventsSearchEdit->setGeometry(mEventsView->x() + 5, 5, radarW, searchH);
    mEventsOverview->setGeometry(mEventsView->x() + 5, mEventsSearchEdit->y() + mEventsSearchEdit->height(), radarW, radarH);

    mButNewEvent      ->setGeometry(0, 0, mButtonWidth, mButtonHeigth);
    mButNewEventKnown ->setGeometry(0, mButtonHeigth, mButtonWidth, mButtonHeigth);
    mButDeleteEvent   ->setGeometry(0, 2*mButtonHeigth, mButtonWidth, mButtonHeigth);
    mButRecycleEvent  ->setGeometry(0, 3*mButtonHeigth, mButtonWidth, mButtonHeigth);
    mButExportEvents  ->setGeometry(0, 4*mButtonHeigth, mButtonWidth, mButtonHeigth);
    mButEventsGlobalView->setGeometry(0, 5*mButtonHeigth, mButtonWidth, mButtonHeigth);
    mButEventsGrid    ->setGeometry(0, 6*mButtonHeigth, mButtonWidth, mButtonHeigth);
    mButProperties    ->setGeometry(0, 7*mButtonHeigth, mButtonWidth, mButtonHeigth);
    mButMultiCalib    ->setGeometry(0, 8*mButtonHeigth, mButtonWidth, mButtonHeigth);
    mButImport        ->setGeometry(0, 9*mButtonHeigth, mButtonWidth, mButtonHeigth);

    mEventsGlobalZoom ->setGeometry(0, 10*mButtonHeigth, mButtonWidth, mLeftRect.height() - 10*mButtonHeigth);

    const int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int helpW = qMin(400, mEventsView->width() - radarW - mMargin - sbe);
    const int helpH = mEventsScene->getHelpView()->heightForWidth(helpW);
    mEventsScene->getHelpView()->setGeometry(mEventsView->width() - mMargin - helpW, mMargin, helpW, helpH);

    // ----------
    if (mCalibrationView->isVisible()) {
        if (mButProperties->isChecked() && mEventPropertiesView->isCalibChecked() )
            mCalibrationView->setGeometry( mLeftRect );
        else
            mCalibrationView->setGeometry(0, 0, 0, 0);
    }
    // ----------
    if (mButProperties->isChecked() || mButMultiCalib->isChecked()
            || mButCurve->isChecked()) {

        mPhasesView->hide();
        mPhasesGlobalView->hide();

        mButNewPhase      ->hide();
        mButDeletePhase   ->hide();
        mButExportPhases  ->hide();
        mButPhasesGlobaliew->hide();
        mButPhasesGrid    ->hide();
        mPhasesGlobalZoom ->hide();

     }  else {
        mCurveSettingsView->hide();

        mPhasesView->show();
        mPhasesView->setGeometry(mRightRect.adjusted(-1, -1, -mButtonWidth, 1));
        if (mButPhasesGlobaliew->isChecked()) {
            mPhasesGlobalView->show();
            mPhasesGlobalView->setGeometry(5, 5, radarW, radarH);
        }
        mButNewPhase->show();
        mButNewPhase->setGeometry(mPhasesView->width() -2, 0              , mButtonWidth, mButtonHeigth);

        mButDeletePhase->show();
        mButDeletePhase->setGeometry(mPhasesView->width() -2, mButtonHeigth  , mButtonWidth, mButtonHeigth);

        mButExportPhases->show();
        mButExportPhases->setGeometry(mPhasesView->width() -2, 2*mButtonHeigth, mButtonWidth, mButtonHeigth);

        mButPhasesGlobaliew->show();
        mButPhasesGlobaliew->setGeometry(mPhasesView->width() -2, 3*mButtonHeigth, mButtonWidth, mButtonHeigth);

        mButPhasesGrid->show();
        mButPhasesGrid->setGeometry(mPhasesView->width() -2, 4*mButtonHeigth, mButtonWidth, mButtonHeigth);
        mPhasesGlobalZoom->show();
        mPhasesGlobalZoom->setGeometry(mPhasesView->width() -2, 5*mButtonHeigth, mButtonWidth, mRightRect.height() - 5*mButtonHeigth);
     }
    update();
}

// Zoom
void ModelView::updateEventsZoom(const double prop)
{
    const qreal scale = 2. * prop;
    mEventsView->setTransform(QTransform::fromScale(scale, scale));
    mEventsScene->adaptItemsForZoom(scale);
}

void ModelView::updatePhasesZoom(const double prop)
{
    const qreal scale = 2. * prop;
    mPhasesView->setTransform(QTransform::fromScale(scale, scale));
    mPhasesScene->adaptItemsForZoom(scale);
}

// Export images
void ModelView::exportEventsScene()
{
    exportSceneImage(mEventsScene);
}

void ModelView::exportPhasesScene()
{
    exportSceneImage(mPhasesScene);
}

void ModelView::exportSceneImage(QGraphicsScene* scene)
{
    //scene->clearSelection();
    scene->setSceneRect(scene->itemsBoundingRect().adjusted(-30, -30, 30, 30));
    QRect r = scene->sceneRect().toRect();
    QFileInfo fileInfo = saveWidgetAsImage(scene, r,
                                           tr("Save model image as..."),
                                           MainWindow::getInstance()->getCurrentPath());
    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
}


/**
 * @brief ModelView::updateCalibration
 * Calibration come from EventPropertiesView::updateCalibRequested
 * @param date
 */
void ModelView::updateCalibration(const QJsonObject& date)
{
    qDebug() <<" ModelView::updateCalibration mUUID" << date.value(STATE_DATE_UUID).toString();
    // Control and calibration if necessary
    if (!date.isEmpty() ) {
        Date d (date);
        if (d.mCalibration == nullptr)
            d.calibrate(ProjectSettings::fromJson(mProject->state().value(STATE_SETTINGS).toObject()), mProject, true);

        // A date has been double-clicked => update CalibrationView only if the date is not null
        if (mEventPropertiesView->isVisible() && mEventPropertiesView->isCalibChecked())
           mCalibrationView->setDate(d);

    }

}

// come from EventPropertiesView::showCalibRequested
void ModelView::showCalibration(bool show)
{
   updateLayout();
   if (show) {
       mEventPropertiesView->updateEvent();

       mCalibrationView->setVisible(true);
       mCalibrationView->repaint();
       mCalibrationView->raise();
       mAnimationCalib->setStartValue(mLeftHiddenRect);
       mAnimationCalib->setEndValue(mLeftRect);

    } else {
        mAnimationCalib->setStartValue(mLeftRect);
        mAnimationCalib->setEndValue(mLeftHiddenRect);
    }
    mAnimationCalib->start();

}

// Mouse Events
void ModelView::mousePressEvent(QMouseEvent* e)
{
    if (mHandlerRect.contains(e->pos()))
        mIsSplitting = true;
}

void ModelView::mouseReleaseEvent(QMouseEvent* e)
{
    (void) e;
    mIsSplitting = false;
}

void ModelView::mouseMoveEvent(QMouseEvent* e)
{
    if (mIsSplitting) {
        qreal x = qBound(200, e->pos().x(), width() - 450);

        mSplitProp = x / (width() - mHandlerW/2);
        mHandlerRect.moveTo(int(x + mHandlerW/2), mTopRect.height());

        updateLayout();
    }
}

void ModelView::keyPressEvent(QKeyEvent* event)
{
    QWidget::keyPressEvent(event);
}


void ModelView::writeSettings()
{
    QSettings settings;
    settings.beginGroup("ModelView");

    int panelIndex = 0;
    if (mButProperties->isChecked())
        panelIndex = 1;
    else if (mButImport->isChecked())
        panelIndex = 2;

    settings.setValue("right_panel", panelIndex);
    settings.setValue("events_zoom", mEventsGlobalZoom->getProp());
    settings.setValue("phases_zoom", mPhasesGlobalZoom->getProp());

    settings.endGroup();
}

void ModelView::readSettings()
{
    QSettings settings;
    settings.beginGroup("ModelView");

    int panelIndex = settings.value("right_panel", 0).toInt();

    if (panelIndex == 1)
        mButProperties->setChecked(true);

    else if (panelIndex == 2)
        mButImport->setChecked(true);

    mEventsGlobalZoom->setProp(settings.value("events_zoom", 1.).toDouble(), true);
    mPhasesGlobalZoom->setProp(settings.value("phases_zoom", 1.).toDouble(), true);

    updateLayout();

    settings.endGroup();
}

void ModelView::showCurveSettings(bool show)
{
    mCurveSettingsVisible = show;

    if (show) {
        if (mButProperties->isChecked()) {
            // hide mEventPropertiesView
            mAnimationHide->setStartValue(mRightRect);
            mAnimationHide->setEndValue(mRightHiddenRect);
            mAnimationHide->setTargetObject(mEventPropertiesView);
            mAnimationHide->start();

            mButProperties->setChecked(false);

        } else if (mButMultiCalib->isChecked()) {
            // hide mMultiCalibrationView
            mAnimationHide->setStartValue(mRightRect);
            mAnimationHide->setEndValue(mRightHiddenRect);
            mAnimationHide->setTargetObject(mMultiCalibrationView);
            mAnimationHide->start();

            mButMultiCalib->setChecked(false);

        } else if (mButImport->isChecked()) {
            // hide mImportDataView
            mAnimationHide->setStartValue(mRightRect);
            mAnimationHide->setEndValue(mRightHiddenRect);
            mAnimationHide->setTargetObject(mImportDataView);
            mAnimationHide->start();

            mButImport->setChecked(false);
        }

        mAnimationShow->setTargetObject(mCurveSettingsView);

        mAnimationShow->setStartValue(mRightHiddenRect);
        mAnimationShow->setEndValue(mRightRect);

       // mCurveSettingsView->raise();
       // mAnimationShow->setTargetObject(mEventPropertiesView);
        mAnimationShow->start();


        mCurveSettingsView->setVisible(show);

        if (mProject) {

            // Save the "enabled" state of Curve in project settings
            QJsonObject state = mProject->mState;
            CurveSettings settings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());
            // settings.mEnabled = toggle;
           // const bool isCurve = (settings.mProcessType != CurveSettings::eProcessTypeNone);
            state[STATE_CURVE] = settings.toJson();


            // Update the Curve settings view with the project's values
            mCurveSettingsView->setSettings(settings);
            mProject->pushProjectState(state, CURVE_SETTINGS_UPDATED_REASON, true);

            // Tell the event properties view if it should display Curve parameters
            //mEventPropertiesView->setCurveSettings(isCurve, settings.mProcessType);

            mEventsScene->updateSceneFromState();
        }

   // updateLayout();

    } else {


        mAnimationHide->setTargetObject(mCurveSettingsView);

        mAnimationHide->setStartValue(mRightRect);
        mAnimationHide->setEndValue(mRightHiddenRect);

        mAnimationHide->start();

        mAnimationShow->setTargetObject(mPhasesView);

        mAnimationShow->setStartValue(mRightHiddenRect);
        mAnimationShow->setEndValue(mRightRect);

       // mPhasesView->raise();

        mAnimationShow->start();




    }
    updateRightPanelTitle();
    updateLayout();

}

void ModelView::updateRightPanelTitle()
{
    QString rightTitle(tr("Phases Scene"));

    if (mCurveSettingsVisible) {
        rightTitle = tr("Curve Settings");

    } else if (mButProperties->isChecked()) {
        rightTitle = tr("Event Properties");

    } else if (mButImport->isChecked()) {
        rightTitle = tr("Import Data");

    } else if (mButMultiCalib->isChecked()) {
        rightTitle = tr("Multi-Calibration View");
    }

    mRightPanelTitle->setText(rightTitle);
}
