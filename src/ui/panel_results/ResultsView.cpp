#include "ResultsView.h"
#include "GraphView.h"
#include "GraphViewDate.h"
#include "GraphViewEvent.h"
#include "GraphViewPhase.h"
#include "GraphViewTempo.h"
#include "Tabs.h"
#include "Ruler.h"
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

#include "AppSettings.h"

#include <QtWidgets>
#include <iostream>
#include <QtSvg>



ResultsView::ResultsView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mResultMaxVariance(1000.),
mHasPhases(false),
mModel(nullptr),
//mGraphHeight (10 * AppSettings::heigthUnit()), // init in applyAppSettings
mTabEventsIndex(0),
mTabPhasesIndex(0),
mEventsScrollArea(nullptr),
mPhasesScrollArea(nullptr),
mTempoScrollArea(nullptr),
forceXSpinSetValue(false),
forceXSlideSetValue(false),
mCurrentTypeGraph(GraphViewResults::ePostDistrib), //mTabs=0
mCurrentVariable(GraphViewResults::eTheta),
mBandwidthUsed(1.06),
mThresholdUsed(95.0),
mNumberOfGraph(APP_SETTINGS_DEFAULT_SHEET),
mMaximunNumberOfVisibleGraph(0),
mMajorScale (100),
mMinorCountScale (4)
{
    mSettings = ProjectSettings();

    mResultMinX = mSettings.mTmin;
    mResultMaxX = mSettings.mTmax;

    QFont ft (AppSettings::font());

    const QFontMetricsF fm(ft);
    setFont(ft);
     titleHeight = 1.5 * AppSettings::heigthUnit();
     labelHeight = AppSettings::heigthUnit();
     lineEditHeight = AppSettings::heigthUnit();
     checkBoxHeight  = AppSettings::heigthUnit();
     comboBoxHeight  = AppSettings::heigthUnit();
     radioButtonHeight = AppSettings::heigthUnit();
     spinBoxHeight  =1.5 * AppSettings::heigthUnit();
     buttonHeight  = 1.5 * AppSettings::heigthUnit();

    mGraphFont = ft;

    mResultCurrentMinX = mResultMinX ;
    mResultCurrentMaxX = mResultMaxX ;
    mResultZoomX = 1.;
    
    mTabs = new Tabs(this);
    mTabs->addTab(tr("Posterior Distrib."));
    mTabs->addTab(tr("History Plot"));
    mTabs->addTab(tr("Acceptance Rate"));
    mTabs->addTab(tr("Autocorrelation"));
    mTabs->setTab(0, false);

    connect(mTabs, &Tabs::tabClicked, this, &ResultsView::graphTypeChange);
    connect(mTabs,static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::updateLayout);
    // -------------

    mRuler = new Ruler(this);
    mRuler->mMax = mSettings.getTmaxFormated();
    mRuler->mMin = mSettings.getTminFormated();
    mRuler->mCurrentMax = mSettings.getTmaxFormated();
    mRuler->mCurrentMin = mSettings.getTminFormated();

  //  mRuler->setFont(mGraphFont);
   // mRulerH = mRuler->height();
   // mRuler->setMarginLeft(fm.width(stringForGraph(DateUtils::convertToAppSettingsFormat(mSettings.getTminFormated())))/ 2) ;
  //  mRuler->setMarginRight(fm.width(stringForGraph(DateUtils::convertToAppSettingsFormat(mSettings.getTmaxFormated())))/ 2);



    //------

    
    mStack = new QStackedWidget(this);
    
    /* mEventsScrollArea and mPhasesScrollArea are made when we need it,
     *  within createEventsScrollArea and within createPhasesScrollArea
     */
    
    mMarker = new Marker(this);
    
    setMouseTracking(true);
    mStack->setMouseTracking(true);
    

    mOptionsWidget = new QWidget(this); // this is the parent of group of widget on the rigth of the panel

    //mOptionsWidget->setFixedWidth(mOptionsW);
    //mOptionsWidget->setFont(ft);

    /* -------------------------------------- mResultsGroup---------------------------------------------------*/

    mResultsGroup = new QWidget(this);

    mEventsfoldCheck = new CheckBox(tr("Unfold Events"), mResultsGroup);
    mEventsfoldCheck->setToolTip(tr("Display phases' events"));

    mDatesfoldCheck = new CheckBox(tr("Unfold Data"), mResultsGroup);
    mDatesfoldCheck->setToolTip(tr("Display Events' data"));

    mDataThetaRadio = new RadioButton(tr("Calendar Dates"), mResultsGroup);
    mDataThetaRadio->setChecked(true);

    mDataSigmaRadio = new RadioButton(tr("Ind. Std. Deviations"), mResultsGroup);

    mDataCalibCheck = new CheckBox(tr("Individual Calib. Dates"), mResultsGroup);
    mDataCalibCheck->setChecked(true);

    mWiggleCheck = new CheckBox(tr("Wiggle shifted"), mResultsGroup);

    mStatCheck = new CheckBox(tr("Show Stat."), mResultsGroup);
    mStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));


/*  ------------- TempoGroup  ------------- */
    mTempoGroup = new QWidget(this);

    mDurationRadio = new RadioButton(tr("Phases Duration"), mTempoGroup);
    mDurationRadio->setChecked(true);

    mTempoRadio = new RadioButton(tr("Phases Tempo"), mTempoGroup);

    mTempoCredCheck = new CheckBox(tr("Tempo Cred."), mTempoGroup);
    mTempoErrCheck = new CheckBox(tr("Tempo Error"), mTempoGroup);

    mActivityRadio = new RadioButton(tr("Phases Activity"), mTempoGroup);

    mTempoStatCheck = new CheckBox(tr("Show Tempo Stat."), mTempoGroup);
    mTempoStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));

/* -------------end TempoGroup */


    mTabByScene = new Tabs(mOptionsWidget);
    mTabByScene->setFixedWidth(mOptionsW);
    // we set the same widget
    mTabByScene->addTab(mResultsGroup, tr("Events"));
    mTabByScene->addTab(mResultsGroup, tr("Phases"));
    mTabByScene->addTab(mTempoGroup, tr("Tempo"));


    connect(mTabByScene, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::changeScrollArea);
    connect(mTabByScene, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::updateVisibleTabs);

    connect(mDataThetaRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mDataSigmaRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mDataCalibCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mWiggleCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mStatCheck, &CheckBox::clicked, this, &ResultsView::showInfos);

    connect(mTempoCredCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mTempoErrCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    connect(mDurationRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mTempoRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mActivityRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mTempoStatCheck, &CheckBox::clicked, this, &ResultsView::showInfos);

    // -------------------------

    /* - mTabDisplayMCMC
     *  show Display (mSpanGroup, mGraphicGroup) and MCMC
     */
    mTabDisplayMCMC = new Tabs(mOptionsWidget);

    mTabDisplay = new QWidget(this);

    mTabMCMC = new QWidget(this);

    mTabDisplayMCMC->addTab(mTabDisplay, tr("Display"));
    mTabDisplayMCMC->addTab(mTabMCMC, tr("Distrib. Options"));

    connect(mTabDisplayMCMC,static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::updateLayout);

    /* ----------------------------------------------------------
     *  Display Options layout
     * ----------------------------------------------------------*/

    // ------ Span Options -----
    mSpanGroup  = new QWidget(mTabDisplay);
    //mSpanGroup->setFont(ft);

    mSpanTitle = new Label(tr("Span Options"), mTabDisplay);
    mSpanTitle->setIsTitle(true);


    mDisplayStudyBut = new Button(tr("Study Period Display"), mSpanGroup);
    mDisplayStudyBut->setToolTip(tr("Restore view with the study period span"));
    mSpanLab = new Label(tr("Span"), mSpanGroup);

    mCurrentXMinEdit = new LineEdit(mSpanGroup);
    mCurrentXMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentXMaxEdit = new LineEdit(mSpanGroup);
   // mCurrentXMaxEdit->setFixedSize(wEdit, lineEditHeight);
    mCurrentXMaxEdit->setToolTip(tr("Enter a maximal value to display the curves"));

    mXScaleLab = new Label(tr("X"), mSpanGroup);
    mXScaleLab->setAlignment(Qt::AlignCenter);

    mXSlider = new QSlider(Qt::Horizontal, mSpanGroup);
    mXSlider->setRange(-100, 100);
    mXSlider->setTickInterval(1);
    forceXSlideSetValue = true;
    mXSlider->setValue(0);

    mXScaleSpin = new QDoubleSpinBox(mSpanGroup);
    mXScaleSpin->setRange(pow(10., (double)mXSlider->minimum()/100.),pow(10., (double)mXSlider->maximum()/100.));
    mXScaleSpin->setSingleStep(.01);
    mXScaleSpin->setDecimals(3);
    forceXSpinSetValue = true;
    mXScaleSpin->setValue(sliderToZoom(mXSlider->value()));
    mXScaleSpin->setToolTip(tr("Enter zoom value to magnify the curves on X span"));

    mMajorScaleLab = new Label(tr("Major Interval"), mSpanGroup);
    mMajorScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mMajorScaleEdit = new LineEdit(mSpanGroup);
    mMajorScaleEdit->setText(QString::number(mMajorScale));
    mMajorScaleEdit->setToolTip(tr("Enter a interval for the main division of the axes under the curves, upper than 1"));


    mMinorScaleLab = new Label(tr("Minor Interval Count"), mSpanGroup);
    mMinorScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mMinorScaleEdit = new LineEdit(mSpanGroup);
    mMinorScaleEdit->setText(QString::number(mMinorCountScale));
    mMinorScaleEdit->setToolTip(tr("Enter a interval for the subdivision of the Major Interval for the scale under the curves, upper than 1"));


    /* -------------------------------------- Graphic Options (old mDisplayGroup) ---------------------------------------------------*/
    
    mGraphicTitle = new Label(tr("Graphic Options"), mTabDisplay);
    mGraphicTitle->setIsTitle(true);

    mGraphicGroup = new QWidget(mTabDisplay);

    mYScaleLab = new Label(tr("Y"), mGraphicGroup);
    mYScaleLab->setAlignment(Qt::AlignCenter);

    mYSlider = new QSlider(Qt::Horizontal, mGraphicGroup);
    mYSlider->setRange(10, 300);
    mYSlider->setTickInterval(1);
    mYSlider->setValue(100);
    
    mYScaleSpin = new QSpinBox(mGraphicGroup);
    mYScaleSpin->setRange(mYSlider->minimum(), mYSlider->maximum());
    mYScaleSpin->setSuffix(" %");
    mYScaleSpin->setValue(mYSlider->value());
    mYScaleSpin->setToolTip(tr("Enter zoom value to magnify the curves on Y scale"));

    labFont = new Label(tr("Font"), mGraphicGroup);
    labFont->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mFontBut = new Button(mGraphFont.family() + ", " + QString::number(mGraphFont.pointSizeF()), mGraphicGroup);
    mFontBut->setToolTip(tr("Click to change the font on the drawing"));
    connect(mFontBut, &QPushButton::clicked, this, &ResultsView::updateGraphFont);
    
    labThickness = new Label(tr("Thickness"), mGraphicGroup);
    labThickness->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mThicknessCombo = new QComboBox(mGraphicGroup);
    mThicknessCombo->addItem("1 px");
    mThicknessCombo->addItem("2 px");
    mThicknessCombo->addItem("3 px");
    mThicknessCombo->addItem("4 px");
    mThicknessCombo->addItem("5 px");

    mThicknessCombo->setToolTip(tr("Select to change the thickness of the drawing"));

    connect(mThicknessCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::updateThickness);
    
    labOpacity = new Label(tr("Opacity"), mGraphicGroup);
    labOpacity->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mOpacityCombo = new QComboBox(mGraphicGroup);
    mOpacityCombo->addItem("0 %");
    mOpacityCombo->addItem("10 %");
    mOpacityCombo->addItem("20 %");
    mOpacityCombo->addItem("30 %");
    mOpacityCombo->addItem("40 %");
    mOpacityCombo->addItem("50 %");
    mOpacityCombo->addItem("60 %");
    mOpacityCombo->addItem("70 %");
    mOpacityCombo->addItem("80 %");
    mOpacityCombo->addItem("90 %");
    mOpacityCombo->addItem("100 %");
    mOpacityCombo->setToolTip(tr("Select to change the opacity of the drawing"));
    mOpacityCombo->setCurrentIndex(5);
    //mOpacitySpin->setStyleSheet("QLineEdit { border-radius: 5px; }"); // not supported

    connect(mOpacityCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::updateOpacity);

   /* -------------------------------------- mChainsGroup---------------------------------------------------*/
    
    mChainsTitle = new Label(tr("MCMC Chains"), mTabMCMC);
    mChainsTitle->setIsTitle(true);

    mChainsGroup = new QWidget(mTabMCMC);

    mAllChainsCheck = new CheckBox(tr("Chains Concatenation"), mChainsGroup);
    mAllChainsCheck->setChecked(true);

    /*
     * QList<CheckBox*> mCheckChainChecks;
     * QList<RadioButton*> mChainRadios;
     * mCheckChainChecks and mChainRadios are created in  initResults() they are children of mChainsGroup
    */

    /* -------------------------------------- mDensityOptsGroup ---------------------------------------------------*/
    
    mDensityOptsTitle = new Label(tr("Density Options"), mTabMCMC);
    mDensityOptsTitle->setIsTitle(true);

    mDensityOptsGroup = new QWidget(mTabMCMC);
    
    mCredibilityCheck = new CheckBox(tr("Show Confidence Bar"), mDensityOptsGroup);
    mCredibilityCheck->setChecked(true);

    mThreshLab = new Label(tr("Confidence Level (%)"), mDensityOptsGroup);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    mHPDEdit = new LineEdit(mDensityOptsGroup);
    mHPDEdit->setText("95");
    
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.0);
    percentValidator->setTop(100.0);
    percentValidator->setDecimals(1);
    mHPDEdit->setValidator(percentValidator);
    
    mFFTLenLab = new Label(tr("Grid Length"), mDensityOptsGroup);
    mFFTLenLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mFFTLenCombo = new QComboBox(mDensityOptsGroup);
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
    
    mComboH = fm.height() + 6;
    mTabsH = mComboH + 2*mMargin;
    
    mBandwidthLab = new Label(tr("Bandwidth Const."), mDensityOptsGroup);
    mBandwidthLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mBandwidthEdit = new LineEdit(mDensityOptsGroup);
    mBandwidthEdit->setText(locale().toString(1.06));


    // ------------------------- CONNECTIONS

    connect(this, &ResultsView::controlsUpdated, this, &ResultsView::updateLayout);
    connect(this, &ResultsView::scalesUpdated, this, &ResultsView::updateControls);

    
    // -------------------------
    connect(mFFTLenCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::setFFTLength);
    connect(mBandwidthEdit, &LineEdit::editingFinished, this, &ResultsView::setBandwidth);
    
    connect(mHPDEdit, &LineEdit::editingFinished, this, &ResultsView::setThreshold);

    connect(mAllChainsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCredibilityCheck, &CheckBox::clicked, this, &ResultsView::generateCurvesRequested);
    
    // -------------------------
    connect(mEventsfoldCheck,&CheckBox::clicked, this, &ResultsView::unfoldToggle);
    connect(mDatesfoldCheck, &CheckBox::clicked, this, &ResultsView::unfoldToggle);

    connect(this, &ResultsView::curvesGenerated, this, &ResultsView::updateCurvesToShow);

    connect(this, &ResultsView::updateScrollAreaRequested, this, &ResultsView::changeScrollArea);
    connect(this, &ResultsView::generateCurvesRequested, this, &ResultsView::updateCurves);
    // -------------------------

    connect(mXSlider, &QSlider::valueChanged, this, &ResultsView::XScaleSliderChanged);

    connect(mXScaleSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ResultsView::XScaleSpinChanged);

    connect(this, &ResultsView::xSpinUpdate, this, &ResultsView::setXScaleSlide);
    connect(this, static_cast<void (ResultsView::*)(int)>(&ResultsView::xSpinUpdate), this, &ResultsView::updateZoomX);

    connect(mCurrentXMinEdit, &LineEdit::editingFinished, this, &ResultsView::editCurrentMinX);
    connect(mCurrentXMaxEdit, &LineEdit::editingFinished, this, &ResultsView::editCurrentMaxX);
    connect(mDisplayStudyBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::setStudyPeriod);

    // Connection
    // QLineEdit::setText() doesn't emit signal textEdited, when the text is changed programmatically
    connect(mMajorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this,  &ResultsView::updateScaleX);
    connect(mMinorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this,  &ResultsView::updateScaleX);


    connect(mRuler, &Ruler::positionChanged, this, &ResultsView::updateScroll);
    
    connect(mYSlider, &QSlider::valueChanged, this, &ResultsView::updateScaleY);
    connect(mYScaleSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::updateScaleY);
    connect(mYScaleSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), mYSlider, &QSlider::setValue);
    // -------------------------


    /*-------------
     * ------Tools for all graph
     * ------------- */
    mToolsWidget = new QWidget(this);

    //const QSize allDensitiesButSize (int(ceil(mOptionsW/2.)), checkBoxHeight);

    mExportImgBut = new Button(tr("Capture"), mToolsWidget);
    mExportImgBut->setFlatHorizontal();
    mExportImgBut->setIcon(QIcon(":picture_save.png"));
    mExportImgBut->setToolTip(tr("Save all currently visible results as an image.<br><u>Note</u> : If you want to copy textual results, see the Log tab."));

    mExportResults = new Button(tr("Results"), mToolsWidget);
    mExportResults->setFlatHorizontal();
    mExportResults->setIcon(QIcon(":csv.png"));
    mExportResults->setToolTip(tr("Export all result in several files"));

    connect(mExportImgBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::exportFullImage);
    connect(mExportResults, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::exportResults);

    /* --------------------------------------Tools for single graph -------------------------------------- */

    //const QSize singleDensityButSize (mOptionsW/4, 50);
    mImageSaveBut = new Button(tr("Save"), mToolsWidget);
    mImageSaveBut->setIcon(QIcon(":picture_save.png"));
    mImageSaveBut->setFlatVertical();
    mImageSaveBut->setToolTip(tr("Save image as file"));

    mImageClipBut = new Button(tr("Copy"), mToolsWidget);
    mImageClipBut->setIcon(QIcon(":clipboard_graph.png"));
    mImageClipBut->setFlatVertical();
    mImageClipBut->setToolTip(tr("Copy image to clipboard"));

    mResultsClipBut = new Button(tr("Copy"), mToolsWidget);
    mResultsClipBut->setIcon(QIcon(":text.png"));
    mResultsClipBut->setFlatVertical();
    mResultsClipBut->setToolTip(tr("Copy text results to clipboard"));

    mDataSaveBut = new Button(tr("Save"), mToolsWidget);
    mDataSaveBut->setIcon(QIcon(":data.png"));
    mDataSaveBut->setFlatVertical();
    mDataSaveBut->setToolTip(tr("Save graph data to file"));

    connect(mImageSaveBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::saveAsImage);
    connect(mImageClipBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::imageToClipboard);
    connect(mResultsClipBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::resultsToClipboard);
    connect(mDataSaveBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::saveGraphData);

    /* -------------------------------------- Page widget -------------------------------------- */
    mPageWidget = new QWidget (this);
    mPageWidget->resize(mOptionsW, mRulerH);

    mSheetNum = new LineEdit(mPageWidget);
    mSheetNum->setEnabled(false);
    mSheetNum->setReadOnly(true);
    mSheetNum->setAlignment(Qt::AlignCenter);
    mSheetNum->setText(QString::number(mMaximunNumberOfVisibleGraph));

    mPreviousSheetBut  = new Button(tr("Prev."), mPageWidget);
    mPreviousSheetBut->setCheckable(false);
    mPreviousSheetBut->setFlatHorizontal();
    mPreviousSheetBut->setToolTip(tr("Display previous data"));
    mPreviousSheetBut->setIconOnly(false);

    mNextSheetBut  = new Button(tr("Next"), mPageWidget);
    mNextSheetBut->setCheckable(false);
    mNextSheetBut->setFlatHorizontal();
    mNextSheetBut->setToolTip(tr("Display next data"));
    mNextSheetBut->setIconOnly(false);

    mNbDensityLab = new Label(tr("Nb Densities / Sheet"), mPageWidget);
    mNbDensityLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mNbDensitySpin = new QSpinBox(mPageWidget);
    mNbDensitySpin->setRange(1, 100);
    mNbDensitySpin->setValue(mNumberOfGraph);
    mNbDensitySpin->setToolTip(tr("Enter the maximum densities to display on a sheet"));

    connect(mPreviousSheetBut, &Button::pressed, this, &ResultsView::previousSheet);
    connect(mNextSheetBut, &Button::pressed, this, &ResultsView::nextSheet);
    connect(mNbDensitySpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::updateNbDensity);

    mTabPageSaving = new Tabs(mOptionsWidget);
    mTabPageSaving->addTab(mPageWidget, tr("Page"));
    mTabPageSaving->addTab(mToolsWidget, tr("Saving"));

   // connect(mTabPageSaving, &Tabs::tabClicked, mTabPageSaving, &Tabs::showWidget);
    connect(mTabPageSaving,static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::updateLayout);

    //connect(mTabDisplayMCMC, &Tabs::tabClicked, this, &ResultsView::updateTabDisplay);

    /* Update font and size with the AppSettings
     * Set mTabs, mTabByScene, mTabDisplayMCMC, mTabPageSaving
     */
    /*
    applyAppSettings();
*/
    updateVisibleTabs(0);

    updateTabByScene();
    updateTabByTempo();
    mTabByScene->setTab(0, false);
    mTabByScene->showWidget(0);

    mMarker->raise();

    mTabDisplayMCMC->setTab(0, false);
    mTabDisplayMCMC->showWidget(0);
    updateTabDisplay(0);//mTabDisplayMCMC->currentIndex());

    mTabPageSaving->setTab(0, false);
    mTabPageSaving->showWidget(0);
    updateTabPageSaving();

}

