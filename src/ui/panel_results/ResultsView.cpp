
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

#include "../PluginAbstract.h"

#include <QtWidgets>
#include <iostream>
#include <QtSvg>

#pragma mark Constructor & Destructor
ResultsView::ResultsView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mResultMaxVariance(1000),
mHasPhases(false),
mModel(0),
mMargin(5),
mOptionsW(200),
mLineH(15),
mGraphLeft(130),
mRulerH(40),
mTabsH(30),
mGraphsH(130)
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
    mTabs->setTab(0, false);
    
    // -------------
    
    mStack = new QStackedWidget(this);
    //connect(mStack, SIGNAL(clicked()), this, SLOT(updateResults()));// don't work
    
    mEventsScrollArea = new QScrollArea();
    mEventsScrollArea->setMouseTracking(true);
    mStack->addWidget(mEventsScrollArea);
    
    mPhasesScrollArea = new QScrollArea();
    mPhasesScrollArea->setMouseTracking(true);
    mStack->addWidget(mPhasesScrollArea);
    
    mMarker = new Marker(this);
    
    setMouseTracking(true);
    mStack->setMouseTracking(true);
    
    // ----------
    mByPhasesBut = new Button(tr("Phases"), this);
    mByPhasesBut->setCheckable(true);
    mByPhasesBut->setFlatHorizontal();
    
    mByEventsBut = new Button(tr("Events"), this);
    mByEventsBut->setCheckable(true);
    mByEventsBut->setFlatHorizontal();
    
    QButtonGroup* butGroup = new QButtonGroup(this);
    butGroup->addButton(mByPhasesBut);
    butGroup->addButton(mByEventsBut);
    butGroup->setExclusive(true);
    
    // -------------------------
    
    
    mUnfoldBut = new Button(tr("Unfold"), this);
    mUnfoldBut->setCheckable(true);
    mUnfoldBut->setFlatHorizontal();
    mUnfoldBut->setIcon(QIcon(":unfold.png"));
    mUnfoldBut->setToolTip(tr("Display event's data or phase's events, depending on the chosen layout."));
    
    mInfosBut = new Button(tr("Stats"));
    mInfosBut->setCheckable(true);
    mInfosBut->setFlatHorizontal();
    mInfosBut->setIcon(QIcon(":stats_w.png"));
    mInfosBut->setFixedHeight(50);
    mInfosBut->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));
    
    mExportResults = new Button(tr("Results"));
    mExportResults->setFlatHorizontal();
    mExportResults->setIcon(QIcon(":csv.png"));
    mExportResults->setFixedHeight(50);

    
    mExportImgBut = new Button(tr("Capture"));
    mExportImgBut->setFlatHorizontal();
    mExportImgBut->setIcon(QIcon(":picture_save.png"));
    mExportImgBut->setFixedHeight(50);
    mExportImgBut->setToolTip(tr("Save all currently visible results as an image.<br><u>Note</u> : If you want to copy textual results, see the Log tab."));
    
    QHBoxLayout* displayButsLayout = new QHBoxLayout();
    displayButsLayout->setContentsMargins(0, 0, 0, 0);
    displayButsLayout->setSpacing(0);
    displayButsLayout->addWidget(mInfosBut);
    displayButsLayout->addWidget(mExportImgBut);
    displayButsLayout->addWidget(mExportResults);
    
    
    mRuler = new Ruler(this);
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

    mCurrentXMaxEdit = new LineEdit(mDisplayGroup);
    mCurrentXMaxEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mCurrentXMaxEdit->setAlignment(Qt::AlignHCenter);
    mCurrentXMaxEdit->setFixedSize(45, 15);
    
    
    mXScaleLab = new Label(tr("Zoom X"), mDisplayGroup);
    mYScaleLab = new Label(tr("Zoom Y"), mDisplayGroup);
    
    mXScaleLab->setAlignment(Qt::AlignCenter);
    mYScaleLab->setAlignment(Qt::AlignCenter);
    
    mXSlider = new QSlider(Qt::Horizontal,mDisplayGroup);
    mXSlider->setRange(0, 100);
    mXSlider->setTickInterval(1);
    
    mYSlider = new QSlider(Qt::Horizontal,mDisplayGroup);
    mYSlider->setRange(0, 100);
    mYSlider->setTickInterval(1);
    mYSlider->setValue(13);
    
    
       /*  keep in memory
    mUpdateDisplay = new Button(tr("Update display"),mScaleGroup);
    mUpdateDisplay->mUseMargin = true;
    
    connect(mUpdateDisplay, SIGNAL(clicked()), this, SLOT(updateModel()));
   */

    mRenderCombo = new QComboBox();
    mRenderCombo->addItem(tr("Standard (faster)"));
    mRenderCombo->addItem(tr("Retina (slower)"));
    
    mFont.setPointSize(pointSize(11.));
    mFontBut = new QPushButton(mFont.family());
    connect(mFontBut, SIGNAL(clicked()), this, SLOT(updateFont()));
    
    mThicknessSpin = new QSpinBox();
    mThicknessSpin->setRange(1, 10);
    mThicknessSpin->setSuffix(" px");
    connect(mThicknessSpin, SIGNAL(valueChanged(int)), this, SLOT(updateThickness(int)));
    
    mOpacitySpin = new QSpinBox();
    mOpacitySpin->setRange(0, 100);
    mOpacitySpin->setValue(50);
    mOpacitySpin->setSuffix(" %");
    connect(mOpacitySpin, SIGNAL(valueChanged(int)), this, SLOT(updateOpacity(int)));
    
    // Grid : 8 columns
    QGridLayout* displayLayout = new QGridLayout();
    displayLayout->setContentsMargins(0, 0, 0, 0);
    displayLayout->setSpacing(6);
    displayLayout->addWidget(mCurrentXMinEdit,0,0,1,2);
    displayLayout->addWidget(mCurrentXMaxEdit,0,6,1,2);
    displayLayout->addWidget(mXScaleLab,0,2,1,4);
    displayLayout->addWidget(mXSlider,1,0,1,8);
    displayLayout->addWidget(mYScaleLab,2,0,1,8);
    displayLayout->addWidget(mYSlider,3,0,1,8);
    
    QLabel* labFont = new QLabel(tr("Font :"));
    QLabel* labThickness = new QLabel(tr("Thickness :"));
    QLabel* labOpacity = new QLabel(tr("Fill Opacity :"));
    QLabel* labRendering = new QLabel(tr("Rendering :"));
    
    labFont->setFont(mFont);
    labThickness->setFont(mFont);
    labOpacity->setFont(mFont);
    labRendering->setFont(mFont);
    
    QFormLayout* displayForm = new QFormLayout();
    displayForm->setContentsMargins(0, 0, 0, 0);
    displayForm->setSpacing(6);
    displayForm->addRow(labFont, mFontBut);
    displayForm->addRow(labThickness, mThicknessSpin);
    displayForm->addRow(labOpacity, mOpacitySpin);
    displayForm->addRow(labRendering, mRenderCombo);
    
    QVBoxLayout* displayLayoutWrapper = new QVBoxLayout();
    displayLayoutWrapper->setContentsMargins(6, 6, 6, 6);
    displayForm->setSpacing(0);
    displayLayoutWrapper->addLayout(displayLayout);
    displayLayoutWrapper->addLayout(displayForm);
    displayLayoutWrapper->addStretch();
    
    mDisplayGroup->setLayout(displayLayoutWrapper);
    mDisplayGroup->setFixedHeight(200);

    
    /* -------------------------------------- mChainsGroup---------------------------------------------------*/
    mChainsGroup = new QWidget();
    
    mChainsTitle = new Label(tr("MCMC Chains"));
    mChainsTitle->setIsTitle(true);
    
    mAllChainsCheck = new CheckBox(tr("Chains concatenation"), mChainsGroup);
    mAllChainsCheck->setChecked(true);
    mChainsGroup->setFixedHeight(2*mMargin + mLineH);
    
    /* -------------------------------------- mResultsGroup---------------------------------------------------*/

    mResultsGroup = new QWidget();
    
    mResultsTitle = new Label(tr("Results options"));
    mResultsTitle->setIsTitle(true);
    
    
    mDataThetaRadio = new RadioButton(tr("Calendar dates"), mResultsGroup);
    mDataSigmaRadio = new RadioButton(tr("Individual std. deviations"), mResultsGroup);
    
    mShowDataUnderPhasesCheck = new CheckBox(tr("Unfold data under Event"), mResultsGroup);
    mDataCalibCheck           = new CheckBox(tr("Individual calib. dates"), mResultsGroup);

    mWiggleCheck              = new CheckBox(tr("Wiggle shifted"), mResultsGroup);
    mDataThetaRadio           -> setChecked(true);
    mDataCalibCheck           -> setChecked(true);
    mShowDataUnderPhasesCheck -> setChecked(false);
   
    /* -------------------------------------- mPostDistGroup ---------------------------------------------------*/
    
    mPostDistOptsTitle = new Label(tr("Post. distrib. options"));
    mPostDistOptsTitle->setIsTitle(true);
    mPostDistGroup = new QWidget();
    
    mCredibilityCheck = new CheckBox(tr("Show credibility"), mPostDistGroup);
    mCredibilityCheck->setChecked(true);
    mThreshLab = new Label(tr("HPD / Credibility (%)") + " :", mPostDistGroup);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    mHPDEdit = new LineEdit(mPostDistGroup);
    mHPDEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mHPDEdit->setText("95");
    
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator -> setBottom(0.);
    percentValidator -> setTop(100.);
    percentValidator -> setDecimals(1);
    mHPDEdit->setValidator(percentValidator);
    
    mFFTLenLab = new Label(tr("Grid length") + " :", mPostDistGroup);
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
    
    mComboH = mFFTLenCombo->sizeHint().height();
    mTabsH = mComboH + 2*mMargin;
    
    mHFactorLab = new Label(tr("Bandwidth const.") + " :", mPostDistGroup);
    mHFactorLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mHFactorEdit = new LineEdit(mPostDistGroup);
    QLocale locale;
    mHFactorEdit->setText(locale.toString(1.06));
    mHFactorEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    
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
    
    connect(this, SIGNAL(posteriorDistribGenerated()), this, SLOT(generateCredibilityAndHPD()));
    connect(this, SIGNAL(credibilityAndHPDGenerated()), this, SLOT(generateCurves()));
    connect(this, SIGNAL(curvesGenerated()), this, SLOT(updateCurvesToShow()));
    
    connect(this, SIGNAL(controlsUpdated()), this, SLOT(updateLayout()));
    
    // -------------------------
    
    connect(mTabs, SIGNAL(tabClicked(int)), this, SLOT(updateControls()));
    connect(mByPhasesBut, SIGNAL(clicked()), this, SLOT(updateControls()));
    connect(mByEventsBut, SIGNAL(clicked()), this, SLOT(updateControls()));
    
    connect(mTabs, SIGNAL(tabClicked(int)), this, SLOT(generateCurves()));
    connect(mByPhasesBut, SIGNAL(clicked()), this, SLOT(generateCurves()));
    connect(mByEventsBut, SIGNAL(clicked()), this, SLOT(generateCurves()));
    
    // -------------------------
    
    connect(mFFTLenCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(generatePosteriorDistribs()));
    connect(mHFactorEdit, SIGNAL(editingFinished()), this, SLOT(generatePosteriorDistribs()));
    
    connect(mHPDEdit, SIGNAL(editingFinished()), this, SLOT(generateCredibilityAndHPD()));
    
    connect(mDataThetaRadio,     SIGNAL(clicked()), this, SLOT(generateCurves()));
    connect(mDataSigmaRadio,     SIGNAL(clicked()), this, SLOT(generateCurves()));
    
    connect(mDataCalibCheck,     SIGNAL(clicked()), this, SLOT(updateCurvesToShow()));
    connect(mWiggleCheck,        SIGNAL(clicked()), this, SLOT(updateCurvesToShow()));
    connect(mAllChainsCheck,     SIGNAL(clicked()), this, SLOT(updateCurvesToShow()));
    connect(mCredibilityCheck,   SIGNAL(clicked()), this, SLOT(updateCurvesToShow()));
    
    // -------------------------
    //connect(mUnfoldBut, &Button::toggled, this, &ResultsView::createEventsScrollArea);
    connect(mUnfoldBut, &Button::toggled, this, &ResultsView::updateGraphsLayout);
    connect(mShowDataUnderPhasesCheck, &CheckBox::toggled, this, &ResultsView::updateGraphsLayout);
    
    // -------------------------
    
    connect(mXSlider, SIGNAL(sliderMoved(int)), this, SLOT(updateZoomX()));
    connect(mXSlider, SIGNAL(sliderPressed()), this, SLOT(updateZoomX()));
    connect(mCurrentXMinEdit, SIGNAL(editingFinished()), this, SLOT(editCurrentMinX()) );
    connect(mCurrentXMaxEdit, SIGNAL(editingFinished()), this, SLOT(editCurrentMaxX()) );
    connect(mRuler, SIGNAL(positionChanged(double, double)), this, SLOT(updateScroll(double, double)));
    
    connect(mYSlider, SIGNAL(valueChanged(int)), this, SLOT(updateScaleY(int)));
    
    // -------------------------
    
    connect(mRenderCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRendering(int)));
    connect(mInfosBut, SIGNAL(toggled(bool)), this, SLOT(showInfos(bool)));
    connect(mExportImgBut, SIGNAL(clicked()), this, SLOT(exportFullImage()));
    connect(mExportResults, SIGNAL(clicked()), this, SLOT(exportResults()));
    
    mMarker->raise();
}

