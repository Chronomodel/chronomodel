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
#include "HelpWidget.h"
#include "MainWindow.h"
#include "Project.h"
#include "QtUtilities.h"
#include "StdUtilities.h"
#include "StepDialog.h"
#include "DateUtils.h"
#include "PluginManager.h"
#include <QtWidgets>
#include <QtSvg>
#include <QPropertyAnimation>
#include <QRectF>
#include <assert.h>

#pragma mark constructor
ModelView::ModelView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mEventsScene(0),
mCurSearchIdx(0),
mPhasesScene(0),
mCurrentRightWidget(0),
mTmin(0),
mTmax(2000),
mProject(nullptr),
mMargin(5),
mToolbarH(60),
mButtonWidth(80),
mSplitProp(0.6f),
mHandlerW(15),
mIsSplitting(false),
mCalibVisible(false)
{
    setMouseTracking(true);


    mLeftWrapper = new QWidget(this);
    mLeftWrapper->setGeometry(0,0, round((this->width()-mHandlerW)/2), this->height());
    
    mRightWrapper = new QWidget(this);
    QRectF mRightWrapperRectF((this->width()-mHandlerW)/2,0, (this->width()-mHandlerW)/2, this->height());
    mRightWrapper->setGeometry(mRightWrapperRectF.toRect());
    
   // ---- Windows on the left hand Event scene --------------
    mEventsView = new QGraphicsView(mLeftWrapper);
    mEventsScene = new EventsScene(mEventsView);
    mEventsView->setScene(mEventsScene);
    mEventsView->setAcceptDrops(true);
    mEventsView->setInteractive(true);
    mEventsView->setDragMode(QGraphicsView::RubberBandDrag);
    
    mEventsGlobalView = new SceneGlobalView(mEventsScene, mEventsView, mEventsView);
    mEventsGlobalView->setVisible(false);
    
    mEventsSearchEdit = new LineEdit(mEventsView);
    mEventsSearchEdit->setPlaceholderText(tr("Search event name..."));
    mEventsSearchEdit->setVisible(false);
    
    mButNewEvent = new Button(tr("New Event"), mLeftWrapper);
    mButNewEvent->setIcon(QIcon(":new_event.png"));
    mButNewEvent->setFlatVertical();
    
    mButNewEventKnown = new Button(tr("New Bound"), mLeftWrapper);
    mButNewEventKnown->setIcon(QIcon(":new_bound.png"));
    mButNewEventKnown->setFlatVertical();
    
    mButDeleteEvent = new Button(tr("Delete"), mLeftWrapper);
    mButDeleteEvent->setIcon(QIcon(":delete.png"));
    mButDeleteEvent->setFlatVertical();
    
    mButRecycleEvent = new Button(tr("Restore"), mLeftWrapper);
    mButRecycleEvent->setIcon(QIcon(":restore.png"));
    mButRecycleEvent->setFlatVertical();
    
    mButExportEvents = new Button(tr("Save Image"), mLeftWrapper);
    mButExportEvents->setIcon(QIcon(":picture_save.png"));
    mButExportEvents->setFlatVertical();
    
    mButEventsOverview = new Button(tr("Overview"), mLeftWrapper);
    mButEventsOverview->setIcon(QIcon(":eye_w.png"));
    mButEventsOverview->setCheckable(true);
    mButEventsOverview->setFlatVertical();
    
    mButEventsGrid = new Button(tr("Grid"), mLeftWrapper);
    mButEventsGrid->setIcon(QIcon(":grid2.png"));
    mButEventsGrid->setCheckable(true);
    mButEventsGrid->setFlatVertical();
    
    mEventsGlobalZoom = new ScrollCompressor(mLeftWrapper);
    mEventsGlobalZoom->setProp(1);
    mEventsGlobalZoom->showText(tr("Zoom"), true);
    
    // just to refresh when selection changes :
    //connect(mEventsScene, SIGNAL(selectionChanged()), mEventsGlobalView, SLOT(update()));
    connect(mButEventsOverview, SIGNAL(toggled(bool)), mEventsGlobalView, SLOT(setVisible(bool)));
    connect(mButEventsOverview, SIGNAL(toggled(bool)), mEventsSearchEdit, SLOT(setVisible(bool)));
    
    connect(mEventsSearchEdit, &LineEdit::returnPressed, this, &ModelView::searchEvent);
    connect(mEventsGlobalZoom, &ScrollCompressor::valueChanged, this, &ModelView::updateEventsZoom);
    connect(mButEventsGrid, SIGNAL(toggled(bool)), mEventsScene, SLOT(showGrid(bool)));
    connect(mButExportEvents, &Button::clicked, this, &ModelView::exportEventsScene);
    
    // -------- Windows Data Importation --------------------
    
    mImportDataView = new ImportDataView(mRightWrapper);
    connect(mEventsScene, &EventsScene::csvDataLineDropAccepted, mImportDataView, &ImportDataView::removeCsvRows);
    connect(mEventsScene, &EventsScene::csvDataLineDropRejected, mImportDataView, &ImportDataView::errorCsvRows);
    
    
    
    // -------- Windows Phase scene ---------------------------
    
    mPhasesWrapper = new QWidget(mRightWrapper);
    
    mPhasesView = new QGraphicsView(mPhasesWrapper);
    mPhasesScene = new PhasesScene(mPhasesView);
    mPhasesView->setScene(mPhasesScene);
    mPhasesView->setInteractive(true);
    mPhasesView->setDragMode(QGraphicsView::RubberBandDrag);

    // this signal has already been connected inside the EventsView constructor, to make sure events are marked as selected.
    // Thus, the following connection can make use in updateCheckedPhases of these marks.
    // -> no more used, changed when set inser and extract button in the PhaseItem
    //connect(mEventsScene, &EventsScene::selectionChanged, mPhasesScene, &PhasesScene::updateCheckedPhases);
    
    mPhasesGlobalView = new SceneGlobalView(mPhasesScene, mPhasesView, mPhasesWrapper);
    mPhasesGlobalView->setVisible(false);
    //connect(mPhasesScene, SIGNAL(selectionChanged()), this, SLOT(update()));
    //connection between scene
    //connect(mPhasesScene, SIGNAL(selectionChanged()), mEventsGlobalView, SLOT(update()));
    //connect(mEventsScene, SIGNAL(selectionChanged()), this, SLOT(update()));

    mButNewPhase = new Button(tr("New Phase"), mPhasesWrapper);
    mButNewPhase->setIcon(QIcon(":new_event.png"));
    mButNewPhase->setFlatVertical();
    
    mButDeletePhase = new Button(tr("Delete Phase"), mPhasesWrapper);
    mButDeletePhase->setIcon(QIcon(":delete.png"));
    mButDeletePhase->setFlatVertical();
    
    mButExportPhases = new Button(tr("Save Image"), mPhasesWrapper);
    mButExportPhases->setIcon(QIcon(":picture_save.png"));
    mButExportPhases->setFlatVertical();
    
    mButPhasesOverview = new Button(tr("Overview"), mPhasesWrapper);
    mButPhasesOverview->setIcon(QIcon(":eye_w.png"));
    mButPhasesOverview->setCheckable(true);
    mButPhasesOverview->setFlatVertical();
    
    mButPhasesGrid = new Button(tr("Grid"), mPhasesWrapper);
    mButPhasesGrid->setIcon(QIcon(":grid2.png"));
    mButPhasesGrid->setCheckable(true);
    mButPhasesGrid->setFlatVertical();
    
    mPhasesGlobalZoom = new ScrollCompressor(mPhasesWrapper);
    mPhasesGlobalZoom->setProp(1);
    mPhasesGlobalZoom->showText(tr("Zoom"), true);
    
    connect(mPhasesGlobalZoom, SIGNAL(valueChanged(double)), this, SLOT(updatePhasesZoom(double)));
    connect(mButExportPhases, SIGNAL(clicked()), this, SLOT(exportPhasesScene()));
    
    connect(mButPhasesOverview, SIGNAL(toggled(bool)), mPhasesGlobalView, SLOT(setVisible(bool)));
    connect(mButPhasesGrid, SIGNAL(toggled(bool)), mPhasesScene, SLOT(showGrid(bool)));
    
   
    
    // -------- Windows Event propreties -----------------------
    
    mStudyLab = new Label(tr("STUDY PERIOD (BC/AD)"), mRightWrapper);
    mMinLab = new Label(tr("Start"), mRightWrapper);
    mMaxLab = new Label(tr("End") , mRightWrapper);
    //mStepLab = new Label(tr("Step") + " :", mRightWrapper);
    
   // qreal butW = 80;
 /*   qDebug()<<"ModelView::ModelView mRightWrapper->x()="<<mRightWrapper->x();
    qDebug()<<"ModelView::ModelView mRightWrapper->y()="<<mRightWrapper->y();
    qDebug()<<"ModelView::ModelView mRightWrapper->width()="<<mRightWrapper->width()<<"height "<<mRightWrapper->height();
  */
    
    mEventPropertiesView = new EventPropertiesView(mRightWrapper);


    
    // -----------Header on mRightWrapper ------------------
    
    mStudyLab->setLight();
    mMinLab  ->setLight();
    mMaxLab  ->setLight();
    //mStepLab->setLight();
    
    mStudyLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    //QFont font(QApplication::font());
    //font.setPointSizeF(pointSize(13.f));
   // mStudyLab->setFont(font);
    QPalette palette = mStudyLab->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    mStudyLab->setPalette(palette);

   // mMinLab->setFont(font);
   // mMaxLab->setFont(font);
    
    //mStepLab->setToolTip(prepareTooltipText(tr("Step :"), tr("The step is useful for large study periods.\nFor example with a step of 10 years, calibrated date's values will be stored every 10 years.\nIt lowers memory requirements and graph plots are faster.\nHowever, interpolation between these points\nleads to less precision in calculations.")));
    
    mMinEdit = new LineEdit(mRightWrapper);
    mMinEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mMinEdit->setAlignment(Qt::AlignHCenter);
    //mMinEdit->setFont(font.style(),font.pointSize(),QFont::Bold,font);
    
    mMaxEdit = new LineEdit(mRightWrapper);
    mMaxEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mMaxEdit->setAlignment(Qt::AlignHCenter);
    //mStepEdit = new LineEdit(mRightWrapper);
    //mMinEdit->setFont(font);
    //mMaxEdit->setFont(font);

    mButApply = new Button(tr("Apply"), mRightWrapper);
    mButApply->setColorState(Button::eWarning);
    
    mButStep = new Button(tr("Calib. Resol."), mRightWrapper);
    
    //connect(mMinEdit, SIGNAL(textChanged(const QString&)), this, SLOT(studyPeriodChanging()));//MaxEditChanging
    //connect(mMaxEdit, SIGNAL(textChanged(const QString&)), this, SLOT(studyPeriodChanging()));
    //connect(mStepEdit, SIGNAL(textChanged(const QString&)), this, SLOT(studyPeriodChanging()));
    
    connect(mMinEdit, SIGNAL(textEdited(const QString&)), this, SLOT(minEditChanging()));
    connect(mMaxEdit, SIGNAL(textEdited(const QString&)), this, SLOT(maxEditChanging()));
    
    connect(mButApply, SIGNAL(clicked()), this, SLOT(applySettings()));
    connect(mButStep,  SIGNAL(clicked()), this, SLOT(adjustStep()));
    
    // --------
    
    mButProperties = new Button(tr("Properties"), mRightWrapper);
    mButProperties->setCheckable(true);
    mButProperties->setAutoExclusive(true);
    mButProperties->setIcon(QIcon(":settings_w.png"));
    mButProperties->setChecked(true);
    mButProperties->setFlatHorizontal();
    
    mButImport = new Button(tr("Data"), mRightWrapper);
    mButImport->setCheckable(true);
    mButImport->setAutoExclusive(true);
    mButImport->setIcon(QIcon(":csv_import.png"));
    mButImport->setFlatHorizontal();
    
    mButPhasesModel = new Button(tr("Phases"), mRightWrapper);
    mButPhasesModel->setCheckable(true);
    mButPhasesModel->setAutoExclusive(true);
    mButPhasesModel->setIcon(QIcon(":model_w.png"));
    mButPhasesModel->setFlatHorizontal();
    
    connect(mButProperties, SIGNAL(clicked()), this, SLOT(showProperties()));
    connect(mButImport, SIGNAL(clicked()), this, SLOT(showImport()));
    connect(mButPhasesModel, SIGNAL(clicked()), this, SLOT(showPhases()));
    
    connect(mEventsScene, SIGNAL(eventDoubleClicked()), mButProperties, SLOT(click()));
    // --- resize
/*    mButProperties ->setGeometry(mRightWrapper->width() - 3*butW, 0, butW, mToolbarH);
    mButImport     ->setGeometry(mRightWrapper->width() - 2*butW, 0, butW, mToolbarH);
    mButPhasesModel->setGeometry(mRightWrapper->width() - butW, 0, butW, mToolbarH);
    qDebug()<<"call setgeometry";
*/    // mEventPropertiesView->setGeometry(0,mToolbarH, mRightWrapper->width(),mRightWrapper->height()- mToolbarH );
  //  qDebug()<<"ModelView::ModelView mEventPropertiesView heigth "<<mEventPropertiesView->height();
    // ------------- Windows calibration ---------------------
    
    mCalibrationView = new CalibrationView(this);
    
    // -------- Animation -------------------------------
    
    mAnimationHide = new QPropertyAnimation();
    mAnimationHide->setPropertyName("geometry");
    mAnimationHide->setDuration(200);
    mAnimationHide->setTargetObject(mEventPropertiesView);
    mAnimationHide->setEasingCurve(QEasingCurve::Linear);
    
    mAnimationShow = new QPropertyAnimation();
    mAnimationShow->setPropertyName("geometry");
    mAnimationShow->setDuration(200);
    mAnimationShow->setEasingCurve(QEasingCurve::OutCubic);
    
    connect(mAnimationHide, SIGNAL(finished()), mAnimationShow, SLOT(start()));
    connect(mAnimationShow, SIGNAL(finished()), this, SLOT(prepareNextSlide()));
    
   
    
    mAnimationCalib = new QPropertyAnimation();
    mAnimationCalib->setPropertyName("geometry");
    mAnimationCalib->setTargetObject(mCalibrationView);
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

  mCalibrationView->setFont(font);

  mStudyLab->setFont(font);
  mMinLab->setFont(font);
  mMaxLab->setFont(font);

  mMinEdit->setFont(font);
  mMaxEdit->setFont(font);
  mButApply->setFont(font);
  mButStep->setFont(font);

  mButProperties->setFont(font);
  mButImport->setFont(font);
  mButPhasesModel->setFont(font);
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

    connect(mButNewPhase, &Button::clicked, mProject, &Project::createPhase);
    connect(mButDeletePhase, &Button::clicked, mPhasesScene, &PhasesScene::deleteSelectedItems);// mProject, &Project::deleteSelectedPhases);


    // when there is no Event selected we must show all data inside phases
    connect(mEventsScene, &EventsScene::noSelection, mPhasesScene, &PhasesScene::noHide);
    connect(mEventsScene, &EventsScene::eventsAreSelected, mPhasesScene, &PhasesScene::eventsSelected);

    // When one or several phases are selected, we hightLigth the data inside the Event includes in the phases
    connect(mPhasesScene, &PhasesScene::noSelection, mEventsScene, &EventsScene::noHide);
    connect(mPhasesScene, &PhasesScene::phasesAreSelected, mEventsScene, &EventsScene::phasesSelected);

    connect(mProject, &Project::currentEventChanged, mEventPropertiesView, &EventPropertiesView::setEvent);
    connect(mEventPropertiesView, &EventPropertiesView::combineDatesRequested, mProject, &Project::combineDates);
    connect(mEventPropertiesView, &EventPropertiesView::splitDateRequested, mProject, &Project::splitDate);

    // Properties View
    connect(mEventPropertiesView, &EventPropertiesView::updateCalibRequested, this, &ModelView::updateCalibration);
    connect(mEventPropertiesView, &EventPropertiesView::showCalibRequested, this, &ModelView::showCalibration);

}