ResultsView::~ResultsView()
{
    mModel = nullptr;
}

void ResultsView::doProjectConnections(Project* project)
{
    /* Starting MCMC calculation does a mModel.clear() at first, and recreate it.
     * Then, it fills its eleme,nts (events, ...) with calculated data (trace, ...)
     * If the process is canceled, we only have unfinished data in storage.
     * => The previous nor the new results can be displayed so we must start by clearing the results view! */

    connect(project, &Project::mcmcStarted, this, &ResultsView::clearResults);
    mModel = project->mModel;
    connect(mModel, &Model::newCalculus, this, &ResultsView::generateCurvesRequested);

}

void ResultsView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    if (mModel->mProject->withResults() )
         updateControls(); // emit controleUpdated which is connected to updateLayout

}
void ResultsView::applyAppSettings()
{
    const QFont ft (AppSettings::font());
    const QFontMetricsF fm(ft);
    setFont(ft);
    titleHeight = 1.5 * AppSettings::heigthUnit();
    labelHeight = AppSettings::heigthUnit();
    lineEditHeight = AppSettings::heigthUnit();
    checkBoxHeight  = 1.1 * AppSettings::heigthUnit();

    radioButtonHeight = 1.1 *AppSettings::heigthUnit();
    spinBoxHeight  =1.4 * AppSettings::heigthUnit();
    buttonHeight  = 1.5 * AppSettings::heigthUnit();

#ifdef Q_OS_MAC
    comboBoxHeight  = 1.3 * AppSettings::heigthUnit();
#else
    comboBoxHeight  = 1.1 * AppSettings::heigthUnit();
#endif

    mMargin = .5* AppSettings::heigthUnit();

    mTabs->setFont(ft);
    mRuler->setFont(ft);
    mRuler->setMarginBottom( ft.pointSize() * 2.2);
    mRulerH = mRuler->height();

    mTabsH = 2 * fm.height();
    mGraphHeight = 10 * AppSettings::heigthUnit(); // same value in ResultsView::updateScaleY(int value)

    mOptionsW = ( fm.width(tr("Nb Densities / Sheet ")) + 2 * mMargin) *3 / 2;

    const int wEdit = (int)ceil((mOptionsW - 4 * mMargin)/3.);
    const QSize allDensitiesButSize (mOptionsW/2, mOptionsW/4);
    const QSize singleDensityButSize (mOptionsW/4, mOptionsW/4);


    mOptionsWidget->setFixedWidth(mOptionsW);

    mOptionsWidget->setFont(ft);

    mPageWidget->resize(mOptionsW, mRulerH);
     /* -------------------------------------- mResultsGroup---------------------------------------------------*/
    mResultsGroup->setFont(ft);

    mEventsfoldCheck->setFont(ft);
    mDatesfoldCheck->setFont(ft);
    mDataThetaRadio->setFont(ft);
    mDataSigmaRadio->setFont(ft);
    mDataCalibCheck->setFont(ft);
    mWiggleCheck->setFont(ft);
    mStatCheck->setFont(ft);

    mEventsfoldCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mDatesfoldCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mDataThetaRadio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);
    mDataSigmaRadio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);
    mDataCalibCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mWiggleCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mStatCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);

     // ______ TempoGroup
    mDurationRadio->setFont(ft);
    mTempoRadio->setFont(ft);
    mActivityRadio->setFont(ft);
    mTempoStatCheck->setFont(ft);
    mTempoCredCheck->setFont(ft);
    mTempoErrCheck->setFont(ft);

    mDurationRadio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);
    mTempoRadio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);
    mActivityRadio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);
    mTempoStatCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mTempoCredCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mTempoErrCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);

     // -------------end TempoGroup
    mTabByScene->setFont(ft);
    mTabByScene->setFixedWidth(mOptionsW);

     /* - mTabDisplayMCMC */
    mTabDisplayMCMC->setFont(ft);
    mTabDisplay->setFont(ft);
    mTabMCMC->setFont(ft);

     /*  Display Options layout */
    mSpanGroup->setFont(ft);
    mSpanTitle->setFont(ft);
    mDisplayStudyBut->setFont(ft);
    mSpanLab->setFont(ft);
    mCurrentXMinEdit->setFont(ft);
    mCurrentXMaxEdit->setFont(ft);
    mXScaleLab->setFont(ft);
    mXScaleSpin->setFont(ft);

    mXScaleSpin->setLocale(QLocale(AppSettings::mLanguage, AppSettings::mCountry));

    mMajorScaleLab->setFont(ft);
    mMajorScaleEdit->setFont(ft);
    mMinorScaleLab->setFont(ft);
    mMinorScaleEdit->setFont(ft);

    mSpanTitle->setFixedSize(mOptionsW, titleHeight);
    mDisplayStudyBut->setFixedSize(mOptionsW - 2*mMargin, buttonHeight);
    mSpanLab->setFixedSize(fm.width(mSpanLab->text()), labelHeight);
    mCurrentXMinEdit->setFixedSize(wEdit, lineEditHeight);
    mCurrentXMaxEdit->setFixedSize(wEdit, lineEditHeight);
    mXScaleLab->setFixedWidth(fm.width(mXScaleLab->text()));
    mXScaleSpin->setFixedSize(mCurrentXMinEdit->width(), spinBoxHeight);
    mMajorScaleLab->setFixedSize(fm.width(mMajorScaleLab->text()), labelHeight);
    mMajorScaleEdit->setFixedSize(wEdit, lineEditHeight);
    mMinorScaleLab->setFixedSize(fm.width(mMinorScaleLab->text()), labelHeight);
    mMinorScaleEdit->setFixedSize(wEdit, lineEditHeight);

    /* -------------------------------------- Graphic Options (old mDisplayGroup) ---------------------------------------------------*/
    mGraphicTitle->setFont(ft);
    mYScaleLab->setFont(ft);
    mYScaleSpin->setFont(ft);
    labFont->setFont(ft);
    mFontBut->setFont(ft);
    labThickness->setFont(ft);
    mThicknessCombo->setFont(ft);
    labOpacity->setFont(ft);
    mOpacityCombo->setFont(ft);

    mGraphicTitle->setFixedSize(mOptionsW, titleHeight);
    mYScaleLab->setFixedSize(fm.width(mYScaleLab->text()), labelHeight);
    mYScaleSpin->setFixedSize(mCurrentXMinEdit->width(), spinBoxHeight);
    labFont->setFixedSize(fm.width(labFont->text()), labelHeight);
    mFontBut->setFixedSize(mOptionsW/2 - mMargin, buttonHeight);
    labThickness->setFixedSize(fm.width(labThickness->text()), comboBoxHeight);
    mThicknessCombo->setFixedSize(mOptionsW/2 - mMargin, comboBoxHeight);//->setFixedSize(wEdit, comboBoxHeight );
    labOpacity->setFixedSize(fm.width(labOpacity->text()), comboBoxHeight);
    mOpacityCombo->setFixedSize(mOptionsW/2 - mMargin, comboBoxHeight);//setFixedSize(wEdit, comboBoxHeight);

     /* -------------------------------------- mChainsGroup---------------------------------------------------*/
    mChainsTitle->setFont(ft);
    mAllChainsCheck->setFont(ft);
    mAllChainsCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);

    if (!mCheckChainChecks.isEmpty())
        for (auto check : mCheckChainChecks)
            check->setFont(ft);

    if (!mChainRadios .isEmpty())
        for (auto radio : mChainRadios)
            radio->setFont(ft);

    mChainsTitle->setFixedSize(mOptionsW, titleHeight);
    mAllChainsCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);

    if (mCheckChainChecks.isEmpty())
           for (auto check : mCheckChainChecks)
                check->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);

    if (mChainRadios.isEmpty())
           for (auto radio : mChainRadios)
                radio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);


     /* -------------------------------------- mDensityOptsGroup ---------------------------------------------------*/
    mDensityOptsTitle->setFont(ft);
    mDensityOptsGroup->setFont(ft);
    mCredibilityCheck->setFont(ft);
    mThreshLab->setFont(ft);
    mHPDEdit->setFont(ft);
    mFFTLenLab->setFont(ft);
    mFFTLenCombo->setFont(ft);
    mBandwidthLab->setFont(ft);
    mBandwidthEdit->setFont(ft);

    mDensityOptsTitle->setFixedSize(mOptionsW, titleHeight);
    mCredibilityCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mThreshLab->setFixedSize( fm.width(mThreshLab->text()), lineEditHeight);
    mHPDEdit->setFixedSize(wEdit, lineEditHeight);
    mFFTLenLab->setFixedSize(fm.width(mFFTLenLab->text()), comboBoxHeight);
    mFFTLenCombo->setFixedSize(wEdit, comboBoxHeight);
    mBandwidthLab->setFixedSize(fm.width(mBandwidthLab->text()), lineEditHeight);
    mBandwidthEdit->setFixedSize(wEdit, lineEditHeight);

     /* --------------------------------------Tools for all graph -------------------------------------- */
    mToolsWidget->setFont(ft);
    mExportImgBut->setFont(ft);
    mExportResults->setFont(ft);
    mImageSaveBut->setFont(ft);
    mImageClipBut->setFont(ft);
    mResultsClipBut->setFont(ft);
    mDataSaveBut->setFont(ft);

    mToolsWidget->resize(mOptionsW, 50);
    mExportImgBut->setFixedSize(allDensitiesButSize);
    mExportResults->setFixedSize(allDensitiesButSize);
    mImageSaveBut->setFixedSize(singleDensityButSize);
    mImageClipBut->setFixedSize(singleDensityButSize);
    mResultsClipBut->setFixedSize(singleDensityButSize);
    mDataSaveBut->setFixedSize(singleDensityButSize);

    /* -------------------------------------- Page widget -------------------------------------- */
    mPageWidget->setFont(ft);
    mSheetNum->setFont(ft);
    mPreviousSheetBut->setFont(ft);
    mNextSheetBut->setFont(ft);
    mNbDensityLab->setFont(ft);
    mNbDensitySpin->setFont(ft);
    mTabPageSaving->setFont(ft);

    mPageWidget->resize(mOptionsW, mRulerH);
    mSheetNum->setFixedSize(fm.width("__ /__"), buttonHeight);
    mPreviousSheetBut->setFixedSize((mOptionsW- mSheetNum->width())/2, buttonHeight);
    mNextSheetBut->setFixedSize((mOptionsW - mSheetNum->width())/2, buttonHeight);
    mNbDensityLab->setFixedSize(fm.width(mNbDensityLab->text()), spinBoxHeight);
    mNbDensitySpin->setFixedSize(mCurrentXMinEdit->width(), spinBoxHeight);
    mTabPageSaving->setFixedWidth(mOptionsW);



    // set the variable and the graphic type
    mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    mCurrentVariable = GraphViewResults::eTheta;

    if (mHasPhases) {
        mTabByScene->setTab(1, false);
        mTabByScene->setTabVisible(1, true);
        mTabByScene->setTabVisible(2, true);
        updateVisibleTabs(1);
        mTabs->setTab(0, false);

     } else {
        mTabByScene->setTabVisible(2, false);
        mTabByScene->setTabVisible(1, false);
        mTabByScene->setTab(0, false);
        updateVisibleTabs(0);
        mTabs->setTab(0, false);
     }

}

