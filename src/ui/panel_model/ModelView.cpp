#include "ModelView.h"
#include "EventsScene.h"
#include "PhasesScene.h"
#include "ProjectManager.h"
#include "Project.h"
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
#include <QtWidgets>
#include <QtSvg>
#include <QPropertyAnimation>


ModelView::ModelView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mMargin(5),
mToolbarH(70),
mRightW(500),
mHandlerW(20),
mIsSplitting(false)
{
    setMouseTracking(true);
    
    // --------
    
    Project* project = ProjectManager::getProject();
    connect(project, SIGNAL(currentDateChanged(const QJsonObject&)), this, SLOT(showCalibration(const QJsonObject&)));
    
    // --------
    
    mMinLab = new Label(tr("Start") + " :", this);
    mMaxLab = new Label(tr("End") + " :", this);
    mStepLab = new Label(tr("Step") + " :", this);
    
    mMinLab->setLight();
    mMaxLab->setLight();
    mStepLab->setLight();
    
    mMinEdit = new LineEdit(this);
    mMaxEdit = new LineEdit(this);
    mStepEdit = new LineEdit(this);
    mButApply = new Button(tr("Apply"), this);
    
    QIntValidator* validator = new QIntValidator();
    mMinEdit->setValidator(validator);
    mMaxEdit->setValidator(validator);
    mStepEdit->setValidator(validator);
    
    mButEvents = new Button(tr("Events"), this);
    mButEvents->setIcon(QIcon(":model_w.png"));
    mButEvents->setCheckable(true);
    mButEvents->setChecked(true);
    mButEvents->setEnabled(false);
    
    connect(mButEvents, SIGNAL(toggled(bool)), this, SLOT(showEvents(bool)));
    connect(mButApply, SIGNAL(clicked()), this, SLOT(applySettings()));
    
    // --------
    
    mButImport = new Button(tr("Import Data"), this);
    mButImport->setCheckable(true);
    mButImport->setAutoExclusive(true);
    //mButImport->setFlat(true);
    mButImport->setIcon(QIcon(":import_w.png"));
    //mButImport->setChecked(true);
    
    mButPhasesModel = new Button(tr("Phases"), this);
    mButPhasesModel->setCheckable(true);
    mButPhasesModel->setAutoExclusive(true);
    //mButPhasesModel->setFlat(true);
    mButPhasesModel->setIcon(QIcon(":model_w.png"));
    
    mButProperties = new Button(tr("Event"), this);
    mButProperties->setCheckable(true);
    mButProperties->setAutoExclusive(true);
    //mButProperties->setFlat(true);
    mButProperties->setIcon(QIcon(":e_w.png"));
    mButProperties->setChecked(true);
    
    connect(mButImport, SIGNAL(clicked()), this, SLOT(slideRightPanel()));
    connect(mButPhasesModel, SIGNAL(clicked()), this, SLOT(slideRightPanel()));
    connect(mButProperties, SIGNAL(clicked()), this, SLOT(slideRightPanel()));
    
    // --------
    
    mLeftWrapper = new QWidget(this);
    mRightWrapper = new QWidget(this);
    
    // --------
    
    mEventsWrapper = new QWidget(mLeftWrapper);
    
    mEventsView = new QGraphicsView(mEventsWrapper);
    mEventsScene = new EventsScene(mEventsView);
    mEventsView->setScene(mEventsScene);
    mEventsView->setAcceptDrops(true);
    mEventsView->setInteractive(true);
    mEventsView->setDragMode(QGraphicsView::RubberBandDrag);
    
    mEventsGlobalView = new SceneGlobalView(mEventsScene, mEventsView, mEventsWrapper);
    mEventsGlobalZoom = new ScrollCompressor(mEventsWrapper);
    mEventsGlobalZoom->setProp(1);
    mEventsGlobalZoom->showText(tr("Zoom"), true);
    
    mEventsButExportPNG = new Button(tr("PNG"), mEventsWrapper);
    //mEventsButExportPNG->setFlat(true);
    
    mEventsButExportSVG = new Button(tr("SVG"), mEventsWrapper);
    //mEventsButExportSVG->setFlat(true);
    
    connect(mEventsGlobalZoom, SIGNAL(valueChanged(float)), this, SLOT(updateEventsZoom(float)));
    connect(mEventsButExportPNG, SIGNAL(clicked()), this, SLOT(exportEventsScenePNG()));
    connect(mEventsButExportSVG, SIGNAL(clicked()), this, SLOT(exportEventsSceneSVG()));
    
    // just to refresh when selection changes :
    connect(mEventsScene, SIGNAL(selectionChanged()), mEventsGlobalView, SLOT(update()));
    connect(mEventsScene, SIGNAL(eventDoubleClicked(Event*)), mButProperties, SLOT(click()));
    
    mButNewEvent = new Button(tr("New Event"), mEventsWrapper);
    mButNewEvent->setIcon(QIcon(":new_w.png"));
    //mButNewEvent->setFlat(true);
    
    mButNewEventKnown = new Button(tr("New Bound"), mEventsWrapper);
    mButNewEventKnown->setIcon(QIcon(":bound_w.png"));
    //mButNewEventKnown->setFlat(true);
    
    mButDeleteEvent = new Button(tr("Delete"), mEventsWrapper);
    mButDeleteEvent->setIcon(QIcon(":trash_w.png"));
    //mButDeleteEvent->setFlat(true);
    
    mButRecycleEvent = new Button(tr("Restore"), mEventsWrapper);
    mButRecycleEvent->setIcon(QIcon(":recycle_w.png"));
    //mButRecycleEvent->setFlat(true);
    
    connect(mButNewEvent, SIGNAL(clicked()), project, SLOT(createEvent()));
    connect(mButNewEventKnown, SIGNAL(clicked()), project, SLOT(createEventKnown()));
    connect(mButDeleteEvent, SIGNAL(clicked()), project, SLOT(deleteSelectedEvents()));
    connect(mButRecycleEvent, SIGNAL(clicked()), project, SLOT(recycleEvents()));
    
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
    mPhasesGlobalZoom = new ScrollCompressor(mPhasesWrapper);
    mPhasesGlobalZoom->setProp(1);
    
    mPhasesButExportPNG = new Button(tr("PNG"), mPhasesWrapper);
    mPhasesButExportPNG->setFlat(true);
    
    mPhasesButExportSVG = new Button(tr("SVG"), mPhasesWrapper);
    mPhasesButExportSVG->setFlat(true);
    
    connect(mPhasesGlobalZoom, SIGNAL(valueChanged(float)), this, SLOT(updatePhasesZoom(float)));
    connect(mPhasesButExportPNG, SIGNAL(clicked()), this, SLOT(exportPhasesScenePNG()));
    connect(mPhasesButExportSVG, SIGNAL(clicked()), this, SLOT(exportPhasesSceneSVG()));
    
    mButNewPhase = new Button(tr("New Phase"), mPhasesView);
    mButNewPhase->setIcon(QIcon(":new_w.png"));
    
    mButDeletePhase = new Button(tr("Delete Phase"), mPhasesView);
    mButDeletePhase->setIcon(QIcon(":trash_w"));
    
    connect(mButNewPhase, SIGNAL(clicked()), project, SLOT(createPhase()));
    connect(mButDeletePhase, SIGNAL(clicked()), project, SLOT(deleteCurrentPhase()));
    
    // --------
    
    mImportDataView = new ImportDataView(mRightWrapper);
    connect(mEventsScene, SIGNAL(csvDataLineDropAccepted(QList<int>)), mImportDataView, SLOT(removeCsvRows(QList<int>)));
    
    mEventPropertiesView = new EventPropertiesView(mRightWrapper);
    
    // -------------
    
    mCalibrationView = new CalibrationView(this);
    mAnimationCalib = new QPropertyAnimation();
    mAnimationCalib->setPropertyName("geometry");
    mAnimationCalib->setTargetObject(mCalibrationView);
    mAnimationCalib->setDuration(400);
    mAnimationCalib->setEasingCurve(QEasingCurve::OutCubic);
    
    // --------
    
    mAnimationHide = new QPropertyAnimation();
    mAnimationHide->setPropertyName("geometry");
    mAnimationHide->setDuration(200);
    mAnimationHide->setTargetObject(mEventPropertiesView);
    //mAnimationHide->setTargetObject(mImportDataView);
    mAnimationHide->setEasingCurve(QEasingCurve::Linear);
    
    mAnimationShow = new QPropertyAnimation();
    mAnimationShow->setPropertyName("geometry");
    mAnimationShow->setDuration(200);
    mAnimationShow->setEasingCurve(QEasingCurve::OutCubic);
    
    connect(mAnimationHide, SIGNAL(finished()), mAnimationShow, SLOT(start()));
    connect(mAnimationShow, SIGNAL(finished()), this, SLOT(prepareNextSlide()));
}