void ModelView::disconnectScenes()
{
    disconnect(mButNewEvent, SIGNAL(clicked(bool)), mProject, SLOT(createEvent()));
    disconnect(mButNewEventKnown, &Button::clicked, mProject, &Project::createEventKnown);
    disconnect(mButDeleteEvent, &Button::clicked, mProject, &Project::deleteSelectedEvents);
    disconnect(mButRecycleEvent, &Button::clicked, mProject, &Project::recycleEvents);

    disconnect(mButNewPhase, &Button::clicked, mProject, &Project::createPhase);
    disconnect(mButDeletePhase, &Button::clicked, mPhasesScene, &PhasesScene::deleteSelectedItems);//deleteSelectedPhases);

    // when there is no Event selected we must show all data inside phases
    disconnect(mEventsScene, &EventsScene::noSelection, mPhasesScene, &PhasesScene::noHide);
    disconnect(mEventsScene, &EventsScene::eventsAreSelected, mPhasesScene, &PhasesScene::eventsSelected);

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
    disconnectScenes();
    mProject = nullptr;
    mEventsScene->clean();
    mPhasesScene->clean();
    mCalibrationView->setDate(QJsonObject());
    mEventPropertiesView->setEvent(QJsonObject());
    mButProperties->click();
    updateLayout();
}