/**
 * @brief ResultsView::updateControls set controls according to the differents tabs positions.  emit controlsUpdated()
*/
void ResultsView::updateControls()
{
   qDebug() << "ResultsView::updateControls()";
   Q_ASSERT(mModel);
    bool byEvents (mTabByScene->currentIndex() == 0);
    bool byPhases (mTabByScene->currentIndex() == 1);
    bool byTempo  (mTabByScene->currentIndex() == 2);


    /* -------------------------------------------------------
     *  Activate specific controls for post. distrib. (first tab)
     * -------------------------------------------------------*/
    if (byEvents) {
        mDataCalibCheck -> setVisible((mCurrentTypeGraph == GraphViewResults::ePostDistrib)
                                                                        && mDatesfoldCheck->isChecked()
                                                                        && mDataThetaRadio->isChecked());
        mWiggleCheck-> setVisible((mCurrentTypeGraph == GraphViewResults::ePostDistrib)
                                                                 && mDatesfoldCheck->isChecked()
                                                                 && mDataThetaRadio->isChecked());

    } else if (byPhases) {
        mDataCalibCheck -> setVisible((mCurrentTypeGraph == GraphViewResults::ePostDistrib)
                                                                         && mEventsfoldCheck->isChecked()
                                                                         && mDatesfoldCheck->isChecked()
                                                                         && mDataThetaRadio->isChecked());
        mWiggleCheck-> setVisible((mCurrentTypeGraph == GraphViewResults::ePostDistrib)
                                                                && mEventsfoldCheck->isChecked()
                                                                && mDatesfoldCheck->isChecked()
                                                                && mDataThetaRadio->isChecked());
    } else {
        mDataCalibCheck -> setVisible(false);
        mWiggleCheck-> setVisible(false);
    }



    
    /* -------------------------------------------------------
     *  Display by phases or by events
     * -------------------------------------------------------*/
    if (byEvents) {
            if (!mEventsScrollArea)
                createEventsScrollArea(mTabEventsIndex);
            mStack->setCurrentWidget(mEventsScrollArea);

    } else if (byPhases) {
        if (!mPhasesScrollArea)
            createPhasesScrollArea(mTabPhasesIndex);
        mStack->setCurrentWidget(mPhasesScrollArea);

    } else if (byTempo) {
        if (!mTempoScrollArea)
            createTempoScrollArea(mTabTempoIndex);
        mStack->setCurrentWidget(mTempoScrollArea);
    }


    /* -------------------------------------------------------
     *  Enable or disable previous and next sheet
     * -------------------------------------------------------*/
    int currentIndex (0);
    int selectedGraph (0);
    int unselectedGraph (0);
    bool showAllGraph (true);

    if (byEvents) {
        for (auto &&ev : mModel->mEvents) {
            if (ev->mIsSelected) {
                selectedGraph += 1;
                if (mDatesfoldCheck->isChecked())
                    selectedGraph += ev->mDates.size();
                showAllGraph = false;
            } else {
                unselectedGraph += 1;
                if (mDatesfoldCheck->isChecked())
                    unselectedGraph += ev->mDates.size();
            }
        }
        currentIndex = mTabEventsIndex;
    }

    else if (byPhases) {
         for (auto &&ph : mModel->mPhases) {
            if (ph->mIsSelected) {
                showAllGraph = false;
                selectedGraph += 1;
                if (mEventsfoldCheck->isChecked())
                    selectedGraph += ph->mEvents.size();

                if (mDatesfoldCheck->isVisible() && mDatesfoldCheck->isChecked()) {
                    for (auto && ev : ph->mEvents)
                        selectedGraph += ev->mDates.size();
                }
            } else {
                    unselectedGraph += 1;
                    if (mEventsfoldCheck->isChecked())
                        unselectedGraph += ph->mEvents.size();

                    if (mDatesfoldCheck->isVisible() && mDatesfoldCheck->isChecked()) {
                        for (auto && ev : ph->mEvents)
                            unselectedGraph += ev->mDates.size();
                    }
                }
         }
        currentIndex = mTabPhasesIndex;
    }

    else if (byTempo) {
         for (auto && ph : mModel->mPhases) {
            if (ph->mIsSelected) {
                showAllGraph = false;
                selectedGraph += 1;

            } else
                    unselectedGraph += 1;

         }
        currentIndex = mTabTempoIndex;
    }

    if (showAllGraph)
        mMaximunNumberOfVisibleGraph = unselectedGraph;
    else
        mMaximunNumberOfVisibleGraph = selectedGraph;

    if (currentIndex == 0)
        mPreviousSheetBut->setEnabled(false);
    else
        mPreviousSheetBut->setEnabled(true);

    if ( ((currentIndex+1)*mNumberOfGraph) < mMaximunNumberOfVisibleGraph)
        mNextSheetBut->setEnabled(true);
    else
        mNextSheetBut->setEnabled(false);

    emit controlsUpdated();
}

void ResultsView:: updateTabByScene()
{
    bool byEvents (mTabByScene->currentIndex() == 0);
    bool byPhases (mTabByScene->currentIndex() == 1);
    if (!byEvents && !byPhases)
        return;

    /* ----------------------------------------------------------
     *  Results options layout, member within  mTabByScene
     * ----------------------------------------------------------*/
    int ySpan (mMargin);

    // RadioButton


    mDataThetaRadio->move(mMargin, ySpan);
    ySpan += mDataThetaRadio->height() + mMargin;

    mDataSigmaRadio -> move(mMargin, ySpan);
    ySpan += mDataSigmaRadio->height() + mMargin;


    // CheckBox

    if (byPhases) {
        mEventsfoldCheck->setVisible(true);
        mEventsfoldCheck->move(mMargin , ySpan);
        ySpan += mEventsfoldCheck->height() + mMargin;

        mDatesfoldCheck->setVisible(mEventsfoldCheck->isChecked());
        if (mEventsfoldCheck->isChecked()) {
            mDatesfoldCheck->move(mMargin , ySpan);
            ySpan += mDatesfoldCheck->height() + mMargin;
        }

    } else if (byEvents) {
        mEventsfoldCheck->setVisible(false);
        mDatesfoldCheck->setVisible(true);
        mDatesfoldCheck->move(mMargin , ySpan);
        ySpan += mDatesfoldCheck->height() + mMargin;

    }

    int dx (2 * mMargin);

    if ((mCurrentTypeGraph == GraphViewResults::ePostDistrib)  && mDataThetaRadio->isChecked() ) {
        if (( byEvents || (byPhases && mEventsfoldCheck->isChecked()) )   && mDatesfoldCheck->isChecked()) {
            mDataCalibCheck -> move(mMargin + dx, ySpan);
            ySpan += mDataCalibCheck->height() + mMargin;

            mWiggleCheck -> move(mMargin + dx, ySpan);
            ySpan += mWiggleCheck->height() + mMargin;
        }
    }

    mStatCheck->move(mMargin , ySpan);
    ySpan += mStatCheck->height() + mMargin;

    mResultsGroup->resize(mOptionsW, ySpan);
    mTabByScene->resize(mOptionsW, mTabByScene->minimalHeight());
    update();
}


void ResultsView::updateTabByTempo()
{
    if (mTabByScene->currentIndex() != 2)
        return;

    /* ----------------------------------------------------------
     *  Results options layout, member within  mTempoGroup
     * ----------------------------------------------------------*/
    int ySpan (mMargin);

    mDurationRadio -> move(mMargin, ySpan);
    ySpan += mDurationRadio->height() + mMargin;

    mTempoRadio -> move(mMargin, ySpan);
    ySpan += mTempoRadio->height() + mMargin;
    int dx (20 + mMargin);

    if (mTempoRadio->isChecked()) {
        mTempoCredCheck->setVisible(true);
        mTempoErrCheck->setVisible(true);
        mTempoCredCheck -> move(mMargin + dx, ySpan);
        ySpan += mTempoCredCheck->height() + mMargin;
        mTempoErrCheck -> move(mMargin + dx, ySpan);
        ySpan += mTempoErrCheck->height() + mMargin;

    } else {
        mTempoCredCheck->setVisible(false);
        mTempoErrCheck->setVisible(false);
    }

    mActivityRadio -> move(mMargin, ySpan);
    ySpan += mActivityRadio->height() + mMargin;

    mTempoStatCheck->move(mMargin , ySpan);
    ySpan += mTempoStatCheck->height() + mMargin;

    mTempoGroup->resize(mOptionsW, ySpan);
    mTabByScene->resize(mOptionsW, mTabByScene->minimalHeight());
}

void ResultsView::updateTabDisplay(const int &i)
{
    const QFontMetrics fm(AppSettings::font());
    /* -------------------------------------------------------
     *  Activate specific controls for post. distrib. (first tab)
     * -------------------------------------------------------*/

    mAllChainsCheck    -> setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);

    mDensityOptsTitle -> setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && ( mCurrentVariable != GraphViewResults::eTempo
                                                                 && mCurrentVariable != GraphViewResults::eActivity) ) {
        mDensityOptsGroup -> setVisible(true);

    } else
        mDensityOptsGroup -> setVisible(false);


    mCredibilityCheck->setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    mThreshLab->setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    mFFTLenLab->setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    mBandwidthLab->setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    /* -------------------------------------------------------
     *  Switch between checkBoxes or Radio-buttons for chains
     * -------------------------------------------------------*/
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {
        for (auto &&checkChain : mCheckChainChecks)
            checkChain->setVisible(true);

        for (auto &&chainRadio : mChainRadios)
            chainRadio->setVisible(false);

    } else {
        for (auto &&checkChain : mCheckChainChecks)
            checkChain->setVisible(false);

        for (auto &&chainRadio : mChainRadios)
            chainRadio->setVisible(true);
    }

    int ySpan(0);
    qreal dy(0); // shift between Y position of the Edit and the y position of the label

    switch (i) {
    case 0: //Display tab
        {
        /*
         * Span Options
         *
         * */

        mSpanTitle->move(0, 3);
        int wBut = mOptionsW/3;
        ySpan =  mMargin;// Reset ySpan inside mSpanGroup
        if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {
            if ( mCurrentVariable == GraphViewResults::eTheta
                || mCurrentVariable == GraphViewResults::eTempo
                || mCurrentVariable == GraphViewResults::eActivity ) {

                mDisplayStudyBut->setText(tr("Study Period Display"));
                mDisplayStudyBut->setVisible(true);
                mDisplayStudyBut->move(mMargin, ySpan);
                ySpan += mDisplayStudyBut->height() + mMargin;
            }
            else if (mCurrentVariable == GraphViewResults::eSigma || (mCurrentVariable == GraphViewResults::eDuration)) {
                mDisplayStudyBut->setText(tr("Fit Display"));
                mDisplayStudyBut->setVisible(true);
                mDisplayStudyBut->move(mMargin, ySpan);
                ySpan += mDisplayStudyBut->height() + mMargin;
            }
            else
                mDisplayStudyBut->setVisible(false);
        } else
            mDisplayStudyBut->setVisible(false);

        mCurrentXMinEdit->move(mMargin, ySpan);

        mCurrentXMaxEdit->move(mOptionsW - mCurrentXMinEdit->width() -  mMargin, ySpan );

        const int w = mSpanLab->width();
        dy = (mXScaleSpin->height() - mSpanLab->height()) /2.;
        //mSpanLab->setGeometry((mCurrentXMinEdit->x() + mCurrentXMinEdit->width() + mCurrentXMaxEdit->x() )/2. - (w/2.), mCurrentXMinEdit->y() , w, mCurrentXMinEdit->height() );
        mSpanLab->move((mCurrentXMinEdit->x() + mCurrentXMinEdit->width() + mCurrentXMaxEdit->x() )/2. - (w/2.), mCurrentXMinEdit->y() + dy);
        ySpan += mMargin + mCurrentXMinEdit->height();

        int heiTemp = mXScaleSpin->height();
        mXScaleLab->setGeometry(mMargin, ySpan , wBut - 4*mMargin, heiTemp);

        mXScaleSpin->move(mOptionsW - mXScaleSpin->width() - mMargin, ySpan);
        const int xSliderWidth = mOptionsW - mXScaleLab->width() - mXScaleSpin->width() - 4*mMargin;
        dy = (mXScaleSpin->height() - mXSlider->height()) /2.;
        mXSlider->setGeometry(mXScaleLab->x() + mXScaleLab->width() + mMargin , ySpan, xSliderWidth, heiTemp );

        ySpan += mXScaleSpin->height() + mMargin;

        if (mCurrentTypeGraph != GraphViewResults::eCorrel) {
            mMajorScaleLab->setVisible(true);
            mMajorScaleEdit->setVisible(true);
            mMinorScaleLab->setVisible(true);
            mMinorScaleEdit->setVisible(true);


            mMajorScaleEdit->move(mOptionsW - mMargin - mMajorScaleEdit->width(), ySpan );
            dy = (mMajorScaleEdit->height() - mMajorScaleLab->height())/2.;
            //mMajorScaleLab->setGeometry(mOptionsW - 2*mMargin - mMajorScaleEdit->width() - fm.width(mMajorScaleLab->text()), ySpan , wBut - 4*mMargin, heiTemp);
            mMajorScaleLab->move(mOptionsW - 2*mMargin - mMajorScaleEdit->width() - fm.width(mMajorScaleLab->text()), ySpan + dy);

            ySpan += mMajorScaleEdit->height() + mMargin;

           // mMinorScaleLab->setGeometry(mMargin, ySpan , wBut - 4*mMargin, heiTemp);
            //mMinorScaleLab->setGeometry(mOptionsW - 2*mMargin - mMinorScaleEdit->width() - fm.width(mMinorScaleLab->text()), ySpan , wBut - 4*mMargin, heiTemp);

            mMinorScaleEdit->move(mOptionsW - mMargin - mMinorScaleEdit->width(), ySpan );
            dy = (mMinorScaleEdit->height() - mMinorScaleLab->height())/2.;
            mMinorScaleLab->move(mOptionsW - 2*mMargin - mMinorScaleEdit->width() - fm.width(mMinorScaleLab->text()), ySpan);
            ySpan += mMinorScaleEdit->height() + mMargin;

        } else {
            mMajorScaleLab->setVisible(false);
            mMajorScaleEdit->setVisible(false);
            mMinorScaleLab->setVisible(false);
            mMinorScaleEdit->setVisible(false);
        }


        // Fit the size and the position of the widget of the group in the mOptionsWidget coordonnate
        mSpanGroup->setGeometry(0, mSpanTitle->y() + mSpanTitle->height() , mOptionsW, ySpan);

        /* ----------------------------------------------------------
           *  Graphic options
           * ----------------------------------------------------------*/
        int ySpan = mMargin;
        mGraphicTitle->move(0, mSpanGroup->y()+ mSpanGroup->height());

        ySpan += mMargin;

        mYScaleSpin->move(mOptionsW - mYScaleSpin->width() - mMargin, ySpan);
        dy = (mYScaleSpin->height() - mYScaleLab->height()) /2.;
        mYScaleLab->move(mMargin, ySpan + dy);
        const int ySliderWidth = mOptionsW - mYScaleLab->width() - mYScaleSpin->width() - 4*mMargin;
        dy = (mYScaleSpin->height() - mYSlider->height()) /2.;
        mYSlider->setGeometry(mYScaleLab->x() + mYScaleLab->width() + mMargin , ySpan + dy, ySliderWidth, heiTemp);

        ySpan += mMargin + mYScaleSpin->height();
        mFontBut->move(mOptionsW/2, ySpan );
        dy = (mFontBut->height() - labFont->height()) /2.;
        labFont->move(mOptionsW/2 - mMargin -fm.width(labFont->text()), ySpan + dy);

        ySpan += mMargin + mFontBut->height();
        mThicknessCombo->move(mOptionsW/2, ySpan);
        dy = (mThicknessCombo->height() - labThickness->height()) /2.;
        labThickness->move(mOptionsW/2 - mMargin -fm.width(labThickness->text()), ySpan + dy);


        ySpan += mMargin + mThicknessCombo->height();
        mOpacityCombo->move(mOptionsW/2, ySpan);
        dy = (mOpacityCombo->height() - labOpacity->height()) /2.;
        labOpacity->move(mOptionsW/2 - mMargin -fm.width(labOpacity->text()), ySpan + dy);

        ySpan += mMargin + mOpacityCombo->height();
        //labRendering->move(mMargin, ySpan);
        //mRenderCombo->move(mOptionsW/2, ySpan);

        // fit the size and the position of the widget of the group in the mTabDisplay coordonnate
       // ySpan += mMargin + mRenderCombo->height();
        mGraphicGroup->setGeometry(0, mGraphicTitle->y()+ mGraphicTitle->height() , mOptionsW, ySpan);

        mTabDisplay->resize(mOptionsW,  mGraphicGroup->y() + mGraphicGroup->height() );

        }
        break;

    default: //Distrib. Options tab
       {
        /* ----------------------------------------------------------
           *  MCMC Chains options layout
           * ----------------------------------------------------------*/

          mChainsTitle->move(0, 3 );

          ySpan = mMargin ;

          // posterior distribution : chains are selectable with checkboxes
          const int tabIdx = mTabs->currentIndex();
          if (tabIdx == 0) {
              // inside mChainsGroup Coordonnate
              mAllChainsCheck->move(mMargin, ySpan);
              ySpan += mAllChainsCheck->height() + mMargin;


              if (mCurrentVariable != GraphViewResults::eTempo && mCurrentVariable != GraphViewResults::eActivity ) {
                  for (auto && check: mCheckChainChecks) {
                      check->move(mMargin, ySpan);
                      ySpan += check->height() + mMargin;
                  }
              }

          } else {      // trace, accept or correl : chains are selectable with radio-buttons
              for (auto && radio : mChainRadios) {
                  radio->move(mMargin, ySpan);
                  ySpan += radio->height() + mMargin;
              }
          }


          mChainsGroup->setGeometry(0, mChainsTitle->y() + mChainsTitle->height(), mOptionsW, ySpan);

          /* ----------------------------------------------------------
           *  Density Options layout
           * ----------------------------------------------------------*/
          if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {
              mDensityOptsTitle->move(0, mChainsGroup->y() + mChainsGroup->height());

              ySpan = mMargin;

              if (mCurrentVariable == GraphViewResults::eTheta || mCurrentVariable == GraphViewResults::eDuration) {
                  mCredibilityCheck->setVisible(true);
                  mCredibilityCheck->move(mMargin, ySpan);
                  ySpan += mCredibilityCheck->height() + mMargin;

              } else
                  mCredibilityCheck->setVisible(false);

              if (mCurrentVariable == GraphViewResults::eTempo || mCurrentVariable == GraphViewResults::eActivity ) {
                  mDensityOptsTitle->setVisible(false);
                  mThreshLab->setVisible(false);
                  mHPDEdit->setVisible(false);
                  mDensityOptsGroup->setGeometry(0, 0, mOptionsW, 0);

              } else {
                  mDensityOptsTitle->setVisible(true);
                  mThreshLab->setVisible(true);
                  mHPDEdit->setVisible(true);

                  mHPDEdit->move(mOptionsW - mMargin - mHPDEdit->width(), ySpan);
                  dy = (mHPDEdit->height() - mThreshLab->height())/2.;
                  mThreshLab->move(mHPDEdit->x() - fm.width(mThreshLab->text()) - mMargin, ySpan + dy);
                  ySpan += mHPDEdit->height() + mMargin;

                  mFFTLenCombo->move(mOptionsW - mMargin - mFFTLenCombo->width(), ySpan);
                  dy = (mFFTLenCombo->height() - mFFTLenLab->height())/2.;
                  mFFTLenLab->move(mFFTLenCombo->x() - fm.width(mFFTLenLab->text()) - mMargin, ySpan + dy);
                  ySpan += mFFTLenCombo->height() + mMargin;

                  mBandwidthEdit->move(mOptionsW - mMargin - mBandwidthEdit->width(), ySpan);
                  dy = (mBandwidthEdit->height() - mBandwidthLab->height())/2.;
                  mBandwidthLab->move(mBandwidthEdit->x() - fm.width(mBandwidthLab->text()) - mMargin, ySpan);
                  ySpan += mBandwidthEdit->height() + mMargin;

                  mDensityOptsGroup->setGeometry(0, mDensityOptsTitle->y() + mDensityOptsTitle->height(), mOptionsW, ySpan);
              }


              mTabMCMC->resize(mOptionsW, mDensityOptsGroup->y() + mDensityOptsGroup->height() + 5) ;

          } else
              mTabMCMC->resize(mOptionsW, mChainsGroup->y() + mChainsGroup->height() + 5) ;


        }
        break;
    }
    mTabDisplayMCMC->resize(mOptionsW, mTabDisplayMCMC->minimalHeight());

}

