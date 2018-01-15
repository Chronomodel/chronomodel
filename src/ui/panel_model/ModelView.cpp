#include "ModelView.h"
#include "EventsScene.h"
#include "PhasesScene.h"
#include "Event.h"
#include "EventKnown.h"
#include "Painting.h"
#include "Button.h"
#include "Label.h"
#include "LineEdit.h"
#include "ImportDataView.h"
#include "EventPropertiesView.h"
#include "SceneGlobalView.h"
#include "ScrollCompressor.h"
#include "CalibrationView.h"
#include "MultiCalibrationView.h"

#include "HelpWidget.h"
#include "MainWindow.h"
#include "Project.h"
#include "QtUtilities.h"
#include "StdUtilities.h"
#include "StudyPeriodDialog.h"
#include "DateUtils.h"
#include "PluginManager.h"

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
mMargin(5),
mToolbarH(50),
mButtonWidth(50),
mSplitProp(0.6),
mHandlerW(10),
mIsSplitting(false),
mCalibVisible(false)
{
    setMouseTracking(true);
    QFontMetrics fm(font());
    mTopRect = QRect(0, 0, width(), 3 * fm.height());
    mTopWrapper = new QWidget(this);
    mTopWrapper->setGeometry(mTopRect);
  //  mTopWrapper->setMouseTracking(true);

    mHandlerRect = QRect((width()-mHandlerW)*mSplitProp, mTopRect.height(), mHandlerW, height() - mTopRect.height());

    mLeftRect = QRect(0, mTopRect.height(), width() - mHandlerRect.x(), height() - mTopRect.height());
    mLeftHiddenRect = mLeftRect.adjusted(0, 0, mLeftRect.width(), 0);

    mLeftWrapper = new QWidget(this);
    mLeftWrapper->setGeometry(mLeftRect);
    
    mRightRect = QRect(mLeftRect.width() + mHandlerRect.width(), mTopRect.height(), width() - mLeftRect.width() - mHandlerRect.width(), height() - mTopRect.height());
    mRightHiddenRect = mRightRect.adjusted(mRightRect.width(), 0, 0, 0);

    mRightWrapper = new QWidget(this);
    mRightWrapper->setGeometry(mRightRect);

    // ---- Header Top Bar with Study period --------------
    // ----------- on mTopWrapper ------------------

    //const int topButtonHeight = fm.height() + 10;
    //const QString studyStr = tr("STUDY PERIOD") + " [ "+ locale().toString(mTmin) +" : "+ locale().toString(mTmax) + " ] BC/AD";
    mButModifyPeriod = new Button(tr("STUDY PERIOD") , mTopWrapper);
    //mButModifyPeriod->setIconOnly(false);
   // mButModifyPeriod ->setGeometry((mTopWrapper->width() - fm.width(mButModifyPeriod->text()) + 30) /2, (mTopWrapper->height() - topButtonHeight)/2, fm.width(mButModifyPeriod->text()) + 30, topButtonHeight );
   // mButModifyPeriod->setFlatHorizontal();
    adaptStudyPeriodButton(mTmin, mTmax);
    connect(mButModifyPeriod,  static_cast<void (QPushButton::*)(bool)>(&Button::clicked), this, &ModelView::modifyPeriod);

    mLeftPanelTitle = new Label(tr("Events' Scene"), mTopWrapper);
    mLeftPanelTitle->setLight();
    mRightPanelTitle = new Label(tr("Phases' Scene"), mTopWrapper);
    mRightPanelTitle->setLight();
    
   // ---- Windows on the left hand Event scene --------------
    mEventsView = new QGraphicsView(mLeftWrapper);
    mEventsScene = new EventsScene(mEventsView);
    mEventsView->setScene(mEventsScene);
    mEventsView->setAcceptDrops(true);
    mEventsView->setInteractive(true);
    mEventsView->setDragMode(QGraphicsView::RubberBandDrag);
    
    mEventsGlobalView = new SceneGlobalView(mEventsScene, mEventsView, mLeftWrapper);
    mEventsGlobalView->setVisible(false);
    
    mEventsSearchEdit = new LineEdit(mLeftWrapper);
    mEventsSearchEdit->setPlaceholderText(tr("Search Event or Data..."));
    mEventsSearchEdit->setVisible(false);
    
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
    mButExportEvents->setToolTip(tr("Save image of the Events' scene as a file"));
    mButExportEvents->setIcon(QIcon(":picture_save.png"));
    mButExportEvents->setFlatVertical();
    
    mButEventsOverview = new Button(tr("Overview"), mLeftWrapper);
    mButEventsOverview->setToolTip(tr("Overview on the global Events' scene"));
    mButEventsOverview->setIcon(QIcon(":eye_w.png"));
    mButEventsOverview->setCheckable(true);
    mButEventsOverview->setFlatVertical();
    
    mButEventsGrid = new Button(tr("Grid"), mLeftWrapper);
    mButEventsGrid->setToolTip(tr("Drawing a grid under the event' scene to organize"));
    mButEventsGrid->setIcon(QIcon(":grid2.png"));
    mButEventsGrid->setCheckable(true);
    mButEventsGrid->setFlatVertical();
    
    mEventsGlobalZoom = new ScrollCompressor(mLeftWrapper);
    mEventsGlobalZoom->setProp(1);
    mEventsGlobalZoom->showText(tr("Zoom"), true);
    mEventsGlobalZoom->setVertical(true);
    
    connect(mButEventsGrid, &Button::toggled, mEventsScene, &EventsScene::showGrid);

    mButProperties = new Button(tr("Properties"), mLeftWrapper);
    mButProperties->setToolTip(tr("Show the propeties of the first selected Event"));
    mButProperties->setCheckable(true);
    mButProperties->setIcon(QIcon(":settings_w.png"));
    mButProperties->setChecked(false);
    mButProperties->setDisabled(true);
    mButProperties->setFlatHorizontal();

    mButMultiCalib = new Button(tr("MultiCalib"), mLeftWrapper);
    mButMultiCalib->setToolTip(tr("Show the calibrated curves of data within selected Events"));
    mButMultiCalib->setCheckable(true);
    mButMultiCalib->setIcon(QIcon(":multiCalib_w.png"));
    mButMultiCalib->setChecked(false);
    mButMultiCalib->setDisabled(true);
    mButMultiCalib->setFlatHorizontal();

    mButImport = new Button(tr("Data"), mLeftWrapper);
    mButImport->setToolTip(tr("Show the data importation panel from file"));
    mButImport->setCheckable(true);
    mButImport->setIcon(QIcon(":csv_import.png"));
    mButImport->setFlatHorizontal();
    //mButImport->setAutoExclusive(false);

    connect(mButImport, static_cast<void (Button::*)(bool)>(&Button::toggled), this, &ModelView::showImport);
   // connect(mEventsScene, &EventsScene::eventDoubleClicked, mButProperties, &Button::click);

    // -------- Windows Data Importation --------------------
    
    mImportDataView = new ImportDataView(mRightWrapper);
    connect(mEventsScene, &EventsScene::csvDataLineDropAccepted, mImportDataView, &ImportDataView::removeCsvRows);
    connect(mEventsScene, &EventsScene::csvDataLineDropRejected, mImportDataView, &ImportDataView::errorCsvRows);
    
    
    
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
    //connect(mPhasesScene, SIGNAL(selectionChanged()), this, SLOT(update()));
    //connection between scene
    //connect(mPhasesScene, SIGNAL(selectionChanged()), mEventsGlobalView, SLOT(update()));
    //connect(mEventsScene, SIGNAL(selectionChanged()), this, SLOT(update()));

    mButNewPhase = new Button(tr("New Phase"), mRightWrapper);
    mButNewPhase->setToolTip(tr("Create a new Phase"));
    mButNewPhase->setIcon(QIcon(":new_event.png"));
    mButNewPhase->setFlatVertical();
    
    mButDeletePhase = new Button(tr("Delete Phase"), mRightWrapper);
    mButDeletePhase->setToolTip(tr("Delete selected Phase(s)"));
    mButDeletePhase->setIcon(QIcon(":delete.png"));
    mButDeletePhase->setFlatVertical();
    
    mButExportPhases = new Button(tr("Save Image"), mRightWrapper);
    mButExportPhases->setToolTip(tr("Save image of the Phases' scene as a file"));
    mButExportPhases->setIcon(QIcon(":picture_save.png"));
    mButExportPhases->setFlatVertical();
    
    mButPhasesOverview = new Button(tr("Overview"), mRightWrapper);
    mButPhasesOverview->setToolTip(tr("Overview on the global Phases' scene"));
    mButPhasesOverview->setIcon(QIcon(":eye_w.png"));
    mButPhasesOverview->setCheckable(true);
    mButPhasesOverview->setFlatVertical();
    
    mButPhasesGrid = new Button(tr("Grid"), mRightWrapper);
    mButEventsGrid->setToolTip(tr("Drawing a grid under the Phases' scene to organize"));
    mButPhasesGrid->setIcon(QIcon(":grid2.png"));
    mButPhasesGrid->setCheckable(true);
    mButPhasesGrid->setFlatVertical();
    
    mPhasesGlobalZoom = new ScrollCompressor(mRightWrapper);
    mPhasesGlobalZoom->setProp(1);
    mPhasesGlobalZoom->showText(tr("Zoom"), true);
    
    connect(mPhasesGlobalZoom, &ScrollCompressor::valueChanged, this, &ModelView::updatePhasesZoom);
    connect(mButExportPhases, &Button::clicked, this, &ModelView::exportPhasesScene);
    
    connect(mButPhasesOverview, &Button::toggled, mPhasesGlobalView, &SceneGlobalView::setVisible);
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

    // ---- update and paint
    
    updateLayout();
}