ResultsView::~ResultsView()
{
    
}

void ResultsView::doProjectConnections(Project* project)
{
    // Starting MCMC calculation does a mModel.clear() at first, and recreate it.
    // Then, it fills its eleme,nts (events, ...) with calculated data (trace, ...)
    // If the process is canceled, we only have unfinished data in storage.
    // => The previous nor the new results can be displayed so we must start by clearing the results view!
    connect(project, SIGNAL(mcmcStarted()), this, SLOT(clearResults()));
    
}

#pragma mark Paint & Resize
void ResultsView::paintEvent(QPaintEvent* )
{
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
    bool showDates = mShowDataUnderPhasesCheck->isChecked();
    
    for(int i=0; i<graphs.size(); ++i)
    {
        QRect rect = graphs[i]->geometry();
        rect.setY(y);
        rect.setWidth(width() - mOptionsW - sbe);
        
        if(byPhases)
        {
            GraphViewPhase* graph = dynamic_cast<GraphViewPhase*>(graphs[i]);
            // It is a Phase Graph :
            if(graph) rect.setHeight(h);
            // Unfold is opened
            else if(open)
            {
                GraphViewEvent* graphEvent = dynamic_cast<GraphViewEvent*>(graphs[i]);
                // It is an Event Graph :
                if(graphEvent) rect.setHeight(h);
                // It is Date Graph :
                else if(showDates){
                    rect.setHeight(h);
                }
                else rect.setHeight(0);
            }
            else rect.setHeight(0);
        }
        else
        {
            GraphViewEvent* graph = dynamic_cast<GraphViewEvent*>(graphs[i]);
            // It is an Event Graph :
            if(graph) rect.setHeight(h);
            // It is an Date Graph and unfold is opened :
            else if(open) rect.setHeight(h);
            // It is an Date Graph and unfold is closed :
            else rect.setHeight(0);
        }
        y += rect.height();
        rects.append(rect);
    }
    return rects;
}