void ResultsView::updateTabPageSaving()
{
    const bool byEvents (mTabByScene->currentIndex() == 0);
    const bool byPhases (mTabByScene->currentIndex() == 1);
    const bool byTempo (mTabByScene->currentIndex() == 2);

    
    switch (mTabPageSaving->currentIndex()) {
    case 0:
        {
            /*
             *  Page Navigator
             */
            int ySpan (mMargin);
            qreal dy (0);
            mPreviousSheetBut->move(0 , ySpan);

            mSheetNum->move(mPreviousSheetBut->width(), ySpan);

            mSheetNum->setText(locale().toString(byPhases ? mTabPhasesIndex+1 : mTabEventsIndex+1 ) + "/" +
                               locale().toString(ceil((double)mMaximunNumberOfVisibleGraph/(double)mNumberOfGraph) ));

            mNextSheetBut->move(mOptionsW - mPreviousSheetBut->width(),  ySpan);

            ySpan += mNextSheetBut->height() + mMargin;

            mNbDensitySpin->move(mOptionsW - mNbDensitySpin->width() - mMargin, ySpan);
            dy = (mNbDensitySpin->height() - mNbDensityLab->height())/2.;
            mNbDensityLab->move(mMargin, ySpan + dy);
            ySpan += mNbDensitySpin->height() + mMargin;

            mPageWidget->resize(mOptionsW, ySpan);

        }
        break;
    case 1:
        {
            /* ----------------------------------------------------------
             *  lookfor selection in the graphView
             * ----------------------------------------------------------*/
            bool singleGraph (false);
            if (byPhases) {
                if (mPhasesScrollArea)
                    for (auto &&graph : mByPhasesGraphs)
                        if (graph->isSelected()) {
                            singleGraph = true;
                            continue;
                        }

            }
            else if (byTempo) {
                if (mTempoScrollArea)
                    for (auto &&graph : mByTempoGraphs)
                        if (graph->isSelected()) {
                            singleGraph = true;
                            continue;
                        }

            }
            else  if (byEvents) {
                if (mEventsScrollArea)
                    for (auto &&graph : mByEventsGraphs)
                        if (graph->isSelected()) {
                            singleGraph = true;
                            continue;
                        }
             }


            if (singleGraph) {
                mImageSaveBut->setVisible(true);
                mImageClipBut->setVisible(true);
                mResultsClipBut->setVisible(true);
                mDataSaveBut->setVisible(true);
                mImageSaveBut->move(0, mMargin);
                mImageClipBut->move(mImageSaveBut->x() + mImageSaveBut->width(), mMargin);
                mResultsClipBut->move(mImageClipBut->x() + mImageClipBut->width(), mMargin);
                mDataSaveBut->move(mResultsClipBut->x() + mResultsClipBut->width(), mMargin);

                mExportImgBut->setVisible(false);
                mExportResults->setVisible(false);

                mToolsWidget->resize(mOptionsW, mDataSaveBut->height() + 2*mMargin);

            } else {
                mImageSaveBut->setVisible(false);
                mImageClipBut->setVisible(false);
                mResultsClipBut->setVisible(false);
                mDataSaveBut->setVisible(false);

                mExportImgBut->setVisible(true);
                mExportResults->setVisible(true);
                mExportImgBut->move(0, mMargin);
                mExportResults->move(mExportImgBut->x() + mExportImgBut->width(), mMargin);

                mToolsWidget->resize(mOptionsW, mExportImgBut->height() + 2*mMargin);
            }


        }
        break;


    default:
        break;
    }

    mTabPageSaving->resize(mOptionsW, mTabPageSaving->minimalHeight());

}

void ResultsView::updateNbDensity(int i)
{
   mNumberOfGraph = i;

   //the same code that unfoldToggle()
   if (mStack->currentWidget() == mEventsScrollArea)
        mTabEventsIndex = 0;

   else if (mStack->currentWidget() == mPhasesScrollArea)
      mTabPhasesIndex = 0;

   else if (mStack->currentWidget() == mTempoScrollArea)
      mTabTempoIndex = 0;

   updateControls();

   emit updateScrollAreaRequested();

}


void ResultsView::changeScrollArea()
{
    const bool byTempo (mTabByScene->currentIndex() == 2);
    const bool byPhases (mTabByScene->currentIndex() == 1);
    const bool byEvents (mTabByScene->currentIndex() == 0);

    if (byPhases && !mEventsfoldCheck->isChecked() ) {
        mDatesfoldCheck->setChecked(false);
        mDatesfoldCheck->setEnabled(false);
    } else
        mDatesfoldCheck->setEnabled(true);

    // Append when we toggle mByPhasesBut to mByEventsBut and mDurationRadio is checked


    if (mDataThetaRadio->isChecked())
        mCurrentVariable = GraphViewResults::eTheta;

    else if (mDataSigmaRadio->isChecked())
        mCurrentVariable = GraphViewResults::eSigma;

    else if (mDurationRadio->isChecked())
            mCurrentVariable = GraphViewResults::eDuration;

    else if (mTempoRadio->isChecked())
            mCurrentVariable = GraphViewResults::eTempo;

    else if (mActivityRadio->isChecked())
            mCurrentVariable = GraphViewResults::eActivity;

    if (byEvents)
            createEventsScrollArea(mTabEventsIndex);

    else if (byPhases)
            createPhasesScrollArea(mTabPhasesIndex);

    else if (byTempo) {
            mCurrentTypeGraph = GraphViewResults::ePostDistrib;
            createTempoScrollArea(mTabTempoIndex);
    }

}

/**
 * @brief ResultsView::updateVisibleTabs Update mTabs according to mTabByScene index
 * @param index
 */
void ResultsView::updateVisibleTabs(const int &index)
{

    switch (index) {
        case 0: //mTabByScene on Events
            {
            mTabs->setTabVisible(0, true); // Posterior Distrib.
            mTabs->setTabVisible(1, true); // History Plot
            mTabs->setTabVisible(2, true); // Acceptance Rate
            mTabs->setTabVisible(3, true); // Autocorrelation
        }
        break;

        case 1: //mTabByScene on Phases
            {
            mTabs->setTabVisible(0, true); // Posterior Distrib.
            mTabs->setTabVisible(1, true); // History Plot
            mTabs->setTabVisible(2, true); // Acceptance Rate
            mTabs->setTabVisible(3, true); // Autocorrelation
        }
        break;
        case 2: //mTabByScene on Tempo
            {
            mTabs->setTabVisible(0, true); // Posterior Distrib.
            mTabs->setTabVisible(1, false); // History Plot
            mTabs->setTabVisible(2, false); // Acceptance Rate
            mTabs->setTabVisible(3, false); // Autocorrelation
            mTabs->setTab(0, true);
        }
        break;


        default:
        break;
    }


}

void ResultsView::updateLayout()
{
   // Q_ASSERT(mModel);

    //qDebug() << "ResultsView::updateLayout()";

    const int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int tabsShift (AppSettings::widthUnit());

    /* ----------------------------------------------------------
     *  Main layout
     * ----------------------------------------------------------*/

    mTabs->setGeometry(tabsShift, mMargin, width() - mOptionsW - sbe - tabsShift, mTabsH);

    mMarker->setGeometry(mMarker->pos().x(), mTabsH + sbe, mMarker->thickness(), height() - sbe - mTabsH);

    setGraphFont(mGraphFont);

    mRuler->updateLayout();

    if (mStatCheck->isChecked() || mTempoStatCheck->isChecked())
         mRuler->setGeometry(0, mTabs->y() + mTabs->height(), (width() - mOptionsW - sbe)*2./3., mRulerH);
    else
        mRuler->setGeometry(0, mTabs->y() + mTabs->height(), width() - mOptionsW - sbe, mRulerH );
    //const int statusHeight = 0;//MainWindow.statusBar()->height();
    mStack->setGeometry(0, mTabsH + 1.2 * mRulerH + 2, width() - mOptionsW, height() - 1.2 * mRulerH - mTabsH);
    mMarker->setGeometry(mMarker->pos().x(), mTabsH + sbe, mMarker->thickness(), height() - sbe - mTabsH);
    
    /* ----------------------------------------------------------
     *  Display Options layout
     * ----------------------------------------------------------*/
    int ySpan(mMargin);

    mTabByScene->move(0, ySpan);
    updateTabByScene();
    updateTabByTempo();

    if (mCurrentVariable == GraphViewResults::eTempo || mCurrentVariable == GraphViewResults::eActivity) {
        mTabDisplayMCMC->setTabVisible(1, false);
        mTabDisplayMCMC->setTab(0, false);
     } else
        mTabDisplayMCMC->setTabVisible(1, true);

    /*
     *  mTabDisplayMCMC
     */

    ySpan = mTabByScene->y() + mTabByScene->height() + 5;

    updateTabDisplay(mTabDisplayMCMC->currentIndex());
    mTabDisplayMCMC->move(0, ySpan);
    mTabDisplayMCMC->resize(mOptionsW, mTabDisplayMCMC->minimalHeight());

    ySpan = mTabDisplayMCMC->y() + mTabDisplayMCMC->height() + 5;
    updateTabPageSaving();
    mTabPageSaving->move(0, ySpan);

    mOptionsWidget->move(width() - mOptionsW, 0);
    mOptionsWidget->resize(mOptionsW, mTabPageSaving->y() + mTabPageSaving->height() + 10 );
    mOptionsWidget->repaint();
    updateGraphsLayout();
}


void ResultsView::updateGraphsLayout()
{
  //qDebug() << "ResultsView::updateGraphsLayout()";
    const int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const bool byEvents (mTabByScene->currentIndex() == 0);
    const bool byPhases (mTabByScene->currentIndex() == 1);
    const bool byTempo  (mTabByScene->currentIndex() == 2);
    //

    /* ----------------------------------------------------------
     *  Graphs by phases layout
     *    mRuler  -> setGeometry(mGraphLeft, mTabsH, width() - mGraphLeft - mOptionsW - sbe, mRulerH);
     * ----------------------------------------------------------*/
    if (byPhases) {
        int y (0);
        if (mPhasesScrollArea) {
            QWidget* wid = mPhasesScrollArea->widget();
            for (auto &&graph : mByPhasesGraphs) {
                graph->setGeometry(0, y, width() - mOptionsW - sbe, mGraphHeight);
                graph->update();
                y += graph->height();
            }
            if (y>0)
                wid->setFixedSize(width() - sbe - mOptionsW, y);
            mPhasesScrollArea->repaint();
       }
    }
    /* ----------------------------------------------------------
     *  Graphs by events layout
     * ----------------------------------------------------------*/
   
    else if (byEvents){
         int y (0);
         if (mEventsScrollArea) {
            QWidget* wid = mEventsScrollArea->widget();

            for (auto &&graph : mByEventsGraphs) {
                graph->setGeometry(0, y, width() - mOptionsW - sbe, mGraphHeight);
                graph->update();
                y += graph->height();
            }
            if (y>0)
                wid->setFixedSize(width() - sbe - mOptionsW, y);
            mEventsScrollArea->repaint();
         }
        
    }
    else if (byTempo){
         int y (0);
         if (mTempoScrollArea) {
            QWidget* wid = mTempoScrollArea->widget();

            for (auto &&graph : mByTempoGraphs) {
                graph->setGeometry(0, y, width() - mOptionsW - sbe, mGraphHeight);
                graph->update();
                y += graph->height();
            }
            if (y>0)
                wid->setFixedSize(width() - sbe - mOptionsW, y);
            mTempoScrollArea->repaint();
         }

    }
    update();
    
}


void ResultsView::clearResults()
{

     if (mChains.size() != mCheckChainChecks.size() ) {

        for (auto &&check : mCheckChainChecks )
            delete check;

        mCheckChainChecks.clear();

        for (auto &&chain : mChainRadios)
            delete chain;

        mChainRadios.clear();
     }

    for (auto &&graph : mByEventsGraphs)
        delete graph;

    mByEventsGraphs.clear();
    mByPhasesGraphs.clear();
    mByTempoGraphs.clear();

    if (mEventsScrollArea) {
        mStack->removeWidget(mEventsScrollArea);
        delete mEventsScrollArea;
    }
    if (mPhasesScrollArea) {
        mStack->removeWidget(mPhasesScrollArea);
        delete mPhasesScrollArea;
    }

    if (mTempoScrollArea) {
        mStack->removeWidget(mTempoScrollArea);
        delete mTempoScrollArea;
    }
    mEventsScrollArea = nullptr;
    mPhasesScrollArea = nullptr;
    mTempoScrollArea = nullptr;

    mTabEventsIndex = 0;
    mTabPhasesIndex = 0;
    mTabTempoIndex = 0;

}

/**
 * @brief ResultsView::updateFormatSetting come from AppSettingsDialog
 * @param model
 */
void ResultsView::updateFormatSetting(Model* model)
{
    if (!mModel && !model)
        return;
    if (model)
        mModel = model;
    mModel->updateFormatSettings();
    applyAppSettings();
    updateControls();

}

/**
 * @brief : This function is call after "Run"
 *
 */
void ResultsView::initResults(Model* model)
{
    if (!mModel && !model)
        return;

    if (model)
        mModel = model;

    mChains = mModel->mChains;
    mSettings = mModel->mSettings;
    mMCMCSettings = mModel->mMCMCSettings;

    clearResults();

    QFontMetricsF gfm(mGraphFont);
    QLocale locale = QLocale();
    mMarginLeft = 0;//gfm.width(stringForGraph(mModel->mSettings.getTminFormated()));
     //       std::max(gfm.width(locale.toString(DateUtils::convertToAppSettingsFormat(mModel->mSettings.mTmin))),
         //                                            gfm.width(locale.toString(DateUtils::convertToAppSettingsFormat(mModel->mSettings.mTmin)))) + 5;
    Scale xScale;
    xScale.findOptimal(mModel->mSettings.mTmin, mModel->mSettings.mTmax, 7);

    mMajorScale = xScale.mark;
    mMinorCountScale = 4;

    mRuler->setRange(mSettings.getTminFormated(), mSettings.getTmaxFormated());
    mRuler->setCurrent(mSettings.getTminFormated(), mSettings.getTmaxFormated());
    mRuler->setScaleDivision(mMajorScale, mMinorCountScale);

    mMajorScaleEdit->setText(locale.toString(mMajorScale));
    mMinorScaleEdit->setText(locale.toString(mMinorCountScale));

    mHasPhases = (mModel->mPhases.size() > 0);

    // set the variable and the graphic type
    mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    mCurrentVariable = GraphViewResults::eTheta;

    if (mHasPhases) {
        mTabByScene->setTab(1, false);
        mTabByScene->setTabVisible(1, true);
        mTabByScene->setTabVisible(2, true);
        updateVisibleTabs(1);

     } else {
        mTabByScene->setTabVisible(2, false);
        mTabByScene->setTabVisible(1, false);
        mTabByScene->setTab(0, false);
        updateVisibleTabs(0);
     }
    /* ----------------------------------------------------
     *  Create Chains option controls (radio and checkboxes under "MCMC Chains")
     * ---------------------------------------------------- */
    if (mCheckChainChecks.isEmpty()) {
        for (int i=0; i<mChains.size(); ++i) {
            CheckBox* check = new CheckBox(tr("Chain %1").arg(QString::number(i+1)), mChainsGroup);
            connect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
            check->setVisible(true);
            check->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
            mCheckChainChecks.append(check);

            RadioButton* radio = new RadioButton(tr("Chain %1").arg(QString::number(i+1)), mChainsGroup);
            connect(radio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);
            radio->setVisible(true);
            if (i == 0)
                radio->setChecked(true);
            radio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);
            mChainRadios.append(radio);
        }
    }

    /* ------------------------------------------------------------
    *  This generates post. densities, HPD and credibilities !
    *  It will then call in chain :
    *  - generateCredibilityAndHPD
    *  - generateCurves
    *  - updateCurvesToShow
    * ------------------------------------------------------------ */

    mResultMaxDuration = 0.;
    mResultMaxVariance = 0.;
    mModel->initDensities(getFFTLength(), getBandwidth(), getThreshold());

    setStudyPeriod();

    showInfos(false);
 //   applyAppSettings();
    updateLayout();
    updateControls();


}

