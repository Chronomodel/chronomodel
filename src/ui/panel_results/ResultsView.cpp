#include "ResultsView.h"
#include "GraphView.h"
#include "GraphViewDate.h"
#include "GraphViewEvent.h"
#include "GraphViewPhase.h"
#include "Ruler.h"
#include "ZoomControls.h"
#include "ResultsControls.h"
#include "ResultsScroller.h"
#include "ResultsMarker.h"

#include "Date.h"
#include "Event.h"
#include "Phase.h"

#include "ProjectManager.h"
#include "Project.h"

#include "Label.h"
#include "Button.h"
#include "LineEdit.h"
#include "GroupBox.h"
#include "CheckBox.h"
#include "RadioButton.h"
#include "Painting.h"

#include <QtWidgets>
#include <iostream>


ResultsView::ResultsView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mMargin(5),
mOptionsW(200),
mLineH(15),
mGraphLeft(150),
mRulerH(40),
mHasPhases(false),
mShowPhasesScene(true)
{
    //Project* project = ProjectManager::getProject();
    //ProjectSettings s = ProjectSettings::fromJson(project->state()[STATE_SETTINGS].toObject());
    
    // -------------
    
    mRuler = new Ruler(this);
    //mRuler->setRange(s.mTmin, s.mTmax);
    mRuler->showControls(false);
    
    mStack = new QStackedWidget(this);
    mResultsScrollerPhases = new ResultsScroller();
    mResultsScrollerEvents = new ResultsScroller();
    mStack->addWidget(mResultsScrollerPhases);
    mStack->addWidget(mResultsScrollerEvents);
    
    mMarker = new ResultsMarker(this);
    
    setMouseTracking(true);
    mStack->setMouseTracking(true);
    mResultsScrollerPhases->setMouseTracking(true);
    mResultsScrollerEvents->setMouseTracking(true);
    
    connect(mRuler, SIGNAL(zoomChanged(float, float)), mResultsScrollerPhases, SLOT(zoom(float, float)));
    connect(mRuler, SIGNAL(zoomChanged(float, float)), mResultsScrollerEvents, SLOT(zoom(float, float)));
    
    // ----------
    
    mPhasesSceneBut = new Button(tr("By phases"), this);
    mPhasesSceneBut->setCheckable(true);
    mPhasesSceneBut->setChecked(true);
    mPhasesSceneBut->setAutoExclusive(true);
    
    mEventsSceneBut = new Button(tr("By events"), this);
    mEventsSceneBut->setCheckable(true);
    mEventsSceneBut->setChecked(false);
    mEventsSceneBut->setAutoExclusive(true);
    
    connect(mPhasesSceneBut, SIGNAL(toggled(bool)), this, SLOT(showPhasesScene(bool)));
    connect(mEventsSceneBut, SIGNAL(toggled(bool)), this, SLOT(showEventsScene(bool)));
    
    // ----------
    
    mZoomInBut = new Button(this);
    mZoomInBut->setIcon(QIcon(":zoom_in_w.png"));
    
    mZoomOutBut = new Button(this);
    mZoomOutBut->setIcon(QIcon(":zoom_out_w.png"));
    
    mZoomDefaultBut = new Button(this);
    mZoomDefaultBut->setIcon(QIcon(":zoom_default_w.png"));
    
    connect(mZoomInBut, SIGNAL(clicked()), mRuler, SLOT(zoomIn()));
    connect(mZoomOutBut, SIGNAL(clicked()), mRuler, SLOT(zoomOut()));
    connect(mZoomDefaultBut, SIGNAL(clicked()), mRuler, SLOT(zoomDefault()));
    
    // ----------
    
    mTypeGroup = new GroupBox(tr("Results to display"), this);
    mHistoRadio = new RadioButton(tr("Histogram"), mTypeGroup);
    mHPDCheck = new CheckBox(tr("HPD") + " :", mTypeGroup);
    mHPDCheck->setChecked(true);
    mHPDEdit = new LineEdit(mTypeGroup);
    mHPDEdit->setText("95");
    mTraceRadio = new RadioButton(tr("Trace"), mTypeGroup);
    mAcceptRadio = new RadioButton(tr("Accept"), mTypeGroup);
    mHistoRadio->setChecked(true);
    
    connect(mHistoRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mHPDCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mHPDEdit, SIGNAL(textChanged(const QString&)), this, SLOT(updateGraphs()));
    connect(mTraceRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mAcceptRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -----------
    
    mChainsGroup = new GroupBox(tr("Chains"), this);
    mAllChainsCheck = new CheckBox(tr("Chains Sum (histos only)"), mChainsGroup);
    mAllChainsCheck->setChecked(true);
    
    connect(mAllChainsCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -----------
    
    mPhasesGroup = new GroupBox(tr("Phases"), this);
    mAlphaCheck = new CheckBox(tr("Start"), mPhasesGroup);
    mBetaCheck = new CheckBox(tr("End"), mPhasesGroup);
    mPredictCheck = new CheckBox(tr("Predict"), mPhasesGroup);
    mAlphaCheck->setChecked(true);
    mBetaCheck->setChecked(true);
    mPredictCheck->setChecked(false);
    
    connect(mAlphaCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mBetaCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mPredictCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -----------
    
    mDataGroup = new GroupBox(tr("Data"), this);
    mDataThetaRadio = new RadioButton(tr("Date"), mDataGroup);
    mDataSigmaRadio = new RadioButton(tr("Variance / event"), mDataGroup);
    mDataDeltaRadio = new RadioButton(tr("Wiggle"), mDataGroup);
    mDataCalibCheck = new CheckBox(tr("Calibration"), mDataGroup);
    mDataThetaRadio->setChecked(true);
    mDataCalibCheck->setChecked(true);
    
    connect(mDataThetaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataCalibCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataSigmaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataDeltaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -------------------------
    
    Project* project = ProjectManager::getProject();
    connect(project, SIGNAL(mcmcFinished(const Model&)), this, SLOT(updateResults(const Model&)));
    //connect(project, SIGNAL(eventPropsUpdated(Event*)), this, SLOT(updateResults()));
    
    mMarker->raise();
}

ResultsView::~ResultsView()
{
    
}

void ResultsView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter p(this);
    
    p.fillRect(0, 0, mGraphLeft, mRulerH, QColor(220, 220, 220));
    p.fillRect(width() - mOptionsW, 0, mOptionsW, mRulerH, QColor(180, 180, 180));
    p.fillRect(width() - mOptionsW, mRulerH, mOptionsW, height() - mRulerH, QColor(220, 220, 220));
}

void ResultsView::mouseMoveEvent(QMouseEvent* e)
{
    int x = e->pos().x() - 5;
    x = (x >= mGraphLeft) ? x : mGraphLeft;
    x = (x <= width() - mOptionsW) ? x : width() - mOptionsW;
    mMarker->setGeometry(x, mMarker->pos().y(), mMarker->width(), mMarker->height());
}

void ResultsView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void ResultsView::updateLayout()
{
    int m = mMargin;
    int boxTitleH = GroupBox::sTitleHeight;
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int dx = mLineH + m;
    
    mPhasesSceneBut->setGeometry(m, m, (mGraphLeft - 3*m)/2, mRulerH - 2*m);
    mEventsSceneBut->setGeometry(2*m + (mGraphLeft - 3*m)/2, m, (mGraphLeft - 3*m)/2, mRulerH - 2*m);
    
    mRuler->setGeometry(mGraphLeft, 0, width() - mGraphLeft - mOptionsW - sbe, mRulerH);
    mStack->setGeometry(0, mRulerH, width() - mOptionsW, height() - mRulerH);
    mMarker->setGeometry(mMarker->pos().x(), sbe, mMarker->thickness(), height() - sbe);
    
    mOptionsRect = QRectF(width() - mOptionsW, 0, mOptionsW, height());
    
    mZoomRect = QRectF(mOptionsRect.x(), 0, mOptionsW, mRulerH);
    int zw = (mZoomRect.width() - 4*m) / 3;
    mZoomInBut->setGeometry(mZoomRect.x() + m, mZoomRect.y() + m, zw, mZoomRect.height() - 2*m);
    mZoomDefaultBut->setGeometry(mZoomRect.x() + 2*m + zw, mZoomRect.y() + m, zw, mZoomRect.height() - 2*m);
    mZoomOutBut->setGeometry(mZoomRect.x() + 3*m + 2*zw, mZoomRect.y() + m, zw, mZoomRect.height() - 2*m);
    
    mTypeGroup->setGeometry(mOptionsRect.x() + m, mZoomRect.y() + mZoomRect.height() + m, mOptionsW - 2*m, boxTitleH + 5*m + 4*mLineH);
    mHistoRadio->setGeometry(m, boxTitleH + m, mTypeGroup->width()-2*m, mLineH);
    mHPDCheck->setGeometry(m + dx, boxTitleH + 2*m + mLineH, mTypeGroup->width()-2*m - dx - (mTypeGroup->width() - 2*m)/2, mLineH);
    mHPDEdit->setGeometry(m + (mTypeGroup->width() - 2*m)/2, boxTitleH + 2*m + mLineH, (mTypeGroup->width() - 2*m)/2, mLineH);
    mTraceRadio->setGeometry(m, boxTitleH + 3*m + 2*mLineH, mTypeGroup->width()-2*m, mLineH);
    mAcceptRadio->setGeometry(m, boxTitleH + 4*m + 3*mLineH, mTypeGroup->width()-2*m, mLineH);
    
    int numChains = mCheckChainChecks.size();
    mChainsGroup->setGeometry(mOptionsRect.x() + m, mTypeGroup->y() + mTypeGroup->height() + m, mOptionsW - 2*m, boxTitleH + (numChains+2)*m + (numChains+1)*mLineH);
    mAllChainsCheck->setGeometry(m, boxTitleH + m, mChainsGroup->width()-2*m, mLineH);
    for(int i=0; i<numChains; ++i)
    {
        mCheckChainChecks[i]->setGeometry(m, boxTitleH + m + (i+1) * (mLineH + m), mChainsGroup->width()-2*m, mLineH);
    }
    
    mPhasesGroup->setGeometry(mOptionsRect.x() + m, mChainsGroup->y() + mChainsGroup->height() + m, mOptionsW - 2*m, boxTitleH + 4*m + 3*mLineH);
    mAlphaCheck->setGeometry(m, boxTitleH + m, mPhasesGroup->width()-2*m, mLineH);
    mBetaCheck->setGeometry(m, boxTitleH + 2*m + mLineH, mPhasesGroup->width()-2*m, mLineH);
    mPredictCheck->setGeometry(m, boxTitleH + 3*m + 2*mLineH, mPhasesGroup->width()-2*m, mLineH);
    
    mDataGroup->setGeometry(mOptionsRect.x() + m, mPhasesGroup->y() + mPhasesGroup->height() + m, mOptionsW - 2*m, boxTitleH + 5*m + 4*mLineH);
    mDataThetaRadio->setGeometry(m, boxTitleH + m, mDataGroup->width() - 2*m, mLineH);
    mDataCalibCheck->setGeometry(m + dx, boxTitleH + 2*m + mLineH, mDataGroup->width() - 2*m - dx, mLineH);
    mDataSigmaRadio->setGeometry(m, boxTitleH + 3*m + 2*mLineH, mDataGroup->width()-2*m, mLineH);
    mDataDeltaRadio->setGeometry(m, boxTitleH + 4*m + 3*mLineH, mDataGroup->width()-2*m, mLineH);
    
    update();
}

void ResultsView::updateGraphs()
{
    Project* project = ProjectManager::getProject();
    ProjectSettings s = ProjectSettings::fromJson(project->state()[STATE_SETTINGS].toObject());
    MCMCSettings mcmc = MCMCSettings::fromJson(project->state()[STATE_MCMC].toObject());
    
    QList<bool> showChainList;
    for(int i=0; i<mCheckChainChecks.size(); ++i)
        showChainList.append(mCheckChainChecks[i]->isChecked());
    
    if(mHistoRadio->isChecked())
    {
        int min = s.mTmin;
        int max = s.mTmax;
        mRuler->setRange(min, max);
        
        mResultsScrollerPhases->showPhasesHistos(mAlphaCheck->isChecked(),
                                                 mBetaCheck->isChecked(),
                                                 mPredictCheck->isChecked(),
                                                 mAllChainsCheck->isChecked(),
                                                 showChainList,
                                                 mHPDCheck->isChecked(),
                                                 mHPDEdit->text().toInt());
        
        mResultsScrollerPhases->showEventsHistos(mAllChainsCheck->isChecked(),
                                                 showChainList,
                                                 mHPDCheck->isChecked(),
                                                 mHPDEdit->text().toInt());
        
        mResultsScrollerPhases->showDataHistos(mDataThetaRadio->isChecked(),
                                               mDataSigmaRadio->isChecked(),
                                               mDataDeltaRadio->isChecked(),
                                               mDataCalibCheck->isChecked(),
                                               mAllChainsCheck->isChecked(),
                                               showChainList,
                                               mHPDCheck->isChecked(),
                                               mHPDEdit->text().toInt());
        
        mResultsScrollerEvents->showEventsHistos(mAllChainsCheck->isChecked(),
                                                 showChainList,
                                                 mHPDCheck->isChecked(),
                                                 mHPDEdit->text().toInt());
        
        mResultsScrollerEvents->showDataHistos(mDataThetaRadio->isChecked(),
                                               mDataSigmaRadio->isChecked(),
                                               mDataDeltaRadio->isChecked(),
                                               mDataCalibCheck->isChecked(),
                                               mAllChainsCheck->isChecked(),
                                               showChainList,
                                               mHPDCheck->isChecked(),
                                               mHPDEdit->text().toInt());
    }
    else if(mTraceRadio->isChecked())
    {
        int min = 0;
        int max = mcmc.mNumBurnIter + mcmc.mFinalBatchIndex * mcmc.mIterPerBatch + mcmc.mNumRunIter;
        
        mRuler->setRange(min, max);
        
        mResultsScrollerPhases->showPhasesTraces(mAlphaCheck->isChecked(),
                                                 mBetaCheck->isChecked(),
                                                 mPredictCheck->isChecked(),
                                                 showChainList);
        
        mResultsScrollerPhases->showEventsTraces(showChainList);
        
        mResultsScrollerPhases->showDataTraces(mDataThetaRadio->isChecked(),
                                               mDataSigmaRadio->isChecked(),
                                               mDataDeltaRadio->isChecked(),
                                               showChainList);
        
        mResultsScrollerEvents->showEventsTraces(showChainList);
        
        mResultsScrollerEvents->showDataTraces(mDataThetaRadio->isChecked(),
                                               mDataSigmaRadio->isChecked(),
                                               mDataDeltaRadio->isChecked(),
                                               showChainList);
    }
    else if(mAcceptRadio->isChecked())
    {
        int min = 0;
        int max = mcmc.mNumBurnIter + mcmc.mFinalBatchIndex * mcmc.mIterPerBatch + mcmc.mNumRunIter;
        
        mRuler->setRange(min, max);
        
        mResultsScrollerPhases->showPhasesAccept(mAlphaCheck->isChecked(),
                                                 mBetaCheck->isChecked(),
                                                 mPredictCheck->isChecked(),
                                                 showChainList);
        
        mResultsScrollerPhases->showEventsAccept(showChainList);
        
        mResultsScrollerPhases->showDataAccept(mDataThetaRadio->isChecked(),
                                               mDataSigmaRadio->isChecked(),
                                               mDataDeltaRadio->isChecked(),
                                               showChainList);
        
        mResultsScrollerEvents->showEventsAccept(showChainList);
        
        mResultsScrollerEvents->showDataAccept(mDataThetaRadio->isChecked(),
                                               mDataSigmaRadio->isChecked(),
                                               mDataDeltaRadio->isChecked(),
                                               showChainList);
    }
    update();
}

void ResultsView::updateResults(const Model& model)
{
    mRuler->setRange(model.mSettings.mTmin, model.mSettings.mTmax);
    
    mHasPhases = (model.mPhases.size() > 0);
    
    mEventsSceneBut->setVisible(mHasPhases);
    mPhasesSceneBut->setVisible(mHasPhases);
    
    for(int i=mCheckChainChecks.size()-1; i>=0; --i)
    {
        CheckBox* check = mCheckChainChecks.takeAt(i);
        disconnect(check, SIGNAL(clicked()), this, SLOT(updateGraphs()));
        check->setParent(0);
        delete check;
    }
    mCheckChainChecks.clear();
    
    for(int i=0; i<(int)model.mMCMCSettings.mNumProcesses; ++i)
    {
        CheckBox* check = new CheckBox(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
        connect(check, SIGNAL(clicked()), this, SLOT(updateGraphs()));
        mCheckChainChecks.append(check);
    }
    
    // ----------------------------------------------------
    //  Phases View
    // ----------------------------------------------------
    mResultsScrollerPhases->clear();
    
    for(int p=0; p<(int)model.mPhases.size(); ++p)
    {
        Phase* phase = (Phase*)&model.mPhases[p];
        GraphViewPhase* graphPhase = new GraphViewPhase();
        graphPhase->setPhase(phase);
        graphPhase->showUnfold(true, tr("Show Events"));
        mResultsScrollerPhases->addElement(graphPhase);
        
        for(int i=0; i<(int)phase->mEvents.size(); ++i)
        {
            Event* event = (Event*)&model.mEvents[i];
            GraphViewEvent* graphEvent = new GraphViewEvent();
            graphEvent->setEvent(event);
            graphEvent->setVisibility(false);
            graphEvent->setParentGraph(graphPhase);
            graphEvent->showUnfold(true, tr("Show Data"));
            mResultsScrollerPhases->addElement(graphEvent);
            
            for(int j=0; j<(int)event->mDates.size(); ++j)
            {
                Date& date = event->mDates[j];
                GraphViewDate* graphDate = new GraphViewDate();
                graphDate->setDate(&date);
                graphDate->setVisibility(false);
                graphDate->setParentGraph(graphEvent);
                mResultsScrollerPhases->addElement(graphDate);
            }
        }
    }
    
    // ----------------------------------------------------
    //  Events View
    // ----------------------------------------------------
    mResultsScrollerEvents->clear();
    
    for(int i=0; i<(int)model.mEvents.size(); ++i)
    {
        Event* event = (Event*)&model.mEvents[i];
        GraphViewEvent* graphEvent = new GraphViewEvent();
        graphEvent->setEvent(event);
        graphEvent->showUnfold(true, tr("Show Data"));
        mResultsScrollerEvents->addElement(graphEvent);
        
        for(int j=0; j<(int)event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            GraphViewDate* graphDate = new GraphViewDate();
            graphDate->setDate(&date);
            graphDate->setVisibility(false);
            graphDate->setParentGraph(graphEvent);
            mResultsScrollerEvents->addElement(graphDate);
        }
    }
    
    if(mHasPhases && mPhasesSceneBut->isChecked())
    {
        mStack->setCurrentWidget(mResultsScrollerPhases);
    }
    else
    {
        mStack->setCurrentWidget(mResultsScrollerEvents);
    }
    
    updateLayout();
    updateGraphs();
}

void ResultsView::updateOptions()
{
    /*Project* project = ProjectManager::getProject();
    
    double hpdThreshold = mHPDValueEdit->text().toInt() / 100.;
    bool showHPD = mShowHPDCheck->isChecked();
    
    if(showHPD)
    {
        for(int i=0; i<(int)model.mEvents.size(); ++i)
        {
            Event* event = model.mEvents[i];
            event->mTheta.generateHPD(1, hpdThreshold);
            
            for(int j=0; j<(int)event->mDates.size(); ++j)
            {
                Date* date = event->mDates[j];
                date->mTheta.generateHPD(1, hpdThreshold);
                date->mSigma.generateHPD(1, hpdThreshold);
                //date->mDelta.generateHPD(1, hpdThreshold);
            }
        }
        for(int i=0; i<(int)model.mPhases.size(); ++i)
        {
            Phase* phase = model.mPhases[i];
            phase->mAlpha.generateHPD(1, hpdThreshold);
            phase->mBeta.generateHPD(1, hpdThreshold);
            phase->mThetaPredict.generateHPD(1, hpdThreshold);
        }
    }
    
    for(QList<GraphViewDate*>::iterator it = mDatesGraphs.begin(); it != mDatesGraphs.end(); ++it)
    {
        (*it)->refresh(showHPD);
    }
    
    for(QList<GraphViewEvent*>::iterator it = mEventsGraphs.begin(); it != mEventsGraphs.end(); ++it)
    {
        (*it)->refresh(showHPD);
    }
    
    for(QList<GraphViewPhase*>::iterator it = mPhasesGraphs.begin(); it != mPhasesGraphs.end(); ++it)
    {
        (*it)->refresh(showHPD);
    }*/
}

void ResultsView::toggleInfos()
{
    /*bool show = mInfosBut->isChecked();
    
    for(QList<GraphViewPhase*>::iterator it = mPhasesGraphs.begin(); it != mPhasesGraphs.end(); ++it)
        (*it)->showInfos(show);
    
    for(QList<GraphViewEvent*>::iterator it = mEventsGraphs.begin(); it != mEventsGraphs.end(); ++it)
        (*it)->showInfos(show);
    
    for(QList<GraphViewDate*>::iterator it = mDatesGraphs.begin(); it != mDatesGraphs.end(); ++it)
        (*it)->showInfos(show);*/
}

void ResultsView::showPhasesScene(bool)
{
    mStack->setCurrentIndex(0);
}

void ResultsView::showEventsScene(bool)
{
    mStack->setCurrentIndex(1);
}
