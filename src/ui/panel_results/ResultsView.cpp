#include "ResultsView.h"
#include "GraphView.h"
#include "GraphViewDate.h"
#include "GraphViewEvent.h"
#include "GraphViewPhase.h"
#include "Tabs.h"
#include "Ruler.h"
#include "ZoomControls.h"
#include "Marker.h"

#include "Date.h"
#include "Event.h"
#include "EventKnown.h"
#include "Phase.h"

#include "Label.h"
#include "Button.h"
#include "LineEdit.h"
#include "CheckBox.h"
#include "RadioButton.h"
#include "Painting.h"

#include "MainWindow.h"
#include "Project.h"

#include "QtUtilities.h"
#include "ModelUtilities.h"

#include <QtWidgets>
#include <iostream>


ResultsView::ResultsView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mModel(0),
mMargin(5),
mOptionsW(200),
mLineH(15),
mGraphLeft(130),
mRulerH(40),
mTabsH(30),
mGraphsH(130),
mHasPhases(false),
mZoomDensity(0),
mZoomTrace(0),
mZoomAccept(0),
mZoomCorrel(0)
{
    mTabs = new Tabs(this);
    mTabs->addTab(tr("Posterior distrib."));
    mTabs->addTab(tr("History plots"));
    mTabs->addTab(tr("Acceptation rate"));
    mTabs->addTab(tr("Autocorrelation"));
    
    connect(mTabs, SIGNAL(tabClicked(int)), this, SLOT(changeTab(int)));
    
    // -------------
    
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
    
    // ----------
    
    mByPhasesBut = new Button(tr("Phases"), this);
    mByPhasesBut->setCheckable(true);
    mByPhasesBut->setChecked(true);
    mByPhasesBut->setAutoExclusive(true);
    mByPhasesBut->setFlatHorizontal();
    
    mByEventsBut = new Button(tr("Events"), this);
    mByEventsBut->setCheckable(true);
    mByEventsBut->setChecked(false);
    mByEventsBut->setAutoExclusive(true);
    mByEventsBut->setFlatHorizontal();
    
    connect(mByPhasesBut, SIGNAL(toggled(bool)), this, SLOT(showByPhases(bool)));
    connect(mByEventsBut, SIGNAL(toggled(bool)), this, SLOT(showByEvents(bool)));
    
    // -------------------------
    
    mDisplayTitle = new Label(tr("Display Options"));
    mDisplayTitle->setIsTitle(true);
    
    mUnfoldBut = new Button(tr("Unfold"));
    mUnfoldBut->setCheckable(true);
    mUnfoldBut->setFlatHorizontal();
    mUnfoldBut->setIcon(QIcon(":unfold.png"));
    mUnfoldBut->setFixedHeight(50);
    mUnfoldBut->setToolTip(tr("Display event's data or phase's events, depending on the chosen layout."));
    
    mInfosBut = new Button(tr("Stats"));
    mInfosBut->setCheckable(true);
    mInfosBut->setFlatHorizontal();
    mInfosBut->setIcon(QIcon(":stats_w.png"));
    mInfosBut->setFixedHeight(50);
    mInfosBut->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));
    
    mExportImgBut = new Button(tr("Save"));
    mExportImgBut->setFlatHorizontal();
    mExportImgBut->setIcon(QIcon(":picture_save.png"));
    mExportImgBut->setFixedHeight(50);
    mExportImgBut->setToolTip(tr("Save all currently visible results as an image.<br><u>Note</u> : If you want to copy textual results, see the Log tab."));
    
    mXScaleLab = new Label(tr("Zoom X :"));
    mYScaleLab = new Label(tr("Zoom Y :"));
    
    mXScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mYScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    mXSlider = new QSlider(Qt::Horizontal);
    mXSlider->setRange(0, 100);
    mXSlider->setTickInterval(1);
    
    mYSlider = new QSlider(Qt::Horizontal);
    mYSlider->setRange(0, 100);
    mYSlider->setTickInterval(1);
    mYSlider->setValue(13);
    
    connect(mXSlider, SIGNAL(valueChanged(int)), this, SLOT(updateScaleX(int)));
    connect(mYSlider, SIGNAL(valueChanged(int)), this, SLOT(updateScaleY(int)));
    
    mRuler = new Ruler(this);
    connect(mRuler, SIGNAL(positionChanged(double, double)), this, SLOT(updateScroll(double, double)));
    
    QVBoxLayout* scaleLayout = new QVBoxLayout();
    scaleLayout->setContentsMargins(5, 5, 5, 5);
    scaleLayout->setSpacing(5);
    scaleLayout->addWidget(mXScaleLab);
    scaleLayout->addWidget(mXSlider);
    scaleLayout->addWidget(mYScaleLab);
    scaleLayout->addWidget(mYSlider);
    
    mRenderLab = new Label(tr("Rendering :"));
    mRenderCombo = new QComboBox();
    mRenderCombo->addItem(tr("Standard (faster)"));
    mRenderCombo->addItem(tr("High (slower)"));
    
    connect(mRenderCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRendering(int)));
    
    QHBoxLayout* renderLayout = new QHBoxLayout();
    renderLayout->setContentsMargins(5, 5, 5, 5);
    renderLayout->setSpacing(5);
    renderLayout->addWidget(mRenderLab);
    renderLayout->addWidget(mRenderCombo);
    
    mUpdateDisplay = new Button(tr("Update display"));
    mUpdateDisplay->mUseMargin = true;
    
    connect(mUpdateDisplay, SIGNAL(clicked()), this, SLOT(updateModel()));
    
    mFont.setPointSize(pointSize(11.));
    mFontBut = new Button(mFont.family() + ", " + QString::number(mFont.pointSizeF()));
    mFontBut->mUseMargin = true;
    
    mThicknessLab = new Label(tr("Graph thichness : "));
    mThicknessSpin = new QSpinBox();
    mThicknessSpin->setRange(1, 5);
    
    connect(mFontBut, SIGNAL(clicked()), this, SLOT(updateFont()));
    connect(mThicknessSpin, SIGNAL(valueChanged(int)), this, SLOT(updateThickness(int)));
    
    QHBoxLayout* thickLayout = new QHBoxLayout();
    thickLayout->setContentsMargins(0, 0, 0, 0);
    thickLayout->setSpacing(5);
    thickLayout->addWidget(mThicknessLab);
    thickLayout->addWidget(mThicknessSpin);
    
    QHBoxLayout* displayButsLayout = new QHBoxLayout();
    displayButsLayout->setContentsMargins(0, 0, 0, 0);
    displayButsLayout->setSpacing(0);
    displayButsLayout->addWidget(mUnfoldBut);
    displayButsLayout->addWidget(mInfosBut);
    displayButsLayout->addWidget(mExportImgBut);
    
    mDisplayGroup = new QWidget();
    QVBoxLayout* displayLayout = new QVBoxLayout();
    displayLayout->setContentsMargins(0, 0, 0, 0);
    displayLayout->setSpacing(5);
    //displayLayout->addWidget(mZoomWidget);
    displayLayout->addLayout(scaleLayout);
    displayLayout->addLayout(renderLayout);
    displayLayout->addWidget(mUpdateDisplay);
    displayLayout->addWidget(mFontBut);
    displayLayout->addLayout(thickLayout);
    mDisplayGroup->setLayout(displayLayout);
    
    connect(mUnfoldBut, SIGNAL(toggled(bool)), this, SLOT(unfoldResults(bool)));
    connect(mInfosBut, SIGNAL(toggled(bool)), this, SLOT(showInfos(bool)));
    connect(mExportImgBut, SIGNAL(clicked()), this, SLOT(exportFullImage()));
    
    // -----------
    
    mChainsTitle = new Label(tr("MCMC Chains"));
    mChainsTitle->setIsTitle(true);
    mChainsGroup = new QWidget();
    mAllChainsCheck = new CheckBox(tr("Chains concatenation"), mChainsGroup);
    mAllChainsCheck->setChecked(true);
    mChainsGroup->setFixedHeight(2*mMargin + mLineH);
    
    connect(mAllChainsCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -----------
    
    mDataTitle = new Label(tr("Results options"));
    mDataTitle->setIsTitle(true);
    mDataGroup = new QWidget();
    
    mDataThetaRadio = new RadioButton(tr("Calendar dates"), mDataGroup);
    mDataSigmaRadio = new RadioButton(tr("Individual std. deviations"), mDataGroup);
    mDataCalibCheck = new CheckBox(tr("Distrib. of calib. dates"), mDataGroup);
    mWiggleCheck = new CheckBox(tr("Wiggle shifted"), mDataGroup);
    mDataThetaRadio->setChecked(true);
    mDataCalibCheck->setChecked(true);
    
    connect(mDataThetaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataCalibCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mWiggleCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mDataSigmaRadio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    // -----------
    
    mPostDistOptsTitle = new Label(tr("Post. distrib. options"));
    mPostDistOptsTitle->setIsTitle(true);
    mPostDistGroup = new QWidget();
    
    mHPDCheck = new CheckBox(tr("Show credibility"), mPostDistGroup);
    mHPDCheck->setChecked(true);
    mThreshLab = new Label(tr("HPD / Credibility (%)") + " :", mPostDistGroup);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mHPDEdit = new LineEdit(mPostDistGroup);
    mHPDEdit->setText("95");
    
    QDoubleValidator* percentValidator = new QDoubleValidator();
    percentValidator->setBottom(0.);
    percentValidator->setTop(100.);
    mHPDEdit->setValidator(percentValidator);
    
    connect(mHPDEdit, SIGNAL(textChanged(const QString&)), this, SLOT(generateHPD()));
    connect(mHPDCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    connect(mHPDEdit, SIGNAL(textChanged(const QString&)), this, SLOT(updateGraphs()));
    
    mRawCheck = new CheckBox(tr("Raw results"), mPostDistGroup);
    mRawCheck->setChecked(false);
    mRawCheck->setVisible(false);
    
    connect(mRawCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    mFFTLenLab = new Label(tr("FFT length") + " :", mPostDistGroup);
    mFFTLenLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mFFTLenCombo = new QComboBox(mPostDistGroup);
    mFFTLenCombo->addItem("32");
    mFFTLenCombo->addItem("64");
    mFFTLenCombo->addItem("128");
    mFFTLenCombo->addItem("256");
    mFFTLenCombo->addItem("512");
    mFFTLenCombo->addItem("1024");
    mFFTLenCombo->addItem("2048");
    mFFTLenCombo->addItem("4096");
    mFFTLenCombo->addItem("8192");
    mFFTLenCombo->addItem("16384");
    mFFTLenCombo->setCurrentText("1024");
    
    mComboH = mFFTLenCombo->sizeHint().height();
    mTabsH = mComboH + 2*mMargin;
    
    connect(mFFTLenCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFFTLength()));
    
    mHFactorLab = new Label(tr("Bandwidth factor") + " :", mPostDistGroup);
    mHFactorLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mHFactorEdit = new LineEdit(mPostDistGroup);
    mHFactorEdit->setText("1");
    
    connect(mHFactorEdit, SIGNAL(returnPressed()), this, SLOT(updateHFactor()));
    
    // -------------------------
    
    mOptionsWidget = new QWidget(this);
    
    QVBoxLayout* optionsLayout = new QVBoxLayout();
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(5);
    optionsLayout->addLayout(displayButsLayout);
    optionsLayout->addWidget(mDisplayTitle);
    optionsLayout->addWidget(mDisplayGroup);
    optionsLayout->addWidget(mChainsTitle);
    optionsLayout->addWidget(mChainsGroup);
    optionsLayout->addWidget(mDataTitle);
    optionsLayout->addWidget(mDataGroup);
    optionsLayout->addWidget(mPostDistOptsTitle);
    optionsLayout->addWidget(mPostDistGroup);
    optionsLayout->addStretch();
    mOptionsWidget->setLayout(optionsLayout);
    
    // -------------------------
    
    mMarker->raise();
}

ResultsView::~ResultsView()
{
    
}

void ResultsView::doProjectConnections(Project* project)
{
    connect(project, SIGNAL(mcmcStarted()), this, SLOT(clearResults()));
    connect(project, SIGNAL(mcmcFinished(Model*)), this, SLOT(updateResults(Model*)));
}

#pragma mark Layout & Paint
void ResultsView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(width() - mOptionsW, 0, mOptionsW, height(), QColor(220, 220, 220));
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

void ResultsView::updateScrollHeights()
{
    if(QWidget* wid = mEventsScrollArea->widget())
    {
        QList<QRect> geometries = getGeometries(mByEventsGraphs, mUnfoldBut->isChecked(), false);
        int h = 0;
        for(int i=0; i<geometries.size(); ++i)
            h += geometries[i].height();
        wid->setFixedHeight(h);
        //qDebug() << "Graph events viewport : " << wid->geometry();
    }
    if(QWidget* wid = mPhasesScrollArea->widget())
    {
        QList<QRect> geometries = getGeometries(mByPhasesGraphs, mUnfoldBut->isChecked(), true);
        int h = 0;
        for(int i=0; i<geometries.size(); ++i)
            h += geometries[i].height();
        wid->setFixedHeight(h);
        //qDebug() << "Graph phases viewport : " << wid->geometry();
    }
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
    mRuler->setGeometry(mGraphLeft + graphYAxis, mTabsH, width() - mGraphLeft - graphYAxis - mOptionsW - sbe - 10, mRulerH);
    mStack->setGeometry(0, mTabsH + mRulerH, width() - mOptionsW, height() - mRulerH - mTabsH);
    mMarker->setGeometry(mMarker->pos().x(), mTabsH + mRulerH, mMarker->thickness(), height() - mRulerH - mTabsH);
    
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
        //qDebug() << "Graph events viewport : " << wid->geometry();
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
        //qDebug() << "Graph phases viewport : " << wid->geometry();
    }
    
    mOptionsWidget->setGeometry(width() - mOptionsW, 0, mOptionsW, height());
    
    int numChains = mCheckChainChecks.size();
    if(mTabs->currentIndex() == 0)
    {
        mChainsGroup->setFixedHeight(m + (numChains+1) * (mLineH + m));
        mAllChainsCheck->setGeometry(m, m, mChainsGroup->width()-2*m, mLineH);
        for(int i=0; i<numChains; ++i)
        {
            QRect geometry(m, m + (i+1) * (mLineH + m), mChainsGroup->width()-2*m, mLineH);
            mCheckChainChecks[i]->setGeometry(geometry);
            mChainRadios[i]->setGeometry(geometry);
        }
    }
    else
    {
        mChainsGroup->setFixedHeight(m + numChains * (mLineH + m));
        for(int i=0; i<numChains; ++i)
        {
            QRect geometry(m, m + i * (mLineH + m), mChainsGroup->width()-2*m, mLineH);
            mCheckChainChecks[i]->setGeometry(geometry);
            mChainRadios[i]->setGeometry(geometry);
        }
    }
    
    int y = m;
    mDataThetaRadio->setGeometry(m, y, mDataGroup->width() - 2*m, mLineH);
    if(mTabs->currentIndex() == 0)
    {
        mDataCalibCheck->setGeometry(m + dx, y += (m + mLineH), mDataGroup->width() - 2*m - dx, mLineH);
        mWiggleCheck->setGeometry(m + dx, y += (m + mLineH), mDataGroup->width() - 2*m - dx, mLineH);
    }
    mDataSigmaRadio->setGeometry(m, y += (m + mLineH), mDataGroup->width()-2*m, mLineH);
    mDataGroup->setFixedHeight(y += (m + mLineH));
    
    y = m;
    int sw = (mPostDistGroup->width() - 3*m) * 0.5;
    int w1 = (mPostDistGroup->width() - 3*m) * 0.7;
    int w2 = (mPostDistGroup->width() - 3*m) * 0.3;
    mHPDCheck->setGeometry(m, y, mPostDistGroup->width() - 2*m, mLineH);
    mThreshLab->setGeometry(m, y += (m + mLineH), w1, mLineH);
    mHPDEdit->setGeometry(2*m + w1, y, w2, mLineH);
    //mRawCheck->setGeometry(m, y += (m + mLineH), mPostDistGroup->width() - 2*m, mLineH);
    mFFTLenLab->setGeometry(m, y += (m + mLineH), sw, mComboH);
    mFFTLenCombo->setGeometry(2*m + sw, y, sw, mComboH);
    mHFactorLab->setGeometry(m, y += (m + mComboH), w1, mLineH);
    mHFactorEdit->setGeometry(2*m + w1, y, w2, mLineH);
    mPostDistGroup->setFixedHeight(y += (m + mLineH));
    
    update();
}

#pragma mark Mouse & Marker
void ResultsView::mouseMoveEvent(QMouseEvent* e)
{
    int x = e->pos().x() - 5;
    x = (x >= mGraphLeft) ? x : mGraphLeft;
    x = (x <= width() - mOptionsW) ? x : width() - mOptionsW;
    mMarker->setGeometry(x, mMarker->pos().y(), mMarker->width(), mMarker->height());
}

#pragma mark Options
void ResultsView::generateHPD()
{
    if(mModel)
    {
        mModel->generateNumericalResults(mChains);
        mModel->generateCredibilityAndHPD(mChains, mHPDEdit->text().toDouble());
        
        updateGraphs();
    }
}

void ResultsView::updateFFTLength()
{
    if(mModel)
    {
        int len = mFFTLenCombo->currentText().toInt();
        double hFactor = mHFactorEdit->text().toDouble();
        
        mModel->generatePosteriorDensities(mChains, len, hFactor);
        mModel->generateNumericalResults(mChains);
        mModel->generateCredibilityAndHPD(mChains, mHPDEdit->text().toDouble());
        
        updateGraphs();
    }
}

void ResultsView::updateHFactor()
{
    if(mModel)
    {
        int len = mFFTLenCombo->currentText().toInt();
        double hFactor = mHFactorEdit->text().toDouble();
        if(!(hFactor > 0 && hFactor <= 100))
        {
            hFactor = 1;
            mHFactorEdit->setText("1");
        }
        
        mModel->generatePosteriorDensities(mChains, len, hFactor);
        mModel->generateNumericalResults(mChains);
        mModel->generateCredibilityAndHPD(mChains, mHPDEdit->text().toDouble());
        
        updateGraphs();
    }
}

#pragma mark Display options
void ResultsView::updateFont()
{
    QFontDialog dialog;
    dialog.setParent(qApp->activeWindow());
    dialog.setFont(mFont);
    
    bool ok;
    QFont font = QFontDialog::getFont(&ok, mFont, this);
    if(ok)
    {
        mFont = font;
        mFontBut->setText(mFont.family() + ", " + QString::number(mFont.pointSizeF()));
        
        for(int i=0; i<mByPhasesGraphs.size(); ++i)
        {
            mByPhasesGraphs[i]->setGraphFont(mFont);
        }
        for(int i=0; i<mByEventsGraphs.size(); ++i)
        {
            mByEventsGraphs[i]->setGraphFont(mFont);
        }
    }
}

void ResultsView::updateThickness(int value)
{
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
    {
        mByPhasesGraphs[i]->setGraphsThickness(value);
    }
    for(int i=0; i<mByEventsGraphs.size(); ++i)
    {
        mByEventsGraphs[i]->setGraphsThickness(value);
    }
}

#pragma mark Update
void ResultsView::clearResults()
{
    mByEventsBut->setVisible(false);
    mByPhasesBut->setVisible(false);
    
    for(int i=mCheckChainChecks.size()-1; i>=0; --i)
    {
        CheckBox* check = mCheckChainChecks.takeAt(i);
        disconnect(check, SIGNAL(clicked()), this, SLOT(updateGraphs()));
        check->setParent(0);
        delete check;
    }
    mCheckChainChecks.clear();
    
    for(int i=mChainRadios.size()-1; i>=0; --i)
    {
        RadioButton* but = mChainRadios.takeAt(i);
        disconnect(but, SIGNAL(clicked()), this, SLOT(updateGraphs()));
        but->setParent(0);
        delete but;
    }
    mChainRadios.clear();
    
    for(int i=0; i<mByEventsGraphs.size(); ++i)
    {
        mByEventsGraphs[i]->setParent(0);
        delete mByEventsGraphs[i];
    }
    mByEventsGraphs.clear();
    
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
    {
        mByPhasesGraphs[i]->setParent(0);
        delete mByPhasesGraphs[i];
    }
    mByPhasesGraphs.clear();
    
    QWidget* eventsWidget = mEventsScrollArea->takeWidget();
    if(eventsWidget)
        delete eventsWidget;
    
    QWidget* phasesWidget = mPhasesScrollArea->takeWidget();
    if(phasesWidget)
        delete phasesWidget;
}

void ResultsView::updateResults(Model* model)
{
    // ------------------------------------------------
    //  Cette fonction est appelée seulement après un "Run"
    // ------------------------------------------------
    
    clearResults();
    
    mModel = model;
    mChains = model->mChains;
    mSettings = mModel->mSettings;
    mMCMCSettings = mModel->mMCMCSettings;
    
    // On force les valeurs par défaut ici puisque
    // les résultats on été calculés avec les valeurs par défaut
    mFFTLenCombo->setCurrentText("1024");
    mHFactorEdit->setText("1");
    
    if(!mModel)
        return;
    
    mMinX = model->mSettings.mTmin;
    mMaxX = model->mSettings.mTmax;
    mRuler->setRange(model->mSettings.mTmin, model->mSettings.mTmax);
    
    mHasPhases = (mModel->mPhases.size() > 0);
    
    mByEventsBut->setVisible(mHasPhases);
    mByPhasesBut->setVisible(mHasPhases);
    
    for(int i=0; i<mChains.size(); ++i)
    {
        CheckBox* check = new CheckBox(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
        connect(check, SIGNAL(clicked()), this, SLOT(updateGraphs()));
        check->setVisible(true);
        mCheckChainChecks.append(check);
        
        RadioButton* radio = new RadioButton(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
        connect(radio, SIGNAL(clicked()), this, SLOT(updateGraphs()));
        radio->setVisible(true);
        if(i == 0)
            radio->setChecked(true);
        mChainRadios.append(radio);
    }
    
    // ----------------------------------------------------
    //  Generate HPD (will then be updated only when HPD value changed)
    // ----------------------------------------------------
    generateHPD();
    
    // ----------------------------------------------------
    //  Phases View
    // ----------------------------------------------------
    
    QWidget* phasesWidget = new QWidget();
    phasesWidget->setMouseTracking(true);
    
    for(int p=0; p<(int)mModel->mPhases.size(); ++p)
    {
        Phase* phase = mModel->mPhases[p];
        GraphViewPhase* graphPhase = new GraphViewPhase(phasesWidget);
        graphPhase->setSettings(mModel->mSettings);
        graphPhase->setMCMCSettings(mModel->mMCMCSettings, mChains);
        graphPhase->setPhase(phase);
        graphPhase->setGraphFont(mFont);
        graphPhase->setGraphsThickness(mThicknessSpin->value());
        mByPhasesGraphs.append(graphPhase);
        
        for(int i=0; i<(int)phase->mEvents.size(); ++i)
        {
            Event* event = phase->mEvents[i];
            GraphViewEvent* graphEvent = new GraphViewEvent(phasesWidget);
            graphEvent->setSettings(mModel->mSettings);
            graphEvent->setMCMCSettings(mModel->mMCMCSettings, mChains);
            graphEvent->setEvent(event);
            graphEvent->setGraphFont(mFont);
            graphEvent->setGraphsThickness(mThicknessSpin->value());
            mByPhasesGraphs.append(graphEvent);
            
            // Do not display datas in phases layout,
            // but if needed, it can be done by uncommenting this :
            
            /*for(int j=0; j<(int)event->mDates.size(); ++j)
             {
             Date& date = event->mDates[j];
             GraphViewDate* graphDate = new GraphViewDate(phasesWidget);
             graphDate->setSettings(mModel->mSettings);
             graphDate->setMCMCSettings(mModel->mMCMCSettings, mChains);
             graphDate->setDate(&date);
             graphDate->setColor(event->mColor);
             mByPhasesGraphs.append(graphDate);
             }*/
        }
    }
    mPhasesScrollArea->setWidget(phasesWidget);
    
    // ----------------------------------------------------
    //  Events View
    // ----------------------------------------------------
    
    QWidget* eventsWidget = new QWidget();
    eventsWidget->setMouseTracking(true);
    
    for(int i=0; i<(int)mModel->mEvents.size(); ++i)
    {
        Event* event = mModel->mEvents[i];
        GraphViewEvent* graphEvent = new GraphViewEvent(eventsWidget);
        graphEvent->setSettings(mModel->mSettings);
        graphEvent->setMCMCSettings(mModel->mMCMCSettings, mChains);
        graphEvent->setEvent(event);
        graphEvent->setGraphFont(mFont);
        graphEvent->setGraphsThickness(mThicknessSpin->value());
        mByEventsGraphs.append(graphEvent);
        
        for(int j=0; j<(int)event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            GraphViewDate* graphDate = new GraphViewDate(eventsWidget);
            graphDate->setSettings(mModel->mSettings);
            graphDate->setMCMCSettings(mModel->mMCMCSettings, mChains);
            graphDate->setDate(&date);
            graphDate->setColor(event->mColor);
            graphDate->setGraphFont(mFont);
            graphDate->setGraphsThickness(mThicknessSpin->value());
            mByEventsGraphs.append(graphDate);
        }
    }
    mEventsScrollArea->setWidget(eventsWidget);
    
    
    if(mHasPhases && mByPhasesBut->isChecked())
        showByPhases(true);
    else
        showByEvents(true);
    
    mTabs->setTab(0);
    
    // Done by changeTab :
    //updateLayout();
    //updateGraphs();
}

void ResultsView::updateGraphs()
{
    updateRulerAreas();
    
    ProjectSettings s = mSettings;
    MCMCSettings mcmc = mMCMCSettings;
    
    GraphViewResults::Variable variable;
    if(mDataThetaRadio->isChecked()) variable = GraphViewResults::eTheta;
    else if(mDataSigmaRadio->isChecked()) variable = GraphViewResults::eSigma;
    
    GraphViewResults::Result result;
    if(mTabs->currentIndex() == 0) result = GraphViewResults::eHisto;
    else if(mTabs->currentIndex() == 1) result = GraphViewResults::eTrace;
    else if(mTabs->currentIndex() == 2) result = GraphViewResults::eAccept;
    else if(mTabs->currentIndex() == 3) result = GraphViewResults::eCorrel;
    
    bool showAllChains = mAllChainsCheck->isChecked();
    QList<bool> showChainList;
    if(mTabs->currentIndex() == 0)
    {
        for(int i=0; i<mCheckChainChecks.size(); ++i)
            showChainList.append(mCheckChainChecks[i]->isChecked());
    }
    else
    {
        for(int i=0; i<mChainRadios.size(); ++i)
            showChainList.append(mChainRadios[i]->isChecked());
    }
    bool showHpd = mHPDCheck->isChecked();
    int hdpThreshold = mHPDEdit->text().toDouble();
    hdpThreshold = qMin(100, hdpThreshold);
    hdpThreshold = qMax(0, hdpThreshold);
    
    bool showRaw = mRawCheck->isChecked();
    
    bool showCalib = mDataCalibCheck->isChecked();
    bool showWiggle = mWiggleCheck->isChecked();
    
    // ---------------------------
    
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
    {
        mByPhasesGraphs[i]->setResultToShow(result, variable, showAllChains, showChainList, showHpd, hdpThreshold, showCalib, showWiggle, showRaw);
    }
    for(int i=0; i<mByEventsGraphs.size(); ++i)
    {
        mByEventsGraphs[i]->setResultToShow(result, variable, showAllChains, showChainList, showHpd, hdpThreshold, showCalib, showWiggle, showRaw);
    }
    
    // Restore current zoom
    if(mTabs->currentIndex() == 0)
        updateScaleX(mZoomDensity);
    else if(mTabs->currentIndex() == 1)
        updateScaleX(mZoomTrace);
    else if(mTabs->currentIndex() == 2)
        updateScaleX(mZoomAccept);
    else if(mTabs->currentIndex() == 3)
        updateScaleX(mZoomCorrel);
    
    updateResultsLog();
    
    update();
}

void ResultsView::updateResultsLog()
{
    QString log;
    
    for(int i=0; i<mModel->mEvents.size(); ++i)
    {
        Event* event = mModel->mEvents[i];
        log += ModelUtilities::eventResultsText(event, true);
    }
    
    for(int i=0; i<mModel->mPhases.size(); ++i)
    {
        Phase* phase = mModel->mPhases[i];
        log += ModelUtilities::phaseResultsText(phase);
    }
    
    emit resultsLogUpdated(log);
}

void ResultsView::updateRulerAreas()
{
    ProjectSettings s = mSettings;
    MCMCSettings mcmc = mMCMCSettings;
    
    if(mTabs->currentIndex() == 0)
    {
        int min = s.mTmin;
        int max = s.mTmax;
        
        mRuler->clearAreas();
        mRuler->setRange(min, max);
        
        if(mDataThetaRadio->isChecked())
        {
            mRuler->setRange(min, max);
        }
        else if(mDataSigmaRadio->isChecked())
        {
            mRuler->setRange(0, max - min);
        }
    }
    else if(mTabs->currentIndex() == 3)
    {
        mRuler->clearAreas();
        mRuler->setRange(0, 100);
    }
    else
    {
        int curChainIdx = -1;
        for(int i=0; i<mChainRadios.size(); ++i)
            if(mChainRadios[i]->isChecked())
                curChainIdx = i;
        
        if(curChainIdx != -1)
        {
            int min = 0;
            int max = mChains[curChainIdx].mNumBurnIter + (mChains[curChainIdx].mBatchIndex * mChains[curChainIdx].mNumBatchIter) + mChains[curChainIdx].mNumRunIter / mChains[curChainIdx].mThinningInterval;
            
            mRuler->setRange(min, max);
            mRuler->clearAreas();
            
            mRuler->addArea(0,
                            mChains[curChainIdx].mNumBurnIter,
                            QColor(235, 115, 100));
            
            mRuler->addArea(mChains[curChainIdx].mNumBurnIter,
                            mChains[curChainIdx].mNumBurnIter + mChains[curChainIdx].mBatchIndex * mChains[curChainIdx].mNumBatchIter,
                            QColor(250, 180, 90));
            
            mRuler->addArea(mChains[curChainIdx].mNumBurnIter + mChains[curChainIdx].mBatchIndex * mChains[curChainIdx].mNumBatchIter,
                            mChains[curChainIdx].mNumBurnIter + mChains[curChainIdx].mBatchIndex * mChains[curChainIdx].mNumBatchIter + mChains[curChainIdx].mNumRunIter / mChains[curChainIdx].mThinningInterval,
                            QColor(130, 205, 110));
        }
    }
    //updateScaleX(mXSlider->value());
    mRuler->setZoom(mXSlider->value());
}

void ResultsView::updateModel()
{
    if(!mModel)
        return;
    
    QJsonObject state = MainWindow::getInstance()->getProject()->state();
    
    QJsonArray events = state[STATE_EVENTS].toArray();
    QJsonArray phases = state[STATE_PHASES].toArray();
    
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        int eventId = event[STATE_ID].toInt();
        QJsonArray dates = event[STATE_EVENT_DATES].toArray();
        
        for(int j=0; j<mModel->mEvents.size(); ++j)
        {
            Event* e = mModel->mEvents[j];
            if(e->mId == eventId)
            {
                e->mName = event[STATE_NAME].toString();
                e->mItemX = event[STATE_ITEM_X].toDouble();
                e->mItemY = event[STATE_ITEM_Y].toDouble();
                e->mColor = QColor(event[STATE_COLOR_RED].toInt(),
                                   event[STATE_COLOR_GREEN].toInt(),
                                   event[STATE_COLOR_BLUE].toInt());
                
                for(int k=0; k<e->mDates.size(); ++k)
                {
                    Date& d = e->mDates[k];
                    
                    for(int l=0; l<dates.size(); ++l)
                    {
                        QJsonObject date = dates[l].toObject();
                        int dateId = date[STATE_ID].toInt();
                        
                        if(dateId == d.mId)
                        {
                            d.mName = date[STATE_NAME].toString();
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    for(int i=0; i<phases.size(); ++i)
    {
        QJsonObject phase = phases[i].toObject();
        int phaseId = phase[STATE_ID].toInt();
        
        for(int j=0; j<mModel->mPhases.size(); ++j)
        {
            Phase* p = mModel->mPhases[j];
            if(p->mId == phaseId)
            {
                p->mName = phase[STATE_NAME].toString();
                p->mItemX = phase[STATE_ITEM_X].toDouble();
                p->mItemY = phase[STATE_ITEM_Y].toDouble();
                p->mColor = QColor(phase[STATE_COLOR_RED].toInt(),
                                   phase[STATE_COLOR_GREEN].toInt(),
                                   phase[STATE_COLOR_BLUE].toInt());
                break;
            }
        }
    }
    
    std::sort(mModel->mEvents.begin(), mModel->mEvents.end(), sortEvents);
    std::sort(mModel->mPhases.begin(), mModel->mPhases.end(), sortPhases);
    
    updateResults(mModel);
}

#pragma mark Display options
void ResultsView::unfoldResults(bool open)
{
    QList<QRect> geometries = getGeometries(mByEventsGraphs, open, false);
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i]->toggle(geometries[i]);

    geometries = getGeometries(mByPhasesGraphs, open, true);
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i]->toggle(geometries[i]);
    
    //updateScrollHeights();
    
    if(open)
    {
        updateScrollHeights();
    }
    else
    {
        mTimer->start(200);
    }
}

void ResultsView::showInfos(bool show)
{
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i]->showNumericalResults(show);
    
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i]->showNumericalResults(show);
}

void ResultsView::exportFullImage()
{
    // Force rendering to HD for export
    int rendering = mRenderCombo->currentIndex();
    updateRendering(1);
    
    QWidget* curWid = (mStack->currentWidget() == mPhasesScrollArea) ? mPhasesScrollArea->widget() : mEventsScrollArea->widget();
    
    QRect r(mGraphLeft, 0, curWid->width() - mGraphLeft, curWid->height());
    QFileInfo fileInfo = saveWidgetAsImage(curWid, r,
                                           tr("Save graph image as..."),
                                           MainWindow::getInstance()->getCurrentPath());
    if(fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
    
    // Reset rendering back to its current value
    updateRendering(rendering);
}

void ResultsView::updateScaleX(int zoom)
{
    if(mTabs->currentIndex() == 0)
        mZoomDensity = zoom;
    else if(mTabs->currentIndex() == 1)
        mZoomTrace = zoom;
    else if(mTabs->currentIndex() == 2)
        mZoomAccept = zoom;
    else if(mTabs->currentIndex() == 3)
        mZoomCorrel = zoom;
    
    mXSlider->setValue(zoom);
    mRuler->setZoom(zoom);
}

void ResultsView::updateScroll(double min, double max)
{
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i]->zoom(min, max);
    
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i]->zoom(min, max);
}

void ResultsView::updateScaleY(int value)
{
    double min = 20;
    double max = 1020;
    double prop = value / 100.f;
    mGraphsH = min + prop * (max - min);
    updateLayout();
}

void ResultsView::updateRendering(int index)
{
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i]->setRendering((GraphView::Rendering) index);
    
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i]->setRendering((GraphView::Rendering) index);
}

void ResultsView::showByPhases(bool)
{
    mStack->setCurrentWidget(mPhasesScrollArea);
}

void ResultsView::showByEvents(bool)
{
    mStack->setCurrentWidget(mEventsScrollArea);
}

void ResultsView::changeTab(int index)
{
    mPostDistOptsTitle->setVisible(index == 0);
    mPostDistGroup->setVisible(index == 0);
    mDataCalibCheck->setVisible(index == 0);
    mWiggleCheck->setVisible(index == 0);
    mAllChainsCheck->setVisible(index == 0);
    mDataCalibCheck->setVisible(index == 0);
    
    if(index == 0)
    {
        for(int i=0; i<mCheckChainChecks.size(); ++i)
            mCheckChainChecks[i]->setVisible(true);
        
        for(int i=0; i<mChainRadios.size(); ++i)
            mChainRadios[i]->setVisible(false);
    }
    else
    {
        for(int i=0; i<mCheckChainChecks.size(); ++i)
            mCheckChainChecks[i]->setVisible(false);
        
        for(int i=0; i<mChainRadios.size(); ++i)
            mChainRadios[i]->setVisible(true);
    }
    
    updateGraphs();
    updateLayout();
}