ModelView::~ModelView()
{
    
}

void ModelView::updateProject()
{
    Project* project = ProjectManager::getProject();
    QJsonObject state = project->state();
    ProjectSettings settings = ProjectSettings::fromJson(state[STATE_SETTINGS].toObject());
    
    mMinEdit->setText(QString::number(settings.mTmin));
    mMaxEdit->setText(QString::number(settings.mTmax));
    mStepEdit->setText(QString::number(settings.mStep));
    
    mEventsScene->updateProject();
    
    // TODO : refresh current date !!
    //mCalibrationView->setDate();
    
    const QJsonObject& event = mEventPropertiesView->getEvent();
    if(!event.isEmpty())
    {
        QJsonArray events = state[STATE_EVENTS].toArray();
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject evt = events[i].toObject();
            if(evt[STATE_EVENT_ID].toInt() == event[STATE_EVENT_ID].toInt())
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
    
    Project* project = ProjectManager::getProject();
    project->setSettings(settings);
}

void ModelView::slideRightPanel()
{
    mButEvents->setChecked(true);
    
    int m = mMargin;
    
    QRect hiddenRect(width(), mToolbarH + m, mRightW - m, height() - mToolbarH - 2*m);
    QRect visibleRect = hiddenRect.adjusted(-mRightW, 0, -mRightW, 0);
    
    mAnimationShow->setStartValue(hiddenRect);
    mAnimationShow->setEndValue(visibleRect);
    
    mAnimationHide->setStartValue(visibleRect);
    mAnimationHide->setEndValue(hiddenRect);
    
    QWidget* target = 0;
    if(mButImport->isChecked())
        target = mImportDataView;
    else if(mButPhasesModel->isChecked())
        target = mPhasesView;
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
        target = mPhasesView;
    else if(mButProperties->isChecked())
        target = mEventPropertiesView;
    
    if(target)
    {
        mAnimationHide->setTargetObject(target);
    }
}


void ModelView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    p.fillRect(rect(), QColor(180, 180, 180));
    p.fillRect(mToolBarRect, QColor(100, 100, 100));
    
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(100, 100, 100));
    QRect handlerRect(width() - mRightW - mHandlerW , mToolbarH + (height()-mToolbarH-mHandlerW)/2, mHandlerW, mHandlerW);
    p.drawEllipse(handlerRect.adjusted(3, 3, -3, -3));
}