ModelView::~ModelView()
{
    
}

void ModelView::setFont(const QFont & font)
{
  mLeftWrapper->setFont(font);
  mButNewEvent->setFont(font);
  mButNewEventKnown->setFont(font);
  mButDeleteEvent->setFont(font);
  mButRecycleEvent->setFont(font);
  mButExportEvents->setFont(font);
  mButEventsOverview->setFont(font);
  mButEventsGrid->setFont(font);

  mImportDataView->setFont(font);
  mEventPropertiesView->setFont(font);

  mRightWrapper->setFont(font);
  mButNewPhase->setFont(font);
  mButDeletePhase->setFont(font);
  mButExportPhases->setFont(font);
  mButPhasesOverview->setFont(font);
  mButPhasesGrid->setFont(font);

  if (mCalibrationView)
      mCalibrationView->setFont(font);
  mMultiCalibrationView->setFont(font);

  mButModifyPeriod->setFont(font);

  mButProperties->setFont(font);
  mButMultiCalib->setFont(font);
  mButImport->setFont(font);
}

void ModelView::setProject(Project* project)
{
    assert(project!= nullptr);
    const bool projectExist = (mProject ? true : false);
    mProject = project;
    mPhasesScene->setProject(mProject);
    mEventsScene->setProject(mProject);

    if (mProject && !projectExist)
        connectScenes();

    else if (projectExist && !mProject)
            disconnectScenes();

    // if there is no phase, we must show all events
    const QJsonArray phases = mProject->state().value(STATE_PHASES).toArray();
    if (phases.size() == 0 )
        mEventsScene->setShowAllThumbs(true);

}