void ModelView::createProject()
{
    showCalibration(false);

    QJsonObject state = mProject->state();
    const ProjectSettings settings = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());

    mTmin = settings.mTmin;
    mTmax = settings.mTmax;

    mMinEdit->setText(QString::number(settings.mTmin));
    mMaxEdit->setText(QString::number(settings.mTmax));
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
    showCalibration(false);
    
    QJsonObject state = mProject->state();
    const ProjectSettings settings = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());
   
    mTmin = settings.mTmin;
    mTmax = settings.mTmax;
    
    mMinEdit->setText(QString::number(settings.mTmin));
    mMaxEdit->setText(QString::number(settings.mTmax));
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

    //mEventsScene->updateSelection(false);
    //mPhasesScene->updateSelection(false);
    
    /** @todo Refresh current date !! */
    
    //mCalibrationView->setDate();
    
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

void ModelView::applySettings()
{
    showCalibration(false);
    
    QJsonObject state = mProject->state();
    ProjectSettings s = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());
    ProjectSettings oldSettings = s;
    
    // This was used to display study period using the date format defined in application settings :
    //mTmax = DateUtils::convertFromAppSettingsFormat(mMaxEdit->text().toDouble());
    //mTmin = DateUtils::convertFromAppSettingsFormat(mMinEdit->text().toDouble());
    
    mTmax = mMaxEdit->text().toDouble();
    mTmin = mMinEdit->text().toDouble();
    
    qDebug()<<"ModelView::applySettings()"<<mTmin<<mTmax;
    
    s.mTmin = (int) mTmin;
    s.mTmax = (int) mTmax;
    if (!s.mStepForced)
        s.mStep = ProjectSettings::getStep(s.mTmin, s.mTmax);
    
    if (!mProject->setSettings(s)) {
        // Min and max are not consistents :
        // go back to previous values
        
        mTmin = oldSettings.mTmin;
        mTmax = oldSettings.mTmax;
        
        mMinEdit->setText(QString::number(mTmin));
        mMaxEdit->setText(QString::number(mTmax));
        
        if (oldSettings.mTmin < oldSettings.mTmax)
            setSettingsValid(true);

    } else {
        state[STATE_SETTINGS_TMIN] = mTmin;
        state[STATE_SETTINGS_TMAX] = mTmax;
       // state[STATE_SETTINGS_STEP] ;

     //   project->pushProjectState(state,"Study Period Change",false,false);
      //  emit project->projectStateChanged();
    }
}