void ModelView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}
void ModelView::updateLayout()
{
    int m = mMargin;
    int editLabelW = 50;
    int editW = 50;
    int editH = (mToolbarH - 4*m) / 3;
    int butW = 80;
    //int tabW = 100;
    
    mToolBarRect = QRect(0, 0, width(), mToolbarH);

    mMinLab->setGeometry(m, m, editLabelW, editH);
    mMaxLab->setGeometry(m, 2*m + editH, editLabelW, editH);
    mStepLab->setGeometry(m, 3*m + 2*editH, editLabelW, editH);
    
    mMinEdit->setGeometry(2*m + editLabelW, m, editW, editH);
    mMaxEdit->setGeometry(2*m + editLabelW, 2*m + editH, editW, editH);
    mStepEdit->setGeometry(2*m + editLabelW, 3*m + 2*editH, editW, editH);
    mButApply->setGeometry(3*m + editLabelW + editW, m, butW, mToolbarH - 2*m);
    mButEvents->setGeometry(4*m + editLabelW + editW + butW, m, butW, mToolbarH - 2*m);
    
    /*mButProperties->setGeometry(width() - 3*tabW, 0, tabW, mToolbarH);
    mButImport->setGeometry(width() - 2*tabW, 0, tabW, mToolbarH);
    mButPhasesModel->setGeometry(width() - 1*tabW, 0, tabW, mToolbarH);*/
    
    mButProperties->setGeometry(width() - 3*(butW + m), m, butW, mToolbarH - 2*m);
    mButImport->setGeometry(width() - 2*(butW + m), m, butW, mToolbarH - 2*m);
    mButPhasesModel->setGeometry(width() - 1*(butW + m), m, butW, mToolbarH - 2*m);
    
    mLeftRect = QRect(m, mToolbarH + m, width() - m - mRightW - mHandlerW, height() - mToolbarH - 2*m);
    mLeftWrapper->setGeometry(mLeftRect);
    mEventsWrapper->setGeometry(0, 0, mLeftRect.width(), mLeftRect.height());
    
    QRect rightRect(width() - mRightW, mToolbarH + m, mRightW - m, height() - mToolbarH - 2*m);
    mRightWrapper->setGeometry(rightRect);
    
    QRect rightR(0, 0, rightRect.width(), rightRect.height());
    QRect rightRH(rightRect.width(), 0, rightRect.width(), rightRect.height());
    
    mPhasesWrapper->setGeometry(mButPhasesModel->isChecked() ? rightR : rightRH);
    mImportDataView->setGeometry(mButImport->isChecked() ? rightR : rightRH);
    mEventPropertiesView->setGeometry(mButProperties->isChecked() ? rightR : rightRH);
    
    int radarCtrlW = 60;
    int sceneButW = 70;
    int sceneButH = 50;
    int radarW = 150;
    int radarH = 2*sceneButH + m;
    
    mEventsView->setGeometry(0, 2*(m + sceneButH), mLeftRect.width(), mLeftRect.height() - 2*(m + sceneButH));
    
    mEventsGlobalView->setGeometry(0, 0, radarW, radarH);
    mEventsGlobalZoom->setGeometry(radarW + m, 0, radarCtrlW, radarH);
    
    mButNewEvent->setGeometry(mEventsWrapper->width() - 3*sceneButW - 2*m, 0, sceneButW, sceneButH);
    mButNewEventKnown->setGeometry(mEventsView->width() - 3*sceneButW - 2*m, m + sceneButH, sceneButW, sceneButH);
    mButDeleteEvent->setGeometry(mEventsView->width() - 2*sceneButW - m, 0, sceneButW, sceneButH);
    mButRecycleEvent->setGeometry(mEventsView->width() - 2*sceneButW - m, m + sceneButH, sceneButW, sceneButH);
    mEventsButExportPNG->setGeometry(mEventsView->width() - 1*sceneButW, 0, sceneButW, sceneButH);
    mEventsButExportSVG->setGeometry(mEventsView->width() - 1*sceneButW, m + sceneButH, sceneButW, sceneButH);
    
    mPhasesGlobalView->setGeometry(0, 0, radarW, radarH);
    mPhasesGlobalZoom->setGeometry(radarW, 0, radarCtrlW, radarH - 2*radarCtrlW);
    mPhasesButExportPNG->setGeometry(radarW, radarH - 1*radarCtrlW, radarCtrlW, radarCtrlW);
    mPhasesButExportSVG->setGeometry(radarW, radarH - 2*radarCtrlW, radarCtrlW, radarCtrlW);
    
    mButNewPhase->setGeometry(mPhasesWrapper->width() - sceneButW, 0, sceneButW, sceneButH);
    mButDeletePhase->setGeometry(mPhasesWrapper->width() - sceneButW, m + sceneButH, sceneButW, sceneButH);
    
    mLeftRectHidden = mLeftRect.adjusted(-mLeftRect.width() - m, 0, -mLeftRect.width() - m, 0);
    mCalibrationView->setGeometry(mButEvents->isChecked() ? mLeftRectHidden : mLeftRect);
    
    update();
}

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