/**
 * @brief This function is call after click on "Results", when switching from Model panel to Result panel
 */
void ResultsView::updateResults(Model* model)
{
    qDebug() << "ResultsView::updateResults()";
    Q_ASSERT(model);

    if (!mModel && !model)
        return;

    mModel = model;

    mChains = mModel->mChains;
    mSettings = mModel->mSettings;
    mMCMCSettings = mModel->mMCMCSettings;

    /* ----------------------------------------------------
    *  Update Chains option controls (radio and checkboxes under "MCMC Chains")
    * ---------------------------------------------------- */

    if (mCheckChainChecks.isEmpty()) {
        for (int i = 0; i<mChains.size(); ++i) {
            CheckBox* check = new CheckBox(tr("Chain %1").arg(QString::number(i+1)), mChainsGroup);
            connect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
            check->setVisible(true);
            check->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
            mCheckChainChecks.append(check);

            RadioButton* radio = new RadioButton(tr("Chain %1").arg(QString::number(i+1)), mChainsGroup);
            connect(radio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);
            radio->setVisible(true);
            radio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);
            if (i == 0)
                radio->setChecked(true);
            mChainRadios.append(radio);
        }
    } else {

        for (int i = 0; i<mChains.size(); ++i) {
           mCheckChainChecks[i]->setVisible(true);
           mChainRadios[i]->setVisible(true);
        }
    }

    /* ------------------------------------------------------------
    *  This generates post. densities, HPD and credibilities !
    *  It will then call in chain :
    *  - generateCredibilityAndHPD
    *  - generateCurves
    *  - updateCurvesToShow
    *
    * mModel->updateDensities() check if FFTlength, Bandwidth or Threshold has changed
    * before calculation
    * ------------------------------------------------------------*/

    mModel->updateDensities(getFFTLength(), getBandwidth(), getThreshold());
    /* ----------------------------------------------------
    *  Events Views : generate all phases graph
    *  No posterior density has been computed yet!
    *  Graphs are empty at this moment
    * ----------------------------------------------------*/
//    if (mHasPhases)
//        mTabByScene->setTab(1, false);
        //createPhasesScrollArea(mTabPhasesIndex);
//    else
 //       mTabByScene->setTab(0, false);
        //createEventsScrollArea(mTabEventsIndex);

    // ------------------------------------------------------------
   // showInfos(false);
    updateControls();

    updateTabByScene();


}


void ResultsView::createEventsScrollArea(const int idx)
{

    qDebug()<<"ResultsView::createEventsScrollArea()";
    if (!mEventsScrollArea) {
        mEventsScrollArea = new QScrollArea();
        mEventsScrollArea->setMouseTracking(true);
        mStack->addWidget(mEventsScrollArea);
        mStack->setMouseTracking(true);
    }

    if (!mByEventsGraphs.isEmpty())
        for (auto && g : mByEventsGraphs) {
            disconnect(g, &GraphViewResults::selected, this, &ResultsView::updateLayout);
            delete g;
        }
    mByEventsGraphs.clear();


    // eventsWidget Creation in the idx limit
    QWidget* eventsWidget = new QWidget(mEventsScrollArea);
    eventsWidget->setMouseTracking(true);

    QList<Event*>::const_iterator iterEvent = mModel->mEvents.cbegin();
    int counter = 1;

    /*
     * Looking for at least one event selected,
     * if not we show all events
     */
    bool showAllEvents (true);
    for (auto && ev : mModel->mEvents) {
        if (ev->mIsSelected) {
            showAllEvents = false;
            break;
        }
    }


    while (iterEvent!=mModel->mEvents.cend()) {
        if ((*iterEvent)->mIsSelected || showAllEvents) {
            if ( (idx*mNumberOfGraph)<counter && counter <= ((idx+1)*mNumberOfGraph) ) {
                GraphViewEvent* graphEvent = new GraphViewEvent(eventsWidget);
                connect(graphEvent, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                graphEvent->setSettings(mModel->mSettings);
                graphEvent->setMCMCSettings(mModel->mMCMCSettings, mChains);
                graphEvent->setEvent((*iterEvent));
                graphEvent->setGraphFont(mGraphFont);
                graphEvent->setGraphsThickness(mThicknessCombo->currentIndex());
                graphEvent->changeXScaleDivision(mMajorScale, mMinorCountScale);

                mByEventsGraphs.append(graphEvent);
              }
            ++counter; //count one event graph
            if (mDatesfoldCheck->isChecked()) {
                    if ((*iterEvent)->mType != Event::eKnown) {
                        for (int j=0; j<(*iterEvent)->mDates.size(); ++j) {
                            if ( (idx*mNumberOfGraph)<counter && counter <= (idx+1)*mNumberOfGraph) {
                                Date& date = (*iterEvent)->mDates[j];
                                /* ----------------------------------------------------
                                *  This just creates the view for the date.
                                *  It sets the Date which triggers an update() to repaint the view.
                                *  The refresh() function which actually creates the graph curves will be called later.
                                * ---------------------------------------------------- */

                                GraphViewDate* graphDate = new GraphViewDate(eventsWidget);
                                graphDate->setSettings(mModel->mSettings);
                                graphDate->setMCMCSettings(mModel->mMCMCSettings, mChains);
                                graphDate->setDate(&date);
                                graphDate->setColor((*iterEvent)->mColor);

                                graphDate->setGraphFont(mGraphFont);
                                graphDate->setGraphsThickness(mThicknessCombo->currentIndex());
                                graphDate->setGraphsOpacity(mOpacityCombo->currentIndex()*10);
                                graphDate->changeXScaleDivision(mMajorScale, mMinorCountScale);

                                connect(graphDate, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                                mByEventsGraphs.append(graphDate);

                            }
                            ++counter;
                        }
                    }

            }
        }
        ++iterEvent;
    }
    
    mEventsScrollArea->setWidget(eventsWidget);

    mEventsScrollArea->update();
    qDebug()<<"ResultsView::createEventsScrollArea()"<<counter<<" items";
    mStack->setMouseTracking(true);
    emit generateCurvesRequested();

}

void ResultsView::createPhasesScrollArea(const int idx)
{
    qDebug()<<"ResultsView::createPhasesScrollArea()";

    if (!mPhasesScrollArea) {
        mPhasesScrollArea = new QScrollArea(this);
        mPhasesScrollArea->setMouseTracking(true);
        mStack->addWidget(mPhasesScrollArea);
        mStack->setMouseTracking(true);
    }
    if ( !mByPhasesGraphs.isEmpty())
        for (auto&& g : mByPhasesGraphs) {
          disconnect(g, &GraphViewResults::selected, this, &ResultsView::updateLayout);
          delete g;
         }
    mByPhasesGraphs.clear();


    QWidget* phasesWidget = new QWidget(this);
    phasesWidget->setMouseTracking(true);

    // In a Phases at least, we have one Event with one Date
    //mByPhasesGraphs.reserve( (int)(3*mModel->mPhases.size()) );

    QList<Phase*>::const_iterator iterPhase = mModel->mPhases.cbegin();
    int counter (1);

    /*
     * Looking for at least one phase selected,
     * if not we show all phases
     */
    bool showAllPhases (true);
    for (auto && ph : mModel->mPhases) {
        if (ph->mIsSelected) {
            showAllPhases = false;
            break;
        }
    }

    while (iterPhase!=mModel->mPhases.cend()) {
        if ((*iterPhase)->mIsSelected || showAllPhases) {
            if ( (idx*mNumberOfGraph)<counter && counter <= ((idx+1)*mNumberOfGraph) ) {
                GraphViewPhase* graphPhase = new GraphViewPhase(phasesWidget);
                graphPhase->setSettings(mModel->mSettings);
                graphPhase->setMCMCSettings(mModel->mMCMCSettings, mChains);
                graphPhase->setPhase((*iterPhase));
                graphPhase->setGraphFont(mGraphFont);
                graphPhase->setGraphsThickness(mThicknessCombo->currentIndex());
                graphPhase->changeXScaleDivision(mMajorScale, mMinorCountScale);
                connect(graphPhase, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                mByPhasesGraphs.append(graphPhase);
             }
            ++ counter;//count one phase graph
            if (mEventsfoldCheck->isChecked()) {

                QList<Event*>::const_iterator iterEvent = (*iterPhase)->mEvents.cbegin();
                while (iterEvent!=(*iterPhase)->mEvents.cend()) {
                    if ( (idx*mNumberOfGraph)<counter && counter <= (idx+1)*mNumberOfGraph) {
                        GraphViewEvent* graphEvent = new GraphViewEvent(phasesWidget);
                        graphEvent->setSettings(mModel->mSettings);
                        graphEvent->setMCMCSettings(mModel->mMCMCSettings, mChains);
                        graphEvent->setEvent((*iterEvent));
                        graphEvent->setGraphFont(mGraphFont);
                        graphEvent->setGraphsThickness(mThicknessCombo->currentIndex());
                        graphEvent->changeXScaleDivision(mMajorScale, mMinorCountScale);
                        connect(graphEvent, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                        mByPhasesGraphs.append(graphEvent);
                    }
                    ++ counter; // count one Event
                    // --------------------------------------------------
                    //  This just creates the GraphView for the date (no curve yet)
                    // --------------------------------------------------
                    if (mDatesfoldCheck->isChecked()) {
                        for (int j=0; j<(*iterEvent)->mDates.size(); ++j) {
                            if ( (idx*mNumberOfGraph)<counter && counter <= (idx+1)*mNumberOfGraph) {
                                Date& date = (*iterEvent)->mDates[j];
                                GraphViewDate* graphDate = new GraphViewDate(phasesWidget);
                                graphDate->setSettings(mModel->mSettings);
                                graphDate->setMCMCSettings(mModel->mMCMCSettings, mChains);
                                graphDate->setDate(&date);
                                graphDate->setColor((*iterEvent)->mColor);
                                graphDate->setGraphFont(mGraphFont);
                                graphDate->setGraphsThickness(mThicknessCombo->currentIndex());
                                graphDate->changeXScaleDivision(mMajorScale, mMinorCountScale);
                                connect(graphDate, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                                mByPhasesGraphs.append(graphDate);
                            }
                            ++ counter; //count one Date
                        }
                    }
                    ++iterEvent;
                }
            }
        }
        ++iterPhase;

    }

    mPhasesScrollArea->setWidget(phasesWidget);
    mPhasesScrollArea->update();

    emit generateCurvesRequested();
}

void ResultsView::createTempoScrollArea(const int idx)
{
    qDebug()<<"ResultsView::createTempoScrollArea()";
    
    if (!mTempoScrollArea) {
        mTempoScrollArea = new QScrollArea(this);
        mTempoScrollArea->setMouseTracking(true);
        mStack->addWidget(mTempoScrollArea);
        mStack->setMouseTracking(true);
    }
    if ( !mByTempoGraphs.isEmpty())
        for (auto&& g : mByTempoGraphs) {
          disconnect(g, &GraphViewTempo::selected, this, &ResultsView::updateLayout);
          delete g;
         }
    mByTempoGraphs.clear();


    QWidget* tempoWidget = new QWidget(this);
    tempoWidget->setMouseTracking(true);
    
    // In a Phases at least, we have one Event with one Date
    //mByPhasesGraphs.reserve( (int)(3*mModel->mPhases.size()) );

    QList<Phase*>::const_iterator iterPhase = mModel->mPhases.cbegin();
    int counter (1);

    /*
     * Looking for at least one phase selected,
     * if not we show all phases
     */
    bool showAllPhases (true);
    for (auto && ph : mModel->mPhases) {
        if (ph->mIsSelected) {
            showAllPhases = false;
            break;
        }
    }

    while (iterPhase!=mModel->mPhases.cend()) {
        if ((*iterPhase)->mIsSelected || showAllPhases) {
            if ( (idx*mNumberOfGraph)<counter && counter <= ((idx+1)*mNumberOfGraph) ) {
                GraphViewTempo* graphTempo = new GraphViewTempo(tempoWidget);
                graphTempo->setSettings(mModel->mSettings);
                graphTempo->setMCMCSettings(mModel->mMCMCSettings, mChains);
                graphTempo->setPhase((*iterPhase));
                graphTempo->setGraphFont(mGraphFont);
                graphTempo->setGraphsThickness(mThicknessCombo->currentIndex());
                graphTempo->changeXScaleDivision(mMajorScale, mMinorCountScale);
                connect(graphTempo, &GraphViewTempo::selected, this, &ResultsView::updateLayout);
                mByTempoGraphs.append(graphTempo);
             }
            ++ counter;//count one phase graph

        }
        ++iterPhase;

    }

    mTempoScrollArea->setWidget(tempoWidget);
    mTempoScrollArea->update();
    //qDebug()<<"ResultsView::createTempoScrollArea()"<<counter<<" items";
    emit generateCurvesRequested();
}

int ResultsView::getFFTLength() const
{
    return mFFTLenCombo->currentText().toInt();
}

void ResultsView::setFFTLength()
{
    qDebug() << "ResultsView::setFFTLength()";
    const int len = mFFTLenCombo->currentText().toInt();
    mModel->setFFTLength(len);
}

double ResultsView::getBandwidth() const
{
    return mBandwidthUsed;
}

void ResultsView::setBandwidth()
{
    qDebug() << "ResultsView::setBandwidth()";
    bool ok;
    const double bandwidth = locale().toDouble(mBandwidthEdit->text(), &ok);
    if (!(bandwidth > 0 && bandwidth <= 100) || !ok)
        mBandwidthEdit->setText(locale().toString(bandwidth));
    
    if (bandwidth != getBandwidth()) {
        mBandwidthUsed = bandwidth;
        mModel->setBandwidth(bandwidth);
    }

}

double ResultsView::getThreshold() const
{
    return mThresholdUsed;
}

void ResultsView::graphTypeChange()
{
   const int tabIdx = mTabs->currentIndex();
    
   if (tabIdx == 0)
        mCurrentTypeGraph = GraphViewResults::ePostDistrib;

   else if (tabIdx == 1)
        mCurrentTypeGraph = GraphViewResults::eTrace;

   else if (tabIdx == 2)
        mCurrentTypeGraph = GraphViewResults::eAccept;

   else if (tabIdx == 3)
        mCurrentTypeGraph = GraphViewResults::eCorrel;

   updateTabDisplay(mTabDisplayMCMC->currentIndex());
   emit generateCurvesRequested();
}

/**
 * @brief ResultsView::setThreshold The controle of the value is done with a QValidator set in the constructor
 */
void ResultsView::setThreshold()
{
    qDebug() << "ResultsView::setThreshold()";
    const QLocale locale;
    const double hpd = locale.toDouble(mHPDEdit->text());
    if (hpd != getThreshold()) {
        mThresholdUsed = hpd;
        mModel->setThreshold(hpd);
    }

}

void ResultsView::unfoldToggle()
{

    if (mStack->currentWidget() == mEventsScrollArea) {
         mTabEventsIndex = 0;
        
    } else if (mStack->currentWidget() == mPhasesScrollArea) {
       mTabPhasesIndex = 0;
    }
    
    emit updateScrollAreaRequested();
}

void ResultsView::nextSheet()
{    
    int* currentIndex (nullptr);

    if (mStack->currentWidget() == mEventsScrollArea)
        currentIndex = &mTabEventsIndex;

    else if (mStack->currentWidget() == mPhasesScrollArea)
        currentIndex = &mTabPhasesIndex;

    else if (mStack->currentWidget() == mTempoScrollArea)
        currentIndex = &mTabTempoIndex;
    
    else
        return;
  
    if ( ( ((*currentIndex) + 1)*mNumberOfGraph) < mMaximunNumberOfVisibleGraph )
        ++(*currentIndex);
    
    emit updateScrollAreaRequested();
    
    
}

void ResultsView::previousSheet()
{
    if ((mTabByScene->currentIndex() == 0) && (mTabEventsIndex>0))
        --mTabEventsIndex;

    else if ((mTabByScene->currentIndex() == 1) && (mTabPhasesIndex>0))
        --mTabPhasesIndex;

    else if ((mTabByScene->currentIndex() == 2) && (mTabTempoIndex>0))
        --mTabTempoIndex;
    
    else
        return;
    
    emit updateScrollAreaRequested();
}

/**
 *  @brief Decide which curve graphs must be show, based on currently selected options.
 *  @brief This function does NOT remove or create any curve in graphs! It only checks if existing curves should be visible or not.
 */
void ResultsView::updateCurvesToShow()
{
    qDebug() << "ResultsView::updateCurvesToShow";
    const bool showAllChains = mAllChainsCheck->isChecked();
    QList<bool> showChainList;
    
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib)
        for (CheckBox* cbButton : mCheckChainChecks)
            showChainList.append(cbButton->isChecked());

    else
        for (RadioButton* rButton : mChainRadios)
            showChainList.append(rButton->isChecked());

    const bool showCalib = mDataCalibCheck->isChecked();
    const bool showWiggle = mWiggleCheck->isChecked();
    const bool showCredibility = mCredibilityCheck->isChecked();
    const bool showStat = mStatCheck->isChecked();

    if (mTabByScene->currentIndex() == 0 )
            for (GraphViewResults* eventGraph : mByEventsGraphs) {
                eventGraph->setShowNumericalResults(showStat);
                eventGraph->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
            }

    else if (mTabByScene->currentIndex() == 1 )
        for (GraphViewResults* phaseGraph : mByPhasesGraphs) {
            phaseGraph->setShowNumericalResults(showStat);
            phaseGraph->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
        }

    else if (mTabByScene->currentIndex() == 2 )
        for (GraphViewResults* tempoGraph : mByTempoGraphs) {
            tempoGraph->setShowNumericalResults(showStat);
            if (mCurrentVariable == GraphViewResults::eTempo)
                tempoGraph->updateCurvesToShow(showAllChains, showChainList, mTempoCredCheck->isChecked(), mTempoErrCheck->isChecked(), showWiggle);

            else // (mCurrentVariable == GraphViewResults::eDuration) || (mCurrentVariable == GraphViewResults::eActivity)
             tempoGraph->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
        }

    updateScales();
}



/**
 *  @brief re-generate all curves in graph views form model data.
 *  @brief Each curve is given a name. This name will be used by updateCurvesToShow() to decide whether the curve is visible or not.
 */
void ResultsView::generateCurves(const QList<GraphViewResults*> &listGraphs)
{
    qDebug() << "ResultsView::generateCurves()";

    if (mTabByScene->currentIndex() == 0 || mTabByScene->currentIndex() ==1) {
        if (mDataThetaRadio->isChecked())
            mCurrentVariable = GraphViewResults::eTheta;

        else if (mDataSigmaRadio->isChecked())
            mCurrentVariable = GraphViewResults::eSigma;

    } else if (mTabByScene->currentIndex() == 2) {
        if (mDurationRadio->isChecked())
            mCurrentVariable = GraphViewResults::eDuration;

        else if (mTempoRadio->isChecked())
            mCurrentVariable = GraphViewResults::eTempo;

        else if (mActivityRadio->isChecked())
            mCurrentVariable = GraphViewResults::eActivity;

    } else
        return;

    for (auto &&graph : listGraphs)
        graph->generateCurves(GraphViewResults::TypeGraph(mCurrentTypeGraph), mCurrentVariable);

    // With variable eDuration, we look for mResultMaxDuration in the curve named "Post Distrib All Chains"
    if (mCurrentVariable == GraphViewResults::eDuration) {
        mResultMaxDuration = 0.;

        QList<GraphViewResults*>::const_iterator constIter;
        constIter = listGraphs.cbegin();
        QList<GraphViewResults*>::const_iterator iterEnd = listGraphs.cend();
        while (constIter != iterEnd) {
            const GraphViewTempo* graphPhase = dynamic_cast<const GraphViewTempo*>(*constIter);

            if (graphPhase) {
                const GraphCurve* graphDurationAll = graphPhase->getGraph()->getCurve("Post Distrib Duration All Chains");
                if (graphDurationAll)
                    mResultMaxDuration = ceil(qMax(mResultMaxDuration, graphDurationAll->mData.lastKey()));

                const int nbCurves = graphPhase->getGraph()->numCurves();
                // if we have Curves there are only Variance curves
                for (int i=0; i <nbCurves; ++i) {
                   const GraphCurve* graphDuration = graphPhase->getGraph()->getCurve("Post Distrib Duration " + QString::number(i));

                    if (graphDuration) {
                        mResultMaxDuration = ceil(qMax(mResultMaxDuration, graphDuration->mData.lastKey()));
                        mResultMaxDuration = std::max(100., mResultMaxDuration);
                    }
                }
            }
            ++constIter;
        }

    }

    // With variable eSigma, we look for mResultMaxVariance in the curve named "Post Distrib All Chains"
    if (mCurrentVariable == GraphViewResults::eSigma) {
       mResultMaxVariance = 0.;

        QList<GraphViewResults*>::const_iterator constIter;
        constIter = listGraphs.cbegin();
        QList<GraphViewResults*>::const_iterator iterEnd = listGraphs.cend();

        while (constIter != iterEnd) {
            QList<GraphCurve> curves = (*constIter)->getGraph()->getCurves();

            for (auto &&curve : curves) {
                 if (curve.mName.contains("Sigma") && (curve.mVisible == true))
                     mResultMaxVariance = ceil(qMax(mResultMaxVariance, curve.mData.lastKey()));
            }
            ++constIter;
        }
         mResultMaxVariance = std::max(100., mResultMaxVariance);

    }
    //qDebug() << "ResultsView::generateCurves()-> emit curvesGenerated()";

    emit curvesGenerated();
}

void ResultsView::updateCurves()
{
    if (mTabByScene->currentIndex() == 0)
        generateCurves(mByEventsGraphs);

    else if (mTabByScene->currentIndex() == 1)
        generateCurves(mByPhasesGraphs);

    else if (mTabByScene->currentIndex() == 2)
        generateCurves(mByTempoGraphs);

    //qDebug() << "ResultsView::updateCurves()-> emit curvesGenerated()";
    emit curvesGenerated();
}

/**
 *   @brief Restore with mZooms and mScales according to mTabs
 *  which are store in updateGraphsZoomX()
 */
void ResultsView::updateScales()
{
    //qDebug() << "ResultsView::updateScales()";
    
    int tabIdx = mTabs->currentIndex();
    ProjectSettings s = mSettings;
    
    /* ------------------------------------------
     *  Get X Range based on current options used to calculate zoom
     * ------------------------------------------*/

    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {
        if ( mCurrentVariable == GraphViewResults::eTheta
             || mCurrentVariable == GraphViewResults::eTempo
             || mCurrentVariable == GraphViewResults::eActivity) {

            mResultMinX = s.getTminFormated();
            mResultMaxX = s.getTmaxFormated();
            const double tCenter = (mResultMinX + mResultMaxX) / 2.;
            const double studySpan = mResultMaxX - mResultMinX;

            forceXSlideSetValue = true;
            mXSlider->setRange(-100, 100);

            forceXSpinSetValue = true;
            mXScaleSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
            mXScaleSpin->setSingleStep(.01);
            mXScaleSpin->setDecimals(3);

            const double tRangeMin = tCenter - ( (studySpan/2.)  / sliderToZoom( mXSlider->minimum()));
            const double tRangeMax = tCenter + ( (studySpan/2.)  / sliderToZoom( mXSlider->minimum()));

            mRuler->setRange(tRangeMin, tRangeMax);
            mRuler->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);

        } else if (mCurrentVariable == GraphViewResults::eSigma) {
            mResultMinX = 0.;
            mResultMaxX = mResultMaxVariance;

            forceXSlideSetValue = true;
            mXSlider->setRange(-100, 100);

            forceXSpinSetValue = true;
            mXScaleSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
            mXScaleSpin->setSingleStep(.01);
            mXScaleSpin->setDecimals(3);

            const double tRangeMax =  ( mResultMaxVariance  / sliderToZoom( mXSlider->minimum()));

            mRuler->setRange(0, tRangeMax);
             mRuler->setFormatFunctX(nullptr);

        } else if (mCurrentVariable == GraphViewResults::eDuration) {
            mResultMinX = 0.;
            mResultMaxX = mResultMaxDuration;

            forceXSlideSetValue = true;
            mXSlider->setRange(-100, 100);

            forceXSpinSetValue = true;
            mXScaleSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
            mXScaleSpin->setSingleStep(.01);
            mXScaleSpin->setDecimals(3);

            const double tRangeMax =  ( mResultMaxDuration  / sliderToZoom( mXSlider->minimum()));

            mRuler->setRange(0, tRangeMax);
            mRuler->setFormatFunctX(nullptr);

        }


    } else if ((mCurrentTypeGraph == GraphViewResults::eTrace) ||  (mCurrentTypeGraph == GraphViewResults::eAccept) ) {
        mResultMinX = 0.;
        for (int i = 0; i < mChainRadios.size(); ++i) {
            if (mChainRadios.at(i)->isChecked()) {
                const ChainSpecs& chain = mChains.at(i);
                mResultMaxX = 1+ chain.mNumBurnIter + (chain.mBatchIndex * chain.mNumBatchIter) + chain.mNumRunIter / chain.mThinningInterval;
                break;
            }
        }
        mRuler->setRange(mResultMinX, mResultMaxX);
        mRuler->setFormatFunctX(nullptr);

        const int rangeZoom = mResultMaxX / 100;
        forceXSlideSetValue = true;
        mXSlider->setRange(1, rangeZoom);

        forceXSpinSetValue = true;
        mXScaleSpin->setRange(1, rangeZoom);
        mXScaleSpin->setSingleStep(1.);

    } else if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
        mResultMinX = 0.;
        mResultMaxX = 40.;

        mRuler->setRange(mResultMinX, mResultMaxX);
        mRuler->setFormatFunctX(nullptr);

        forceXSlideSetValue = true;
        mXSlider->setRange(1, 5);   // we can zoom 5 time

        forceXSpinSetValue = true;
        mXScaleSpin->setRange(1, 5);
        mXScaleSpin->setSingleStep(1.);
        mXScaleSpin->setDecimals(0);

    }
   

    /* ------------------------------------------
     *  Restore last zoom values; must be stored in unformated value
     * ------------------------------------------*/
    QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph> situ (mCurrentVariable, mCurrentTypeGraph);

    if (mZooms.find(situ) != mZooms.end()) {
       if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && (mCurrentVariable == GraphViewResults::eTheta
                                                                || mCurrentVariable == GraphViewResults::eTempo
                                                                || mCurrentVariable == GraphViewResults::eActivity)) {
            std::pair<double, double> currentMinMax = std::minmax(DateUtils::convertToAppSettingsFormat(mZooms.value(situ).first),
                                             DateUtils::convertToAppSettingsFormat(mZooms.value(situ).second));

            mResultCurrentMinX = currentMinMax.first;
            mResultCurrentMaxX = currentMinMax.second;

        } else {
            mResultCurrentMinX = mZooms.value(situ).first;
            mResultCurrentMaxX = mZooms.value(situ).second;
        }

    } else {
        mResultCurrentMinX = mResultMinX;
        mResultCurrentMaxX = mResultMaxX;
    }    

    if (mScales.find(situ) != mScales.end()) {
        mMajorScale = mScales.value(situ).first;
        mMinorCountScale = mScales.value(situ).second;

    } else {
        if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
            mMajorScale = 10.;
            mMinorCountScale = 10;
        } else {
            Scale xScale;
            xScale.findOptimal(mResultCurrentMinX, mResultCurrentMaxX, 10);
            mMajorScale = xScale.mark;
            mMinorCountScale = 10;
        }
    }

    mResultZoomX = (mResultMaxX - mResultMinX)/(mResultCurrentMaxX - mResultCurrentMinX);

    /* ------------------------------------------
     *  Set Ruler Current Position
     * ------------------------------------------*/


    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    mRuler->setScaleDivision(mMajorScale, mMinorCountScale);
    /* ------------------------------------------
     *  Set Ruler Areas (Burn, Adapt, Run)
     * ------------------------------------------*/
    mRuler->clearAreas();

    if (tabIdx == 1 || tabIdx == 2) {
        for (int i=0; i<mChainRadios.size(); ++i) {
            if (mChainRadios.at(i)->isChecked()){
                const ChainSpecs& chain = mChains.at(i);
                unsigned long adaptSize = chain.mBatchIndex * chain.mNumBatchIter;
                unsigned long runSize = chain.mNumRunIter / chain.mThinningInterval;

                mRuler->addArea(0., chain.mNumBurnIter, QColor(235, 115, 100));
                mRuler->addArea(chain.mNumBurnIter, chain.mNumBurnIter + adaptSize, QColor(250, 180, 90));
                mRuler->addArea(chain.mNumBurnIter + adaptSize, chain.mNumBurnIter + adaptSize + runSize, QColor(130, 205, 110));

                break;
            }
        }
    }

    /* -----------------------------------------------
     *  Set All Graphs Ranges (This is not done by generateCurves !)
     * -----------------------------------------------*/

    if (mTabByScene->currentIndex() == 0) {
        for (GraphViewResults* allGraph : mByEventsGraphs) {
            allGraph->setRange(mRuler->mMin, mRuler->mMax);
            allGraph->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
            allGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
            allGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
        }

    } else if (mTabByScene->currentIndex() == 1) {
        for (GraphViewResults* allGraph : mByPhasesGraphs) {
            allGraph->setRange(mRuler->mMin, mRuler->mMax);
            allGraph->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
            allGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
            allGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
        }

    } else if (mTabByScene->currentIndex() == 2) {
        for (GraphViewResults* allGraph : mByTempoGraphs) {
            allGraph->setRange(mRuler->mMin, mRuler->mMax);
            allGraph->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
            allGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
            allGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
        }

    }

    /* ------------------------------------------
     *  Set Zoom Slider & Zoom Edit
     * ------------------------------------------*/

    forceXSlideSetValue = true;
    setXScaleSlide( zoomToSlider(mResultZoomX));

    forceXSpinSetValue = true;
    setXScaleSpin(mResultZoomX);

    updateZoomEdit();
    updateScaleEdit();
    
    // Already done when setting graphs new range (above)
    //updateGraphsZoomX();

    qDebug()<< "ResultsView::updateScales emit scalesUpdated()";
    emit scalesUpdated();
}

