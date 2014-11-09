#include "ResultsView.h"
#include "GraphView.h"
#include "GraphViewDate.h"
#include "GraphViewEvent.h"
#include "GraphViewPhase.h"
#include "Tabs.h"
#include "Ruler.h"
#include "ZoomControls.h"
#include "Marker.h"
#include "ScrollCompressor.h"

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
mModel(0),
mMargin(5),
mOptionsW(200),
mLineH(15),
mGraphLeft(130),
mRulerH(40),
mTabsH(25),
mGraphsH(100),
mHasPhases(false)
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
    
    mEventsScrollArea = new QScrollArea();
    mEventsScrollArea->setMouseTracking(true);
    mStack->addWidget(mEventsScrollArea);
    
    mPhasesScrollArea = new QScrollArea();
    mPhasesScrollArea->setMouseTracking(true);
    mStack->addWidget(mPhasesScrollArea);
    
    mTimer = new QTimer();
    mTimer->setSingleShot(true);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(updateScrollHeights()));
    
    mMarker = new Marker(this);
    
    setMouseTracking(true);
    mStack->setMouseTracking(true);
    
    connect(mRuler, SIGNAL(zoomChanged(float, float)), this, SLOT(setGraphZoom(float, float)));
    
    // ----------
    
    mByPhasesBut = new Button(tr("By phases"), this);
    mByPhasesBut->setCheckable(true);
    mByPhasesBut->setChecked(true);
    mByPhasesBut->setAutoExclusive(true);
    mByPhasesBut->setFlatHorizontal();
    
    mByEventsBut = new Button(tr("By events"), this);
    mByEventsBut->setCheckable(true);
    mByEventsBut->setChecked(false);
    mByEventsBut->setAutoExclusive(true);
    mByEventsBut->setFlatHorizontal();
    
    connect(mByPhasesBut, SIGNAL(toggled(bool)), this, SLOT(showByPhases(bool)));
    connect(mByEventsBut, SIGNAL(toggled(bool)), this, SLOT(showByEvents(bool)));
    
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
    
    connect(mHPDCheck, SIGNAL(clicked()), this, SLOT(updateHPD()));
    connect(mHPDEdit, SIGNAL(textChanged(const QString&)), this, SLOT(updateHPD()));
    
    // -----------
    
    mChainsTitle = new Label(tr("MCMC Chains"));
    mChainsTitle->setIsTitle(true);
    mChainsGroup = new QWidget();
    mAllChainsCheck = new CheckBox(tr("Chains concatenation"), mChainsGroup);
    mAllChainsCheck->setChecked(true);
    mChainsGroup->setFixedHeight(2*mMargin + mLineH);
    
    connect(mAllChainsCheck, SIGNAL(clicked()), this, SLOT(updateChains()));
    
    // -----------
    
    mPhasesTitle = new Label(tr("Phases results"));
    mPhasesTitle->setIsTitle(true);
    mPhasesGroup = new QWidget();
    mAlphaCheck = new CheckBox(tr("Begin"), mPhasesGroup);
    mBetaCheck = new CheckBox(tr("End"), mPhasesGroup);
    mTauCheck = new CheckBox(tr("Duration"), mPhasesGroup);
    mAlphaCheck->setChecked(true);
    mBetaCheck->setChecked(true);
    mTauCheck->setChecked(false);
    mPhasesGroup->setFixedHeight(4*mMargin + 3*mLineH);
    
    connect(mAlphaCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mBetaCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mTauCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -----------
    
    mDataTitle = new Label(tr("Data results"));
    mDataTitle->setIsTitle(true);
    mDataGroup = new QWidget();
    mDataThetaRadio = new RadioButton(tr("Distribution"), mDataGroup);
    mDataSigmaRadio = new RadioButton(tr("Variance history"), mDataGroup);
    mDataDeltaRadio = new RadioButton(tr("Wiggle maching"), mDataGroup);
    mDataCalibCheck = new CheckBox(tr("Distrib. of calib. dates"), mDataGroup);
    mDataThetaRadio->setChecked(true);
    mDataCalibCheck->setChecked(true);
    mDataGroup->setFixedHeight(5*mMargin + 4*mLineH);
    
    connect(mDataThetaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataCalibCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataSigmaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataDeltaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -------------------------
    
    mDisplayTitle = new Label(tr("Display options"));
    mDisplayTitle->setIsTitle(true);
    
    mCompressor = new ScrollCompressor(this);
    mCompressor->setVertical(false);
    
    mUnfoldBut = new Button(tr("Unfold results"));
    mUnfoldBut->setCheckable(true);
    
    mInfosBut = new Button(tr("Numerical results"));
    mInfosBut->setCheckable(true);
    
    mDisplayWidget = new QWidget();
    QVBoxLayout* displayLayout = new QVBoxLayout();
    displayLayout->setContentsMargins(mMargin, mMargin, mMargin, mMargin);
    displayLayout->setSpacing(mMargin);
    displayLayout->addWidget(mCompressor);
    displayLayout->addWidget(mUnfoldBut);
    displayLayout->addWidget(mInfosBut);
    mDisplayWidget->setLayout(displayLayout);
    
    connect(mCompressor, SIGNAL(valueChanged(float)), this, SLOT(compress(float)));
    connect(mUnfoldBut, SIGNAL(toggled(bool)), this, SLOT(unfoldResults(bool)));
    connect(mInfosBut, SIGNAL(toggled(bool)), this, SLOT(showInfos(bool)));
    
    
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
    optionsLayout->addWidget(mDisplayTitle);
    optionsLayout->addWidget(mDisplayWidget);
    optionsLayout->addStretch();
    mOptionsWidget->setLayout(optionsLayout);
    
    // -------------------------
    
    Project* project = ProjectManager::getProject();
    connect(project, SIGNAL(mcmcStarted()), this, SLOT(clearResults()));
    connect(project, SIGNAL(mcmcFinished(Model*)), this, SLOT(updateResults(Model*)));
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
    p.fillRect(width() - mOptionsW, 0, mOptionsW, height(), QColor(220, 220, 220));
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

QList<QRect> ResultsView::getGeometries(const QList<GraphViewResults*>& graphs, bool open, bool byPhases)
{
    QList<QRect> rects;
    int y = 0;
    int h = mGraphsH;
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    
    for(int i=0; i<graphs.size(); ++i)
    {
        QRect rect = graphs[i]->geometry();
        rect.setY(y);
        rect.setWidth(width() - mOptionsW - sbe);
        
        if(byPhases)
        {
            GraphViewPhase* graph = dynamic_cast<GraphViewPhase*>(graphs[i]);
            rect.setHeight((graph || open) ? h : 0);
        }
        else
        {
            GraphViewEvent* graph = dynamic_cast<GraphViewEvent*>(graphs[i]);
            rect.setHeight((graph || open) ? h : 0);
        }
        y += rect.height();
        rects.append(rect);
    }
    return rects;
}

void ResultsView::updateLayout()
{
    int m = mMargin;
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int dx = mLineH + m;
    int graphYAxis = 50;
    
    mByPhasesBut->setGeometry(0, 0, mGraphLeft/2, mRulerH);
    mByEventsBut->setGeometry(mGraphLeft/2, 0, mGraphLeft/2, mRulerH);
    
    mTabs->setGeometry(mGraphLeft + graphYAxis, 0, width() - mGraphLeft - mOptionsW - sbe - graphYAxis, mTabsH);
    mRuler->setGeometry(mGraphLeft + graphYAxis, mTabsH, width() - mGraphLeft - mOptionsW - sbe - graphYAxis, mRulerH);
    mStack->setGeometry(0, mTabsH + mRulerH, width() - mOptionsW, height() - mRulerH - mTabsH);
    mMarker->setGeometry(mMarker->pos().x(), mTabsH + sbe, mMarker->thickness(), height() - sbe - mTabsH);
    
    if(QWidget* wid = mEventsScrollArea->widget())
    {
        QList<QRect> geometries = getGeometries(mByEventsGraphs, mUnfoldBut->isChecked(), false);
        int h = 0;
        for(int i=0; i<mByEventsGraphs.size(); ++i)
        {
            mByEventsGraphs[i]->setGeometry(geometries[i]);
            h += geometries[i].height();
        }
        wid->setFixedSize(width() - sbe - mOptionsW, h);
        qDebug() << "Graph events viewport : " << wid->geometry();
    }
    
    if(QWidget* wid = mPhasesScrollArea->widget())
    {
        QList<QRect> geometries = getGeometries(mByPhasesGraphs, mUnfoldBut->isChecked(), true);
        int h = 0;
        for(int i=0; i<mByPhasesGraphs.size(); ++i)
        {
            mByPhasesGraphs[i]->setGeometry(geometries[i]);
            h += geometries[i].height();
        }
        wid->setFixedSize(width() - sbe - mOptionsW, h);
        qDebug() << "Graph phases viewport : " << wid->geometry();
    }
    
    mHPDEdit->setGeometry(width() - mOptionsW - sbe - m - 40, m, 40, mLineH);
    mHPDCheck->setGeometry(width() - mOptionsW - sbe - 2*m - 40 - 50, m, 50, mLineH);
    
    mOptionsWidget->setGeometry(width() - mOptionsW, 0, mOptionsW, height());
    
    float zw = mOptionsW / 3;
    float zh = mRulerH;
    mZoomInBut->setGeometry(0, 0, zw, zh);
    mZoomDefaultBut->setGeometry(zw, 0, zw, zh);
    mZoomOutBut->setGeometry(2*zw, 0, zw, zh);
    
    int numChains = mCheckChainChecks.size();
    if(mTabs->currentIndex() == 0)
    {
        mChainsGroup->setFixedHeight(m + (numChains+1) * (mLineH + m));
        mAllChainsCheck->setGeometry(m, m, mChainsGroup->width()-2*m, mLineH);
        for(int i=0; i<numChains; ++i)
            mCheckChainChecks[i]->setGeometry(m, m + (i+1) * (mLineH + m), mChainsGroup->width()-2*m, mLineH);
    }
    else
    {
        mChainsGroup->setFixedHeight(m + numChains * (mLineH + m));
        for(int i=0; i<numChains; ++i)
            mCheckChainChecks[i]->setGeometry(m, m + i * (mLineH + m), mChainsGroup->width()-2*m, mLineH);
    }
    
    mAlphaCheck->setGeometry(m, m, mPhasesGroup->width()-2*m, mLineH);
    mBetaCheck->setGeometry(m, 2*m + mLineH, mPhasesGroup->width()-2*m, mLineH);
    mTauCheck->setGeometry(m, 3*m + 2*mLineH, mPhasesGroup->width()-2*m, mLineH);
    
    mDataThetaRadio->setGeometry(m, m, mDataGroup->width() - 2*m, mLineH);
    mDataCalibCheck->setGeometry(m + dx, 2*m + mLineH, mDataGroup->width() - 2*m - dx, mLineH);
    mDataSigmaRadio->setGeometry(m, 3*m + 2*mLineH, mDataGroup->width()-2*m, mLineH);
    mDataDeltaRadio->setGeometry(m, 4*m + 3*mLineH, mDataGroup->width()-2*m, mLineH);
    
    update();
}

void ResultsView::updateChains()
{
    QList<bool> showChainList;
    for(int i=0; i<mCheckChainChecks.size(); ++i)
        showChainList.append(mCheckChainChecks[i]->isChecked());
    
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i]->updateChains(mAllChainsCheck->isChecked(), showChainList);
    
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i]->updateChains(mAllChainsCheck->isChecked(), showChainList);
}

