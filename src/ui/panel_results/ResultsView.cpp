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

const qreal titleHeight (20);
const qreal labelHeight (20);
const qreal lineEditHeight (15);
const qreal checkBoxHeight (17);
const qreal comboBoxHeight (20);
const qreal radioButtonHeight (17);
const qreal spinBoxHeight (22);
const qreal buttonHeight (20);


ResultsView::ResultsView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mResultMaxVariance(1000.),
mHasPhases(false),
mModel(nullptr),
mMargin(5),
mOptionsW(200),
mRulerH(40),
mTabsH(30),
mGraphsH(150),
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

    QFont ft = QFont();
    ft.setPointSize(APP_SETTINGS_DEFAULT_FONT_SIZE);
    const QFontMetricsF fm(QFont(APP_SETTINGS_DEFAULT_FONT_FAMILY, APP_SETTINGS_DEFAULT_FONT_SIZE));
   // mLineH = 20;//boxHeight(fm);

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
    mRuler->mMarginLeft = 50.;
    mRuler->mMarginRight = 10.;

    mRuler->mMax = mSettings.mTmax;
    mRuler->mMin = mSettings.mTmin;
    mRuler->mCurrentMax = mSettings.mTmax;
    mRuler->mCurrentMin = mSettings.mTmin;

    //------

    
    mStack = new QStackedWidget(this);
    
    /* mEventsScrollArea and mPhasesScrollArea are made when we need it,
     *  within createEventsScrollArea and within createPhasesScrollArea
     */
    
    mMarker = new Marker(this);
    
    setMouseTracking(true);
    mStack->setMouseTracking(true);
    

    mOptionsWidget = new QWidget(this); // this is the parent of group of widget on the rigth of the panel
    mOptionsWidget->setFixedWidth(mOptionsW);

    /* -------------------------------------- mResultsGroup---------------------------------------------------*/

    mResultsGroup = new QWidget();

    mEventsfoldCheck = new CheckBox(tr("Unfold Events"), mResultsGroup);
    mEventsfoldCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mEventsfoldCheck->setToolTip(tr("Display phases' events"));

    mDatesfoldCheck = new CheckBox(tr("Unfold Data"), mResultsGroup);
    mDatesfoldCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mDatesfoldCheck->setToolTip(tr("Display Events' data"));

    mDataThetaRadio = new RadioButton(tr("Calendar Dates"), mResultsGroup);
    mDataThetaRadio->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mDataThetaRadio->setChecked(true);

    mDataSigmaRadio = new RadioButton(tr("Ind. Std. Deviations"), mResultsGroup);
    mDataSigmaRadio->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);

    mDataCalibCheck = new CheckBox(tr("Individual Calib. Dates"), mResultsGroup);
    mDataCalibCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mDataCalibCheck->setChecked(true);

    mWiggleCheck = new CheckBox(tr("Wiggle shifted"), mResultsGroup);
    mWiggleCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);

    mStatCheck = new CheckBox(tr("Show Stat."), mResultsGroup);
    mStatCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));


// ______ TempoGroup
    mTempoGroup = new QWidget();

    mDurationRadio = new RadioButton(tr("Phases Duration"), mTempoGroup);
    mDurationRadio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);
    mDurationRadio->setChecked(true);

    mTempoRadio = new RadioButton(tr("Phases Tempo"), mTempoGroup);
    mTempoRadio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);

    mActivityRadio = new RadioButton(tr("Phases Activity"), mTempoGroup);
    mActivityRadio->setFixedSize(int(mOptionsW - 2*mMargin), radioButtonHeight);

    mTempoStatCheck = new CheckBox(tr("Show Tempo Stat."), mTempoGroup);
    mTempoStatCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mTempoStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));

