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

#pragma mark constructor
ModelView::ModelView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mCurSearchIdx(0),
mCurrentRightWidget(0),
mTmin(0),
mTmax(2000),
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
    
   
    // --------
   
    
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
    connect(mEventsScene, SIGNAL(selectionChanged()), mEventsGlobalView, SLOT(update()));
    connect(mButEventsOverview, SIGNAL(toggled(bool)), mEventsGlobalView, SLOT(setVisible(bool)));
    connect(mButEventsOverview, SIGNAL(toggled(bool)), mEventsSearchEdit, SLOT(setVisible(bool)));
    
    connect(mEventsSearchEdit, SIGNAL(returnPressed()), this, SLOT(searchEvent()));
    connect(mEventsGlobalZoom, SIGNAL(valueChanged(double)), this, SLOT(updateEventsZoom(double)));
    connect(mButEventsGrid, SIGNAL(toggled(bool)), mEventsScene, SLOT(showGrid(bool)));
    connect(mButExportEvents, SIGNAL(clicked()), this, SLOT(exportEventsScene()));
    
    // -------- Windows Data Importation --------------------
    
    mImportDataView = new ImportDataView(mRightWrapper);
    connect(mEventsScene, SIGNAL(csvDataLineDropAccepted(QList<int>)), mImportDataView, SLOT(removeCsvRows(QList<int>)));
    
    
    
    // -------- Windows Phase scene ---------------------------
    
    mPhasesWrapper = new QWidget(mRightWrapper);
    
    mPhasesView = new QGraphicsView(mPhasesWrapper);
    mPhasesScene = new PhasesScene(mPhasesView);
    mPhasesView->setScene(mPhasesScene);
    mPhasesView->setInteractive(true);
    mPhasesView->setDragMode(QGraphicsView::RubberBandDrag);
    
    // this signal has already been connected inside the EventsView constructor, to make sure events are marked as selected.
    // Thus, the following connection can make use in updateCheckedPhases of these marks.
    connect(mEventsScene, SIGNAL(selectionChanged()), mPhasesScene, SLOT(updateCheckedPhases()));
    
    mPhasesGlobalView = new SceneGlobalView(mPhasesScene, mPhasesView, mPhasesWrapper);
    mPhasesGlobalView->setVisible(false);
    
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
    mMinLab = new Label(tr("Start") + " :", mRightWrapper);
    mMaxLab = new Label(tr("End")   + " :", mRightWrapper);
    //mStepLab = new Label(tr("Step") + " :", mRightWrapper);
    
   // qreal butW = 80;
 /*   qDebug()<<"ModelView::ModelView mRightWrapper->x()="<<mRightWrapper->x();
    qDebug()<<"ModelView::ModelView mRightWrapper->y()="<<mRightWrapper->y();
    qDebug()<<"ModelView::ModelView mRightWrapper->width()="<<mRightWrapper->width()<<"height "<<mRightWrapper->height();
  */
    
    mEventPropertiesView = new EventPropertiesView(mRightWrapper);
 
    
    connect(mEventPropertiesView, SIGNAL(updateCalibRequested(const QJsonObject&)), this, SLOT(updateCalibration(const QJsonObject&)));
    
    connect(mEventPropertiesView, SIGNAL(showCalibRequested(bool)), this, SLOT(showCalibration(bool)));
    
    // -----------Header on mRightWrapper ------------------
    
    mStudyLab->setLight();
    mMinLab  ->setLight();
    mMaxLab  ->setLight();
    //mStepLab->setLight();
    
    mStudyLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont font;
    font.setPointSizeF(pointSize(13.f));
    mStudyLab->setFont(font);
    QPalette palette = mStudyLab->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    mStudyLab->setPalette(palette);
    
    //mStepLab->setToolTip(prepareTooltipText(tr("Step :"), tr("The step is useful for large study periods.\nFor example with a step of 10 years, calibrated date's values will be stored every 10 years.\nIt lowers memory requirements and graph plots are faster.\nHowever, interpolation between these points\nleads to less precision in calculations.")));
    
    mMinEdit = new LineEdit(mRightWrapper);
    mMinEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mMinEdit->setAlignment(Qt::AlignHCenter);
    //mMinEdit->setFont(font.style(),font.pointSize(),QFont::Bold,font);
    
    mMaxEdit = new LineEdit(mRightWrapper);
    mMaxEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mMaxEdit->setAlignment(Qt::AlignHCenter);
    //mStepEdit = new LineEdit(mRightWrapper);
    
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