void ModelView::adjustStep()
{
    QJsonObject state = mProject->state();
    ProjectSettings s = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());
    
    double defaultVal = ProjectSettings::getStep(s.mTmin, s.mTmax);
    
    StepDialog dialog(qApp->activeWindow(), Qt::Sheet);
    dialog.setStep(s.mStep, s.mStepForced, defaultVal);
    
    if (dialog.exec() == QDialog::Accepted) {
        s.mStepForced = dialog.forced();
        if (s.mStepForced) {
            if (s.mStep != dialog.step()) {
                s.mStep = dialog.step();
                // rebuild all calibration
                QList<Event*> events = mProject->mModel->mEvents;

                QProgressDialog *progress = new QProgressDialog("Calibration curve generation","Wait" , 1, 10, qApp->activeWindow());
                progress->setWindowModality(Qt::WindowModal);
                progress->setCancelButton(0);
                progress->setMinimumDuration(4);
                progress->setMinimum(0);

                int position(0);
                for (auto && ev : events)
                    position += ev->mDates.size();
                progress->setMaximum(position);

                position = 0;

                for (auto && ev : events)
                    for (auto && date : ev->mDates) {
                        date.mCalibration->mCurve.clear();
                        date.calibrate(s, mProject);
                        ++position;
                        progress->setValue(position);
                    }
            }

        } else
            s.mStep = defaultVal;
        
        mProject -> setSettings(s);
        MainWindow::getInstance() -> setResultsEnabled(false);
        MainWindow::getInstance() -> setLogEnabled(false);
    }
}