void ModelView::connectScenes()
{
    connect(mButNewEvent, &Button::clicked, mProject, &Project::createEvent);
    connect(mButNewEventKnown, &Button::clicked, mProject, &Project::createEventKnown);
    connect(mButDeleteEvent, &Button::clicked, mProject, &Project::deleteSelectedEvents);
    connect(mButRecycleEvent, &Button::clicked, mProject, &Project::recycleEvents);
    connect(mButEventsOverview, &Button::toggled, mEventsGlobalView, &SceneGlobalView::setVisible);
    connect(mButEventsOverview, &Button::toggled, mEventsSearchEdit, &LineEdit::setVisible);
    connect(mEventsSearchEdit, &LineEdit::returnPressed, this, &ModelView::searchEvent);
    connect(mEventsGlobalZoom, &ScrollCompressor::valueChanged, this, &ModelView::updateEventsZoom);
    connect(mButExportEvents, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &ModelView::exportEventsScene);

    //connect(mButProperties, static_cast<void (Button::*)(bool)> (&Button::toggled), this, &ModelView::showProperties);
    connect(mButProperties, &Button::clicked, this, &ModelView::showProperties);

    connect(mButNewPhase, &Button::clicked, mProject, &Project::createPhase);
    connect(mButDeletePhase, &Button::clicked, mPhasesScene, &PhasesScene::deleteSelectedItems);// mProject, &Project::deleteSelectedPhases);


    // when there is no Event selected we must show all data inside phases
    //connect(mEventsScene, &EventsScene::noSelection, mPhasesScene, &PhasesScene::noHide);
    connect(mEventsScene, &EventsScene::noSelection, this, &ModelView::noEventSelected);
    //connect(mEventsScene, &EventsScene::eventsAreSelected, mPhasesScene, &PhasesScene::eventsSelected);
    connect(mEventsScene, &EventsScene::eventsAreSelected, this, &ModelView::eventsAreSelected);

    connect(mEventsScene, &EventsScene::eventDoubleClicked, this, &ModelView::togglePropeties);//mButProperties, &Button::toggle);

    // When one or several phases are selected, we hightLigth the data inside the Events included in the phases
    connect(mPhasesScene, &PhasesScene::noSelection, mEventsScene, &EventsScene::noHide);
    connect(mPhasesScene, &PhasesScene::phasesAreSelected, mEventsScene, &EventsScene::phasesSelected);

    connect(mProject, &Project::currentEventChanged, mEventPropertiesView, &EventPropertiesView::setEvent);
    connect(mEventPropertiesView, &EventPropertiesView::combineDatesRequested, mProject, &Project::combineDates);
    connect(mEventPropertiesView, &EventPropertiesView::splitDateRequested, mProject, &Project::splitDate);

    // Properties View
    connect(mEventPropertiesView, &EventPropertiesView::updateCalibRequested, this, &ModelView::updateCalibration);
    connect(mEventPropertiesView, &EventPropertiesView::showCalibRequested, this, &ModelView::showCalibration);

    connect(mButMultiCalib,  &Button::clicked, this, &ModelView::showMultiCalib);
    connect(mProject, &Project::projectStateChanged, this, &ModelView::updateMultiCalibration);
    mMultiCalibrationView->setProject(mProject);

}

