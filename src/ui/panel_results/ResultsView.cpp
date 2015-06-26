
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
#include "StdUtilities.h"
#include "ModelUtilities.h"
#include "DoubleValidator.h"

#include "PluginAbstract.h"

#include <QtWidgets>
#include <iostream>
#include <QtSvg>

#pragma mark Constructor & Destructor
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
    
    
    mResultMinX = mSettings.mTmin;
    mResultMaxX = mSettings.mTmax;
    mResultCurrentMinX = mResultMinX ;
    mResultCurrentMaxX = mResultMaxX ;
    mResultZoomX = 1;
    
    mTabs = new Tabs(this);
    mTabs->addTab(tr("Posterior distrib."));
    mTabs->addTab(tr("History plots"));
    mTabs->addTab(tr("Acceptation rate"));
    mTabs->addTab(tr("Autocorrelation"));
    
    connect(mTabs, SIGNAL(tabClicked(int)), this, SLOT(changeTab(int)));
    
    // -------------
    
    mStack = new QStackedWidget(this);
    //connect(mStack, SIGNAL(clicked()), this, SLOT(updateResults()));// don't work
    
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
    QWidget* PhaseEvent(this);
    mByPhasesBut = new Button(tr("Phases"), PhaseEvent);
    mByPhasesBut->setCheckable(true);
    mByPhasesBut->setChecked(mHasPhases);
    mByPhasesBut->setAutoExclusive(true);
    mByPhasesBut->setFlatHorizontal();
    
    mByEventsBut = new Button(tr("Events"), PhaseEvent);
    mByEventsBut->setCheckable(true);
    mByEventsBut->setChecked(!mHasPhases);
    mByEventsBut->setAutoExclusive(true);
    mByEventsBut->setFlatHorizontal();
    
    mByEventsBut->setVisible(mHasPhases);
    mByPhasesBut->setVisible(mHasPhases);
    
    //connect(mByPhasesBut, SIGNAL(toggled(bool)), this, SLOT(showByPhases(bool)));//clicked
    //connect(mByEventsBut, SIGNAL(toggled(bool)), this, SLOT(showByEvents(bool)));
    connect(mByPhasesBut, SIGNAL(clicked(bool)), this, SLOT(showByPhases(bool)));//clicked
    connect(mByEventsBut, SIGNAL(clicked(bool)), this, SLOT(showByEvents(bool)));
    
    // -------------------------
    
    
    mUnfoldBut = new Button(tr("Unfold"));
    mUnfoldBut->setCheckable(true);
    mUnfoldBut->setFlatHorizontal();
    mUnfoldBut->setIcon(QIcon(":unfold.png"));
    mUnfoldBut->setFixedHeight(50);
    mUnfoldBut->setToolTip(tr("Display event's data or phase's events, depending on the chosen layout."));
    connect(mUnfoldBut, SIGNAL(toggled(bool)), this, SLOT(unfoldResults(bool)));
    
    mInfosBut = new Button(tr("Stats"));
    mInfosBut->setCheckable(true);
    mInfosBut->setFlatHorizontal();
    mInfosBut->setIcon(QIcon(":stats_w.png"));
    mInfosBut->setFixedHeight(50);
    mInfosBut->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));
    connect(mInfosBut, SIGNAL(toggled(bool)), this, SLOT(showInfos(bool)));

    
    mExportImgBut = new Button(tr("Save"));
    mExportImgBut->setFlatHorizontal();
    mExportImgBut->setIcon(QIcon(":picture_save.png"));
    mExportImgBut->setFixedHeight(50);
    mExportImgBut->setToolTip(tr("Save all currently visible results as an image.<br><u>Note</u> : If you want to copy textual results, see the Log tab."));
    connect(mExportImgBut, SIGNAL(clicked()), this, SLOT(exportFullImage()));
    
    QHBoxLayout* displayButsLayout = new QHBoxLayout();
    displayButsLayout->setContentsMargins(0, 0, 0, 0);
    displayButsLayout->setSpacing(0);
    displayButsLayout->addWidget(mUnfoldBut);
    displayButsLayout->addWidget(mInfosBut);
    displayButsLayout->addWidget(mExportImgBut);
    
    
    mRuler = new Ruler(this);
        //connect(mRuler, SIGNAL(valueChanged(int)), this, SLOT(updateRuler(int)));
    connect(mRuler, SIGNAL(positionChanged(double, double)), this, SLOT(updateScroll(double, double)));
        // connect(mRuler, SIGNAL(positionChanged(double, double)), this, SLOT(currentChanged(double, double)));
    mRuler->mMarginLeft = 50;
    mRuler->mMarginRight = 10;
    
    mRuler->mMax = mSettings.mTmax;
    mRuler->mMin = mSettings.mTmin;
    mRuler->mCurrentMax = mSettings.mTmax;
    mRuler->mCurrentMin = mSettings.mTmin;
    
    /* -------------------------------------- mDisplayGroup---------------------------------------------------*/
    mDisplayGroup = new QWidget();
   
    
    mDisplayTitle = new Label(tr("Display Options"));
    mDisplayTitle->setIsTitle(true);
    
    mCurrentXMinEdit = new LineEdit(mDisplayGroup);
    mCurrentXMinEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mCurrentXMinEdit->setAlignment(Qt::AlignHCenter);
    mCurrentXMinEdit->setFixedSize(45, 15);
    //mCurrentXMinEdit->setValidator(new QDoubleValidator(-99999.0, 99999.0, 1, mCurrentXMinEdit));
        //connect(mCurrentXMinEdit, SIGNAL(editingFinished()), this, SLOT(setCurrentMinX()) ); // textEdited
    connect(mCurrentXMinEdit, SIGNAL(editingFinished()), this, SLOT(editCurrentMinX()) );

    
    mCurrentXMaxEdit = new LineEdit(mDisplayGroup);
    mCurrentXMaxEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mCurrentXMaxEdit->setAlignment(Qt::AlignHCenter);
    mCurrentXMaxEdit->setFixedSize(45, 15);
        // mCurrentXMaxEdit->setGeometry(0, 5, 50, 20);
    //mCurrentXMaxEdit->setValidator(new QDoubleValidator(-99999.0, 99999.0, 1, mCurrentXMaxEdit));
        //connect(mCurrentXMaxEdit, SIGNAL(editingFinished()), this, SLOT(setCurrentMaxX()) ); //editCurrentMaxX
    connect(mCurrentXMaxEdit, SIGNAL(editingFinished()), this, SLOT(editCurrentMaxX()) );
    
    
    
    mXScaleLab = new Label(tr("Zoom X :"),mDisplayGroup);
    mYScaleLab = new Label(tr("Zoom Y :"),mDisplayGroup);
    
    mXScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mYScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    mXSlider = new QSlider(Qt::Horizontal,mDisplayGroup);
    mXSlider->setRange(0, 100);
    mXSlider->setTickInterval(1);
    
    mYSlider = new QSlider(Qt::Horizontal,mDisplayGroup);
    mYSlider->setRange(0, 100);
    mYSlider->setTickInterval(1);
    mYSlider->setValue(13);
    
    connect(mXSlider, SIGNAL(sliderMoved(int)), this, SLOT(updateZoomX())); //sliderPressed()
    connect(mXSlider, SIGNAL(sliderPressed()), this, SLOT(updateZoomX())); //sliderPressed()
    
    connect(mYSlider, SIGNAL(valueChanged(int)), this, SLOT(updateScaleY(int)));
    
    
    mRenderLab = new Label(tr("Rendering :"),mDisplayGroup);
    mRenderCombo = new QComboBox(mDisplayGroup);
    mRenderCombo->addItem(tr("Standard (faster)"));
    mRenderCombo->addItem(tr("High (slower)"));
    
    connect(mRenderCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRendering(int)));
    
    
    
   /*  keep in memory
    mUpdateDisplay = new Button(tr("Update display"),mScaleGroup);
    mUpdateDisplay->mUseMargin = true;
    
    connect(mUpdateDisplay, SIGNAL(clicked()), this, SLOT(updateModel()));
   */
    
    
    
    mFont.setPointSize(pointSize(11.));
    mFontBut = new Button(mFont.family() + ", " + QString::number(mFont.pointSizeF()),mDisplayGroup);
    mFontBut->mUseMargin = true;
    
    mThicknessLab = new Label(tr("Graph thichness : "),mDisplayGroup);
    mThicknessSpin = new QSpinBox();
    mThicknessSpin->setRange(1, 5);
    connect(mFontBut, SIGNAL(clicked()), this, SLOT(updateFont()));
    connect(mThicknessSpin, SIGNAL(valueChanged(int)), this, SLOT(updateThickness(int)));
    
    
    QGridLayout* displayLayout = new QGridLayout(mDisplayGroup);
    displayLayout->setContentsMargins(3, 3, 3, 3);
    displayLayout->setSpacing(3);

    
    displayLayout->addWidget(mCurrentXMinEdit,1,0,1,2);
    displayLayout->addWidget(mXScaleLab,1,3,1,2);
    displayLayout->addWidget(mCurrentXMaxEdit,1,6,1,2);
    
    displayLayout->addWidget(mXSlider,2,0,1,8);
    
    displayLayout->addWidget(mYScaleLab,3,3,1,2);
    
    displayLayout->addWidget(mYSlider,4,0,1,8);
    
    displayLayout->addWidget(mRenderLab,5,0,1,3);
    displayLayout->addWidget(mRenderCombo,5,3,1,5);
    
    //displayLayout->addWidget(mUpdateDisplay,6,0);
    
    displayLayout->addWidget(mFontBut,6,0,1,8);
    
    displayLayout->addWidget(mThicknessLab,7,0,1,5);
    displayLayout->addWidget(mThicknessSpin,7,5,1,2);
    
    mDisplayGroup->setLayout(displayLayout);
    mDisplayGroup->setFixedHeight(140);

    
    /* -------------------------------------- mChainsGroup---------------------------------------------------*/
    mChainsGroup = new QWidget();
    
    mChainsTitle = new Label(tr("MCMC Chains"));
    mChainsTitle->setIsTitle(true);
    
    mAllChainsCheck = new CheckBox(tr("Chains concatenation"), mChainsGroup);
    mAllChainsCheck->setChecked(true);
    mChainsGroup->setFixedHeight(2*mMargin + mLineH);
    
    connect(mAllChainsCheck, SIGNAL(clicked()), this, SLOT(updateGraphs()));
    
    /* -------------------------------------- mResultsGroup---------------------------------------------------*/

    mResultsGroup = new QWidget();
    
    mResultsTitle = new Label(tr("Results options"));
    mResultsTitle->setIsTitle(true);
    
    
    mDataThetaRadio = new RadioButton(tr("Calendar dates"), mResultsGroup);
    mDataSigmaRadio = new RadioButton(tr("Individual std. deviations"), mResultsGroup);
    
    mShowDataUnderPhasesCheck = new CheckBox(tr("Unfold data under Event"),mResultsGroup);
    mDataCalibCheck           = new CheckBox(tr("Individual calib. dates"), mResultsGroup);

    mWiggleCheck              = new CheckBox(tr("Wiggle shifted"), mResultsGroup);
    mDataThetaRadio           -> setChecked(true);
    mDataCalibCheck           -> setChecked(true);
    mShowDataUnderPhasesCheck -> setChecked(false);
        //  mResultsGroup->setFixedHeight(mResultsTitle->height()+mDataThetaRadio->QPaintDevice::height()+mDataSigmaRadio->height()+mShowDataUnderPhasesCheck->height()+
        //                              mDataCalibCheck->QPaintDevice::height()+mWiggleCheck->height()+100);
    connect(mShowDataUnderPhasesCheck, SIGNAL(toggled(bool)), this, SLOT(updateResults()));
    
    connect(mDataThetaRadio,     SIGNAL(clicked()), this, SLOT(updateResults()));
    connect(mDataCalibCheck,     SIGNAL(clicked()), this, SLOT(updateResults()));
    connect(mWiggleCheck,        SIGNAL(clicked()), this, SLOT(updateResults()));
    connect(mDataSigmaRadio,     SIGNAL(clicked()), this, SLOT(updateResults()));
    
    //connect(mDataGroup,     SIGNAL(clicked()), this, SLOT(updateResults()));// Unusefull
  /*  QVBoxLayout* mResultsLayout = new QVBoxLayout();
    mResultsLayout->addWidget(mShowDataUnderPhasesCheck);
    mResultsLayout->addWidget(mDataThetaRadio);
        mResultsLayout->addWidget(mDataSigmaRadio);
        mResultsLayout->addWidget(mDataThetaRadio);
    mResultsGroup->setLayout(mResultsLayout);
   */
    /* -------------------------------------- mPostDistGroup ---------------------------------------------------*/
    
    mPostDistOptsTitle = new Label(tr("Post. distrib. options"));
    mPostDistOptsTitle->setIsTitle(true);
    mPostDistGroup = new QWidget();
    
    mCredibilityCheck = new CheckBox(tr("Show credibility"), mPostDistGroup);
    mCredibilityCheck->setChecked(true);
    mCredibilityCheck->setVisible(true);
    mThreshLab = new Label(tr("HPD / Credibility (%)") + " :", mPostDistGroup);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mHPDEdit = new LineEdit(mPostDistGroup);
    mHPDEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mHPDEdit->setText("95");
    mHPDEdit->setVisible(true);
    
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator -> setBottom(0.);
    percentValidator -> setTop(100.);
    percentValidator -> setDecimals(1);
    mHPDEdit->setValidator(percentValidator);
    
    //connect(mHPDEdit,  SIGNAL(textEdited(const QString&)),  this, SLOT(generateHPD()));
    
    connect(mCredibilityCheck, SIGNAL(clicked()),                   this, SLOT(updateGraphs()));
    //connect(mHPDEdit,  SIGNAL(textChanged(const QString&)), this, SLOT(updateGraphs()));
    connect(mHPDEdit,  SIGNAL(returnPressed()), this, SLOT(generateHPD()));
    
    mRawCheck = new CheckBox(tr("Raw results"), mPostDistGroup);
    mRawCheck -> setChecked(false);
    mRawCheck -> setVisible(false);
    
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
    mFFTLenCombo->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mFFTLenCombo->setVisible(true);
    
    mComboH = mFFTLenCombo->sizeHint().height();
    mTabsH = mComboH + 2*mMargin;
    
    connect(mFFTLenCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFFTLength()));
    
    mHFactorLab = new Label(tr("Bandwidth factor") + " :", mPostDistGroup);
    mHFactorLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mHFactorEdit = new LineEdit(mPostDistGroup);
    mHFactorEdit->setText("1");
    mHFactorEdit->setVisible(true);
    mHFactorEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    connect(mHFactorEdit, SIGNAL(returnPressed()), this, SLOT(updateHFactor()));
    
    // -------------------------
    
    mOptionsWidget = new QWidget(this);
    
    QVBoxLayout* optionsLayout = new QVBoxLayout();
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(0);
    optionsLayout->addLayout(displayButsLayout);
    optionsLayout->addWidget(mDisplayTitle);
    optionsLayout->addWidget(mDisplayGroup);
    optionsLayout->addWidget(mChainsTitle);
    optionsLayout->addWidget(mChainsGroup);
    optionsLayout->addWidget(mResultsTitle);
    optionsLayout->addWidget(mResultsGroup);
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