void ModelView::exportEventsScenePNG()
{
    exportSceneImage(mEventsScene, false);
}

void ModelView::exportPhasesScenePNG()
{
    exportSceneImage(mPhasesScene, false);
}

void ModelView::exportEventsSceneSVG()
{
    exportSceneImage(mEventsScene, true);
}

void ModelView::exportPhasesSceneSVG()
{
    exportSceneImage(mPhasesScene, true);
}

void ModelView::exportSceneImage(QGraphicsScene* scene, bool asSvg)
{
    QString filter = asSvg ? tr("Scalable Vector Graphics (*.svg)") : tr("Images (*.png)");
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save model as..."),
                                                    ProjectManager::getCurrentPath(),
                                                    filter);
    if(!fileName.isEmpty())
    {
        scene->clearSelection();
        scene->setSceneRect(scene->itemsBoundingRect());
        QRect r = scene->sceneRect().toRect();
        
        if(asSvg)
        {
            QSvgGenerator svgGen;
            svgGen.setFileName(fileName);
            svgGen.setSize(r.size());
            svgGen.setViewBox(QRect(0, 0, r.width(), r.height()));
            QPainter p(&svgGen);
            p.setRenderHint(QPainter::Antialiasing);
            scene->render(&p);
        }
        else
        {
            QImage image(r.size(), QImage::Format_ARGB32);
            image.fill(Qt::transparent);
            QPainter p(&image);
            p.setRenderHint(QPainter::Antialiasing);
            scene->render(&p);
            image.save(fileName, "PNG");
        }
        QFileInfo fileInfo(fileName);
        ProjectManager::setCurrentPath(fileInfo.dir().absolutePath());
        
        
        // Usefull one day ???
        /*QMimeData * d = new QMimeData();
         d->setData("image/svg+xml", b.buffer());
         QApplication::clipboard()->setMimeData(d, QClipboard::Clipboard);*/
    }
}