void ResultsView::settingChange()
{
    if (mModel) {
        updateResults();
        updateResultsLog();
    }
}

void ResultsView::updateResultsLog()
{
    QString log;
    try {
        for (auto &&phase : mModel->mPhases)
            log += ModelUtilities::phaseResultsHTML(phase);

        for (auto &&phase : mModel->mPhases)
            log += ModelUtilities::tempoResultsHTML(phase);

        for (auto &&phaseConstraint : mModel->mPhaseConstraints) {
            log += ModelUtilities::constraintResultsHTML(phaseConstraint);

         for (auto &&event : mModel->mEvents)
              log += ModelUtilities::eventResultsHTML(event, true, mModel);

          log += "<hr>";
        }


    } catch (std::exception const & e) {
        qDebug()<< "in ResultsView::updateResultsLog() Error"<<e.what();
        log = tr("Impossible to compute");
    }
qDebug()<< "ResultsView::updateResultsLog()-> emit resultsLogUpdated(log)";
    emit resultsLogUpdated(log);
}

void ResultsView::mouseMoveEvent(QMouseEvent* e)
{
    int shiftX (0);
    
    int x = e->pos().x() - shiftX;
    x = (x <= width() - mOptionsW) ? x : width() - mOptionsW;
    mMarker->setGeometry(x, mMarker->pos().y(), mMarker->width(), mMarker->height());
}


/**
 * @brief ResultsView::XScaleSliderChanged
 * @param value
 * connected to mXSlider, &QSlider::valueChanged(value)
 */
void ResultsView::XScaleSliderChanged(int value)
{
    if (!forceXSlideSetValue) {
        forceXSpinSetValue = true;
        setXScaleSpin(sliderToZoom(value));
        updateZoomX();
    }
}

/**
 * @brief ResultsView::XScaleSpinChanged slot connected to mXScaleSpin->setValue()
 * @param value
 */
void ResultsView::XScaleSpinChanged(double value)
{
    if (!forceXSpinSetValue) {
        forceXSlideSetValue = true;
        setXScaleSlide(zoomToSlider(value));
        updateZoomX();
    }
}

/**
 * @brief ResultsView::setXScaleSpin
 * Slot to control, if we want to force just set the value, or if we want to propagate the signal throw xSinUpdate
 * @param value
 */
void ResultsView::setXScaleSpin(const double value)
{
    mXScaleSpin->setValue(value);
    if (!forceXSpinSetValue)
        emit xSpinUpdate(value);
    else
        forceXSpinSetValue = false;
}

/**
 * @brief ResultsView::setXScaleSlide
 * Slot to control, if we want to force the just set the value, or if we want to propagate the signal throw xSlideUpdate
 * @param value
 */
void ResultsView::setXScaleSlide(const int value)
{
    mXSlider->setValue(value);
    if (!forceXSlideSetValue)
        emit xSlideUpdate(value);
    else
        forceXSlideSetValue = false;
}

double ResultsView::sliderToZoom(const int &coef)
{
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib)
         return pow(10., (double)coef/100.);
    else
        return coef;
}

int ResultsView::zoomToSlider(const double &zoom)
{
   if (mCurrentTypeGraph == GraphViewResults::ePostDistrib)
        return (int)round(log10(zoom) * (100.));
   else
       return zoom;
}