void ResultsView::updateHPD()
{
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i]->updateHPD(mHPDCheck->isChecked(), mHPDEdit->text().toInt());
    
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i]->updateHPD(mHPDCheck->isChecked(), mHPDEdit->text().toInt());
}

void ResultsView::updateGraphs()
{
    Project* project = ProjectManager::getProject();
    ProjectSettings s = ProjectSettings::fromJson(project->state()[STATE_SETTINGS].toObject());
    MCMCSettings mcmc = MCMCSettings::fromJson(project->state()[STATE_MCMC].toObject());
    
    GraphViewDate::Variable dataVariable;
    if(mDataThetaRadio->isChecked())
        dataVariable = GraphViewDate::eTheta;
    else if(mDataSigmaRadio->isChecked())
        dataVariable = GraphViewDate::eSigma;
    else if(mDataDeltaRadio->isChecked())
        dataVariable = GraphViewDate::eDelta;
    
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
    {
        if(GraphViewPhase* graph = dynamic_cast<GraphViewPhase*>(mByPhasesGraphs[i]))
        {
            graph->setVariablesToShow(mAlphaCheck->isChecked(),
                                      mBetaCheck->isChecked(),
                                      mTauCheck->isChecked());
        }
        else if(GraphViewEvent* graph = dynamic_cast<GraphViewEvent*>(mByPhasesGraphs[i]))
        {
            graph->showVariances(dataVariable == GraphViewDate::eSigma);
        }
        else if(GraphViewDate* graph = dynamic_cast<GraphViewDate*>(mByPhasesGraphs[i]))
        {
            graph->setVariableToShow(dataVariable);
            graph->showCalib(mDataCalibCheck->isChecked());
        }
    }
    for(int i=0; i<mByEventsGraphs.size(); ++i)
    {
        if(GraphViewDate* graph = dynamic_cast<GraphViewDate*>(mByEventsGraphs[i]))
        {
            graph->setVariableToShow(dataVariable);
            graph->showCalib(mDataCalibCheck->isChecked());
        }
        else if(GraphViewEvent* graph = dynamic_cast<GraphViewEvent*>(mByEventsGraphs[i]))
        {
            graph->showVariances(dataVariable == GraphViewDate::eSigma);
        }
    }
    
    if(mTabs->currentIndex() == 0)
    {
        int min = s.mTmin;
        int max = s.mTmax;
        mRuler->setRange(min, max);
        
        for(int i=0; i<mByPhasesGraphs.size(); ++i)
            mByPhasesGraphs[i]->showHisto();
        
        for(int i=0; i<mByEventsGraphs.size(); ++i)
            mByEventsGraphs[i]->showHisto();
    }
    else if(mTabs->currentIndex() == 1)
    {
        int min = 0;
        int max = mcmc.mNumBurnIter + mcmc.mFinalBatchIndex * mcmc.mIterPerBatch + mcmc.mNumRunIter;
        mRuler->setRange(min, max);
        
        for(int i=0; i<mByPhasesGraphs.size(); ++i)
            mByPhasesGraphs[i]->showTrace();
        
        for(int i=0; i<mByEventsGraphs.size(); ++i)
            mByEventsGraphs[i]->showTrace();
    }
    else if(mTabs->currentIndex() == 2)
    {
        int min = 0;
        int max = mcmc.mNumBurnIter + mcmc.mFinalBatchIndex * mcmc.mIterPerBatch + mcmc.mNumRunIter;
        mRuler->setRange(min, max);
        
        for(int i=0; i<mByPhasesGraphs.size(); ++i)
            mByPhasesGraphs[i]->showAccept();
        
        for(int i=0; i<mByEventsGraphs.size(); ++i)
            mByEventsGraphs[i]->showAccept();
    }
    else if(mTabs->currentIndex() == 3)
    {
        /*int min = 0;
        int max = mcmc.mNumBurnIter + mcmc.mFinalBatchIndex * mcmc.mIterPerBatch + mcmc.mNumRunIter;
        mRuler->setRange(min, max);*/
        
        for(int i=0; i<mByPhasesGraphs.size(); ++i)
            mByPhasesGraphs[i]->showCorrel();
        
        for(int i=0; i<mByEventsGraphs.size(); ++i)
            mByEventsGraphs[i]->showCorrel();
    }
    update();
}

