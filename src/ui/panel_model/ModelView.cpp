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
#include <QtWidgets>
#include <QtSvg>
#include <QPropertyAnimation>


ModelView::ModelView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mMargin(5),
mToolbarH(60),
mSplitProp(0.6f),
mHandlerW(15),
mIsSplitting(false),
mCalibVisible(false)
{
    setMouseTracking(true);
    
    // --------
    
    mLeftWrapper = new QWidget(this);
    mRightWrapper = new QWidget(this);
    
    // --------
    
    mEventsView = new QGraphicsView(mLeftWrapper);
    mEventsScene = new EventsScene(mEventsView);
    mEventsView->setScene(mEventsScene);
    mEventsView->setAcceptDrops(true);
    mEventsView->setInteractive(true);
    mEventsView->setDragMode(QGraphicsView::RubberBandDrag);
    
    mEventsGlobalView = new SceneGlobalView(mEventsScene, mEventsView, mEventsView);
    mEventsGlobalView->setVisible(false);
    
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
    
    mButExportEvents = new Button(tr("Export"), mLeftWrapper);
    mButExportEvents->setIcon(QIcon(":topng.png"));
    mButExportEvents->setFlatVertical();
    
    mButEventsOverview = new Button(tr("Overview"), mLeftWrapper);
    mButEventsOverview->setIcon(QIcon(":eye_w.png"));
    mButEventsOverview->setCheckable(true);
    mButEventsOverview->setFlatVertical();
    
    mEventsGlobalZoom = new ScrollCompressor(mLeftWrapper);
    mEventsGlobalZoom->setProp(1);
    mEventsGlobalZoom->showText(tr("Zoom"), true);
    
    // just to refresh when selection changes :
    connect(mEventsScene, SIGNAL(selectionChanged()), mEventsGlobalView, SLOT(update()));
    connect(mButEventsOverview, SIGNAL(toggled(bool)), mEventsGlobalView, SLOT(setVisible(bool)));
    
    connect(mEventsGlobalZoom, SIGNAL(valueChanged(float)), this, SLOT(updateEventsZoom(float)));
    connect(mButExportEvents, SIGNAL(clicked()), this, SLOT(exportEventsScene()));
    
    // --------
    
    mImportDataView = new ImportDataView(mRightWrapper);
    connect(mEventsScene, SIGNAL(csvDataLineDropAccepted(QList<int>)), mImportDataView, SLOT(removeCsvRows(QList<int>)));
    
    mEventPropertiesView = new EventPropertiesView(mRightWrapper);
    connect(mEventPropertiesView, SIGNAL(calibRequested(const QJsonObject&)), this, SLOT(updateCalibration(const QJsonObject&)));
    connect(mEventPropertiesView, SIGNAL(calibRequested(const QJsonObject&)), this, SLOT(showCalibration()));
    
    // --------
    
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
    
    mButExportPhases = new Button(tr("Export"), mPhasesWrapper);
    mButExportPhases->setIcon(QIcon(":topng.png"));
    mButExportPhases->setFlatVertical();
    
    mButPhasesOverview = new Button(tr("Overview"), mPhasesWrapper);
    mButPhasesOverview->setIcon(QIcon(":eye_w.png"));
    mButPhasesOverview->setCheckable(true);
    mButPhasesOverview->setFlatVertical();
    
    mPhasesGlobalZoom = new ScrollCompressor(mPhasesWrapper);
    mPhasesGlobalZoom->setProp(1);
    mPhasesGlobalZoom->showText(tr("Zoom"), true);
    
    connect(mPhasesGlobalZoom, SIGNAL(valueChanged(float)), this, SLOT(updatePhasesZoom(float)));
    connect(mButExportPhases, SIGNAL(clicked()), this, SLOT(exportPhasesScene()));
    
    connect(mButPhasesOverview, SIGNAL(toggled(bool)), mPhasesGlobalView, SLOT(setVisible(bool)));
    
    // --------
    
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
    
    // -------------
    
    mCalibrationView = new CalibrationView(this);
    
    mAnimationCalib = new QPropertyAnimation();
    mAnimationCalib->setPropertyName("geometry");
    mAnimationCalib->setTargetObject(mCalibrationView);
    mAnimationCalib->setDuration(400);
    mAnimationCalib->setEasingCurve(QEasingCurve::OutCubic);
    
    mButBackEvents = new Button(tr("Close"), mCalibrationView);
    mButBackEvents->setIsClose(true);
    connect(mButBackEvents, SIGNAL(clicked()), this, SLOT(hideCalibration()));
    
    // --------
    
    mMinLab = new Label(tr("Start") + " :", mRightWrapper);
    mMaxLab = new Label(tr("End") + " :", mRightWrapper);
    mStepLab = new Label(tr("Step") + " :", mRightWrapper);
    
    mMinLab->setLight();
    mMaxLab->setLight();
    mStepLab->setLight();
    
    mMinEdit = new LineEdit(mRightWrapper);
    mMaxEdit = new LineEdit(mRightWrapper);
    mStepEdit = new LineEdit(mRightWrapper);
    mButApply = new Button(tr("Apply"), mRightWrapper);
    
    mButApply->setColorState(Button::eWarning);
    
    QIntValidator* validator = new QIntValidator();
    //mMinEdit->setValidator(validator);
    //mMaxEdit->setValidator(validator);
    
    validator->setBottom(1);
    mStepEdit->setValidator(validator);
    
    connect(mMinEdit, SIGNAL(textChanged(const QString&)), this, SLOT(studyPeriodChanging()));
    connect(mMaxEdit, SIGNAL(textChanged(const QString&)), this, SLOT(studyPeriodChanging()));
    connect(mStepEdit, SIGNAL(textChanged(const QString&)), this, SLOT(studyPeriodChanging()));
    connect(mButApply, SIGNAL(clicked()), this, SLOT(applySettings()));
    
    // --------
    
    mButProperties = new Button(tr("Properties"), mRightWrapper);
    mButProperties->setCheckable(true);
    mButProperties->setAutoExclusive(true);
    mButProperties->setIcon(QIcon(":settings_w.png"));
    mButProperties->setChecked(true);
    mButProperties->setFlatHorizontal();
    
    mButImport = new Button(tr("Import Data"), mRightWrapper);
    mButImport->setCheckable(true);
    mButImport->setAutoExclusive(true);
    mButImport->setIcon(QIcon(":from_csv.png"));
    mButImport->setFlatHorizontal();
    
    mButPhasesModel = new Button(tr("Phases"), mRightWrapper);
    mButPhasesModel->setCheckable(true);
    mButPhasesModel->setAutoExclusive(true);
    mButPhasesModel->setIcon(QIcon(":model_w.png"));
    mButPhasesModel->setFlatHorizontal();
    
    connect(mButProperties, SIGNAL(clicked()), this, SLOT(slideRightPanel()));
    connect(mButImport, SIGNAL(clicked()), this, SLOT(slideRightPanel()));
    connect(mButPhasesModel, SIGNAL(clicked()), this, SLOT(slideRightPanel()));
    
    connect(mEventsScene, SIGNAL(eventDoubleClicked()), mButProperties, SLOT(click()));
}