void ModelView::doProjectConnections(Project* project)
{
    connect(mButNewEvent, SIGNAL(clicked()), project, SLOT(createEvent()));
    connect(mButNewEventKnown, SIGNAL(clicked()), project, SLOT(createEventKnown()));
    connect(mButDeleteEvent, SIGNAL(clicked()), project, SLOT(deleteSelectedEvents()));
    connect(mButRecycleEvent, SIGNAL(clicked()), project, SLOT(recycleEvents()));
    
    connect(mButNewPhase, SIGNAL(clicked()), project, SLOT(createPhase()));
    connect(mButDeletePhase, SIGNAL(clicked()), project, SLOT(deleteSelectedPhases()));

    connect(project, SIGNAL(selectedEventsChanged()), mPhasesScene, SLOT(updateCheckedPhases()));
    connect(project, SIGNAL(selectedPhasesChanged()), mEventsScene, SLOT(updateSelectedEventsFromPhases()));
    
    connect(project, SIGNAL(currentEventChanged(const QJsonObject&)), mEventPropertiesView, SLOT(setEvent(const QJsonObject&)));
    connect(mEventPropertiesView, SIGNAL(mergeDatesRequested(const int, const QList<int>&)), project, SLOT(mergeDates(const int, const QList<int>&)));
    connect(mEventPropertiesView, SIGNAL(splitDateRequested(const int, const int)), project, SLOT(splitDate(const int, const int)));
    
    connect(project, SIGNAL(eyedPhasesModified(const QMap<int, bool>&)), mEventsScene, SLOT(updateGreyedOutEvents(const QMap<int, bool>&)));
}

void ModelView::resetInterface()
{
    mEventsScene->clean();
    mPhasesScene->clean();
    mCalibrationView->setDate(QJsonObject());
    mEventPropertiesView->setEvent(QJsonObject());
    mButProperties->click();
    updateLayout();
}

void ModelView::updateProject()
{
    mEventPropertiesView->hideCalibration();
    
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    ProjectSettings settings = ProjectSettings::fromJson(state[STATE_SETTINGS].toObject());
   
    mTmin = settings.mTmin;
    mTmax = settings.mTmax;
    //mStepEdit->setText(QString::number(settings.mStep));
    
    // This was used to display study period using the date format defined in application settings :
    //mMinEdit->setText(DateUtils::convertToAppSettingsFormatStr(settings.mTmin));
    //mMaxEdit->setText(DateUtils::convertToAppSettingsFormatStr(settings.mTmax));
    
    mMinEdit->setText(QString::number(settings.mTmin));
    mMaxEdit->setText(QString::number(settings.mTmax));
    
    setSettingsValid(settings.mTmin < settings.mTmax);
    
    mEventsScene -> updateProject();
    mPhasesScene -> updateProject();
    
    // Les sélections dans les scènes doivent être mises à jour après que
    // LES 2 SCENES aient été updatées
    // false : ne pas envoyer de notification pour updater l'état du projet,
    // puisque c'est justement ce que l'on fait ici!
    mEventsScene -> updateSelection(false);
    mPhasesScene -> updateSelection(false);
    
    /** @todo Refresh current date !! */
    
    //mCalibrationView->setDate();
    
    const QJsonObject& event = mEventPropertiesView->getEvent();
    if(!event.isEmpty())
    {
        QJsonArray events = state[STATE_EVENTS].toArray();
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject evt = events[i].toObject();
            if(evt[STATE_ID].toInt() == event[STATE_ID].toInt())
            {
                if(evt != event)
                    mEventPropertiesView->setEvent(evt);
            }
        }
    }
}