void ResultsView::memoZoom(const double& zoom)
{
    if(mTabs->currentIndex() == 0)
        mZoomDensity = zoom;
    else if(mTabs->currentIndex() == 1)
        mZoomTrace = zoom;
    else if(mTabs->currentIndex() == 2)
        mZoomAccept = zoom;
    else if(mTabs->currentIndex() == 3)
        mZoomCorrel = zoom;
}
void ResultsView::restoreZoom()
{
    if(mTabs->currentIndex() == 0)
        mResultZoomX =  mZoomDensity;
    else if(mTabs->currentIndex() == 1)
        mResultZoomX = mZoomTrace;
    else if(mTabs->currentIndex() == 2)
        mResultZoomX = mZoomAccept;
    else if(mTabs->currentIndex() == 3)
        mResultZoomX = mZoomCorrel;
}
#pragma mark Layout & Paint
void ResultsView::paintEvent(QPaintEvent* )
{
    QPainter p(this);
    p.fillRect(width() - mOptionsW, 0, mOptionsW, height(), QColor(220, 220, 220));
}

void ResultsView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);

    updateLayout(); // it is usefull, after the constructor it refresh the layout with the good size

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
    if(QWidget* wid = mEventsScrollArea->widget()) {
        QList<QRect> geometries = getGeometries(mByEventsGraphs, mUnfoldBut->isChecked(), false);
        int h = 0;
        for(int i=0; i<geometries.size(); ++i)
            h += geometries[i].height();
        wid->setFixedHeight(h);
    }
    if(QWidget* wid = mPhasesScrollArea->widget()) {
        QList<QRect> geometries = getGeometries(mByPhasesGraphs, mUnfoldBut->isChecked(), true);
        int h = 0;
        for(int i=0; i<geometries.size(); ++i)
            h += geometries[i].height();
        wid->setFixedHeight(h);
    }
    //updateLayout();
}


 void ResultsView::updateLayout()
{
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int graphYAxis = 50;
    int m = mMargin;
    int dx = mLineH + mMargin;//m;
    
    //updateLayout(); infinite loop in PaintEvent
    if(mTabs->currentIndex() == 0) // // it's mean posterior distribution
    {
        mCurrentXMinEdit->setText( DateUtils::convertToAppSettingsFormatStr(mResultCurrentMinX) );
        mCurrentXMaxEdit->setText( DateUtils::convertToAppSettingsFormatStr(mResultCurrentMaxX) );
    }
    else {
        mCurrentXMinEdit->setText( QString::number(mResultCurrentMinX) );
        mCurrentXMaxEdit->setText( QString::number(mResultCurrentMaxX) );
    }
    
    mByPhasesBut -> setGeometry(0, 0, (int)(mGraphLeft/2), mRulerH);
    mByEventsBut -> setGeometry(mGraphLeft/2, 0, (int)(mGraphLeft/2), mRulerH);
    
    mTabs   -> setGeometry(mGraphLeft + graphYAxis, 0, width() - mGraphLeft - mOptionsW - sbe - graphYAxis, mTabsH);
    mRuler  -> setGeometry(mGraphLeft, mTabsH, width() - mGraphLeft - mOptionsW - sbe, mRulerH);
    mStack  -> setGeometry(0, mTabsH + mRulerH, width() - mOptionsW, height() - mRulerH - mTabsH);
    mMarker -> setGeometry(mMarker->pos().x(), mTabsH + mRulerH, mMarker->thickness(), height() - mRulerH - mTabsH);
    
    mOptionsWidget -> setGeometry(this->width() - mOptionsW, 0, mOptionsW, this->height());
    mDisplayGroup  -> setGeometry(0, mDisplayTitle->y()+ mDisplayTitle->height(), mOptionsW, mDisplayGroup->height());
    
    //   mOptionsWidget->move(width() - mOptionsW, 0);
    int numChains = mCheckChainChecks.size();
    if(mTabs->currentIndex() == 0) // it's mean posterior distribution
    {
        mChainsGroup->setFixedHeight(m + (numChains+1) * (mLineH + m));
        mAllChainsCheck->setGeometry(m, m, (int)(mChainsGroup->width()-2*m), mLineH);
        for(int i=0; i<numChains; ++i)
        {
            QRect geometry(m, m + (i+1) * (mLineH + m), (int)(mChainsGroup->width()-2*m), mLineH);
            mCheckChainChecks[i]->setGeometry(geometry);
            mChainRadios[i]->setGeometry(geometry);
        }
    }
    else
    {
        mChainsGroup->setFixedHeight(m + numChains * (mLineH + m));
        for(int i=0; i<numChains; ++i)
        {
            QRect geometry(m, (int)(m + i * (mLineH + m)), (int)(mChainsGroup->width()-2*m), mLineH);
            mCheckChainChecks[i] -> setGeometry(geometry);
            mChainRadios[i]      -> setGeometry(geometry);
        }
    }
    
    int y = m;
    mDataThetaRadio->setGeometry(m, y, (int)(mResultsGroup->width() - 2*m), mLineH);
    
    if(mTabs->currentIndex() == 0) // // it's mean posterior distribution
    {
        
        mDataCalibCheck -> setGeometry(m + dx, y += (m + mLineH),(int) (mResultsGroup->width() - 2*m - dx), mLineH);
        mWiggleCheck    -> setGeometry(m + dx, y += (m + mLineH),(int)( mResultsGroup->width() - 2*m - dx), mLineH);
    }
    if (mShowDataUnderPhasesCheck->isVisible()) {
        mShowDataUnderPhasesCheck->setGeometry(m + dx, y += (m + mLineH),(int) (mResultsGroup->width() - 2*m - dx), mLineH);
    }
    
    
    mDataSigmaRadio -> setGeometry(m, y += (m + mLineH), mResultsGroup->width()-2*m, mLineH);
    mResultsGroup   -> setFixedHeight(y += (m + mLineH));
    
    y = m;
    int sw = (mPostDistGroup->width() - 3*m) * 0.5;
    int w1 = (mPostDistGroup->width() - 3*m) * 0.7;
    int w2 = (mPostDistGroup->width() - 3*m) * 0.3;
    
    mCredibilityCheck  -> setGeometry(m, y, mPostDistGroup->width() - 2*m, mLineH);
    mThreshLab -> setGeometry(m, y += (m + mLineH), w1, mLineH);
    mHPDEdit   -> setGeometry(2*m + w1, y, w2, mLineH);
    mFFTLenLab     -> setGeometry(m, y += (m + mLineH), sw, mComboH);
    mFFTLenCombo   -> setGeometry(2*m + sw, y, sw, mComboH);
    mHFactorLab    -> setGeometry(m, y += (m + mComboH), w1, mLineH);
    mHFactorEdit   -> setGeometry(2*m + w1, y, w2, mLineH);
    mPostDistGroup -> setFixedHeight(y += (m + mLineH));
    
    
    
    
    
  
    //    setUpdatesEnabled(false); // prevent from recursive repaint, when setGeometry
    
   if(mByEventsBut->isChecked()){
        QWidget* wid = mEventsScrollArea->widget();
        QList<QRect> geometries = getGeometries(mByEventsGraphs, mUnfoldBut->isChecked(), false);
        int h = 0;
        for(int i=0; i<mByEventsGraphs.size(); ++i) {
            
            mByEventsGraphs[i]->update();
            mByEventsGraphs[i]->setGeometry(geometries[i]);
            
            h += geometries[i].height();
        }
        wid->setFixedSize(width() - sbe - mOptionsW, h);
        
    }
    
    
    if(mByPhasesBut->isChecked()){
        // qDebug()<<"phase is checked";
        QWidget* wid = mPhasesScrollArea->widget();
        QList<QRect> geometries = getGeometries(mByPhasesGraphs, mUnfoldBut->isChecked(), true);
        int h = 0;
        for(int i=0; i<mByPhasesGraphs.size(); ++i){
            mByPhasesGraphs[i]->update();
            mByPhasesGraphs[i]->setGeometry(geometries[i]);
           
            h += geometries[i].height();
        }
        // mPhasesScrollArea->repaint();
        wid->setFixedSize(width() - sbe - mOptionsW, h);
        //mPhasesScrollArea->setFixedSize(width() - sbe - mOptionsW, h);
    }
    
    // setUpdatesEnabled(true);

}