#pragma mark Options & Layout (Chained Functions)
void ResultsView::updateControls()
{
    qDebug() << "ResultsView::updateControls";
    
    int tabIdx = mTabs->currentIndex();
    bool byEvents = mByEventsBut->isChecked();
    
    if(tabIdx == 0) mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    else if(tabIdx == 1) mCurrentTypeGraph = GraphViewResults::eTrace;
    else if(tabIdx == 2) mCurrentTypeGraph = GraphViewResults::eAccept;
    else if(tabIdx == 3) mCurrentTypeGraph = GraphViewResults::eCorrel;
    
    // -------------------------------------------------------
    //  Activate specific controls for post. distrib. (first tab)
    // -------------------------------------------------------
    mAllChainsCheck    -> setVisible(tabIdx == 0);
    mDataCalibCheck    -> setVisible(tabIdx == 0);
    mWiggleCheck       -> setVisible(tabIdx == 0);
    mPostDistOptsTitle -> setVisible(tabIdx == 0);
    mPostDistGroup     -> setVisible(tabIdx == 0);
    
    // -------------------------------------------------------
    //  Switch between checkboxes or Radio-buttons for chains
    // -------------------------------------------------------
    if(tabIdx == 0)
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
    
    // -------------------------------------------------------
    //  Display by phases or by events
    // -------------------------------------------------------
    mStack->setCurrentWidget(byEvents ? mEventsScrollArea : mPhasesScrollArea);
    mShowDataUnderPhasesCheck->setVisible(!byEvents);
    
    emit controlsUpdated();
}

