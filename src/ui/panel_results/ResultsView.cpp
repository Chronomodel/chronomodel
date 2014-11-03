#include "ResultsView.h"
#include "GraphView.h"
#include "GraphViewDate.h"
#include "GraphViewEvent.h"
#include "GraphViewPhase.h"
#include "Tabs.h"
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
mTabsH(25),
mHasPhases(false),
mShowPhasesScene(true)
{
    //Project* project = ProjectManager::getProject();
    //ProjectSettings s = ProjectSettings::fromJson(project->state()[STATE_SETTINGS].toObject());
    
    // -------------
    
    mTabs = new Tabs(this);
    mTabs->addTab(tr("Results"));
    mTabs->addTab(tr("Traces"));
    mTabs->addTab(tr("Acceptation rate"));
    mTabs->addTab(tr("Auto-correlation"));
    
    connect(mTabs, SIGNAL(tabClicked(int)), this, SLOT(changeTab(int)));
    
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
    mPhasesSceneBut->setFlatHorizontal();
    
    mEventsSceneBut = new Button(tr("By events"), this);
    mEventsSceneBut->setCheckable(true);
    mEventsSceneBut->setChecked(false);
    mEventsSceneBut->setAutoExclusive(true);
    mEventsSceneBut->setFlatHorizontal();
    
    connect(mPhasesSceneBut, SIGNAL(toggled(bool)), this, SLOT(showPhasesScene(bool)));
    connect(mEventsSceneBut, SIGNAL(toggled(bool)), this, SLOT(showEventsScene(bool)));
    
    // ----------
    
    mZoomWidget = new QWidget();
    mZoomWidget->setFixedHeight(mRulerH);
    
    mZoomInBut = new Button(mZoomWidget);
    mZoomInBut->setIcon(QIcon(":zoom_plus.png"));
    mZoomInBut->setFlatHorizontal();
    
    mZoomOutBut = new Button(mZoomWidget);
    mZoomOutBut->setIcon(QIcon(":zoom_minus.png"));
    mZoomOutBut->setFlatHorizontal();
    
    mZoomDefaultBut = new Button(mZoomWidget);
    mZoomDefaultBut->setIcon(QIcon(":zoom_default.png"));
    mZoomDefaultBut->setFlatHorizontal();
    
    connect(mZoomInBut, SIGNAL(clicked()), mRuler, SLOT(zoomIn()));
    connect(mZoomOutBut, SIGNAL(clicked()), mRuler, SLOT(zoomOut()));
    connect(mZoomDefaultBut, SIGNAL(clicked()), mRuler, SLOT(zoomDefault()));
    
    // ----------
    
    mHPDCheck = new CheckBox(tr("HPD") + " :", this);
    mHPDCheck->setChecked(true);
    mHPDEdit = new LineEdit(this);
    mHPDEdit->setText("95");
    
    connect(mHPDCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mHPDEdit, SIGNAL(textChanged(const QString&)), this, SLOT(updateGraphs()));
    
    // -----------
    
    mChainsTitle = new Label(tr("MCMC Chains"));
    mChainsTitle->setIsTitle(true);
    mChainsGroup = new QWidget();
    mAllChainsCheck = new CheckBox(tr("Chains Sum (histos only)"), mChainsGroup);
    mAllChainsCheck->setChecked(true);
    mChainsGroup->setFixedHeight(2*mMargin + 1*mLineH);
    
    connect(mAllChainsCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -----------
    
    mPhasesTitle = new Label(tr("Phases results"));
    mPhasesTitle->setIsTitle(true);
    mPhasesGroup = new QWidget();
    mAlphaCheck = new CheckBox(tr("Start"), mPhasesGroup);
    mBetaCheck = new CheckBox(tr("End"), mPhasesGroup);
    mPredictCheck = new CheckBox(tr("Predict"), mPhasesGroup);
    mAlphaCheck->setChecked(true);
    mBetaCheck->setChecked(true);
    mPredictCheck->setChecked(false);
    mPhasesGroup->setFixedHeight(4*mMargin + 3*mLineH);
    
    connect(mAlphaCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mBetaCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mPredictCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -----------
    
    mDataTitle = new Label(tr("Data results"));
    mDataTitle->setIsTitle(true);
    mDataGroup = new QWidget();
    mDataThetaRadio = new RadioButton(tr("Date"), mDataGroup);
    mDataSigmaRadio = new RadioButton(tr("Variance / event"), mDataGroup);
    mDataDeltaRadio = new RadioButton(tr("Wiggle"), mDataGroup);
    mDataCalibCheck = new CheckBox(tr("Calibration"), mDataGroup);
    mDataThetaRadio->setChecked(true);
    mDataCalibCheck->setChecked(true);
    mDataGroup->setFixedHeight(5*mMargin + 4*mLineH);
    
    connect(mDataThetaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataCalibCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataSigmaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataDeltaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -------------------------
    
    mOptionsWidget = new QWidget(this);
    
    QVBoxLayout* optionsLayout = new QVBoxLayout();
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(0);
    optionsLayout->addWidget(mZoomWidget);
    optionsLayout->addWidget(mChainsTitle);
    optionsLayout->addWidget(mChainsGroup);
    optionsLayout->addWidget(mDataTitle);
    optionsLayout->addWidget(mDataGroup);
    optionsLayout->addWidget(mPhasesTitle);
    optionsLayout->addWidget(mPhasesGroup);
    optionsLayout->addStretch();
    mOptionsWidget->setLayout(optionsLayout);
    
    // -------------------------
    
    Project* project = ProjectManager::getProject();
    connect(project, SIGNAL(mcmcStarted()), this, SLOT(clearResults()));
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
    p.fillRect(width() - mOptionsW, 0, mOptionsW, height() - mRulerH, QColor(220, 220, 220));
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
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int dx = mLineH + m;
    
    mPhasesSceneBut->setGeometry(0, 0, mGraphLeft/2, mRulerH);
    mEventsSceneBut->setGeometry(mGraphLeft/2, 0, mGraphLeft/2, mRulerH);
    
    mTabs->setGeometry(mGraphLeft, 0, width() - mGraphLeft - mOptionsW - sbe, mTabsH);
    mRuler->setGeometry(mGraphLeft, mTabsH, width() - mGraphLeft - mOptionsW - sbe, mRulerH);
    mStack->setGeometry(0, mTabsH + mRulerH, width() - mOptionsW, height() - mRulerH - mTabsH);
    mMarker->setGeometry(mMarker->pos().x(), mTabsH + sbe, mMarker->thickness(), height() - sbe - mTabsH);
    
    mHPDEdit->setGeometry(width() - mOptionsW - sbe - m - 40, m, 40, mLineH);
    mHPDCheck->setGeometry(width() - mOptionsW - sbe - 2*m - 40 - 50, m, 50, mLineH);
    
    mOptionsWidget->setGeometry(width() - mOptionsW, 0, mOptionsW, height());
    
    float zw = mOptionsW / 3;
    float zh = mRulerH;
    mZoomInBut->setGeometry(0, 0, zw, zh);
    mZoomDefaultBut->setGeometry(zw, 0, zw, zh);
    mZoomOutBut->setGeometry(2*zw, 0, zw, zh);
    
    int numChains = mCheckChainChecks.size();
    mChainsGroup->setFixedHeight(m + (numChains+1) * (mLineH + m));
    mAllChainsCheck->setGeometry(m, m, mChainsGroup->width()-2*m, mLineH);
    for(int i=0; i<numChains; ++i)
    {
        mCheckChainChecks[i]->setGeometry(m, m + (i+1) * (mLineH + m), mChainsGroup->width()-2*m, mLineH);
    }
    
    mAlphaCheck->setGeometry(m, m, mPhasesGroup->width()-2*m, mLineH);
    mBetaCheck->setGeometry(m, 2*m + mLineH, mPhasesGroup->width()-2*m, mLineH);
    mPredictCheck->setGeometry(m, 3*m + 2*mLineH, mPhasesGroup->width()-2*m, mLineH);
    
    mDataThetaRadio->setGeometry(m, m, mDataGroup->width() - 2*m, mLineH);
    mDataCalibCheck->setGeometry(m + dx, 2*m + mLineH, mDataGroup->width() - 2*m - dx, mLineH);
    mDataSigmaRadio->setGeometry(m, 3*m + 2*mLineH, mDataGroup->width()-2*m, mLineH);
    mDataDeltaRadio->setGeometry(m, 4*m + 3*mLineH, mDataGroup->width()-2*m, mLineH);
    
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
    
    if(mTabs->currentIndex() == 0)
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
    else if(mTabs->currentIndex() == 1)
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
    else if(mTabs->currentIndex() == 2)
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

void ResultsView::clearResults()
{
    mEventsSceneBut->setVisible(false);
    mPhasesSceneBut->setVisible(false);
    
    for(int i=mCheckChainChecks.size()-1; i>=0; --i)
    {
        CheckBox* check = mCheckChainChecks.takeAt(i);
        disconnect(check, SIGNAL(clicked()), this, SLOT(updateGraphs()));
        check->setParent(0);
        delete check;
    }
    mCheckChainChecks.clear();
    
    mResultsScrollerPhases->clear();
    mResultsScrollerEvents->clear();
}

void ResultsView::updateResults(const Model& model)
{
    clearResults();
    
    mRuler->setRange(model.mSettings.mTmin, model.mSettings.mTmax);
    
    mHasPhases = (model.mPhases.size() > 0);
    
    mEventsSceneBut->setVisible(mHasPhases);
    mPhasesSceneBut->setVisible(mHasPhases);
    
    for(int i=0; i<(int)model.mMCMCSettings.mNumProcesses; ++i)
    {
        CheckBox* check = new CheckBox(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
        connect(check, SIGNAL(clicked()), this, SLOT(updateGraphs()));
        check->setVisible(true);
        mCheckChainChecks.append(check);
    }
    
    // ----------------------------------------------------
    //  Phases View
    // ----------------------------------------------------
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
        mResultsScrollerEvents->setVisible(true);
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
    mPhasesTitle->setVisible(true);
    mPhasesGroup->setVisible(true);
}

void ResultsView::showEventsScene(bool)
{
    mStack->setCurrentIndex(1);
    mPhasesTitle->setVisible(false);
    mPhasesGroup->setVisible(false);
}

void ResultsView::changeTab(int index)
{
    mHPDCheck->setVisible(index == 0);
    mHPDEdit->setVisible(index == 0);
    
    //mChainsGroup->setVisible(true);
    
    updateGraphs();
}