void ModelView::minEditChanging()
{
    //QLocale locale = QLocale();
    //mTmin = DateUtils::convertFromAppSettingsFormat(locale.toDouble(mMinEdit->text()));
    
    mTmin = mMinEdit->text().toDouble();
    setSettingsValid(false);
    showCalibration(false);
}

void ModelView::maxEditChanging()
{
    //QLocale locale =QLocale();
    //mTmax = DateUtils::convertFromAppSettingsFormat(locale.toDouble(mMaxEdit->text()));
    
    mTmax = mMaxEdit->text().toDouble();
    setSettingsValid(false);
    showCalibration(false);
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
    mButApply->setColorState(valid ? Button::eReady : Button::eWarning);
    
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
        
        for (int i=0; i<events.size(); ++i) {
            QJsonObject event = events.at(i).toObject();
            int id = event.value(STATE_ID).toInt();
            QString name = event.value(STATE_NAME).toString();
            
            if (name.contains(search, Qt::CaseInsensitive))
                mSearchIds.push_back(id);

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

#pragma mark Right animation
void ModelView::showProperties()
{
    mButProperties  -> setChecked(true);
    mButImport      -> setChecked(false);
    mButPhasesModel -> setChecked(false);
    mPhasesScene    -> clearSelection();
    slideRightPanel();
}
void ModelView::showImport()
{
    if (mProject->studyPeriodIsValid()) {
        showCalibration(false);
        
        mButProperties  -> setChecked(false);
        mButImport      -> setChecked(true);
        mButPhasesModel -> setChecked(false);
        mPhasesScene    -> clearSelection();
        slideRightPanel();
    } else {
        mButProperties  -> setChecked(true);
        mButImport      -> setChecked(false);
        mButPhasesModel -> setChecked(false);
    }
}
void ModelView::showPhases()
{
    if (mProject->studyPeriodIsValid()) {
        showCalibration(false);
        
        mButProperties->setChecked(false);
        mButImport->setChecked(false);
        mButPhasesModel->setChecked(true);
        slideRightPanel();

    } else {
        mButProperties->setChecked(true);
        mButImport->setChecked(false);
        mButPhasesModel->setChecked(false);
    }
}
void ModelView::slideRightPanel()
{
    mAnimationShow->setStartValue(mRightSubHiddenRect);
    mAnimationShow->setEndValue(mRightSubRect);
    
    mAnimationHide->setStartValue(mRightSubRect);
    mAnimationHide->setEndValue(mRightSubHiddenRect);
    
    QWidget* target = 0;
    if (mButImport->isChecked())
        target = mImportDataView;
    else if (mButPhasesModel->isChecked())
        target = mPhasesWrapper;
    else if (mButProperties->isChecked())
        target = mEventPropertiesView;
    
    if ( target && (target != mCurrentRightWidget) ) {
        mCurrentRightWidget = target;
        target->raise();
        mAnimationShow->setTargetObject(target);
        mAnimationHide->start();
    }
    target = 0;
}

void ModelView::prepareNextSlide()
{
    QWidget* target = 0;
    if (mButImport->isChecked())
        target = mImportDataView;
    else if (mButPhasesModel->isChecked())
        target = mPhasesWrapper;
    else if (mButProperties->isChecked())
        target = mEventPropertiesView;
    
    if (target)
        mAnimationHide->setTargetObject(target);

    target = 0;
}

#pragma mark Painting
void ModelView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(mHandlerRect, QColor(50, 50, 50));
    p.fillRect(mRightRect, QColor(50, 50, 50));
}

#pragma mark Layout
void ModelView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}
void ModelView::updateLayout()
{
    qreal x = width() * mSplitProp;

    qreal minLeft = 200;
    qreal minRight = 580 + mHandlerW/2;
    x = (x < minLeft) ? minLeft : x;
    x = (x > width() - minRight) ? width() - minRight : x;
    
    QRectF mEventPropWrapperRectF(x+(mHandlerW)/2,mToolbarH, width()-x-mHandlerW/2, height()-mToolbarH);
    mHandlerRect = QRect(x - mHandlerW/2, 0, mHandlerW, height());
    
    // ----------
    mLeftRect = QRect(0, 0, x - mHandlerW/2, height());
    mLeftHiddenRect = mLeftRect.adjusted(-mLeftRect.width(), 0, -mLeftRect.width(), 0);
    mLeftWrapper->setGeometry(mLeftRect);

    mRightRect = QRect(x + mHandlerW/2, 0, this->width() - x - mHandlerW/2, this->height());
    mRightWrapper->setGeometry(mRightRect);
  
    // ----------

    qreal labelW = 60;
    qreal editW = 100;
    qreal editH = (mToolbarH - 3*mMargin) / 2;

    qreal butW2 = 85;
   
    qreal y = 2*mMargin + editH;
    
    mStudyLab->setGeometry(mMargin, mMargin, 160, editH);
    mButApply->setGeometry(160, mMargin, butW2, editH);
    mButStep->setGeometry(160 + butW2 + mMargin, mMargin, butW2, editH);
    mMinLab ->setGeometry(0, y, labelW, editH);
    mMinEdit->setGeometry(mMargin + labelW, y, editW, editH);
    mMaxLab ->setGeometry(2*mMargin + labelW + editW, y, labelW, editH);
    mMaxEdit->setGeometry(3*mMargin + 2*labelW + editW, y, editW, editH);

    mButProperties ->setGeometry(mRightWrapper->width()- 3*mButtonWidth, 0, mButtonWidth, mToolbarH);
    mButImport     ->setGeometry(mRightWrapper->width()- 2*mButtonWidth, 0, mButtonWidth, mToolbarH);
    mButPhasesModel->setGeometry(mRightWrapper->width()- mButtonWidth, 0, mButtonWidth, mToolbarH);
    
 
    // ----------
    mRightSubRect = QRect(0, mToolbarH, this->width() - x, this->height() - mToolbarH);
    mRightSubHiddenRect = mRightSubRect.adjusted(mRightSubRect.width() + 2*mMargin, 0, mRightSubRect.width() + 2*mMargin, 0);
   
    mEventPropertiesView->setGeometry(mButProperties->isChecked() ? mRightSubRect : mRightSubHiddenRect);
    mEventPropertiesView->setMinimumHeight(mButProperties->isChecked() ? mRightSubRect.height() : mRightSubHiddenRect.height());

    mPhasesWrapper  ->setGeometry(mButPhasesModel->isChecked() ? mRightSubRect : mRightSubHiddenRect);
    mImportDataView ->setGeometry(mButImport     ->isChecked() ? mRightSubRect : mRightSubHiddenRect);

    // ----------
    
    const qreal butH = 60;
    const qreal radarW = 150;
    const qreal radarH = 200;
    const qreal searchH = 30;
    
    mEventsView      ->setGeometry(mButtonWidth, 0, mLeftRect.width() - mButtonWidth, mLeftRect.height());
    mEventsSearchEdit->setGeometry(0, 0, radarW, searchH);
    mEventsGlobalView->setGeometry(0, searchH, radarW, radarH);
    
    mButNewEvent      ->setGeometry(0, 0, mButtonWidth, butH);
    mButNewEventKnown ->setGeometry(0, butH, mButtonWidth, butH);
    mButDeleteEvent   ->setGeometry(0, 2*butH, mButtonWidth, butH);
    mButRecycleEvent  ->setGeometry(0, 3*butH, mButtonWidth, butH);
    mButExportEvents  ->setGeometry(0, 4*butH, mButtonWidth, butH);
    mButEventsOverview->setGeometry(0, 5*butH, mButtonWidth, butH);
    mButEventsGrid    ->setGeometry(0, 6*butH, mButtonWidth, butH);
    mEventsGlobalZoom ->setGeometry(0, 7*butH, mButtonWidth, mLeftRect.height() - 7*butH);
    
    const qreal helpW = qMin(400.0, mEventsView->width() - radarW - mMargin);
    const qreal helpH = mEventsScene->getHelpView()->heightForWidth(helpW);
    mEventsScene->getHelpView()->setGeometry(mEventsView->width() - mMargin - helpW, mMargin, helpW, helpH);

    // ----------
    
    mPhasesView      ->setGeometry(0, 0, mRightSubRect.width() - mButtonWidth, mRightSubRect.height());
    mPhasesGlobalView->setGeometry(0, 0, radarW, radarH);
    
    mButNewPhase      ->setGeometry(mRightSubRect.width() - mButtonWidth, 0, mButtonWidth, butH);
    mButDeletePhase   ->setGeometry(mRightSubRect.width() - mButtonWidth, butH, mButtonWidth, butH);
    mButExportPhases  ->setGeometry(mRightSubRect.width() - mButtonWidth, 2*butH, mButtonWidth, butH);
    mButPhasesOverview->setGeometry(mRightSubRect.width() - mButtonWidth, 3*butH, mButtonWidth, butH);
    mButPhasesGrid    ->setGeometry(mRightSubRect.width() - mButtonWidth, 4*butH, mButtonWidth, butH);
    mPhasesGlobalZoom ->setGeometry(mRightSubRect.width() - mButtonWidth, 5*butH, mButtonWidth, mPhasesWrapper->height() - 5*butH);

    mCalibrationView->setGeometry(mCalibVisible ? mLeftRect : mLeftHiddenRect);
    
    update();
}