void ResultsView::updateLayout()
{
    if(!mModel) return;
    qDebug() << "ResultsView::updateLayout";
    
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int graphYAxis = 50;
    int m = mMargin;
    int dx = mLineH + mMargin;//m;
    
    int tabIdx = mTabs->currentIndex();
    
    if( (tabIdx == 0) and  !(mDataSigmaRadio->isChecked()) ) // // it's mean posterior distribution and not a value of sigma
    {
        mCurrentXMinEdit->setText( DateUtils::convertToAppSettingsFormatStr(mResultCurrentMinX) );
        mCurrentXMaxEdit->setText( DateUtils::convertToAppSettingsFormatStr(mResultCurrentMaxX) );
    }
    else {
        mCurrentXMinEdit->setText( QString::number(mResultCurrentMinX) );
        mCurrentXMaxEdit->setText( QString::number(mResultCurrentMaxX) );
    }
    
    // ----------------------------------------------------------
    //  Main layout
    // ----------------------------------------------------------
    mByPhasesBut->setGeometry(0, 0, (int)(mGraphLeft/2), mTabsH);
    mByEventsBut->setGeometry(mGraphLeft/2, 0, (int)(mGraphLeft/2), mTabsH);
    mUnfoldBut->setGeometry(0, mTabsH, mGraphLeft, mRulerH);
    
    mTabs   -> setGeometry(mGraphLeft + graphYAxis, 0, width() - mGraphLeft - mOptionsW - sbe - graphYAxis, mTabsH);
    mRuler  -> setGeometry(mGraphLeft, mTabsH, width() - mGraphLeft - mOptionsW - sbe, mRulerH);
    mStack  -> setGeometry(0, mTabsH + mRulerH, width() - mOptionsW, height() - mRulerH - mTabsH);
    mMarker -> setGeometry(mMarker->pos().x(), mTabsH + sbe, mMarker->thickness(), height() - sbe - mTabsH);
    
    mOptionsWidget -> setGeometry(this->width() - mOptionsW, 0, mOptionsW, this->height());
    
    // ----------------------------------------------------------
    //  Display Options options layout
    // ----------------------------------------------------------
    mDisplayGroup  -> setGeometry(0, mDisplayTitle->y()+ mDisplayTitle->height(), mOptionsW, mDisplayGroup->height());
    
    
    // ----------------------------------------------------------
    //  MCMC Chains options layout
    // ----------------------------------------------------------
    //   mOptionsWidget->move(width() - mOptionsW, 0);
    int numChains = mCheckChainChecks.size();
    
    // posterior distribution : chains are selectable with checkboxes
    if(tabIdx == 0)
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
    // trace, accept or correl : chains are selectable with radio-buttons
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
    
    // ----------------------------------------------------------
    //  Results options layout
    // ----------------------------------------------------------
    int y = m;
    mDataThetaRadio->setGeometry(m, y, (int)(mResultsGroup->width() - 2*m), mLineH);
    
    // posterior distribution
    if(tabIdx == 0)
    {
        mDataCalibCheck -> setGeometry(m + dx, y += (m + mLineH),(int) (mResultsGroup->width() - 2*m - dx), mLineH);
        mWiggleCheck    -> setGeometry(m + dx, y += (m + mLineH),(int)( mResultsGroup->width() - 2*m - dx), mLineH);
    }
    if(mShowDataUnderPhasesCheck->isVisible()) {
        mShowDataUnderPhasesCheck->setGeometry(m + dx, y += (m + mLineH),(int) (mResultsGroup->width() - 2*m - dx), mLineH);
    }
    mDataSigmaRadio -> setGeometry(m, y += (m + mLineH), mResultsGroup->width()-2*m, mLineH);
    mResultsGroup   -> setFixedHeight(y += (m + mLineH));
    
    // ----------------------------------------------------------
    //  Post. Distrib. options layout
    // ----------------------------------------------------------
    mPostDistGroup->setFixedWidth(mOptionsW);
    
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
    
    updateGraphsLayout();
}

void ResultsView::updateGraphsLayout()
{
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    
    // ----------------------------------------------------------
    //  Graphs by events layout
    // ----------------------------------------------------------
    QWidget* wid = mEventsScrollArea->widget();
    QList<QRect> geometries = getGeometries(mByEventsGraphs, mUnfoldBut->isChecked(), false);
    int h = 0;
    for(int i=0; i<mByEventsGraphs.size(); ++i)
    {
        mByEventsGraphs[i]->setGeometry(geometries[i]);
        h += geometries[i].height();
    }
    if(h>0)
        wid->setFixedSize(width() - sbe - mOptionsW, h);
    
    // ----------------------------------------------------------
    //  Graphs by phases layout
    // ----------------------------------------------------------
    wid = mPhasesScrollArea->widget();
    geometries = getGeometries(mByPhasesGraphs, mUnfoldBut->isChecked(), true);
    h = 0;
    for(int i=0; i<mByPhasesGraphs.size(); ++i){
        mByPhasesGraphs[i]->setGeometry(geometries[i]);
        h += geometries[i].height();
    }
    if(h>0)
        wid->setFixedSize(width() - sbe - mOptionsW, h);
}