void ModelView::disconnectScenes()
{
    disconnect(mButNewEvent, SIGNAL(clicked(bool)), mProject, SLOT(createEvent()));
    disconnect(mButNewEventKnown, &Button::clicked, mProject, &Project::createEventKnown);
    disconnect(mButDeleteEvent, &Button::clicked, mProject, &Project::deleteSelectedEvents);
    disconnect(mButRecycleEvent, &Button::clicked, mProject, &Project::recycleEvents);
    disconnect(mButEventsOverview, &Button::toggled, mEventsGlobalView, &SceneGlobalView::setVisible);
    disconnect(mButEventsOverview, &Button::toggled, mEventsSearchEdit, &LineEdit::setVisible);
    disconnect(mEventsSearchEdit, &LineEdit::returnPressed, this, &ModelView::searchEvent);
    disconnect(mEventsGlobalZoom, &ScrollCompressor::valueChanged, this, &ModelView::updateEventsZoom);
    disconnect(mButExportEvents, &Button::clicked, this, &ModelView::exportEventsScene);
    disconnect(mButProperties, &Button::clicked, this, &ModelView::showProperties);

    disconnect(mButMultiCalib,  &Button::clicked, this, &ModelView::showMultiCalib);
    mMultiCalibrationView->setProject(nullptr);

    disconnect(mButNewPhase, &Button::clicked, mProject, &Project::createPhase);
    disconnect(mButDeletePhase, &Button::clicked, mPhasesScene, &PhasesScene::deleteSelectedItems);

    // when there is no Event selected we must show all data inside phases
    disconnect(mEventsScene, &EventsScene::noSelection, this, &ModelView::noEventSelected);
    disconnect(mEventsScene, &EventsScene::eventsAreSelected, mPhasesScene, &PhasesScene::eventsSelected);
    disconnect(mEventsScene, &EventsScene::eventDoubleClicked, this, &ModelView::togglePropeties);
    // When one or several phases are selected, we hightLigth the data inside the Event includes in the phases
    disconnect(mPhasesScene, &PhasesScene::noSelection, mEventsScene, &EventsScene::noHide);
    disconnect(mPhasesScene, &PhasesScene::phasesAreSelected, mEventsScene, &EventsScene::phasesSelected);

    disconnect(mProject, &Project::currentEventChanged, mEventPropertiesView, &EventPropertiesView::setEvent);
    disconnect(mEventPropertiesView, &EventPropertiesView::combineDatesRequested, mProject, &Project::combineDates);
    disconnect(mEventPropertiesView, &EventPropertiesView::splitDateRequested, mProject, &Project::splitDate);

    // Properties View
    disconnect(mEventPropertiesView, &EventPropertiesView::updateCalibRequested, this, &ModelView::updateCalibration);
    disconnect(mEventPropertiesView, &EventPropertiesView::showCalibRequested, this, &ModelView::showCalibration);

}

Project* ModelView::getProject() const
{
   return  mProject;
}

void ModelView::resetInterface()
{
    mButProperties->setEnabled(false);
    mButMultiCalib->setEnabled(false);
    mButDeleteEvent->setEnabled(false);

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

    updateLayout();
}

void ModelView::adaptStudyPeriodButton(const double& min, const double& max)
{
    QFontMetrics fm(font());
    const int topButtonHeight = fm.height() + 10;
    const int hMarg = 15;
    const QString studyStr = tr("STUDY PERIOD") + " [ "+ locale().toString(min) +" : "+ locale().toString(max) + " ] BC/AD";
    mButModifyPeriod->setText(studyStr);
    mButModifyPeriod->setIconOnly(false);
    mButModifyPeriod ->setGeometry((mTopWrapper->width() - fm.width(mButModifyPeriod->text())) /2 -hMarg, (mTopWrapper->height() - topButtonHeight)/2, fm.width(mButModifyPeriod->text()) + hMarg, topButtonHeight );
}

void ModelView::createProject()
{
    showCalibration(false);

    QJsonObject state = mProject->state();
    const ProjectSettings settings = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());

    mTmin = settings.mTmin;
    mTmax = settings.mTmax;

    adaptStudyPeriodButton(settings.mTmin, settings.mTmax);

    mProject->mState[STATE_SETTINGS_TMIN] = settings.mTmin;
    mProject->mState[STATE_SETTINGS_TMAX] = settings.mTmax;
    mProject->mState[STATE_SETTINGS_STEP] = settings.mStep;

    mProject->mState[STATE_SETTINGS_STEP_FORCED] = settings.mStepForced;

    setSettingsValid(settings.mTmin < settings.mTmax);

    //Unselect all Item in all scene
    mProject->unselectedAllInState();

    mEventsScene->createSceneFromState();
    mPhasesScene->createSceneFromState();

    // open and set the properties View, if there is no phase
    const QJsonArray phases = mProject->mState.value(STATE_PHASES).toArray();
    if (phases.isEmpty()) {
        const QJsonObject& event = mEventPropertiesView->getEvent();
        if (!event.isEmpty()) {
            const QJsonArray events = state.value(STATE_EVENTS).toArray();
            for (int i=0; i<events.size(); ++i) {
                const QJsonObject evt = events.at(i).toObject();
                if (evt.value(STATE_ID).toInt() == event.value(STATE_ID).toInt()) {
                    if (evt != event)
                        mEventPropertiesView->setEvent(evt);
                }
            }
        }
    }
}