void ResultsView::updateAllZoom()
{
    for(int i=0; i<mByPhasesGraphs.size(); ++i){
        mByPhasesGraphs[i]->zoom(mResultCurrentMinX, mResultCurrentMaxX);
        mByPhasesGraphs[i]->update() ;
        //mByPhasesGraphs[i]->update() ;
    }
   
    for(int i=0; i<mByEventsGraphs.size(); ++i) {
        mByEventsGraphs[i]->zoom(mResultCurrentMinX, mResultCurrentMaxX);
        mByEventsGraphs[i]->update();
    }
    memoZoom(mResultZoomX);
    updateLayout();
    //update();
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
        QString input = mHPDEdit->text();
        mHPDEdit->validator()->fixup(input);
        mHPDEdit->setText(input);
        
        //clearHisto();
        //clearChainHistos();
        // I don't need to clear the rawHisto
        //clearRawHisto();
        clearCredibilityAndHPD();
        mModel->generateNumericalResults(mChains);
        mModel->generateCredibilityAndHPD(mChains, mHPDEdit->text().toDouble());
        
        updateGraphs();
        //for(int i=0; i<mByPhasesGraphs.size(); ++i) mByPhasesGraphs[i]->update();
        //for(int i=0; i<mByEventsGraphs.size(); ++i) mByEventsGraphs[i]->update();
    }
}