void ModelView::applySettings()
{
    mEventPropertiesView->hideCalibration();
    
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    ProjectSettings s = ProjectSettings::fromJson(state[STATE_SETTINGS].toObject());
    ProjectSettings oldSettings = s;
    
    // This was used to display study period using the date format defined in application settings :
    //mTmax = DateUtils::convertFromAppSettingsFormat(mMaxEdit->text().toDouble());
    //mTmin = DateUtils::convertFromAppSettingsFormat(mMinEdit->text().toDouble());
    
    mTmax = mMaxEdit->text().toDouble();
    mTmin = mMinEdit->text().toDouble();
    
    qDebug()<<"ModelView::applySettings()"<<mTmin<<mTmax;
    
    s.mTmin = (int) mTmin;
    s.mTmax = (int) mTmax;
    if(!s.mStepForced)
        s.mStep = ProjectSettings::getStep(s.mTmin, s.mTmax);
    
    if(!project->setSettings(s))
    {
        // Min and max are not consistents :
        // go back to previous values
        
        mTmin = oldSettings.mTmin;
        mTmax = oldSettings.mTmax;
        
        mMinEdit->setText(QString::number(mTmin));
        mMaxEdit->setText(QString::number(mTmax));
        
        if(oldSettings.mTmin < oldSettings.mTmax)
        {
            setSettingsValid(true);
        }
    }
}

void ModelView::adjustStep()
{
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    ProjectSettings s = ProjectSettings::fromJson(state[STATE_SETTINGS].toObject());
    
    double defaultVal = ProjectSettings::getStep(s.mTmin, s.mTmax);
    
    StepDialog dialog(qApp->activeWindow(), Qt::Sheet);
    dialog.setStep(s.mStep, s.mStepForced, defaultVal);
    
    if(dialog.exec() == QDialog::Accepted)
    {
        s.mStepForced = dialog.forced();
        if(s.mStepForced)
            s.mStep = dialog.step();
        else
            s.mStep = defaultVal;
        
        project -> setSettings(s);
        MainWindow::getInstance() -> setResultsEnabled(false);
        MainWindow::getInstance() -> setLogEnabled(false);
    }
}

/* Original code by HL ignore in 2015/06/04
void ModelView::studyPeriodChanging()
{
    qDebug()<<"ModelView::studyPeriodChanging() avant"<<g_FormatDate<<mTmin<<mTmax;
    mTmin = dateInDouble( mMinEdit->text().toDouble() );
    mTmax = dateInDouble( mMaxEdit->text().toDouble() );
    setSettingsValid(false);
    mEventPropertiesView->hideCalibration();
    qDebug()<<"ModelView::studyPeriodChanging() apres"<<g_FormatDate<<mTmin<<mTmax;
}
*/
void ModelView::minEditChanging()
{
    //QLocale locale = QLocale();
    //mTmin = DateUtils::convertFromAppSettingsFormat(locale.toDouble(mMinEdit->text()));
    
    mTmin = mMinEdit->text().toDouble();
    setSettingsValid(false);
    mEventPropertiesView->hideCalibration();
}

void ModelView::maxEditChanging()
{
    //QLocale locale =QLocale();
    //mTmax = DateUtils::convertFromAppSettingsFormat(locale.toDouble(mMaxEdit->text()));
    
    mTmax = mMaxEdit->text().toDouble();
    setSettingsValid(false);
    mEventPropertiesView->hideCalibration();
}

void ModelView::setSettingsValid(bool valid)
{
    mButApply->setColorState(valid ? Button::eReady : Button::eWarning);
    
    // Important : just disable "run" when typing in tmin and tmax edit boxes
    // Run will be enabled again when validating all dates in the next valid study period
    if(!valid){
        MainWindow::getInstance()->setRunEnabled(false);
    }
}

void ModelView::showHelp(bool show)
{
    mEventsScene->showHelp(show);
}