#pragma mark Update (Chained Functions)
void ResultsView::clearResults()
{
    mByEventsBut->setEnabled(false);
    mByPhasesBut->setEnabled(false);
    
    for(int i=mCheckChainChecks.size()-1; i>=0; --i)
    {
        CheckBox* check = mCheckChainChecks.takeAt(i);
        disconnect(check, SIGNAL(clicked()), this, SLOT(updateCurvesToShow()));
        check->setParent(0);
        delete check;
    }
    mCheckChainChecks.clear();
    
    for(int i=mChainRadios.size()-1; i>=0; --i)
    {
        RadioButton* but = mChainRadios.takeAt(i);
        disconnect(but, SIGNAL(clicked()), this, SLOT(updateCurvesToShow()));
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

/*
 * @brief : This function is call after "Run" and after click on "Results", when switching from Model panel to Result panel
 *
 */
void ResultsView::updateResults(Model* model)
{
    clearResults();

    qDebug() << "ResultsView::updateResults";
    
    if(!mModel && !model)
        return;
    
    if(model)
        mModel = model;
    
    mChains = mModel->mChains;
    mSettings = mModel->mSettings;
    mMCMCSettings = mModel->mMCMCSettings;
    
    mHasPhases = (mModel->mPhases.size() > 0);
    
    mByPhasesBut->setChecked(mHasPhases);
    mByEventsBut->setChecked(!mHasPhases);
    
    mByEventsBut->setEnabled(mHasPhases);
    mByPhasesBut->setEnabled(mHasPhases);
    
    // ----------------------------------------------------
    //  Create Chains option controls (radio and checkboxes under "MCMC Chains")
    // ----------------------------------------------------
    for(int i=0; i<mChains.size(); ++i)
    {
        CheckBox* check = new CheckBox(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
        connect(check, SIGNAL(clicked()), this, SLOT(updateCurvesToShow()));
        check->setVisible(true);
        mCheckChainChecks.append(check);
        
        RadioButton* radio = new RadioButton(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
        connect(radio, SIGNAL(clicked()), this, SLOT(updateCurvesToShow()));
        radio->setVisible(true);
        if(i == 0)
            radio->setChecked(true);
        mChainRadios.append(radio);
    }
    
    // ----------------------------------------------------
    //  Phases Views : generate all phases graph
    //  No posterior density has been computed yet!
    //  Graphs are empty at the moment
    // ----------------------------------------------------

    createPhasesScrollArea();

    // ----------------------------------------------------
    //  Events Views : generate all phases graph
    //  No posterior density has been computed yet!
    //  Graphs are empty at the moment
    // ----------------------------------------------------

    createEventsScrollArea();
    // ------------------------------------------------------------
    
    showInfos(mInfosBut->isChecked());
    
    // ------------------------------------------------------------
    //  Controls have been reset (by events, first tab to show post. distribs...) :
    //  => updateControls() will call updateLayout()
    //  Graphs have been recreated :
    //  => updateLayout() will call updateGraphsLayout()
    // ------------------------------------------------------------
    updateControls();
    
    // ------------------------------------------------------------
    //  This generates post. densities, HPD and credibilities !
    //  It will then call in chain :
    //  - generateCredibilityAndHPD
    //  - generateCurves
    //  - updateCurvesToShow
    // ------------------------------------------------------------
    generatePosteriorDistribs();
}

void ResultsView::createEventsScrollArea()
{
     //int tabIdx = mTabs->currentIndex();
    //if(mTabs->currentIndex()==0) return; // this is a phase view
    QWidget* eventsWidget = new QWidget();
    eventsWidget->setMouseTracking(true);

    for(int i=0; i<(int)mModel->mEvents.size(); ++i)
    {
        Event* event = mModel->mEvents[i];
        GraphViewEvent* graphEvent = new GraphViewEvent(eventsWidget);

        // ----------------------------------------------------
        //  This just creates the view for the event.
        //  It sets the Event which triggers an update() to repaint the view.
        //  The refresh() function which actually creates the graph curves will be called later.
        // ----------------------------------------------------
        graphEvent->setSettings(mModel->mSettings);
        graphEvent->setMCMCSettings(mModel->mMCMCSettings, mChains);
        graphEvent->setEvent(event);
        graphEvent->setGraphFont(mFont);
        graphEvent->setGraphsThickness(mThicknessSpin->value());
        mByEventsGraphs.append(graphEvent);
       // if(mUnfoldBut->isChecked()) {
        if(event->mType != Event::eKnown) {
            for(int j=0; j<(int)event->mDates.size(); ++j) {
                Date& date = event->mDates[j];
                // ----------------------------------------------------
                //  This just creates the view for the date.
                //  It sets the Date which triggers an update() to repaint the view.
                //  The refresh() function which actually creates the graph curves will be called later.
                // ----------------------------------------------------
                GraphViewDate* graphDate = new GraphViewDate(eventsWidget);
                graphDate->setSettings(mModel->mSettings);
                graphDate->setMCMCSettings(mModel->mMCMCSettings, mChains);
                graphDate->setDate(&date);
                graphDate->setColor(date.getEventColor());//  event->getColor());

                graphDate->setGraphFont(mFont);
                graphDate->setGraphsThickness(mThicknessSpin->value());
                mByEventsGraphs.append(graphDate);
            }
       }
    }
    mEventsScrollArea->setWidget(eventsWidget);
}

void ResultsView::createPhasesScrollArea()
{
    QWidget* phasesWidget = new QWidget();
    phasesWidget->setMouseTracking(true);

    for(int p=0; p<(int)mModel->mPhases.size(); ++p)
    {
        // ----------------------------------------------------
        //  This just creates the GraphView for the phase (no curve yet)
        // ----------------------------------------------------
        Phase* phase = mModel->mPhases[p];
        GraphViewPhase* graphPhase = new GraphViewPhase(phasesWidget);
        connect(graphPhase, &GraphViewPhase::durationDisplay, this,&ResultsView::adjustDuration);

        graphPhase->setSettings(mModel->mSettings);
        graphPhase->setMCMCSettings(mModel->mMCMCSettings, mChains);
        graphPhase->setPhase(phase);
        graphPhase->setGraphFont(mFont);
        graphPhase->setGraphsThickness(mThicknessSpin->value());
        mByPhasesGraphs.append(graphPhase);

        for(int i=0; i<(int)phase->mEvents.size(); ++i)
        {
            // ----------------------------------------------------
            //  This just creates the GraphView for the event (no curve yet)
            // ----------------------------------------------------
            Event* event = phase->mEvents[i];
            GraphViewEvent* graphEvent = new GraphViewEvent(phasesWidget);
            graphEvent->setSettings(mModel->mSettings);
            graphEvent->setMCMCSettings(mModel->mMCMCSettings, mChains);
            graphEvent->setEvent(event);
            graphEvent->setGraphFont(mFont);
            graphEvent->setGraphsThickness(mThicknessSpin->value());
            mByPhasesGraphs.append(graphEvent);

            // --------------------------------------------------
            //  This just creates the GraphView for the date (no curve yet)
            // --------------------------------------------------
            for(int j=0; j<(int)event->mDates.size(); ++j)
            {
                Date& date = event->mDates[j];
                GraphViewDate* graphDate = new GraphViewDate(phasesWidget);
                graphDate->setSettings(mModel->mSettings);
                graphDate->setMCMCSettings(mModel->mMCMCSettings, mChains);
                graphDate->setDate(&date);
                graphDate->setGraphFont(mFont);
                graphDate->setGraphsThickness(mThicknessSpin->value());

                graphDate->setColor(event->mColor);
                mByPhasesGraphs.append(graphDate);
            }
        }
    }
    mPhasesScrollArea->setWidget(phasesWidget);

}

void ResultsView::generatePosteriorDistribs()
{
    qDebug() << "ResultsView::generatePosteriorDistribs";
    if(mModel)
    {
        QLocale locale;
        bool ok;
        int len = mFFTLenCombo->currentText().toInt();
        double hFactor = locale.toDouble(mHFactorEdit->text(),&ok);
        if(!(hFactor > 0 && hFactor <= 100) || !ok)
        {
            hFactor = 1.06;

            mHFactorEdit->setText(locale.toString(hFactor));

        }
        mModel->clearPosteriorDensities();
        
        mModel->generatePosteriorDensities(mChains, len, hFactor);
        mModel->generateNumericalResults(mChains);
        
        emit posteriorDistribGenerated();
    }
}

void ResultsView::generateCredibilityAndHPD()
{
    qDebug() << "ResultsView::generateCredibilityAndHPD";
    if(mModel)
    {
        QLocale locale;
        bool ok;
        QString input = mHPDEdit->text();
        mHPDEdit->validator()->fixup(input);
        mHPDEdit->setText(input);
        
        double_t hpd = locale.toDouble(mHPDEdit->text(),&ok);
        // ??? mModel->generateNumericalResults(mChains);
        if (!ok) hpd = 95;
        mModel->clearCredibilityAndHPD();
        mModel->generateCredibilityAndHPD(mChains, hpd);
        
        emit credibilityAndHPDGenerated();
    }
}

/**
 *  @brief re-generate all curves in graph views form model data.
 *  @brief Each curve is given a name. This name will be used by updateCurvesToShow() to decide whether the curve is visible or not.
 */
void ResultsView::generateCurves()
{
    qDebug() << "ResultsView::generateCurves";
    
    GraphViewResults::Variable variable;
    if(mDataThetaRadio->isChecked()) variable = GraphViewResults::eTheta;
    else if(mDataSigmaRadio->isChecked()) variable = GraphViewResults::eSigma;
    
    QList<GraphViewResults*>::iterator iter;
    QList<GraphViewResults*>::const_iterator iterEnd;

    if (mByPhasesBut->isChecked()) {
        iter = mByPhasesGraphs.begin();
        iterEnd = mByPhasesGraphs.constEnd();
    }
    else {
        iter = mByEventsGraphs.begin();
        iterEnd = mByEventsGraphs.constEnd();
    }

    while(iter != iterEnd) {
        (*iter)->generateCurves(GraphViewResults::TypeGraph(mCurrentTypeGraph), variable);
        ++iter;
    }

    // With variable eSigma, we look for mResultMaxVariance in the curve named "Post Distrib All Chains"
    if(variable == GraphViewResults::eSigma) {
        mResultMaxVariance = 0;

        QList<GraphViewResults*>::const_iterator constIter = mByEventsGraphs.constBegin();

        while(constIter != mByEventsGraphs.constEnd()) {
            GraphViewDate* graphDate = dynamic_cast<GraphViewDate*>(*constIter);
            if(graphDate) {
               GraphCurve* graphDuration = graphDate->mGraph->getCurve("Post Distrib All Chains");
               if(graphDuration) {
                   mResultMaxVariance = qMax(mResultMaxVariance, graphDuration->mData.lastKey());
               }
            }
            ++constIter;
        }

    }
    emit curvesGenerated();
}

/**
 *  @brief Decide which curve graphs must show, based on currently selected options.
 *  @brief This function does NOT remove or create any curve in graphs! It only checks if existing curves should be visible or not.
 */
void ResultsView::updateCurvesToShow()
{
    qDebug() << "ResultsView::updateCurvesToShow";
    
    ProjectSettings s = mSettings;
    MCMCSettings mcmc = mMCMCSettings;
    
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
    bool showCalib = mDataCalibCheck->isChecked();
    bool showWiggle = mWiggleCheck->isChecked();
    bool showCredibility = mCredibilityCheck->isChecked();
    
    for(int i=0; i<mByPhasesGraphs.size(); ++i){
        mByPhasesGraphs[i]->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
    }
    for(int i=0; i<mByEventsGraphs.size(); ++i){
        mByEventsGraphs[i]->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
    }
    
    updateResultsLog();
    updateScales();
}
/**
 *   @brief Restore zoom according to mTabs
 *   @todo Memory must containt mDataSigmaRadio state
 */
void ResultsView::updateScales()
{
    qDebug() << "ResultsView::updateScales";
    
    int tabIdx = mTabs->currentIndex();
    ProjectSettings s = mSettings;
    
    // ------------------------------------------
    //  Get X Range based on current options
    // ------------------------------------------
    if(tabIdx == 0){
        if(mDataThetaRadio->isChecked()) {
            mResultMinX = s.mTmin;
            mResultMaxX = s.mTmax;
        }
        else if(mDataSigmaRadio->isChecked())  {
            mResultMinX = 0;
            mResultMaxX = mResultMaxVariance;//s.mTmax - s.mTmin;

        }
    }
    else if(tabIdx == 1 || tabIdx == 2){
        mResultMinX = 0;
        for(int i=0; i<mChainRadios.size(); ++i){
            if(mChainRadios[i]->isChecked()){
                const Chain& chain = mChains[i];
                mResultMaxX = chain.mNumBurnIter + (chain.mBatchIndex * chain.mNumBatchIter) + chain.mNumRunIter / chain.mThinningInterval;
                break;
            }
        }
    }
    else if(tabIdx == 3) {
        mResultMinX = 0;
        mResultMaxX = 39;
    }
   
    // ------------------------------------------
    //  Restore last zoom values
    // ------------------------------------------
    if(mZooms.find(tabIdx) != mZooms.end()){
        mResultCurrentMinX = mZooms[tabIdx].first;
        mResultCurrentMaxX = mZooms[tabIdx].second;
        // controle if the current value is in rigth range depending to mDataThetaRadio and mDataSigmaRadio
        mResultCurrentMinX = qBound(mResultMinX, mResultCurrentMinX, mResultMaxX);
        mResultCurrentMaxX = qBound(mResultCurrentMinX, mResultCurrentMaxX, mResultMaxX);
    }else{
        
        mResultCurrentMinX = mResultMinX;
        mResultCurrentMaxX = mResultMaxX;
    }
    
    
    
    
    
    mResultZoomX = (mResultCurrentMaxX - mResultCurrentMinX) / (mResultMaxX - mResultMinX) * 100;
    
    
    
    // -----------------------------------------------
    //  Set All Graphs Ranges (This is not done by generateCurves !)
    // -----------------------------------------------
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
    {
        mByPhasesGraphs[i]->setRange(mResultMinX, mResultMaxX);
        mByPhasesGraphs[i]->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
    }
    for(int i=0; i<mByEventsGraphs.size(); ++i)
    {
        mByEventsGraphs[i]->setRange(mResultMinX, mResultMaxX);
        mByEventsGraphs[i]->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
    }
    
    // ------------------------------------------
    //  Set Zoom Slider & Zoom Edit
    // ------------------------------------------
    int zoom = 100 - (int)mResultZoomX;
    mXSlider->setValue(zoom);
    updateZoomEdit();
    
    // Already done when setting graphs new range (above)
    //updateGraphsZoomX();
    
    // ------------------------------------------
    //  Set Ruler Range
    // ------------------------------------------
    if( (tabIdx == 0) and (mDataThetaRadio->isChecked()) ){
            mRuler->setFormatFunctX(DateUtils::convertToAppSettingsFormatStr);
    }
    else {
        mRuler->setFormatFunctX(0);
    }
    mRuler->setRange(mResultMinX, mResultMaxX);
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    
    // ------------------------------------------
    //  Set Ruler Areas (Burn, Adapt, Run)
    // ------------------------------------------
    mRuler->clearAreas();
    if(tabIdx == 1 || tabIdx == 2){
        for(int i=0; i<mChainRadios.size(); ++i)
        {
            if(mChainRadios[i]->isChecked()){
                const Chain& chain = mChains[i];
                unsigned long adaptSize = chain.mBatchIndex * chain.mNumBatchIter;
                unsigned long runSize = chain.mNumRunIter / chain.mThinningInterval;
                
                mRuler->addArea(0, chain.mNumBurnIter, QColor(235, 115, 100));
                mRuler->addArea(chain.mNumBurnIter, chain.mNumBurnIter + adaptSize, QColor(250, 180, 90));
                mRuler->addArea(chain.mNumBurnIter + adaptSize, chain.mNumBurnIter + adaptSize + runSize, QColor(130, 205, 110));
                
                break;
            }
        }
    }
}

#pragma mark Log results
void ResultsView::settingChange()
{
    if(mModel){
        updateResults();
        updateResultsLog();
    }
}

void ResultsView::updateResultsLog()
{
    QString log;
    for(int i=0; i<mModel->mEvents.size(); ++i)
    {
        Event* event = mModel->mEvents[i];
        //log += ModelUtilities::eventResultsText(event, true);
        log += ModelUtilities::eventResultsHTML(event, true);
    }
    for(int i=0; i<mModel->mPhases.size(); ++i)
    {
        Phase* phase = mModel->mPhases[i];
        //log += ModelUtilities::phaseResultsText(phase);
        log += ModelUtilities::phaseResultsHTML(phase);
    }
    emit resultsLogUpdated(log);
}

#pragma mark Mouse & Marker
void ResultsView::mouseMoveEvent(QMouseEvent* e)
{
    int shiftX = 0;
    
    int x = e->pos().x() - shiftX;
    x = (x >= mGraphLeft) ? x : mGraphLeft;
    x = (x <= width() - mOptionsW) ? x : width() - mOptionsW;
    mMarker->setGeometry(x, mMarker->pos().y(), mMarker->width(), mMarker->height());
}

#pragma mark Zoom X
void ResultsView::updateZoomX()
{
    // --------------------------------------------------
    //  Find new current min & max, update mResultZoomX
    // --------------------------------------------------
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
    
    // --------------------------------------------------
    //  Update other elements
    // --------------------------------------------------
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    updateZoomEdit();
    updateGraphsZoomX();
}

// signal from the Ruler
void ResultsView::updateScroll(const double min, const double max)
{
    // --------------------------------------------------
    //  Find new current min & max
    // --------------------------------------------------
    mResultCurrentMinX = min;
    mResultCurrentMaxX = max;
    
    // --------------------------------------------------
    //  Update other elements
    // --------------------------------------------------
    updateZoomEdit();
    updateGraphsZoomX();
}

void ResultsView::editCurrentMinX()
{
    // --------------------------------------------------
    //  Find new current min & max (check range validity !!!)
    //  Update mResultZoomX
    // --------------------------------------------------
    QString str = mCurrentXMinEdit->text();
    bool isNumber;
    double value = str.toDouble(&isNumber);
    if (isNumber) {
        bool isDate = mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked();
        if(isDate){
            value = DateUtils::convertFromAppSettingsFormat(value);
        }
        double current = qBound(mResultMinX, value, mResultCurrentMaxX);
        if (current == mResultCurrentMaxX) {
            current = mResultMinX;
        }
        mResultCurrentMinX = current;
        mResultZoomX = (mResultCurrentMaxX - mResultCurrentMinX)/ (mResultMaxX - mResultMinX) * 100;
        
        // --------------------------------------------------
        //  Update other elements
        // --------------------------------------------------
        int zoom = int(100-mResultZoomX);
        mXSlider->setValue(zoom);
        
        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
        updateZoomEdit();
        updateGraphsZoomX();
    }
}

void ResultsView::editCurrentMaxX()
{
    // --------------------------------------------------
    //  Find new current min & max (check range validity !!!)
    //  Update mResultZoomX
    // --------------------------------------------------
    QString str = mCurrentXMaxEdit->text();
    bool isNumber;
    double value = str.toDouble(&isNumber);
    
    if (isNumber) {
        bool isDate = mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked();
        if(isDate){
            value = DateUtils::convertFromAppSettingsFormat(value);
        }
        double current = qBound(mResultCurrentMinX, value, mResultMaxX);
        if (current == mResultCurrentMinX) {
            current = mResultMaxX;
        }
        mResultCurrentMaxX = current;
        mResultZoomX = (mResultCurrentMaxX - mResultCurrentMinX)/ (mResultMaxX - mResultMinX)* 100;
        
        // --------------------------------------------------
        //  Update other elements
        // --------------------------------------------------
        int zoom = int(100-mResultZoomX);
        mXSlider->setValue(zoom);
        
        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
        updateZoomEdit();
        updateGraphsZoomX();
    }
}

void ResultsView::updateZoomEdit()
{
    if(mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked()){
        mCurrentXMinEdit->setText(DateUtils::convertToAppSettingsFormatStr(mResultCurrentMinX));
        mCurrentXMaxEdit->setText(DateUtils::convertToAppSettingsFormatStr(mResultCurrentMaxX));
    }else{
        mCurrentXMinEdit->setText(QString::number(mResultCurrentMinX));
        mCurrentXMaxEdit->setText(QString::number(mResultCurrentMaxX));
    }
}

void ResultsView::updateGraphsZoomX()
{
    for(int i=0; i<mByPhasesGraphs.size(); ++i){
        mByPhasesGraphs[i]->zoom(mResultCurrentMinX, mResultCurrentMaxX);
    }
    for(int i=0; i<mByEventsGraphs.size(); ++i) {
        mByEventsGraphs[i]->zoom(mResultCurrentMinX, mResultCurrentMaxX);
    }
    
    // --------------------------------------------------
    //  Store zoom values
    // --------------------------------------------------
    int tabIdx = mTabs->currentIndex();
    mZooms[tabIdx] = QPair<double, double>(mResultCurrentMinX, mResultCurrentMaxX);
}

#pragma mark Zoom Y
void ResultsView::updateScaleY(int value)
{
    double min = 70;
    double max = 1070;
    double prop = value / 100.f;
    mGraphsH = min + prop * (max - min);
    
    updateGraphsLayout();
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
        GraphViewPhase* graphPhase = dynamic_cast<GraphViewPhase*>(mByPhasesGraphs[i]);
        if(graphPhase) {
            graphPhase->setGraphsThickness(value);
        }
    }
    for(int i=0; i<mByEventsGraphs.size(); ++i)
    {
        mByEventsGraphs[i]->setGraphsThickness(value);
    }
}

void ResultsView::updateOpacity(int value)
{
    for(int i=0; i<mByPhasesGraphs.size(); ++i) {
        GraphViewPhase* graphPhase = dynamic_cast<GraphViewPhase*>(mByPhasesGraphs[i]);
        if(graphPhase) {
            graphPhase->setGraphsOpacity(value);
        }
    }
    for(int i=0; i<mByEventsGraphs.size(); ++i) {
        mByEventsGraphs[i]->setGraphsOpacity(value);
    }
}

void ResultsView::updateRendering(int index)
{
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i] -> setRendering((GraphView::Rendering) index);
    
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i] -> setRendering((GraphView::Rendering) index);
}

void ResultsView::showInfos(bool show)
{
    for(int i=0; i<mByEventsGraphs.size(); ++i)
        mByEventsGraphs[i]->showNumericalResults(show);
    
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
        mByPhasesGraphs[i]->showNumericalResults(show);
}

void ResultsView::exportResults()
{
    if(mModel){
        AppSettings settings = MainWindow::getInstance()->getAppSettings();
        QString csvSep = settings.mCSVCellSeparator;
        
        QLocale csvLocal = settings.mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
        qDebug()<<"mCSVDecSeparator"<<settings.mCSVDecSeparator<<"mCSVCellSeparator"<<settings.mCSVCellSeparator;
        csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);
        
        QString currentPath = MainWindow::getInstance()->getCurrentPath();
        QString dirPath = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                        tr("Export to directory..."),
                                                        currentPath,
                                                       tr("Directory"));
        
        if(!dirPath.isEmpty())
        {
            QDir dir(dirPath);
            if(dir.exists()){
                /*if(QMessageBox::question(qApp->activeWindow(), tr("Are you sure?"), tr("This directory already exists and all its content will be deleted. Do you really want to replace it?")) == QMessageBox::No){
                    return;
                }*/
                dir.removeRecursively();
            }
            dir.mkpath(".");
            
            QList<QStringList> stats = mModel->getStats(csvLocal);
            saveCsvTo(stats, dirPath + "/stats.csv", csvSep);
            
            if(mModel->mPhases.size() > 0){
                QList<QStringList> phasesTraces = mModel->getPhasesTraces(csvLocal);
                saveCsvTo(phasesTraces, dirPath + "/phases.csv", csvSep);
                
                for(int i=0; i<mModel->mPhases.size(); ++i){
                    QList<QStringList> phaseTrace = mModel->getPhaseTrace(i,csvLocal);
                    QString name = mModel->mPhases[i]->mName.toLower().simplified().replace(" ", "_");
                    saveCsvTo(phaseTrace, dirPath + "/phase_" + name + ".csv", csvSep);
                }
            }
            QList<QStringList> eventsTraces = mModel->getEventsTraces(csvLocal);
            saveCsvTo(eventsTraces, dirPath + "/events.csv", csvSep);
        }
    }
}