// -------------end TempoGroup


    mTabByScene = new Tabs(mOptionsWidget);
    mTabByScene->setFixedWidth(mOptionsW);
    // we set the same widget
    mTabByScene->addTab(mResultsGroup, tr("Events"));
    mTabByScene->addTab(mResultsGroup, tr("Phases"));
    mTabByScene->addTab(mTempoGroup, tr("Tempo"));

    connect(mTabByScene, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::updateTabs);
    connect(mTabByScene, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::changeScrollArea);

    connect(mDataThetaRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mDataSigmaRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mDataCalibCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mWiggleCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mStatCheck, &CheckBox::clicked, this, &ResultsView::showInfos);

    connect(mDurationRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mTempoRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mActivityRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mTempoStatCheck, &CheckBox::clicked, this, &ResultsView::showInfos);

    // -------------------------

    /* - mTabDisplayMCMC
     *  show Display (mSpanGroup, mGraphicGroup) and MCMC
     */
    mTabDisplayMCMC = new Tabs(mOptionsWidget);

    mTabDisplay = new QWidget();
    mTabMCMC = new QWidget();
    mTabDisplayMCMC->addTab(mTabDisplay, tr("Display"));
    mTabDisplayMCMC->addTab(mTabMCMC, tr("Distrib. Options"));

    //connect(mTabDisplayMCMC, &Tabs::tabClicked, mTabDisplayMCMC, &Tabs::showWidget);
    connect(mTabDisplayMCMC,static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::updateLayout);

    /* ----------------------------------------------------------
     *  Display Options layout
     * ----------------------------------------------------------*/

    // ------ Span Options -----
    mSpanTitle = new Label(tr("Span Options"), mTabDisplay);
    mSpanTitle->setIsTitle(true);
    mSpanTitle->setFixedSize(mOptionsW, titleHeight);

    mSpanGroup  = new QWidget(mTabDisplay);

    mDisplayStudyBut = new Button(tr("Study Period Display"), mSpanGroup);
    mDisplayStudyBut->setFixedSize(mOptionsW - 2*mMargin, buttonHeight);
    mDisplayStudyBut->setToolTip(tr("Restore view with the study period span"));

    mSpanLab = new Label(tr("Span"), mSpanGroup);
    mSpanLab->setFixedSize(fm.width(mSpanLab->text()), labelHeight);

    const int wEdit = (int)ceil(mOptionsW/3.);
    mCurrentXMinEdit = new LineEdit(mSpanGroup);
    mCurrentXMinEdit->setFixedSize(wEdit, lineEditHeight);
    mCurrentXMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentXMaxEdit = new LineEdit(mSpanGroup);
    mCurrentXMaxEdit->setFixedSize(wEdit, lineEditHeight);
    mCurrentXMaxEdit->setToolTip(tr("Enter a maximal value to display the curves"));

    mXScaleLab = new Label(tr("X"), mSpanGroup);
    mXScaleLab->setAlignment(Qt::AlignCenter);
    mXScaleLab->setFixedWidth(fm.width(mXScaleLab->text()));

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
    mXScaleSpin->setFixedSize(mCurrentXMinEdit->width(), spinBoxHeight);
    mXScaleSpin->setToolTip(tr("Enter zoom value to magnify the curves on X span"));

    mMajorScaleLab = new Label(tr("Major Interval"), mSpanGroup);
    mMajorScaleLab->setFixedSize(fm.width(mMajorScaleLab->text()), labelHeight);

    mMajorScaleEdit = new LineEdit(mSpanGroup);
    mMajorScaleEdit->setText(locale().toString(mMajorScale));
    mMajorScaleEdit->setFixedSize(wEdit, lineEditHeight);
    mMajorScaleEdit->setToolTip(tr("Enter a interval for the main division of the axes under the curves, upper than 1"));

    mMinorScaleLab = new Label(tr("Minor Interval Count"), mSpanGroup);
    mMinorScaleLab->setFixedSize(fm.width(mMinorScaleLab->text()), labelHeight);

    mMinorScaleEdit = new LineEdit(mSpanGroup);
    mMinorScaleEdit->setText(locale().toString(mMinorCountScale));
    mMinorScaleEdit->setFixedSize(wEdit, lineEditHeight);
    mMinorScaleEdit->setToolTip(tr("Enter a interval for the subdivision of the Major Interval for the scale under the curves, upper than 1"));

    /* -------------------------------------- Graphic Options (old mDisplayGroup) ---------------------------------------------------*/
    
    mGraphicTitle = new Label(tr("Graphic Options"), mTabDisplay);
    mGraphicTitle->setIsTitle(true);
    mGraphicTitle->setFixedSize(mOptionsW, titleHeight);

    mGraphicGroup = new QWidget(mTabDisplay);

    mYScaleLab = new Label(tr("Y"), mGraphicGroup);
    mYScaleLab->setAlignment(Qt::AlignCenter);
    mYScaleLab->setFixedSize(fm.width(mYScaleLab->text()), labelHeight);

    mYSlider = new QSlider(Qt::Horizontal, mGraphicGroup);
    mYSlider->setRange(10, 300);
    mYSlider->setTickInterval(1);
    mYSlider->setValue(100);
    
    mYScaleSpin = new QSpinBox(mGraphicGroup);
    mYScaleSpin->setRange(mYSlider->minimum(), mYSlider->maximum());
    mYScaleSpin->setSuffix(" %");
    mYScaleSpin->setValue(mYSlider->value());
    mYScaleSpin->setFixedSize(mCurrentXMinEdit->width(), labelHeight);
    mYScaleSpin->setToolTip(tr("Enter zoom value to magnify the curves on Y scale"));

    labFont = new Label(tr("Font"), mGraphicGroup);
    labFont->setFixedSize(fm.width(labFont->text()), labelHeight);
    labFont->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mFont.setPointSize(font().pointSize());
    mFontBut = new Button(mFont.family(), mGraphicGroup);
    mFontBut->setFixedSize(mOptionsW/2 - mMargin, labelHeight);
    mFontBut->setToolTip(tr("Click to change the font on the drawing"));
    connect(mFontBut, &QPushButton::clicked, this, &ResultsView::updateFont);
    
    labThickness = new Label(tr("Thickness"), mGraphicGroup);
    labThickness->setFixedSize(fm.width(labThickness->text()), comboBoxHeight);
    labThickness->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mThicknessCombo = new QComboBox(mGraphicGroup);
    mThicknessCombo->addItem(tr("1 px"));
    mThicknessCombo->addItem(tr("2 px"));
    mThicknessCombo->addItem(tr("3 px"));
    mThicknessCombo->addItem(tr("4 px"));
    mThicknessCombo->addItem(tr("5 px"));

    mThicknessCombo->setFixedSize(wEdit, comboBoxHeight );
    mThicknessCombo->setToolTip(tr("Select to change the thickness of the drawing"));

   // mThicknessSpin->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }"); // not supported
    connect(mThicknessCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::updateThickness);
    
    labOpacity = new Label(tr("Opacity"), mGraphicGroup);
    labOpacity->setFixedSize(fm.width(labOpacity->text()), comboBoxHeight);
    labOpacity->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mOpacityCombo = new QComboBox(mGraphicGroup);
    mOpacityCombo->addItem(tr("0 %"));
    mOpacityCombo->addItem(tr("10 %"));
    mOpacityCombo->addItem(tr("20 %"));
    mOpacityCombo->addItem(tr("30 %"));
    mOpacityCombo->addItem(tr("40 %"));
    mOpacityCombo->addItem(tr("50 %"));
    mOpacityCombo->addItem(tr("60 %"));
    mOpacityCombo->addItem(tr("70 %"));
    mOpacityCombo->addItem(tr("80 %"));
    mOpacityCombo->addItem(tr("90 %"));
    mOpacityCombo->addItem(tr("100 %"));
    mOpacityCombo->setFixedSize(wEdit, comboBoxHeight);
    mOpacityCombo->setToolTip(tr("Select to change the opacity of the drawing"));
    mOpacityCombo->setCurrentIndex(5);
    //mOpacitySpin->setStyleSheet("QLineEdit { border-radius: 5px; }"); // not supported

    connect(mOpacityCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::updateOpacity);

    labRendering = new Label(tr("Rendering"), mGraphicGroup);
    labRendering->setFixedSize(fm.width(labRendering->text()), comboBoxHeight);
    labRendering->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mRenderCombo = new QComboBox(mGraphicGroup);
    mRenderCombo->addItem(tr("Standard (faster)"));
    mRenderCombo->addItem(tr("Retina (slower)"));
   const  int renderWidth = std::min((int) fm.width(tr("Standard (faster)")),(int)( mOptionsW/2 - mMargin) );
    mRenderCombo->setFixedSize(renderWidth, comboBoxHeight);

   /* -------------------------------------- mChainsGroup---------------------------------------------------*/
    
    mChainsTitle = new Label(tr("MCMC Chains"), mTabMCMC);
    mChainsTitle->setIsTitle(true);
    mChainsTitle->setFixedSize(mOptionsW, titleHeight);

    mChainsGroup = new QWidget(mTabMCMC);

    mAllChainsCheck = new CheckBox(tr("Chains Concatenation"), mChainsGroup);
    mAllChainsCheck->setChecked(true);
    mAllChainsCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);

    /*
     * QList<CheckBox*> mCheckChainChecks;
     * QList<RadioButton*> mChainRadios;
     * mCheckChainChecks and mChainRadios are created in  initResults() they are children of mChainsGroup
    */

    /* -------------------------------------- mDensityOptsGroup ---------------------------------------------------*/
    
    mDensityOptsTitle = new Label(tr("Density Options"), mTabMCMC);
    mDensityOptsTitle->setIsTitle(true);
    mDensityOptsTitle->setFixedSize(mOptionsW, titleHeight);

    mDensityOptsGroup = new QWidget(mTabMCMC);
    
    mCredibilityCheck = new CheckBox(tr("Show Confidence Bar"), mDensityOptsGroup);
    mCredibilityCheck->setChecked(true);
    mCredibilityCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);

    mThreshLab = new Label(tr("Confidence Level") + " (%)", mDensityOptsGroup);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mThreshLab->setFixedHeight(lineEditHeight);
    
    mHPDEdit = new LineEdit(mDensityOptsGroup);
    mHPDEdit->setFixedSize(wEdit, lineEditHeight);
    mHPDEdit->setText("95");
    
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.0);
    percentValidator->setTop(100.0);
    percentValidator->setDecimals(1);
    mHPDEdit->setValidator(percentValidator);
    
    mFFTLenLab = new Label(tr("Grid Length"), mDensityOptsGroup);
    mFFTLenLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mFFTLenLab->setFixedHeight(comboBoxHeight);

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
    mFFTLenCombo->setFixedSize(wEdit, comboBoxHeight);
    mFFTLenCombo->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    mComboH = fm.height() + 6;
    mTabsH = mComboH + 2*mMargin;
    
    mBandwidthLab = new Label(tr("Bandwidth Const."), mDensityOptsGroup);
    mBandwidthLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mBandwidthLab->setFixedSize(fm.width(mBandwidthLab->text()), lineEditHeight);

    mBandwidthEdit = new LineEdit(mDensityOptsGroup);
    mBandwidthEdit->setFixedSize(wEdit, lineEditHeight);
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
    connect(mDisplayStudyBut, &Button::clicked, this, &ResultsView::setStudyPeriod);

    // Connection
    // QLineEdit::setText() doesn't emit signal textEdited, when the text is changed programmatically
    connect(mMajorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this,  &ResultsView::updateScaleX);
    connect(mMinorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this,  &ResultsView::updateScaleX);


    connect(mRuler, &Ruler::positionChanged, this, &ResultsView::updateScroll);
    
    connect(mYSlider, &QSlider::valueChanged, this, &ResultsView::updateScaleY);
    connect(mYScaleSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::updateScaleY);
    connect(mYScaleSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), mYSlider, &QSlider::setValue);
    // -------------------------
    
    connect(mRenderCombo,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::updateRendering);


    /*-------------
     * ------Tools for all graph
     * ------------- */
    mToolsWidget = new QWidget(this);
    mToolsWidget->resize(mOptionsW, 50);

    const QSize allDensitiesButSize (int(ceil(mOptionsW/2.)), 50);

    mExportImgBut = new Button(tr("Capture"), mToolsWidget);
    mExportImgBut->setFlatHorizontal();
    mExportImgBut->setIcon(QIcon(":picture_save.png"));
    mExportImgBut->setFixedSize(allDensitiesButSize);
    mExportImgBut->setToolTip(tr("Save all currently visible results as an image.<br><u>Note</u> : If you want to copy textual results, see the Log tab."));

    mExportResults = new Button(tr("Results"), mToolsWidget);
    mExportResults->setFlatHorizontal();
    mExportResults->setIcon(QIcon(":csv.png"));
    mExportResults->setFixedSize(allDensitiesButSize);
    mExportResults->setToolTip(tr("Export all result in several files"));

    connect(mExportImgBut, &Button::clicked, this, &ResultsView::exportFullImage);
    connect(mExportResults, &Button::clicked, this, &ResultsView::exportResults);


    /*-------------
     * ------Tools for single graph
     * ------------- */

    const QSize singleDensityButSize (mOptionsW/4, 50);
    mImageSaveBut = new Button(tr("Save"), mToolsWidget);
    mImageSaveBut->setIcon(QIcon(":picture_save.png"));
    mImageSaveBut->setFlatVertical();
    mImageSaveBut->setToolTip(tr("Save image as file"));
    mImageSaveBut->setFixedSize(singleDensityButSize);

    mImageClipBut = new Button(tr("Copy"), mToolsWidget);
    mImageClipBut->setIcon(QIcon(":clipboard_graph.png"));
    mImageClipBut->setFlatVertical();
    mImageClipBut->setToolTip(tr("Copy image to clipboard"));
    mImageClipBut->setFixedSize(singleDensityButSize);

    mResultsClipBut = new Button(tr("Copy"), mToolsWidget);
    mResultsClipBut->setIcon(QIcon(":text.png"));
    mResultsClipBut->setFlatVertical();
    mResultsClipBut->setToolTip(tr("Copy text results to clipboard"));
    mResultsClipBut->setFixedSize(singleDensityButSize);

    mDataSaveBut = new Button(tr("Save"), mToolsWidget);
    mDataSaveBut->setIcon(QIcon(":data.png"));
    mDataSaveBut->setFlatVertical();
    mDataSaveBut->setToolTip(tr("Save graph data to file"));
    mDataSaveBut->setFixedSize(singleDensityButSize);

    connect(mImageSaveBut, &Button::clicked, this, &ResultsView::saveAsImage);
    connect(mImageClipBut, &Button::clicked, this, &ResultsView::imageToClipboard);
    connect(mResultsClipBut, &Button::clicked, this, &ResultsView::resultsToClipboard);
    connect(mDataSaveBut, &Button::clicked, this, &ResultsView::saveGraphData);

    // Page widget
    mPageWidget = new QWidget (this);
    mPageWidget->resize(mOptionsW, mRulerH);

    mSheetNum = new LineEdit(mPageWidget);
    mSheetNum->setEnabled(false);
    mSheetNum->setReadOnly(true);
    mSheetNum->setFixedSize(fm.width("__/__"), 25);
    mSheetNum->setAlignment(Qt::AlignCenter);
    mSheetNum->setText(locale().toString(mMaximunNumberOfVisibleGraph));

    mPreviousSheetBut  = new Button(tr("Prev."), mPageWidget);
    mPreviousSheetBut->setFixedSize((mOptionsW- mSheetNum->width())/2, 25);
    mPreviousSheetBut->setCheckable(false);
    mPreviousSheetBut->setFlatHorizontal();
    mPreviousSheetBut->setToolTip(tr("Display previous data"));
    mPreviousSheetBut->setIconOnly(false);

    mNextSheetBut  = new Button(tr("Next"), mPageWidget);
    mNextSheetBut->setFixedSize((mOptionsW - mSheetNum->width())/2, 25);
    mNextSheetBut->setCheckable(false);
    mNextSheetBut->setFlatHorizontal();
    mNextSheetBut->setToolTip(tr("Display next data"));
    mNextSheetBut->setIconOnly(false);

    mNbDensityLab = new Label(tr("Nb Densities / Sheet"), mPageWidget);
    mNbDensityLab->setFixedSize(fm.width(mNbDensityLab->text()), spinBoxHeight);
    mNbDensityLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mNbDensitySpin = new QSpinBox(mPageWidget);
    mNbDensitySpin->setRange(1, 100);
    mNbDensitySpin->setValue(mNumberOfGraph);
    mNbDensitySpin->setFixedSize(mCurrentXMinEdit->width(), spinBoxHeight);
    mNbDensitySpin->setToolTip(tr("Enter the maximum densities to display on a sheet"));

    connect(mPreviousSheetBut, &Button::pressed, this, &ResultsView::previousSheet);
    connect(mNextSheetBut, &Button::pressed, this, &ResultsView::nextSheet);
    connect(mNbDensitySpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::updateNbDensity);

    mTabPageSaving = new Tabs(mOptionsWidget);
    mTabPageSaving->setFixedWidth(mOptionsW);
    mTabPageSaving->addTab(mPageWidget, tr("Page"));
    mTabPageSaving->addTab(mToolsWidget, tr("Saving"));

   // connect(mTabPageSaving, &Tabs::tabClicked, mTabPageSaving, &Tabs::showWidget);
    connect(mTabPageSaving,static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::updateLayout);

    //connect(mTabDisplayMCMC, &Tabs::tabClicked, this, &ResultsView::updateTabDisplay);

    updateTabByScene();
    updateTabByTempo();
    mTabByScene->setTab(0, false);
    mTabByScene->showWidget(0);

    mMarker->raise();
    mTabDisplayMCMC->setTab(0, false);
    mTabDisplayMCMC->showWidget(0);
    updateTabDisplay(mTabDisplayMCMC->currentIndex());

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