void ModelView::showCalibration(const QJsonObject& date)
{
    Date d = Date::fromJson(date);
    if(!date.isEmpty())
    {
        mCalibrationView->setDate(date);
        mCalibrationView->raise();
        if(mButEvents->isChecked())
        {
            mButEvents->setChecked(false);
        }
    }
    else
    {
        if(!mButEvents->isChecked())
        {
            mButEvents->setChecked(true);
        }
    }
}

void ModelView::showEvents(bool show)
{
    mButEvents->setEnabled(!show);
    mAnimationCalib->setStartValue(mLeftRectHidden);
    mAnimationCalib->setEndValue(mLeftRect);
    mAnimationCalib->setDirection(show ? QAbstractAnimation::Backward : QAbstractAnimation::Forward);
    qDebug() << mLeftRect;
    mAnimationCalib->start();
}

void ModelView::mousePressEvent(QMouseEvent* e)
{
    QRect handlerRect(width() - mRightW - mHandlerW, mToolbarH, mHandlerW, height() - mToolbarH);
    if(handlerRect.contains(e->pos()))
    {
        mIsSplitting = true;
    }
}
void ModelView::mouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    mIsSplitting = false;
}
void ModelView::mouseMoveEvent(QMouseEvent* e)
{
    /*QRect handlerRect(width() - mRightW - mHandlerW, mToolbarH, mHandlerW, height() - mToolbarH);
    if(handlerRect.contains(e->pos()))
    {
        setCursor(Qt::SplitHCursor);
    }
    else
    {
        setCursor(Qt::ArrowCursor);
    }*/
    
    if(mIsSplitting)
    {
        int leftMinW = 200;
        int maxW = width() - (2*mMargin + mHandlerW + leftMinW);
        
        mRightW = width() - e->pos().x() - mHandlerW / 2;
        mRightW = (mRightW < 0) ? 0 : mRightW;
        mRightW = (mRightW > maxW) ? maxW : mRightW;
        
        updateLayout();
    }
}

void ModelView::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Escape)
    {
        showCalibration(QJsonObject());
    }
    else
        event->ignore();
}
