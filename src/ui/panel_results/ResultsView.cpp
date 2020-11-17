/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

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

#include "ModelChronocurve.h"

#include <QtWidgets>
#include <iostream>
#include <QtSvg>
#include <QFontDialog>



ResultsView::ResultsView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mModel(nullptr),
mResultZoomX(1.),
mResultMinX(0.),
mResultMaxX(0.),
mResultCurrentMinX(0.),
mResultCurrentMaxX(0.),
mResultMaxVariance(1000.),
mResultMaxDuration(0.),
mHasPhases(false),

mMargin(5),
mOptionsW(250),
mCurrentPage(0),

mTempoScrollArea(nullptr),
forceXSpinSetValue(false),
forceXSlideSetValue(false),
mCurrentTypeGraph(GraphViewResults::ePostDistrib), //mGraphTypeTabs=0
mCurrentVariable(GraphViewResults::eTheta),
mGraphsPerPage(APP_SETTINGS_DEFAULT_SHEET),
mMaximunNumberOfVisibleGraph(0),
mMajorScale(100),
mMinorCountScale (4)
{
    setMouseTracking(true);
    
    // -----------------------------------------------------------------
    //  Left part : Tabs, Ruler, Stack
    // -----------------------------------------------------------------
    mGraphTypeTabs = new Tabs(this);
    mGraphTypeTabs->addTab(tr("Posterior Distrib."));
    mGraphTypeTabs->addTab(tr("History Plot"));
    mGraphTypeTabs->addTab(tr("Acceptance Rate"));
    mGraphTypeTabs->addTab(tr("Autocorrelation"));
    mGraphTypeTabs->setTab(0, false);
    
    mRuler = new Ruler(this);
    mMarker = new Marker(this);
    
    mEventsScrollArea = new QScrollArea();
    mEventsScrollArea->setMouseTracking(true);
    QWidget* eventsWidget = new QWidget();
    eventsWidget->setMouseTracking(true);
    mEventsScrollArea->setWidget(eventsWidget);
    
    mPhasesScrollArea = new QScrollArea(this);
    mPhasesScrollArea->setMouseTracking(true);
    QWidget* phasesWidget = new QWidget();
    phasesWidget->setMouseTracking(true);
    mPhasesScrollArea->setWidget(phasesWidget);
    
    mTempoScrollArea = new QScrollArea(this);
    mTempoScrollArea->setMouseTracking(true);
    QWidget* tempoWidget = new QWidget();
    tempoWidget->setMouseTracking(true);
    mTempoScrollArea->setWidget(tempoWidget);
    
    mCurveScrollArea = new QScrollArea(this);
    mCurveScrollArea->setMouseTracking(true);
    QWidget* curveWidget = new QWidget();
    curveWidget->setMouseTracking(true);
    mCurveScrollArea->setWidget(curveWidget);
    
    mGraphListStack = new QStackedWidget(this);
    mGraphListStack->setMouseTracking(true);
    mGraphListStack->addWidget(mEventsScrollArea);
    mGraphListStack->addWidget(mPhasesScrollArea);
    mGraphListStack->addWidget(mTempoScrollArea);
    mGraphListStack->addWidget(mCurveScrollArea);
    
    connect(mGraphTypeTabs, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::graphTypeChange);
    
    
    // -----------------------------------------------------------------
    //  Right part
    // -----------------------------------------------------------------
    mOptionsWidget = new QWidget(this);

    // -----------------------------------------------------------------
    //  Results Group (if graph list tab = events or phases)
    // -----------------------------------------------------------------
    mResultsGroup = new QWidget();
    mResultsGroup->setPalette(QLabel().palette());

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
    
    QVBoxLayout* resultsGroupLayout = new QVBoxLayout();
    resultsGroupLayout->setContentsMargins(0, 0, 0, 0);
    resultsGroupLayout->setSpacing(0);
    resultsGroupLayout->addWidget(mDataThetaRadio);
    resultsGroupLayout->addWidget(mDataSigmaRadio);
    resultsGroupLayout->addWidget(mEventsfoldCheck);
    resultsGroupLayout->addWidget(mDatesfoldCheck);
    resultsGroupLayout->addWidget(mDataCalibCheck);
    resultsGroupLayout->addWidget(mWiggleCheck);
    resultsGroupLayout->addWidget(mStatCheck);
    mResultsGroup->setLayout(resultsGroupLayout);
    
    connect(mDataThetaRadio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mDataSigmaRadio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mDataCalibCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mWiggleCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mStatCheck, &CheckBox::clicked, this, &ResultsView::showInfos);

    // -----------------------------------------------------------------
    //  Tempo Group (if graph list tab = duration)
    // -----------------------------------------------------------------
    mTempoGroup = new QWidget();

    mDurationRadio = new RadioButton(tr("Phase Duration"), mTempoGroup);
    mDurationRadio->setChecked(true);

    mTempoRadio = new RadioButton(tr("Phase Tempo"), mTempoGroup);

    mTempoCredCheck = new CheckBox(tr("Tempo Cred."), mTempoGroup);
    mTempoErrCheck = new CheckBox(tr("Tempo Error"), mTempoGroup);

    mActivityRadio = new RadioButton(tr("Phase Activity"), mTempoGroup);

    mTempoStatCheck = new CheckBox(tr("Show Tempo Stat."), mTempoGroup);
    mTempoStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));
    
    QVBoxLayout* tempoGroupLayout = new QVBoxLayout();
    tempoGroupLayout->setContentsMargins(0, 0, 0, 0);
    tempoGroupLayout->setSpacing(0);
    resultsGroupLayout->addWidget(mDurationRadio);
    resultsGroupLayout->addWidget(mTempoRadio);
    resultsGroupLayout->addWidget(mTempoCredCheck);
    resultsGroupLayout->addWidget(mTempoErrCheck);
    resultsGroupLayout->addWidget(mActivityRadio);
    resultsGroupLayout->addWidget(mTempoStatCheck);
    mTempoGroup->setLayout(tempoGroupLayout);
    
    connect(mDurationRadio, &RadioButton::clicked, this, &ResultsView::setCurrentVariable);
    connect(mTempoRadio, &RadioButton::clicked, this, &ResultsView::setCurrentVariable);
    connect(mActivityRadio, &RadioButton::clicked, this, &ResultsView::setCurrentVariable);
    
    connect(mTempoStatCheck, &CheckBox::clicked, this, &ResultsView::showInfos);
    connect(mTempoCredCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mTempoErrCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    // -----------------------------------------------------------------
    //  Graph List tab (has to be created after mResultsGroup and mTempoGroup)
    // -----------------------------------------------------------------
    mGraphListTab = new Tabs();
    mGraphListTab->addTab(tr("Events"));
    mGraphListTab->addTab(tr("Phases"));
    mGraphListTab->addTab(tr("Tempo"));
    mGraphListTab->addTab(tr("Curve"));
    mGraphListTab->setTab(0, false);

    connect(mGraphListTab, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::setGraphListTab);
    connect(mGraphListTab, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), mGraphListStack, &QStackedWidget::setCurrentIndex);

    // -----------------------------------------------------------------
    //  Tabs : Display / Distrib. Options
    // -----------------------------------------------------------------
    mTabDisplayMCMC = new Tabs();
    
    mTabDisplayMCMC->addTab(tr("Display"));
    mTabDisplayMCMC->addTab(tr("Distrib. Options"));
    mTabDisplayMCMC->setTab(0, false);
    
    // Necessary to reposition all elements inside the selected tab :
    connect(mTabDisplayMCMC, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::setDisplayTab);

    /* ----------------------------------------------------------
     *  Display Options layout
     * ----------------------------------------------------------*/
    mTabDisplay = new QWidget();
    mSpanGroup  = new QWidget();
    
    mSpanTitle = new Label(tr("Span Options"), mTabDisplay);
    mSpanTitle->setIsTitle(true);
    
    mDisplayStudyBut = new Button(tr("Study Period Display"), mSpanGroup);
    mDisplayStudyBut->setToolTip(tr("Restore view with the study period span"));
    mSpanLab = new Label(tr("Span"), mSpanGroup);
    mSpanLab->setAdjustText(false);

    mCurrentXMinEdit = new LineEdit(mSpanGroup);
    mCurrentXMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentXMaxEdit = new LineEdit(mSpanGroup);
    mCurrentXMaxEdit->setToolTip(tr("Enter a maximal value to display the curves"));

    mXScaleLab = new Label(tr("X"), mSpanGroup);
    mXScaleLab->setAlignment(Qt::AlignCenter);
    mXScaleLab->setAdjustText(false);

    mXSlider = new QSlider(Qt::Horizontal, mSpanGroup);
    mXSlider->setRange(-100, 100);
    mXSlider->setTickInterval(1);
    forceXSlideSetValue = true;
    mXSlider->setValue(0);

    mXScaleSpin = new QDoubleSpinBox(mSpanGroup);
    mXScaleSpin->setRange(pow(10., double (mXSlider->minimum()/100.)),pow(10., double (mXSlider->maximum()/100.)));
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


    // ------------------------------------
    //  Graphic Options
    // ------------------------------------
    mGraphicTitle = new Label(tr("Graphic Options"), mTabDisplay);
    mGraphicTitle->setIsTitle(true);

    mGraphicGroup = new QWidget(mTabDisplay);

    mYScaleLab = new Label(tr("Y"), mGraphicGroup);
    mYScaleLab->setAlignment(Qt::AlignCenter);
    mYScaleLab->setAdjustText(false);

    mYSlider = new QSlider(Qt::Horizontal, mGraphicGroup);
    mYSlider->setRange(10, 300);
    mYSlider->setTickInterval(1);
    mYSlider->setValue(100);

    mYScaleSpin = new QSpinBox(mGraphicGroup);
    mYScaleSpin->setRange(mYSlider->minimum(), mYSlider->maximum());
    mYScaleSpin->setValue(mYSlider->value());
    mYScaleSpin->setToolTip(tr("Enter zoom value to magnify the curves on Y scale"));

    mLabFont = new QLabel(tr("Font"), mGraphicGroup);
    mLabFont->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mFontBut = new Button(font().family() + ", " + QString::number(font().pointSizeF()), mGraphicGroup);
    mFontBut->setToolTip(tr("Click to change the font on the drawing"));

    mLabThickness = new QLabel(tr("Thickness"), mGraphicGroup);
    mLabThickness->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mThicknessCombo = new QComboBox(mGraphicGroup);
    mThicknessCombo->addItem("1 px");
    mThicknessCombo->addItem("2 px");
    mThicknessCombo->addItem("3 px");
    mThicknessCombo->addItem("4 px");
    mThicknessCombo->addItem("5 px");
    mThicknessCombo->setToolTip(tr("Select to change the thickness of the drawing"));

    mLabOpacity = new QLabel(tr("Opacity"), mGraphicGroup);
    mLabOpacity->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

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

    connect(mYSlider, &QSlider::valueChanged, this, &ResultsView::updateScaleY);
    connect(mYScaleSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::updateScaleY);
    connect(mYScaleSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), mYSlider, &QSlider::setValue);
    connect(mFontBut, &QPushButton::clicked, this, &ResultsView::updateFont);
    connect(mThicknessCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::updateThickness);
    connect(mOpacityCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::updateOpacity);

    // ------------------------------------
    //  MCMC Chains
    //  Note : mCheckChainChecks and mChainRadios are populated by initResults()
    // ------------------------------------
    mTabMCMC = new QWidget();
    
    mChainsTitle = new Label(tr("MCMC Chains"), mTabMCMC);
    mChainsTitle->setIsTitle(true);
    
    mChainsGroup = new QWidget(mTabMCMC);

    mAllChainsCheck = new CheckBox(tr("Chain Concatenation"), mChainsGroup);
    mAllChainsCheck->setChecked(true);
    
    connect(mAllChainsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    // ------------------------------------
    //  Density Options
    // ------------------------------------
    mDensityOptsTitle = new Label(tr("Density Options"), mTabMCMC);
    mDensityOptsTitle->setIsTitle(true);

    mDensityOptsGroup = new QWidget(mTabMCMC);
    
    mCredibilityCheck = new CheckBox(tr("Show Confidence Bar"), mDensityOptsGroup);
    mCredibilityCheck->setChecked(true);
    
    mThreshLab = new Label(tr("Confidence Level (%)"), mDensityOptsGroup);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mThresholdEdit = new LineEdit(mDensityOptsGroup);
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.0);
    percentValidator->setTop(100.0);
    percentValidator->setDecimals(1);
    mThresholdEdit->setValidator(percentValidator);
    
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

    mBandwidthLab = new Label(tr("Bandwidth Const."), mDensityOptsGroup);
    mBandwidthLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mBandwidthEdit = new LineEdit(mDensityOptsGroup);
    DoubleValidator* bandwidthValidator = new DoubleValidator();
    bandwidthValidator->setBottom(1);
    bandwidthValidator->setTop(100);
    bandwidthValidator->setDecimals(0);
    mBandwidthEdit->setValidator(bandwidthValidator);
    
    connect(mCredibilityCheck, &CheckBox::clicked, this, &ResultsView::generateCurvesRequested);
    connect(mFFTLenCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::setFFTLength);
    connect(mBandwidthEdit, &LineEdit::editingFinished, this, &ResultsView::setBandwidth);
    connect(mThresholdEdit, &LineEdit::editingFinished, this, &ResultsView::setThreshold);

    
    // ------------------------------------
    //  Pagination / Saving
    // ------------------------------------
    mPageWidget = new QWidget();
    mToolsWidget = new QWidget();
    
    mTabPageSaving = new Tabs();
    mTabPageSaving->addTab(tr("Page"));
    mTabPageSaving->addTab(tr("Saving"));
    mTabPageSaving->setTab(0, false);

    connect(mTabPageSaving, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::setPageSavingTab);
    
    // ------------------------------------
    //  Pagination
    // ------------------------------------
    mPageEdit = new LineEdit(mPageWidget);
    mPageEdit->setEnabled(false);
    mPageEdit->setReadOnly(true);
    mPageEdit->setAlignment(Qt::AlignCenter);
    mPageEdit->setText(QString::number(mMaximunNumberOfVisibleGraph));

    mPreviousPageBut = new Button(tr("Prev."), mPageWidget);
    mPreviousPageBut->setCheckable(false);
    mPreviousPageBut->setFlatHorizontal();
    mPreviousPageBut->setToolTip(tr("Display previous data"));
    mPreviousPageBut->setIconOnly(false);

    mNextPageBut  = new Button(tr("Next"), mPageWidget);
    mNextPageBut->setCheckable(false);
    mNextPageBut->setFlatHorizontal();
    mNextPageBut->setToolTip(tr("Display next data"));
    mNextPageBut->setIconOnly(false);

    mGraphsPerPageLab = new Label(tr("Nb Densities / Page"), mPageWidget);
    mGraphsPerPageLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mGraphsPerPageSpin = new QSpinBox(mPageWidget);
    mGraphsPerPageSpin->setRange(1, 100);
    mGraphsPerPageSpin->setValue(mGraphsPerPage);
    mGraphsPerPageSpin->setToolTip(tr("Enter the maximum densities to display on a sheet"));

    connect(mPreviousPageBut, &Button::pressed, this, &ResultsView::previousPage);
    connect(mNextPageBut, &Button::pressed, this, &ResultsView::nextPage);
    connect(mGraphsPerPageSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::updateGraphsPerPage);
    
    // ------------------------------------
    //  Tools Buttons (multiple graphs)
    // ------------------------------------
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

    // ------------------------------------
    //  Tools Buttons (single graph)
    // ------------------------------------
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



    // -------------------------
    connect(mEventsfoldCheck, &CheckBox::clicked, this, &ResultsView::updateScrollAreaRequested);
    connect(mDatesfoldCheck, &CheckBox::clicked, this, &ResultsView::updateScrollAreaRequested);

    connect(this, &ResultsView::updateScrollAreaRequested, this, &ResultsView::setCurrentVariable);
    connect(this, &ResultsView::generateCurvesRequested, this, &ResultsView::generateCurves);
    
    // ---------------------------------------------------------------------------
    //  X Axis Scale
    // ---------------------------------------------------------------------------
    connect(mDisplayStudyBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::setStudyPeriod);
    connect(mCurrentXMinEdit, &LineEdit::editingFinished, this, &ResultsView::editCurrentMinX);
    connect(mCurrentXMaxEdit, &LineEdit::editingFinished, this, &ResultsView::editCurrentMaxX);
    connect(mXSlider, &QSlider::valueChanged, this, &ResultsView::scaleXSliderChanged);
    connect(mXScaleSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ResultsView::scaleXSpinChanged);
    connect(mMajorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this,  &ResultsView::updateScaleX);
    connect(mMinorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this,  &ResultsView::updateScaleX);
    
    connect(this, &ResultsView::xSpinUpdate, this, &ResultsView::setScaleXSlide);
    connect(this, static_cast<void (ResultsView::*)(int)>(&ResultsView::xSpinUpdate), this, &ResultsView::updateZoomX);
    
    connect(mRuler, &Ruler::positionChanged, this, &ResultsView::updateScroll);


    
    // -----
    
    QVBoxLayout* optionsLayout = new QVBoxLayout();
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(0);
    optionsLayout->addWidget(mGraphListTab);
    optionsLayout->addWidget(mResultsGroup);
    optionsLayout->addWidget(mTempoGroup);
    optionsLayout->addWidget(mTabDisplayMCMC);
    optionsLayout->addWidget(mTabDisplay);
    optionsLayout->addWidget(mTabMCMC);
    optionsLayout->addWidget(mTabPageSaving);
    optionsLayout->addWidget(mPageWidget);
    optionsLayout->addWidget(mToolsWidget);
    mOptionsWidget->setLayout(optionsLayout);
    
    
    setGraphListTab(0);
    setDisplayTab(0);
    setPageSavingTab(0);

    updateControls();
    
    applyAppSettings();

    mMarker->raise();
    
    updateGraphTypeOptions();
}