void ResultsView::clearResults()
{
    mByEventsBut->setVisible(false);
    mByPhasesBut->setVisible(false);
    
    for(int i=mCheckChainChecks.size()-1; i>=0; --i)
    {
        CheckBox* check = mCheckChainChecks.takeAt(i);
        disconnect(check, SIGNAL(clicked()), this, SLOT(updateChains()));
        check->setParent(0);
        delete check;
    }
    mCheckChainChecks.clear();
    
    for(int i=0; i<mByEventsGraphs.size(); ++i)
    {
        mByEventsGraphs[i]->setParent(0);
        delete mByEventsGraphs[i];
    }
    
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
    {
        mByPhasesGraphs[i]->setParent(0);
        delete mByPhasesGraphs[i];
    }
    
    QWidget* eventsWidget = mEventsScrollArea->takeWidget();
    if(eventsWidget)
        delete eventsWidget;
    
    QWidget* phasesWidget = mPhasesScrollArea->takeWidget();
    if(phasesWidget)
        delete phasesWidget;
}

void ResultsView::updateResults(Model* model)
{
    clearResults();
    
    if(!model)
        return;
    
    mRuler->setRange(model->mSettings.mTmin, model->mSettings.mTmax);
    
    mHasPhases = (model->mPhases.size() > 0);
    
    mByEventsBut->setVisible(mHasPhases);
    mByPhasesBut->setVisible(mHasPhases);
    
    for(int i=0; i<(int)model->mMCMCSettings.mNumChains; ++i)
    {
        CheckBox* check = new CheckBox(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
        connect(check, SIGNAL(clicked()), this, SLOT(updateChains()));
        check->setVisible(true);
        mCheckChainChecks.append(check);
    }
    
    // ----------------------------------------------------
    //  Phases View
    // ----------------------------------------------------
    
    QWidget* phasesWidget = new QWidget();
    phasesWidget->setMouseTracking(true);
    
    for(int p=0; p<(int)model->mPhases.size(); ++p)
    {
        Phase* phase = (Phase*)&model->mPhases[p];
        GraphViewPhase* graphPhase = new GraphViewPhase(phasesWidget);
        graphPhase->setSettings(model->mSettings);
        graphPhase->setPhase(phase);
        mByPhasesGraphs.append(graphPhase);
        
        for(int i=0; i<(int)phase->mEvents.size(); ++i)
        {
            Event* event = (Event*)&model->mEvents[i];
            GraphViewEvent* graphEvent = new GraphViewEvent(phasesWidget);
            graphEvent->setSettings(model->mSettings);
            graphEvent->setEvent(event);
            mByPhasesGraphs.append(graphEvent);
            
            for(int j=0; j<(int)event->mDates.size(); ++j)
            {
                Date& date = event->mDates[j];
                GraphViewDate* graphDate = new GraphViewDate(phasesWidget);
                graphDate->setSettings(model->mSettings);
                graphDate->setDate(&date);
                graphDate->setColor(event->mColor);
                mByPhasesGraphs.append(graphDate);
            }
        }
    }
    mPhasesScrollArea->setWidget(phasesWidget);
    
    // ----------------------------------------------------
    //  Events View
    // ----------------------------------------------------
    
    QWidget* eventsWidget = new QWidget();
    eventsWidget->setMouseTracking(true);
    
    for(int i=0; i<(int)model->mEvents.size(); ++i)
    {
        Event* event = (Event*)&model->mEvents[i];
        GraphViewEvent* graphEvent = new GraphViewEvent(eventsWidget);
        graphEvent->setSettings(model->mSettings);
        graphEvent->setEvent(event);
        mByEventsGraphs.append(graphEvent);
        
        for(int j=0; j<(int)event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            GraphViewDate* graphDate = new GraphViewDate(eventsWidget);
            graphDate->setSettings(model->mSettings);
            graphDate->setDate(&date);
            graphDate->setColor(event->mColor);
            mByEventsGraphs.append(graphDate);
        }
    }
    mEventsScrollArea->setWidget(eventsWidget);
    
    
    if(mHasPhases && mByPhasesBut->isChecked())
        showByPhases(true);
    else
        showByEvents(true);
    
    updateHPD();
    updateChains();
    updateLayout();
    updateGraphs();
}

void ResultsView::unfoldResults(bool open)
{
    QList<QRect> geometries = getGeometries(mByEventsGraphs, open, false);
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i]->toggle(geometries[i]);

    geometries = getGeometries(mByPhasesGraphs, open, true);
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i]->toggle(geometries[i]);
    
    if(open)
        updateScrollHeights();
    else
    {
        //mTimer->start(200);
    }
}