ModelView::~ModelView()
{
    
}

void ModelView::doProjectConnections(Project* project)
{
    connect(project, SIGNAL(currentDateChanged(const QJsonObject&)), this, SLOT(updateCalibration(const QJsonObject&)));
    
    connect(mButNewEvent, SIGNAL(clicked()), project, SLOT(createEvent()));
    connect(mButNewEventKnown, SIGNAL(clicked()), project, SLOT(createEventKnown()));
    connect(mButDeleteEvent, SIGNAL(clicked()), project, SLOT(deleteSelectedEvents()));
    connect(mButRecycleEvent, SIGNAL(clicked()), project, SLOT(recycleEvents()));
    
    connect(mButNewPhase, SIGNAL(clicked()), project, SLOT(createPhase()));
    connect(mButDeletePhase, SIGNAL(clicked()), project, SLOT(deleteSelectedPhases()));

    connect(project, SIGNAL(selectedEventsChanged()), mPhasesScene, SLOT(updateCheckedPhases()));
    connect(project, SIGNAL(selectedPhasesChanged()), mEventsScene, SLOT(updateSelectedEventsFromPhases()));
    
    connect(project, SIGNAL(currentEventChanged(const QJsonObject&)), mEventPropertiesView, SLOT(setEvent(const QJsonObject&)));
    
    connect(project, SIGNAL(eyedPhasesModified(const QMap<int, bool>&)), mEventsScene, SLOT(updateGreyedOutEvents(const QMap<int, bool>&)));
}