void ResultsView::updateFFTLength()
{
    if(mModel)
    {
        int len = mFFTLenCombo->currentText().toInt();
        double hFactor = mHFactorEdit->text().toDouble();
        clearRawHisto();
        clearHisto();
        clearChainHistos();
        clearCredibilityAndHPD();
        mModel->generatePosteriorDensities(mChains, len, hFactor);
        mModel->generateNumericalResults(mChains);
        mModel->generateCredibilityAndHPD(mChains, mHPDEdit->text().toDouble());
        
        updateGraphs(); // update mByPhasesGraphs and mByEventsGraphs
        //for(int i=0; i<mByPhasesGraphs.size(); ++i) mByPhasesGraphs[i]->update();
        //for(int i=0; i<mByEventsGraphs.size(); ++i) mByEventsGraphs[i]->update();
        
    }
    
    
    // mPhasesScrollArea->repaint();
   // mEventsScrollArea->repaint();
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
        clearRawHisto();
        clearHisto();
        clearChainHistos();
        clearCredibilityAndHPD();
        mModel->generatePosteriorDensities(mChains, len, hFactor);
        mModel->generateNumericalResults(mChains);
        mModel->generateCredibilityAndHPD(mChains, mHPDEdit->text().toDouble());
        
        updateGraphs(); // update mByPhasesGraphs and mByEventsGraphs
        //for(int i=0; i<mByPhasesGraphs.size(); ++i) mByPhasesGraphs[i]->update();
        //for(int i=0; i<mByEventsGraphs.size(); ++i) mByEventsGraphs[i]->update();
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
        mPhasesScrollArea->setFont(mFont);
        for(int i=0; i<mByEventsGraphs.size(); ++i)
        {
            
            mByEventsGraphs[i]->setGraphFont(mFont);
        }
        mEventsScrollArea->setFont(mFont);
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
   // repaint();
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
    
    // On force les valeurs par défaut ici puisque
    // les résultats on été calculés avec les valeurs par défaut
    mFFTLenCombo->setCurrentText("1024");
    mHFactorEdit->setText("1");
    
    if(!mModel && !model)
        return;
    
    if(model)
        mModel = model;
    
    mChains = mModel->mChains;
    mSettings = mModel->mSettings;
    mMCMCSettings = mModel->mMCMCSettings;
    
    mResultZoomX     = 1;
    
    mRuler -> setRange(mResultMinX, mResultMaxX);
    mRuler -> setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    updateRulerAreas();
    
    mCurrentXMinEdit -> setText( DateUtils::convertToAppSettingsFormatStr(mResultMinX) );
    mCurrentXMaxEdit -> setText( DateUtils::convertToAppSettingsFormatStr(mResultMaxX) );
    
    mHasPhases = (mModel->mPhases.size() > 0);
    mByPhasesBut -> setAutoExclusive(true);
    
    mByPhasesBut -> setChecked( mHasPhases);
    mByEventsBut -> setChecked(!mHasPhases);
    
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
            
            // --------------------------------------------------
            //  Display dates only if required
            // --------------------------------------------------
            if(mShowDataUnderPhasesCheck->isChecked())
            {
                for(int j=0; j<(int)event->mDates.size(); ++j)
                {
                    Date& date = event->mDates[j];
                    //PluginAbstract* mDatePlugin =  date.mPlugin;
                    GraphViewDate* graphDate = new GraphViewDate(phasesWidget);
                    graphDate->setSettings(mModel->mSettings);
                    graphDate->setMCMCSettings(mModel->mMCMCSettings, mChains);
                    graphDate->setDate(&date);
                    graphDate->setGraphFont(mFont);
                    graphDate->setGraphsThickness(mThicknessSpin->value());
                    //QColor dataPluginColor = date.mPlugin->getColor();
                    
                    graphDate->setColor(event->mColor);
                    mByPhasesGraphs.append(graphDate);
                    /*qDebug()<<"mName"<<date.mName;
                    for(int k=0; k<(int)date.mCalibration.size(); ++k)
                    {
                       qDebug()<<"cal"<<date.mCalibration[k];
                    }*/
                    
                    
                }
            }
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
    showInfos(mInfosBut->isChecked());
    
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
    bool showCredibility = mCredibilityCheck->isChecked();
    
    
    float hpdThreshold = inRange<float>(mHPDEdit->text().toFloat(),0.0 ,100.0);
    
    
    bool showRaw = mRawCheck->isChecked();
    
    bool showCalib = mDataCalibCheck->isChecked();
    bool showWiggle = mWiggleCheck->isChecked();
    
    // --------------------------- mCurrentTypeGraph = GraphViewResults::TypeGraph(
   // if(QWidget* wid = mPhasesScrollArea->widget())
    if(mByPhasesBut->isChecked()) {
        for(int i=0; i<mByPhasesGraphs.size(); ++i){
            mByPhasesGraphs[i]->setResultToShow(GraphViewResults::TypeGraph(mCurrentTypeGraph), variable, showAllChains, showChainList, showCredibility, hpdThreshold, showCalib, showWiggle, showRaw);
            mByPhasesGraphs[i]->update();
        }
    }
    //if(QWidget* wid = mEventsScrollArea->widget())
    if(mByEventsBut->isChecked()){
        for(int i=0; i<mByEventsGraphs.size(); ++i){
            mByEventsGraphs[i]->setResultToShow(GraphViewResults::TypeGraph(mCurrentTypeGraph), variable, showAllChains, showChainList, showCredibility, hpdThreshold, showCalib, showWiggle, showRaw);
            mByEventsGraphs[i]->update();
       }
 }
    // Restore current zoom
    restoreZoom();
    
    updateResultsLog();
    
    //update();
    //updateLayout();
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
        mRuler->setCurrent(min, max);
        
        if(mDataThetaRadio->isChecked())
        {
            mRuler->setRange(min, max);
            mRuler->setCurrent(min, max);
        }
        else if(mDataSigmaRadio->isChecked())
        {
            mRuler->setRange(0, max - min);
            mRuler->setCurrent(0, max - min);
            
        }
    }
    else if(mTabs->currentIndex() == 3)
    {
        mRuler->clearAreas();
        mRuler->setRange(0, 100);
        mRuler->setCurrent(0, 100);
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
            mRuler->setCurrent(min, max);
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
   // mRuler->setZoom(mXSlider->value());
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
                e->mName  = event[STATE_NAME].toString();
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
    if (mByEventsBut->isChecked()) {
        QList<QRect> geometries = getGeometries(mByEventsGraphs, open, false);
        for(int i=0; i<mByEventsGraphs.size(); ++i)
            mByEventsGraphs[i]->toggle(geometries[i]);
    }
    
    if (mByPhasesBut->isChecked()) {
        QList<QRect> geometries = getGeometries(mByPhasesGraphs, open, true);
        for(int i=0; i<mByPhasesGraphs.size(); ++i)
            mByPhasesGraphs[i]->toggle(geometries[i]);
    }
    
    
    //updateScrollHeights();
    
    if(open)
    {
        updateScrollHeights();
    }
    else
    {
        mTimer->start(200);
    }
    mUnfoldBut->repaint();
    updateLayout();
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
    //  define ScrollArea
    enum ScrollArrea{
        eScrollPhases = 0,
        eScrollEvents= 1
    };
    
    ScrollArrea witchScroll;
    bool printAxis = false;
    
    QWidget* curWid;
    
    if (mStack->currentWidget() == mPhasesScrollArea) {
        curWid = mPhasesScrollArea->widget();
        curWid->setFont(mByPhasesGraphs[0]->font());
        witchScroll = eScrollPhases;
        printAxis = (mByPhasesGraphs[0]->mGraph->getXAxisMode() == GraphView::eHidden);
        //  hide all buttons in the both scrollAreaWidget
        for(int i=0; i<mByPhasesGraphs.size(); ++i){
            mByPhasesGraphs[i]->mButtonVisible = false;
            mByPhasesGraphs[i]->update();
        }
    }
    //else if (mStack->currentWidget() == mEventsScrollArea) {
    else  {
        curWid = mEventsScrollArea->widget();
        curWid->setFont(mByEventsGraphs[0]->font());
        witchScroll = eScrollEvents;
        printAxis = (mByEventsGraphs[0]->mGraph->getXAxisMode() == GraphView::eHidden);
        //  hide all buttons in the both scrollAreaWidget
        for(int i=0; i<mByEventsGraphs.size(); ++i) {
            mByEventsGraphs[i]->mButtonVisible = false;
            mByEventsGraphs[i]->update();
        }
    }




    // --------------------------------------------------------------------
    // Force rendering to HD for export
    int rendering = mRenderCombo->currentIndex();
    updateRendering(1);
    
    //QWidget* curWid = (mStack->currentWidget() == mPhasesScrollArea) ? mPhasesScrollArea->widget() : mEventsScrollArea->widget();
    
    // boolprintAxis=  (mStack->currentWidget() == mPhasesScrollArea) ? (mByPhasesGraphs[0]->mGraph->getXAxisMode()!= GraphView::eHidden) : (mByEventsGraphs[0]->mGraph->getXAxisMode() != GraphView::eHidden);
  
    //qDebug()<<"printAxis"<<printAxis;
    //qDebug()<<"printAxis"<<mByPhasesGraphs[0]->mGraph->getXAxisMode();
    //mPhasesScrollArea->
    
    AxisWidget* axisWidget = 0;
    QLabel* axisLegend = 0;
    int axeHeight = 20;
    int legendHeight = 20;
    
    if (printAxis) {
        curWid->setFixedHeight(curWid->height() + axeHeight + legendHeight);
        
        FormatFunc f = 0;
        if(mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked())
            f = DateUtils::convertToAppSettingsFormatStr;
        
        axisWidget = new AxisWidget(f, curWid);
        axisWidget->mMarginLeft = 50;
        axisWidget->mMarginRight = 10;
        axisWidget->setGeometry(0, curWid->height() - axeHeight, curWid->width(), axeHeight);
        axisWidget->mShowText = true;
        axisWidget->setAutoFillBackground(true);
        axisWidget->mShowSubs = true;
        axisWidget->mShowSubSubs = true;
        axisWidget->mShowArrow = true;
        axisWidget->mShowText = true;
        axisWidget->updateValues(curWid->width() - axisWidget->mMarginLeft - axisWidget->mMarginRight, axeHeight, mResultCurrentMinX, mResultCurrentMaxX);
        axisWidget->raise();
        axisWidget->setVisible(true);
        
        axisLegend = new QLabel(DateUtils::getAppSettingsFormat(), curWid);
        axisLegend->setGeometry(0, curWid->height() - axeHeight - legendHeight, curWid->width(), legendHeight);
        axisLegend->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        axisLegend->raise();
        axisLegend->setVisible(true);
    }
    
    QFileInfo fileInfo = saveWidgetAsImage(curWid,
                                           QRect(0, 0, curWid->width() , curWid->height()),
                                           tr("Save graph image as..."),
                                           MainWindow::getInstance()->getCurrentPath(), MainWindow::getInstance()->getAppSettings());
    
    // Delete additional widgets if necessary :
    if (printAxis) {
        if (axisWidget) {
            axisWidget->setParent(0);
            delete axisWidget;
        }
        if (axisLegend) {
            axisLegend->setParent(0);
            delete axisLegend;
        }
        curWid->setFixedHeight(curWid->height() - axeHeight - legendHeight);
    }
    
    
    // Revert to default display :
    
    if(fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
    
    // Reset rendering back to its current value
    updateRendering(rendering);

    //  show all buttons
    if (mByPhasesBut->isChecked()) {
        for(int i=0; i<mByPhasesGraphs.size(); ++i) {
            mByPhasesGraphs[i]->mButtonVisible = true;
            mByPhasesGraphs[i]->update();
        }
        
    }
    else {
        for(int i=0; i<mByEventsGraphs.size(); ++i) {
            mByEventsGraphs[i]->mButtonVisible = true;
            mByEventsGraphs[i]->update();
        }
    }
}

void ResultsView::updateZoomX()
{
    int zoom = mXSlider->value();
    
    // Ici, 10 correspond à la différence minimale de valeur (quand le zoom est le plus fort)
    double minProp = 10 / (mResultMaxX - mResultMinX);
    double zoomProp = (100. - zoom) / 100.;
    if(zoomProp < minProp)
        zoomProp = minProp;
    zoom = 100 * (1 - zoomProp);
    
    mResultZoomX = (double)zoom;
    double span = (mResultMaxX - mResultMinX)* (100-mResultZoomX)/100;
    double mid = (mResultCurrentMaxX + mResultCurrentMinX)/2;
    double curMin = mid - span/2;
    double curMax = mid + span/2;
    if (curMin < mResultMinX) {
        curMin = mResultMinX;
        curMax = curMin + span;
    }
    if (curMax> mResultMaxX) {
        curMax = mResultMaxX;
        curMin = curMax - span;
    }
    
    mResultCurrentMinX = ceil(curMin);
    mResultCurrentMaxX = floor(curMax);
    
    if(zoom == 0){
        mResultCurrentMinX = mResultMinX;
        mResultCurrentMaxX = mResultMaxX;
    }
    
    
    // update Current Min Max lineEdit
    if(mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked()){
        mCurrentXMinEdit->setText(DateUtils::convertToAppSettingsFormatStr(mResultCurrentMinX));
        mCurrentXMaxEdit->setText(DateUtils::convertToAppSettingsFormatStr(mResultCurrentMaxX));
    }else{
        mCurrentXMinEdit->setText(QString::number(mResultCurrentMinX));
        mCurrentXMaxEdit->setText(QString::number(mResultCurrentMaxX));
    }
    
    
    // change Ruler
    
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    //mRuler->setZoom(int (zoom)); // unusefull
    
    // update graphes
    updateAllZoom();
}

// signal from the Ruler
void ResultsView::updateScroll(const double min, const double max)
{
    mResultCurrentMinX = min;
    mResultCurrentMaxX = max;
    
    // update Current Min Max lineEdit
    /*
    mCurrentXMinEdit->setText(QString::number(mResultCurrentMinX));
    mCurrentXMaxEdit->setText(QString::number(mResultCurrentMaxX));
    */
    mCurrentXMinEdit->setText(DateUtils::convertToAppSettingsFormatStr(mResultCurrentMinX));
    mCurrentXMaxEdit->setText(DateUtils::convertToAppSettingsFormatStr(mResultCurrentMaxX));
    
    // Don't need to change XSlider
    
      updateAllZoom();
   
}

void ResultsView::updateRuler(int value)
{
    int scale = (mResultMaxX-mResultMinX)/mResultZoomX;
    double min = mResultCurrentMinX - scale* (value);
    double max = mResultCurrentMaxX + scale* ( value);
    mResultCurrentMinX = inRange(mResultMinX, min, mResultCurrentMaxX);
    mResultCurrentMinX = inRange(mResultCurrentMinX, max, mResultMaxX);
   
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    
}

void ResultsView::updateScaleY(int value)
{
    
    double min = 20;
    double max = 1020;
    double prop = value / 100.f;
    mGraphsH = min + prop * (max - min);
    //mYSlider->();
   // updateLayout();
   // repaint();
    updateAllZoom();
}
void ResultsView::editCurrentMinX()
{
    QString str = mCurrentXMinEdit->text();
    bool isNumber;
    double value = str.toDouble(&isNumber);
    if (isNumber) {
        bool isDate = mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked();
        if(isDate){
            value = DateUtils::convertFromAppSettingsFormat(value);
        }
        
        double current = inRange(mResultMinX, value, mResultCurrentMaxX);
        /*if (current == mResultCurrentMaxX) {
            return;
        }*/
        
        
        mResultCurrentMinX = current;
        if(isDate){
            mCurrentXMinEdit->setText(DateUtils::convertToAppSettingsFormatStr(mResultCurrentMinX));
        }else{
            mCurrentXMinEdit->setText(QString::number(mResultCurrentMinX));
        }
        
        mResultZoomX =(mResultCurrentMaxX - mResultCurrentMinX)/ (mResultMaxX - mResultMinX) * 100;
    
        // move XSlider position
        int zoom = int(100-mResultZoomX);
        mXSlider->setValue(zoom);
        
    
        // change Ruler
    
        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    
        // update graphes
   
        updateAllZoom();
    
    }
}

void ResultsView::setCurrentMinX()
{
    //QString str = mCurrentXMaxEdit->text();
    bool isNumber;
    double value = mCurrentXMinEdit->text().toDouble(&isNumber);
    if (isNumber) {
        
        mResultCurrentMinX = inRange(mResultMinX, value, mResultCurrentMaxX);
        
        
        mResultZoomX =(mResultCurrentMaxX - mResultCurrentMinX)/ (mResultMaxX - mResultMinX) * 100;
        
        // move XSlider position
        int zoom = int(100-mResultZoomX);
        mXSlider->setValue(zoom);
        
        //mXSlider->setTracking(false);
        //mXSlider->setSliderPosition(zoom);// setSliderPosition function don't notify the valueChanged signal when trackin=false
        //mXSlider->setTracking(true);
        
        // change Ruler
        
        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
        
        // update graphes

        updateAllZoom();
  
   }
}

void ResultsView::editCurrentMaxX()
{
    QString str = mCurrentXMaxEdit->text();
    bool isNumber;
    double value = str.toDouble(&isNumber);
    
    if (isNumber) {
        bool isDate = mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked();
        if(isDate){
            value = DateUtils::convertFromAppSettingsFormat(value);
        }
        double current = inRange(mResultCurrentMinX, value, mResultMaxX);
        if (current == mResultCurrentMinX) {
            return;
        }
        mResultCurrentMaxX = current;
        
        if(isDate){
            mCurrentXMaxEdit->setText(DateUtils::convertToAppSettingsFormatStr(mResultCurrentMaxX));
        }else{
            mCurrentXMaxEdit->setText(QString::number(mResultCurrentMaxX));
        }
        
        mResultZoomX =(mResultCurrentMaxX - mResultCurrentMinX)/ (mResultMaxX - mResultMinX)* 100;
        
        // move XSlider position
        int zoom = int(100-mResultZoomX);
        
        mXSlider->setValue(zoom);
      
        // change Ruler
        
        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
        
        // update graphes
        
        updateAllZoom();
        
    }
}
void ResultsView::setCurrentMaxX()
{
    //QString str = mCurrentXMaxEdit->text();
    bool isNumber;
    double value = mCurrentXMaxEdit->text().toDouble(&isNumber);
    if (isNumber) {
        double current = inRange(mResultCurrentMinX, value, mResultMaxX);
        if (current==mResultCurrentMinX) {
            return;
        }
        mResultCurrentMaxX = inRange(mResultCurrentMinX, value, mResultMaxX);
        
        mResultZoomX =(mResultCurrentMaxX - mResultCurrentMinX)/ (mResultMaxX - mResultMinX)* 100;
        
        // move XSlider position
        int zoom = int(100-mResultZoomX);
         mXSlider->setTracking(false);
        /* mXSlider->setSliderPosition(zoom);
        mXSlider->setTracking(true); */
        mXSlider->setValue(zoom);
        // update Current Min Max lineEdit
        mCurrentXMinEdit->setText(QString::number(mResultCurrentMinX));
        mCurrentXMaxEdit->setText(QString::number(mResultCurrentMaxX));
        
        // change Ruler
        
        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
        
        // update graphes

        updateAllZoom();
        
    }
}

void ResultsView::updateRendering(int index)
{
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i] -> setRendering((GraphView::Rendering) index);
    
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i] -> setRendering((GraphView::Rendering) index);
}