void ModelView::updateProject()
{
   // showCalibration(false);
    
    QJsonObject state = mProject->state();
    const ProjectSettings settings = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());
   
    mTmin = settings.mTmin;
    mTmax = settings.mTmax;

    adaptStudyPeriodButton(settings.mTmin, settings.mTmax);

    mProject->mState[STATE_SETTINGS_TMIN] = settings.mTmin;
    mProject->mState[STATE_SETTINGS_TMAX] = settings.mTmax;
    mProject->mState[STATE_SETTINGS_STEP] = settings.mStep;

    mProject->mState[STATE_SETTINGS_STEP_FORCED] = settings.mStepForced;

    setSettingsValid(settings.mTmin < settings.mTmax);
    
    mEventsScene->updateSceneFromState();

    mPhasesScene->updateSceneFromState();
    
    // Les sélections dans les scènes doivent être mises à jour après que
    // LES 2 SCENES aient été updatées
    // false : ne pas envoyer de notification pour updater l'état du projet,
    // puisque c'est justement ce que l'on fait ici!
     // DONE BY UPDATEPROJECT ????
    
    const QJsonObject& event = mEventPropertiesView->getEvent();
    if (!event.isEmpty()) {
        const QJsonArray events = state.value(STATE_EVENTS).toArray();
        for (int i=0; i<events.size(); ++i) {
            const QJsonObject evt = events.at(i).toObject();
            if (evt.value(STATE_ID).toInt() == event.value(STATE_ID).toInt()) {
                if (evt != event)
                    mEventPropertiesView->setEvent(evt);
            }
        }
    }
}