void ResultsView::exportFullImage()
{
    //  define ScrollArea
    enum ScrollArrea{
        eScrollPhases = 0,
        eScrollEvents= 1
    };
    
    ScrollArrea witchScroll;
    bool printAxis = true;
    
    QWidget* curWid;
    
    if (mStack->currentWidget() == mPhasesScrollArea) {
        curWid = mPhasesScrollArea->widget();
        curWid->setFont(mByPhasesGraphs[0]->font());
        witchScroll = eScrollPhases;
        //  hide all buttons in the both scrollAreaWidget
        for(int i=0; i<mByPhasesGraphs.size(); ++i){
            mByPhasesGraphs[i]->setButtonsVisible(false);
        }
    }
    //else if (mStack->currentWidget() == mEventsScrollArea) {
    else  {
        curWid = mEventsScrollArea->widget();
        curWid->setFont(mByEventsGraphs[0]->font());
        witchScroll = eScrollEvents;
        //  hide all buttons in the both scrollAreaWidget
        for(int i=0; i<mByEventsGraphs.size(); ++i) {
            mByEventsGraphs[i]->setButtonsVisible(false);
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
        axisWidget->updateValues(curWid->width() - axisWidget->mMarginLeft - axisWidget->mMarginRight, 50, mResultCurrentMinX, mResultCurrentMaxX);
        axisWidget->raise();
        axisWidget->setVisible(true);
        
        axisLegend = new QLabel(DateUtils::getAppSettingsFormat(), curWid);
        axisLegend->setGeometry(0, curWid->height() - axeHeight - legendHeight, curWid->width() - 10, legendHeight);
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
            mByPhasesGraphs[i]->setButtonsVisible(true);
        }
        
    }
    else {
        for(int i=0; i<mByEventsGraphs.size(); ++i) {
            mByEventsGraphs[i]->setButtonsVisible(true);
        }
    }
}