void ResultsView::showByPhases(bool)
{
    updateGraphs();
    updateLayout();
    mByPhasesBut -> setChecked(true);
    mStack -> setCurrentWidget(mPhasesScrollArea);
    mShowDataUnderPhasesCheck -> setVisible(true);

}

void ResultsView::showByEvents(bool)
{
    updateGraphs();
    updateLayout();
    mByEventsBut -> setChecked(true);
    mStack -> setCurrentWidget(mEventsScrollArea);
    mShowDataUnderPhasesCheck -> setVisible(false);
}

void ResultsView::changeTab(int index)
{
    mPostDistOptsTitle -> setVisible(index == 0);
    mPostDistGroup     -> setVisible(index == 0);
    mDataCalibCheck    -> setVisible(index == 0);
    mWiggleCheck       -> setVisible(index == 0);
    mAllChainsCheck    -> setVisible(index == 0);
    mDataCalibCheck    -> setVisible(index == 0);
    
    ProjectSettings s = mSettings;

    if(mTabs->currentIndex() == 0){
        mCurrentTypeGraph = eHisto;
        if(mDataThetaRadio->isChecked()) {
            mResultCurrentMinX = s.mTmin;
            mResultMinX = s.mTmin;
            mResultCurrentMaxX = s.mTmax;
            mResultMaxX = s.mTmax;
        }
        else if(mDataSigmaRadio->isChecked())  {
            mResultCurrentMinX = 0;
            mResultMinX = 0;
            mResultCurrentMaxX = s.mTmax - s.mTmin;
            mResultMaxX = s.mTmax - s.mTmin;
        }
    }
    else if(mTabs->currentIndex() == 1) {
        mCurrentTypeGraph = eTrace;
        
        mResultCurrentMinX = 0;
        mResultMinX = 0;
        
        int curChainIdx = -1;
        for(int i=0; i<mChainRadios.size(); ++i)
            if(mChainRadios[i]->isChecked())
                curChainIdx = i;
        if(curChainIdx != -1)  {
            mResultCurrentMaxX = mChains[curChainIdx].mNumBurnIter + (mChains[curChainIdx].mBatchIndex * mChains[curChainIdx].mNumBatchIter) + mChains[curChainIdx].mNumRunIter / mChains[curChainIdx].mThinningInterval;
            mResultMaxX = mResultCurrentMaxX;
        }

    }
    else if(mTabs->currentIndex() == 2) {
        mCurrentTypeGraph = eAccept;
        int curChainIdx = -1;
        for(int i=0; i<mChainRadios.size(); ++i)
            if(mChainRadios[i]->isChecked())
                curChainIdx = i;
        
        if(curChainIdx != -1)  {
            mResultCurrentMinX = 0;
            mResultMinX = 0;
            mResultCurrentMaxX = mChains[curChainIdx].mNumBurnIter + (mChains[curChainIdx].mBatchIndex * mChains[curChainIdx].mNumBatchIter) + mChains[curChainIdx].mNumRunIter / mChains[curChainIdx].mThinningInterval;
            mResultMaxX = mResultCurrentMaxX;
        }
        
    }
    else if(mTabs->currentIndex() == 3) {
        mCurrentTypeGraph = eCorrel;
        mResultCurrentMinX = 0;
        mResultMinX = 0;
        mResultCurrentMaxX = 100;
        mResultMaxX = 100;
        
    }
    mXSlider->setValue(1);
    mResultZoomX     = 1;
    
    mRuler->setRange(mResultMinX, mResultMaxX);
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    updateRulerAreas();
    
    mCurrentXMinEdit->setText( QString::number(mResultMinX) );
    mCurrentXMaxEdit->setText( QString::number(mResultMaxX) );
    
    
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
    updateAllZoom();
    //updateLayout();
    
}
//void clearRawHisto();
//void clearChainHisto();
void ResultsView::clearHisto()
{
    for(int i=0; i<mModel->mEvents.size(); ++i) {
        Event* event = mModel->mEvents[i];
        for(int j=0; j<mModel->mEvents[i]->mDates.size(); ++j) {
            Date& date = event->mDates[j];
            date.mTheta.mHisto.clear();
            date.mSigma.mHisto.clear();
        }
        event->mTheta.mHisto.clear();
    }
    
    for(int i=0; i<mModel->mPhases.size(); ++i) {
        Phase* phase = mModel->mPhases[i];
        phase->mAlpha.mHisto.clear();
        phase->mBeta.mHisto.clear();
    }
}
void ResultsView::clearChainHistos()
{
    for(int i=0; i<mModel->mEvents.size(); ++i) {
        Event* event = mModel->mEvents[i];
        for(int j=0; j<mModel->mEvents[i]->mDates.size(); ++j) {
            Date& date = event->mDates[j];
            date.mTheta.mChainsHistos.clear();
            date.mSigma.mChainsHistos.clear();
        }
        event->mTheta.mChainsHistos.clear();
    }
    
    for(int i=0; i<mModel->mPhases.size(); ++i) {
        Phase* phase = mModel->mPhases[i];
        phase->mAlpha.mHisto.clear();
        phase->mBeta.mHisto.clear();
    }
}
void ResultsView::clearRawHisto()
{
  
        for(int i=0; i<mModel->mEvents.size(); ++i) {
            Event* event = mModel->mEvents[i];
            for(int j=0; j<mModel->mEvents[i]->mDates.size(); ++j) {
                Date& date = event->mDates[j];
                date.mTheta.mRawHisto.clear();
                date.mSigma.mRawHisto.clear();
            }
            event->mTheta.mRawHisto.clear();
        }
        
        for(int i=0; i<mModel->mPhases.size(); ++i) {
            Phase* phase = mModel->mPhases[i];
            phase->mAlpha.mRawHisto.clear();
            phase->mBeta.mRawHisto.clear();
        }
    
}
void ResultsView::clearCredibilityAndHPD()
{
    for(int i=0; i<mModel->mEvents.size(); ++i) {
        Event* event = mModel->mEvents[i];
        for(int j=0; j<mModel->mEvents[i]->mDates.size(); ++j) {
            Date& date = event->mDates[j];
            date.mTheta.mHPD.clear();
            date.mSigma.mHPD.clear();
            
        }
        event->mTheta.mHPD.clear();
    }
    
    for(int i=0; i<mModel->mPhases.size(); ++i) {
        Phase* phase = mModel->mPhases[i];
        phase->mAlpha.mHPD.clear();
        phase->mBeta.mHPD.clear();
    }
}