void ModelView::updateProject()
{
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    ProjectSettings settings = ProjectSettings::fromJson(state[STATE_SETTINGS].toObject());
    
    mMinEdit->setText(QString::number(settings.mTmin));
    mMaxEdit->setText(QString::number(settings.mTmax));
    mStepEdit->setText(QString::number(settings.mStep));
    
    mButApply->setColorState(Button::eReady);
    
    mEventsScene->updateProject();
    mPhasesScene->updateProject();
    
    // TODO : refresh current date !!
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
                qDebug() << evt[STATE_EVENT_KNOWN_TYPE].toInt();
                qDebug() << event[STATE_EVENT_KNOWN_TYPE].toInt();
                if(evt != event)
                    mEventPropertiesView->setEvent(evt);
            }
        }
    }
}

void ModelView::applySettings()
{
    ProjectSettings settings;
    
    settings.mTmin = mMinEdit->text().toInt();
    settings.mTmax = mMaxEdit->text().toInt();
    settings.mStep = mStepEdit->text().toInt();
    
    Project* project = MainWindow::getInstance()->getProject();
    project->setSettings(settings);
}

void ModelView::studyPeriodChanging()
{
    mButApply->setColorState(Button::eWarning);
}

void ModelView::showHelp(bool show)
{
    mEventsScene->showHelp(show);
}