void ResultsView::updateScaleX()
{
    QString str = mMajorScaleEdit->text();
    bool isNumber(true);
    double aNumber = locale().toDouble(&str, &isNumber);

    if (!isNumber || aNumber<1)
        return;

    mMajorScale = aNumber;

    str = mMinorScaleEdit->text();
    aNumber = locale().toDouble(&str, &isNumber);

    if (isNumber && aNumber>=1) {
        mMinorCountScale =  aNumber;

        mRuler->setScaleDivision(mMajorScale, mMinorCountScale);
        QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph> situ (mCurrentVariable, mCurrentTypeGraph);

        mScales[situ] = QPair<double, int>(mMajorScale, mMinorCountScale);

        if (mTabByScene->currentIndex() == 0) {
            for (GraphViewResults* eventGraph : mByEventsGraphs)
                if (eventGraph)
                    eventGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);

        } else if (mTabByScene->currentIndex() == 1) {
            for (GraphViewResults* phaseGraph : mByPhasesGraphs)
                if (phaseGraph)
                    phaseGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);

        } else if (mTabByScene->currentIndex() == 2) {
            for (GraphViewResults* tempoGraph : mByTempoGraphs)
                if (tempoGraph)
                    tempoGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
       }
    } else
        return;

}

/**
 * @brief ResultsView::updateZoomX
 * Happen when we move the mXSlider
 */
void ResultsView::updateZoomX()
{
    /* --------------------------------------------------
     *  Find new current min & max, update mResultZoomX
     * --------------------------------------------------*/
    double zoom = mXScaleSpin->value();

    mResultZoomX = 1./zoom;

    const double tCenter = (mResultCurrentMaxX + mResultCurrentMinX)/2.;
    const double span = (mResultMaxX - mResultMinX)* (1./ zoom);

    double curMin = tCenter - span/2.;
    double curMax = tCenter + span/2.;

    if (curMin < mRuler->mMin) {
        curMin = mRuler->mMin;
        curMax = curMin + span;

    } else if (curMax> mRuler->mMax) {
        curMax = mRuler->mMax;
        curMin = curMax - span;
    }

    mResultCurrentMinX = curMin;
    mResultCurrentMaxX = curMax;
    
    /* --------------------------------------------------
     *  Update other elements
     * --------------------------------------------------*/
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);

    updateZoomEdit();
    updateGraphsZoomX();
}

/**
 * @brief ResultsView::updateScroll signal from the Ruler
 * @param min
 * @param max
 */
void ResultsView::updateScroll(const double min, const double max)
{
    /* --------------------------------------------------
     *  Find new current min & max
     * --------------------------------------------------*/
    mResultCurrentMinX = min;
    mResultCurrentMaxX = max;
    
    /* --------------------------------------------------
     *  Update other elements
     * --------------------------------------------------*/
    updateZoomEdit();
    updateGraphsZoomX();
}

void ResultsView::editCurrentMinX()
{
    /* --------------------------------------------------
     *  Find new current min & max (check range validity !!!)
     *  Update mResultZoomX
     * --------------------------------------------------*/
    //QLocale locale = QLocale();
    QString str = mCurrentXMinEdit->text();
    bool isNumber(true);
    double value =  locale().toDouble(&str, &isNumber);

    if (isNumber && value != mResultCurrentMinX) {
        const double minVisible = mRuler->mMin ;
        const double current = qBound(minVisible, value, mResultCurrentMaxX);
        mResultCurrentMinX = current;

        mResultZoomX = (mResultMaxX - mResultMinX)/ (mResultCurrentMaxX - mResultCurrentMinX);
        
        /* --------------------------------------------------
         *  Update other elements
         * --------------------------------------------------*/

        forceXSlideSetValue = true;
        setXScaleSlide(zoomToSlider(mResultZoomX));

        forceXSpinSetValue = true;
        setXScaleSpin(mResultZoomX);

        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);

        updateZoomEdit();
        updateGraphsZoomX();
    }
}

void ResultsView::editCurrentMaxX()
{
    /* --------------------------------------------------
     *  Find new current min & max (check range validity !!!)
     *  Update mResultZoomX
     * --------------------------------------------------*/
    QString str = mCurrentXMaxEdit->text();
    bool isNumber(true);
    double value =  locale().toDouble(&str, &isNumber);

    if (isNumber && value != mResultCurrentMaxX) {
        const double maxVisible = mRuler->mMax;
        double current = qBound(mResultCurrentMinX, value, maxVisible);

        mResultCurrentMaxX = current;

        mResultZoomX =  (mResultMaxX - mResultMinX)/(mResultCurrentMaxX - mResultCurrentMinX) ;
        
        /* --------------------------------------------------
         *  Update other elements
         * --------------------------------------------------*/

        forceXSlideSetValue = true;
        setXScaleSlide(zoomToSlider(mResultZoomX));// the signal valueChange() must be not connected with a slot

        forceXSpinSetValue = true;
        setXScaleSpin(mResultZoomX);

        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);

        updateZoomEdit();

        updateGraphsZoomX();
    }
}

void ResultsView:: setStudyPeriod()
{
    qDebug()<<"ResultsView::setStudyPeriod()";
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && (mCurrentVariable == GraphViewResults::eTheta
                                                                || mCurrentVariable == GraphViewResults::eTempo
                                                                || mCurrentVariable == GraphViewResults::eActivity)) {
        mResultCurrentMinX = mSettings.getTminFormated();
        mResultCurrentMaxX = mSettings.getTmaxFormated();
        mResultZoomX = (mResultMaxX - mResultMinX)/(mResultCurrentMaxX - mResultCurrentMinX);
     }

    else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && mCurrentVariable == GraphViewResults::eSigma) {
        mResultCurrentMinX = 0.;
        mResultCurrentMaxX = mResultMaxVariance;
        mResultZoomX = (mResultMaxX - mResultMinX)/(mResultCurrentMaxX - mResultCurrentMinX);
     }

    else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && mCurrentVariable == GraphViewResults::eDuration) {
        mResultCurrentMinX = 0.;
        mResultCurrentMaxX = mResultMaxDuration;
        mResultZoomX = (mResultMaxX - mResultMinX)/(mResultCurrentMaxX - mResultCurrentMinX);
     }

    else
        return;

    Scale Xscale;
    Xscale.findOptimal(mResultCurrentMinX, mResultCurrentMaxX, 10);
    mMajorScale = Xscale.mark;
    mMinorCountScale = Xscale.tip;
    updateScaleEdit();

    forceXSlideSetValue = true;
    setXScaleSlide(zoomToSlider(mResultZoomX));

    forceXSpinSetValue = true;
    setXScaleSpin(mResultZoomX);

    mRuler->setScaleDivision(Xscale);
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    updateZoomEdit();
    updateGraphsZoomX();
}

void ResultsView::updateZoomEdit()
{
    QLocale locale = QLocale();
    QFont adaptedFont (AppSettings::font());
    QFontMetricsF fm (AppSettings::font());
    qreal textSize = fm.width(locale.toString(mResultCurrentMinX,'f',0));
    if (textSize > (mCurrentXMinEdit->width() - 2. )) {
        const qreal fontRate = textSize / (mCurrentXMinEdit->width() - 2. );
        const qreal ptSiz = adaptedFont.pointSizeF() / fontRate;
        adaptedFont.setPointSizeF(ptSiz);
        mCurrentXMinEdit->setFont(adaptedFont);
    }
    else
        mCurrentXMinEdit->setFont(AppSettings::font());

    mCurrentXMinEdit->setText(locale.toString(mResultCurrentMinX,'f',0));

    textSize = fm.width(locale.toString(mResultCurrentMaxX,'f',0));
    if (textSize > (mCurrentXMaxEdit->width() - 2. )) {
        const qreal fontRate = textSize / (mCurrentXMaxEdit->width() - 2. );
        const qreal ptSiz = adaptedFont.pointSizeF() / fontRate;
        adaptedFont.setPointSizeF(ptSiz);
        mCurrentXMaxEdit->setFont(adaptedFont);
    }
    else
        mCurrentXMaxEdit->setFont(AppSettings::font());

    mCurrentXMaxEdit->setText(locale.toString(mResultCurrentMaxX,'f',0));

}

void ResultsView::updateScaleEdit()
{
     QLocale locale = QLocale();
    mMinorScaleEdit->setText(locale.toString(mMinorCountScale));
    mMajorScaleEdit->setText(locale.toString(mMajorScale));
}

/**
 * @brief ResultsView::updateGraphsZoomX
 * Update graphs display according to the current values and store them in mZooms[] and mScales[]
 * used in updateScales()
 */
void ResultsView::updateGraphsZoomX()
{
   // qDebug()<<"ResultsView::updateGraphsZoomX()";

    /* --------------------------------------------------
     *   Resize the ruler and the marges inside the graphs
     * --------------------------------------------------*/
  //  setGraphFont(mGraphFont);

    /* --------------------------------------------------
     *  Redraw the graphs
     * --------------------------------------------------*/

    if (mTabByScene->currentIndex() == 0) {
        for (GraphViewResults* eventGraph : mByEventsGraphs)
            if (eventGraph) {
                eventGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                eventGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
            }
    }
    else if (mTabByScene->currentIndex() == 1) {
        for (GraphViewResults* phaseGraph : mByPhasesGraphs)
            if (phaseGraph) {
                phaseGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                phaseGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
            }
   }
    else if (mTabByScene->currentIndex() == 2) {
        for (GraphViewResults* tempoGraph : mByTempoGraphs)
            if (tempoGraph) {
                tempoGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                tempoGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
        }
   }

    /* --------------------------------------------------
     * Store zoom values in an unformated value AD/BD for Post. distrib tab
     * --------------------------------------------------*/

    QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph> situ (mCurrentVariable, mCurrentTypeGraph);

    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && (mCurrentVariable == GraphViewResults::eTheta
                                                                || mCurrentVariable == GraphViewResults::eTempo
                                                                || mCurrentVariable == GraphViewResults::eActivity)) {

        std::pair<double, double> resultMinMax = std::minmax( DateUtils::convertFromAppSettingsFormat(mResultCurrentMinX),
                                     DateUtils::convertFromAppSettingsFormat(mResultCurrentMaxX));

        mZooms[situ] = QPair<double, double>(resultMinMax.first, resultMinMax.second);


    } else
        mZooms[situ] = QPair<double, double>(mResultCurrentMinX, mResultCurrentMaxX);

    mScales[situ] = QPair<double, int>(mMajorScale, mMinorCountScale);

}

/**
 * @brief ResultsView::updateScaleY
 * update all graph, the maximum is controle by the range of mYSlider
 * @param value
 */
void ResultsView::updateScaleY(int value)
{
    //GraphViewResults::mHeightForVisibleAxis(7 * AppSettings::heigthUnit()),

    mYScaleSpin->setValue(value);
    const double min (3 * AppSettings::heigthUnit());//(70.);
    const double origin (10 * AppSettings::heigthUnit());// (150.); Same value in ResultsView::applyAppSettings()
    const double prop = (double)value / 100.;
    mGraphHeight = min + prop * (origin - min);
    
    updateGraphsLayout();
}


// Display options
void ResultsView::setGraphFont(const QFont &font)
{
    mGraphFont = font;
    const QFontMetrics gfm (mGraphFont);

    /* Variable identic in AxisTool */

#ifdef Q_OS_MAC
    int heightText = 1.5 * gfm.height();
#else
    int heightText = gfm.height();
#endif
    qreal graduationSize = heightText /3;

    switch(mTabs->currentIndex()) {
        case 0: // Posterior Distrib.
                {
                  //  const int marg = qMax(gfm.width(stringForLocal(mResultCurrentMinX))/ 2., gfm.width(stringForLocal(mResultCurrentMaxX))/ 2.);
                    mMarginLeft =  1.5 * gfm.width(stringForLocal( 5 * mSettings.getTminFormated()))/ 2. +  2*graduationSize;
                    mMarginRight = 1.5 * gfm.width(stringForLocal(5 * mSettings.getTmaxFormated()))/ 2.;
                  }
         break;

        case 1: // History Plot
        case 2:// Acceptance Rate
                 {
                        const int maxIter = 1+ mChains.at(0).mNumBurnIter + (mChains.at(0).mBatchIndex * mChains.at(0).mNumBatchIter) + mChains.at(0).mNumRunIter / mChains.at(0).mThinningInterval;
                        const int marg = gfm.width(stringForLocal(maxIter));
                        mMarginLeft =  marg + 2*graduationSize ;
                        mMarginRight = marg;// gfm.width(stringForLocal(mSettings:))/ 2;
                    }
        break;

         case 3:// Autocorrelation
                    {
                       const int marg = qMax(gfm.width(stringForLocal(40))/ 2, gfm.width(stringForGraph(100)));
                       mMarginLeft = marg + 2*graduationSize ;
                       mMarginRight = 4 * gfm.width(stringForLocal(40))/ 2.;
                }
        break;

        default:
                break;
      }

    mRuler->setMarginLeft(mMarginLeft);
    mRuler->setMarginRight(mMarginRight);

    mRuler->updateLayout();
    mRulerH = mRuler->height();

    for (GraphViewResults* phaseGraph : mByPhasesGraphs) {
        phaseGraph->setMarginLeft(mMarginLeft);
        phaseGraph->setMarginRight(mMarginRight);
        phaseGraph->setGraphFont(mGraphFont);
    }

    for (GraphViewResults* eventGraph : mByEventsGraphs) {
        eventGraph->setMarginLeft(mMarginLeft);
        eventGraph->setMarginRight(mMarginRight);
        eventGraph->setGraphFont(mGraphFont);
    }

    for (GraphViewResults* tempoGraph : mByTempoGraphs) {
        tempoGraph->setMarginLeft(mMarginLeft);
        tempoGraph->setMarginRight(mMarginRight);
        tempoGraph->setGraphFont(mGraphFont);
    }
}


/**
 * @brief ResultsView::updateFont only on graph
 */
void ResultsView::updateGraphFont()
{
    QFontDialog dialog;
    dialog.setParent(qApp->activeWindow());
    dialog.setFont(mGraphFont);

    bool ok;
    const QFont font = QFontDialog::getFont(&ok, mGraphFont, this);
    if (ok) {
        setGraphFont(font);
        mFontBut->setText(mGraphFont.family() + ", " + QString::number(mGraphFont.pointSizeF()));        
    }
}

void ResultsView::updateThickness(int value)
{
    for (GraphViewResults* allKindGraph : mByEventsGraphs)
        allKindGraph->setGraphsThickness(value);

    for (GraphViewResults* allKindGraph : mByPhasesGraphs)
        allKindGraph->setGraphsThickness(value);

    for (GraphViewResults* allKindGraph : mByTempoGraphs)
        allKindGraph->setGraphsThickness(value);

}

void ResultsView::updateOpacity(int value)
{
    const int opValue (value*10);
    for (GraphViewResults* allKindGraph : mByEventsGraphs)
        allKindGraph->setGraphsOpacity(opValue);

    for (GraphViewResults* allKindGraph : mByPhasesGraphs)
       allKindGraph->setGraphsOpacity(opValue);
    
    for (GraphViewResults* allKindGraph : mByTempoGraphs)
        allKindGraph->setGraphsOpacity(opValue);
    
}


void ResultsView::showInfos(bool show)
{
    mTempoStatCheck->setChecked(show);
    mStatCheck->setChecked(show);
    for (GraphViewResults* allKindGraph : mByEventsGraphs)
        allKindGraph->showNumericalResults(show);

    for (GraphViewResults* allKindGraph : mByPhasesGraphs)
        allKindGraph->showNumericalResults(show);

    for (GraphViewResults* allKindGraph : mByTempoGraphs)
        allKindGraph->showNumericalResults(show);

    updateLayout();
}