void ResultsView::paintEvent(QPaintEvent* )
{
    //qDebug()<< "ResultsView::paintEvent()";
}

void ResultsView::setFont(const QFont & font)
{
    mTabs->setFont(font);
}

void ResultsView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateControls(); // emit controleUpdated which is connected to updateLayout
}


/**
 * @brief ResultsView::updateControls set controls according to the differents tabs positions.  emit controlsUpdated()
*/
void ResultsView::updateControls()
{
   qDebug() << "ResultsView::updateControls()";
    bool byEvents (mTabByScene->currentIndex() == 0);
    bool byPhases (mTabByScene->currentIndex() == 1);
    bool byTempo  (mTabByScene->currentIndex() == 2);


    /* -------------------------------------------------------
     *  Activate specific controls for post. distrib. (first tab)
     * -------------------------------------------------------*/

//    mAllChainsCheck    -> setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    mDataCalibCheck    -> setVisible((mCurrentTypeGraph == GraphViewResults::ePostDistrib)
                                     && mDatesfoldCheck->isVisible()
                                     && mDatesfoldCheck->isChecked()
                                     && mDataThetaRadio->isChecked());
    mWiggleCheck       -> setVisible((mCurrentTypeGraph == GraphViewResults::ePostDistrib)
                                     && mDatesfoldCheck->isVisible()
                                     && mDatesfoldCheck->isChecked()
                                     && mDataThetaRadio->isChecked());