ResultsView::~ResultsView()
{
    mModel = nullptr;
}

#pragma mark Project & Model

void ResultsView::doProjectConnections(Project* project)
{
    /* Starting MCMC calculation does a mModel.clear() at first, and recreate it.
     * Then, it fills its elements (events, ...) with calculated data (trace, ...)
     * If the process is canceled, we only have unfinished data in storage.
     * => The previous nor the new results can be displayed so we must start by clearing the results view! */
    
    setModel(project->mModel);
    connect(project, &Project::mcmcStarted, this, &ResultsView::clearResults);
}

void ResultsView::setModel(Model* model)
{
    mModel = model;
    
    mFFTLenCombo->setCurrentText(QString::number(mModel->getFFTLength()));
    mBandwidthEdit->setText(QString::number(mModel->getBandwidth()));
    mThresholdEdit->setText(QString::number(mModel->getThreshold()));
    
    connect(mModel, &Model::newCalculus, this, &ResultsView::generateCurvesRequested);
}

#pragma mark Layout

void ResultsView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    if (mModel->mProject->withResults()){
         updateControls(); // emit controleUpdated which is connected to updateLayout
    }
}


void ResultsView::updateLayout()
{
    // The scroll bar extent (width or height depending on the orientation)
    // depends on the native platform, and must be taken into account.
    const int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    
    const QFontMetrics fm(font());
    
    int leftWidth = width() - mOptionsW - sbe;
    int graphWidth = leftWidth;
    int tabsH = 40;
    int rulerH = mRuler->height();
    int stackH = height() - tabsH - rulerH;
    
    if (mStatCheck->isChecked() || mTempoStatCheck->isChecked()){
        graphWidth = (2/3) * leftWidth;
    }

    // ----------------------------------------------------------
    //  Left layout
    // ----------------------------------------------------------
    mGraphTypeTabs->setGeometry(0, 0, leftWidth, tabsH);
    mRuler->setGeometry(0, tabsH, graphWidth, rulerH);
    mGraphListStack->setGeometry(0, tabsH + rulerH, leftWidth + sbe, stackH);
    
    mEventsScrollArea->setGeometry(0, 0, leftWidth + sbe, stackH);
    mPhasesScrollArea->setGeometry(0, 0, leftWidth + sbe, stackH);
    mTempoScrollArea->setGeometry(0, 0, leftWidth + sbe, stackH);
    mCurveScrollArea->setGeometry(0, 0, leftWidth + sbe, stackH);
    
    // Update layout of all current graph list
    updateGraphsLayout();

    updateMarkerGeometry(mMarker->pos().x());
    //updateGraphsFont(); // to be moved elsewhere ?
    
    // --------------------------------------------------------
    //  Right layout
    // --------------------------------------------------------
    mOptionsWidget->setGeometry(leftWidth, 0, mOptionsW, height());

    // --------------------------------------------------------
    //  Pagination / Saving Tabs
    // --------------------------------------------------------
    int buttonHeight = 30;
    int paginationLabWidth = 50;
    int paginationButWidth = (mOptionsW - paginationLabWidth) / 2;
    int graphPerPageLabWidth = fm.boundingRect(mGraphsPerPageLab->text()).width();
    int graphPerPageSpinWidth = mOptionsW - 2*mMargin - graphPerPageLabWidth;
    int buttonSide = mOptionsW / 4;
    
    mTabPageSaving->resize(mOptionsW, tabsH);
    mPageWidget->resize(mOptionsW, 2*buttonHeight + mMargin);
    mToolsWidget->resize(mOptionsW, buttonSide);
    
    // --------------------------------------------------------
    //  Pagination layout
    // --------------------------------------------------------
    mPreviousPageBut->setGeometry(0, 0, paginationButWidth, buttonHeight);
    mPageEdit->setGeometry(paginationButWidth, 0, paginationLabWidth, buttonHeight);
    mNextPageBut->setGeometry(paginationButWidth + paginationLabWidth, 0, paginationButWidth, buttonHeight);
    mGraphsPerPageLab->setGeometry(mMargin, buttonHeight + mMargin, graphPerPageLabWidth, buttonHeight);
    mGraphsPerPageSpin->setGeometry(mMargin + graphPerPageLabWidth, buttonHeight + mMargin, graphPerPageSpinWidth, buttonHeight);
    
    // --------------------------------------------------------
    //  Tools layout
    // --------------------------------------------------------
    mImageSaveBut->setGeometry(0, 0, buttonSide, buttonSide);
    mImageClipBut->setGeometry(buttonSide, 0, buttonSide, buttonSide);
    mResultsClipBut->setGeometry(2 * buttonSide, 0, buttonSide, buttonSide);
    mDataSaveBut->setGeometry(3 * buttonSide, 0, buttonSide, buttonSide);
    
    mExportImgBut->setGeometry(0, 0, 2*buttonSide, buttonSide);
    mExportResults->setGeometry(2*buttonSide, 0, 2*buttonSide, buttonSide);
    
    
    

    int ySpan = 0;
    qreal dy(0); // shift between Y position of the Edit and the y position of the label

    switch (mTabDisplayMCMC->currentIndex()) {
        case 0: //Display tab
        {
        /*
         * Span Options
         *
         * */

        mSpanTitle->move(0, 3);

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

        if ((mCurrentTypeGraph != GraphViewResults::eTrace) && (mCurrentTypeGraph != GraphViewResults::eAccept))
            mRuler->clearAreas();

        mCurrentXMinEdit->move(mMargin, ySpan);

        mCurrentXMaxEdit->move(mOptionsW - mCurrentXMinEdit->width() -  mMargin, ySpan );

        const int w (mSpanLab->width());
        mSpanLab->move(int (mOptionsW/2. - (w/2.)), int (mCurrentXMinEdit->y() ));

        ySpan += mMargin + mCurrentXMinEdit->height();


        mXScaleSpin->move(mOptionsW - mXScaleSpin->width() - mMargin, ySpan);

        dy = (mXScaleSpin->height() - labelHeight) /2.;
        mXScaleLab->move(mMargin, int (ySpan +dy));

        const int xSliderWidth = mOptionsW - mXScaleLab->width() - mXScaleSpin->width() - 4*mMargin;

#ifdef Q_OS_MAC
        mXSlider->setGeometry(mXScaleLab->x() + mXScaleLab->width() + mMargin , mXScaleSpin->y(), xSliderWidth, mXSlider->height() );
#endif

#ifdef Q_OS_WIN
        mXSlider->setGeometry(mXScaleLab->x() + mXScaleLab->width() + mMargin , mXScaleSpin->y(), xSliderWidth, mXSlider->height() );
#endif

#ifdef Q_OS_LINUX
        mXSlider->setGeometry(mXScaleLab->x() + mXScaleLab->width() + mMargin , mXScaleSpin->y(), xSliderWidth, mXSlider->height() );
#endif
        ySpan += mXScaleSpin->height() + mMargin;

        if (mCurrentTypeGraph != GraphViewResults::eCorrel) {
            mMajorScaleLab->setVisible(true);
            mMajorScaleEdit->setVisible(true);
            mMinorScaleLab->setVisible(true);
            mMinorScaleEdit->setVisible(true);


            mMajorScaleEdit->move(mOptionsW - mMargin - mMajorScaleEdit->width(), ySpan );
            dy = (mMajorScaleEdit->height() - mMajorScaleLab->height())/2.;
            mMajorScaleLab->move(mOptionsW - 2*mMargin - mMajorScaleEdit->width() - fm.boundingRect(mMajorScaleLab->text()).width(), int (ySpan + dy));

            ySpan += mMajorScaleEdit->height() + mMargin;

            mMinorScaleEdit->move(mOptionsW - mMargin - mMinorScaleEdit->width(), ySpan );
            dy = (mMinorScaleEdit->height() - mMinorScaleLab->height())/2.;
            mMinorScaleLab->move(mOptionsW - 2*mMargin - mMinorScaleEdit->width() - fm.boundingRect(mMinorScaleLab->text()).width(), ySpan);
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
        int ySpan (0);
        mGraphicTitle->move(0, mSpanGroup->y()+ mSpanGroup->height());

        ySpan += mMargin;

        mYScaleSpin->move(mOptionsW - mYScaleSpin->width() - mMargin, ySpan);
        dy = (mYScaleSpin->height() - mYScaleLab->height() ) /2.;
        mYScaleLab->move(mMargin, int (ySpan + dy ) );
        const int ySliderWidth = mOptionsW - mYScaleLab->width() - mYScaleSpin->width() - 4 * mMargin;
#ifdef Q_OS_MAC
        dy = (mYSlider->height() - labelHeight) /2;
        mYSlider->setGeometry(mYScaleLab->x() + mYScaleLab->width() + mMargin, int (mYScaleLab->y() - dy), ySliderWidth, mYSlider->height());
#endif

#ifdef Q_OS_WIN
        dy = (mYScaleSpin->height() -  mYSlider->height()) /2.;
        mYSlider->setGeometry(mYScaleLab->x() + mYScaleLab->width() + mMargin, mYScaleSpin->y(), ySliderWidth, mYSlider->height());
#endif

#ifdef Q_OS_LINUX
       // dy = (mYScaleSpin->height() -  mYSlider->height()) /2.;
        mYSlider->setGeometry(mYScaleLab->x() + mYScaleLab->width() + mMargin, mYScaleSpin->y(), ySliderWidth, mYSlider->height());
#endif
        int maxTextWidth (fm.boundingRect(mLabFont->text()).width() + mMargin);
        int buttonWidth (mOptionsW - maxTextWidth - 2*mMargin);
        ySpan += mMargin + mYScaleSpin->height();
        mFontBut->move(maxTextWidth + mMargin, ySpan );
        mFontBut->setFixedWidth(buttonWidth);
        dy = (mFontBut->height() - mLabFont->height()) /2.;
        mLabFont->move(maxTextWidth - fm.boundingRect(mLabFont->text()).width(), int ( ySpan + dy));

        maxTextWidth = fm.boundingRect(mLabThickness->text()).width() + mMargin;

#ifdef Q_OS_MAC
    const int arrowWidth (12);
#endif

#ifdef Q_OS_WIN
    const int arrowWidth (0);
#endif

#ifdef Q_OS_LINUX
    const int arrowWidth (0);
#endif

        buttonWidth = mThicknessCombo->fontMetrics().boundingRect("999 px").width() + 2*mMargin + arrowWidth;
        ySpan += mMargin + mFontBut->height();
        mThicknessCombo->setFixedWidth(buttonWidth);
        mThicknessCombo->move(mOptionsW - buttonWidth -mMargin,  ySpan);
        dy = (mThicknessCombo->height() - mLabThickness->height()) /2.;
        mLabThickness->move(mThicknessCombo->x() - fm.boundingRect(mLabThickness->text()).width() - mMargin, int (ySpan + dy));

        maxTextWidth = fm.boundingRect(mLabOpacity->text()).width() + mMargin;
        buttonWidth = mOpacityCombo->fontMetrics().boundingRect("9999 %").width() + 2*mMargin + arrowWidth;
        ySpan += mMargin + mThicknessCombo->height();
        mOpacityCombo->setFixedWidth(buttonWidth);
        mOpacityCombo->move(mOptionsW - buttonWidth -mMargin, ySpan);
        dy = (mOpacityCombo->height() - mLabOpacity->height()) /2.;
        mLabOpacity->move(mOpacityCombo->x() - fm.boundingRect(mLabOpacity->text()).width() - mMargin, int (ySpan + dy));

        ySpan += mMargin + mOpacityCombo->height();

        // Fit the size and the position of the widget of the group in the mTabDisplay coordonnate
        mGraphicGroup->setGeometry(0, mGraphicTitle->y() + mGraphicTitle->height() , mOptionsW, ySpan);

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
              const int tabIdx = mGraphTypeTabs->currentIndex();
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
                      mThresholdEdit->setVisible(false);
                      mDensityOptsGroup->setGeometry(0, 0, mOptionsW, 0);

                  } else {
                      mDensityOptsTitle->setVisible(true);
                      mThreshLab->setVisible(true);
                      mThresholdEdit->setVisible(true);

                      mThresholdEdit->move(mOptionsW - mMargin - mThresholdEdit->width(), ySpan);
                      dy = (mThresholdEdit->height() - mThreshLab->height())/2.;
                      mThreshLab->move(mThresholdEdit->x() - fm.boundingRect(mThreshLab->text()).width() - mMargin, int (ySpan + dy));
                      ySpan += mThresholdEdit->height() + mMargin;

                      mFFTLenCombo->move(mOptionsW - mMargin - mFFTLenCombo->width(), ySpan);
                      dy = (mFFTLenCombo->height() - mFFTLenLab->height())/2.;
                      mFFTLenLab->move(mFFTLenCombo->x() - fm.boundingRect(mFFTLenLab->text()).width() - mMargin, int( ySpan + dy));
                      ySpan += mFFTLenCombo->height() + mMargin;

                      mBandwidthEdit->move(mOptionsW - mMargin - mBandwidthEdit->width(), ySpan);
                      dy = (mBandwidthEdit->height() - mBandwidthLab->height())/2.;
                      mBandwidthLab->move(mBandwidthEdit->x() - fm.boundingRect(mBandwidthLab->text()).width() - mMargin, ySpan);
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


void ResultsView::updateGraphsLayout()
{
    if(mGraphListTab->currentIndex() == 0)
    {
        updateGraphsLayout(mEventsScrollArea, mByEventsGraphs);
    }
    else if(mGraphListTab->currentIndex() == 1)
    {
        updateGraphsLayout(mPhasesScrollArea, mByPhasesGraphs);
    }
    else if(mGraphListTab->currentIndex() == 2)
    {
        updateGraphsLayout(mTempoScrollArea, mByTempoGraphs);
    }
    else if(mGraphListTab->currentIndex() == 3)
    {
        updateGraphsLayout(mCurveScrollArea, mByCurveGraphs);
    }
}

void ResultsView::updateGraphsLayout(QScrollArea* scrollArea, QList<GraphViewResults*> graphs)
{
    const int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    QWidget* widget = scrollArea->widget();
    if(widget)
    {
        for (int i=0; i<graphs.size(); ++i)
        {
            graphs[i]->setGeometry(0, i * mGraphHeight, width() - mOptionsW - sbe, mGraphHeight);
            graphs[i]->update();
        }
        widget->setFixedSize(scrollArea->width(), graphs.size() * mGraphHeight);
    }
}

void ResultsView::applyAppSettings()
{
    // ------------------------------------------------------------------------
    //  First, define some variables used in all the layout :
    // ------------------------------------------------------------------------
    const QFont ft (font());
    const QFontMetrics fm (ft);
    
    titleHeight = int ( 1.5 * fm.height());
    labelHeight = int ( fm.height());
    lineEditHeight = int (1.1 * (fm.ascent() + fm.descent())) ;
    checkBoxHeight  = int ( fm.height());
    comboBoxHeight  = mThicknessCombo->height(); //int (0.3 * AppSettings::heigthUnit());
    radioButtonHeight = int ( fm.height());
    spinBoxHeight = mXScaleSpin->height();
    buttonHeight =  int (1.7 * (fm.ascent() + fm.descent()));//int (0.5 * AppSettings::heigthUnit());
    GraphViewResults::mHeightForVisibleAxis = int (4 * AppSettings::heigthUnit());

    // ------------------------------------------------------------------------
    //  Left part of the view : tabs, ruler and graphs
    // ------------------------------------------------------------------------
    mRuler->setMarginBottom(fm.ascent() * 2.0);
    
    mGraphHeight = GraphViewResults::mHeightForVisibleAxis ; // same value in ResultsView::updateScaleY(int value)

    const int wEdit = int (ceil((mOptionsW - 4 * mMargin)/3.));
    const QSize allDensitiesButSize (mOptionsW/2, mOptionsW/4);
    const QSize singleDensityButSize (mOptionsW/4, mOptionsW/4);


    mOptionsWidget->setFixedWidth(mOptionsW);

    mPageWidget->resize(mOptionsW, mRuler->height());
     /* -------------------------------------- mResultsGroup---------------------------------------------------*/

     /* - mTabDisplayMCMC */

     /*  Display Options layout */

    mXScaleSpin->setLocale(QLocale(AppSettings::mLanguage, AppSettings::mCountry));

    mSpanTitle->setFixedSize(mOptionsW, titleHeight);
    mDisplayStudyBut->setFixedSize(mOptionsW - 2*mMargin, buttonHeight);
    mSpanLab->setFixedSize( fm.boundingRect(mSpanLab->text()).width(), labelHeight);

    mCurrentXMinEdit->setFixedSize(wEdit, lineEditHeight);
    mCurrentXMaxEdit->setFixedSize(wEdit, lineEditHeight);
    mXScaleLab->setFixedWidth( int (fm.width(mXScaleLab->text())));
    mXScaleSpin->setFixedSize(wEdit, spinBoxHeight);

    mMajorScaleLab->setFixedSize(mOptionsW - wEdit - 2* mMargin, labelHeight);
    mMajorScaleEdit->setFixedSize(wEdit, lineEditHeight);

    mMinorScaleLab->setFixedSize(mOptionsW - wEdit - 2* mMargin, labelHeight);
    mMinorScaleEdit->setFixedSize(wEdit, lineEditHeight);

    /* -------------------------------------- Graphic Options (old mDisplayGroup) ---------------------------------------------------*/
    mGraphicTitle->setFixedSize(mOptionsW, titleHeight);
    mYScaleLab->setFixedSize(fm.boundingRect(mYScaleLab->text()).width(), labelHeight);

    mYScaleSpin->setFixedSize(mCurrentXMinEdit->width(), spinBoxHeight);
    mLabFont->setFixedSize(fm.boundingRect(mLabFont->text()).width(), labelHeight);

    mFontBut->setFixedSize(mOptionsW - fm.boundingRect(mLabFont->text()).width()  - mMargin, buttonHeight);
    mLabThickness->setFixedSize(fm.boundingRect(mLabThickness->text()).width(), comboBoxHeight);


#ifdef Q_OS_MAC
    const int arrowWidth = 12;
#endif

#ifdef Q_OS_WIN
    const int arrowWidth = 0;
#endif

#ifdef Q_OS_LINUX
    const int arrowWidth = 0;
#endif

    mThicknessCombo->setFixedSize( fm.boundingRect("999 px").width() + 2*mMargin + arrowWidth, comboBoxHeight);
    mLabOpacity->setFixedSize(mOptionsW/2 - mMargin, labelHeight);
    mOpacityCombo->setFixedSize(fm.boundingRect("9999 %").width() + 2*mMargin, comboBoxHeight);

     /* -------------------------------------- mChainsGroup---------------------------------------------------*/

    mAllChainsCheck->setFixedSize(mOptionsW - 2*mMargin, checkBoxHeight);

    mChainsTitle->setFixedSize(mOptionsW, titleHeight);
    mAllChainsCheck->setFixedSize(mOptionsW - 2*mMargin, checkBoxHeight);

    if (mCheckChainChecks.isEmpty())
           for (auto check : mCheckChainChecks)
                check->setFixedSize(mOptionsW - 2*mMargin, checkBoxHeight);

    if (mChainRadios.isEmpty())
           for (auto radio : mChainRadios)
                radio->setFixedSize(mOptionsW - 2*mMargin, radioButtonHeight);


    // ------------------------------------
    //  Density Options
    // ------------------------------------
    mDensityOptsTitle->setFixedSize(mOptionsW, titleHeight);
    mCredibilityCheck->setFixedSize(int(mOptionsW - 2*mMargin), checkBoxHeight);
    mThreshLab->setFixedSize(fm.boundingRect(mThreshLab->text()).width(), lineEditHeight);
    mThresholdEdit->setFixedSize(wEdit, lineEditHeight);
    mFFTLenLab->setFixedSize(fm.boundingRect(mFFTLenLab->text()).width(), comboBoxHeight);
    mFFTLenCombo->setFixedSize(mOptionsW/2 - mMargin, comboBoxHeight);
    mBandwidthLab->setFixedSize(fm.boundingRect(mBandwidthLab->text()).width(), lineEditHeight);
    mBandwidthEdit->setFixedSize(wEdit, lineEditHeight);

    // set the variable and the graphic type
    mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    mCurrentVariable = GraphViewResults::eTheta;
    
    if (mHasPhases) {
        mGraphListTab->setTab(1, false);
        mGraphListTab->setTabVisible(1, true);
        mGraphListTab->setTabVisible(2, true);
        mGraphTypeTabs->setTab(0, false);

     } else {
        mGraphListTab->setTabVisible(2, false);
        mGraphListTab->setTabVisible(1, false);
        mGraphListTab->setTab(0, false);
        mGraphTypeTabs->setTab(0, false);
     }

}

/**
 * @brief ResultsView::updateControls set controls according to the differents tabs positions, and calls updateLayout
*/
void ResultsView::updateControls()
{
    bool isPostDistrib = (mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    
    // -------------------------------------------------------------------------------------
    //  Update controls depending on current graph list
    // -------------------------------------------------------------------------------------
    if(mGraphListTab->currentIndex() == 0)
    {
        mGraphTypeTabs->setTabVisible(1, true); // History Plot
        mGraphTypeTabs->setTabVisible(2, true); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, true); // Autocorrelation
        
        mResultsGroup->setVisible(true);
        mTempoGroup->setVisible(false);
        
        mEventsfoldCheck->setVisible(false);
        
        bool showCalibControl = isPostDistrib && mDataThetaRadio->isChecked() && mDatesfoldCheck->isChecked();
        
        mDataCalibCheck->setVisible(showCalibControl);
        mWiggleCheck->setVisible(showCalibControl);
    }
    else if(mGraphListTab->currentIndex() == 1)
    {
        mGraphTypeTabs->setTabVisible(1, true); // History Plot
        mGraphTypeTabs->setTabVisible(2, true); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, true); // Autocorrelation
        
        mResultsGroup->setVisible(true);
        mTempoGroup->setVisible(false);
        
        mEventsfoldCheck->setVisible(true);
        mDatesfoldCheck->setEnabled(mEventsfoldCheck->isChecked());
        
        if(!mEventsfoldCheck->isChecked())
        {
            mDatesfoldCheck->setChecked(false);
        }
        
        bool showCalibControl = isPostDistrib && mEventsfoldCheck->isChecked() && mDatesfoldCheck->isChecked() && mDataThetaRadio->isChecked();
        
        mDataCalibCheck->setVisible(showCalibControl);
        mWiggleCheck->setVisible(showCalibControl);
    }
    else if(mGraphListTab->currentIndex() == 2)
    {
        mGraphTypeTabs->setTabVisible(1, false); // History Plot
        mGraphTypeTabs->setTabVisible(2, false); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, false); // Autocorrelation
        mGraphTypeTabs->setTab(0, true);
        
        mResultsGroup->setVisible(false);
        mTempoGroup->setVisible(true);
        
        mTempoCredCheck->setVisible(mTempoRadio->isChecked());
        mTempoErrCheck->setVisible(mTempoRadio->isChecked());
    }
    
    // -------------------------------------------------------------------------------------
    //  Update controls depending on current display tab
    // -------------------------------------------------------------------------------------
    mTabDisplay->setVisible(mTabDisplayMCMC->currentIndex() == 0);
    mTabMCMC->setVisible(mTabDisplayMCMC->currentIndex() == 1);
    
    if (mCurrentVariable == GraphViewResults::eTheta
        || mCurrentVariable == GraphViewResults::eTempo
        || mCurrentVariable == GraphViewResults::eActivity)
    {
        mDisplayStudyBut->setText(tr("Study Period Display"));
        mDisplayStudyBut->setVisible(true);
    }
    else if (mCurrentVariable == GraphViewResults::eSigma || (mCurrentVariable == GraphViewResults::eDuration))
    {
        mDisplayStudyBut->setText(tr("Fit Display"));
        mDisplayStudyBut->setVisible(true);
    }
    else{
        mDisplayStudyBut->setVisible(false);
    }
    
    // -------------------------------------------------------------------------------------
    //  MCMC Display options are not visible for mCurrentVariable = Tempo or Activity
    // -------------------------------------------------------------------------------------
    if(mCurrentVariable == GraphViewResults::eTempo || mCurrentVariable == GraphViewResults::eActivity)
    {
        mTabDisplayMCMC->setTabVisible(1, false);
        mTabDisplayMCMC->setTab(0, false);
    }
    else
    {
        mTabDisplayMCMC->setTabVisible(1, true);
    }
    
    // -------------------------------------------------------------------------------------
    //  Pagination options
    // -------------------------------------------------------------------------------------
    bool showPagination = (mTabPageSaving->currentIndex() == 0);
    
    mPageWidget->setVisible(showPagination);
    mToolsWidget->setVisible(!showPagination);
    
    QList<GraphViewResults*> graphsSelected = currentGraphs(true);
    bool hasSelection = (graphsSelected.size() > 0);
    
    mImageSaveBut->setVisible(hasSelection);
    mImageClipBut->setVisible(hasSelection);
    mResultsClipBut->setVisible(hasSelection);
    mDataSaveBut->setVisible(hasSelection);
    
    mExportImgBut->setVisible(!hasSelection);
    mExportResults->setVisible(!hasSelection);
    

    // -------------------------------------------------------------------------------------
    //  To be moved ??
    // -------------------------------------------------------------------------------------
    QList<GraphViewResults*> graphs = currentGraphs(hasSelection);
    
    mCurrentPage = 0;
    mMaximunNumberOfVisibleGraph = graphs.size();
    
    updateLayout();
}

void ResultsView::updateGraphTypeOptions()
{
    // ------------------------------------
    //  MCMC Chains
    //  Switch between checkBoxes or Radio-buttons for chains
    // ------------------------------------
    bool isPostDistrib = (mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    
    mAllChainsCheck->setVisible(isPostDistrib);
    
    for(auto &&checkChain : mCheckChainChecks){
        checkChain->setVisible(isPostDistrib);
    }
    
    for(auto &&chainRadio : mChainRadios){
        chainRadio->setVisible(!isPostDistrib);
    }

    // ------------------------------------
    //  Density Options
    // ------------------------------------
    mDensityOptsGroup->setVisible(isPostDistrib && (mCurrentVariable != GraphViewResults::eTempo && mCurrentVariable != GraphViewResults::eActivity));
    
    mCredibilityCheck->setVisible(isPostDistrib);
    mThreshLab->setVisible(isPostDistrib);
    mFFTLenLab->setVisible(isPostDistrib);
    mBandwidthLab->setVisible(isPostDistrib);
    
    // ------------------------------------
    //  Display Options
    // ------------------------------------
    if(isPostDistrib)
    {
        QString fitText = tr("Fit Display");
        if(mCurrentVariable == GraphViewResults::eTheta || mCurrentVariable == GraphViewResults::eTempo || mCurrentVariable == GraphViewResults::eActivity) {
            fitText = tr("Study Period Display");
        }
        mDisplayStudyBut->setText(fitText);
    }
}

#pragma mark Tabs changes listeners

/**
 * This is called when mGraphListTab is clicked.
 * MCMC display options are only available for mCurrentVariable in : eTheta, eSigma, eDuration
 * so mTabDisplayMCMC must be displayed accordingly
 */
void ResultsView::setGraphListTab(int tabIndex)
{
    if (tabIndex == 2 || tabIndex == 3)
    {
        mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    }
    
    createGraphs();
}

void ResultsView::setCurrentVariable()
{
    if (mDataThetaRadio->isChecked())
    {
        mCurrentVariable = GraphViewResults::eTheta;
    }
    else if (mDataSigmaRadio->isChecked())
    {
        mCurrentVariable = GraphViewResults::eSigma;
    }
    else if (mDurationRadio->isChecked())
    {
        mCurrentVariable = GraphViewResults::eDuration;
    }
    else if (mTempoRadio->isChecked())
    {
        mCurrentVariable = GraphViewResults::eTempo;
    }
    else if (mActivityRadio->isChecked())
    {
        mCurrentVariable = GraphViewResults::eActivity;
    }
    
    createGraphs();
}

void ResultsView::setDisplayTab(int tabIndex)
{
    updateControls();
}

void ResultsView::setPageSavingTab(int tabIndex)
{
    updateControls();
}

#pragma mark Controls listener

void ResultsView::clearResults()
{
     if (mModel->mChains.size() != mCheckChainChecks.size() )
     {
        for (auto &&check : mCheckChainChecks )
            delete check;

        mCheckChainChecks.clear();

        for (auto &&chain : mChainRadios)
            delete chain;

        mChainRadios.clear();
     }

    deleteAllGraphsInList(mByEventsGraphs);
    deleteAllGraphsInList(mByPhasesGraphs);
    deleteAllGraphsInList(mByTempoGraphs);
    deleteAllGraphsInList(mByCurveGraphs);
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

    clearResults();

    QFontMetricsF gfm(font());
    QLocale locale = QLocale();
    mMarginLeft = 0;
    Scale xScale;
    xScale.findOptimal(mModel->mSettings.mTmin, mModel->mSettings.mTmax, 7);

    mMajorScale = xScale.mark;
    mMinorCountScale = 4;

    mRuler->setRange(mModel->mSettings.getTminFormated(), mModel->mSettings.getTmaxFormated());
    mRuler->setCurrent(mModel->mSettings.getTminFormated(), mModel->mSettings.getTmaxFormated());
    mRuler->setScaleDivision(mMajorScale, mMinorCountScale);

    mMajorScaleEdit->setText(locale.toString(mMajorScale));
    mMinorScaleEdit->setText(locale.toString(mMinorCountScale));

    mHasPhases = (mModel->mPhases.size() > 0);

    // set the variable and the graphic type

    if (mHasPhases) {
        mGraphListTab->setTabVisible(1, true); // Phases
        mGraphListTab->setTabVisible(2, true); // Tempo
        mGraphListTab->setTab(1, false);        // Phases

     } else {
        mGraphListTab->setTabVisible(1, false); // Phases
        mGraphListTab->setTabVisible(2, false); // Tempo
        mGraphListTab->setTab(0, false);        // Events
     }



    /* ----------------------------------------------------
     *  Create Chains option controls (radio and checkboxes under "MCMC Chains")
     * ---------------------------------------------------- */
    if (mCheckChainChecks.isEmpty()) {
        for (int i=0; i<mModel->mChains.size(); ++i) {
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
    
    mModel->initDensities();
    setStudyPeriod();

    showInfos(false);

    //mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    mCurrentVariable = GraphViewResults::eTheta;
    mGraphTypeTabs->tabClicked(-1);
    
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

    /* ----------------------------------------------------
    *  Update Chains option controls (radio and checkboxes under "MCMC Chains")
    * ---------------------------------------------------- */

    if (mCheckChainChecks.isEmpty()) {
        for (int i = 0; i<mModel->mChains.size(); ++i) {
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

        for (int i = 0; i<mModel->mChains.size(); ++i) {
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

    mModel->updateDensities();
    
    createGraphs();
}

#pragma mark Re-create graphs

void ResultsView::createGraphs()
{
    if(!mModel){
        return;
    }
    
    if (mGraphListTab->currentIndex() == 0)
    {
        createByEventsGraphs();
    }
    else if (mGraphListTab->currentIndex() == 1)
    {
        createByPhasesGraphs();
    }
    else if (mGraphListTab->currentIndex() == 2)
    {
        createByTempoGraphs();
    }
    else if (mGraphListTab->currentIndex() == 3)
    {
        createByCurveGraph();
    }
}

/**
 * this method (re-)creates all the events graphs
 */
void ResultsView::createByEventsGraphs()
{
    Q_ASSERT(mModel);
    
    // ----------------------------------------------------------------------
    //  Disconnect and delete existing graphs
    // ----------------------------------------------------------------------
    deleteAllGraphsInList(mByEventsGraphs);
    
    // ----------------------------------------------------------------------
    // Show all events unless at least one is selected
    // ----------------------------------------------------------------------
    bool showAllEvents = ! mModel->hasSelectedEvents();

    // ----------------------------------------------------------------------
    //  Iterate through all events and create corresponding graphs
    // ----------------------------------------------------------------------
    QWidget* eventsWidget = mEventsScrollArea->widget();
    int graphIndex = 0;
    
    for(int i=0; i<mModel->mEvents.size(); ++i)
    {
        Event* event = mModel->mEvents[i];
        if(event->mIsSelected || showAllEvents)
        {
            if(graphIndexIsInCurrentPage(graphIndex))
            {
                GraphViewEvent* graphEvent = new GraphViewEvent(eventsWidget);
                graphEvent->setSettings(mModel->mSettings);
                graphEvent->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                graphEvent->setEvent(event);
                graphEvent->setGraphFont(font());
                graphEvent->setGraphsThickness(mThicknessCombo->currentIndex());
                graphEvent->changeXScaleDivision(mMajorScale, mMinorCountScale);
                
                mByEventsGraphs.append(graphEvent);
                connect(graphEvent, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                ++graphIndex;
                
                if(mDatesfoldCheck->isChecked())
                {
                    for (int j=0; j<event->mDates.size(); ++j)
                    {
                        if(graphIndexIsInCurrentPage(graphIndex))
                        {
                            Date& date = event->mDates[j];
                            
                            GraphViewDate* graphDate = new GraphViewDate(eventsWidget);
                            graphDate->setSettings(mModel->mSettings);
                            graphDate->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                            graphDate->setDate(&date);
                            graphDate->setGraphFont(font());
                            graphDate->setGraphsThickness(mThicknessCombo->currentIndex());
                            graphDate->changeXScaleDivision(mMajorScale, mMinorCountScale);
                            graphDate->setColor(event->mColor);
                            graphDate->setGraphsOpacity(mOpacityCombo->currentIndex()*10);

                            mByEventsGraphs.append(graphDate);
                            connect(graphDate, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                            ++graphIndex;
                        }
                    }
                }
            }
        }
    }

    mEventsScrollArea->update();
    emit generateCurvesRequested();
}

void ResultsView::createByPhasesGraphs()
{
    Q_ASSERT(mModel);
    
    // ----------------------------------------------------------------------
    //  Disconnect and delete existing graphs
    // ----------------------------------------------------------------------
    deleteAllGraphsInList(mByPhasesGraphs);
    
    // ----------------------------------------------------------------------
    // Show all, unless at least one is selected
    // ----------------------------------------------------------------------
    bool showAllPhases = ! mModel->hasSelectedPhases();

    // ----------------------------------------------------------------------
    //  Iterate through all, and create corresponding graphs
    // ----------------------------------------------------------------------
    QWidget* phasesWidget = mPhasesScrollArea->widget();
    int graphIndex = 0;
    
    for(int i=0; i<mModel->mPhases.size(); ++i)
    {
        Phase* phase = mModel->mPhases[i];
        if(phase->mIsSelected || showAllPhases)
        {
            if(graphIndexIsInCurrentPage(graphIndex))
            {
                GraphViewPhase* graph = new GraphViewPhase(phasesWidget);
                graph->setSettings(mModel->mSettings);
                graph->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                graph->setPhase(phase);
                graph->setGraphFont(font());
                graph->setGraphsThickness(mThicknessCombo->currentIndex());
                graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                
                mByPhasesGraphs.append(graph);
                connect(graph, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                ++graphIndex;
            }
            
            if(mEventsfoldCheck->isChecked())
            {
                for(int j=0; j<phase->mEvents.size(); ++j)
                {
                    Event* event = phase->mEvents[j];
                    
                    if(graphIndexIsInCurrentPage(graphIndex))
                    {
                        GraphViewEvent* graph = new GraphViewEvent(phasesWidget);
                        graph->setSettings(mModel->mSettings);
                        graph->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                        graph->setEvent(event);
                        graph->setGraphFont(font());
                        graph->setGraphsThickness(mThicknessCombo->currentIndex());
                        graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                        
                        mByPhasesGraphs.append(graph);
                        connect(graph, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                        ++graphIndex;
                    }
                        
                    if(mDatesfoldCheck->isChecked())
                    {
                        for (int k=0; k<event->mDates.size(); ++k)
                        {
                            if(graphIndexIsInCurrentPage(graphIndex))
                            {
                                Date& date = event->mDates[k];
                                
                                GraphViewDate* graph = new GraphViewDate(phasesWidget);
                                graph->setSettings(mModel->mSettings);
                                graph->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                                graph->setDate(&date);
                                graph->setGraphFont(font());
                                graph->setGraphsThickness(mThicknessCombo->currentIndex());
                                graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                                graph->setColor(event->mColor);
                                graph->setGraphsOpacity(mOpacityCombo->currentIndex()*10);

                                mByPhasesGraphs.append(graph);
                                connect(graph, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                                ++graphIndex;
                            }
                        }
                    }
                }
            }
        }
    }

    mPhasesScrollArea->update();
    emit generateCurvesRequested();
}

void ResultsView::createByTempoGraphs()
{
    Q_ASSERT(mModel);
    
    // ----------------------------------------------------------------------
    //  Disconnect and delete existing graphs
    // ----------------------------------------------------------------------
    deleteAllGraphsInList(mByTempoGraphs);
    
    // ----------------------------------------------------------------------
    // Show all, unless at least one is selected
    // ----------------------------------------------------------------------
    bool showAllPhases = ! mModel->hasSelectedPhases();

    // ----------------------------------------------------------------------
    //  Iterate through all, and create corresponding graphs
    // ----------------------------------------------------------------------
    QWidget* tempoWidget = mTempoScrollArea->widget();
    int graphIndex = 0;
    
    for(int i=0; i<mModel->mPhases.size(); ++i)
    {
        Phase* phase = mModel->mPhases[i];
        if(phase->mIsSelected || showAllPhases)
        {
            if(graphIndexIsInCurrentPage(graphIndex))
            {
                GraphViewTempo* graph = new GraphViewTempo(tempoWidget);
                graph->setSettings(mModel->mSettings);
                graph->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                graph->setPhase(phase);
                graph->setGraphFont(font());
                graph->setGraphsThickness(mThicknessCombo->currentIndex());
                graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                
                mByTempoGraphs.append(graph);
                connect(graph, &GraphViewResults::selected, this, &ResultsView::updateLayout);
                ++graphIndex;
            }
        }
    }

    mTempoScrollArea->update();
    emit generateCurvesRequested();
}

void ResultsView::createByCurveGraph()
{
    Q_ASSERT(mModel);
    
    // ----------------------------------------------------------------------
    //  Disconnect and delete existing graphs
    // ----------------------------------------------------------------------
    deleteAllGraphsInList(mByCurveGraphs);
    
    QWidget* widget = mCurveScrollArea->widget();

    GraphViewTempo* graphAlpha = new GraphViewTempo(widget);
    graphAlpha->setSettings(mModel->mSettings);
    graphAlpha->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
    graphAlpha->setGraphFont(font());
    graphAlpha->setGraphsThickness(mThicknessCombo->currentIndex());
    graphAlpha->changeXScaleDivision(mMajorScale, mMinorCountScale);
    mByCurveGraphs.append(graphAlpha);

    mCurveScrollArea->update();
    emit generateCurvesRequested();
}



#pragma mark Utilities to re-create Graphs

void ResultsView::deleteAllGraphsInList(QList<GraphViewResults*>& list)
{
    for(auto && graph : list){
        disconnect(graph, &GraphViewResults::selected, this, &ResultsView::updateLayout);
        delete graph;
    }
    list.clear();
}

#pragma mark FFTLength, Threshold, Bandwidth

void ResultsView::setFFTLength()
{
    const int len = mFFTLenCombo->currentText().toInt();
    mModel->setFFTLength(len);
}

/**
* @brief ResultsView::setBandwidth The control of the value is done with a QValidator set in the constructor
*/
void ResultsView::setBandwidth()
{
    const double bandwidth = locale().toDouble(mBandwidthEdit->text());
    mModel->setBandwidth(bandwidth);
}

/**
 * @brief ResultsView::setThreshold The control of the value is done with a QValidator set in the constructor
 */
void ResultsView::setThreshold()
{
    const double hpd = locale().toDouble(mThresholdEdit->text());
    mModel->setThreshold(hpd);
}

#pragma mark Pagination

void ResultsView::nextPage()
{
    if((mCurrentPage + 1) * mGraphsPerPage < mMaximunNumberOfVisibleGraph)
    {
        ++mCurrentPage;
        mPageEdit->setText(locale().toString(mCurrentPage + 1) + "/" + locale().toString(ceil(mMaximunNumberOfVisibleGraph/mGraphsPerPage)));
        emit updateScrollAreaRequested();
    }
}

void ResultsView::previousPage()
{
    if(mCurrentPage > 0)
    {
        --mCurrentPage;
        mPageEdit->setText(locale().toString(mCurrentPage + 1) + "/" + locale().toString(ceil(mMaximunNumberOfVisibleGraph/mGraphsPerPage)));
        emit updateScrollAreaRequested();
    }
}

void ResultsView::updateGraphsPerPage(int graphsPerPage)
{
   mGraphsPerPage = graphsPerPage;
   updateControls();
   emit updateScrollAreaRequested();
}


bool ResultsView::graphIndexIsInCurrentPage(int graphIndex)
{
    int firstIndexToShow = mCurrentPage * mGraphsPerPage;
    return (graphIndex >= firstIndexToShow) && (graphIndex < (firstIndexToShow + mGraphsPerPage));
}


#pragma mark Tabs navigation

void ResultsView::graphTypeChange()
{
    mCurrentTypeGraph = (GraphViewResults::TypeGraph) mGraphTypeTabs->currentIndex();
    updateGraphTypeOptions();
    emit generateCurvesRequested();
}

#pragma mark Curves generation

/**
*  @brief re-generate all curves in graph views form model data.
*  @brief Each curve is given a name. This name will be used by updateCurvesToShow() to decide whether the curve is visible or not.
*  @param listGraphs is the list of existings graph for which we want to generate curves. It may be mByEventsGraphs, mByPhasesGraphs, etc...
*  Depending on the selected tab, options may differ. For example, we don't have the same display options for events and durations !
*  We thus have to check which tab is selected, to gather the corresponding options, and generate the curves.
*/
void ResultsView::generateCurves()
{
    QList<GraphViewResults*> listGraphs;
    
    // -----------------------------------------------------------------
    //  The "Events" tab is selected
    // -----------------------------------------------------------------
    if(mGraphListTab->currentIndex() == 0)
    {
        listGraphs = mByEventsGraphs;
        
        if(mDataThetaRadio->isChecked()){
            mCurrentVariable = GraphViewResults::eTheta;
        }else if (mDataSigmaRadio->isChecked()){
            mCurrentVariable = GraphViewResults::eSigma;
        }
    }
    // -----------------------------------------------------------------
    //  The "Phases" tab is selected
    // -----------------------------------------------------------------
    else if (mGraphListTab->currentIndex() == 1)
    {
        listGraphs = mByPhasesGraphs;
        
        if(mDataThetaRadio->isChecked()){
            mCurrentVariable = GraphViewResults::eTheta;
        }else if (mDataSigmaRadio->isChecked()){
            mCurrentVariable = GraphViewResults::eSigma;
        }
    }
    // -----------------------------------------------------------------
    //  The "Duration" tab is selected
    // -----------------------------------------------------------------
    else if (mGraphListTab->currentIndex() == 2)
    {
        listGraphs = mByTempoGraphs;
        
        if (mDurationRadio->isChecked()){
            mCurrentVariable = GraphViewResults::eDuration;
        }
        else if (mTempoRadio->isChecked()){
            mCurrentVariable = GraphViewResults::eTempo;
        }
        else if (mActivityRadio->isChecked()){
            mCurrentVariable = GraphViewResults::eActivity;
        }
    }
    // -----------------------------------------------------------------
    //  The "Curves" tab is selected
    // -----------------------------------------------------------------
    else if (mGraphListTab->currentIndex() == 3)
    {
        listGraphs = mByCurveGraphs;
    }

    // -----------------------------------------------------------------
    //  Generate all graphs curves in the current list
    // -----------------------------------------------------------------
    for(auto &&graph : listGraphs)
    {
        graph->generateCurves(GraphViewResults::TypeGraph(mCurrentTypeGraph), mCurrentVariable);
    }
    
    // -----------------------------------------------------------------
    // With variable eDuration, we look for mResultMaxDuration in the curve named "Post Distrib All Chains"
    // A simplifier / factoriser ?
    // -----------------------------------------------------------------
    if (mCurrentVariable == GraphViewResults::eDuration)
    {
        mResultMaxDuration = 0.;

        QList<GraphViewResults*>::const_iterator constIter;
        constIter = listGraphs.cbegin();
        QList<GraphViewResults*>::const_iterator iterEnd = listGraphs.cend();
        while (constIter != iterEnd)
        {
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

    // -----------------------------------------------------------------
    // With variable eSigma, we look for mResultMaxVariance in the curve named "Post Distrib All Chains"
    // A simplifier / factoriser ?
    // -----------------------------------------------------------------
    if (mCurrentVariable == GraphViewResults::eSigma)
    {
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
    
    updateCurvesToShow();
}

/**
 *  @brief Decide which curve graphs must be show, based on currently selected options.
 *  @brief This function does NOT remove or create any curve in graphs! It only checks if existing curves should be visible or not.
 */
void ResultsView::updateCurvesToShow()
{
    // --------------------------------------------------------
    //  Gather selected options
    // --------------------------------------------------------
    bool showAllChains = mAllChainsCheck->isChecked();
    bool showCalib = mDataCalibCheck->isChecked();
    bool showWiggle = mWiggleCheck->isChecked();
    bool showCredibility = mCredibilityCheck->isChecked();
    bool showStat = mStatCheck->isChecked();
    
    if(mCurrentVariable == GraphViewResults::eTempo){
        showCalib = mTempoErrCheck->isChecked();
        showCredibility = mTempoCredCheck->isChecked();
    }
    
    // --------------------------------------------------------
    //  showChainList is a list of booleans describing which chains are visible or not.
    //  For Post distribs, multiple chains can be visible at once (checkboxes)
    //  In other cases, only one chain can be displayed (radios)
    // --------------------------------------------------------
    QList<bool> showChainList;
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib)
    {
        for (CheckBox* cbButton : mCheckChainChecks){
            showChainList.append(cbButton->isChecked());
        }
    }
    else
    {
        for (RadioButton* rButton : mChainRadios){
            showChainList.append(rButton->isChecked());
        }
    }
    
    // --------------------------------------------------------
    //  Find the currently selected list of graphs
    // --------------------------------------------------------
    QList<GraphViewResults*> listGraphs;

    if(mGraphListTab->currentIndex() == 0)
    {
        listGraphs = mByEventsGraphs;
    }
    else if(mGraphListTab->currentIndex() == 1)
    {
        listGraphs = mByPhasesGraphs;
    }
    else if(mGraphListTab->currentIndex() == 2)
    {
        listGraphs = mByTempoGraphs;
    }
    else if(mGraphListTab->currentIndex() == 3)
    {
        listGraphs = mByCurveGraphs;
    }
    
    // --------------------------------------------------------
    //  Update Graphs with selected options
    // --------------------------------------------------------
    for (GraphViewResults* graph : listGraphs)
    {
        graph->setShowNumericalResults(showStat);
        graph->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
    }

    updateScales();
}

/**
 *  @brief Restore with mZooms and mScales according to mGraphTypeTabs
 *  which are store in updateGraphsZoomX()
 */
void ResultsView::updateScales()
{
    int tabIdx = mGraphTypeTabs->currentIndex();
    ProjectSettings s = mModel->mSettings;

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
                const ChainSpecs& chain = mModel->mChains.at(i);
                mResultMaxX = 1 + chain.mNumBurnIter + (chain.mBatchIndex * chain.mNumBatchIter) + chain.mNumRunIter / chain.mThinningInterval;
                break;
            }
        }
        mRuler->setRange(mResultMinX, mResultMaxX);
        mRuler->setFormatFunctX(nullptr);

        const int rangeZoom = int (mResultMaxX / 100);
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
                const ChainSpecs& chain = mModel->mChains.at(i);
                int adaptSize = chain.mBatchIndex * chain.mNumBatchIter;
                int runSize = int (chain.mNumRunIter / chain.mThinningInterval);

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

    if (mGraphListTab->currentIndex() == 0) {
        for (GraphViewResults* allGraph : mByEventsGraphs) {
            allGraph->setRange(mRuler->mMin, mRuler->mMax);
            allGraph->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
            allGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
            allGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
        }

    } else if (mGraphListTab->currentIndex() == 1) {
        for (GraphViewResults* allGraph : mByPhasesGraphs) {
            allGraph->setRange(mRuler->mMin, mRuler->mMax);
            allGraph->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
            allGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
            allGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
        }

    } else if (mGraphListTab->currentIndex() == 2) {
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
    setScaleXSlide( zoomToSlider(mResultZoomX));

    forceXSpinSetValue = true;
    setScaleXSpin(mResultZoomX);

    updateZoomEdit();
    updateScaleEdit();

    // Already done when setting graphs new range (above)
    //updateGraphsZoomX();

    updateControls();
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
    updateMarkerGeometry(e->pos().x());
}

void ResultsView::updateMarkerGeometry(const int x)
{
    const int markerXPos = inRange(0, x, mRuler->x() + mRuler->width());
    mMarker->setGeometry(markerXPos, mGraphTypeTabs->height() + mRuler->height(), mMarker->thickness(), height() - mGraphTypeTabs->height() - mRuler->height());
}



/**
 * @brief ResultsView::scaleXSliderChanged
 * @param value
 * connected to mXSlider, &QSlider::valueChanged(value)
 */
void ResultsView::scaleXSliderChanged(int value)
{
    if (!forceXSlideSetValue) {
        forceXSpinSetValue = true;
        setScaleXSpin(sliderToZoom(value));
        updateZoomX();
    }
}

/**
 * @brief ResultsView::scaleXSpinChanged slot connected to mXScaleSpin->setValue()
 * @param value
 */
void ResultsView::scaleXSpinChanged(double value)
{
    if (!forceXSpinSetValue) {
        forceXSlideSetValue = true;
        setScaleXSlide(zoomToSlider(value));
        updateZoomX();
    }
}

/**
 * @brief ResultsView::setScaleXSpin
 * Slot to control, if we want to force just set the value, or if we want to propagate the signal throw xSinUpdate
 * @param value
 */
void ResultsView::setScaleXSpin(const double value)
{
    mXScaleSpin->setValue(value);
    if (!forceXSpinSetValue)
        emit xSpinUpdate(int(value));
    else
        forceXSpinSetValue = false;
}

/**
 * @brief ResultsView::setScaleXSlide
 * Slot to control, if we want to force the just set the value, or if we want to propagate the signal throw xSlideUpdate
 * @param value
 */
void ResultsView::setScaleXSlide(const int value)
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
         return pow(10., double (coef/100.));
    else
        return coef;
}

int ResultsView::zoomToSlider(const double &zoom)
{
   if (mCurrentTypeGraph == GraphViewResults::ePostDistrib)
        return int (round(log10(zoom) * (100.)));
   else
       return int (zoom);
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
        mMinorCountScale =  int (aNumber);

        mRuler->setScaleDivision(mMajorScale, mMinorCountScale);
        QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph> situ (mCurrentVariable, mCurrentTypeGraph);

        mScales[situ] = QPair<double, int>(mMajorScale, mMinorCountScale);

        if (mGraphListTab->currentIndex() == 0) {
            for (GraphViewResults* eventGraph : mByEventsGraphs)
                if (eventGraph)
                    eventGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);

        } else if (mGraphListTab->currentIndex() == 1) {
            for (GraphViewResults* phaseGraph : mByPhasesGraphs)
                if (phaseGraph)
                    phaseGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);

        } else if (mGraphListTab->currentIndex() == 2) {
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

#pragma mark X axis scale

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
        setScaleXSlide(zoomToSlider(mResultZoomX));

        forceXSpinSetValue = true;
        setScaleXSpin(mResultZoomX);

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
        setScaleXSlide(zoomToSlider(mResultZoomX));// the signal valueChange() must be not connected with a slot

        forceXSpinSetValue = true;
        setScaleXSpin(mResultZoomX);

        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);

        updateZoomEdit();

        updateGraphsZoomX();
    }
}

void ResultsView::setStudyPeriod()
{
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && (mCurrentVariable == GraphViewResults::eTheta
                                                                || mCurrentVariable == GraphViewResults::eTempo
                                                                || mCurrentVariable == GraphViewResults::eActivity)) {
        mResultCurrentMinX = mModel->mSettings.getTminFormated();
        mResultCurrentMaxX = mModel->mSettings.getTmaxFormated();
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
    setScaleXSlide(zoomToSlider(mResultZoomX));

    forceXSpinSetValue = true;
    setScaleXSpin(mResultZoomX);

    mRuler->setScaleDivision(Xscale);
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    
    updateZoomEdit();
    updateGraphsZoomX();
}

void ResultsView::updateZoomEdit()
{
    QLocale locale = QLocale();
    QFont adaptedFont (font());
    QFontMetricsF fm (font());
    qreal textSize = fm.width(locale.toString(mResultCurrentMinX,'f',0));
    if (textSize > (mCurrentXMinEdit->width() - 2. )) {
        const qreal fontRate = textSize / (mCurrentXMinEdit->width() - 2. );
        const qreal ptSiz = adaptedFont.pointSizeF() / fontRate;
        adaptedFont.setPointSizeF(ptSiz);
        mCurrentXMinEdit->setFont(adaptedFont);
    }
    else
        mCurrentXMinEdit->setFont(font());

    mCurrentXMinEdit->setText(locale.toString(mResultCurrentMinX,'f',0));

    textSize = fm.width(locale.toString(mResultCurrentMaxX,'f',0));
    if (textSize > (mCurrentXMaxEdit->width() - 2. )) {
        const qreal fontRate = textSize / (mCurrentXMaxEdit->width() - 2. );
        const qreal ptSiz = adaptedFont.pointSizeF() / fontRate;
        adaptedFont.setPointSizeF(ptSiz);
        mCurrentXMaxEdit->setFont(adaptedFont);
    }
    else
        mCurrentXMaxEdit->setFont(font());

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
    /* --------------------------------------------------
     *  Redraw the graphs
     * --------------------------------------------------*/

    if (mGraphListTab->currentIndex() == 0) {
        for (GraphViewResults* eventGraph : mByEventsGraphs)
            if (eventGraph) {
                eventGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                eventGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
            }
    }
    else if (mGraphListTab->currentIndex() == 1) {
        for (GraphViewResults* phaseGraph : mByPhasesGraphs)
            if (phaseGraph) {
                phaseGraph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                phaseGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
            }
   }
    else if (mGraphListTab->currentIndex() == 2) {
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
    const double min (2 * AppSettings::heigthUnit());//(70.);
    const double origin (GraphViewResults::mHeightForVisibleAxis);
    const double prop = double(value / 100.);
    mGraphHeight = int (min + prop * (origin - min));
    updateGraphsLayout();
}


// Display options
void ResultsView::updateGraphsFont()
{
    const QFontMetrics gfm (font());

#ifdef Q_OS_MAC
    int heightText = int (1.5 * gfm.height());
#else
    int heightText = gfm.height();
#endif
    qreal graduationSize = heightText /3;

    switch(mGraphTypeTabs->currentIndex()) {
        case 0: // Posterior Distrib.
                {
                  //  const int marg = qMax(gfm.width(stringForLocal(mResultCurrentMinX))/ 2., gfm.width(stringForLocal(mResultCurrentMaxX))/ 2.);
                    mMarginLeft =  1.5 * gfm.boundingRect(stringForLocal( 5 * mModel->mSettings.getTminFormated())).width()/ 2. +  2*graduationSize;
                    mMarginRight = 1.5 * gfm.boundingRect(stringForLocal(5 * mModel->mSettings.getTmaxFormated())).width()/ 2.;
                  }
         break;

        case 1: // History Plot
        case 2:// Acceptance Rate
                 {
                        const int maxIter = 1+ mModel->mChains.at(0).mNumBurnIter + (mModel->mChains.at(0).mBatchIndex * mModel->mChains.at(0).mNumBatchIter) + mModel->mChains.at(0).mNumRunIter / mModel->mChains.at(0).mThinningInterval;
                        const int marg = gfm.boundingRect(stringForLocal(maxIter)).width();
                        mMarginLeft =  marg + 2*graduationSize ;
                        mMarginRight = marg;
                    }
        break;

         case 3:// Autocorrelation
                    {
                       const int marg = qMax(gfm.boundingRect(stringForLocal(40)).width()/ 2, gfm.boundingRect(stringForGraph(100)).width());
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

    for (GraphViewResults* phaseGraph : mByPhasesGraphs) {
        phaseGraph->setMarginLeft(mMarginLeft);
        phaseGraph->setMarginRight(mMarginRight);
        phaseGraph->setGraphFont(font());
    }

    for (GraphViewResults* eventGraph : mByEventsGraphs) {
        eventGraph->setMarginLeft(mMarginLeft);
        eventGraph->setMarginRight(mMarginRight);
        eventGraph->setGraphFont(font());
    }

    for (GraphViewResults* tempoGraph : mByTempoGraphs) {
        tempoGraph->setMarginLeft(mMarginLeft);
        tempoGraph->setMarginRight(mMarginRight);
        tempoGraph->setGraphFont(font());
    }
}


/**
 * @brief ResultsView::updateFont only on graph
 */
void ResultsView::updateFont()
{
    bool ok = false;
    const QFont& currentFont = font();
    QFont font(QFontDialog::getFont(&ok, currentFont, this));
    if (ok) {
        setFont(font);
        updateGraphsFont();
        mFontBut->setText(font.family() + ", " + QString::number(font.pointSizeF()));
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

#pragma mark Graph selection and export

void ResultsView::saveGraphData()
{
    QList<GraphViewResults*> graphs = currentGraphs(true);
    for (auto && graph : graphs){
        graph->saveGraphData();
    }
}

void ResultsView::resultsToClipboard()
{
    QString resultText;
    QList<GraphViewResults*> graphs = currentGraphs(true);
    for (auto && graph : graphs){
        resultText += graph->getTextAreaToPlainText();
    }
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(resultText);
}

void ResultsView::imageToClipboard()
{
    QList<GraphViewResults*> graphs = currentGraphs(true);
    
    if(!graphs.isEmpty())
    {
        GraphViewResults* firstGraph = graphs.at(0);
        
        const int versionHeight = 20;
        short pr = short(AppSettings::mPixelRatio);

        QImage image(firstGraph->width() * pr, (graphs.size() * firstGraph->height() + versionHeight) * pr , QImage::Format_ARGB32_Premultiplied);

        image.setDevicePixelRatio(pr);
        image.fill(Qt::transparent);

        QPainter p;
        p.begin(&image);
        p.setRenderHint(QPainter::Antialiasing);

        QPoint ptStart (0, 0);
        for (auto &&graph : graphs)
        {
            graph->showSelectedRect(false);
            graph->render(&p, ptStart, QRegion(0, 0, graph->width(), graph->height()));
            ptStart = QPoint(0, ptStart.y() + graph->height());
            graph->showSelectedRect(true);
        }
        
        p.setPen(Qt::black);
        p.setBrush(Qt::white);
        p.fillRect(0, ptStart.y(), firstGraph->width(), versionHeight, Qt::white);
        p.drawText(0, ptStart.y(), firstGraph->width(), versionHeight,
                   Qt::AlignCenter,
                   qApp->applicationName() + " " + qApp->applicationVersion());


        p.end();

        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setImage(image);
    }
}

void ResultsView::saveAsImage()
{
    QList<GraphViewResults*> graphs = currentGraphs(true);
    if (graphs.isEmpty()){
        return;
    }
        
    // --------------------------------------------------
    //  Ask for a file name and type (SVG or Image)
    // --------------------------------------------------
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
        tr("Save graph image as..."),
        MainWindow::getInstance()->getCurrentPath(),
        QObject::tr("Image (*.png);;Photo (*.jpg);;Scalable Vector Graphics (*.svg)"));

    if (!fileName.isEmpty())
    {
        // --------------------------------------------------
        //  Get the file extension
        // --------------------------------------------------
        QFileInfo fileInfo;
        fileInfo = QFileInfo(fileName);
        QString fileExtension = fileInfo.suffix();
        bool asSvg = fileName.endsWith(".svg");
        
        
        const int heightText (2 * qApp->fontMetrics().height());
        const QString versionStr = qApp->applicationName() + " " + qApp->applicationVersion();
        
        // --- if png
        //const int versionHeight (20);
        if (!asSvg) {

            const short pr = short (AppSettings::mPixelRatio);

            QImage image (graphs.first()->width() * pr, (graphs.size() * graphs.first()->height() + heightText) * pr , QImage::Format_ARGB32_Premultiplied); //Format_ARGB32_Premultiplied //Format_ARGB32

            if (image.isNull() )
                qDebug()<< " image width = 0";

            image.setDevicePixelRatio(pr);
            image.fill(Qt::transparent);

            QPainter p;
            p.begin(&image);
            p.setRenderHint(QPainter::Antialiasing);
            p.setFont(qApp->font());

            QPoint ptStart (0, 0);
            for (auto &&graph : graphs) {
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
            p.fillRect(0, ptStart.y(), graphs.first()->width(), heightText, Qt::white);
            p.drawText(0, ptStart.y(), graphs.first()->width(), heightText,
                       Qt::AlignCenter,
                       versionStr);
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
            const int wGraph = graphs.first()->width();
            const int hGraph = graphs.size() * graphs.first()->height();
            QRect rTotal ( 0, 0, wGraph, hGraph + heightText );
            // Set SVG Generator
            
            QSvgGenerator svgGen;
            svgGen.setFileName(fileName);
            //svgGen.setSize(rTotal.size());
            svgGen.setViewBox(rTotal);
            svgGen.setTitle(versionStr);
            svgGen.setDescription(fileName);

            QPainter painter;
            painter.begin(&svgGen);
            //font().wordSpacing();

            QPoint ptStart (0, 0);
            for (auto &&graph : graphs) {
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
            painter.drawText(0, ptStart.y(), wGraph, heightText,
                             Qt::AlignCenter,
                             versionStr);

            painter.end();

        }
    // end if not Empty filename
    }
}

bool ResultsView::hasSelectedGraphs()
{
    return (currentGraphs(true).size() > 0);
}

QList<GraphViewResults*> ResultsView::currentGraphs(bool onlySelected)
{
    QList<GraphViewResults*> graphs;
    
    if(mGraphListTab->currentIndex() == 0)
    {
        for(auto &&graph : mByEventsGraphs)
        {
            if (onlySelected || graph->isSelected())
            {
                graphs.append(graph);
            }
        }
    }
    else if (mGraphListTab->currentIndex() == 1)
    {
        for(auto &&graph : mByPhasesGraphs)
        {
            if (onlySelected || graph->isSelected())
            {
                graphs.append(graph);
            }
        }
    }
    else if (mGraphListTab->currentIndex() == 2)
    {
        for(auto &&graph : mByTempoGraphs)
        {
            if (onlySelected || graph->isSelected())
            {
                graphs.append(graph);
            }
        }
    }
    else if (mGraphListTab->currentIndex() == 3)
    {
        for(auto &&graph : mByCurveGraphs)
        {
            if (onlySelected || graph->isSelected())
            {
                graphs.append(graph);
            }
        }
    }
    
    return graphs;
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
                output<<"<!DOCTYPE html>"<< endl;
                output<<"<html>"<< endl;
                output<<"<body>"<< endl;

                output<<"<h2>"<< version << "</h2>" << endl;
                output<<"<h2>"<< projectName+ "</h2>" << endl;
                output<<"<hr>";
                output<<mModel->getModelLog();

                output<<"</body>"<< endl;
                output<<"</html>"<< endl;
            }
            file.close();

            file.setFileName(dirPath + "/Log_MCMC_Initialization.html");

            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<"<!DOCTYPE html>"<< endl;
                output<<"<html>"<< endl;
                output<<"<body>"<< endl;

                output<<"<h2>"<< version << "</h2>" << endl;
                output<<"<h2>"<< projectName+ "</h2>" << endl;
                output<<"<hr>";
                output<<mModel->getMCMCLog();

                output<<"</body>"<< endl;
                output<<"</html>"<< endl;
            }
            file.close();

            file.setFileName(dirPath + "/Log_Posterior_Distrib_Stats.html");

            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<"<!DOCTYPE html>"<< endl;
                output<<"<html>"<< endl;
                output<<"<body>"<< endl;

                output<<"<h2>"<< version << "</h2>" << endl;
                output<<"<h2>"<< projectName+ "</h2>" << endl;
                output<<"<hr>";
                output<<mModel->getResultsLog();

                output<<"</body>"<< endl;
                output<<"</html>"<< endl;
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
    bool printAxis = (mGraphHeight < GraphViewResults::mHeightForVisibleAxis);

    QWidget* curWid (nullptr);

    type_data max;

    if (mGraphListStack->currentWidget() == mEventsScrollArea) {
        curWid = mEventsScrollArea->widget();
        curWid->setFont(mByEventsGraphs.at(0)->font());
        max = mByEventsGraphs.at(0)->getGraph()->maximumX();
    }

    else if (mGraphListStack->currentWidget() == mPhasesScrollArea) {
        curWid = mPhasesScrollArea->widget();
        curWid->setFont(mByPhasesGraphs.at(0)->font());
        max = mByPhasesGraphs.at(0)->getGraph()->maximumX();
    }

    else if (mGraphListStack->currentWidget() == mTempoScrollArea) {
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
    QFontMetrics fm (font());
    int axeHeight (int (fm.ascent() * 2.2)); // equal MarginBottom()

    int legendHeight (int (fm.ascent() *2));
    if (printAxis) {
        curWid->setFixedHeight(curWid->height() + axeHeight + legendHeight );

        axisWidget = new AxisWidget(nullptr, curWid);
        axisWidget->mMarginLeft = mMarginLeft;
        axisWidget->mMarginRight = mMarginRight;
        axisWidget->setScaleDivision(mMajorScale, mMinorCountScale);

        if (mStatCheck->isChecked()) {
            axisWidget->setGeometry(0, curWid->height() - axeHeight, int (curWid->width()*2./3.), axeHeight);
            axisWidget->updateValues(int (curWid->width()*2./3. - axisWidget->mMarginLeft - axisWidget->mMarginRight), 50, mResultCurrentMinX, mResultCurrentMaxX);

        } else {
            axisWidget->setGeometry(0, curWid->height() - axeHeight, curWid->width(), axeHeight);
            axisWidget->updateValues(int (curWid->width() - axisWidget->mMarginLeft - axisWidget->mMarginRight), 50, mResultCurrentMinX, mResultCurrentMaxX);
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

        axisLegend->setFont(font());
        QFontMetrics fm(font());
        if (mStatCheck->isChecked())
            axisLegend->setGeometry(fm.boundingRect(legend).width(), curWid->height() - axeHeight - legendHeight, int (curWid->width()*2./3. - 10), legendHeight);
        else
            axisLegend->setGeometry(int (curWid->width() - fm.boundingRect(legend).width() - mMarginRight), curWid->height() - axeHeight - legendHeight, fm.boundingRect(legend).width() , legendHeight);

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

    for ( auto &&p : mModel->mPhases ) {
        std::sort(p->mEvents.begin(), p->mEvents.end(), sortEvents);
    }

    updateResults(mModel);
}


bool ResultsView::isChronocurve() const
{
    return (this->modelChronocurve() != nullptr);
}

ModelChronocurve* ResultsView::modelChronocurve() const
{
    return dynamic_cast<ModelChronocurve*>(mModel);
}