#pragma mark Right animation
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
    
    if(target)
    {
        target->raise();
        mAnimationShow->setTargetObject(target);
    }
    mAnimationHide->start();
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
    int m = mMargin;
    
    int x = width() * mSplitProp;
    int minLeft = 200;
    int minRight = 430 + mHandlerW/2;
    x = (x < minLeft) ? minLeft : x;
    x = (x > width() - minRight) ? width() - minRight : x;
    
    mLeftRect = QRect(0, 0, x - mHandlerW/2, height());
    mRightRect = QRect(x + mHandlerW/2, 0, width() - x - mHandlerW/2, height());
    mLeftHiddenRect = mLeftRect.adjusted(-mLeftRect.width(), 0, -mLeftRect.width(), 0);
    mRightSubRect = QRect(0, mToolbarH, mRightRect.width() - m, mRightRect.height() - mToolbarH - m);
    mRightSubHiddenRect = mRightSubRect.adjusted(mRightSubRect.width() + 2*m, 0, mRightSubRect.width() + 2*m, 0);
    mHandlerRect = QRect(x - mHandlerW/2, 0, mHandlerW, height());
    
    // ----------
    
    mLeftWrapper->setGeometry(mLeftRect);
    mRightWrapper->setGeometry(mRightRect);
    
    // ----------

    int editLabelW = 35;
    int editW = 50;
    int editH = (mToolbarH - 3*m) / 2;
    int butW = 80;
    
    mMinLab->setGeometry(0, m, editLabelW, editH);
    mMaxLab->setGeometry(0, 2*m + editH, editLabelW, editH);
    mStepLab->setGeometry(2*m + editLabelW + editW, m, editLabelW, editH);
    
    mMinEdit->setGeometry(m + editLabelW, m, editW, editH);
    mMaxEdit->setGeometry(m + editLabelW, 2*m + editH, editW, editH);
    mStepEdit->setGeometry(3*m + 2*editLabelW + editW, m, editW, editH);
    mButApply->setGeometry(2*m + editLabelW + editW, 2*m + editH, editLabelW + m + editW, editH);
    
    mButProperties->setGeometry(mRightRect.width() - 3*butW, 0, butW, mToolbarH);
    mButImport->setGeometry(mRightRect.width() - 2*butW, 0, butW, mToolbarH);
    mButPhasesModel->setGeometry(mRightRect.width() - butW, 0, butW, mToolbarH);
    
    // ----------
    
    mEventPropertiesView->setGeometry(mButProperties->isChecked() ? mRightSubRect : mRightSubHiddenRect);
    mPhasesWrapper->setGeometry(mButPhasesModel->isChecked() ? mRightSubRect : mRightSubHiddenRect);
    mImportDataView->setGeometry(mButImport->isChecked() ? mRightSubRect : mRightSubHiddenRect);
    
    // ----------
    
    int butH = 60;
    int radarW = 150;
    int radarH = 200;
    
    mEventsView->setGeometry(butW, 0, mLeftRect.width() - butW, mLeftRect.height());
    mEventsGlobalView->setGeometry(0, 0, radarW, radarH);
    
    mButNewEvent->setGeometry(0, 0, butW, butH);
    mButNewEventKnown->setGeometry(0, butH, butW, butH);
    mButDeleteEvent->setGeometry(0, 2*butH, butW, butH);
    mButRecycleEvent->setGeometry(0, 3*butH, butW, butH);
    mButExportEvents->setGeometry(0, 4*butH, butW, butH);
    mButEventsOverview->setGeometry(0, 5*butH, butW, butH);
    mEventsGlobalZoom->setGeometry(0, 6*butH, butW, mLeftRect.height() - 6*butH);
    
    int helpW = qMin(400, mEventsView->width() - radarW - m);
    int helpH = mEventsScene->getHelpView()->heightForWidth(helpW);
    mEventsScene->getHelpView()->setGeometry(mEventsView->width() - m - helpW, m, helpW, helpH);
    
    // ----------
    
    mPhasesView->setGeometry(0, 0, mRightSubRect.width() - butW, mRightSubRect.height());
    mPhasesGlobalView->setGeometry(0, 0, radarW, radarH);
    
    mButNewPhase->setGeometry(mRightSubRect.width() - butW, 0, butW, butH);
    mButDeletePhase->setGeometry(mRightSubRect.width() - butW, butH, butW, butH);
    mButExportPhases->setGeometry(mRightSubRect.width() - butW, 2*butH, butW, butH);
    mButPhasesOverview->setGeometry(mRightSubRect.width() - butW, 3*butH, butW, butH);
    mPhasesGlobalZoom->setGeometry(mRightSubRect.width() - butW, 4*butH, butW, mRightRect.height() - 4*butH);
    
    mCalibrationView->setGeometry(mLeftHiddenRect);
    mButBackEvents->setGeometry(mCalibrationView->width() - 30, 5, 25, 25);
    
    update();
}

#pragma mark Zoom
void ModelView::updateEventsZoom(float prop)
{
    qreal scale = prop;
    QMatrix matrix;
    matrix.scale(scale, scale);
    mEventsView->setMatrix(matrix);
}

void ModelView::updatePhasesZoom(float prop)
{
    qreal scale = prop;
    QMatrix matrix;
    matrix.scale(scale, scale);
    mPhasesView->setMatrix(matrix);
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
                                           MainWindow::getInstance()->getCurrentPath());
    if(fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
    
    // Usefull one day ???
    /*QMimeData * d = new QMimeData();
     d->setData("image/svg+xml", b.buffer());
     QApplication::clipboard()->setMimeData(d, QClipboard::Clipboard);*/
}

#pragma mark Toggle Calibration
void ModelView::updateCalibration(const QJsonObject& date)
{
    Date d = Date::fromJson(date);
    if(!date.isEmpty())
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

void ModelView::hideCalibration()
{
    showCalibration(false);
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
        mSplitProp = (float)e->pos().x() / (float)width();
        updateLayout();
    }
}

void ModelView::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Escape)
        showCalibration(false);
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
    
    mEventsGlobalZoom->setProp(settings.value("events_zoom", 1.f).toFloat(), true);
    mPhasesGlobalZoom->setProp(settings.value("phases_zoom", 1.f).toFloat(), true);
    
    prepareNextSlide();
    updateLayout();
    
    settings.endGroup();
}