//    mDensityOptsTitle -> setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
//    mDensityOptsGroup -> setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
//    mCredibilityCheck->setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
//    mThreshLab->setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
//    mFFTLenLab->setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
//    mBandwidthLab->setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
//    /* -------------------------------------------------------
//     *  Switch between checkBoxes or Radio-buttons for chains
//     * -------------------------------------------------------*/
//    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {
//        for (auto &&checkChain : mCheckChainChecks)
//            checkChain->setVisible(true);
        
//        for (auto &&chainRadio : mChainRadios)
//            chainRadio->setVisible(false);

//    } else {
//        for (auto &&checkChain : mCheckChainChecks)
//            checkChain->setVisible(false);
        
//        for (auto &&chainRadio : mChainRadios)
//            chainRadio->setVisible(true);
//    }
    
    /* -------------------------------------------------------
     *  Display by phases or by events
     * -------------------------------------------------------*/
    if (byEvents) {
            if (!mTempoScrollArea)
                createTempoScrollArea(mTabTempoIndex);
            mStack->setCurrentWidget(mEventsScrollArea);
    } else if (byPhases) {
        if (!mTempoScrollArea)
            createTempoScrollArea(mTabTempoIndex);
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

    int dx (20 + mMargin);

    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {
        if (mDataCalibCheck->isVisible()) {
            mDataCalibCheck -> move(mMargin + dx, ySpan);
            ySpan += mDataCalibCheck->height() + mMargin;
        }
        if (mWiggleCheck->isVisible()) {
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

    mActivityRadio -> move(mMargin, ySpan);
    ySpan += mActivityRadio->height() + mMargin;

    mTempoStatCheck->move(mMargin , ySpan);
    ySpan += mTempoStatCheck->height() + mMargin;

    mTempoGroup->resize(mOptionsW, ySpan);
    mTabByScene->resize(mOptionsW, mTabByScene->minimalHeight());
}

void ResultsView::updateTabDisplay(const int &i)
{

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

        mCurrentXMaxEdit->move(mOptionsW - mCurrentXMinEdit->width() - mMargin, ySpan );

        const int w = mSpanLab->width();
        mSpanLab->setGeometry((mCurrentXMinEdit->x() + mCurrentXMinEdit->width() + mCurrentXMaxEdit->x() )/2. - (w/2.), mCurrentXMinEdit->y() , w, mCurrentXMinEdit->height() );

        ySpan += mMargin + mCurrentXMinEdit->height();

        int heiTemp = mXScaleSpin->height();
        mXScaleLab->setGeometry(mMargin, ySpan , wBut - 4*mMargin, heiTemp);

        mXScaleSpin->move(mOptionsW - mXScaleSpin->width() - mMargin, ySpan);
        const int xSliderWidth = mOptionsW - mXScaleLab->width() - mXScaleSpin->width() - 4*mMargin;
        mXSlider->setGeometry(mXScaleLab->x() + mXScaleLab->width() + mMargin , ySpan, xSliderWidth, heiTemp );

        ySpan += mXScaleSpin->height() + mMargin;

        if (mCurrentTypeGraph != GraphViewResults::eCorrel) {
            mMajorScaleLab->setVisible(true);
            mMajorScaleEdit->setVisible(true);
            mMinorScaleLab->setVisible(true);
            mMinorScaleEdit->setVisible(true);

            mMajorScaleLab->setGeometry(mMargin, ySpan , wBut - 4*mMargin, heiTemp);
            mMajorScaleEdit->move(mOptionsW - mMargin - mMajorScaleEdit->width(), ySpan );
            ySpan += mMajorScaleEdit->height() + mMargin;

            mMinorScaleLab->setGeometry(mMargin, ySpan , wBut - 4*mMargin, heiTemp);
            mMinorScaleEdit->move(mOptionsW - mMargin - mMinorScaleEdit->width(), ySpan );
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
        mYScaleLab->setGeometry(mMargin, ySpan , wBut - 4*mMargin, heiTemp);
        mYScaleSpin->move(mOptionsW - mYScaleSpin->width() - mMargin, ySpan);
        const int ySliderWidth = mOptionsW - mYScaleLab->width() - mYScaleSpin->width() - 4*mMargin;
        mYSlider->setGeometry(mYScaleLab->x() + mYScaleLab->width() + mMargin , ySpan, ySliderWidth, heiTemp );

        ySpan += mMargin + mYScaleSpin->height();
        labFont->move(mMargin, ySpan);
        mFontBut->move(mOptionsW/2, ySpan );

        ySpan += mMargin + mFontBut->height();
        labThickness->move(mMargin, ySpan);
        mThicknessCombo->move(mOptionsW/2, ySpan);

        ySpan += mMargin + mThicknessCombo->height();
        labOpacity->move(mMargin, ySpan);
        mOpacityCombo->move(mOptionsW/2, ySpan);

        ySpan += mMargin + mOpacityCombo->height();
        labRendering->move(mMargin, ySpan);
        mRenderCombo->move(mOptionsW/2, ySpan);

        // fit the size and the position of the widget of the group in the mTabDisplay coordonnate
        ySpan += mMargin + mRenderCombo->height();
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
                  mThreshLab->move(mMargin, ySpan);
                  mHPDEdit->move(mOptionsW - mMargin - mHPDEdit->width(), ySpan);
                  ySpan += mHPDEdit->height() + mMargin;

                  mFFTLenLab->move(mMargin, ySpan);
                  mFFTLenCombo->move(mOptionsW - mMargin - mFFTLenCombo->width(), ySpan);
                  ySpan += mFFTLenCombo->height() + mMargin;

                  mBandwidthLab->move(mMargin, ySpan);
                  mBandwidthEdit->move(mOptionsW - mMargin - mBandwidthEdit->width(), ySpan);
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
            mPreviousSheetBut->move(0 , ySpan);

            mSheetNum->move(mPreviousSheetBut->width(), ySpan);

            mSheetNum->setText(locale().toString(byPhases ? mTabPhasesIndex+1 : mTabEventsIndex+1 ) + "/"
                               + locale().toString(ceil((double)mMaximunNumberOfVisibleGraph/(double)mNumberOfGraph) ));

            mNextSheetBut->move(mOptionsW - mPreviousSheetBut->width(),  ySpan);

            ySpan += mNextSheetBut->height() + mMargin;

            mNbDensityLab->move(mMargin, ySpan);
            mNbDensitySpin->move(mOptionsW - mNbDensitySpin->width() - mMargin, ySpan);
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

    else if (byTempo)
            createTempoScrollArea(mTabTempoIndex);

}

/**
 * @brief ResultsView::updateTabs Update mTabs according to mTabByScene index
 * @param index
 */
void ResultsView:: updateTabs(const int &index)
{
    switch (index) {
        case 0: //mTabByScene on Events

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
    int graphYAxis (50);

    /* ----------------------------------------------------------
     *  Main layout
     * ----------------------------------------------------------*/

    mTabs->setGeometry(graphYAxis, 0, width() - mOptionsW - sbe - graphYAxis, mTabsH);
    mStack->setGeometry(0, mTabsH + mRulerH, width() - mOptionsW, height() - mRulerH - mTabsH);

    mMarker->setGeometry(mMarker->pos().x(), mTabsH + sbe, mMarker->thickness(), height() - sbe - mTabsH);

    if (mStatCheck->isChecked() || mTempoStatCheck->isChecked())
         mRuler->setGeometry(0, mTabsH, (width() - mOptionsW - sbe)*2./3., mRulerH);
    else
        mRuler->setGeometry(0, mTabsH, width() - mOptionsW - sbe, mRulerH);

    mMarker->setGeometry(mMarker->pos().x(), mTabsH + sbe, mMarker->thickness(), height() - sbe - mTabsH);
    
    /* ----------------------------------------------------------
     *  Display Options layout
     * ----------------------------------------------------------*/
    int ySpan(5);

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
                graph->setGeometry(0, y, width() - mOptionsW - sbe, mGraphsH);
                graph->update();
                y += graph->height();
            }
            if (y>0)
                wid->setFixedSize(width() - sbe - mOptionsW, y);
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
                graph->setGeometry(0, y, width() - mOptionsW - sbe, mGraphsH);
                graph->update();
                y += graph->height();
            }
            if (y>0)
                wid->setFixedSize(width() - sbe - mOptionsW, y);
         }
        
    }
    else if (byTempo){
         int y (0);
         if (mTempoScrollArea) {
            QWidget* wid = mTempoScrollArea->widget();

            for (auto &&graph : mByTempoGraphs) {
                graph->setGeometry(0, y, width() - mOptionsW - sbe, mGraphsH);
                graph->update();
                y += graph->height();
            }
            if (y>0)
                wid->setFixedSize(width() - sbe - mOptionsW, y);
         }

    }
    update();
    
}