void ResultsView::updateScrollHeights()
{
    if(QWidget* wid = mEventsScrollArea->widget())
    {
        QList<QRect> geometries = getGeometries(mByEventsGraphs, mUnfoldBut->isChecked(), false);
        int h = 0;
        for(int i=0; i<geometries.size(); ++i)
            h += geometries[i].height();
        wid->setFixedHeight(h);
        qDebug() << "Graph events viewport : " << wid->geometry();
    }
    if(QWidget* wid = mPhasesScrollArea->widget())
    {
        QList<QRect> geometries = getGeometries(mByPhasesGraphs, mUnfoldBut->isChecked(), true);
        int h = 0;
        for(int i=0; i<geometries.size(); ++i)
            h += geometries[i].height();
        wid->setFixedHeight(h);
        qDebug() << "Graph phases viewport : " << wid->geometry();
    }
}

void ResultsView::showInfos(bool show)
{
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i]->showNumericalValues(show);
    
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i]->showNumericalValues(show);
}

void ResultsView::compress(float prop)
{
    float min = 20;
    float max = 200;
    mGraphsH = min + prop * (max - min);
    updateLayout();
}

void ResultsView::setGraphZoom(float min, float max)
{
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i]->zoom(min, max);
        
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i]->zoom(min, max);
}

void ResultsView::showByPhases(bool)
{
    mStack->setCurrentWidget(mPhasesScrollArea);
    mPhasesTitle->setVisible(true);
    mPhasesGroup->setVisible(true);
}

void ResultsView::showByEvents(bool)
{
    mStack->setCurrentWidget(mEventsScrollArea);
    mPhasesTitle->setVisible(false);
    mPhasesGroup->setVisible(false);
}

void ResultsView::changeTab(int index)
{
    mHPDCheck->setVisible(index == 0);
    mHPDEdit->setVisible(index == 0);
    mAllChainsCheck->setVisible(index == 0);
    
    updateLayout();
    updateGraphs();
}