void ModelView::searchEvent()
{
    QString search = mEventsSearchEdit->text();
    
    if(search.isEmpty())
        return;
        
    // Search text has changed : regenerate corresponding events list
    if(search != mLastSearch){
        mLastSearch = search;
        mSearchIds.clear();
        
        QJsonObject state = MainWindow::getInstance()->getProject()->state();
        QJsonArray events = state[STATE_EVENTS].toArray();
        
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            int id = event[STATE_ID].toInt();
            QString name = event[STATE_NAME].toString();
            
            if(name.contains(search, Qt::CaseInsensitive)){
                mSearchIds.push_back(id);
            }
        }
        mCurSearchIdx = 0;
    }
    else if(mSearchIds.size() > 0){
        mCurSearchIdx += 1;
        if(mCurSearchIdx >= mSearchIds.size())
            mCurSearchIdx = 0;
    }
    
    if(mCurSearchIdx < mSearchIds.size())
        mEventsScene->centerOnEvent(mSearchIds[mCurSearchIdx]);
    
    
    /*QString search = mEventsSearchEdit->text();
    QJsonObject state = MainWindow::getInstance()->getProject()->state();
    QJsonArray events = state[STATE_EVENTS].toArray();

    int foundId;
    QString bestName;
    int bestScore = 99999999;
    
    int counter = 0;
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        int id = event[STATE_ID].toInt();
        QString name = event[STATE_NAME].toString();
        
        int score = compareStrings(std::string(name.toUtf8()), std::string(search.toUtf8()));
        if(abs(score) < bestScore)
        {
            bestScore = abs(score);
            foundId = id;
            bestName = name;
            //qDebug() << name;
        }
        ++counter;
    }
    //qDebug() << "Names tried : " << counter << ", bestScore : " << bestScore;
    mEventsScene->centerOnEvent(foundId);*/
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
    Project* project = MainWindow::getInstance()->getProject();
    if(project->studyPeriodIsValid())
    {
        mEventPropertiesView -> hideCalibration();
        
        mButProperties  -> setChecked(false);
        mButImport      -> setChecked(true);
        mButPhasesModel -> setChecked(false);
        mPhasesScene    -> clearSelection();
        slideRightPanel();
    }
    else
    {
        mButProperties  -> setChecked(true);
        mButImport      -> setChecked(false);
        mButPhasesModel -> setChecked(false);
    }
}
void ModelView::showPhases()
{
    Project* project = MainWindow::getInstance()->getProject();
    if(project->studyPeriodIsValid())
    {
        mEventPropertiesView->hideCalibration();
        
        mButProperties->setChecked(false);
        mButImport->setChecked(false);
        mButPhasesModel->setChecked(true);
        slideRightPanel();
    }
    else
    {
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
    if(mButImport->isChecked())
        target = mImportDataView;
    else if(mButPhasesModel->isChecked())
        target = mPhasesWrapper;
    else if(mButProperties->isChecked())
        target = mEventPropertiesView;
    
    if(target != mCurrentRightWidget)
    {
        mCurrentRightWidget = target;
        target->raise();
        mAnimationShow->setTargetObject(target);
        mAnimationHide->start();
    }
}

void ModelView::prepareNextSlide()
{
    QWidget* target = 0;
    if(mButImport->isChecked())
        target = mImportDataView;
    else if(mButPhasesModel->isChecked())
        target = mPhasesWrapper;
    else if(mButProperties->isChecked())
        target = mEventPropertiesView;
    
    if(target)
    {
        mAnimationHide->setTargetObject(target);
    }
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

    //qreal butW = mButtonWidth;
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
    //mRightSubRect = mRightWrapper->rect().adjusted(0, mButPhasesModel->y()+mButPhasesModel->height(), 0, mRightWrapper->rect().height()-mButPhasesModel->y()-mButPhasesModel->height());
    
    //mRightSubRect=QRect(0, mButPhasesModel->y()+mButPhasesModel->height(), 0, mRightWrapper->rect().height()-mButPhasesModel->y()-mButPhasesModel->height());
    //qDebug() << "ModelView::updateLayout() mRightSubRect"  << mRightSubRect;
    
    
    //mRightWrapper->setGeometry(mEventPropWrapperRectF.toRect());
    
    
   //mEventPropertiesView->setGeometry(mButProperties->isChecked() ? mEventPropWrapperRectF.toRect() : mRightSubHiddenRect);
    
    mRightSubRect=QRect(0, mToolbarH, this->width()-x, this->height()-mToolbarH);
    mRightSubHiddenRect = mRightSubRect.adjusted(mRightSubRect.width() + 2*mMargin, 0, mRightSubRect.width() + 2*mMargin, 0);
   
    mEventPropertiesView->setGeometry(mButProperties->isChecked() ? mRightSubRect : mRightSubHiddenRect);
   
    

    mPhasesWrapper  ->setGeometry(mButPhasesModel->isChecked() ? mRightSubRect : mRightSubHiddenRect);
    mImportDataView ->setGeometry(mButImport     ->isChecked() ? mRightSubRect : mRightSubHiddenRect);

    // ----------
    
    qreal butH = 60;
    qreal radarW = 150;
    qreal radarH = 200;
    qreal searchH = 30;
    
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
    
    qreal helpW = qMin(400.0, mEventsView->width() - radarW - mMargin);
    qreal helpH = mEventsScene->getHelpView()->heightForWidth(helpW);
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
void ModelView::updateEventsZoom(double prop)
{
    qreal scale = prop;
    QMatrix matrix;
    matrix.scale(scale, scale);
    mEventsView->setMatrix(matrix);
    mEventsScene->adaptItemsForZoom(scale);
}

void ModelView::updatePhasesZoom(double prop)
{
    qreal scale = prop;
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
    if(fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
}

#pragma mark Toggle Calibration
void ModelView::updateCalibration(const QJsonObject& date)
{
    if(!date.isEmpty())// && date[STATE_DATE_VALID].toBool()==true)
    {
        mCalibrationView->setDate(date);
    }
}

void ModelView::showCalibration(bool show)
{
    if(mCalibVisible != show)
    {
        mCalibVisible = show;
        if(mCalibVisible)
        {
            mCalibrationView->raise();
            mAnimationCalib->setStartValue(mLeftHiddenRect);
            mAnimationCalib->setEndValue(mLeftRect);
            
        }
        else
        {
            mAnimationCalib->setStartValue(mLeftRect);
            mAnimationCalib->setEndValue(mLeftHiddenRect);
        }
        mAnimationCalib->start();
    }
}

#pragma mark Mouse Events
void ModelView::mousePressEvent(QMouseEvent* e)
{
    if(mHandlerRect.contains(e->pos()))
        mIsSplitting = true;
}
void ModelView::mouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    mIsSplitting = false;
}
void ModelView::mouseMoveEvent(QMouseEvent* e)
{
    if(mIsSplitting)
    {
        mSplitProp = (double)e->pos().x() / (double)width();
        updateLayout();
    }
}

void ModelView::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Escape)
    {
        mEventPropertiesView->hideCalibration();
    }
    else
        event->ignore();
}



void ModelView::writeSettings()
{
    QSettings settings;
    settings.beginGroup("ModelView");
    
    int panelIndex = 0;
    if(mButProperties->isChecked()) panelIndex = 1;
    else if(mButImport->isChecked()) panelIndex = 2;
    else if(mButPhasesModel->isChecked()) panelIndex = 3;
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
    
    if(panelIndex == 1) mButProperties->setChecked(true);
    else if(panelIndex == 2) mButImport->setChecked(true);
    else if(panelIndex == 3) mButPhasesModel->setChecked(true);
    
    mEventsGlobalZoom->setProp(settings.value("events_zoom", 1.f).toDouble(), true);
    mPhasesGlobalZoom->setProp(settings.value("phases_zoom", 1.f).toDouble(), true);
    
    prepareNextSlide();
    updateLayout();
    
    settings.endGroup();
}