void ResultsView::clearResults()
{
    for (auto &&check : mCheckChainChecks )
        delete check;

    mCheckChainChecks.clear();
    
    for (auto &&chain : mChainRadios)
        delete chain;

    mChainRadios.clear();

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


void ResultsView::updateFormatSetting(Model* model, const AppSettings* appSet)
{
    if (!mModel && !model)
        return;
    if (model)
        mModel = model;
    mModel->updateFormatSettings(appSet);

}

/**
 * @brief : This function is call after "Run"
 *
 */
void ResultsView::initResults(Model* model)
{
    clearResults();

    //qDebug() << "ResultsView::initResults";

    if (!mModel && !model)
        return;

    if (model)
        mModel = model;
    //qDebug() << "ResultsView::initResults with model";
    QFontMetricsF fm(qApp->font());

    mMarginLeft = std::max(fm.width(locale().toString(DateUtils::convertToAppSettingsFormat(mModel->mSettings.mTmin))),
                           fm.width(locale().toString(DateUtils::convertToAppSettingsFormat(mModel->mSettings.mTmin)))) + 5;
    Scale xScale;
    xScale.findOptimal(mModel->mSettings.mTmin, mModel->mSettings.mTmax, 7);

    mMajorScale = xScale.mark;
    mMinorCountScale = 4;

    mRuler->setScaleDivision(mMajorScale, mMinorCountScale);

    mMajorScaleEdit->setText(locale().toString(mMajorScale));
    mMinorScaleEdit->setText(locale().toString(mMinorCountScale));

    mChains = mModel->mChains;
    mSettings = mModel->mSettings;
    mMCMCSettings = mModel->mMCMCSettings;

    mHasPhases = (mModel->mPhases.size() > 0);

    if (mHasPhases) {
        mTabByScene->setTab(1, false);
        mTabByScene->setTabVisible(1, true);
        mTabByScene->setTabVisible(2, true);

     } else {
        mTabByScene->setTabVisible(2, false);
        mTabByScene->setTabVisible(1, false);
        mTabByScene->setTab(0, false);
     }
    /* ----------------------------------------------------
     *  Create Chains option controls (radio and checkboxes under "MCMC Chains")
     * ---------------------------------------------------- */
    if (mCheckChainChecks.isEmpty()) {
        for (int i=0; i<mChains.size(); ++i) {
            CheckBox* check = new CheckBox(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
            connect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
            check->setVisible(true);
            check->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
            mCheckChainChecks.append(check);

            RadioButton* radio = new RadioButton(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
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

    if (mHasPhases)
        createPhasesScrollArea(mTabPhasesIndex);
    else
        createEventsScrollArea(mTabEventsIndex);

    // ------------------------------------------------------------
    showInfos(false);
    updateControls();

}

/**
 * @brief This function is call after click on "Results", when switching from Model panel to Result panel
 */
void ResultsView::updateResults(Model* model)
{
    qDebug() << "ResultsView::updateResults";

    if (!mModel && !model)
        return;

    if (model)
        mModel = model;

    mChains = mModel->mChains;
    mSettings = mModel->mSettings;
    mMCMCSettings = mModel->mMCMCSettings;

    QFontMetricsF fm(qApp->font());

    mMarginLeft = std::max(fm.width(locale().toString(DateUtils::convertToAppSettingsFormat(mModel->mSettings.mTmin))),
                           fm.width(locale().toString(DateUtils::convertToAppSettingsFormat(mModel->mSettings.mTmin)))) + 5;

    mHasPhases = (mModel->mPhases.size() > 0);

    if (mHasPhases) {
        mTabByScene->setTab(1, false);
        mTabByScene->setTabVisible(1, true);
        mTabByScene->setTabVisible(2, true);

     } else {
        mTabByScene->setTabVisible(2, false);
        mTabByScene->setTabVisible(1, false);
        mTabByScene->setTab(0, false);
     }

    /* ----------------------------------------------------
    *  Update Chains option controls (radio and checkboxes under "MCMC Chains")
    * ---------------------------------------------------- */
    if (mCheckChainChecks.isEmpty()) {
        for (int i = 0; i<mChains.size(); ++i) {
            CheckBox* check = new CheckBox(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
            connect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
            check->setVisible(true);
            check->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
            mCheckChainChecks.append(check);

            RadioButton* radio = new RadioButton(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
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
    if (mHasPhases)
        createPhasesScrollArea(mTabPhasesIndex);
    else
        createEventsScrollArea(mTabEventsIndex);

    // ------------------------------------------------------------
    showInfos(false);
    updateControls();


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
                graphEvent->setGraphFont(mFont);
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

                                graphDate->setGraphFont(mFont);
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
                graphPhase->setGraphFont(mFont);
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
                        graphEvent->setGraphFont(mFont);
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
                                graphDate->setGraphFont(mFont);
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
                graphTempo->setGraphFont(mFont);
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
                 if (curve.mName.contains("Sigma") && (curve.mVisible == true)) {
                     mResultMaxVariance = ceil(qMax(mResultMaxVariance, curve.mData.lastKey()));
                     qDebug()<<"Sigma Date: "<<curve.mName<<curve.mData.lastKey()<<mResultMaxVariance;
                 }
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
    mRuler->setFormatFunctX(stringWithAppSettings);

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
        log = tr("impossible to compute");
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
    mCurrentXMinEdit->setText(stringWithAppSettings(mResultCurrentMinX));
    mCurrentXMaxEdit->setText(stringWithAppSettings(mResultCurrentMaxX));
}

void ResultsView::updateScaleEdit()
{
    mMinorScaleEdit->setText(locale().toString(mMinorCountScale));
    mMajorScaleEdit->setText(locale().toString(mMajorScale));
}

/**
 * @brief ResultsView::updateGraphsZoomX
 * Update graphs display according to the current values and store them in mZooms[] and mScales[]
 * used in updateScales()
 */
void ResultsView::updateGraphsZoomX()
{
   // qDebug()<<"ResultsView::updateGraphsZoomX()";

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
    mYScaleSpin->setValue(value);
    const double min (70.);
    const double origin (150.);
    const double prop = (double)value / 100.;
    mGraphsH = min + prop * (origin - min);
    
    updateGraphsLayout();
}


// Display options
/**
 * @brief ResultsView::updateFont only on graph
 */
void ResultsView::updateFont()
{
    QFontDialog dialog;
    dialog.setParent(qApp->activeWindow());
    dialog.setFont(mFont);
    
    bool ok;
    const QFont font = QFontDialog::getFont(&ok, mFont, this);
    if (ok) {
        mFont = font;
        mFontBut->setText(mFont.family() + ", " + QString::number(mFont.pointSizeF()));
        
        for (GraphViewResults* phaseGraph : mByPhasesGraphs)
            phaseGraph->setGraphFont(mFont);

        for (GraphViewResults* eventGraph : mByEventsGraphs)
            eventGraph->setGraphFont(mFont);

        for (GraphViewResults* tempoGraph : mByTempoGraphs)
            tempoGraph->setGraphFont(mFont);

        mRuler->setFont(mFont);

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

void ResultsView::updateRendering(int index)
{
    for (GraphViewResults* allKindGraph : mByEventsGraphs)
        allKindGraph->setRendering((GraphView::Rendering) index);

    for (GraphViewResults* allKindGraph : mByPhasesGraphs)
        allKindGraph->setRendering((GraphView::Rendering) index);

    for (GraphViewResults* allKindGraph : mByTempoGraphs)
        allKindGraph->setRendering((GraphView::Rendering) index);

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

            const short pr = MainWindow::getInstance()->getAppSettings().mPixelRatio;

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
                GraphView::Rendering memoRendering= graph->getRendering();
                graph->setRendering(GraphView::eHD);
                graph->render(&p, ptStart, QRegion(0, 0, graph->width(), graph->height()));
                ptStart = QPoint(0, ptStart.y() + graph->height());
                graph->showSelectedRect(true);
                graph->setRendering(memoRendering);
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
                int imageQuality = MainWindow::getInstance()->getAppSettings().mImageQuality;
                image.save(fileName, "jpg",imageQuality);
            }
            else if (fileExtension == "bmp") {
                image.save(fileName, "bmp");
            }

        } // not svg
        // if svg type
        else {
            //Rendering memoRendering= mRendering;
            QRect rTotal( QRect(0, 0, width(), height()+versionHeight) );
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
                GraphView::Rendering memoRendering= graph->getRendering();
                graph->setRendering(GraphView::eHD);
                graph->render(&painter, ptStart, QRegion(0, 0, graph->width(), graph->height()));
                ptStart = QPoint(0, ptStart.y() + graph->height());
                graph->showSelectedRect(true);
                graph->setRendering(memoRendering);
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
    short pr = MainWindow::getInstance()->getAppSettings().mPixelRatio;

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
        GraphView::Rendering memoRendering= graph->getRendering();
        graph->setRendering(GraphView::eHD);
        graph->render(&p, ptStart, QRegion(0, 0, graph->width(), graph->height()));
        ptStart = QPoint(0, ptStart.y() + graph->height());
        graph->showSelectedRect(true);
        graph->setRendering(memoRendering);
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

    if (mTabByScene->currentIndex() == 1) {
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

    if (mTabByScene->currentIndex() == 1) {
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
        AppSettings settings = MainWindow::getInstance()->getAppSettings();
        const QString csvSep = settings.mCSVCellSeparator;
        const int precision = settings.mPrecision;
        QLocale csvLocal = settings.mCSVDecSeparator == "." ? QLocale::English : QLocale::French;

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
            const QString projectName = tr("Project filename")+" : "+ MainWindow::getInstance()->getNameProject()+ "<br>";

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
    
    type_data max (0.);

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
    int rendering = mRenderCombo->currentIndex();
    updateRendering(1);
    
    AxisWidget* axisWidget = nullptr;
    QLabel* axisLegend = nullptr;
    int axeHeight (20);
    int legendHeight (20);
    
    if (printAxis) {
        curWid->setFixedHeight(curWid->height() + axeHeight + legendHeight );
        
        FormatFunc f = nullptr;
        if (mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked())
            f = stringWithAppSettings;

        QFontMetricsF fmAxe (qApp->font());
        qreal marginRight = floor(fmAxe.width(stringWithAppSettings(max)) / 2.);


        axisWidget = new AxisWidget(f, curWid);
        axisWidget->mMarginLeft = 50.;
        axisWidget->mMarginRight = marginRight;
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
        if (mStatCheck->isChecked())
            axisLegend->setGeometry(0, curWid->height() - axeHeight - legendHeight, curWid->width()*2./3. - 10, legendHeight);
        else
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
    updateRendering(rendering);
    
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