void ResultsView::saveAsImage()
{
    QList<GraphViewResults*> selectedGraph;

    if (mTabByScene->currentIndex() == 0) {
        if (mEventsScrollArea)
            for (auto && graph : mByEventsGraphs)
                if (graph->isSelected())
                   selectedGraph.append(graph);
    }

    else if (mTabByScene->currentIndex() == 1) {
        if (mPhasesScrollArea)
            for (auto && graph : mByPhasesGraphs)
                if (graph->isSelected())
                   selectedGraph.append(graph);
    }

    else if (mTabByScene->currentIndex() == 2) {
        if (mTempoScrollArea)
            for (auto && graph : mByTempoGraphs)
                if (graph->isSelected())
                   selectedGraph.append(graph);
    }


//----
    if (selectedGraph.isEmpty())
        return;
    // ---- Ask for file name and type (SVG or png)
    QString filter = QObject::tr("Image (*.png);;Photo (*.jpg);;Scalable Vector Graphics (*.svg)");
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save graph image as..."),
                                                    MainWindow::getInstance()->getCurrentPath(),
                                                    filter);

    if (!fileName.isEmpty()) {
        QFileInfo fileInfo;
        fileInfo = QFileInfo(fileName);
        QString fileExtension = fileInfo.suffix();
        bool asSvg = fileName.endsWith(".svg");
        // --- if png
        const int versionHeight (20);
        if (!asSvg) {

            const short pr = AppSettings::mPixelRatio;

            QImage image (selectedGraph.at(0)->width() * pr, (selectedGraph.size() * selectedGraph.at(0)->height() + versionHeight) * pr , QImage::Format_ARGB32_Premultiplied); //Format_ARGB32_Premultiplied //Format_ARGB32

            if (image.isNull() )
                qDebug()<< " image width = 0";

            image.setDevicePixelRatio(pr);
            image.fill(Qt::transparent);

            QPainter p;
            p.begin(&image);
            p.setRenderHint(QPainter::Antialiasing);

            QPoint ptStart (0, 0);
            for (auto &&graph : selectedGraph) {
                graph->showSelectedRect(false);
                //GraphView::Rendering memoRendering= graph->getRendering();
                //graph->setRendering(GraphView::eHD);
                graph->render(&p, ptStart, QRegion(0, 0, graph->width(), graph->height()));
                ptStart = QPoint(0, ptStart.y() + graph->height());
                graph->showSelectedRect(true);
               // graph->setRendering(memoRendering);
            }
            p.setPen(Qt::black);
            p.setBrush(Qt::white);
            p.fillRect(0, ptStart.y(), selectedGraph.at(0)->width(), versionHeight, Qt::white);
            p.drawText(0, ptStart.y(), selectedGraph.at(0)->width(), versionHeight,
                       Qt::AlignCenter,
                       qApp->applicationName() + " " + qApp->applicationVersion());
            p.end();

            if (fileExtension=="png") {
                image.save(fileName, "png");
            }
            else if (fileExtension == "jpg") {
                int imageQuality = AppSettings::mImageQuality;
                image.save(fileName, "jpg",imageQuality);
            }
            else if (fileExtension == "bmp") {
                image.save(fileName, "bmp");
            }

        } // not svg
        // if svg type
        else {
            //Rendering memoRendering= mRendering;
            QRect rTotal( QRect(0, 0, width(), height() + versionHeight) );
            // Set SVG Generator
            QSvgGenerator svgGen;
            svgGen.setFileName(fileName);
            svgGen.setSize(rTotal.size());
            svgGen.setViewBox(rTotal);
            svgGen.setTitle(fileName);
            svgGen.setDescription(fileName);

            QPainter painter;
            painter.begin(&svgGen);
            font().wordSpacing();

            QPoint ptStart (0, versionHeight);
            for (auto &&graph : selectedGraph) {
                graph->showSelectedRect(false);
                 /* We can not have a svg graph in eSD Rendering Mode */
                //GraphView::Rendering memoRendering= graph->getRendering();
                //graph->setRendering(GraphView::eHD);
                graph->render(&painter, ptStart, QRegion(0, 0, graph->width(), graph->height()));
                ptStart = QPoint(0, ptStart.y() + graph->height());
                graph->showSelectedRect(true);
                //graph->setRendering(memoRendering);
            }
            painter.setPen(Qt::black);
            painter.drawText(0, ptStart.y(), selectedGraph.at(0)->width(), versionHeight,
                             Qt::AlignCenter,
                             qApp->applicationName() + " " + qApp->applicationVersion());

            painter.end();

        }
    // end if not Empty filename
    }
}

void ResultsView::imageToClipboard()
{
    QList<GraphViewResults*> selectedGraph;

    if (mTabByScene->currentIndex() == 0) {
        if (mEventsScrollArea)
            for (auto &&graph : mByEventsGraphs)
                if (graph->isSelected())
                   selectedGraph.append(graph);
    }

    else if (mTabByScene->currentIndex() == 1) {
        if (mPhasesScrollArea)
            for (auto &&graph : mByPhasesGraphs)
                if (graph->isSelected())
                   selectedGraph.append(graph);
    }

    else if (mTabByScene->currentIndex() == 2) {
        if (mTempoScrollArea)
            for (auto &&graph : mByTempoGraphs)
                if (graph->isSelected())
                   selectedGraph.append(graph);
    }


    QClipboard* clipboard = QApplication::clipboard();

    if (selectedGraph.isEmpty())
        return;
    const int versionHeight (20);
    short pr = AppSettings::mPixelRatio;

    QImage image (selectedGraph.at(0)->width() * pr, (selectedGraph.size() * selectedGraph.at(0)->height() + versionHeight) * pr , QImage::Format_ARGB32_Premultiplied); //Format_ARGB32_Premultiplied //Format_ARGB32

    if (image.isNull() )
        qDebug()<< " image width = 0";

    image.setDevicePixelRatio(pr);
    image.fill(Qt::transparent);

    QPainter p;
    p.begin(&image);
    p.setRenderHint(QPainter::Antialiasing);

    QPoint ptStart (0, 0);
    for (auto &&graph : selectedGraph) {
        graph->showSelectedRect(false);
        //GraphView::Rendering memoRendering= graph->getRendering();
       // graph->setRendering(GraphView::eHD);
        graph->render(&p, ptStart, QRegion(0, 0, graph->width(), graph->height()));
        ptStart = QPoint(0, ptStart.y() + graph->height());
        graph->showSelectedRect(true);
       // graph->setRendering(memoRendering);
    }
    p.setPen(Qt::black);
    p.setBrush(Qt::white);
    p.fillRect(0, ptStart.y(), selectedGraph.at(0)->width(), versionHeight, Qt::white);
    p.drawText(0, ptStart.y(), selectedGraph.at(0)->width(), versionHeight,
               Qt::AlignCenter,
               qApp->applicationName() + " " + qApp->applicationVersion());


    p.end();

    clipboard->setImage(image);

}

void ResultsView::resultsToClipboard()
{
    QString resultText;

    if (mTabByScene->currentIndex() == 0) {
        if (mEventsScrollArea) {
                    for (auto && graph : mByEventsGraphs)
                        if (graph->isSelected())
                            resultText += graph->getTextAreaToPlainText();
            }
    }
   else if (mTabByScene->currentIndex() == 1) {
        if (mPhasesScrollArea)
            for (auto && graph : mByPhasesGraphs)
                if (graph->isSelected())
                   resultText += graph->getTextAreaToPlainText();
    }
    else if (mTabByScene->currentIndex() == 2) {
         if (mTempoScrollArea)
             for (auto && graph : mByTempoGraphs)
                 if (graph->isSelected())
                    resultText += graph->getTextAreaToPlainText();
     }

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(resultText);
}
void ResultsView::saveGraphData()
{
    QList<GraphViewResults*> selectedGraph;

    if (mTabByScene->currentIndex() == 0) {
        if (mEventsScrollArea) {
            for (auto &&graph : mByEventsGraphs)
                if (graph->isSelected())
                    selectedGraph.append(graph);
        }

    } else if (mTabByScene->currentIndex() == 1) {
        if (mPhasesScrollArea)
            for (auto &&graph : mByPhasesGraphs)
                if (graph->isSelected())
                   selectedGraph.append(graph);

    } else if (mTabByScene->currentIndex() == 2) {
        if (mTempoScrollArea)
            for (auto &&graph : mByTempoGraphs)
                if (graph->isSelected())
                   selectedGraph.append(graph);
    }

    for (auto &&graph : selectedGraph)
        graph->saveGraphData();
}



/**
 * @brief ResultsView::exportResults export result into several files
 *
 */
void ResultsView::exportResults()
{
    if (mModel) {

        const QString csvSep = AppSettings::mCSVCellSeparator;
        const int precision = AppSettings::mPrecision;
        QLocale csvLocal = AppSettings::mCSVDecSeparator == "." ? QLocale::English : QLocale::French;

        csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);
        
        const QString currentPath = MainWindow::getInstance()->getCurrentPath();
        const QString dirPath = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                        tr("Export to directory..."),
                                                        currentPath,
                                                       tr("Directory"));
        
        if (!dirPath.isEmpty()) {
            QDir dir(dirPath);
            if (dir.exists()){
                /*if(QMessageBox::question(qApp->activeWindow(), tr("Are you sure?"), tr("This directory already exists and all its content will be deleted. Do you really want to replace it?")) == QMessageBox::No){
                    return;
                }*/
                dir.removeRecursively();
            }
            dir.mkpath(".");

            // copy tabs ------------------------------------------
            const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
            const QString projectName = tr("Project filename : %1").arg(MainWindow::getInstance()->getNameProject()) + "<br>";

            QFile file(dirPath + "/Log_Model_Description.html");
            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<version+"<br>";
                output<<projectName+ "<br>";
                output<<"<hr>";
                output<<mModel->getModelLog();
            }
            file.close();

            file.setFileName(dirPath + "/Log_MCMC_Initialization.html");
            
            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<version+"<br>";
                output<<projectName+ "<br>";
                output<<"<hr>";
                output<<mModel->getMCMCLog();
            }
            file.close();

            file.setFileName(dirPath + "/Log_Posterior_Distrib_Stats.html");
            
            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<version+"<br>";
                output<<projectName+ "<br>";
                output<<"<hr>";
                output<<mModel->getResultsLog();
            }
            file.close();

            const QList<QStringList> stats = mModel->getStats(csvLocal, precision, true);
            saveCsvTo(stats, dirPath + "/Synthetic_Stats_Table.csv", csvSep, true);
            
            if (mModel->mPhases.size() > 0) {
                const QList<QStringList> phasesTraces = mModel->getPhasesTraces(csvLocal, false);
                saveCsvTo(phasesTraces, dirPath + "/Chain_all_Phases.csv", csvSep, false);
                
                for (int i=0; i<mModel->mPhases.size(); ++i) {
                    const QList<QStringList> phaseTrace = mModel->getPhaseTrace(i,csvLocal, false);
                    const QString name = mModel->mPhases.at(i)->mName.toLower().simplified().replace(" ", "_");
                    saveCsvTo(phaseTrace, dirPath + "/Chain_Phase_" + name + ".csv", csvSep, false);
                }
            }
            QList<QStringList> eventsTraces = mModel->getEventsTraces(csvLocal, false);
            saveCsvTo(eventsTraces, dirPath + "/Chain_all_Events.csv", csvSep, false);
        }
    }
}

void ResultsView::exportFullImage()
{
    //  define ScrollArea
    enum ScrollArrea{
        eScrollPhases = 0,
        eScrollEvents = 1,
        eScrollTempo = 2
    };
    
    //ScrollArrea witchScroll;
    bool printAxis = true;
    
    QWidget* curWid (nullptr);
    
    type_data max;

    if (mStack->currentWidget() == mEventsScrollArea) {
        curWid = mEventsScrollArea->widget();
        curWid->setFont(mByEventsGraphs.at(0)->font());
        max = mByEventsGraphs.at(0)->getGraph()->maximumX();
    }

    else if (mStack->currentWidget() == mPhasesScrollArea) {
        curWid = mPhasesScrollArea->widget();
        curWid->setFont(mByPhasesGraphs.at(0)->font());
        max = mByPhasesGraphs.at(0)->getGraph()->maximumX();
    }

    else if (mStack->currentWidget() == mTempoScrollArea) {
       curWid = mTempoScrollArea->widget();
       curWid->setFont(mByTempoGraphs.at(0)->font());
       max = mByTempoGraphs.at(0)->getGraph()->maximumX();
    }
    else
        return;

    // --------------------------------------------------------------------
    // Force rendering to HD for export
    //int rendering = mRenderCombo->currentIndex();
  //  updateRendering(1);
    
    AxisWidget* axisWidget = nullptr;
    QLabel* axisLegend = nullptr;
    int axeHeight (mGraphFont.pointSize() * 2.2); // equal MarginBottom()
    int legendHeight (2* AppSettings::font().pointSizeF());// 20);
    
    if (printAxis) {
        curWid->setFixedHeight(curWid->height() + axeHeight + legendHeight );

        axisWidget = new AxisWidget(nullptr, curWid);
        axisWidget->mMarginLeft = mMarginLeft;
        axisWidget->mMarginRight = mMarginRight;
        axisWidget->setScaleDivision(mMajorScale, mMinorCountScale);

        if (mStatCheck->isChecked()) {
            axisWidget->setGeometry(0, curWid->height() - axeHeight, curWid->width()*2./3., axeHeight);
            axisWidget->updateValues(curWid->width()*2./3. - axisWidget->mMarginLeft - axisWidget->mMarginRight, 50, mResultCurrentMinX, mResultCurrentMaxX);

        } else {
            axisWidget->setGeometry(0, curWid->height() - axeHeight, curWid->width(), axeHeight);
            axisWidget->updateValues(curWid->width() - axisWidget->mMarginLeft - axisWidget->mMarginRight, 50, mResultCurrentMinX, mResultCurrentMaxX);
        }

        axisWidget->mShowText = true;
        axisWidget->setAutoFillBackground(true);
        axisWidget->mShowSubs = true;
        axisWidget->mShowSubSubs = true;
        axisWidget->mShowArrow = true;
        axisWidget->mShowText = true;

        axisWidget->raise();
        axisWidget->setVisible(true);
        
        QString legend = "";
        if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && ( mCurrentVariable == GraphViewResults::eTheta
                                                                     || mCurrentVariable == GraphViewResults::eTempo
                                                                     || mCurrentVariable == GraphViewResults::eActivity) )
            legend = DateUtils::getAppSettingsFormatStr();

        else if (mCurrentTypeGraph == GraphViewResults::eTrace || mCurrentTypeGraph == GraphViewResults::eAccept)
            legend = "Iterations";

        else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && mCurrentVariable == GraphViewResults::eDuration)
            legend = "Years";


        axisLegend = new QLabel(legend, curWid);
        axisLegend->setFont(AppSettings::font());
        QFontMetrics fm(AppSettings::font());
        if (mStatCheck->isChecked())
            axisLegend->setGeometry(fm.width(legend), curWid->height() - axeHeight - legendHeight, curWid->width()*2./3. - 10, legendHeight);
        else
            axisLegend->setGeometry(curWid->width() - fm.width(legend) - mMarginRight, curWid->height() - axeHeight - legendHeight, fm.width(legend) , legendHeight);

        axisLegend->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        axisLegend->raise();
        axisLegend->setVisible(true);
    }
    
    QFileInfo fileInfo = saveWidgetAsImage(curWid,
                                           QRect(0, 0, curWid->width() , curWid->height()),
                                           tr("Save graph image as..."),
                                           MainWindow::getInstance()->getCurrentPath());
    
    // Delete additional widgets if necessary :
    if (printAxis) {
        if (axisWidget) {
            axisWidget->setParent(nullptr);
            delete axisWidget;
        }
        if (axisLegend) {
            axisLegend->setParent(nullptr);
            delete axisLegend;
        }
        curWid->setFixedHeight(curWid->height() - axeHeight - legendHeight);
    }
    
    
    // Revert to default display :
    
    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
    
    // Reset rendering back to its current value
   // updateRendering(rendering);
    
}

/**
 * @brief ResultsView::updateModel Update Design
 */
void ResultsView::updateModel()
{
    if (!mModel)
        return;
    
    const QJsonObject state = MainWindow::getInstance()->getProject()->state();
    
    const QJsonArray events = state.value(STATE_EVENTS).toArray();
    const QJsonArray phases = state.value(STATE_PHASES).toArray();
    
    QJsonArray::const_iterator iterJSONEvent = events.constBegin();
    while (iterJSONEvent != events.constEnd()) {
        const QJsonObject eventJSON = (*iterJSONEvent).toObject();
        const int eventId = eventJSON.value(STATE_ID).toInt();
        const QJsonArray dates = eventJSON.value(STATE_EVENT_DATES).toArray();
        
        QList<Event *>::iterator iterEvent = mModel->mEvents.begin();
        /*
         * (*iterEvent)->mIsSelected is already Ok--> false
         */
        while (iterEvent != mModel->mEvents.cend()) {

            if ((*iterEvent)->mId == eventId) {
                (*iterEvent)->mName  = eventJSON.value(STATE_NAME).toString();
                (*iterEvent)->mItemX = eventJSON.value(STATE_ITEM_X).toDouble();
                (*iterEvent)->mItemY = eventJSON.value(STATE_ITEM_Y).toDouble();
                (*iterEvent)->mColor = QColor(eventJSON.value(STATE_COLOR_RED).toInt(),
                                              eventJSON.value(STATE_COLOR_GREEN).toInt(),
                                              eventJSON.value(STATE_COLOR_BLUE).toInt());
                (*iterEvent)->mIsSelected = eventJSON.value(STATE_IS_SELECTED).toBool();

                for (int k=0; k<(*iterEvent)->mDates.size(); ++k) {
                    Date& d = (*iterEvent)->mDates[k];
                    
                    for (auto &&dateVal : dates) {

                        const QJsonObject date = dateVal.toObject();
                        const int dateId = date.value(STATE_ID).toInt();
                        
                        if (dateId == d.mId) {
                            d.mName = date.value(STATE_NAME).toString();
                            d.mColor = (*iterEvent)->mColor;
                            break;
                        }
                    }
                }
                break;
            }
            ++iterEvent;
        }
        ++iterJSONEvent;
    }

    QJsonArray::const_iterator iterJSONPhase = phases.constBegin();
    while (iterJSONPhase != phases.constEnd()) {

        const QJsonObject phaseJSON = (*iterJSONPhase).toObject();
        const int phaseId = phaseJSON.value(STATE_ID).toInt();
        
        for ( auto &&p : mModel->mPhases ) {
            if (p->mId == phaseId) {
                p->mName = phaseJSON.value(STATE_NAME).toString();
                p->mItemX = phaseJSON.value(STATE_ITEM_X).toDouble();
                p->mItemY = phaseJSON.value(STATE_ITEM_Y).toDouble();
                p->mColor = QColor(phaseJSON.value(STATE_COLOR_RED).toInt(),
                                   phaseJSON.value(STATE_COLOR_GREEN).toInt(),
                                   phaseJSON.value(STATE_COLOR_BLUE).toInt());
                p->mIsSelected = phaseJSON.value(STATE_IS_SELECTED).toBool();
                break;
            }
        }
        ++iterJSONPhase;
    }
    
    std::sort(mModel->mEvents.begin(), mModel->mEvents.end(), sortEvents);
    std::sort(mModel->mPhases.begin(), mModel->mPhases.end(), sortPhases);
    
    updateResults(mModel);
}