#pragma mark Refresh All Model
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
               // e->setJson(& MainWindow::getInstance()->getProject()->state(), j);
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
               // p->setJson(MainWindow::getInstance()->getProject()->state(), j);
                break;
            }
        }
    }
    
    std::sort(mModel->mEvents.begin(), mModel->mEvents.end(), sortEvents);
    std::sort(mModel->mPhases.begin(), mModel->mPhases.end(), sortPhases);
    
    updateResults(mModel);
}

void ResultsView::adjustDuration(bool visible)
{
    int durationMax = 0;
    // find the durationMax
    for(int i=0; i<mByPhasesGraphs.size(); ++i){
        GraphViewPhase* phasesGraphs = dynamic_cast<GraphViewPhase*>(mByPhasesGraphs[i]);
        if(phasesGraphs) {
            GraphView *durationGraph =phasesGraphs->mDurationGraph;

            if(durationGraph && durationGraph->isVisible() ) {
                GraphCurve *durationCurve = durationGraph->getCurve("Duration");
                if( durationCurve){
                   durationMax = qMax((int)durationCurve->mData.lastKey(),durationMax);
                }
             }
         }
    }
    // set the same RangeX with durationMax
    for(int i=0; i<mByPhasesGraphs.size(); ++i){
        GraphViewPhase* phasesGraphs = dynamic_cast<GraphViewPhase*>(mByPhasesGraphs[i]);
        if(phasesGraphs) {
            GraphView *durationGraph =phasesGraphs->mDurationGraph;
            if(durationGraph) {
                GraphCurve *durationCurve = durationGraph->getCurve("Duration");
                if(durationCurve){
                   durationGraph->setRangeX(0, durationMax);
                   durationGraph->setCurrentX(0, durationMax);
                }
             }
         }

    }

}