bool ModelView::findCalibrateMissing()
{
    bool calibMissing (false);
    QJsonObject state = mProject->state();

    QJsonArray Qevents = state.value(STATE_EVENTS).toArray();

    /* If the Events' Scene isEmpty (i.e. when the project is created)
    * There is no date to calibrate
    */
    if (!Qevents.isEmpty()) {
        QList<Event> events;
        for (auto Qev: Qevents)
            events.append(Event::fromJson(Qev.toObject()));

        QProgressDialog *progress = new QProgressDialog("Calibration curve missing","Wait" , 1, 10, qApp->activeWindow());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(0);
        progress->setMinimumDuration(4);
        progress->setMinimum(0);

        int position(0);
        for (auto && ev : events)
            position += ev.mDates.size();
        progress->setMaximum(position);

        position = 0;
        // look for missing calibration

        for (auto && ev : events) {
            for (auto && date : ev.mDates) {
                // look if the refCurve is still in the Plugin Ref. Curves list
                // because the mProject->mCalibCurves is still existing but maybe not the curve in the list of the plugin
                if (date.mCalibration) {
                    const QStringList refsNames = date.getPlugin()->getRefsNames();
                    const QString dateRefName = date.getPlugin()->getDateRefCurveName(&date);

                    if (!dateRefName.isEmpty() && !refsNames.contains(dateRefName) ) {
                        calibMissing = true;
                        continue;
                    }
                }
                // look inside mProject->mCalibCurves, if there is a missing calibration
                // to try to rebuild it after
                const QString toFind (date.mName+date.getDesc());
                QMap<QString, CalibrationCurve>::const_iterator it = mProject->mCalibCurves.find(toFind);
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

    /* If the Events' Scene isEmpty (i.e. when the project is created)
    * There is no date to calibrate
    */
    if (!Qevents.isEmpty()) {
        mProject->mCalibCurves.clear();
        QList<Event> events;
        for (auto Qev: Qevents)
            events.append(Event::fromJson(Qev.toObject()));

        QProgressDialog *progress = new QProgressDialog("Calibration curve generation","Wait" , 1, 10, qApp->activeWindow());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(0);
        progress->setMinimumDuration(4);
        progress->setMinimum(0);

        int position(0);
        for (auto && ev : events)
            position += ev.mDates.size();
        progress->setMaximum(position);

        position = 0;
        // rebuild all calibration
        for (auto && ev : events)
            for (auto && date : ev.mDates) {
                // date.mCalibration->mCurve.clear();
                date.calibrate(newS, mProject);
                ++position;
                progress->setValue(position);
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

          mProject -> setSettings(newS); //do pushProjectState
          MainWindow::getInstance() -> setResultsEnabled(false);
          MainWindow::getInstance() -> setLogEnabled(false);
        }

    }

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
    if (search != mLastSearch){
        mLastSearch = search;
        mSearchIds.clear();
        
        QJsonObject state = mProject->state();
        QJsonArray events = state.value(STATE_EVENTS).toArray();
        
        for (auto evJSON : events) {
            const QJsonObject event = evJSON.toObject();
            const int eventId = event.value(STATE_ID).toInt();
            const QString name = event.value(STATE_NAME).toString();
            
            if (name.contains(search, Qt::CaseInsensitive))
                mSearchIds.push_back(eventId);

            else {
                const QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();

                 for (auto &&datJSON : dates) {
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

/**
 * @brief ModelView::showProperties is done to work with ModelView::showMultiCalib()
 * Both manage button to keep only one of them checked or any of them
 */
void ModelView::showProperties()
{
   updateLayout();

   if (mButProperties->isChecked() && mButProperties->isEnabled()) {
       if (mButMultiCalib->isChecked()) {
           // hide mMultiCalibrationView
           mAnimationHide->setStartValue(mRightRect);
           mAnimationHide->setEndValue(mRightHiddenRect);
           mAnimationHide->setTargetObject(mMultiCalibrationView);
           mAnimationHide->start();

           mButMultiCalib->setChecked(false);
       }
        mButImport-> setChecked(false);

        // show Properties View
        mEventPropertiesView->updateEvent();

        mCalibrationView->repaint();
        mEventPropertiesView->show();

        mAnimationCalib->setTargetObject(mCalibrationView);

        mAnimationShow->setStartValue(mRightHiddenRect);
        mAnimationShow->setEndValue(mRightRect);
        mEventPropertiesView->raise();
        mAnimationShow->setTargetObject(mEventPropertiesView);
        mAnimationShow->start();

    } else {
       mAnimationCalib->setTargetObject(nullptr);
      // delete mCalibrationView;

        mAnimationHide->setStartValue(mRightRect);
        mAnimationHide->setEndValue(mRightHiddenRect);
        mAnimationHide->setTargetObject(mEventPropertiesView);
        mAnimationHide->start();



     }

}

/**
 * @brief ModelView::showMultiCalib is done to work with ModelView::showProperties()
 * Both manage button to keep only one of them checked or any of them
 */
void ModelView::showMultiCalib()
{
    updateLayout();
    if (mButMultiCalib->isChecked() && mButMultiCalib->isEnabled()) {
        if (mButProperties->isChecked()) {
            // hide mEventPropertiesView
            mAnimationHide->setStartValue(mRightRect);
            mAnimationHide->setEndValue(mRightHiddenRect);
            mAnimationHide->setTargetObject(mEventPropertiesView);
            mAnimationHide->start();

            mButProperties->setChecked(false);
        }

        mMultiCalibrationView->updateGraphList();
        mMultiCalibrationView->setVisible(true);

        mButImport      -> setChecked(false);
        mAnimationShow->setStartValue(mRightHiddenRect);
        mAnimationShow->setEndValue(mRightRect);
        mMultiCalibrationView->raise();

        mAnimationShow->setTargetObject(mMultiCalibrationView);
        mAnimationShow->start();

     } else {
             mAnimationHide->setStartValue(mRightRect);
             mAnimationHide->setEndValue(mRightHiddenRect);
             mAnimationHide->setTargetObject(mMultiCalibrationView);
             mAnimationHide->start();
      }

}

void ModelView::updateMultiCalibration()
{
    if (mButMultiCalib->isChecked()) {
        mMultiCalibrationView->updateMultiCalib();
    }
}

void ModelView::showImport()
{
   updateLayout();
   if (mButImport->isChecked()) {
       if (mProject->studyPeriodIsValid()) {
            mButProperties  -> setChecked(false);
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
 * if there is a panel with the phases' scene
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

        /* useless
        * mButProperties->setAutoExclusive(false);
        * mButMultiCalib->setAutoExclusive(false);
        */
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

    /* useless
     * mButProperties->setAutoExclusive(false);
     * mButMultiCalib->setAutoExclusive(false);
     */


mButProperties->setChecked(true);
mButProperties->update();
  //  updateLayout();
showProperties();

    /*
    mButProperties->click();

    mEventPropertiesView->updateEvent();
    mButImport      -> setChecked(false);
    mCalibrationView->repaint();

    mAnimationCalib->setTargetObject(mCalibrationView);

    mAnimationShow->setStartValue(mRightHiddenRect);
    mAnimationShow->setEndValue(mRightRect);
    mEventPropertiesView->raise();
    mAnimationShow->setTargetObject(mEventPropertiesView);
    mAnimationShow->start();
    */
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
    mButMultiCalib->setDisabled(true);

   // if (mButProperties->isChecked())
        mButProperties->setChecked(false);

   // if (mButMultiCalib->isChecked())
        mButMultiCalib->setChecked(false);

    mButDeleteEvent->setDisabled(true);
    updateLayout();

    mPhasesScene->noHide();
}

void ModelView::showPhases()
{
    if (mProject->studyPeriodIsValid()) {
        showCalibration(false);
        
        //mButProperties->setChecked(false);
        mButImport->setChecked(false);
        slideRightPanel();

    } else {
        //mButProperties->setChecked(true);
        mButImport->setChecked(false);
    }
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
//    else if (mButPhasesModel->isChecked())
//        target = mPhasesWrapper;
    else if (mButProperties->isChecked())
        target = mEventPropertiesView;
    
    if ( target && (target != mCurrentRightWidget) ) {
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
 //   else if (mButPhasesModel->isChecked())
 //       target = mPhasesWrapper;
    else if (mButProperties->isChecked())
        target = mEventPropertiesView;
    
    if (target)
        mAnimationHide->setTargetObject(target);

    target = nullptr;
}

//#pragma mark Painting
void ModelView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(mTopRect, QColor(50, 50, 50));
  //  p.fillRect(mLeftRect, QColor(50, 50, 50));
    p.fillRect(mHandlerRect, QColor(50, 50, 50));
 //   p.fillRect(mRightRect, QColor(50, 50, 50));

}

//#pragma mark Layout
void ModelView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void ModelView::updateLayout()
{

    const QFontMetrics fm (font());
    const int textSpacer(fm.width("_") * 2);
    mTopRect = QRect(0, 0, width(), 3 * fm.height() );
    const int topButtonHeight = fm.height() + 6;
    mTopWrapper->setGeometry(mTopRect);
    mTopWrapper->setGeometry(mTopRect);
    //-------------- Top Flag
    // ---------- Panel Title
    QString leftTitle (tr("Events' Scene"));
    if (mButProperties->isChecked()  && mEventPropertiesView->isCalibChecked() && !mButMultiCalib->isChecked())
        leftTitle = tr("Calibrated Data View");

    mLeftPanelTitle->setText(leftTitle);
    mLeftPanelTitle ->setGeometry(textSpacer, mButModifyPeriod->y(), fm.width(mLeftPanelTitle->text()), topButtonHeight );

    QString rightTitle (tr("Phases' Scene"));
    if (mButProperties->isChecked() )
        rightTitle = tr("Event Properties");
    else if (mButImport->isChecked())
        rightTitle = tr("Import Data");
    else if (mButMultiCalib->isChecked())
            rightTitle = tr("Multi-Calibration View");

    mRightPanelTitle->setText(rightTitle);
    mRightPanelTitle->setGeometry( width() - (fm.width(mRightPanelTitle->text()) + textSpacer), mButModifyPeriod->y(), fm.width(mRightPanelTitle->text()), topButtonHeight );

    //------- Study Period

    //mButModifyPeriod ->setGeometry((mTopWrapper->width() - fm.width(mButModifyPeriod->text()) + 10) /2, (mTopWrapper->height() - topButtonHeight)/2, fm.width(mButModifyPeriod->text()) + 5, topButtonHeight );
    mButModifyPeriod ->setGeometry((mTopWrapper->width() - fm.width(mButModifyPeriod->text())) /2 -15, (mTopWrapper->height() - topButtonHeight)/2, fm.width(mButModifyPeriod->text()) + 15, topButtonHeight );

    // coordinates in ModelView

    mHandlerRect = QRect((width()-mHandlerW)*mSplitProp, mTopRect.height(), mHandlerW, height() - mTopRect.height());
    mLeftWrapper->setGeometry(QRect(0, mTopRect.height(), mHandlerRect.x(), height() - mTopRect.height()));

    mRightWrapper->setGeometry(QRect(mHandlerRect.x() + mHandlerRect.width(), mTopRect.height(), width() - mHandlerRect.x() - mHandlerRect.width(), height() - mTopRect.height()));

    // coordinates in mLeftWrapper
    mLeftRect = QRect(0, 0, mLeftWrapper->width(), mLeftWrapper->height());
    mLeftHiddenRect = mLeftRect.adjusted(0, 0, -mLeftWrapper->width(), 0);

    // coordinates in mRightWrapper
    mRightRect = QRect(0, 0, mRightWrapper->width(), mRightWrapper->height());
    mRightHiddenRect = mRightRect.adjusted(mRightWrapper->width(), 0, 0, 0);

    mEventPropertiesView->setGeometry(mButProperties->isChecked() ? mRightRect : mRightHiddenRect);

    mMultiCalibrationView->setGeometry(mButMultiCalib->isChecked() ? mRightRect : mRightHiddenRect);

    mImportDataView->setGeometry(mButImport->isChecked() ? mRightRect : mRightHiddenRect);

    // ----------

    const qreal radarW (150.);
    const qreal radarH (200.);
    const qreal searchH (fm.height() + 6);
    if (mButProperties->isChecked() && mEventPropertiesView->isCalibChecked())
        mEventsView      ->setGeometry(0, 0, 0, 0);
    else
        mEventsView      ->setGeometry(mLeftRect.adjusted(mButtonWidth, 0, 0, 0));

    mEventsSearchEdit->setGeometry(mEventsView->x() + 5, 5, radarW, searchH);
    mEventsGlobalView->setGeometry(mEventsView->x() + 5, mEventsSearchEdit->y() + mEventsSearchEdit->height(), radarW, radarH);
    
    mButNewEvent      ->setGeometry(0, 0, mButtonWidth, mButtonWidth);
    mButNewEventKnown ->setGeometry(0, mButtonWidth, mButtonWidth, mButtonWidth);
    mButDeleteEvent   ->setGeometry(0, 2*mButtonWidth, mButtonWidth, mButtonWidth);
    mButRecycleEvent  ->setGeometry(0, 3*mButtonWidth, mButtonWidth, mButtonWidth);
    mButExportEvents  ->setGeometry(0, 4*mButtonWidth, mButtonWidth, mButtonWidth);
    mButEventsOverview->setGeometry(0, 5*mButtonWidth, mButtonWidth, mButtonWidth);
    mButEventsGrid    ->setGeometry(0, 6*mButtonWidth, mButtonWidth, mButtonWidth);
    mButProperties ->setGeometry(0, 7*mButtonWidth, mButtonWidth, mButtonWidth);
    mButMultiCalib->setGeometry(0, 8*mButtonWidth, mButtonWidth, mButtonWidth);
    mButImport     ->setGeometry(0, 9*mButtonWidth, mButtonWidth, mButtonWidth);

    mEventsGlobalZoom ->setGeometry(0, 10*mButtonWidth, mButtonWidth, mLeftRect.height() - 10*mButtonWidth);

    const int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const qreal helpW = qMin(400.0, mEventsView->width() - radarW - mMargin - sbe);
    const qreal helpH = mEventsScene->getHelpView()->heightForWidth(helpW);
    mEventsScene->getHelpView()->setGeometry(mEventsView->width() - mMargin - helpW, mMargin, helpW, helpH);

    // ----------
    if (mCalibrationView) {
        if (mButProperties->isChecked() && mEventPropertiesView->isCalibChecked() )
            mCalibrationView->setGeometry( mLeftRect );
        else
            mCalibrationView->setGeometry(0, 0, 0, 0);
    }
    // ----------
    if (mButProperties->isChecked() || mButMultiCalib->isChecked()) {
        mPhasesView ->resize(0, 0);
        mPhasesGlobalView->setGeometry(5, 5, radarW, radarH);

        mButNewPhase->resize(0, 0);
        mButDeletePhase->resize(0, 0);
        mButExportPhases->resize(0, 0);
        mButPhasesOverview->resize(0, 0);
        mButPhasesGrid->resize(0, 0);
        mPhasesGlobalZoom->resize(0, 0);
     }  else {
        mPhasesView->setGeometry(mRightRect.adjusted(0, 0, -mButtonWidth, 0));

        mPhasesGlobalView->setGeometry(5, 5, radarW, radarH);

        mButNewPhase      ->setGeometry(mPhasesView->width(), 0, mButtonWidth, mButtonWidth);
        mButDeletePhase   ->setGeometry(mPhasesView->width(), mButtonWidth, mButtonWidth, mButtonWidth);
        mButExportPhases  ->setGeometry(mPhasesView->width(), 2*mButtonWidth, mButtonWidth, mButtonWidth);
        mButPhasesOverview->setGeometry(mPhasesView->width(), 3*mButtonWidth, mButtonWidth, mButtonWidth);
        mButPhasesGrid    ->setGeometry(mPhasesView->width(), 4*mButtonWidth, mButtonWidth, mButtonWidth);
        mPhasesGlobalZoom ->setGeometry(mPhasesView->width(), 5*mButtonWidth, mButtonWidth, mRightRect.height() - 5*mButtonWidth);
     }
    update();
}

// Zoom
void ModelView::updateEventsZoom(const double prop)
{
    const qreal scale = prop;
    QMatrix matrix;
    matrix.scale(scale, scale);
    mEventsView->setMatrix(matrix);
    mEventsScene->adaptItemsForZoom(scale);
}

void ModelView::updatePhasesZoom(const double prop)
{
    const qreal scale = prop;
    QMatrix matrix;
    matrix.scale(scale, scale);
    mPhasesView->setMatrix(matrix);
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
    scene->setSceneRect(scene->itemsBoundingRect());
    QRect r = scene->sceneRect().toRect();
    QFileInfo fileInfo = saveWidgetAsImage(scene, r,
                                           tr("Save model image as..."),
                                           MainWindow::getInstance()->getCurrentPath(),MainWindow::getInstance()->getAppSettings());
    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
}

//Calibration come from EventPropertiesView::updateCalibRequested
void ModelView::updateCalibration(const QJsonObject& date)
{
    // A date has been double-clicked => update CalibrationView only if the date is not null
    if (!date.isEmpty() && mEventPropertiesView->isCalibChecked())
        mCalibrationView->setDate(date);

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
    Q_UNUSED(e);
    mIsSplitting = false;
}

void ModelView::mouseMoveEvent(QMouseEvent* e)
{
    if (mIsSplitting) {
        mSplitProp = (double)e->pos().x() / (double)width();
        qreal x = width() * mSplitProp;

        qreal minLeft = 200.;
        qreal minRight = 580. + mHandlerW/2;
        x = qBound(minLeft, x, width()- minRight);
        mSplitProp = x / (double) width();
        mHandlerRect.moveTo(x+(mHandlerW)/2, mTopRect.height());

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