#pragma mark Zoom
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

#pragma mark Export images
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

#pragma mark Calibration
void ModelView::updateCalibration(const QJsonObject& date)
{
    // A date has been double-clicked => update CalibrationView only if the date is not null
    if (!date.isEmpty())
        mCalibrationView->setDate(date);

}

void ModelView::showCalibration(bool show)
{
    if ((show && mEventPropertiesView->hasEventWithDates()) || !show) {
        mEventPropertiesView->setCalibChecked(show);
        if (mCalibVisible != show) {
            mCalibVisible = show;
            if (mCalibVisible) {
                mCalibrationView->raise();
                mAnimationCalib->setStartValue(mLeftHiddenRect);
                mAnimationCalib->setEndValue(mLeftRect);
            } else {
                mAnimationCalib->setStartValue(mLeftRect);
                mAnimationCalib->setEndValue(mLeftHiddenRect);
            }
            mAnimationCalib->start();
        }
    }
}

#pragma mark Mouse Events
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
        updateLayout();
    }
}

void ModelView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
        showCalibration(false);
    else if (event->key() == Qt::Key_C)
        showCalibration(!mCalibVisible);
    else
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
    else if(mButPhasesModel->isChecked())
        panelIndex = 3;
    
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
    else if (panelIndex == 3)
        mButPhasesModel->setChecked(true);
    
    mEventsGlobalZoom->setProp(settings.value("events_zoom", 1.).toDouble(), true);
    mPhasesGlobalZoom->setProp(settings.value("phases_zoom", 1.).toDouble(), true);
    
    prepareNextSlide();
    updateLayout();
    
    settings.endGroup();
}

